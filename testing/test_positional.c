/*
 * test_positional.c — tests for cliargsPos
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

/* ── single positional ── */
static void test_single_positional(void)
{
    /* The first bare word is always the subcommand; positionals start after it.
     * Use "sub" as the subcommand and "input.txt" as the single positional. */
    char *argv[] = {"prog", "sub", "input.txt", NULL};
    cliargsReset();
    cliargsParse(3, argv);
    TEST("single positional [0]",
         cliargsPos(0) != NULL && strcmp(cliargsPos(0), "input.txt") == 0);
    TEST("single positional [1] == NULL", cliargsPos(1) == NULL);
    TEST("subcommand is sub",
         cliargsSubcommand() != NULL && strcmp(cliargsSubcommand(), "sub") == 0);
}

/* ── multiple positionals ── */
static void test_multiple_positionals(void)
{
    char *argv[] = {"prog", "a", "b", "c", NULL};
    cliargsReset();
    cliargsParse(4, argv);
    /* first bare word is subcommand; b and c are positionals */
    TEST("multiple positionals [0] == b",
         cliargsPos(0) != NULL && strcmp(cliargsPos(0), "b") == 0);
    TEST("multiple positionals [1] == c",
         cliargsPos(1) != NULL && strcmp(cliargsPos(1), "c") == 0);
    TEST("multiple positionals [2] == NULL", cliargsPos(2) == NULL);
}

/* ── correct indexing ── */
static void test_indexing(void)
{
    /* Use --flag to avoid subcommand confusion */
    char *argv[] = {"prog", "--flag", "alpha", "beta", "gamma", NULL};
    cliargsReset();
    cliargsParse(5, argv);
    /* --flag: no next non-flag token immediately... but "alpha" follows,
       so --flag is KEYVAL with value "alpha".  beta and gamma are
       subcommand and positional respectively. */
    /* Let's use a cleaner setup: all bare words, first is subcommand */
    /* Re-do: prog sub one two three */
    cliargsReset();
    char *argv2[] = {"prog", "sub", "one", "two", "three", NULL};
    cliargsParse(5, argv2);
    TEST("indexing [0] == one",
         cliargsPos(0) != NULL && strcmp(cliargsPos(0), "one") == 0);
    TEST("indexing [1] == two",
         cliargsPos(1) != NULL && strcmp(cliargsPos(1), "two") == 0);
    TEST("indexing [2] == three",
         cliargsPos(2) != NULL && strcmp(cliargsPos(2), "three") == 0);
    TEST("indexing [3] == NULL", cliargsPos(3) == NULL);
}

/* ── out-of-bounds returns NULL ── */
static void test_out_of_bounds(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    TEST("out-of-bounds [0] NULL", cliargsPos(0) == NULL);
    TEST("out-of-bounds [-1] NULL", cliargsPos(-1) == NULL);
    TEST("out-of-bounds [99] NULL", cliargsPos(99) == NULL);
}

/* ── positionals exclude flag values ── */
static void test_positionals_exclude_flag_values(void)
{
    /* --out output.txt: output.txt is a key-value, not positional */
    char *argv[] = {"prog", "--out", "output.txt", "positional1", NULL};
    cliargsReset();
    cliargsParse(4, argv);
    /* "positional1" is the first bare word → subcommand */
    TEST("flag value not in positionals",
         cliargsPos(0) == NULL);
    TEST("subcommand is positional1",
         cliargsSubcommand() != NULL &&
         strcmp(cliargsSubcommand(), "positional1") == 0);
}

/* ── positionals exclude subcommand ── */
static void test_positionals_exclude_subcommand(void)
{
    char *argv[] = {"prog", "serve", "arg1", "arg2", NULL};
    cliargsReset();
    cliargsParse(4, argv);
    TEST("subcommand not in positionals[0]",
         cliargsPos(0) != NULL && strcmp(cliargsPos(0), "arg1") == 0);
    TEST("positional[1] == arg2",
         cliargsPos(1) != NULL && strcmp(cliargsPos(1), "arg2") == 0);
}

/* ── positionals with mixed flags and values ── */
static void test_positionals_mixed(void)
{
    /* -v:    flag (next token "pos1" is a plain word, not a value)
     * pos1:  subcommand (first bare word)
     * -o file.txt: key-value (file.txt contains '.' → looks like a value)
     * pos2:  positional[0] */
    char *argv[] = {"prog", "-v", "pos1", "-o", "file.txt", "pos2", NULL};
    cliargsReset();
    cliargsParse(6, argv);
    TEST("mixed: -v is flag", cliargsFlag(NULL, 'v') == true);
    TEST("mixed: subcommand == pos1",
         cliargsSubcommand() != NULL && strcmp(cliargsSubcommand(), "pos1") == 0);
    TEST("mixed: positional[0] == pos2",
         cliargsPos(0) != NULL && strcmp(cliargsPos(0), "pos2") == 0);
    TEST("mixed: positional[1] == NULL", cliargsPos(1) == NULL);
}

int main(void)
{
    printf("Running test_positional...\n");

    test_single_positional();
    test_multiple_positionals();
    test_indexing();
    test_out_of_bounds();
    test_positionals_exclude_flag_values();
    test_positionals_exclude_subcommand();
    test_positionals_mixed();

    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
