BIN:=tuya_daemon
PROGRAM_NAME:=tuya_daemon
SRCS:=$(wildcard *.c)
HEADERS:=$(wildcard *.h)
OBJS:=$(SRCS:.c=.o)
# Garbage produced by make check.
ASSEMBLY_FILES:=$(wildcard *.s)
CPPFLAGS:=-Iinclude
CFLAGS:=-std=gnu11 -Wall -Wextra -Wpedantic -Wconversion -Wmissing-prototypes\
-Wstrict-prototypes
# Program arguments should be supplied when running make.
PROGRAM_ARGS:=
EXEC_ENV:=LD_LIBRARY_PATH=lib
EXEC_BIN:=$(EXEC_ENV) ./$(BIN)
LDLIBS:=-llink_core -lmiddleware_implementation -lplatform_port -lutils_modules
LDFLAGS:=-Llib

.PHONY: debug clangd clean check format cppcheck showlog clearlog
.DELETE_ON_ERROR:

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

# Creates a separate rule for each .o file
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

run: $(BIN)
	$(EXEC_BIN) $(PROGRAM_ARGS)

debug: $(BIN)
	$(EXEC_ENV) gdb --args ./$(BIN) $(PROGRAM_ARGS)

valgrind: $(BIN)
	valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes -s env $(EXEC_BIN) $(PROGRAM_ARGS)

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
	journalctl --no-hostname --no-pager -r -t $(PROGRAM_NAME)

# WARNING: this clears all logs in the system,
# because it is not possible to clear only relevant ones.
clearlog:
	sudo journalctl --rotate
	sudo journalctl --vacuum-time 1s

# Kills the daemon
# Apply a little protection, only kill if the process is started by current user.
kill:
	killall -i -u $(USER) $(BIN)
