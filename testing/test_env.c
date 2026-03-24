/*
 * test_env.c — tests for cliargsEnvFallback
 */

#include "../cliargs.h"

#include <stdio.h>
#include <stdlib.h>
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

/* ── env fallback returns value when arg not on command line ── */
static void test_env_fallback_used(void)
{
    setenv("CLIARGS_PORT", "4242", 1);

    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsEnvFallback(true);
    cliargsParse(1, argv);

    char *v = cliargsArg("port", 'p');
    TEST("env fallback returns env value",
         v != NULL && strcmp(v, "4242") == 0);

    unsetenv("CLIARGS_PORT");
}

/* ── env fallback not used when arg is on command line ── */
static void test_env_fallback_not_used_when_present(void)
{
    setenv("CLIARGS_PORT", "9999", 1);

    char *argv[] = {"prog", "--port", "1234", NULL};
    cliargsReset();
    cliargsEnvFallback(true);
    cliargsParse(3, argv);

    char *v = cliargsArg("port", 'p');
    TEST("command-line arg overrides env fallback",
         v != NULL && strcmp(v, "1234") == 0);

    unsetenv("CLIARGS_PORT");
}

/* ── env fallback inactive when disabled ── */
static void test_env_fallback_disabled(void)
{
    setenv("CLIARGS_PORT", "7777", 1);

    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsEnvFallback(false);
    cliargsParse(1, argv);

    char *v = cliargsArg("port", 'p');
    TEST("env fallback disabled returns NULL", v == NULL);

    unsetenv("CLIARGS_PORT");
}

/* ── env fallback inactive by default ── */
static void test_env_fallback_default_off(void)
{
    setenv("CLIARGS_PORT", "5555", 1);

    char *argv[] = {"prog", NULL};
    cliargsReset();
    /* no cliargsEnvFallback call */
    cliargsParse(1, argv);

    char *v = cliargsArg("port", 'p');
    TEST("env fallback off by default returns NULL", v == NULL);

    unsetenv("CLIARGS_PORT");
}

/* ── correct env var name: dashes to underscores, uppercase ── */
static void test_env_name_derivation(void)
{
    setenv("CLIARGS_OUTPUT_FILE", "derived.txt", 1);

    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsEnvFallback(true);
    cliargsParse(1, argv);

    char *v = cliargsArg("output-file", 0);
    TEST("dashes to underscores in env name",
         v != NULL && strcmp(v, "derived.txt") == 0);

    unsetenv("CLIARGS_OUTPUT_FILE");
}

/* ── env fallback with ArgDefault ── */
static void test_env_fallback_with_default(void)
{
    setenv("CLIARGS_HOST", "localhost", 1);

    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsEnvFallback(true);
    cliargsParse(1, argv);

    /* env var is set, so fallback string should NOT be returned */
    char *v = cliargsArgDefault("host", 0, "127.0.0.1");
    TEST("env value overrides ArgDefault fallback",
         v != NULL && strcmp(v, "localhost") == 0);

    unsetenv("CLIARGS_HOST");
}

/* ── env fallback with ArgInt ── */
static void test_env_fallback_int(void)
{
    setenv("CLIARGS_PORT", "8080", 1);

    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsEnvFallback(true);
    cliargsParse(1, argv);

    TEST("env fallback ArgInt returns 8080",
         cliargsArgInt("port", 'p') == 8080);

    unsetenv("CLIARGS_PORT");
}

/* ── env var absent means NULL returned ── */
static void test_env_var_absent(void)
{
    unsetenv("CLIARGS_MISSING");

    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsEnvFallback(true);
    cliargsParse(1, argv);

    char *v = cliargsArg("missing", 0);
    TEST("absent env var returns NULL", v == NULL);
}

int main(void)
{
    printf("Running test_env...\n");

    test_env_fallback_used();
    test_env_fallback_not_used_when_present();
    test_env_fallback_disabled();
    test_env_fallback_default_off();
    test_env_name_derivation();
    test_env_fallback_with_default();
    test_env_fallback_int();
    test_env_var_absent();

    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
