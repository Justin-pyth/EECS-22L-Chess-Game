# ─────────────────────────────────────────────────────────────────────────────
# Anteater Chess — Makefile
#
# Targets:
#   all / make   (default)  GUI build using Gui.c + Core.c
#   test                    Run the built executable
#   clean                   Remove object files and executable
#   tar                     Create the source package archive
#
# SETUP:
#   1. Place all source files and headers inside src/
#   2. Place documentation PDFs inside doc/
#   3. The executable will be generated in bin/
#   4. Run: make
#
# NOTES:
#   - This Makefile is configured for the GUI version only.
#   - Chess.c is not compiled, because Core.c already contains the
#     shared game logic used by the GUI build.
#   - -DGUI_BUILD disables Core.c's terminal main() automatically.
#
# REQUIREMENTS:
#   GTK+ 3.x dev libraries:
#     Ubuntu/Debian: sudo apt install libgtk-3-dev
#     Fedora/RHEL:   sudo dnf install gtk3-devel
#     macOS:         brew install gtk+3
#     MSYS2:         pacman -S mingw-w64-x86_64-gtk3
# ─────────────────────────────────────────────────────────────────────────────

CC      = gcc
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS   = $(shell pkg-config --libs gtk+-3.0)
# -DGUI_BUILD: disables terminal main() in Core.c and terminal-only code paths
CFLAGS  = -Wall -Wextra -O2 -std=c11 -DGUI_BUILD $(GTK_CFLAGS)
LIBS    = $(GTK_LIBS) -lm

BIN_DIR = bin
DOC_DIR = doc

TARGET  = $(BIN_DIR)/chess

HEADERS = types.h Moves.h Ant.h Eval.h Hash.h TT.h Engine.h

OBJ = Gui.o Core.o Moves.o Ant.o Eval.o Hash.o TT.o Engine.o

.PHONY: all test clean tar

all: $(TARGET)

# ── GUI build ─────────────────────────────────────────────────────────────────
$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) -o $(TARGET) $(OBJ) $(LIBS)

# ── Binary output directory ──────────────────────────────────────────────────
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# ── Object build rule ────────────────────────────────────────────────────────
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Test target ──────────────────────────────────────────────────────────────
test: $(TARGET)
	./$(TARGET)

# ── Clean target ─────────────────────────────────────────────────────────────
clean:
	rm -f *.o $(TARGET)
	@echo "Cleaned."

# ── Source package target ────────────────────────────────────────────────────
tar: clean
	cd .. && tar -czvf Chess_V1.0_src.tar.gz Chess_V1.0_src/