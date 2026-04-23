/*
 * gui.c  —  GTK4 front-end for Anteater Chess
 *
 * Build alongside Chess.c, Engine.c, Moves.c
 * (terminalTestingFunctions.c is NOT linked; we provide our own
 *  promptPromotion replacement via a GTK dialog.)
 *
 * Compile (example):
 *   gcc -Wall -O2 $(pkg-config --cflags gtk4) \
 *       gui.c Chess.c Engine.c Moves.c \
 *       $(pkg-config --libs gtk4) -lm -o anteater_chess
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "types.h"
#include "Moves.h"
#include "Engine.h"

/* ── Forward declarations from Chess.c ─────────────────────────── */
extern void         initializeBoard(struct piece* board[8][10]);
extern void         initGameState  (struct gameState* state);
extern bool         isCheckmate    (struct gameState* gs);
extern bool         isStalemate    (struct gameState* gs);
extern bool         isKingInCheck  (struct piece* board[8][10], enum pieceColor color);

/* ── Constants ─────────────────────────────────────────────────── */
#define SQUARE_SIZE   68
#define BOARD_COLS    10
#define BOARD_ROWS     8
#define SIDEBAR_W    220

/* ── Colours (RGBA as 0..1) ────────────────────────────────────── */
static const GdkRGBA COL_LIGHT      = {0.93, 0.85, 0.71, 1.0};
static const GdkRGBA COL_DARK       = {0.46, 0.31, 0.18, 1.0};
static const GdkRGBA COL_SELECT     = {0.20, 0.70, 0.30, 0.80};
static const GdkRGBA COL_HINT       = {0.25, 0.65, 0.95, 0.55};
static const GdkRGBA COL_CHECK      = {0.90, 0.15, 0.15, 0.65};

/* ── Piece Unicode glyphs ──────────────────────────────────────── */
/* White pieces: ♔♕♖♗♘♙   Black pieces: ♚♛♜♝♞♟ */
static const char* PIECE_GLYPH[2][7] = {
    /* WHITE */ { "♔", "♕", "♘", "♗", "♖", "♙", "🐜" },
    /* BLACK */ { "♚", "♛", "♞", "♝", "♜", "♟", "🐜" },
};
/* order matches enum pieceType: KING QUEEN KNIGHT BISHOP ROOK ANT ANTEATER */

/* ── Application state ─────────────────────────────────────────── */
typedef struct {
    struct gameState gs;

    /* Selection */
    int  selRow, selCol;       /* -1 = nothing selected            */
    bool hasSel;

    /* Legal move highlights for selected piece */
    bool hintMap[BOARD_ROWS][BOARD_COLS];

    /* All legal moves cache */
    Move legalMoves[MAX_MOVES];
    int  legalCount;

    /* Game mode */
    enum gameMode mode;
    enum pieceColor humanColor;
    int  aiDifficulty;

    /* State flags */
    bool gameOver;
    bool pendingPromo;          /* waiting for promotion dialog     */
    Move promoMove;             /* the ant-advance that needs promo */

    /* Move log (simple string list) */
    GtkWidget* logList;         /* GtkListBox                       */
    int        fullMove;

    /* Widgets we need to update */
    GtkWidget* boardArea;
    GtkWidget* statusLabel;
    GtkWidget* turnLabel;
    GtkWidget* undoButton;

    /* Undo stack — store last move's undo record */
    struct MoveUndo undoStack[256];
    int             undoTop;    /* index of next free slot          */
} AppState;

static AppState app;

/* ── Prototypes ────────────────────────────────────────────────── */
static void     refresh_board  (void);
static void     refresh_status (void);
static void     rebuild_hints  (int row, int col);
static void     clear_hints    (void);
static void     apply_move_gui (Move m);
static void     ai_move_async  (void);
static gboolean ai_move_idle   (gpointer data);
static void     append_log     (const char* text);
static void     show_game_over (const char* msg);

/* ── Helpers ───────────────────────────────────────────────────── */

static const char* glyph_for(const struct piece* p) {
    if (!p) return NULL;
    return PIECE_GLYPH[p->color == WHITE ? 0 : 1][p->piece];
}

static void set_rgba(cairo_t* cr, GdkRGBA c) {
    cairo_set_source_rgba(cr, c.red, c.green, c.blue, c.alpha);
}

/* Returns true if (row,col) is a hint square */
static bool is_hint(int row, int col) {
    return app.hintMap[row][col];
}

