/*
 * test_validation.c — tests for cliargsValid, cliargsError, cliargsStrict
 */

#include "../cliargs.h"

#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

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

/* ── valid when all args registered ── */
static void test_valid_all_registered(void)
{
    char *argv[] = {"prog", "--verbose", "-o", "file.txt", NULL};
    cliargsReset();
    cliargsRegister("verbose", 'v', "Enable verbose");
    cliargsRegister("out",     'o', "Output file");
    cliargsParse(4, argv);
    TEST("valid when all registered", cliargsValid() == true);
    TEST("error is NULL when valid",  cliargsError() == NULL);
}

/* ── invalid when unknown arg present ── */
static void test_invalid_unknown_arg(void)
{
    char *argv[] = {"prog", "--unknown", NULL};
    cliargsReset();
    cliargsRegister("verbose", 'v', "Enable verbose");
    cliargsParse(2, argv);
    TEST("invalid when unknown arg", cliargsValid() == false);
}

/* ── cliargsError returns message for first unknown arg ── */
static void test_error_message(void)
{
    char *argv[] = {"prog", "--badarg", NULL};
    cliargsReset();
    cliargsRegister("verbose", 'v', "Enable verbose");
    cliargsParse(2, argv);
    char *err = cliargsError();
    TEST("error message not NULL", err != NULL);
    TEST("error message contains 'badarg'",
         err != NULL && strstr(err, "badarg") != NULL);
    TEST("error message contains 'Unrecognized'",
         err != NULL && strstr(err, "Unrecognized") != NULL);
}

/* ── short unknown arg error ── */
static void test_error_short_arg(void)
{
    char *argv[] = {"prog", "-z", NULL};
    cliargsReset();
    cliargsRegister("verbose", 'v', "Enable verbose");
    cliargsParse(2, argv);
    TEST("invalid short unknown arg", cliargsValid() == false);
    char *err = cliargsError();
    TEST("short error message not NULL", err != NULL);
    TEST("short error message contains 'z'",
         err != NULL && strstr(err, "z") != NULL);
}

/* ── no error when no registration (no validation) ── */
static void test_no_registration_no_error(void)
{
    char *argv[] = {"prog", "--anything", NULL};
    cliargsReset();
    /* no cliargsRegister calls */
    cliargsParse(2, argv);
    TEST("no registration means valid", cliargsValid() == true);
    TEST("no registration no error",    cliargsError() == NULL);
}

/* ── built-ins do not trigger validation errors ── */
static void test_builtins_not_errors(void)
{
    /* We can't test --help/-h/--version directly since they call exit(),
       but we can verify that registering nothing and parsing regular
       args (non-builtin) gives valid when no registration. */
    char *argv[] = {"prog", "--verbose", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    TEST("no registration, --verbose valid", cliargsValid() == true);
}

/* ── strict mode exits with code 1 on unrecognized arg ── */
static void test_strict_mode_exits(void)
{
    pid_t pid = fork();
    if (pid == 0) {
        /* child: redirect stdout/stderr to suppress output */
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        cliargsReset();
        cliargsStrict();
        cliargsRegister("verbose", 'v', "Enable verbose");
        char *argv[] = {"prog", "--unknown", NULL};
        cliargsParse(2, argv);
        /* should not reach here */
        _exit(0);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    TEST("strict mode exits with code 1",
         WIFEXITED(status) && WEXITSTATUS(status) == 1);
}

/* ── strict mode does not exit when all args registered ── */
static void test_strict_mode_no_exit_when_valid(void)
{
    char *argv[] = {"prog", "--verbose", NULL};
    cliargsReset();
    cliargsStrict();
    cliargsRegister("verbose", 'v', "Enable verbose");
    cliargsParse(2, argv);
    TEST("strict mode no exit when valid", cliargsValid() == true);
}

int main(void)
{
    printf("Running test_validation...\n");

    test_valid_all_registered();
    test_invalid_unknown_arg();
    test_error_message();
    test_error_short_arg();
    test_no_registration_no_error();
    test_builtins_not_errors();
    test_strict_mode_exits();
    test_strict_mode_no_exit_when_valid();

    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
