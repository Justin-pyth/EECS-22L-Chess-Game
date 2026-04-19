#ifndef MOVES_H
#define MOVES_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"

#define MAX_MOVES 1024

/* ── Move flags ─────────────────────────────────────────────────────── */
#define MOVE_NORMAL         0
#define MOVE_EN_PASSANT     1
#define MOVE_CASTLE_KS      2
#define MOVE_CASTLE_QS      3
#define MOVE_PROMO_QUEEN    4
#define MOVE_PROMO_ROOK     5
#define MOVE_PROMO_BISHOP   6
#define MOVE_PROMO_KNIGHT   7
#define MOVE_PROMO_ANTEATER 8
#define MOVE_ANTEATING      9

typedef char* string;

struct pos {
    char rank;
    char file;
};

struct move {
    struct pos pos1;
    struct pos pos2;
};

struct location {
    int row;
    int col;
};

/* Saved state needed to undo a Move */
struct MoveUndo {
    struct piece*   board[8][10];
    enum pieceColor currentPlayer;
    bool whiteKingMoved, blackKingMoved;
    bool whiteRookMovedQS, whiteRookMovedKS;
    bool blackRookMovedQS, blackRookMovedKS;
    int  enPassantCol, enPassantRow;
    int  halfMove_count;
};

/* ── Bit-packed Move encode / decode ────────────────────────────────── */
static inline uint32_t createMove(int fromRow, int fromCol,
                                   int toRow,   int toCol, int flags) {
    return (uint32_t)(fromRow)        |
           (uint32_t)(fromCol) << 4   |
           (uint32_t)(toRow)   << 8   |
           (uint32_t)(toCol)   << 12  |
           (uint32_t)(flags)   << 16;
}
static inline int getFromRow(uint32_t move) { return  move        & 0xF; }
static inline int getFromCol(uint32_t move) { return (move >>  4) & 0xF; }
static inline int getToRow  (uint32_t move) { return (move >>  8) & 0xF; }
static inline int getToCol  (uint32_t move) { return (move >> 12) & 0xF; }
static inline int getFlags  (uint32_t move) { return (move >> 16);       }

/* combined from/to accessors used by Engine.c */
static inline int getFrom(uint32_t move) { return  move        & 0xFF; }
static inline int getTo  (uint32_t move) { return (move >>  8) & 0xFF; }
static inline int getRow (int pos)       { return  pos         & 0xF;  }
static inline int getCol (int pos)       { return (pos >>  4)  & 0xF;  }

/* ── Function declarations ──────────────────────────────────────────── */

/* Primary move generator — fills moves[] with all legal moves for the
   current player and sets *moveCount.                                  */
void getMoves(struct gameState* gs, Move* moves, int* moveCount);

/* Returns true if the current player's king is in check.              */
bool inCheck(const struct gameState* gs);

/* Apply m to gs, optionally saving undo state into *u (may be NULL).  */
void applyMove(struct gameState* gs, Move m, struct MoveUndo* u);

/* Restore gs to the state saved by the matching applyMove call.       */
void undoMove(struct gameState* gs, const struct MoveUndo* u);

/* Returns true if moveMade is among the legal moves in gs.            */
bool isLegalMove(struct move moveMade, const struct gameState* gs);

/* Per-piece struct-move generators (used by UI / display code).
   Caller must free() the returned array.                               */
struct move* getAntMoves     (struct piece* board[8][10], int row, int col, int* moveCount);
struct move* getBishopMoves  (struct piece* board[8][10], int row, int col, int* moveCount);
struct move* getKnightMoves  (struct piece* board[8][10], int row, int col, int* moveCount);
struct move* getRookMoves    (struct piece* board[8][10], int row, int col, int* moveCount);
struct move* getQueenMoves   (struct piece* board[8][10], int row, int col, int* moveCount);
struct move* getKingMoves    (struct piece* board[8][10], int row, int col, int* moveCount);
struct move* getAnteaterMoves(struct piece* board[8][10], int row, int col, int* moveCount);

#endif /* MOVES_H */
