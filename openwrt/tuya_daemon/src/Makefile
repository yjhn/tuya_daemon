BIN:=tuya_daemon
SRCS:=$(wildcard *.c)
HEADERS:=$(wildcard *.h)
OBJS:=$(SRCS:.c=.o)
CPPFLAGS:=-finput-charset=UTF-8
CFLAGS:=-std=gnu11 -Wall -Wextra -Wconversion -Wmissing-prototypes \
-Wstrict-prototypes -Wunused-parameter -Wuninitialized -Wshadow \
-Wbad-function-cast -Wcast-qual -Wdouble-promotion -Wformat=2 \
-Wformat-overflow=2 -Wformat-signedness -Wformat-truncation=2 \
-Wnull-dereference -Winit-self -Wmissing-include-dirs -Wswitch-default \
-Wstrict-overflow=4 -Wduplicated-branches -Wduplicated-cond \
-Wlogical-op -Wold-style-definition -Wvla -Wwrite-strings \
-Wunused-macros -Wundef -Wpointer-arith -Wfloat-equal -Walloc-zero -Wunused \
-Werror -Wno-error=unused -Wno-error=unused-macros -Wno-error=float-equal \
-Wno-error=unused-but-set-variable \
-funsigned-char -ftrapv \
-fhonour-copts -Os

LDLIBS:=-llink_core -lmiddleware_implementation -lplatform_port -lutils_modules \
-luci -lubus -lubox -lblobmsg_json

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

# Creates a separate rule for each .o file
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.PHONY: format
format:
	clang-format -i --style=file --verbose $(SRCS) $(HEADERS)

.PHONY: cppcheck
cppcheck:
	cppcheck --enable=all .
