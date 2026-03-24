/*
 * cliargs.c — implementation of the cliargs argument-parsing library.
 *
 * Compile with: gcc -Wall -Wextra -std=c99 -pedantic -c cliargs.c
 */

#include "cliargs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ─────────────────────────────────────────────────────────────────── */
/* Limits                                                               */
/* ─────────────────────────────────────────────────────────────────── */

#define MAX_TOKENS      512
#define MAX_REGISTERED   64
#define MAX_POSITIONAL   32
#define MAX_REMAINING   128
#define MAX_ARGALL       64
#define MAX_ERROR_BUF   256
#define MAX_ENV_NAME    128
#define HELP_COL_WIDTH   28

/* ─────────────────────────────────────────────────────────────────── */
/* Internal data structures                                             */
/* ─────────────────────────────────────────────────────────────────── */

#define TOKEN_FLAG   0
#define TOKEN_KEYVAL 1

typedef struct {
    int   type;       /* TOKEN_FLAG or TOKEN_KEYVAL              */
    char *name;       /* long name without "--", or NULL         */
    char  shorthand;  /* short char, or '\0'                     */
    char *value;      /* non-NULL only for TOKEN_KEYVAL          */
} ParsedToken;

typedef struct {
    char *name;
    char  shorthand;
    char *helpText;
} RegisteredArg;

/* ─────────────────────────────────────────────────────────────────── */
/* Global state (all static — not visible outside this translation unit)*/
/* ─────────────────────────────────────────────────────────────────── */

static ParsedToken   s_tokens[MAX_TOKENS];
static int           s_token_count;

static RegisteredArg s_registered[MAX_REGISTERED];
static int           s_registered_count;

static char *s_positionals[MAX_POSITIONAL];
static int   s_positional_count;

static char *s_remaining_arr[MAX_REMAINING];
static int   s_remaining_count;
static int   s_has_remaining;   /* 1 if "--" separator was seen */

static char *s_subcommand;
static char *s_program_name;
static char *s_description;
static char *s_version;

static int  s_strict;
static int  s_env_fallback;

static char s_error_buf[MAX_ERROR_BUF];
static int  s_has_error;

static char *s_argall_buf[MAX_ARGALL]; /* scratch buffer for cliargsArgAll */

/* ─────────────────────────────────────────────────────────────────── */
/* Internal helpers                                                     */
/* ─────────────────────────────────────────────────────────────────── */

/* Returns 1 if token t matches a query (name, shorthand). */
static int token_matches(const ParsedToken *t,
                         const char *name, char shorthand)
{
    if (shorthand != '\0' && t->shorthand != '\0' &&
        t->shorthand == shorthand)
        return 1;
    if (name != NULL && t->name != NULL &&
        strcmp(t->name, name) == 0)
        return 1;
    return 0;
}

/* Returns 1 if (name, shorthand) matches any registered option. */
static int is_registered_opt(const char *name, char shorthand)
{
    int i;
    for (i = 0; i < s_registered_count; i++) {
        if (shorthand != '\0' &&
            s_registered[i].shorthand == shorthand)
            return 1;
        if (name != NULL && s_registered[i].name != NULL &&
            strcmp(s_registered[i].name, name) == 0)
            return 1;
    }
    return 0;
}

/*
 * Returns 1 when a token looks like a value (not a bare word / subcommand).
 *
 * Heuristic: the token is a value if it starts with a digit, or contains
 * at least one of '.', '/', '=', ':'.  Plain command-words like "serve"
 * or "deploy" do not match and are left for subcommand / positional
 * collection rather than being consumed as a flag's value.
 */
static int looks_like_value(const char *token)
{
    int i;
    if (token[0] >= '0' && token[0] <= '9')
        return 1;
    for (i = 0; token[i] != '\0'; i++) {
        char c = token[i];
        if (c == '.' || c == '/' || c == '=' || c == ':' || c == '\\')
            return 1;
    }
    return 0;
}

/*
 * Called after adding each parsed token.
 * Records the first validation error and, in strict mode, exits.
 */
