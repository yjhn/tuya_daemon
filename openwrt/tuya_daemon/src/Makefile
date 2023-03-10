BIN:=tuya_daemon
SRCS:=$(wildcard *.c)
HEADERS:=$(wildcard *.h)
OBJS:=$(SRCS:.c=.o)
# Garbage produced by make check.
ASSEMBLY_FILES:=$(wildcard *.s)
CFLAGS:=-std=gnu11 -Wall -Wextra -Wconversion -Wmissing-prototypes \
-Wstrict-prototypes -Wunused-parameter -Wuninitialized -Wshadow \
-Wbad-function-cast -Wcast-qual -Wdouble-promotion -Wformat=2 \
-Wformat-overflow=2 -Wformat-signedness -Wformat-truncation=2 \
-Wnull-dereference -Winit-self -Wmissing-include-dirs -Wswitch-default \
-fhonour-copts
LDLIBS:=-llink_core -lmiddleware_implementation -lplatform_port -lutils_modules \
-luci -lubus -lubox -lblobmsg_json

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

# Creates a separate rule for each .o file
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(OBJS) $(BIN) $(ASSEMBLY_FILES)

# Produces .s files.
.PHONY: check
check:
	gcc $(CPPFLAGS) $(CFLAGS) -fanalyzer -S $(SRCS)

.PHONY: format
format:
	clang-format -i --style=file --verbose $(SRCS) $(HEADERS)

.PHONY: cppcheck
cppcheck:
	cppcheck --enable=all .

.PHONY: clangd
clangd: clean
	bear -- $(MAKE)