/* Returns true if the current player's king is on this square */
static bool is_king_in_check_sq(int row, int col) {
    struct piece* p = app.gs.board[row][col];
    if (!p || p->piece != KING || p->color != app.gs.currentPlayer) return false;
    return isKingInCheck((struct piece*(*)[10])app.gs.board, app.gs.currentPlayer);
}

/* Build app.legalMoves cache */
static void cache_legal_moves(void) {
    app.legalCount = 0;
    getMoves(&app.gs, app.legalMoves, &app.legalCount);
}

/* Rebuild hint map for piece at (row,col) */
static void rebuild_hints(int row, int col) {
    memset(app.hintMap, 0, sizeof(app.hintMap));
    for (int i = 0; i < app.legalCount; i++) {
        if (getFromRow(app.legalMoves[i]) == row &&
            getFromCol(app.legalMoves[i]) == col)
        {
            int tr = getToRow(app.legalMoves[i]);
            int tc = getToCol(app.legalMoves[i]);
            app.hintMap[tr][tc] = true;
        }
    }
}

static void clear_hints(void) {
    memset(app.hintMap, 0, sizeof(app.hintMap));
    app.hasSel = false;
    app.selRow = app.selCol = -1;
}

/* ── Board drawing ─────────────────────────────────────────────── */

static void draw_board(GtkDrawingArea* da, cairo_t* cr,
                        int width, int height, gpointer udata)
{
    (void)da; (void)width; (void)height; (void)udata;

    /* Draw each square */
    for (int row = 0; row < BOARD_ROWS; row++) {
        for (int col = 0; col < BOARD_COLS; col++) {
            int displayRow = BOARD_ROWS - 1 - row;  /* row 7 at top */
            double x = col * SQUARE_SIZE;
            double y = displayRow * SQUARE_SIZE;

            /* Base square colour */
            bool light = (row + col) % 2 == 0;
            set_rgba(cr, light ? COL_LIGHT : COL_DARK);
            cairo_rectangle(cr, x, y, SQUARE_SIZE, SQUARE_SIZE);
            cairo_fill(cr);

            /* Overlays */
            if (is_king_in_check_sq(row, col)) {
                set_rgba(cr, COL_CHECK);
                cairo_rectangle(cr, x, y, SQUARE_SIZE, SQUARE_SIZE);
                cairo_fill(cr);
            }
            if (app.hasSel && app.selRow == row && app.selCol == col) {
                set_rgba(cr, COL_SELECT);
                cairo_rectangle(cr, x, y, SQUARE_SIZE, SQUARE_SIZE);
                cairo_fill(cr);
            }
            if (is_hint(row, col)) {
                set_rgba(cr, COL_HINT);
                /* Draw a circle hint */
                struct piece* target = app.gs.board[row][col];
                if (target) {
                    /* Capture ring */
                    cairo_set_line_width(cr, 4.0);
                    cairo_arc(cr, x + SQUARE_SIZE/2.0, y + SQUARE_SIZE/2.0,
                              SQUARE_SIZE/2.0 - 4, 0, 2*G_PI);
                    cairo_stroke(cr);
                } else {
                    /* Move dot */
                    cairo_arc(cr, x + SQUARE_SIZE/2.0, y + SQUARE_SIZE/2.0,
                              SQUARE_SIZE/6.0, 0, 2*G_PI);
                    cairo_fill(cr);
                }
            }

            /* Piece glyph */
            struct piece* p = app.gs.board[row][col];
            if (p) {
                const char* glyph = glyph_for(p);
                PangoLayout* layout = pango_cairo_create_layout(cr);
                PangoFontDescription* fd =
                    pango_font_description_from_string("Noto Emoji, Segoe UI Emoji 34");
                pango_layout_set_font_description(layout, fd);
                pango_font_description_free(fd);
                pango_layout_set_text(layout, glyph, -1);

                /* Shadow for white pieces on light squares */
                int pw, ph;
                pango_layout_get_pixel_size(layout, &pw, &ph);
                double tx = x + (SQUARE_SIZE - pw) / 2.0;
                double ty = y + (SQUARE_SIZE - ph) / 2.0;

                /* Shadow */
                cairo_set_source_rgba(cr, 0, 0, 0, 0.45);
                cairo_move_to(cr, tx + 1.5, ty + 1.5);
                pango_cairo_show_layout(cr, layout);

                /* Piece */
                if (p->color == WHITE)
                    cairo_set_source_rgb(cr, 0.97, 0.97, 0.97);
                else
                    cairo_set_source_rgb(cr, 0.08, 0.08, 0.08);
                cairo_move_to(cr, tx, ty);
                pango_cairo_show_layout(cr, layout);
                g_object_unref(layout);
            }
        }
    }

    /* Rank/file labels */
    cairo_set_source_rgba(cr, 0.6, 0.5, 0.4, 1.0);
    PangoLayout* lbl = pango_cairo_create_layout(cr);
    PangoFontDescription* fd2 = pango_font_description_from_string("Monospace 9");
    pango_layout_set_font_description(lbl, fd2);
    pango_font_description_free(fd2);

    for (int col = 0; col < BOARD_COLS; col++) {
        char buf[4]; buf[0] = (char)('a' + col); buf[1] = '\0';
        pango_layout_set_text(lbl, buf, -1);
        cairo_move_to(cr, col * SQUARE_SIZE + 3,
                      BOARD_ROWS * SQUARE_SIZE - 14);
        pango_cairo_show_layout(cr, lbl);
    }
    for (int row = 0; row < BOARD_ROWS; row++) {
        char buf[4]; snprintf(buf, sizeof(buf), "%d", row + 1);
        pango_layout_set_text(lbl, buf, -1);
        int displayRow = BOARD_ROWS - 1 - row;
        cairo_move_to(cr, 2, displayRow * SQUARE_SIZE + 3);
        pango_cairo_show_layout(cr, lbl);
    }
    g_object_unref(lbl);
}