static void validate_token(const char *name, char shorthand)
{
    /* Skip if no registration has been done — nothing to validate */
    if (s_registered_count == 0)
        return;

    /* Built-ins are always valid */
    if (shorthand == 'h')
        return;
    if (name != NULL &&
        (strcmp(name, "help") == 0 || strcmp(name, "version") == 0))
        return;

    if (!is_registered_opt(name, shorthand)) {
        if (!s_has_error) {
            if (name != NULL)
                snprintf(s_error_buf, sizeof(s_error_buf),
                         "Unrecognized argument: --%s", name);
            else
                snprintf(s_error_buf, sizeof(s_error_buf),
                         "Unrecognized argument: -%c", shorthand);
            s_has_error = 1;
        }
        if (s_strict) {
            fprintf(stderr, "%s\n", s_error_buf);
            cliargsPrintHelp();
            exit(1);
        }
    }
}

/* Append a token to the internal list. */
static void add_token(int type, char *name, char shorthand, char *value)
{
    if (s_token_count >= MAX_TOKENS)
        return;
    s_tokens[s_token_count].type      = type;
    s_tokens[s_token_count].name      = name;
    s_tokens[s_token_count].shorthand = shorthand;
    s_tokens[s_token_count].value     = value;
    s_token_count++;

    validate_token(name, shorthand);
}

/*
 * Build the env-var name for a long arg name:
 *   "output-file" → "CLIARGS_OUTPUT_FILE"
 * Returns pointer to a static buffer.
 */
static char *env_name_for(const char *name)
{
    static char buf[MAX_ENV_NAME];
    const char *prefix = "CLIARGS_";
    int plen = (int)strlen(prefix);
    int i;

    if (plen >= MAX_ENV_NAME)
        return NULL;
    memcpy(buf, prefix, (size_t)plen);

    for (i = 0; name[i] != '\0' && plen + i < MAX_ENV_NAME - 1; i++) {
        buf[plen + i] = (name[i] == '-')
                        ? '_'
                        : (char)toupper((unsigned char)name[i]);
    }
    buf[plen + i] = '\0';
    return buf;
}

/* ─────────────────────────────────────────────────────────────────── */
/* Public API — Setup & Parsing                                         */
/* ─────────────────────────────────────────────────────────────────── */

void cliargsReset(void)
{
    int i;

    s_token_count      = 0;
    s_registered_count = 0;
    s_positional_count = 0;
    s_remaining_count  = 0;
    s_has_remaining    = 0;
    s_subcommand       = NULL;
    s_program_name     = NULL;
    s_description      = NULL;
    s_version          = NULL;
    s_strict           = 0;
    s_env_fallback     = 0;
    s_has_error        = 0;
    s_error_buf[0]     = '\0';

    for (i = 0; i < MAX_TOKENS; i++) {
        s_tokens[i].type      = 0;
        s_tokens[i].name      = NULL;
        s_tokens[i].shorthand = '\0';
        s_tokens[i].value     = NULL;
    }
    for (i = 0; i < MAX_REGISTERED; i++) {
        s_registered[i].name      = NULL;
        s_registered[i].shorthand = '\0';
        s_registered[i].helpText  = NULL;
    }
    for (i = 0; i < MAX_POSITIONAL;  i++) s_positionals[i]   = NULL;
    for (i = 0; i < MAX_REMAINING;   i++) s_remaining_arr[i] = NULL;
    for (i = 0; i < MAX_ARGALL;      i++) s_argall_buf[i]    = NULL;
}

void cliargsSetDescription(char *desc)
{
    s_description = desc;
}

void cliargsSetVersion(char *version)
{
    s_version = version;
}

void cliargsRegister(char *name, char shorthand, char *helpText)
{
    if (s_registered_count >= MAX_REGISTERED)
        return;
    s_registered[s_registered_count].name      = name;
    s_registered[s_registered_count].shorthand = shorthand;
    s_registered[s_registered_count].helpText  = helpText;
    s_registered_count++;
}

void cliargsStrict(void)
{
    s_strict = 1;
}

void cliargsEnvFallback(bool enabled)
{
    s_env_fallback = enabled ? 1 : 0;
}

