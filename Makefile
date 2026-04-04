CC = gcc
CFLAGS = -Wall -Wextra -pedantic
TARGET = gooSQL

SRC = \
	main.c \
	lang/Lexer.c \
	lang/Parser.c \
	lang/Ast.c \
	runtime/Database.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
