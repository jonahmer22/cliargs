# cliargs

A lightweight, portable argument parser for C — no dependencies, no fuss.

---

## Overview

`cliargs` is a fully-featured command-line argument parsing library for C. It supports short and long flags, key-value arguments, positional arguments, subcommands, typed getters, help/version generation, environment variable fallbacks, and more — all behind a clean, minimal API.

---

## Quick Start

```c
#include "cliargs.h"

int main(int argc, char **argv) {
    cliargsSetDescription("My awesome CLI tool");
    cliargsSetVersion("1.0.0");
    cliargsRegister("out", 'o', "Output file path");
    cliargsRegister("verbose", 'v', "Enable verbose output");

    cliargsParse(argc, argv);

    if (cliargsFlag("verbose", 'v')) {
        printf("Verbose mode enabled\n");
    }

    char *output = cliargsArgDefault("out", 'o', "output.txt");
    printf("Writing to: %s\n", output);

    return 0;
}
```

---

## API Reference

### Setup & Parsing

---

#### `void cliargsParse(int argc, char **argv)`

Parses the command-line arguments passed to `main`. This **must** be called before any query functions. Typically called at the top of `main` after any registration calls.

```c
cliargsParse(argc, argv);
```

---

#### `void cliargsSetDescription(char *desc)`

Sets a short description of the program, used when generating `--help` output.

```c
cliargsSetDescription("A tool for converting files.");
```

---

#### `void cliargsSetVersion(char *version)`

Sets the program version string. Automatically handles the `--version` flag — when passed, it prints the version and exits.

```c
cliargsSetVersion("1.0.0");
// ./mytool --version  →  prints "1.0.0" and exits
```

---

#### `void cliargsRegister(char *name, char shorthand, char *helpText)`

Registers an argument or flag with its long name, optional short form, and help text. Registration is optional but required for help generation and strict mode validation. Use `0` or `'\0'` as the shorthand if no short form is desired.

```c
cliargsRegister("output", 'o', "Path to the output file");
cliargsRegister("verbose", 'v', "Enable verbose logging");
cliargsRegister("dry-run", 0, "Simulate actions without executing");
```

---

#### `void cliargsReset()`

Resets all internal state, clearing all parsed arguments, registered options, and settings. Useful for testing or re-parsing.

```c
cliargsReset();
```

---

### Flags

Flags are boolean switches. They begin with a single dash for short form (`-v`) or double dash for long form (`--verbose`). Short flags can be composed together: `-abc` is equivalent to `-a -b -c`.

---

#### `bool cliargsFlag(char *name, char shorthand)`

Returns `true` if the flag was present in the arguments, `false` otherwise. Matches both `--name` and `-shorthand`. Pass `0` for shorthand if no short form exists.

```c
// Matches --verbose or -v
if (cliargsFlag("verbose", 'v')) {
    printf("Verbose mode\n");
}

// Matches --dry-run only
if (cliargsFlag("dry-run", 0)) {
    printf("Dry run mode\n");
}
```

---

#### `int cliargsCount(char *name, char shorthand)`

Returns how many times a flag was provided. Useful for flags that scale in intensity, like verbosity. Counts both repeated long flags (`--verbose --verbose`) and stacked short flags (`-vvv`).

```c
int verbosity = cliargsCount("verbose", 'v');
// -vvv or -v -v -v → returns 3
// --verbose --verbose → returns 2
```

---

### Arguments (Key-Value)

Arguments are key-value pairs. Long form: `--out "file.txt"`. Short form: `-o "file.txt"`. The value immediately follows the flag, separated by a space. Quotes are handled by the shell.

---

#### `char *cliargsArg(char *name, char shorthand)`

Returns the string value associated with the argument, or `NULL` if not provided. Matches both `--name value` and `-shorthand value`.

```c
char *output = cliargsArg("out", 'o');
if (output == NULL) {
    fprintf(stderr, "No output file specified\n");
}
```

---

#### `char *cliargsArgDefault(char *name, char shorthand, char *fallback)`

Same as `cliargsArg` but returns `fallback` instead of `NULL` when the argument was not provided.

```c
char *output = cliargsArgDefault("out", 'o', "output.txt");
// If --out wasn't passed, returns "output.txt"
```

---

#### `int cliargsArgInt(char *name, char shorthand)`

Returns the argument value parsed as an integer. Returns `0` if not provided or if the value is not a valid integer.

```c
int port = cliargsArgInt("port", 'p');
// --port 8080  →  returns 8080
```

---

#### `int cliargsArgIntDefault(char *name, char shorthand, int fallback)`

Same as `cliargsArgInt` but returns `fallback` if the argument was not provided.

```c
int port = cliargsArgIntDefault("port", 'p', 3000);
// If --port wasn't passed, returns 3000
```

---

#### `float cliargsArgFloat(char *name, char shorthand)`

Returns the argument value parsed as a float. Returns `0.0` if not provided or not a valid float.

```c
float scale = cliargsArgFloat("scale", 's');
// --scale 1.5  →  returns 1.5f
```

---

#### `float cliargsArgFloatDefault(char *name, char shorthand, float fallback)`

Same as `cliargsArgFloat` but returns `fallback` if the argument was not provided.

```c
float scale = cliargsArgFloatDefault("scale", 's', 1.0f);
```

---

#### `char **cliargsArgAll(char *name, char shorthand, int *count)`

Returns all values provided for a repeated argument as an array of strings. The number of values is written to `count`. Returns `NULL` if the argument was never provided.

Useful for arguments that can be specified multiple times, like `--file a.txt --file b.txt`.

```c
int count = 0;
char **files = cliargsArgAll("file", 'f', &count);
for (int i = 0; i < count; i++) {
    printf("File: %s\n", files[i]);
}
// --file a.txt --file b.txt --file c.txt  →  count = 3
```