void cliargsParse(int argc, char **argv)
{
    int i, j;
    int seen_subcommand = 0;

    s_program_name = argv[0];

    i = 1;
    while (i < argc) {
        char *arg = argv[i];

        /* ── passthrough separator ── */
        if (strcmp(arg, "--") == 0) {
            s_has_remaining = 1;
            for (j = i + 1;
                 j < argc && s_remaining_count < MAX_REMAINING;
                 j++) {
                s_remaining_arr[s_remaining_count++] = argv[j];
            }
            break;
        }

        /* ── long option: --name ── */
        if (arg[0] == '-' && arg[1] == '-' && arg[2] != '\0') {
            char *name = arg + 2;

            if (strcmp(name, "help") == 0) {
                cliargsPrintHelp();
                exit(0);
            }
            if (strcmp(name, "version") == 0) {
                if (s_version)
                    printf("%s\n", s_version);
                exit(0);
            }

            /* next token is the value if it doesn't start with '-'
             * AND looks like a value (not a bare command word) */
            if (i + 1 < argc && argv[i + 1][0] != '-' &&
                looks_like_value(argv[i + 1])) {
                add_token(TOKEN_KEYVAL, name, '\0', argv[i + 1]);
                i += 2;
            } else {
                add_token(TOKEN_FLAG, name, '\0', NULL);
                i++;
            }
            continue;
        }

        /* ── short option(s): -x or -abc ── */
        if (arg[0] == '-' && arg[1] != '\0' && arg[1] != '-') {
            int len = (int)strlen(arg);

            if (len == 2) {
                char c = arg[1];

                if (c == 'h') {
                    cliargsPrintHelp();
                    exit(0);
                }

                if (i + 1 < argc && argv[i + 1][0] != '-' &&
                    looks_like_value(argv[i + 1])) {
                    add_token(TOKEN_KEYVAL, NULL, c, argv[i + 1]);
                    i += 2;
                } else {
                    add_token(TOKEN_FLAG, NULL, c, NULL);
                    i++;
                }
            } else {
                /* composed flags: -vvv / -abc */
                for (j = 1; j < len; j++) {
                    char c = arg[j];
                    if (c == 'h') {
                        cliargsPrintHelp();
                        exit(0);
                    }
                    add_token(TOKEN_FLAG, NULL, c, NULL);
                }
                i++;
            }
            continue;
        }

        /* ── bare word: subcommand or positional ── */
        if (!seen_subcommand) {
            s_subcommand    = arg;
            seen_subcommand = 1;
        } else if (s_positional_count < MAX_POSITIONAL) {
            s_positionals[s_positional_count++] = arg;
        }
        i++;
    }
}

/* ─────────────────────────────────────────────────────────────────── */
/* Public API — Flags                                                   */
/* ─────────────────────────────────────────────────────────────────── */

bool cliargsFlag(char *name, char shorthand)
{
    int i;
    for (i = 0; i < s_token_count; i++) {
        if (s_tokens[i].type == TOKEN_FLAG &&
            token_matches(&s_tokens[i], name, shorthand))
            return true;
    }
    return false;
}

int cliargsCount(char *name, char shorthand)
{
    int i, n = 0;
    for (i = 0; i < s_token_count; i++) {
        if (s_tokens[i].type == TOKEN_FLAG &&
            token_matches(&s_tokens[i], name, shorthand))
            n++;
    }
    return n;
}

/* ─────────────────────────────────────────────────────────────────── */
/* Public API — Arguments (Key-Value)                                   */
/* ─────────────────────────────────────────────────────────────────── */

char *cliargsArg(char *name, char shorthand)
{
    int i;
    for (i = 0; i < s_token_count; i++) {
        if (s_tokens[i].type == TOKEN_KEYVAL &&
            token_matches(&s_tokens[i], name, shorthand))
            return s_tokens[i].value;
    }
    /* environment fallback */
    if (s_env_fallback && name != NULL) {
        char *ev = env_name_for(name);
        if (ev != NULL) {
            char *val = getenv(ev);
            if (val != NULL)
                return val;
        }
    }
    return NULL;
}

char *cliargsArgDefault(char *name, char shorthand, char *fallback)
{
    char *v = cliargsArg(name, shorthand);
    return (v != NULL) ? v : fallback;
}

int cliargsArgInt(char *name, char shorthand)
{
    char *v = cliargsArg(name, shorthand);
    return (v != NULL) ? atoi(v) : 0;
}

int cliargsArgIntDefault(char *name, char shorthand, int fallback)
{
    char *v = cliargsArg(name, shorthand);
    return (v != NULL) ? atoi(v) : fallback;
}

float cliargsArgFloat(char *name, char shorthand)
{
    char *v = cliargsArg(name, shorthand);
    return (v != NULL) ? (float)atof(v) : 0.0f;
}

float cliargsArgFloatDefault(char *name, char shorthand, float fallback)
{
    char *v = cliargsArg(name, shorthand);
    return (v != NULL) ? (float)atof(v) : fallback;
}

