#ifndef MOVES_H
#define MOVES_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"

//TEMPORARY DEFINITIONS
//=======================================================
typedef char* string;

struct pos {
    char rank; // "f"
    char file; // "4"
};

struct move {
    struct pos pos1; // "a4", current position (from)
    struct pos pos2; // "a5", destination (to)
};

struct location { // use to log moves
    int row; // 0-9 (ex: "f"=5)
    int col; // 0-7 (ex: 4)
};


//=======================================================

void getMoves(struct gameState* gs, struct move* moves, int* moveCount);

struct move* getLegalMoves(struct piece p, int* moveCount);

bool isLegalMove(struct move, const struct gameState* gs);

//Helper function (for move encoding/decoding)s:
static inline uint32_t createMove(int fromRow, int fromCol, int toRow, int toCol, int flags) {
    return (uint32_t)(fromRow)        |
           (uint32_t)(fromCol) << 4   |
           (uint32_t)(toRow)   << 8   |
           (uint32_t)(toCol)   << 12  |
           (uint32_t)(flags)   << 16;
}

//fetch pos & flags
static inline int getFromRow(uint32_t move) 
{ return  move & 0xF; }
static inline int getFromCol(uint32_t move)
{ return (move >> 4) & 0xF; }
static inline int getToRow  (uint32_t move)
{ return (move >> 8) & 0xF; }
static inline int getToCol  (uint32_t move)
{ return (move >> 12)& 0xF; }
static inline int getFlags  (uint32_t move)
{ return (move >> 16); }
#endif