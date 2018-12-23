WARNINGS := -Wall -Wextra -Werror -Wpedantic -Wcast-align -Wwrite-strings \
	-Wredundant-decls -Wconversion

CFLAGS := -std=c99 -g $(WARNINGS) $(CFLAGS)

TARGET := treef
PREFIX ?= /usr/local
BINDIR := $(DESTDIR)$(PREFIX)/bin

OBJ := $(patsubst %.c, %.o, $(wildcard *.c))

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(TARGET)
	$(RM) $(OBJ)
	$(RM) -r $(TARGET).dSYM

test: $(TARGET)
	./test.sh

install: $(TARGET)
	install -d $(BINDIR)
	install -s $(TARGET) $(BINDIR)

.PHONY: clean all test install
