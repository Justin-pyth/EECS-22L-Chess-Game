/*
 * chess_gui.c — GTK3 graphical front-end for Anteater Chess
 *
 * Compile alongside Chess.c, Moves.c, Engine.c:
 *   gcc -o chess chess_gui.c Chess.c Moves.c Engine.c \
 *       $(pkg-config --cflags --libs gtk+-3.0) -lm -O2
 *
 * This file replaces the terminal I/O in Chess.c's main().
 * All game logic is untouched — only presentation changes.
 */

#define _USE_MATH_DEFINES
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include "types.h"
#include "Moves.h"
/* Engine.h defines MAX/MIN; undef glib's first to silence redefinition warnings */
#ifdef MAX
#undef MAX
#endif
#ifdef MIN
#undef MIN
#endif
#include "Engine.h"

/* ═══════════════════════════════════════════════════════════
   Forward declarations from Chess.c (not in headers)
   ═══════════════════════════════════════════════════════════ */
extern void initializeBoard(struct piece* board[8][10]);
extern void initGameState(struct gameState* state);
extern bool isKingInCheck(struct piece* board[8][10], enum pieceColor color);
extern bool isCheckmate(struct gameState* gs);
extern bool isStalemate(struct gameState* gs);
extern struct piece* allocatePromotion(enum pieceType type, enum pieceColor color);

/* ═══════════════════════════════════════════════════════════
   Constants & palette
   ═══════════════════════════════════════════════════════════ */
#define SQUARE_SIZE   72
#define BOARD_COLS    10
#define BOARD_ROWS    8
#define BOARD_W       (BOARD_COLS * SQUARE_SIZE)
#define BOARD_H       (BOARD_ROWS * SQUARE_SIZE)
#define LOG_WIDTH     280
#define SIDEBAR_W     240
#define WINDOW_W      (BOARD_W + LOG_WIDTH + SIDEBAR_W + 32)
#define WINDOW_H      (BOARD_H + 120)
#define MAX_LOG_LINES 512

/* Colour palette */
#define COL_LIGHT_SQ   0.93, 0.87, 0.76   /* warm cream  */
#define COL_DARK_SQ    0.46, 0.29, 0.19   /* walnut      */
#define COL_SELECT     0.20, 0.75, 0.30   /* vivid green */
#define COL_LEGAL      0.20, 0.65, 0.85   /* sky blue    */
#define COL_LASTMOVE   0.85, 0.75, 0.20   /* gold        */
#define COL_CHECK      0.90, 0.15, 0.15   /* red         */
#define COL_BG         0.10, 0.10, 0.13   /* near-black  */
#define COL_PANEL      0.14, 0.14, 0.18
#define COL_BORDER     0.25, 0.22, 0.18

/* ═══════════════════════════════════════════════════════════
   Application state
   ═══════════════════════════════════════════════════════════ */
typedef struct {
    struct gameState gs;

    /* Selection */
    int  selRow, selCol;          /* -1 if nothing selected */
    bool hasSel;

    /* Legal moves for selected piece */
    Move legalMoves[MAX_MOVES];
    int  legalCount;

    /* Last move highlight */
    int lastFR, lastFC, lastTR, lastTC;
    bool hasLast;

    /* Game mode */
    enum gameMode mode;
    enum pieceColor humanColor;
    int aiDifficulty;

    /* Game over */
    bool gameOver;
    char gameOverMsg[128];

    /* Promotion pending */
    bool promoWaiting;
    int  promoFromR, promoFromC, promoToR, promoToC;
    Move promoMoves[5];           /* one per promotion type */

    /* Undo stack */
    struct MoveUndo undoStack[512];
    int  undoDepth;

    /* Move log */
    char logLines[MAX_LOG_LINES][64];
    int  logCount;
    int  fullMove;

    /* Widgets */
    GtkWidget* window;
    GtkWidget* boardArea;
    GtkWidget* logView;
    GtkTextBuffer* logBuf;
    GtkWidget* statusLabel;
    GtkWidget* undoBtn;
    GtkWidget* newGameBtn;

    /* AI idle source */
    guint aiSource;
} AppState;

static AppState app;

/* ═══════════════════════════════════════════════════════════
   Helpers
   ═══════════════════════════════════════════════════════════ */

static const char* pieceName(enum pieceType t) {
    switch (t) {
        case KING:     return "King";
        case QUEEN:    return "Queen";
        case ROOK:     return "Rook";
        case BISHOP:   return "Bishop";
        case KNIGHT:   return "Knight";
        case ANT:      return "Ant";
        case ANTEATER: return "Anteater";
    }
    return "?";
}

/* Convert row (0-based, 0=rank1) to board-display row (white at bottom) */
static int displayRow(int row) { return BOARD_ROWS - 1 - row; }

static void clearSelection(void) {
    app.hasSel = false;
    app.selRow = app.selCol = -1;
    app.legalCount = 0;
}