/* ── Refresh helpers ───────────────────────────────────────────── */

static void refresh_board(void) {
    gtk_widget_queue_draw(app.boardArea);
}

static void refresh_status(void) {
    const char* color = (app.gs.currentPlayer == WHITE) ? "White" : "Black";
    char buf[128];
    snprintf(buf, sizeof(buf), "%s's turn", color);
    gtk_label_set_text(GTK_LABEL(app.turnLabel), buf);
}

/* ── Move log ──────────────────────────────────────────────────── */

static void append_log(const char* text) {
    GtkWidget* row = gtk_list_box_row_new();
    GtkWidget* lbl = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    gtk_widget_add_css_class(lbl, "log-entry");
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), lbl);
    gtk_list_box_append(GTK_LIST_BOX(app.logList), row);
    /* Scroll to bottom */
    GtkAdjustment* adj = gtk_scrollable_get_vadjustment(
        GTK_SCROLLABLE(gtk_widget_get_parent(app.logList)));
    if (adj) gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
}

static void log_move(Move m, bool wasWhite) {
    int fc = getFromCol(m), fr = getFromRow(m);
    int tc = getToCol(m),   tr = getToRow(m);
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%d.  %c%d→%c%d",
             wasWhite ? "" : "    ",
             wasWhite ? app.fullMove : app.fullMove,
             'a'+fc, fr+1, 'a'+tc, tr+1);
    if (!wasWhite) app.fullMove++;
    append_log(buf);
}

/* ── Game over ─────────────────────────────────────────────────── */

static void show_game_over(const char* msg) {
    app.gameOver = true;
    gtk_label_set_text(GTK_LABEL(app.statusLabel), msg);

    GtkWidget* top = GTK_WIDGET(gtk_widget_get_root(app.boardArea));
    GtkAlertDialog* dlg = gtk_alert_dialog_new("%s", msg);
    gtk_alert_dialog_set_buttons(dlg, (const char*[]){"OK", NULL});
    gtk_alert_dialog_show(dlg, GTK_WINDOW(top));
    g_object_unref(dlg);
}

/* ── Check for end-of-game conditions ─────────────────────────── */

static void check_game_end(void) {
    if (isCheckmate(&app.gs)) {
        const char* winner = (app.gs.currentPlayer == WHITE) ? "Black" : "White";
        char msg[64]; snprintf(msg, sizeof(msg), "Checkmate! %s wins! 🎉", winner);
        show_game_over(msg);
    } else if (isStalemate(&app.gs)) {
        show_game_over("Stalemate — Draw!");
    } else if (app.gs.halfMove_count >= 100) {
        show_game_over("Draw by 50-move rule.");
    } else {
        if (isKingInCheck((struct piece*(*)[10])app.gs.board, app.gs.currentPlayer))
            gtk_label_set_text(GTK_LABEL(app.statusLabel), "⚠ Check!");
        else
            gtk_label_set_text(GTK_LABEL(app.statusLabel), "");
    }
}

