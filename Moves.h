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

struct move* getMoves(int* moveCount);

struct move* getLegalMoves(struct piece p, int* moveCount);

bool isLegalMove(struct move, const struct gameState* gs);

#endif