char **cliargsArgAll(char *name, char shorthand, int *count)
{
    int i, n = 0;
    for (i = 0; i < s_token_count && n < MAX_ARGALL; i++) {
        if (s_tokens[i].type == TOKEN_KEYVAL &&
            token_matches(&s_tokens[i], name, shorthand))
            s_argall_buf[n++] = s_tokens[i].value;
    }
    *count = n;
    return (n > 0) ? s_argall_buf : NULL;
}

/* ─────────────────────────────────────────────────────────────────── */
/* Public API — Positional Arguments                                    */
/* ─────────────────────────────────────────────────────────────────── */

char *cliargsPos(int index)
{
    if (index < 0 || index >= s_positional_count)
        return NULL;
    return s_positionals[index];
}

/* ─────────────────────────────────────────────────────────────────── */
/* Public API — Subcommands                                             */
/* ─────────────────────────────────────────────────────────────────── */

char *cliargsSubcommand(void)
{
    return s_subcommand;
}

/* ─────────────────────────────────────────────────────────────────── */
/* Public API — Passthrough / Remaining                                 */
/* ─────────────────────────────────────────────────────────────────── */

char **cliargsRemaining(int *count)
{
    *count = s_remaining_count;
    if (!s_has_remaining)
        return NULL;
    return s_remaining_arr;
}

/* ─────────────────────────────────────────────────────────────────── */
/* Public API — Validation & Error Handling                             */
/* ─────────────────────────────────────────────────────────────────── */

bool cliargsValid(void)
{
    if (s_registered_count == 0)
        return true;
    return !s_has_error;
}

char *cliargsError(void)
{
    if (!s_has_error)
        return NULL;
    return s_error_buf;
}

/* ─────────────────────────────────────────────────────────────────── */
/* Public API — Help Generation                                         */
/* ─────────────────────────────────────────────────────────────────── */

void cliargsPrintHelp(void)
{
    int i, j, col, len;
    const char *prog = s_program_name ? s_program_name : "program";
    const char *slash = strrchr(prog, '/');
    if (slash)
        prog = slash + 1;

    /* description */
    if (s_description)
        printf("%s\n\n", s_description);

    printf("Usage: %s [options] [subcommand]\n\n", prog);
    printf("Options:\n");

    /* Compute dynamic column width */
    col = HELP_COL_WIDTH;
    for (i = 0; i < s_registered_count; i++) {
        int w = 0;
        const RegisteredArg *ra = &s_registered[i];
        if (ra->shorthand != '\0')
            w = 6; /* "  -X, " */
        else
            w = 6; /* "      " */
        w += 2;    /* "--" */
        if (ra->name)
            w += (int)strlen(ra->name);
        w += 8;    /* " <value>" */
        if (w > col)
            col = w;
    }
    /* ensure built-in entries fit */
    if (col < 17) col = 17; /* "      --version" = 15, + 2 */

    /* print registered args */
    for (i = 0; i < s_registered_count; i++) {
        char opt[128];
        const RegisteredArg *ra = &s_registered[i];

        if (ra->shorthand != '\0' && ra->name != NULL)
            snprintf(opt, sizeof(opt), "  -%c, --%s <value>",
                     ra->shorthand, ra->name);
        else if (ra->shorthand != '\0')
            snprintf(opt, sizeof(opt), "  -%c <value>", ra->shorthand);
        else if (ra->name != NULL)
            snprintf(opt, sizeof(opt), "      --%s <value>", ra->name);
        else
            continue;

        len = (int)strlen(opt);
        printf("%s", opt);
        for (j = len; j < col; j++)
            putchar(' ');
        if (len >= col)
            putchar(' ');
        if (ra->helpText)
            printf("%s", ra->helpText);
        putchar('\n');
    }

    /* built-in --help */
    {
        const char *s = "      --help";
        len = (int)strlen(s);
        printf("%s", s);
        for (j = len; j < col; j++) putchar(' ');
        if (len >= col) putchar(' ');
        printf("Show this help message\n");
    }

    /* built-in --version */
    {
        const char *s = "      --version";
        len = (int)strlen(s);
        printf("%s", s);
        for (j = len; j < col; j++) putchar(' ');
        if (len >= col) putchar(' ');
        printf("Show version information\n");
    }
}

/* ─────────────────────────────────────────────────────────────────── */
/* Public API — Utility                                                 */
/* ─────────────────────────────────────────────────────────────────── */

bool cliargsIsFlagSet(char *name, char shorthand)
{
    /* Checks only explicit command-line tokens, no env / defaults */
    return cliargsFlag(name, shorthand);
}
