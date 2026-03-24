/*
 * test_subcommands.c — tests for cliargsSubcommand
 */

#include "../cliargs.h"

#include <stdio.h>
#include <string.h>

static int passed = 0;
static int failed = 0;

#define TEST(desc, expr) do { \
    if (expr) { \
        passed++; \
    } else { \
        fprintf(stderr, "  FAIL: %s (line %d)\n", (desc), __LINE__); \
        failed++; \
    } \
} while (0)

/* ── subcommand detected as first bare word ── */
static void test_subcommand_detected(void)
{
    char *argv[] = {"prog", "serve", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    char *sc = cliargsSubcommand();
    TEST("subcommand detected", sc != NULL && strcmp(sc, "serve") == 0);
}

/* ── NULL returned when no subcommand ── */
static void test_no_subcommand(void)
{
    char *argv[] = {"prog", "--verbose", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    TEST("no subcommand returns NULL", cliargsSubcommand() == NULL);
}

/* ── NULL when no args at all ── */
static void test_no_args(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    TEST("no args: subcommand NULL", cliargsSubcommand() == NULL);
}

/* ── subcommand not included in positional args ── */
static void test_subcommand_not_positional(void)
{
    char *argv[] = {"prog", "build", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    TEST("subcommand not in positionals",  cliargsPos(0) == NULL);
    TEST("subcommand string correct",
         cliargsSubcommand() != NULL &&
         strcmp(cliargsSubcommand(), "build") == 0);
}

/* ── args after subcommand parsed normally ── */
static void test_args_after_subcommand(void)
{
    char *argv[] = {"prog", "serve", "--port", "9000", NULL};
    cliargsReset();
    cliargsParse(4, argv);
    TEST("subcommand == serve",
         cliargsSubcommand() != NULL &&
         strcmp(cliargsSubcommand(), "serve") == 0);
    TEST("--port after subcommand parsed",
         cliargsArgInt("port", 'p') == 9000);
}

/* ── flags before subcommand still work ── */
static void test_flags_before_subcommand(void)
{
    char *argv[] = {"prog", "--verbose", "deploy", NULL};
    cliargsReset();
    cliargsParse(3, argv);
    TEST("flag before subcommand", cliargsFlag("verbose", 'v') == true);
    TEST("subcommand after flag",
         cliargsSubcommand() != NULL &&
         strcmp(cliargsSubcommand(), "deploy") == 0);
}

/* ── positionals after subcommand ── */
static void test_positionals_after_subcommand(void)
{
    char *argv[] = {"prog", "run", "file1.txt", "file2.txt", NULL};
    cliargsReset();
    cliargsParse(4, argv);
    TEST("subcommand == run",
         cliargsSubcommand() != NULL &&
         strcmp(cliargsSubcommand(), "run") == 0);
    TEST("positional[0] == file1.txt",
         cliargsPos(0) != NULL &&
         strcmp(cliargsPos(0), "file1.txt") == 0);
    TEST("positional[1] == file2.txt",
         cliargsPos(1) != NULL &&
         strcmp(cliargsPos(1), "file2.txt") == 0);
}

/* ── first bare word is subcommand even if flags come first ── */
static void test_first_bare_word_is_subcommand(void)
{
    char *argv[] = {"prog", "-v", "migrate", "extra", NULL};
    cliargsReset();
    cliargsParse(4, argv);
    TEST("first bare word after flags is subcommand",
         cliargsSubcommand() != NULL &&
         strcmp(cliargsSubcommand(), "migrate") == 0);
    TEST("subsequent bare word is positional",
         cliargsPos(0) != NULL &&
         strcmp(cliargsPos(0), "extra") == 0);
}

int main(void)
{
    printf("Running test_subcommands...\n");

    test_subcommand_detected();
    test_no_subcommand();
    test_no_args();
    test_subcommand_not_positional();
    test_args_after_subcommand();
    test_flags_before_subcommand();
    test_positionals_after_subcommand();
    test_first_bare_word_is_subcommand();

    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
