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
LIB_INSTALL_DIR:=/usr/local/lib/x86_64-linux-gnu
BIN_INSTALL_DIR:=/usr/local/bin
LIBS:=$(wildcard lib/*.so)
INSTALLED_LIBS:=$(LIBS:lib/%=$(LIB_INSTALL_DIR)/%)

.DELETE_ON_ERROR:

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

# Creates a separate rule for each .o file
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.PHONY: run
run: $(BIN)
	$(EXEC_BIN) $(PROGRAM_ARGS)

.PHONY: debug
debug: $(BIN)
	$(EXEC_ENV) gdb --args ./$(BIN) $(PROGRAM_ARGS)

.PHONY: valgrind
valgrind: $(BIN)
	valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes \
	-s env $(EXEC_BIN) $(PROGRAM_ARGS)

# compile_commands.json is needed for clangd to work
# we generate them using bear:
# https://github.com/rizsotto/Bear
# Clean build artifacts first to force make to run the build commands.
.PHONY: clangd
clangd: clean
	bear -- $(MAKE)

.PHONY: clean
clean:
	$(RM) $(OBJS) $(BIN) $(ASSEMBLY_FILES)

# Produces .s files.
.PHONY: check
check:
	gcc $(CPPFLAGS) $(CFLAGS) -fanalyzer -S $(SRCS)

.PHONY: install
install: $(BIN)
	sudo mkdir -p $(LIB_INSTALL_DIR) $(BIN_INSTALL_DIR)
	sudo cp $(LIBS) $(LIB_INSTALL_DIR)
	sudo cp $(BIN) $(BIN_INSTALL_DIR)
	sudo ldconfig

.PHONY: uninstall
uninstall:
	sudo rm -f $(INSTALLED_LIBS) $(BIN_INSTALL_DIR)/$(BIN)
	sudo ldconfig

.PHONY: format
format:
	clang-format -i --style=file --verbose $(SRCS) $(HEADERS)

.PHONY: cppcheck
cppcheck:
	cppcheck --enable=all .

.PHONY: showlog
showlog:
	journalctl --no-hostname --no-pager -r -t $(PROGRAM_NAME)

# WARNING: this clears all logs in the system,
# because it is not possible to clear only relevant ones.
.PHONY: clearlog
clearlog:
	sudo journalctl --rotate
	sudo journalctl --vacuum-time 1s

# Kills the daemon
# Apply a little protection, only kill if the process is started by current user.
.PHONY: kill
kill:
	killall -i -u $(USER) $(BIN)
