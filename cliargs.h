#ifndef CLIARGS_H
#define CLIARGS_H

/*
 * cliargs - A lightweight CLI argument parsing library for C.
 *
 * All returned char * pointers point into the original argv memory or
 * into static internal buffers. Callers must not free them.
 *
 * Registration (cliargsRegister) is optional but required for help
 * generation and strict-mode validation.  Call cliargsReset() between
 * parse sessions (e.g. in unit tests) to clear all internal state.
 */

#include <stdbool.h>

/* ── Setup & Parsing ─────────────────────────────────────────────── */

void cliargsParse(int argc, char **argv);
void cliargsSetDescription(char *desc);
void cliargsSetVersion(char *version);
void cliargsRegister(char *name, char shorthand, char *helpText);
void cliargsReset(void);

/* ── Flags ───────────────────────────────────────────────────────── */

bool cliargsFlag(char *name, char shorthand);
int  cliargsCount(char *name, char shorthand);

/* ── Arguments (Key-Value) ───────────────────────────────────────── */

char  *cliargsArg(char *name, char shorthand);
char  *cliargsArgDefault(char *name, char shorthand, char *fallback);
int    cliargsArgInt(char *name, char shorthand);
int    cliargsArgIntDefault(char *name, char shorthand, int fallback);
float  cliargsArgFloat(char *name, char shorthand);
float  cliargsArgFloatDefault(char *name, char shorthand, float fallback);
char **cliargsArgAll(char *name, char shorthand, int *count);

/* ── Positional Arguments ────────────────────────────────────────── */

char *cliargsPos(int index);

/* ── Subcommands ─────────────────────────────────────────────────── */

char *cliargsSubcommand(void);

/* ── Passthrough Arguments ───────────────────────────────────────── */

char **cliargsRemaining(int *count);

/* ── Validation & Error Handling ─────────────────────────────────── */

bool  cliargsValid(void);
char *cliargsError(void);
void  cliargsStrict(void);

/* ── Help Generation ─────────────────────────────────────────────── */

void cliargsPrintHelp(void);

/* ── Environment Variable Fallback ──────────────────────────────── */

void cliargsEnvFallback(bool enabled);

/* ── Utility ─────────────────────────────────────────────────────── */

bool cliargsIsFlagSet(char *name, char shorthand);

#endif /* CLIARGS_H */
