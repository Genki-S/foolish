CC = gcc
CFLAGS = -W -Wall -Wextra -Wunreachable-code -Wstrict-prototypes -Wmissing-prototypes
# LDFLAGS: flags passed to the compiler for use during linking
LDFLAGS = -Wl
# LIBS: libraries to link with
LIBS = -lefence -lfl
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
EXE = foolish

all: $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

# Parsers
parse.tab.c parse.tab.h: parse.y
	bison -d $<

lex.yy.c: parse.l parse.tab.h
	flex $<

.PHONY: clean

clean:
	rm *.o
