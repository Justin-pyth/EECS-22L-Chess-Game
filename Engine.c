#include "Engine.h"

int getScore(const struct gameState* gs)
{
    int score = 0;
    //int checkmate = 1e8;

    //ranks = rows (row)
    //file  = columns (col)

    for(int row = 0; row < 8; row++)
    {   
        //find each piece on board
        for(int col = 0 ; col < 10; col++)
        {
            struct piece* p = gs->board[row][col];
            if (!p) continue;

            //get value of piece
            int value = weight[p->piece];
            
            //due to the engine picking the same pawn to move in a straight line
            //positional multipliers were added

            //give up to +35 bonus to increase pawn advancement
            if (p->piece == ANT)
                value += ((p->color == WHITE) ? row : (7-row)) * 5;
            
            //discourage these pieces from staying near their home row
            if (p->piece == KNIGHT || p->piece == BISHOP || p->piece == ANTEATER)
                if (!((p->color == WHITE) ? (row == 0) : (row == 7))) 
                    value += 20;

            //for all pieces, increase frequency that center tiles are occupied
            if (col >= 3 && col <= 6 && row >= 2 && row <= 5)
            //inner
                value += 10;
            else if (col >= 2 && col <= 7 && row >= 1 && row <= 6)
            //outer
                value += 5;

            //if White, add the score, else if Black, subtract the score
            score += (p->color == WHITE) ? value : -value;
        }
    }

    // return relative to side to move
    return (gs->currentPlayer == WHITE) ? score : -score;
}

int negaMax(struct gameState* gs, int depth, int alpha, int beta)
{

    //find all legal moves
    int moveCount = 0;
    uint32_t moves[MAX_MOVES];
    getMoves(gs, moves, &moveCount);
    //sort according to MVV-LVA
    preSort(gs, moves, moveCount);


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
    if (depth <= 0) return getScore(gs);

    int bestScore = -INF;
    for(int i = 0; i < moveCount; i++)
    {
        struct MoveUndo u;
        applyMove(gs, moves[i], &u);

        int score = -negaMax(gs, depth - 1, -beta, -alpha);

        undoMove(gs, &u);

        if(score > bestScore)
            bestScore = score;

        //recompute alpha and prune
        alpha = MAX(alpha, bestScore);
        if (beta <= alpha) break;
    }

    return bestScore;
}

uint32_t findBestMove(struct gameState* gs, int depth)
{
    int maxScore = -INF;

    //get legal moves
    int moveCount = 0;
    uint32_t moves[MAX_MOVES];
    getMoves(gs, moves, &moveCount);
    //sort according to MVV-LVA
    preSort(gs, moves, moveCount);

    //pick an initial move, maybe add randomness to this
    if(moveCount == 0) return 0; //<---if no legal moves, don't access moves[0]
    uint32_t bestMove = moves[0];
    
    int alpha = -INF;
    int beta = INF;
    for(int i = 0 ; i < moveCount; i++)
    {
        struct MoveUndo u;
        applyMove(gs, moves[i], &u);

        int score = -negaMax(gs, depth - 1, -beta, -alpha);

        undoMove(gs, &u);

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
    
    int depth = 0;


    switch(difficulty)
    {
        case 0:
            depth = 1;
            break;
        case 1:
            depth = 2;
            break;
        case 2:
            depth = 3;              
            break;
    }


    uint32_t bestMove = findBestMove(gs, depth);
    applyMove(gs, bestMove, NULL);
}

int MVV_LVA(const struct gameState* gs, uint32_t move)
{
    //get [from] and [to] tiles
    int from = getFrom(move);
    int to = getTo(move);

    //access the specific piece from board
    struct piece* attacker = gs->board[getRow(from)][getCol(from)];
    struct piece* victim = gs->board[getRow(to)][getCol(to)];


    //if not a capture, then return 0
    if(!victim) return 0;
    //if capture, subtract the victim's value by attackers value
    //goal is to get the highest victim value, lowest attacker value
    //ex: pawn->queen
    return weight[victim->piece] - weight[attacker->piece];

}

void preSort(const struct gameState* gs, uint32_t* moves, int moveCount)
{
    //compute weights
    int scores[MAX_MOVES];
    for(int i = 0 ; i<moveCount; i++)
        scores[i] = MVV_LVA(gs, moves[i]);

    //a modified version of insertion sort w/ shifting instead of swaps
    //note : if i = 0, it would already be sorted, so just skip to next index 1
    for(int i = 1; i<moveCount; i++)
    {
        uint32_t move =  moves[i];
        int score = scores[i];

        int j = i-1;
        //while the current score is lower
        while((j>=0) && (scores[j] < score))
        {
            //shift elements forward (move worse element 1 unit right)
            moves[j+1]=moves[j];
            scores[j+1]=scores[j];
            j--;
        }
        //place in correct position after shifts
        moves[j+1] = move;
        scores[j+1] = score;
    }
}