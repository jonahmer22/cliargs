CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -I..
SRC     = ../cliargs.c

TESTS = test_flags \
        test_args \
        test_positional \
        test_subcommands \
        test_remaining \
        test_validation \
        test_env \
        test_help

.PHONY: all clean run

all: $(TESTS) run

$(TESTS): %: %.c $(SRC) ../cliargs.h
	$(CC) $(CFLAGS) -o $@ $< $(SRC)

run: $(TESTS)
	@FAILED=0; \
	for t in $(TESTS); do \
		if ./$$t; then \
			echo "PASSED: $$t"; \
		else \
			echo "FAILED: $$t"; \
			FAILED=1; \
		fi; \
	done; \
	if [ $$FAILED -eq 0 ]; then \
		echo "All tests passed."; \
	else \
		echo "Some tests FAILED."; \
	fi; \
	exit $$FAILED

clean:
	rm -f $(TESTS)
