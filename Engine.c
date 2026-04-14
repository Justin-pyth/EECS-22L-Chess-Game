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
        //king weight might not be needed
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
    //ADD THIS TO MAIN
    srand(time(NULL));

    //find all legal moves
    int moveCount = 0;
    struct move* moves = getMoves(&moveCount);

    //check if no legal moves can be made
    int checkState = checkmate(gs);
    //can replace 1 with integer representing CHECKMATE, when checkmate function is finished
    if(checkState = 1) 
        return (playerColor) ? -1000000 : 1000000;
    //can replace 2 with integer representing STALEMATE, when checmate function is finished
    else if (checkState = 2)
        return 0;


    //only use getScore after ensuring no checkmate condition
    //base condition
    if (depth == 0) return getScore(gs);

    //pick an initial move randomly, so its less predictable (aka not moves[0])
    struct move bestMove = moves[rand() % moveCount];

    //playerColor = true (white), false (black)
    if(playerColor)
    {
        int maxScore = -1000000;
        //for every move
        for(int i = 0 ; i < moveCount; i++)
        {
            //makeMove(gs, moves[i]);

            int score = minimax(gs, depth - 1, alpha, beta, !playerColor);

            //undoMove(gs, moves[i]);

            if(score > maxScore)
                maxScore = score;

            //recompute alpha and prune
            alpha = MAX(alpha, score);
            if (beta <= alpha) break;
        }

        return maxScore;
    }
    else
    {
        int minScore = 1000000;
        //for every move
        for(int i = 0 ; i < moveCount; i++)
        {
            //makeMove(gs, moves[i]);

            int score = minimax(gs, depth - 1, alpha, beta, playerColor);

            //undoMove(gs, moves[i]);

            if(score < minScore)
                minScore = score;

            //recompute beta and prune
            beta = MIN(beta, score);
            if (beta <= alpha) break;
        }

        return minScore;
    }

}

int movePiece_Computer(struct gameState* gs);