/* Compute legal moves for the piece at (row,col) */
static void computeLegalForPiece(int row, int col) {
    app.legalCount = 0;
    Move all[MAX_MOVES];
    int cnt = 0;
    getMoves(&app.gs, all, &cnt);
    for (int i = 0; i < cnt; i++) {
        if (getFromRow(all[i]) == row && getFromCol(all[i]) == col)
            app.legalMoves[app.legalCount++] = all[i];
    }
}

static bool isLegalDest(int row, int col) {
    for (int i = 0; i < app.legalCount; i++)
        if (getToRow(app.legalMoves[i]) == row && getToCol(app.legalMoves[i]) == col)
            return true;
    return false;
}

/* Collect all moves going to (row,col) from current selection */
static int movesTo(int row, int col, Move* out) {
    int n = 0;
    for (int i = 0; i < app.legalCount; i++)
        if (getToRow(app.legalMoves[i]) == row && getToCol(app.legalMoves[i]) == col)
            out[n++] = app.legalMoves[i];
    return n;
}

/* ───── Log ───── */
static void appendLog(const char* line) {
    if (app.logCount >= MAX_LOG_LINES) return;
    strncpy(app.logLines[app.logCount++], line, 63);
    /* Append to text buffer */
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(app.logBuf, &end);
    gtk_text_buffer_insert(app.logBuf, &end, line, -1);
    gtk_text_buffer_insert(app.logBuf, &end, "\n", -1);
    /* Scroll to bottom */
    gtk_text_buffer_get_end_iter(app.logBuf, &end);
    GtkTextMark* m = gtk_text_buffer_get_insert(app.logBuf);
    gtk_text_buffer_place_cursor(app.logBuf, &end);
    if (app.logView)
        gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(app.logView), m);
}

static const char fileChar[10] = {'a','b','c','d','e','f','g','h','i','j'};

static void logMove(Move m, struct piece* movedPiece, bool capture, bool check, bool checkmate) {
    char buf[64];
    int fr = getFromRow(m), fc = getFromCol(m);
    int tr = getToRow(m),   tc = getToCol(m);
    int flags = getFlags(m);

    char fromSq[4], toSq[4];
    snprintf(fromSq, sizeof(fromSq), "%c%d", fileChar[fc], fr+1);
    snprintf(toSq,   sizeof(toSq),   "%c%d", fileChar[tc], tr+1);

    const char* pn = movedPiece ? pieceName(movedPiece->piece) : "?";
    const char* cap = capture ? "x" : "-";
    const char* suf = checkmate ? "#" : check ? "+" : "";
    const char* spc = "";
    if (flags == MOVE_CASTLE_KS) spc = " [O-O]";
    else if (flags == MOVE_CASTLE_QS) spc = " [O-O-O]";
    else if (flags == MOVE_EN_PASSANT) spc = " [e.p.]";
    else if (flags == MOVE_ANTEATING)  spc = " [eat]";

    const char* promoStr = "";
    char protmp[16] = "";
    if (flags >= MOVE_PROMO_QUEEN && flags <= MOVE_PROMO_ANTEATER) {
        const char* names[] = {"=Q","=R","=B","=N","=A"};
        promoStr = names[flags - MOVE_PROMO_QUEEN];
    }

    if (movedPiece && movedPiece->color == WHITE) {
        snprintf(buf, sizeof(buf), "%d. %s %s%s%s%s%s%s",
                 app.fullMove, pn, fromSq, cap, toSq, promoStr, spc, suf);
    } else {
        snprintf(buf, sizeof(buf), "   ... %s %s%s%s%s%s%s",
                 pn, fromSq, cap, toSq, promoStr, spc, suf);
        app.fullMove++;
    }
    appendLog(buf);
}

/* ───── Status label ───── */
static void updateStatus(void) {
    if (app.gameOver) {
        gtk_label_set_text(GTK_LABEL(app.statusLabel), app.gameOverMsg);
        return;
    }
    const char* player = (app.gs.currentPlayer == WHITE) ? "White" : "Black";
    bool check = isKingInCheck(app.gs.board, app.gs.currentPlayer);
    char buf[128];
    if (check)
        snprintf(buf, sizeof(buf), "⚠ %s's turn — King in CHECK!", player);
    else
        snprintf(buf, sizeof(buf), "%s's turn", player);
    gtk_label_set_text(GTK_LABEL(app.statusLabel), buf);
}

static void redraw(void) {
    if (app.boardArea) gtk_widget_queue_draw(app.boardArea);
    updateStatus();
}

/* ═══════════════════════════════════════════════════════════
   Drawing helpers — Unicode chess symbols
   ═══════════════════════════════════════════════════════════ */

/* White piece Unicode: ♔♕♖♗♘♙ | Black: ♚♛♜♝♞♟
   Anteater: use Ⓐ/ⓐ as a custom stand-in */
