WARNINGS := -Wall -Wextra -Werror -Wpedantic -Wcast-align -Wwrite-strings \
	-Wredundant-decls -Wconversion

CFLAGS ?= -std=c99 -g $(WARNINGS)

TARGET = treef

all: $(TARGET)

$(TARGET): treef.c
	@$(CC) $(CFLAGS) -o $@ $^

clean:
	@$(RM) $(TARGET)
	@$(RM) -r $(TARGET).dSYM

test: $(TARGET)
	@./test.sh

.PHONY: clean all test