/* ── Apply a fully-resolved move ──────────────────────────────── */

static void apply_move_gui(Move m) {
    bool wasWhite = (app.gs.currentPlayer == WHITE);
    /* Save undo */
    if (app.undoTop < 256)
        applyMove(&app.gs, m, &app.undoStack[app.undoTop++]);
    else
        applyMove(&app.gs, m, NULL);

    log_move(m, wasWhite);
    clear_hints();
    cache_legal_moves();
    refresh_status();
    refresh_board();
    check_game_end();
}

/* ── AI move (run in idle so UI doesn't freeze for shallow depth) */

static gboolean ai_move_idle(gpointer data) {
    (void)data;
    if (!app.gameOver)
        movePiece_Computer(&app.gs, app.aiDifficulty);

    bool wasWhite = (app.gs.currentPlayer == WHITE);   /* after move, player flipped */
    /* We logged the move inside movePiece_Computer indirectly — 
       approximate log entry */
    (void)wasWhite;
    clear_hints();
    cache_legal_moves();
    refresh_status();
    refresh_board();
    check_game_end();
    gtk_label_set_text(GTK_LABEL(app.statusLabel), "");
    return G_SOURCE_REMOVE;
}

static void ai_move_async(void) {
    gtk_label_set_text(GTK_LABEL(app.statusLabel), "AI thinking…");
    g_idle_add(ai_move_idle, NULL);
}

/* ── Promotion dialog ──────────────────────────────────────────── */

typedef struct { Move base; enum pieceType choice; GtkWindow* win; } PromoData;

static void promo_chosen(GtkButton* btn, gpointer udata) {
    PromoData* pd = (PromoData*)udata;
    const char* label = gtk_button_get_label(btn);
    if      (strcmp(label, "♕ Queen")    == 0) pd->choice = QUEEN;
    else if (strcmp(label, "♖ Rook")     == 0) pd->choice = ROOK;
    else if (strcmp(label, "♗ Bishop")   == 0) pd->choice = BISHOP;
    else if (strcmp(label, "♘ Knight")   == 0) pd->choice = KNIGHT;
    else if (strcmp(label, "🐜 Anteater") == 0) pd->choice = ANTEATER;
    gtk_window_close(pd->win);
}

/* Blocking-style: show dialog modally and return chosen piece */
static enum pieceType run_promo_dialog(GtkWindow* parent) {
    GtkWidget* dlg = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dlg), "Promote Ant");
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
    gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_window_set_child(GTK_WINDOW(dlg), box);

    GtkWidget* lbl = gtk_label_new("Choose promotion piece:");
    gtk_box_append(GTK_BOX(box), lbl);

    const char* choices[] = {
        "♕ Queen", "♖ Rook", "♗ Bishop", "♘ Knight", "🐜 Anteater"
    };

    PromoData pd = { .base = app.promoMove, .choice = QUEEN,
                     .win  = GTK_WINDOW(dlg) };

    for (int i = 0; i < 5; i++) {
        GtkWidget* btn = gtk_button_new_with_label(choices[i]);
        g_signal_connect(btn, "clicked", G_CALLBACK(promo_chosen), &pd);
        gtk_box_append(GTK_BOX(box), btn);
    }

    gtk_window_present(GTK_WINDOW(dlg));
    /* Spin the main loop until dialog is closed */
    while (gtk_widget_get_visible(dlg))
        g_main_context_iteration(NULL, TRUE);

    return pd.choice;
}

/* ── Square click handler ──────────────────────────────────────── */

