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
#   - Core.c's terminal main() should be disabled manually
#     (for example with #if 0 ... #endif) if still present.
#
# REQUIREMENTS:
#   GTK+ 3.x dev libraries:
#     Ubuntu/Debian: sudo apt install libgtk-3-dev
#     Fedora/RHEL:   sudo dnf install gtk3-devel
#     macOS:         brew install gtk+3  
# ─────────────────────────────────────────────────────────────────────────────

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11 -I src
LIBS    = -lm
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS   = $(shell pkg-config --libs gtk+-3.0)

BIN_DIR = bin
SRC_DIR = src
DOC_DIR = doc

TARGET  = $(BIN_DIR)/chess

HEADERS = $(SRC_DIR)/types.h \
          $(SRC_DIR)/Moves.h \
          $(SRC_DIR)/Ant.h \
          $(SRC_DIR)/Eval.h \
          $(SRC_DIR)/Hash.h \
          $(SRC_DIR)/TT.h \
          $(SRC_DIR)/Engine.h

OBJ = Gui.o Core.o Moves.o Ant.o Eval.o Hash.o TT.o Engine.o

.PHONY: all test clean tar

all: $(TARGET)

# ── GUI build ─────────────────────────────────────────────────────────────────
# Uses Gui.c as the entry point and Core.c for shared game logic.
# The final executable is generated in bin/chess to match the required package
# hierarchy for the source code release.
$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) \
	    -o $(TARGET) $(OBJ) \
	    $(GTK_LIBS) $(LIBS)

# ── Binary output directory ──────────────────────────────────────────────────
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# ── Object build rule ────────────────────────────────────────────────────────
%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

# ── Test target ──────────────────────────────────────────────────────────────
# Runs the generated executable from bin/
test: $(TARGET)
	./$(TARGET)

# ── Clean target ─────────────────────────────────────────────────────────────
clean:
	rm -f *.o $(TARGET)
	@echo "Cleaned."

# ── Source package target ────────────────────────────────────────────────────
# Creates the source code archive from the parent directory.
tar: clean
	cd .. && tar -czvf Chess_V1.0_src.tar.gz Chess_V1.0_src/
