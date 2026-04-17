#include "Engine.h"

int getScore(const struct gameState* gs)
{
    int score = 0;
    //int checkmate = 1e8;

    static const int weight[7]=
    {
        [King] = 0,
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

    // return relative to side to move
    return (gs->currentPlayer == White) ? score : -score;
}

int negaMax(struct gameState* gs, int depth, int alpha, int beta)
{

    //find all legal moves
    int moveCount = 0;
    uint16_t moves[MAX_MOVES];
    getMoves(gs, moves, &moveCount);

    //if no legal moves
    if(moveCount == 0)
    {
        //and in check
        if(inCheck(gs)) return -INF+depth; //then checkmate
        //and not in check
        else return 0;  //stalemate
    }

    //only use getScore after ensuring no checkmate condition
    //base condition
    if (depth == 0) return getScore(gs);

    int bestScore = -INF;
    for(int i = 0; i < moveCount; i++)
    {
        //makeMove(gs, moves[i]);

        int score = -negaMax(gs, depth - 1, -beta, -alpha);

        //undoMove(gs, moves[i]);

        if(score > bestScore)
            bestScore = score;

        //recompute alpha and prune
        alpha = MAX(alpha, bestScore);
        if (beta <= alpha) break;
    }

    return bestScore;
}

uint16_t findBestMove(struct gameState* gs, int depth)
{
    int maxScore = -INF;

    //get legal moves
    int moveCount = 0;
    uint16_t moves[MAX_MOVES];
    getMoves(gs, moves, &moveCount);

    //pick an initial move, maybe add randomness to this
    if(moveCount == 0) return 0; //<---if no legal moves, don't access moves[0]
    uint16_t bestMove = moves[0];
    
    int alpha = -INF;
    int beta = INF;
    for(int i = 0 ; i < moveCount; i++)
    {
        //makeMove(gs, moves[i]);

        // !!increase depth once more stable, 3 is testing depth, can be higher for smarter play!!
        int score = -negaMax(gs, depth - 1, -beta, -alpha);

        //undoMove(gs, moves[i]);

        if(score > maxScore)
        {
            maxScore = score;
            bestMove = moves[i];
        }
        alpha = MAX(alpha, maxScore);
    }

    return bestMove;
}

void movePiece_Computer(struct gameState* gs, int difficulty)
{
    srand(time(NULL));
    int depth = 0;

    //example of dificulty
    enum level
    {
        easy, medium, hard
    };

    switch(difficulty)
    {
        // ** SHOULD CHANGE DEPTHS BASED ON TIME TESTING
        case easy:
            //pick random depth 1-4
            depth = (rand() % 4 ) + 1;
            break;
        case medium:
            //pick random depth 5-7
            depth = (rand() % 3 ) + 5;
            break;
        case hard:
            depth = 8;
            break;

    }

    uint16_t bestMove = findBestMove(gs, depth);
    
    //makeMove(gs, bestMove)
}
