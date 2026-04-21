CC = gcc
CFLAGS = -Wall -Wextra -O2
SRC = src/Chess.c src/Engine.c src/Moves.c src/terminalTestingFunctions.c
OBJ = Chess.o Engine.o Moves.o terminalTestingFunctions.o
BIN_DIR = bin
TARGET = $(BIN_DIR)/chess

all: $(TARGET)

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f *.o $(TARGET)
tar:
	cd .. && tar -czvf Chess_Alpha_src.tar.gz \
		--exclude='Chess_Alpha_src/*.o' \
		--exclude='Chess_Alpha_src/bin/chess' \
		Chess_Alpha_src/
