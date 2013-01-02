CC = gcc
CFLAGS =
# LDFLAGS: flags passed to the compiler for use during linking
LDFLAGS =
# LIBS: libraries to link with
LIBS = -lfl
SOURCES = command.c lex.yy.c mysh.c parse.tab.c path.c util.c
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

.PHONY: clean cleanest

clean:
	rm *.o

cleanest: clean
	rm $(EXE) parse.tab.*
