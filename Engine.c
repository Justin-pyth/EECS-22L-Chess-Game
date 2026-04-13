#include "Engine.h"

struct Move* getMoves(int* moveCount);

struct Move* getLegalMoves(struct piece p, int* moveCount);

//probably belongs in another file
bool isLegalMove(struct Move, const struct gameState* gs);

int getScore(const struct gameState* gs)
{
    int score = 0;
    //int checkmate = 1e8;

    static const int weight[7]=
    {
        [King] = 100000000,
        [Queen] = 900,
        [Knight] = 300,
        [Bishop] = 350,
        [Rook] = 500,
        [Ant] = 100,
        [Anteater] = 330
    };

    //ranks = rows (row)
    //file  = columns (col)

    for(int row = 0; row < 8; row++)
    {   
        //find each piece on board
        for(int col = 0 ; col < 10; col++)
        {
            struct piece* p = gs->board[row][col];
            if (p) {
                //get value of piece
                int value = weight[p->piece_];
                //if White, add the score, else if Black, subtract the score
                score += (p->color == White) ? value : -value;
            }
        }
    }

    return score;
}

int miniMax(const struct gameState* gs, int depth, int alpha, int beta, bool playerColor)
{
    if (depth == 0) return getScore(gs);

    int moveCount = 0;
    //struct Move* moves = getMoves
    return 0;//placeholder
}

int movePiece_Computer(struct gameState* gs);