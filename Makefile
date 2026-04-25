# ─────────────────────────────────────────────────────────────────────────────
# Anteater Chess — Makefile
#
# Targets:
#   chess_gui   (default)  GUI build using gui.c + core.c
#   chess_term             Terminal build using Chess.c (original, unchanged)
#   clean
#
# SETUP:
#   1. Copy core.c alongside your existing Chess.c — don't delete Chess.c.
#      core.c is Chess.c with main() guarded by #ifndef GUI_BUILD.
#   2. Copy gui.c into the same directory.
#   3. Run: make chess_gui
#
# REQUIREMENTS:
#   GTK+ 3.x dev libraries:
#     Ubuntu/Debian: sudo apt install libgtk-3-dev
#     Fedora/RHEL:   sudo dnf install gtk3-devel
#     macOS:         brew install gtk+3
# ─────────────────────────────────────────────────────────────────────────────

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LIBS    = -lm
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS   = $(shell pkg-config --libs gtk+-3.0)

HEADERS = types.h Moves.h Ant.h Eval.h Hash.h TT.h Engine.h

.PHONY: all chess_gui chess_term clean

all: chess_gui

# ── GUI build ─────────────────────────────────────────────────────────────────
# Uses core.c (Chess.c with main() behind #ifndef GUI_BUILD).
# gui.c provides the GTK3 entry point.
chess_gui: Gui.c Core.c Moves.c Ant.c Eval.c Hash.c TT.c Engine.c $(HEADERS)
	$(CC) $(CFLAGS) -DGUI_BUILD $(GTK_CFLAGS) \
	    Gui.c Core.c Moves.c Ant.c Eval.c Hash.c TT.c Engine.c \
	    -o chess_gui \
	    $(GTK_LIBS) $(LIBS)

# ── Terminal build ────────────────────────────────────────────────────────────
# Uses the original Chess.c unchanged.
chess_term: Chess.c Moves.c Ant.c Eval.c Hash.c TT.c Engine.c terminalTestingFunctions.c $(HEADERS) terminalTestingFunctions.h
	$(CC) $(CFLAGS) \
	    -o chess_term \
	    Chess.c Moves.c Ant.c Eval.c Hash.c TT.c Engine.c terminalTestingFunctions.c \
	    $(LIBS)
	@echo "► chess_term built successfully."

clean:
	rm -f chess_gui chess_term *.o
	@echo "Cleaned."
