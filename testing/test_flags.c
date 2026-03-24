/*
 * test_flags.c — tests for cliargsFlag, cliargsCount, cliargsIsFlagSet
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

/* ── short flag present ── */
static void test_short_flag_present(void)
{
    char *argv[] = {"prog", "-v", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    TEST("short flag -v present", cliargsFlag("verbose", 'v') == true);
}

/* ── short flag absent ── */
static void test_short_flag_absent(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    TEST("short flag -v absent", cliargsFlag("verbose", 'v') == false);
}

/* ── long flag present ── */
static void test_long_flag_present(void)
{
    char *argv[] = {"prog", "--verbose", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    TEST("long flag --verbose present", cliargsFlag("verbose", 'v') == true);
}

/* ── long flag absent ── */
static void test_long_flag_absent(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    TEST("long flag --verbose absent", cliargsFlag("verbose", 'v') == false);
}

/* ── composed short flags -abc ── */
static void test_composed_flags(void)
{
    char *argv[] = {"prog", "-abc", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    TEST("composed -abc: flag a", cliargsFlag("a", 'a') == true);
    TEST("composed -abc: flag b", cliargsFlag("b", 'b') == true);
    TEST("composed -abc: flag c", cliargsFlag("c", 'c') == true);
    TEST("composed -abc: flag d absent", cliargsFlag("d", 'd') == false);
}

/* ── flag count: stacked short -vvv ── */
static void test_count_stacked_short(void)
{
    char *argv[] = {"prog", "-vvv", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    TEST("stacked -vvv count == 3", cliargsCount("verbose", 'v') == 3);
}

/* ── flag count: repeated short -v -v -v ── */
static void test_count_repeated_short(void)
{
    char *argv[] = {"prog", "-v", "-v", "-v", NULL};
    cliargsReset();
    cliargsParse(4, argv);
    TEST("repeated -v -v -v count == 3", cliargsCount("verbose", 'v') == 3);
}

/* ── flag count: repeated long --verbose --verbose ── */
static void test_count_repeated_long(void)
{
    char *argv[] = {"prog", "--verbose", "--verbose", NULL};
    cliargsReset();
    cliargsParse(3, argv);
    TEST("repeated --verbose --verbose count == 2",
         cliargsCount("verbose", 'v') == 2);
}

/* ── flag count when absent ── */
static void test_count_absent(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    TEST("count absent == 0", cliargsCount("verbose", 'v') == 0);
}

/* ── cliargsIsFlagSet: flag explicitly set ── */
static void test_is_flag_set_true(void)
{
    char *argv[] = {"prog", "--verbose", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    TEST("isFlagSet true when passed", cliargsIsFlagSet("verbose", 'v') == true);
}

/* ── cliargsIsFlagSet: flag not passed ── */
static void test_is_flag_set_false(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    TEST("isFlagSet false when not passed",
         cliargsIsFlagSet("verbose", 'v') == false);
}

/* ── flag matching by name only (no shorthand) ── */
static void test_flag_no_shorthand(void)
{
    char *argv[] = {"prog", "--dry-run", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    TEST("long-only flag --dry-run present",
         cliargsFlag("dry-run", 0) == true);
}

/* ── mixed short and long ── */
static void test_count_mixed(void)
{
    char *argv[] = {"prog", "-v", "--verbose", "-vv", NULL};
    cliargsReset();
    cliargsParse(4, argv);
    /* -v=1, --verbose=1, -vv=2 → total 4 */
    TEST("mixed count == 4", cliargsCount("verbose", 'v') == 4);
}

int main(void)
{
    printf("Running test_flags...\n");

    test_short_flag_present();
    test_short_flag_absent();
    test_long_flag_present();
    test_long_flag_absent();
    test_composed_flags();
    test_count_stacked_short();
    test_count_repeated_short();
    test_count_repeated_long();
    test_count_absent();
    test_is_flag_set_true();
    test_is_flag_set_false();
    test_flag_no_shorthand();
    test_count_mixed();

    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
