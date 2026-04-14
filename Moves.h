#ifndef MOVES_H
#define MOVES_H

#include <stdbool.h>

struct Move* getMoves(int* moveCount);

struct Move* getLegalMoves(struct piece p, int* moveCount);

bool isLegalMove(struct Move, const struct gameState* gs);

#endif