static void on_square_clicked(GtkGestureClick* gesture, int n_press,
                               double x, double y, gpointer udata)
{
    (void)gesture; (void)n_press; (void)udata;
    if (app.gameOver) return;

    int col = (int)(x / SQUARE_SIZE);
    int row = BOARD_ROWS - 1 - (int)(y / SQUARE_SIZE);
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) return;

    /* Is it the human's turn? */
    bool humanTurn;
    if      (app.mode == HUMAN_VS_HUMAN) humanTurn = true;
    else if (app.mode == AI_VS_AI)       humanTurn = false;
    else    humanTurn = (app.gs.currentPlayer == app.humanColor);
    if (!humanTurn) return;

    struct piece* clicked = app.gs.board[row][col];

    if (!app.hasSel) {
        /* First click — select own piece */
        if (clicked && clicked->color == app.gs.currentPlayer) {
            app.hasSel  = true;
            app.selRow  = row;
            app.selCol  = col;
            rebuild_hints(row, col);
            refresh_board();
        }
        return;
    }

    /* Second click */
    if (clicked && clicked->color == app.gs.currentPlayer) {
        /* Re-select different own piece */
        app.selRow = row; app.selCol = col;
        rebuild_hints(row, col);
        refresh_board();
        return;
    }

    /* Attempt to move selRow/selCol → row/col */
    Move chosen = 0;
    bool isPromo = false;

    for (int i = 0; i < app.legalCount; i++) {
        Move mv = app.legalMoves[i];
        if (getFromRow(mv) == app.selRow && getFromCol(mv) == app.selCol &&
            getToRow(mv)   == row        && getToCol(mv)   == col)
        {
            int flags = getFlags(mv);
            if (flags >= MOVE_PROMO_QUEEN && flags <= MOVE_PROMO_ANTEATER) {
                isPromo = true;
                chosen  = mv;   /* store any promo flag; we'll replace it */
                break;
            }
            chosen = mv;
            break;
        }
    }

    if (!chosen) {
        /* Clicked empty/enemy non-reachable square — deselect */
        clear_hints();
        refresh_board();
        return;
    }

    if (isPromo) {
        /* Show promotion dialog */
        GtkWidget* root = GTK_WIDGET(gtk_widget_get_root(app.boardArea));
        enum pieceType pt = run_promo_dialog(GTK_WINDOW(root));
        int pf;
        switch (pt) {
            case QUEEN:    pf = MOVE_PROMO_QUEEN;    break;
            case ROOK:     pf = MOVE_PROMO_ROOK;     break;
            case BISHOP:   pf = MOVE_PROMO_BISHOP;   break;
            case KNIGHT:   pf = MOVE_PROMO_KNIGHT;   break;
            case ANTEATER: pf = MOVE_PROMO_ANTEATER; break;
            default:       pf = MOVE_PROMO_QUEEN;    break;
        }
        chosen = createMove(app.selRow, app.selCol, row, col, pf);
    }

    apply_move_gui(chosen);

    /* Trigger AI if needed */
    if (!app.gameOver) {
        bool aiTurn;
        if      (app.mode == HUMAN_VS_HUMAN) aiTurn = false;
        else if (app.mode == AI_VS_AI)       aiTurn = true;
        else     aiTurn = (app.gs.currentPlayer != app.humanColor);
        if (aiTurn) ai_move_async();
    }
}

/* ── Undo button ───────────────────────────────────────────────── */

static void on_undo_clicked(GtkButton* btn, gpointer udata) {
    (void)btn; (void)udata;
    if (app.undoTop == 0) return;
    app.undoTop--;
    undoMove(&app.gs, &app.undoStack[app.undoTop]);
    app.gameOver = false;
    clear_hints();
    cache_legal_moves();
    refresh_status();
    refresh_board();
    gtk_label_set_text(GTK_LABEL(app.statusLabel), "");
    /* Also undo AI half-move if HvAI */
    if (app.mode == HUMAN_VS_AI && app.undoTop > 0) {
        app.undoTop--;
        undoMove(&app.gs, &app.undoStack[app.undoTop]);
        cache_legal_moves();
        refresh_status();
        refresh_board();
    }
}

/* ── New game dialog ───────────────────────────────────────────── */

typedef struct {
    GtkWidget*  modeCombo;
    GtkWidget*  colorCombo;
    GtkWidget*  diffCombo;
    GtkWindow*  win;
    bool        confirmed;
} NewGameData;

static void ng_confirm(GtkButton* b, gpointer ud) {
    (void)b;
    NewGameData* ng = (NewGameData*)ud;
    ng->confirmed = true;
    gtk_window_close(ng->win);
}