static const char* pieceSymbol(enum pieceType t, enum pieceColor c) {
    if (c == WHITE) {
        switch (t) {
            case KING:     return "♔";
            case QUEEN:    return "♕";
            case ROOK:     return "♖";
            case BISHOP:   return "♗";
            case KNIGHT:   return "♘";
            case ANT:      return "♙";
            case ANTEATER: return "Ⓐ";
        }
    } else {
        switch (t) {
            case KING:     return "♚";
            case QUEEN:    return "♛";
            case ROOK:     return "♜";
            case BISHOP:   return "♝";
            case KNIGHT:   return "♞";
            case ANT:      return "♟";
            case ANTEATER: return "ⓐ";
        }
    }
    return "?";
}

/* Draw a single square background */
static void drawSquareBG(cairo_t* cr, int drow, int col,
                         bool selected, bool legalDest,
                         bool lastMove, bool inCheck) {
    double x = col * SQUARE_SIZE;
    double y = drow * SQUARE_SIZE;

    /* base tile colour */
    bool light = ((drow + col) % 2 == 0);
    if (light) cairo_set_source_rgb(cr, COL_LIGHT_SQ);
    else        cairo_set_source_rgb(cr, COL_DARK_SQ);
    cairo_rectangle(cr, x, y, SQUARE_SIZE, SQUARE_SIZE);
    cairo_fill(cr);

    /* overlays */
    if (inCheck) {
        cairo_set_source_rgba(cr, COL_CHECK, 0.55);
        cairo_rectangle(cr, x, y, SQUARE_SIZE, SQUARE_SIZE);
        cairo_fill(cr);
    }
    if (lastMove) {
        cairo_set_source_rgba(cr, COL_LASTMOVE, 0.45);
        cairo_rectangle(cr, x, y, SQUARE_SIZE, SQUARE_SIZE);
        cairo_fill(cr);
    }
    if (selected) {
        cairo_set_source_rgba(cr, COL_SELECT, 0.60);
        cairo_rectangle(cr, x, y, SQUARE_SIZE, SQUARE_SIZE);
        cairo_fill(cr);
    }
    if (legalDest) {
        /* subtle dot */
        double cx = x + SQUARE_SIZE / 2.0;
        double cy = y + SQUARE_SIZE / 2.0;
        cairo_set_source_rgba(cr, COL_LEGAL, 0.70);
        cairo_arc(cr, cx, cy, SQUARE_SIZE * 0.15, 0, 2 * G_PI);
        cairo_fill(cr);
    }
}

/* Draw piece glyph using Pango */
static void drawPiece(cairo_t* cr, GtkWidget* widget,
                      struct piece* p, int drow, int col) {
    if (!p) return;
    double x = col * SQUARE_SIZE;
    double y = drow * SQUARE_SIZE;

    PangoLayout* layout = gtk_widget_create_pango_layout(widget, NULL);
    PangoFontDescription* fd = pango_font_description_from_string("Segoe UI Symbol 36");
    pango_layout_set_font_description(layout, fd);
    pango_layout_set_text(layout, pieceSymbol(p->piece, p->color), -1);

    int pw, ph;
    pango_layout_get_pixel_size(layout, &pw, &ph);

    double tx = x + (SQUARE_SIZE - pw) / 2.0;
    double ty = y + (SQUARE_SIZE - ph) / 2.0;

    /* shadow */
    cairo_set_source_rgba(cr, 0, 0, 0, 0.35);
    cairo_move_to(cr, tx + 1.5, ty + 1.5);
    pango_cairo_show_layout(cr, layout);

    /* glyph */
    if (p->color == WHITE)
        cairo_set_source_rgb(cr, 0.97, 0.95, 0.88);
    else
        cairo_set_source_rgb(cr, 0.10, 0.08, 0.06);
    cairo_move_to(cr, tx, ty);
    pango_cairo_show_layout(cr, layout);

    pango_font_description_free(fd);
    g_object_unref(layout);
}

/* Draw rank/file labels */
static void drawLabels(cairo_t* cr, GtkWidget* widget) {
    PangoLayout* layout = gtk_widget_create_pango_layout(widget, NULL);
    PangoFontDescription* fd = pango_font_description_from_string("Monospace Bold 9");
    pango_layout_set_font_description(layout, fd);

    for (int r = 0; r < BOARD_ROWS; r++) {
        char buf[4]; snprintf(buf, sizeof(buf), "%d", r + 1);
        pango_layout_set_text(layout, buf, -1);
        int drow = displayRow(r);
        cairo_set_source_rgba(cr, 0.6, 0.5, 0.4, 0.9);
        cairo_move_to(cr, 3, drow * SQUARE_SIZE + 3);
        pango_cairo_show_layout(cr, layout);
    }
    for (int c = 0; c < BOARD_COLS; c++) {
        char buf[4]; snprintf(buf, sizeof(buf), "%c", fileChar[c]);
        pango_layout_set_text(layout, buf, -1);
        cairo_set_source_rgba(cr, 0.6, 0.5, 0.4, 0.9);
        cairo_move_to(cr, c * SQUARE_SIZE + SQUARE_SIZE - 11,
                          BOARD_H - 13);
        pango_cairo_show_layout(cr, layout);
    }
    pango_font_description_free(fd);
    g_object_unref(layout);
}

