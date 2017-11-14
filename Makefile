WARNINGS := -Wall -Wextra -Werror -Wpedantic -Wcast-align -Wwrite-strings \
	-Wredundant-decls -Wconversion

CFLAGS ?= -std=c99 -g $(WARNINGS)

TARGET = treef
PREFIX = /usr/local
BINDIR = $(DESTDIR)$(PREFIX)/bin

all: $(TARGET)

$(TARGET): treef.c
	@$(CC) $(CFLAGS) -o $@ $^

clean:
	@$(RM) $(TARGET)
	@$(RM) -r $(TARGET).dSYM

test: $(TARGET)
	@./test.sh

install: $(TARGET)
	@install -d $(BINDIR)
	@install -s $(TARGET) $(BINDIR)

.PHONY: clean all test install
