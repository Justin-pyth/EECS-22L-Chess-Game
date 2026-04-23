CC = gcc
CFLAGS = -Wall -Wextra -O2
SRC = src/Chess.c src/Engine.c src/Moves.c src/terminalTestingFunctions.c
OBJ = Chess.o Engine.o Moves.o terminalTestingFunctions.o
BIN_DIR = bin
TARGET = $(BIN_DIR)/chess

GTK_CFLAGS := $(shell pkg-config --cflags gtk4 2>/dev/null)
GTK_LIBS   := $(shell pkg-config --libs   gtk4 2>/dev/null)

GUI_OBJ = Chess_gui.o Engine_gui.o Moves_gui.o gui_gtk.o
GUI_BIN = chess_gui

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
	rm -f *.o $(TARGET) $(GUI_BIN)

tar:
	cd .. && tar -czvf Chess_Alpha_src.tar.gz \
		--exclude='Chess_Alpha_src/*.o' \
		--exclude='Chess_Alpha_src/bin/chess' \
		Chess_Alpha_src/

Chess_gui.o: Chess.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -DGUI_BUILD -c $< -o $@

Engine_gui.o: Engine.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -DGUI_BUILD -c $< -o $@

Moves_gui.o: Moves.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -DGUI_BUILD -c $< -o $@

gui_gtk.o: gui.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -DGUI_BUILD -c $< -o $@

$(GUI_BIN): $(GUI_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(GTK_LIBS) -lm

gui: $(GUI_BIN)

run-gui: $(GUI_BIN)
	./$(GUI_BIN)

.PHONY: all test clean tar gui run-gui