/* ═══════════════════════════════════════════════════════════
   Promotion dialog
   ═══════════════════════════════════════════════════════════ */
static enum pieceType showPromotionDialog(void) {
    GtkWidget* dlg = gtk_dialog_new_with_buttons(
        "Promote Ant",
        GTK_WINDOW(app.window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        NULL, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 340, 120);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(box), 16);

    struct { const char* label; enum pieceType type; int resp; } opts[] = {
        { "♕ Queen",     QUEEN,    1 },
        { "♖ Rook",      ROOK,     2 },
        { "♗ Bishop",    BISHOP,   3 },
        { "♘ Knight",    KNIGHT,   4 },
        { "Ⓐ Anteater",  ANTEATER, 5 },
    };
    for (int i = 0; i < 5; i++) {
        GtkWidget* btn = gtk_button_new_with_label(opts[i].label);
        g_object_set_data(G_OBJECT(btn), "resp", GINT_TO_POINTER(opts[i].resp));
        g_signal_connect_swapped(btn, "clicked",
                                 G_CALLBACK(gtk_dialog_response), dlg);
        /* Each button stores its own response */
        gtk_widget_set_name(btn, opts[i].label);
        /* Use response through dialog_response: work around by tag */
        g_object_set_data(G_OBJECT(dlg), opts[i].label, GINT_TO_POINTER(opts[i].type));
        gtk_box_pack_start(GTK_BOX(box), btn, TRUE, TRUE, 0);
        /* Wire response id manually */
        GtkWidget* area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
        (void)area;
    }

    /* Simpler: use a local enum result via button callbacks */
    GtkWidget* area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_box_pack_start(GTK_BOX(GTK_BOX(area)), box, TRUE, TRUE, 0);
    gtk_widget_show_all(dlg);

    /* We'll track which was clicked by response integer */
    /* Re-wire: connect each button to gtk_dialog_response with id */
    GList* children = gtk_container_get_children(GTK_CONTAINER(box));
    int id = 1;
    for (GList* l = children; l; l = l->next, id++) {
        g_signal_handlers_disconnect_matched(l->data, G_SIGNAL_MATCH_FUNC,
                                             0, 0, NULL,
                                             G_CALLBACK(gtk_dialog_response), NULL);
        gtk_dialog_add_action_widget(GTK_DIALOG(dlg), GTK_WIDGET(l->data), id);
    }
    g_list_free(children);

    gint response = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);

    switch (response) {
        case 1: return QUEEN;
        case 2: return ROOK;
        case 3: return BISHOP;
        case 4: return KNIGHT;
        case 5: return ANTEATER;
        default: return QUEEN;
    }
}

/* ═══════════════════════════════════════════════════════════
   New Game dialog
   ═══════════════════════════════════════════════════════════ */
static bool showNewGameDialog(void) {
    GtkWidget* dlg = gtk_dialog_new_with_buttons(
        "New Game — Anteater Chess",
        GTK_WINDOW(app.window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Start",  GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 360, 220);

    GtkWidget* area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 18);
    gtk_box_pack_start(GTK_BOX(area), grid, TRUE, TRUE, 0);

    /* Mode */
    GtkWidget* lMode = gtk_label_new("Game Mode:");
    gtk_widget_set_halign(lMode, GTK_ALIGN_START);
    GtkWidget* cbMode = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cbMode), "Human vs Human");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cbMode), "Human vs AI");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cbMode), "AI vs AI");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cbMode), 1);
    gtk_grid_attach(GTK_GRID(grid), lMode,  0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cbMode, 1, 0, 1, 1);

    /* Color */
    GtkWidget* lCol = gtk_label_new("Play as:");
    gtk_widget_set_halign(lCol, GTK_ALIGN_START);
    GtkWidget* cbCol = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cbCol), "White");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cbCol), "Black");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cbCol), 0);
    gtk_grid_attach(GTK_GRID(grid), lCol,  0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cbCol, 1, 1, 1, 1);

    /* Difficulty */
    GtkWidget* lDif = gtk_label_new("AI Difficulty:");
    gtk_widget_set_halign(lDif, GTK_ALIGN_START);
    GtkWidget* cbDif = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cbDif), "Easy");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cbDif), "Medium");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cbDif), "Hard");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cbDif), 1);
    gtk_grid_attach(GTK_GRID(grid), lDif,  0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cbDif, 1, 2, 1, 1);

    gtk_widget_show_all(dlg);
    gint resp = gtk_dialog_run(GTK_DIALOG(dlg));

    if (resp == GTK_RESPONSE_OK) {
        int modeIdx = gtk_combo_box_get_active(GTK_COMBO_BOX(cbMode));
        int colIdx  = gtk_combo_box_get_active(GTK_COMBO_BOX(cbCol));
        int difIdx  = gtk_combo_box_get_active(GTK_COMBO_BOX(cbDif));
        app.mode        = (enum gameMode)modeIdx;
        app.humanColor  = (colIdx == 0) ? WHITE : BLACK;
        app.aiDifficulty = difIdx;
    }

    gtk_widget_destroy(dlg);
    return (resp == GTK_RESPONSE_OK);
}