static void start_new_game(enum gameMode mode,
                            enum pieceColor humanColor,
                            int difficulty)
{
    app.mode        = mode;
    app.humanColor  = humanColor;
    app.aiDifficulty = difficulty;
    app.gameOver    = false;
    app.undoTop     = 0;
    app.fullMove    = 1;
    app.hasSel      = false;
    app.selRow = app.selCol = -1;
    memset(app.hintMap, 0, sizeof(app.hintMap));

    initGameState(&app.gs);
    initializeBoard(app.gs.board);
    cache_legal_moves();

    /* Clear log */
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(app.logList)) != NULL)
        gtk_list_box_remove(GTK_LIST_BOX(app.logList), child);

    refresh_status();
    refresh_board();
    gtk_label_set_text(GTK_LABEL(app.statusLabel), "");

    /* If AI goes first */
    if (mode == AI_VS_AI ||
        (mode == HUMAN_VS_AI && humanColor == BLACK))
        ai_move_async();
}

static void on_new_game_clicked(GtkButton* btn, gpointer udata) {
    (void)btn;
    GtkWindow* parent = GTK_WINDOW(udata);

    GtkWidget* dlg = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dlg), "New Game");
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
    gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);

    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_top(grid, 20);
    gtk_widget_set_margin_bottom(grid, 20);
    gtk_widget_set_margin_start(grid, 24);
    gtk_widget_set_margin_end(grid, 24);
    gtk_window_set_child(GTK_WINDOW(dlg), grid);

    /* Mode */
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Mode:"), 0, 0, 1, 1);
    GtkWidget* modeCombo = gtk_drop_down_new_from_strings(
        (const char*[]){"Human vs Human", "Human vs AI", "AI vs AI", NULL});
    gtk_drop_down_set_selected(GTK_DROP_DOWN(modeCombo), 1);
    gtk_grid_attach(GTK_GRID(grid), modeCombo, 1, 0, 1, 1);

    /* Color */
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Play as:"), 0, 1, 1, 1);
    GtkWidget* colorCombo = gtk_drop_down_new_from_strings(
        (const char*[]){"White", "Black", NULL});
    gtk_grid_attach(GTK_GRID(grid), colorCombo, 1, 1, 1, 1);

    /* Difficulty */
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Difficulty:"), 0, 2, 1, 1);
    GtkWidget* diffCombo = gtk_drop_down_new_from_strings(
        (const char*[]){"Easy", "Medium", "Hard", NULL});
    gtk_drop_down_set_selected(GTK_DROP_DOWN(diffCombo), 1);
    gtk_grid_attach(GTK_GRID(grid), diffCombo, 1, 2, 1, 1);

    /* Start button */
    GtkWidget* startBtn = gtk_button_new_with_label("Start Game");
    gtk_grid_attach(GTK_GRID(grid), startBtn, 0, 3, 2, 1);

    NewGameData ng = {
        .modeCombo  = modeCombo,
        .colorCombo = colorCombo,
        .diffCombo  = diffCombo,
        .win        = GTK_WINDOW(dlg),
        .confirmed  = false
    };
    g_signal_connect(startBtn, "clicked", G_CALLBACK(ng_confirm), &ng);

    gtk_window_present(GTK_WINDOW(dlg));
    while (gtk_widget_get_visible(dlg))
        g_main_context_iteration(NULL, TRUE);

    if (!ng.confirmed) return;

    guint modeIdx  = gtk_drop_down_get_selected(GTK_DROP_DOWN(modeCombo));
    guint colorIdx = gtk_drop_down_get_selected(GTK_DROP_DOWN(colorCombo));
    guint diffIdx  = gtk_drop_down_get_selected(GTK_DROP_DOWN(diffCombo));

    enum gameMode  gmode  = (enum gameMode)modeIdx;
    enum pieceColor gcol  = (colorIdx == 0) ? WHITE : BLACK;
    int             gdiff = (int)diffIdx;

    start_new_game(gmode, gcol, gdiff);
}

/* ── CSS ───────────────────────────────────────────────────────── */

static void load_css(void) {
    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css,
        "window { background-color: #211e1c; }"
        ".sidebar { background-color: #2b2724; border-radius: 8px; }"
        ".log-entry { font-family: monospace; font-size: 12px; color: #e8e0d0; padding: 1px 4px; }"
        "button { border-radius: 6px; }"
        ".title-bar { font-size: 18px; font-weight: bold; color: #d4a84b; }"
        "label.status { font-size: 13px; color: #e05050; font-weight: bold; }"
        "label.turn   { font-size: 14px; color: #90c878; font-weight: bold; }"
    );
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
}

/* ── Main window ───────────────────────────────────────────────── */

