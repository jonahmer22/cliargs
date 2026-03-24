/*
 * test_remaining.c — tests for cliargsRemaining
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

/* ── args after -- captured correctly ── */
static void test_remaining_captured(void)
{
    char *argv[] = {"prog", "--out", "file.txt", "--",
                    "--these", "are", "not", "parsed", NULL};
    cliargsReset();
    cliargsParse(8, argv);
    int count = 0;
    char **rest = cliargsRemaining(&count);
    TEST("remaining not NULL", rest != NULL);
    TEST("remaining count == 4", count == 4);
    TEST("remaining[0] == --these",
         rest != NULL && strcmp(rest[0], "--these") == 0);
    TEST("remaining[1] == are",
         rest != NULL && strcmp(rest[1], "are") == 0);
    TEST("remaining[2] == not",
         rest != NULL && strcmp(rest[2], "not") == 0);
    TEST("remaining[3] == parsed",
         rest != NULL && strcmp(rest[3], "parsed") == 0);
}

/* ── args before -- parsed normally ── */
static void test_before_separator_parsed(void)
{
    char *argv[] = {"prog", "--verbose", "--", "extra", NULL};
    cliargsReset();
    cliargsParse(4, argv);
    TEST("--verbose before -- parsed",
         cliargsFlag("verbose", 'v') == true);
    int count = 0;
    char **rest = cliargsRemaining(&count);
    TEST("remaining count == 1", count == 1);
    TEST("remaining[0] == extra",
         rest != NULL && strcmp(rest[0], "extra") == 0);
}

/* ── NULL and count=0 when no -- present ── */
static void test_no_separator(void)
{
    char *argv[] = {"prog", "--verbose", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    int count = 99;
    char **rest = cliargsRemaining(&count);
    TEST("no -- returns NULL", rest == NULL);
    TEST("no -- count == 0", count == 0);
}

/* ── -- with no following args ── */
static void test_empty_remaining(void)
{
    char *argv[] = {"prog", "--", NULL};
    cliargsReset();
    cliargsParse(2, argv);
    int count = 99;
    char **rest = cliargsRemaining(&count);
    /* -- was present, so not NULL; count == 0 */
    TEST("empty remaining: not NULL", rest != NULL);
    TEST("empty remaining: count == 0", count == 0);
}

/* ── remaining args not parsed as flags ── */
static void test_remaining_not_parsed(void)
{
    char *argv[] = {"prog", "--", "--verbose", NULL};
    cliargsReset();
    cliargsParse(3, argv);
    /* --verbose after -- should NOT be treated as a flag */
    TEST("--verbose after -- not a flag",
         cliargsFlag("verbose", 'v') == false);
    int count = 0;
    char **rest = cliargsRemaining(&count);
    TEST("remaining[0] == --verbose",
         rest != NULL && strcmp(rest[0], "--verbose") == 0);
}

/* ── only remaining present ── */
static void test_only_remaining(void)
{
    char *argv[] = {"prog", "--", "a", "b", "c", NULL};
    cliargsReset();
    cliargsParse(5, argv);
    int count = 0;
    char **rest = cliargsRemaining(&count);
    TEST("only remaining count == 3", count == 3);
    TEST("only remaining [2] == c",
         rest != NULL && strcmp(rest[2], "c") == 0);
}

int main(void)
{
    printf("Running test_remaining...\n");

    test_remaining_captured();
    test_before_separator_parsed();
    test_no_separator();
    test_empty_remaining();
    test_remaining_not_parsed();
    test_only_remaining();

    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