/* ═══════════════════════════════════════════════════════════
   Game state helpers
   ═══════════════════════════════════════════════════════════ */

static void checkGameOver(void) {
    if (isCheckmate(&app.gs)) {
        const char* winner = (app.gs.currentPlayer == WHITE) ? "Black" : "White";
        snprintf(app.gameOverMsg, sizeof(app.gameOverMsg),
                 "♛ Checkmate! %s wins.", winner);
        app.gameOver = true;
        appendLog(app.gameOverMsg);
    } else if (isStalemate(&app.gs)) {
        snprintf(app.gameOverMsg, sizeof(app.gameOverMsg), "½ Stalemate — Draw.");
        app.gameOver = true;
        appendLog(app.gameOverMsg);
    } else if (app.gs.halfMove_count >= 100) {
        snprintf(app.gameOverMsg, sizeof(app.gameOverMsg), "½ Draw — 50-move rule.");
        app.gameOver = true;
        appendLog(app.gameOverMsg);
    }
}

static bool isHumanTurn(void) {
    if (app.mode == HUMAN_VS_HUMAN) return true;
    if (app.mode == AI_VS_AI)       return false;
    return (app.gs.currentPlayer == app.humanColor);
}

/* ───── Execute a move (human or AI) ───── */
static void executeMove(Move m) {
    struct piece* moving = app.gs.board[getFromRow(m)][getFromCol(m)];
    bool cap = (app.gs.board[getToRow(m)][getToCol(m)] != NULL)
               || getFlags(m) == MOVE_EN_PASSANT
               || getFlags(m) == MOVE_ANTEATING;

    /* Save undo */
    if (app.undoDepth < 512)
        applyMove(&app.gs, m, &app.undoStack[app.undoDepth++]);
    else
        applyMove(&app.gs, m, NULL);

    storePositionHash(&app.gs);

    app.hasLast = true;
    app.lastFR = getFromRow(m); app.lastFC = getFromCol(m);
    app.lastTR = getToRow(m);   app.lastTC = getToCol(m);

    bool chk  = isKingInCheck(app.gs.board, app.gs.currentPlayer);
    bool chkm = isCheckmate(&app.gs);
    logMove(m, moving, cap, chk, chkm);

    checkGameOver();
    clearSelection();
    redraw();
}

/* ═══════════════════════════════════════════════════════════
   AI move in idle callback (non-blocking)
   ═══════════════════════════════════════════════════════════ */
static gboolean aiMoveIdle(gpointer data) {
    (void)data;
    app.aiSource = 0;
    if (app.gameOver || isHumanTurn()) return G_SOURCE_REMOVE;

    /* Temporarily show "thinking" */
    gtk_label_set_text(GTK_LABEL(app.statusLabel), "⚙ AI thinking…");
    while (gtk_events_pending()) gtk_main_iteration();

    int depth = 0;
    switch (app.aiDifficulty) {
        case 0: depth = (rand() % 2) + 1; break;
        case 1: depth = (rand() % 2) + 3; break;
        case 2: depth = 50; break;
        default: depth = 3;
    }
    Move best = findBestMove(&app.gs, depth);
    if (best) executeMove(best);

    /* If still AI's turn (AI vs AI), schedule again */
    if (!app.gameOver && !isHumanTurn()) {
        app.aiSource = g_timeout_add(300, aiMoveIdle, NULL);
    }
    return G_SOURCE_REMOVE;
}

static void scheduleAI(void) {
    if (app.aiSource) return;
    if (!app.gameOver && !isHumanTurn())
        app.aiSource = g_timeout_add(150, aiMoveIdle, NULL);
}

/* ═══════════════════════════════════════════════════════════
   New game
   ═══════════════════════════════════════════════════════════ */
