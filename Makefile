WARNINGS := -Wall -Wextra -Werror -pedantic -Wcast-align -Wwrite-strings \
	-Wredundant-decls -Wconversion

CFLAGS := -MMD $(WARNINGS) $(CFLAGS)

TARGET := treef
PREFIX ?= /usr/local
BINDIR := $(DESTDIR)$(PREFIX)/bin

src := $(wildcard *.c)
obj := $(src:%.c=%.o)
dep := $(src:%.c=%.d)

all: $(TARGET)

-include $(dep)

$(TARGET): $(obj)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(TARGET)
	$(RM) -r $(TARGET).dSYM
	$(RM) $(obj)
	$(RM) $(dep)

test: $(TARGET)
	./test.sh

install: $(TARGET)
	install -d $(BINDIR)
	install -s $(TARGET) $(BINDIR)

.PHONY: clean all test install
