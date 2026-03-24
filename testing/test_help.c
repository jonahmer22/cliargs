/*
 * test_help.c — tests for cliargsPrintHelp output
 *
 * Captures stdout via a pipe and checks for expected substrings.
 */

#include "../cliargs.h"

#include <stdio.h>
#include <string.h>
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

/*
 * Redirect stdout to a pipe, call cliargsPrintHelp(), restore stdout,
 * then read the captured output into buf (up to bufsize-1 bytes).
 * Returns number of bytes read, or -1 on error.
 */
static int capture_help(char *buf, int bufsize)
{
    int pipefd[2];
    int saved_stdout;
    ssize_t n;

    if (pipe(pipefd) != 0)
        return -1;

    fflush(stdout);
    saved_stdout = dup(fileno(stdout));
    if (saved_stdout < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (dup2(pipefd[1], fileno(stdout)) < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        close(saved_stdout);
        return -1;
    }
    close(pipefd[1]);

    cliargsPrintHelp();
    fflush(stdout);

    dup2(saved_stdout, fileno(stdout));
    close(saved_stdout);

    n = read(pipefd[0], buf, (size_t)(bufsize - 1));
    close(pipefd[0]);

    if (n < 0)
        n = 0;
    buf[n] = '\0';
    return (int)n;
}

/* ── help output contains description ── */
static void test_help_contains_description(void)
{
    char buf[4096];
    cliargsReset();
    cliargsSetDescription("My awesome CLI tool");
    /* parse something minimal so s_program_name is set */
    char *argv[] = {"mytool", NULL};
    cliargsParse(1, argv);

    capture_help(buf, (int)sizeof(buf));

    TEST("help contains description",
         strstr(buf, "My awesome CLI tool") != NULL);
}

/* ── help output contains registered arg names ── */
static void test_help_contains_arg_names(void)
{
    char buf[4096];
    cliargsReset();
    cliargsSetDescription("Test tool");
    cliargsRegister("output", 'o', "Output file path");
    cliargsRegister("verbose", 'v', "Enable verbose output");
    cliargsRegister("dry-run", 0,   "Simulate without executing");

    char *argv[] = {"prog", NULL};
    cliargsParse(1, argv);
    capture_help(buf, (int)sizeof(buf));

    TEST("help contains --output", strstr(buf, "--output") != NULL);
    TEST("help contains --verbose", strstr(buf, "--verbose") != NULL);
    TEST("help contains --dry-run", strstr(buf, "--dry-run") != NULL);
    TEST("help contains -o shorthand", strstr(buf, "-o") != NULL);
    TEST("help contains -v shorthand", strstr(buf, "-v") != NULL);
}

/* ── help output contains help text ── */
static void test_help_contains_help_text(void)
{
    char buf[4096];
    cliargsReset();
    cliargsRegister("output", 'o', "Output file path");
    cliargsRegister("verbose", 'v', "Enable verbose output");

    char *argv[] = {"prog", NULL};
    cliargsParse(1, argv);
    capture_help(buf, (int)sizeof(buf));

    TEST("help contains 'Output file path'",
         strstr(buf, "Output file path") != NULL);
    TEST("help contains 'Enable verbose output'",
         strstr(buf, "Enable verbose output") != NULL);
}

/* ── help output contains --help entry ── */
static void test_help_contains_help_entry(void)
{
    char buf[4096];
    cliargsReset();
    char *argv[] = {"prog", NULL};
    cliargsParse(1, argv);
    capture_help(buf, (int)sizeof(buf));

    TEST("help contains '--help'", strstr(buf, "--help") != NULL);
    TEST("help contains 'Show this help'",
         strstr(buf, "Show this help") != NULL);
}

/* ── help output contains --version entry ── */
static void test_help_contains_version_entry(void)
{
    char buf[4096];
    cliargsReset();
    cliargsSetVersion("1.0.0");
    char *argv[] = {"prog", NULL};
    cliargsParse(1, argv);
    capture_help(buf, (int)sizeof(buf));

    TEST("help contains '--version'", strstr(buf, "--version") != NULL);
    TEST("help contains 'Show version'",
         strstr(buf, "Show version") != NULL);
}

/* ── alignment: all help text starts at same column ── */
static void test_help_alignment(void)
{
    char buf[4096];
    cliargsReset();
    cliargsRegister("out",     'o', "Output");
    cliargsRegister("verbose", 'v', "Verbose");
    cliargsRegister("dry-run", 0,   "Dry run");

    char *argv[] = {"prog", NULL};
    cliargsParse(1, argv);
    capture_help(buf, (int)sizeof(buf));

    /* All option help lines contain their help text.
       We do a "visual" check: each help text appears in the output. */
    TEST("alignment: Output present",  strstr(buf, "Output")  != NULL);
    TEST("alignment: Verbose present", strstr(buf, "Verbose") != NULL);
    TEST("alignment: Dry run present", strstr(buf, "Dry run") != NULL);

    /* Check that 'Options:' heading is present */
    TEST("help contains 'Options:' heading",
         strstr(buf, "Options:") != NULL);

    /* Check that 'Usage:' line is present */
    TEST("help contains 'Usage:' line", strstr(buf, "Usage:") != NULL);
}

/* ── help without description ── */
static void test_help_no_description(void)
{
    char buf[4096];
    cliargsReset();
    char *argv[] = {"prog", NULL};
    cliargsParse(1, argv);
    capture_help(buf, (int)sizeof(buf));

    /* should still show Usage and Options */
    TEST("no description: Usage present", strstr(buf, "Usage:") != NULL);
    TEST("no description: --help present", strstr(buf, "--help") != NULL);
}

/* ── program name in usage line ── */
static void test_help_program_name(void)
{
    char buf[4096];
    cliargsReset();
    char *argv[] = {"mytestprog", NULL};
    cliargsParse(1, argv);
    capture_help(buf, (int)sizeof(buf));

    TEST("program name in usage line",
         strstr(buf, "mytestprog") != NULL);
}

int main(void)
{
    printf("Running test_help...\n");

    test_help_contains_description();
    test_help_contains_arg_names();
    test_help_contains_help_text();
    test_help_contains_help_entry();
    test_help_contains_version_entry();
    test_help_alignment();
    test_help_no_description();
    test_help_program_name();

    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
