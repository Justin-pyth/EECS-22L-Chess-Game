#ifndef ENGINE_H
#define ENGINE_H

//useful utility functions for alpha-beta minimax
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define INF 1000000
#define MAX_DEPTH 10
#define MAX_MOVES 256


#include <time.h>
#include "Moves.h"

//retrieve the score by computing total weights of pieces on board
int getScore(const struct gameState* gs);

//recursive function that returns a score
//the highest score is returned if white, otherwise, the lowest score
int miniMax(const struct gameState* gs, int depth, int alpha, int beta, bool playerColor);

//acts as a wrapper for miniMax, returning bestMove, based off depth <--- determines difficulty
struct move findBestMove(struct gameState* gs, int depth);

//calls findBestMove (based off difficulty level), and makes the move
void movePiece_Computer(struct gameState* gs, int difficulty);

#endif