static void activate(GtkApplication* gapp, gpointer udata) {
    (void)udata;
    load_css();

    GtkWidget* win = gtk_application_window_new(gapp);
    gtk_window_set_title(GTK_WINDOW(win), "Anteater Chess");
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);

    /* Outer horizontal box: board | sidebar */
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(win), hbox);

    /* ── Board drawing area ── */
    app.boardArea = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(app.boardArea),
                                        BOARD_COLS * SQUARE_SIZE);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(app.boardArea),
                                         BOARD_ROWS * SQUARE_SIZE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(app.boardArea),
                                   draw_board, NULL, NULL);

    GtkGesture* click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed", G_CALLBACK(on_square_clicked), NULL);
    gtk_widget_add_controller(app.boardArea, GTK_EVENT_CONTROLLER(click));
    gtk_box_append(GTK_BOX(hbox), app.boardArea);

    /* ── Sidebar ── */
    GtkWidget* sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(sidebar, "sidebar");
    gtk_widget_set_size_request(sidebar, SIDEBAR_W, -1);
    gtk_widget_set_margin_top   (sidebar, 12);
    gtk_widget_set_margin_bottom(sidebar, 12);
    gtk_widget_set_margin_start (sidebar, 10);
    gtk_widget_set_margin_end   (sidebar, 12);
    gtk_box_append(GTK_BOX(hbox), sidebar);

    /* Title */
    GtkWidget* title = gtk_label_new("🐜 Anteater Chess");
    gtk_widget_add_css_class(title, "title-bar");
    gtk_box_append(GTK_BOX(sidebar), title);

    gtk_box_append(GTK_BOX(sidebar), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Turn label */
    app.turnLabel = gtk_label_new("White's turn");
    gtk_widget_add_css_class(app.turnLabel, "turn");
    gtk_box_append(GTK_BOX(sidebar), app.turnLabel);

    /* Status (check / game-over) */
    app.statusLabel = gtk_label_new("");
    gtk_widget_add_css_class(app.statusLabel, "status");
    gtk_box_append(GTK_BOX(sidebar), app.statusLabel);

    gtk_box_append(GTK_BOX(sidebar), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Move log */
    GtkWidget* logTitle = gtk_label_new("Move Log");
    gtk_box_append(GTK_BOX(sidebar), logTitle);

    app.logList = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app.logList),
                                     GTK_SELECTION_NONE);

    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), app.logList);
    gtk_box_append(GTK_BOX(sidebar), scroll);

    gtk_box_append(GTK_BOX(sidebar), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Buttons */
    GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(btnBox, 8);
    gtk_widget_set_margin_end  (btnBox, 8);
    gtk_box_append(GTK_BOX(sidebar), btnBox);

    GtkWidget* newBtn = gtk_button_new_with_label("New Game");
    g_signal_connect(newBtn, "clicked", G_CALLBACK(on_new_game_clicked), win);
    gtk_box_append(GTK_BOX(btnBox), newBtn);

    app.undoButton = gtk_button_new_with_label("Undo");
    g_signal_connect(app.undoButton, "clicked", G_CALLBACK(on_undo_clicked), NULL);
    gtk_box_append(GTK_BOX(btnBox), app.undoButton);

    /* Piece legend */
    gtk_box_append(GTK_BOX(sidebar), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    GtkWidget* legendLbl = gtk_label_new("🐜 = Anteater (unique piece)");
    gtk_label_set_wrap(GTK_LABEL(legendLbl), TRUE);
    gtk_widget_set_margin_start(legendLbl, 8);
    gtk_widget_set_margin_end  (legendLbl, 8);
    gtk_box_append(GTK_BOX(sidebar), legendLbl);

    /* Init game */
    start_new_game(HUMAN_VS_AI, WHITE, 1);

    gtk_window_present(GTK_WINDOW(win));
}

/* ── Entry point ───────────────────────────────────────────────── */

int main(int argc, char** argv) {
    /* We need promptPromotion to exist since Chess.c references it,
       but in GUI mode the function is never called from the engine
       (humans use the dialog; AI only reaches non-pawn promos via
       findBestMove → applyMove with a pre-chosen flag).  We define a
       stub here so the linker is happy. */

    GtkApplication* app_obj = gtk_application_new(
        "edu.uci.blunderboys.chess", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app_obj, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app_obj), argc, argv);
    g_object_unref(app_obj);
    return status;
}

/* Stub — required by linker (Chess.c defines a terminal version) */
enum pieceType promptPromotion(void) { return QUEEN; }