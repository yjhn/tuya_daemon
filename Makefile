BIN:=daemon
PROGRAM_NAME:=tuya_daemon
SRCS:=$(wildcard *.c)
# Garbage produced by make check.
ASSEMBLY_FILES:=$(wildcard *.s)
CPPFLAGS:=
CFLAGS:=-std=gnu11 -Wall -Wextra -Wpedantic -Wconversion -Wmissing-prototypes\
-Wstrict-prototypes
EXEC_BIN:=$(BIN)

.PHONY: debug clangd clean check format cppcheck
.DELETE_ON_ERROR:

all: $(BIN)

$(BIN):
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRCS) -o $(BIN)

debug: $(BIN)
	$(EXEC_ENV) gdb --args ./$(BIN)

valgrind: $(BIN)
	valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes -s env $(EXEC_BIN)

# compile_commands.json is needed for clangd to work
# we generate them using bear:
# https://github.com/rizsotto/Bear
# Clean build artifacts first to force make to run the build commands.
clangd: clean
	bear -- $(MAKE)

clean:
	$(RM) $(OBJS) $(BIN) $(ASSEMBLY_FILES)

# Produces .s files.
check:
	gcc $(CPPFLAGS) $(CFLAGS) -fanalyzer -S $(SRCS)

format:
	clang-format -i --style=file --verbose $(SRCS) $(HEADERS)

cppcheck:
	cppcheck --enable=all .

showlog:
	journalctl --no-hostname -r -t $(PROGRAM_NAME)

# WARNING: this clears all logs in the system,
# because it is not possible to clear only relevant ones.
clearlog:
	sudo journalctl --rotate
	sudo journalctl --vacuum-time 1s