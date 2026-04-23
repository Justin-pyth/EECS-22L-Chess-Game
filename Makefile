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
GTK     = $(shell pkg-config --cflags --libs gtk+-3.0)
LIBS    = -lm

HEADERS = types.h Moves.h Engine.h

.PHONY: all chess_gui chess_term clean

all: chess_gui

# ── GUI build ─────────────────────────────────────────────────────────────────
# Uses core.c (Chess.c with main() behind #ifndef GUI_BUILD).
# gui.c provides the GTK3 entry point.
chess_gui: gui.c core.c Moves.c Engine.c $(HEADERS)
	$(CC) $(CFLAGS) -DGUI_BUILD $(GTK) \
	    -o chess_gui \
	    gui.c core.c Moves.c Engine.c \
	    $(LIBS)
	@echo "► chess_gui built successfully."

# ── Terminal build ────────────────────────────────────────────────────────────
# Uses the original Chess.c unchanged.
chess_term: Chess.c Moves.c Engine.c terminalTestingFunctions.c $(HEADERS) terminalTestingFunctions.h
	$(CC) $(CFLAGS) \
	    -o chess_term \
	    Chess.c Moves.c Engine.c terminalTestingFunctions.c \
	    $(LIBS)
	@echo "► chess_term built successfully."

clean:
	rm -f chess_gui chess_term *.o
	@echo "Cleaned."