static void startNewGame(void) {
    if (app.aiSource) { g_source_remove(app.aiSource); app.aiSource = 0; }
    initGameState(&app.gs);
    resetRepetitionTracking();
    initializeBoard(app.gs.board);
    clearSelection();
    app.hasLast   = false;
    app.gameOver  = false;
    app.undoDepth = 0;
    app.logCount  = 0;
    app.fullMove  = 1;
    app.promoWaiting = false;
    gtk_text_buffer_set_text(app.logBuf, "", 0);

    appendLog("═══════ Anteater Chess ═══════");
    appendLog("Board: 8×10  |  Pieces: +Anteater");
    appendLog("──────────────────────────────");
    char modebuf[64];
    const char* modeNames[] = { "Human vs Human", "Human vs AI", "AI vs AI" };
    snprintf(modebuf, sizeof(modebuf), "Mode: %s", modeNames[app.mode]);
    appendLog(modebuf);
    if (app.mode == HUMAN_VS_AI) {
        const char* difNames[] = { "Easy", "Medium", "Hard" };
        char difbuf[48];
        snprintf(difbuf, sizeof(difbuf), "AI: %s  |  You: %s",
                 difNames[app.aiDifficulty],
                 app.humanColor == WHITE ? "White" : "Black");
        appendLog(difbuf);
    }
    appendLog("──────────────────────────────");

    redraw();
    scheduleAI();
}

/* ═══════════════════════════════════════════════════════════
   Board click handler
   ═══════════════════════════════════════════════════════════ */
static gboolean onBoardClick(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    (void)widget; (void)data;
    if (app.gameOver || !isHumanTurn()) return TRUE;

    int col  = (int)(event->x / SQUARE_SIZE);
    int drow = (int)(event->y / SQUARE_SIZE);
    int row  = displayRow(drow);

    if (col < 0 || col >= BOARD_COLS || row < 0 || row >= BOARD_ROWS) return TRUE;

    struct piece* clicked = app.gs.board[row][col];

    if (app.hasSel) {
        /* Try to move */
        if (isLegalDest(row, col)) {
            Move candidates[8];
            int nc = movesTo(row, col, candidates);
            if (nc == 0) { clearSelection(); redraw(); return TRUE; }

            /* Check if promotion */
            bool isPromo = false;
            for (int i = 0; i < nc; i++) {
                int f = getFlags(candidates[i]);
                if (f >= MOVE_PROMO_QUEEN && f <= MOVE_PROMO_ANTEATER) { isPromo = true; break; }
            }

            if (isPromo) {
                enum pieceType choice = showPromotionDialog();
                int pflag;
                switch (choice) {
                    case QUEEN:    pflag = MOVE_PROMO_QUEEN;    break;
                    case ROOK:     pflag = MOVE_PROMO_ROOK;     break;
                    case BISHOP:   pflag = MOVE_PROMO_BISHOP;   break;
                    case KNIGHT:   pflag = MOVE_PROMO_KNIGHT;   break;
                    case ANTEATER: pflag = MOVE_PROMO_ANTEATER; break;
                    default:       pflag = MOVE_PROMO_QUEEN;    break;
                }
                for (int i = 0; i < nc; i++) {
                    if (getFlags(candidates[i]) == pflag) {
                        executeMove(candidates[i]);
                        scheduleAI();
                        return TRUE;
                    }
                }
            } else {
                executeMove(candidates[0]);
                scheduleAI();
                return TRUE;
            }
        }

        /* Clicked same piece — deselect */
        if (clicked && clicked->color == app.gs.currentPlayer
            && app.selRow == row && app.selCol == col) {
            clearSelection();
            redraw();
            return TRUE;
        }
    }

    /* Select a piece */
    if (clicked && clicked->color == app.gs.currentPlayer) {
        app.hasSel  = true;
        app.selRow  = row;
        app.selCol  = col;
        computeLegalForPiece(row, col);
        redraw();
    } else {
        clearSelection();
        redraw();
    }
    return TRUE;
}

/* ═══════════════════════════════════════════════════════════
   Board draw handler
   ═══════════════════════════════════════════════════════════ */
static gboolean onBoardDraw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    (void)data;

    /* Board border */
    cairo_set_source_rgb(cr, COL_BORDER);
    cairo_rectangle(cr, 0, 0, BOARD_W, BOARD_H);
    cairo_fill(cr);

    /* Find king position if in check */
    int checkKingRow = -1, checkKingCol = -1;
    if (!app.gameOver) {
        bool chk = isKingInCheck(app.gs.board, app.gs.currentPlayer);
        if (chk) {
            for (int r = 0; r < 8; r++)
                for (int c = 0; c < 10; c++) {
                    struct piece* p = app.gs.board[r][c];
                    if (p && p->piece == KING && p->color == app.gs.currentPlayer) {
                        checkKingRow = r; checkKingCol = c;
                    }
                }
        }
    }

    for (int row = 0; row < BOARD_ROWS; row++) {
        int drow = displayRow(row);
        for (int col = 0; col < BOARD_COLS; col++) {
            bool sel      = app.hasSel && app.selRow == row && app.selCol == col;
            bool legal    = app.hasSel && isLegalDest(row, col);
            bool lastMv   = app.hasLast &&
                            ((row == app.lastFR && col == app.lastFC) ||
                             (row == app.lastTR && col == app.lastTC));
            bool inChk    = (row == checkKingRow && col == checkKingCol);

            drawSquareBG(cr, drow, col, sel, legal, lastMv, inChk);
            drawPiece(cr, widget, app.gs.board[row][col], drow, col);
        }
    }

    drawLabels(cr, widget);
    return FALSE;
}

