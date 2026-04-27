#ifndef ANT_H
#define ANT_H

#include "Moves.h"

void getAnteaterMoves(struct piece* board[8][10], int row, int col, uint32_t* moves, int* moveCount);

bool buildAnteaterPath(struct piece* board[8][10],
                       int fromRow, int fromCol,
                       int eatRow, int eatCol,
                       int toRow, int toCol,
                       enum pieceColor attackerColor,
                       struct location* path,
                       int* pathCount);

Move chooseBestAnteaterMove(const struct gameState* gs, Move* candidates, int count);


#endif
