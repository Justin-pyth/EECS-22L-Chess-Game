#include "Engine.h"

int nodeCount = 0;
const int weight[7] =
{
    [KING]     = 100000,
    [QUEEN]    = 900,
    [KNIGHT]   = 300,
    [BISHOP]   = 350,
    [ROOK]     = 500,
    [ANT]      = 100,
    [ANTEATER] = 330
};

bool stop_search = false;
double time_start = 0;
double time_allot = 1000; //10 s

//start of killer move implementation
uint32_t K_MOVES[64][2];    //max depth, 2 killer moves stored per depth
//history implementation
int HISTORY[8][10][8][10];  //row col row col

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
            {
                //distance traveled by pawn from starting pos
                int tilPromo = (p->color == WHITE) ? row : (7 - row);
                int row_bonus[] = {0, 10, 30, 80, 200, 400, 600, 750};

                if (tilPromo < 0) tilPromo = 0;
                if (tilPromo > 7) tilPromo = 7;
                value += row_bonus[tilPromo];

            }

            if (p->piece == QUEEN) value += 200;
            
            //discourage these pieces from staying near their home row
            if (p->piece == KNIGHT || p->piece == BISHOP || p->piece == ANTEATER)
                if (!((p->color == WHITE) ? (row == 0) : (row == 7))) 
                    value += 20;

            if (p->piece == ANTEATER) 
            {
                int near_Ant = 0;
                int chainAnt = 0;
                enum pieceColor opponent_color = (p->color == WHITE) ? BLACK : WHITE;

                //(deltaR, deltaC) <--- (change in row, change in col)
                //iterate through all directions aka 1 move in orthogonal or diagonal 
                for(int deltaRow = -1 ; deltaRow <= 1 ; deltaRow++)
                {
                    for(int deltaCol = -1; deltaCol <= 1 ; deltaCol++)
                    {
                        //ignore self
                        if(deltaRow == 0 && deltaCol == 0) continue;

                        //find new row and col
                        int newRow = row + deltaRow;
                        int newCol = col + deltaCol;
                        //make sure its not out of bounds ... segfault
                        if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 10) continue;

                        //identify the victim
                        struct piece* victim = gs->board[newRow][newCol];

                        //check if theres a piece there, check if its an ant,  check if its the color of opponent
                        if(victim && victim->piece == ANT && victim->color == opponent_color)
                        {
                            near_Ant++; //theres an ant nearby

                            //scan through chaining
                            int chainRow = newRow + deltaRow;
                            int chainCol = newCol + deltaCol;

                            //while in bounds
                            while(chainRow >= 0 && chainRow < 8 && chainCol >= 0 && chainCol < 10)
                            {
                                struct piece* chainVictim = gs->board[chainRow][chainCol];;

                                if(!chainVictim || chainVictim->piece != ANT || chainVictim->color != opponent_color) break;

                                //new ant to chain
                                chainAnt++;
                                //check next
                                chainRow += deltaRow;
                                chainCol += deltaCol;
                            }
                        }
                    }
                }

                //bonuses (arbitrary TO ADJUST LATER)
                value += near_Ant * (weight[ANT] / 20);
                value += MIN(chainAnt, 4) * (weight[ANT] / 30);


                //increase value for ability to capture on current turn
                if(near_Ant > 0)
                    value += 10;
            }

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
    nodeCount++;
    //find all legal moves

    if (nodeCount % 2048 == 0) 
    { 
        if (get_elapsed_time(time_start) >= time_allot) 
            stop_search = true;
    }
    if (stop_search) return 0;

    int moveCount = 0;
    uint32_t moves[MAX_MOVES];
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
    if (depth <= 0) return Quiesce(gs, alpha, beta);

    //sort according to MVV-LVA (after base conditions)
    preSort(gs, moves, moveCount);

    //place the killer moves in front, note that moves[0] will contain the best move from preSort
    //which could possibly be better than killer moves so start at 1 to avoid that
    for (int i = 1; i < moveCount; i++) 
    {
        if (moves[i] == K_MOVES[depth][0] || moves[i] == K_MOVES[depth][1]) 
        {
            uint32_t tempMove = moves[i];
            moves[i] = moves[0];
            moves[0] = tempMove;
            break;
        }
    }

    int bestScore = -INF;
    for(int i = 0; i < moveCount; i++)
    {
        struct MoveUndo u;
        applyMove(gs, moves[i], &u);

        int score = -negaMax(gs, depth - 1, -beta, -alpha);

        undoMove(gs, &u);

        //premature exit
        if (stop_search) return 0;

        if(score > bestScore)
            bestScore = score;

        //recompute alpha and prune
        alpha = MAX(alpha, bestScore);
        if (beta <= alpha)
        {
            //store moves that cause beta-cut off AKA very one-sided move
            //so it stores the worst move trees for the previous player, which are the best moves for the current
            //since sides switch
            if (!isCapture(gs, moves[i])) 
            {
                K_MOVES[depth][1] = K_MOVES[depth][0];
                K_MOVES[depth][0] = moves[i];
                //increment history since a move caused beta-cutoff
                HISTORY[getFromRow(moves[i])][getFromCol(moves[i])][getToRow(moves[i])][getToCol(moves[i])] += depth * depth; //give higher scores at higher depths
                //^ a high depth means that it is early on into the tree, a move that is causing beta-cutoff early into the tree is seen as the best move for the next player
            }
            break;
        }
    }

    return bestScore;
}