/* ═══════════════════════════════════════════════════════════
   Button callbacks
   ═══════════════════════════════════════════════════════════ */
static void onUndoClicked(GtkButton* btn, gpointer data) {
    (void)btn; (void)data;
    if (app.gameOver) { app.gameOver = false; }
    if (app.undoDepth <= 0) return;

    /* In HvsAI, undo two plies so human gets their turn back */
    int pliesToUndo = (app.mode == HUMAN_VS_AI) ? 2 : 1;
    for (int i = 0; i < pliesToUndo && app.undoDepth > 0; i++) {
        app.undoDepth--;
        undoMove(&app.gs, &app.undoStack[app.undoDepth]);
    }

    /* Remove last log line(s) */
    for (int i = 0; i < pliesToUndo && app.logCount > 0; i++)
        app.logCount--;
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(app.logBuf, &start);
    gtk_text_buffer_get_end_iter(app.logBuf, &end);
    gtk_text_buffer_delete(app.logBuf, &start, &end);
    for (int i = 0; i < app.logCount; i++) {
        gtk_text_buffer_get_end_iter(app.logBuf, &end);
        gtk_text_buffer_insert(app.logBuf, &end, app.logLines[i], -1);
        gtk_text_buffer_insert(app.logBuf, &end, "\n", -1);
    }
    if (app.undoDepth > 0) {
        app.hasLast = true;
        /* Guess last move from undo stack top — not stored; just clear */
        app.hasLast = false;
    } else {
        app.hasLast = false;
    }
    clearSelection();
    redraw();
}

static void onNewGameClicked(GtkButton* btn, gpointer data) {
    (void)btn; (void)data;
    if (showNewGameDialog())
        startNewGame();
}

/* ═══════════════════════════════════════════════════════════
   CSS styling
   ═══════════════════════════════════════════════════════════ */
static void applyCSS(void) {
    GtkCssProvider* css = gtk_css_provider_new();
    const char* style =
        "window { background-color: #1a1a21; }"
        "label#status {"
        "  font-family: 'Georgia', serif;"
        "  font-size: 15px;"
        "  color: #e8d9b0;"
        "  padding: 6px 12px;"
        "}"
        "button {"
        "  background: linear-gradient(to bottom, #3a2e22, #261e15);"
        "  color: #e8d9b0;"
        "  border: 1px solid #5a4a38;"
        "  border-radius: 4px;"
        "  padding: 6px 14px;"
        "  font-family: 'Georgia', serif;"
        "  font-size: 13px;"
        "}"
        "button:hover {"
        "  background: linear-gradient(to bottom, #4a3e2e, #362818);"
        "}"
        "textview {"
        "  background-color: #111116;"
        "  color: #c8b896;"
        "  font-family: 'Courier New', monospace;"
        "  font-size: 12px;"
        "}"
        "scrolledwindow { background-color: #111116; }"
        "frame {"
        "  border: 1px solid #3a3028;"
        "}"
        "frame > label {"
        "  color: #a89070;"
        "  font-family: 'Georgia', serif;"
        "  font-size: 11px;"
        "}";
    gtk_css_provider_load_from_data(css, style, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
}

/* ═══════════════════════════════════════════════════════════
   Piece legend widget
   ═══════════════════════════════════════════════════════════ */
static GtkWidget* buildLegend(void) {
    GtkWidget* frame = gtk_frame_new("Pieces");
    GtkWidget* grid  = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);

    struct { const char* sym; const char* name; } pieces[] = {
        { "♔/♚", "King"     },
        { "♕/♛", "Queen"    },
        { "♖/♜", "Rook"     },
        { "♗/♝", "Bishop"   },
        { "♘/♞", "Knight"   },
        { "♙/♟", "Ant (Pawn)" },
        { "Ⓐ/ⓐ", "Anteater" },
    };

    for (int i = 0; i < 7; i++) {
        GtkWidget* sym  = gtk_label_new(pieces[i].sym);
        GtkWidget* name = gtk_label_new(pieces[i].name);
        gtk_widget_set_halign(sym,  GTK_ALIGN_START);
        gtk_widget_set_halign(name, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), sym,  0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), name, 1, i, 1, 1);
    }
    gtk_container_add(GTK_CONTAINER(frame), grid);
    return frame;
}

