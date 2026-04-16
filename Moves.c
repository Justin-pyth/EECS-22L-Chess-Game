#include "Moves.h"
#include "Chess.c" // TODO: is this allowed? to get the enums and structs from Chess.c

void getMoves(struct gameState* gs, struct move* moves, int* moveCount)
{
    ;
}


/*
   Returns a list of legal moves that the specified piece can perform
*/
struct move* getLegalMoves(struct piece p, int* moveCount)
{
   ;
}


/*
   Checks if the specified move for that piece is legal
*/
bool isLegalMove(struct move moveMade, const struct gameState* gs)
{
   ;
}


/*
    Returns the moves that a given Ant (pawn) piece can make
*/
struct move* getAntMoves(struct piece* board[8][10], int row, int col, int* moveCount)
{
    struct move* moves = malloc(4 * sizeof(struct move)); // an ant piece has at most 4 possible moves
    struct piece* p = board[row][col];
    moveCount = 0;

    int direction = 0;
    if (p->color == WHITE)
    {
        direction = 1;
    }
    else
    {
        direction = -1;
    }

    int oneStep = row + direction;

    // If ant in starting position, can move one or two squares forward
    if (p->color == WHITE && row == 1 || p->color == BLACK && row == 6)
    {
        int twoSteps = row + (2 * direction); // can only move two steps if in starting position

        if (board[row+oneStep][col] == NULL && board[row+twoSteps][col] == NULL)
        {
            // add possible move to moves
            sprintf(moves[*moveCount].pos1, "%c%d", 'a' + col, row + 1); // current position
            sprintf(moves[*moveCount].pos2, "%c%d", 'a' + col, twoSteps + 1); // destination
            (*moveCount)++;
        }
    }

    // Otherwise, ant can only move one square directly forward
}


// Bishop
struct move* getBishopMoves(struct piece p, int* moveCount)
{

}


// Knight
struct move* getKnightMoves(struct piece p, int* moveCount)
{

}


// Rook
struct move* getRookMoves(struct piece p, int* moveCount)
{

}


// Queen
struct move* getQueenMoves(struct piece p, int* moveCount)
{

}


// King
struct move* getKingMoves(struct piece p, int* moveCount)
{

}


// Anteater
struct move* getAnteaterMoves(struct piece p, int* moveCount)
{

}

