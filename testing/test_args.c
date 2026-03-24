/*
 * test_args.c — tests for key-value argument functions
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

/* ── short key-value ── */
static void test_short_arg(void)
{
    char *argv[] = {"prog", "-o", "file.txt", NULL};
    cliargsReset();
    cliargsParse(3, argv);
    char *v = cliargsArg("out", 'o');
    TEST("short -o value", v != NULL && strcmp(v, "file.txt") == 0);
}

/* ── long key-value ── */
static void test_long_arg(void)
{
    char *argv[] = {"prog", "--out", "result.txt", NULL};
    cliargsReset();
    cliargsParse(3, argv);
    char *v = cliargsArg("out", 'o');
    TEST("long --out value", v != NULL && strcmp(v, "result.txt") == 0);
}

/* ── missing arg returns NULL ── */
static void test_arg_missing(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    TEST("missing arg returns NULL", cliargsArg("out", 'o') == NULL);
}

/* ── cliargsArgDefault with value present ── */
static void test_arg_default_present(void)
{
    char *argv[] = {"prog", "--out", "custom.txt", NULL};
    cliargsReset();
    cliargsParse(3, argv);
    char *v = cliargsArgDefault("out", 'o', "default.txt");
    TEST("ArgDefault returns arg value when present",
         v != NULL && strcmp(v, "custom.txt") == 0);
}

/* ── cliargsArgDefault with value absent ── */
static void test_arg_default_absent(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    char *v = cliargsArgDefault("out", 'o', "default.txt");
    TEST("ArgDefault returns fallback when absent",
         v != NULL && strcmp(v, "default.txt") == 0);
}

/* ── cliargsArgInt ── */
static void test_arg_int(void)
{
    char *argv[] = {"prog", "--port", "8080", NULL};
    cliargsReset();
    cliargsParse(3, argv);
    TEST("ArgInt returns 8080", cliargsArgInt("port", 'p') == 8080);
}

/* ── cliargsArgInt missing returns 0 ── */
static void test_arg_int_missing(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    TEST("ArgInt missing returns 0", cliargsArgInt("port", 'p') == 0);
}

/* ── cliargsArgIntDefault ── */
static void test_arg_int_default(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    TEST("ArgIntDefault returns fallback",
         cliargsArgIntDefault("port", 'p', 3000) == 3000);
}

/* ── cliargsArgIntDefault with value present ── */
static void test_arg_int_default_present(void)
{
    char *argv[] = {"prog", "--port", "9090", NULL};
    cliargsReset();
    cliargsParse(3, argv);
    TEST("ArgIntDefault returns parsed value when present",
         cliargsArgIntDefault("port", 'p', 3000) == 9090);
}

/* ── cliargsArgFloat ── */
static void test_arg_float(void)
{
    char *argv[] = {"prog", "--scale", "1.5", NULL};
    cliargsReset();
    cliargsParse(3, argv);
    float v = cliargsArgFloat("scale", 's');
    TEST("ArgFloat returns 1.5", v > 1.49f && v < 1.51f);
}

/* ── cliargsArgFloat missing returns 0.0f ── */
static void test_arg_float_missing(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    TEST("ArgFloat missing returns 0.0f",
         cliargsArgFloat("scale", 's') == 0.0f);
}

/* ── cliargsArgFloatDefault ── */
static void test_arg_float_default(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    float v = cliargsArgFloatDefault("scale", 's', 2.5f);
    TEST("ArgFloatDefault returns fallback", v > 2.49f && v < 2.51f);
}

/* ── cliargsArgFloatDefault with value present ── */
static void test_arg_float_default_present(void)
{
    char *argv[] = {"prog", "--scale", "3.14", NULL};
    cliargsReset();
    cliargsParse(3, argv);
    float v = cliargsArgFloatDefault("scale", 's', 1.0f);
    TEST("ArgFloatDefault returns parsed value when present",
         v > 3.13f && v < 3.15f);
}

/* ── cliargsArgAll with multiple repeated args ── */
static void test_arg_all(void)
{
    char *argv[] = {"prog", "--file", "a.txt",
                    "--file", "b.txt", "--file", "c.txt", NULL};
    cliargsReset();
    cliargsParse(7, argv);
    int count = 0;
    char **files = cliargsArgAll("file", 'f', &count);
    TEST("ArgAll count == 3", count == 3);
    TEST("ArgAll not NULL", files != NULL);
    TEST("ArgAll [0] == a.txt",
         files != NULL && strcmp(files[0], "a.txt") == 0);
    TEST("ArgAll [1] == b.txt",
         files != NULL && strcmp(files[1], "b.txt") == 0);
    TEST("ArgAll [2] == c.txt",
         files != NULL && strcmp(files[2], "c.txt") == 0);
}

/* ── cliargsArgAll with short and long mixed ── */
static void test_arg_all_mixed(void)
{
    char *argv[] = {"prog", "-f", "x.txt", "--file", "y.txt", NULL};
    cliargsReset();
    cliargsParse(5, argv);
    int count = 0;
    cliargsArgAll("file", 'f', &count);
    TEST("ArgAll mixed count == 2", count == 2);
}

/* ── cliargsArgAll absent returns NULL ── */
static void test_arg_all_absent(void)
{
    char *argv[] = {"prog", NULL};
    cliargsReset();
    cliargsParse(1, argv);
    int count = 0;
    char **files = cliargsArgAll("file", 'f', &count);
    TEST("ArgAll absent returns NULL", files == NULL);
    TEST("ArgAll absent count == 0", count == 0);
}

int main(void)
{
    printf("Running test_args...\n");

    test_short_arg();
    test_long_arg();
    test_arg_missing();
    test_arg_default_present();
    test_arg_default_absent();
    test_arg_int();
    test_arg_int_missing();
    test_arg_int_default();
    test_arg_int_default_present();
    test_arg_float();
    test_arg_float_missing();
    test_arg_float_default();
    test_arg_float_default_present();
    test_arg_all();
    test_arg_all_mixed();
    test_arg_all_absent();

    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