uint32_t depthSearch(struct gameState* gs, int depth, uint32_t pvMove)
{
    int maxScore = -INF;
    //get legal moves
    int moveCount = 0;
    uint32_t moves[MAX_MOVES];
    getMoves(gs, moves, &moveCount);

    //pick an initial move, maybe add randomness to this
    if(moveCount == 0) return 0; //<---if no legal moves, don't access moves[0]

    //if pvMove exists, aka not first depth to be searched
    if (pvMove != 0) 
    {   
        //iterate through each move
        for (int i = 0; i < moveCount; i++) 
        {
            //find the previous best move
            if (moves[i] == pvMove) 
            {
                //put the previous best move in front of the movesList
                uint32_t temp = moves[0];
                moves[0] = moves[i];
                moves[i] = temp;
                break;
            }
        }
    }

    //index offset so the previous best move remains in front
    if (moveCount > 1) {
        //sort according to MVV-LVA + History
        preSort(gs, &moves[1], moveCount - 1);
    }


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
        if(beta <= alpha) break;
    }

    return bestMove;
}


uint32_t findBestMove(struct gameState* gs, int depth)
{
    //reset killer moves and history
    memset(K_MOVES, 0, sizeof(K_MOVES));
    memset(HISTORY, 0, sizeof(HISTORY));
    stop_search = false;
    time_start = get_current_time();

    uint32_t bestMove = 0;
    uint32_t previousBestMove = 0;

    //iterate at each depth
    for(int i = 1 ; i <= depth; i++)
    {
        //return the best move at the depth
        uint32_t move = depthSearch(gs, i, previousBestMove);
        //if move is valid, then set it as best
        if(!stop_search)
        {
            previousBestMove = move;
            bestMove = move;
        }
        else break;

    }
    return bestMove;
}

void movePiece_Computer(struct gameState* gs, int difficulty)
{
    
    int depth = 0;


    switch(difficulty)
    {
        case 0:
            depth = (rand() % 2) + 1; //1 or 2
            break;
        case 1:
            depth = (rand() % 2) + 3; //3 or 4
            break;
        case 2:
            depth = 50; //4 set to 5 or more later            
            break;
    }


    uint32_t bestMove = findBestMove(gs, depth);
    applyMove(gs, bestMove, NULL);
}

int MVV_LVA(const struct gameState* gs, uint32_t move)
{

    //access the specific piece from board
    struct piece* attacker = gs->board[getFromRow(move)][getFromCol(move)];
    struct piece* victim = gs->board[getToRow(move)][getToCol(move)];


    //if not a capture, then 
    //if its a non capture move, return the score as the history to preSort
    if(!victim)
        return HISTORY[getFromRow(move)][getFromCol(move)][getToRow(move)][getToCol(move)];


    if (isPromotion(move)) 
    {
        int promoValue;
        int flags = getFlags(move);
        switch (flags)
        {
            case MOVE_PROMO_QUEEN:    promoValue = weight[QUEEN]; break;
            case MOVE_PROMO_ROOK:     promoValue = weight[ROOK]; break;
            case MOVE_PROMO_BISHOP:   promoValue = weight[BISHOP]; break;
            case MOVE_PROMO_KNIGHT:   promoValue = weight[KNIGHT]; break;
            case MOVE_PROMO_ANTEATER: promoValue = weight[ANTEATER]; break;
        }

    return 10000 + promoValue;
}
    int flags = getFlags(move);
    if (flags == MOVE_EN_PASSANT || flags == MOVE_ANTEATING) {
        return 10100; //tune this, just make it above 10k so q-funct doesnt skip it
    }

    //if capture, subtract the victim's value by attackers value
    //goal is to get the highest victim value, lowest attacker value
    //ex: pawn->queen
    return 10000 + weight[victim->piece] - weight[attacker->piece];

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

int Quiesce(struct gameState* gs, int alpha, int beta)
{
    nodeCount++;
    if (nodeCount % 2048 == 0) 
    { 
        if (get_elapsed_time(time_start) >= time_allot) 
            stop_search = true;
    }
    if (stop_search) return alpha;

    int static_score = getScore(gs);

    int best_score = static_score;
    if(best_score >= beta)
        return best_score;
    if(best_score > alpha)
        alpha = best_score;
    
    uint32_t moves[MAX_MOVES];
    int moveCount = 0;
    getMoves(gs, moves, &moveCount);
    preSort(gs, moves, moveCount); //give better moves first for earlier beta cutoffs

    for(int i = 0; i< moveCount ; i++)
    {
        uint32_t move = moves[i];

        //filter out non-captures (quiet moves)
        if (!isCapture(gs, move) && !isPromotion(move)) continue;


        struct MoveUndo u;
        applyMove(gs, move, &u);

        int score = -Quiesce(gs, -beta, -alpha);

        undoMove(gs, &u);

        if(score >= beta)
            return score;
        if(score > best_score)
            best_score = score;
        if(score > alpha)
            alpha = score;
    }

    return best_score;
}

bool isCapture(struct gameState* gs, uint32_t move)
{
    int toRow = getToRow(move); int toCol = getToCol(move); int flags = getFlags(move);

    if (toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 10) return false;

    if (flags == MOVE_ANTEATING || flags == MOVE_EN_PASSANT) return true;

    //check if theres a piece and if that piece is an enemy piece (return it)
    struct piece* victim = gs->board[toRow][toCol];

    //return true if there is a opponent piece that can be captured
    return (victim && victim->color != gs->currentPlayer);
}

double get_current_time() 
{
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}

double get_elapsed_time(double start) 
{
    return get_current_time() - start;
}

bool isPromotion(uint32_t move)
{
    int flags = getFlags(move);

    return (
        flags == MOVE_PROMO_QUEEN ||
        flags == MOVE_PROMO_ROOK ||
        flags == MOVE_PROMO_BISHOP ||
        flags == MOVE_PROMO_KNIGHT ||
        flags == MOVE_PROMO_ANTEATER
    );
}