---

### Positional Arguments

Positional arguments are bare values passed without a flag, e.g. `./mytool input.txt output.txt`.

---

#### `char *cliargsPos(int index)`

Returns the positional argument at the given zero-based index. Returns `NULL` if no argument exists at that index. Positional arguments are collected in the order they appear, excluding flag names and their values.

```c
char *input = cliargsPos(0);   // first positional arg
char *output = cliargsPos(1);  // second positional arg
// ./mytool input.txt output.txt  →  "input.txt", "output.txt"
```

---

### Subcommands

Subcommands allow git-style command dispatching: `./mytool serve --port 8080`.

---

#### `char *cliargsSubcommand()`

Returns the subcommand string if one was provided as the first argument, or `NULL` if none was given. A subcommand is any bare word (not starting with `-`) that appears as the first argument.

```c
char *cmd = cliargsSubcommand();
if (cmd && strcmp(cmd, "serve") == 0) {
    int port = cliargsArgIntDefault("port", 'p', 8080);
    startServer(port);
} else if (cmd && strcmp(cmd, "build") == 0) {
    runBuild();
}
// ./mytool serve --port 9000  →  cliargsSubcommand() = "serve"
```

---

### Passthrough / Remaining Arguments

Everything after a `--` separator is treated as raw passthrough and is not parsed.

---

#### `char **cliargsRemaining(int *count)`

Returns all arguments that appeared after the `--` separator as an array of strings. The number of remaining arguments is written to `count`. Returns `NULL` if `--` was not present.

```c
int count = 0;
char **rest = cliargsRemaining(&count);
// ./mytool --out file.txt -- --these are not parsed
// rest = ["--these", "are", "not", "parsed"], count = 4
```

---

### Validation & Error Handling

---

#### `bool cliargsValid()`

Returns `true` if all provided arguments were recognized (i.e. matched a registered flag or argument), `false` if any unrecognized arguments were encountered. Requires `cliargsRegister` to have been called for each expected argument.

```c
if (!cliargsValid()) {
    fprintf(stderr, "Error: %s\n", cliargsError());
    cliargsPrintHelp();
    return 1;
}
```

---

#### `char *cliargsError()`

Returns a human-readable error message describing the first validation error encountered, or `NULL` if there are no errors. Typically used alongside `cliargsValid()`.

```c
char *err = cliargsError();
if (err) {
    fprintf(stderr, "%s\n", err);
}
```

---

#### `void cliargsStrict()`

Enables strict mode. In strict mode, any unrecognized argument causes the program to print an error, display help, and exit with a non-zero status. Call before `cliargsParse`.

```c
cliargsStrict();
cliargsParse(argc, argv);
// Unrecognized args will now cause an automatic fatal error
```

---

### Help Generation

---

#### `void cliargsPrintHelp()`

Prints an auto-generated help message to stdout based on registered arguments, the program description, and the program version. Also automatically triggered when the user passes `--help` or `-h`.

Output format:
```
My awesome CLI tool

Usage: mytool [options] [subcommand]

Options:
  -o, --out <value>    Output file path
  -v, --verbose        Enable verbose output
      --dry-run        Simulate actions without executing
      --help           Show this help message
      --version        Show version information
```

```c
cliargsPrintHelp();
```

---

### Environment Variable Fallback

---

#### `void cliargsEnvFallback(bool enabled)`

When enabled, any argument not explicitly provided on the command line will fall back to checking a corresponding environment variable. The variable name is derived by uppercasing the argument name and replacing dashes with underscores, prefixed with `CLIARGS_`.

For example, `--output-file` would fall back to `$CLIARGS_OUTPUT_FILE`.

```c
cliargsEnvFallback(true);
cliargsParse(argc, argv);

// If --port is not passed, checks $CLIARGS_PORT automatically
int port = cliargsArgIntDefault("port", 'p', 3000);
```

---

### Utility

---

#### `bool cliargsIsFlagSet(char *name, char shorthand)`

Returns `true` if the flag was **explicitly set** by the user, `false` otherwise. Distinct from `cliargsFlag` in that it does not consider default values or environment fallbacks — only whether the user literally passed it.

Useful when you need to know if a flag was intentionally provided vs just falling back to a default.

```c
if (cliargsIsFlagSet("verbose", 'v')) {
    printf("User explicitly asked for verbose\n");
}
```

---

## Behavior Summary

| Input | Example | Function |
|---|---|---|
| Short flag | `-v` | `cliargsFlag("verbose", 'v')` |
| Long flag | `--verbose` | `cliargsFlag("verbose", 'v')` |
| Composed short flags | `-abc` | `cliargsFlag("a", 'a')` etc. |
| Short key-value | `-o file.txt` | `cliargsArg("out", 'o')` |
| Long key-value | `--out file.txt` | `cliargsArg("out", 'o')` |
| Positional | `./tool input.txt` | `cliargsPos(0)` |
| Subcommand | `./tool serve` | `cliargsSubcommand()` |
| Repeated arg | `--file a --file b` | `cliargsArgAll("file", 'f', &n)` |
| Passthrough | `-- --raw args` | `cliargsRemaining(&n)` |
| Repeated flag | `-vvv` | `cliargsCount("verbose", 'v')` |

---

## Design Notes

- **Global state**: `cliargs` uses a single global parse context. Call `cliargsReset()` between parses if needed (e.g. in tests).
- **No required registration**: `cliargsRegister` is optional unless using strict mode or help generation.
- **NULL safety**: All query functions that return pointers return `NULL` on missing values — check before use or use the `Default` variants.
- **No dynamic allocation by the caller**: All returned strings point into the original `argv` memory or internal static buffers. Do not free them.