/* Anteater rules info */
static GtkWidget* buildAnteaterInfo(void) {
    GtkWidget* frame = gtk_frame_new("Anteater Rules");
    GtkWidget* tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(tv), 6);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(tv), 6);
    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_text_buffer_set_text(buf,
        "• Moves 1 square in any direction\n"
        "• Can only capture Ants (Pawns)\n"
        "• Does NOT threaten the King\n"
        "• Ant Eating: capturing an Ant chains to capture adjacent Ants along the same rank/file",
        -1);
    GtkWidget* sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(sw, -1, 110);
    gtk_container_add(GTK_CONTAINER(sw), tv);
    gtk_container_add(GTK_CONTAINER(frame), sw);
    return frame;
}

/* ═══════════════════════════════════════════════════════════
   Build the full UI
   ═══════════════════════════════════════════════════════════ */
static void buildUI(void) {
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Anteater Chess — Blunder Boys + Katrina");
    gtk_window_set_resizable(GTK_WINDOW(app.window), FALSE);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    applyCSS();

    /* Root horizontal box */
    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(root), 8);
    gtk_container_add(GTK_CONTAINER(app.window), root);

    /* ── Left column: board + status + buttons ── */
    GtkWidget* leftCol = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(root), leftCol, FALSE, FALSE, 0);

    /* Board drawing area */
    app.boardArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(app.boardArea, BOARD_W, BOARD_H);
    gtk_widget_add_events(app.boardArea, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(app.boardArea, "draw",           G_CALLBACK(onBoardDraw),  NULL);
    g_signal_connect(app.boardArea, "button-press-event", G_CALLBACK(onBoardClick), NULL);
    gtk_box_pack_start(GTK_BOX(leftCol), app.boardArea, FALSE, FALSE, 0);

    /* Status */
    app.statusLabel = gtk_label_new("Anteater Chess");
    gtk_widget_set_name(app.statusLabel, "status");
    gtk_widget_set_halign(app.statusLabel, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(leftCol), app.statusLabel, FALSE, FALSE, 2);

    /* Button row */
    GtkWidget* btnRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(btnRow, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(leftCol), btnRow, FALSE, FALSE, 2);

    app.newGameBtn = gtk_button_new_with_label("⊞ New Game");
    app.undoBtn    = gtk_button_new_with_label("↩ Undo");
    g_signal_connect(app.newGameBtn, "clicked", G_CALLBACK(onNewGameClicked), NULL);
    g_signal_connect(app.undoBtn,    "clicked", G_CALLBACK(onUndoClicked),    NULL);
    gtk_box_pack_start(GTK_BOX(btnRow), app.newGameBtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btnRow), app.undoBtn,    FALSE, FALSE, 0);

    /* ── Middle column: move log ── */
    GtkWidget* midCol = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(root), midCol, FALSE, FALSE, 0);

    GtkWidget* logFrame = gtk_frame_new("Move Log");
    gtk_box_pack_start(GTK_BOX(midCol), logFrame, TRUE, TRUE, 0);

    GtkWidget* logSW = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(logSW),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(logSW, LOG_WIDTH, BOARD_H + 60);
    gtk_container_add(GTK_CONTAINER(logFrame), logSW);

    app.logView = gtk_text_view_new();
    app.logBuf  = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app.logView));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app.logView), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(app.logView), FALSE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(app.logView), 8);
    gtk_container_add(GTK_CONTAINER(logSW), app.logView);

    /* ── Right column: legend + rules ── */
    GtkWidget* rightCol = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_size_request(rightCol, SIDEBAR_W, -1);
    gtk_box_pack_start(GTK_BOX(root), rightCol, FALSE, FALSE, 0);

    GtkWidget* legend = buildLegend();
    gtk_box_pack_start(GTK_BOX(rightCol), legend, FALSE, FALSE, 0);

    GtkWidget* anteaterInfo = buildAnteaterInfo();
    gtk_box_pack_start(GTK_BOX(rightCol), anteaterInfo, FALSE, FALSE, 0);

    /* Half-move clock display */
    char hmcbuf[64];
    snprintf(hmcbuf, sizeof(hmcbuf), "50-move rule: 0/100");
    GtkWidget* hmcLabel = gtk_label_new(hmcbuf);
    gtk_widget_set_name(hmcLabel, "status");
    gtk_box_pack_start(GTK_BOX(rightCol), hmcLabel, FALSE, FALSE, 6);

    gtk_widget_show_all(app.window);
}

/* ═══════════════════════════════════════════════════════════
   main
   ═══════════════════════════════════════════════════════════ */
int main(int argc, char* argv[]) {
    srand((unsigned)time(NULL));
    gtk_init(&argc, &argv);

    memset(&app, 0, sizeof(app));
    app.selRow = app.selCol = -1;
    app.mode         = HUMAN_VS_AI;
    app.humanColor   = WHITE;
    app.aiDifficulty = 1;

    buildUI();

    /* Show new-game dialog on startup */
    if (!showNewGameDialog()) {
        /* Default: HvAI medium */
    }
    startNewGame();

    gtk_main();
    return 0;
}