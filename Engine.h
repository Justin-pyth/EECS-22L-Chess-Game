#ifndef ENGINE_H
#define ENGINE_H

//useful utility functions for alpha-beta minimax
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define INF 1000000
#define MAX_MOVES 1024
#define MAX_DEPTH 64


#include "Moves.h"
#include "Eval.h"
#include "Hash.h"
#include "TT.h"
extern int nodeCount;
extern int HISTORY[8][10][8][10];

// Forward declaration for attack checking function
bool isSquareAttackedBy(struct piece* board[8][10], int row, int col, enum pieceColor attackerColor);

//recursive function that returns a score
int negaMax(struct gameState* gs, int depth, int alpha, int beta);


uint32_t depthSearch(struct gameState* gs, int depth, uint32_t pvMove);
//acts as a wrapper for negaMax, returning bestMove, based off depth <--- determines difficulty
uint32_t findBestMove(struct gameState* gs, int depth);

//calls findBestMove (based off difficulty level), and makes the move
void movePiece_Computer(struct gameState* gs, int difficulty);

//most-valuable-victim, least-valuable-attacker:
//returns a weight, weights are higher when attacker is low value
//and captured piece is high
//think: pawn captures queen
int MVV_LVA(const struct gameState* gs, uint32_t move, int depth);
//insertion sorts by weight (from MVV_LVA)
void preSort(const struct gameState* gs, uint32_t* moves, int moveCount, int depth);

int Quiesce(struct gameState* gs, int alpha, int beta);
bool isCapture(struct gameState* gs, uint32_t move);
double get_current_time();
double get_elapsed_time(double start);
bool isPromotion(uint32_t move);

#endif
