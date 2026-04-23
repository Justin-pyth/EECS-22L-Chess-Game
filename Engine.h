#ifndef ENGINE_H
#define ENGINE_H

//useful utility functions for alpha-beta minimax
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#define INF 1000000
#define MAX_MOVES 1024


#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include "Moves.h"


//retrieve the score by computing total weights of pieces on board
int getScore(const struct gameState* gs);

//recursive function that returns a score
//the highest score is returned if white, otherwise, the lowest score
int miniMax(const struct gameState* gs, int depth, int alpha, int beta, bool playerColor);

//acts as a wrapper for miniMax, returning bestMove, based off depth <--- determines difficulty
uint32_t findBestMove(struct gameState* gs, int depth);

//calls findBestMove (based off difficulty level), and makes the move
void movePiece_Computer(struct gameState* gs, int difficulty);

//most-valuable-victim, least-valuable-attacker:
//returns a weight, weights are higher when attacker is low value
//and captured piece is high
//think: pawn captures queen
int MVV_LVA(const struct gameState* gs, uint32_t move);
//insertion sorts by weight (from MVV_LVA)
void preSort(const struct gameState* gs, uint32_t* moves, int moveCount);

#endif