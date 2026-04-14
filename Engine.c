#include "Engine.h"

int getScore(const struct gameState* gs)
{
    int score = 0;
    //int checkmate = 1e8;

    static const int weight[7]=
    {
        //king weight might not be needed
        [King] = 10*INF,
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
    //find all legal moves
    int moveCount = 0;
    struct move* moves = getMoves(&moveCount);

    //check if no legal moves can be made
    int checkState = checkmate(gs);
    //can replace 1 with integer representing CHECKMATE, when checkmate function is finished
    if(checkState == 1) 
        return (playerColor) ? -INF : INF;
    //can replace 2 with integer representing STALEMATE, when checmate function is finished
    else if (checkState == 2)
        return 0;


    //only use getScore after ensuring no checkmate condition
    //base condition
    if (depth == 0) return getScore(gs);

    //playerColor = true (white), false (black)
    if(playerColor)
    {
        int maxScore = -INF;
        //for every move
        for(int i = 0 ; i < moveCount; i++)
        {
            //makeMove(gs, moves[i]);

            int score = minimax(gs, depth - 1, alpha, beta, !playerColor);

            //undoMove(gs, moves[i]);

            if(score > maxScore)
                maxScore = score;

            //recompute alpha and prune
            alpha = MAX(alpha, maxScore);
            if (beta <= alpha) break;
        }

        return maxScore;
    }
    else
    {
        int minScore = INF;
        //for every move
        for(int i = 0 ; i < moveCount; i++)
        {
            //makeMove(gs, moves[i]);

            int score = minimax(gs, depth - 1, alpha, beta, playerColor);

            //undoMove(gs, moves[i]);

            if(score < minScore)
                minScore = score;

            //recompute beta and prune
            beta = MIN(beta, minScore);
            if (beta <= alpha) break;
        }

        return minScore;
    }

}

struct move findBestMove(struct gameState* gs, int depth)
{
    int maxScore = -INF;

    //find all legal moves
    int moveCount = 0;
    struct move* moves = getMoves(&moveCount);

    //pick an initial move, maybe add randomness to this
    struct move bestMove = moves[0];

    for(int i = 0 ; i < moveCount; i++)
    {
        //makeMove(gs, moves[i]);

        // !!increase depth once more stable, 3 is testing depth, can be higher for smarter play!!
        int score = miniMax(gs, depth - 1, -INF, INF, false);

        //undoMove(gs, moves[i]);

        if(score > maxScore)
        {
            maxScore = score;
            bestMove = moves[i];
        }
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

    struct move bestMove = findBestMove(gs, depth);
    
    //makeMove(gs, bestMove)
}