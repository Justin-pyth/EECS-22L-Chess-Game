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
double time_allot = 2000; //2 seconds alloted per move (can change)

//start of killer move implementation
uint32_t K_MOVES[64][2];    //max depth, 2 killer moves stored per depth
//history implementation
int HISTORY[8][10][8][10];  //row col row col

//
static int kingSafetyScore(const struct gameState* gs, enum pieceColor color, int row, int col, bool endgame)
{
    int score = 0;
    int homeRow = (color == WHITE) ? 0 : 7;
    int homeKingCol = 5;
    bool kingMoved = (color == WHITE) ? gs->whiteKingMoved : gs->blackKingMoved;
    bool rookMovedQS = (color == WHITE) ? gs->whiteRookMovedQS : gs->blackRookMovedQS;
    bool rookMovedKS = (color == WHITE) ? gs->whiteRookMovedKS : gs->blackRookMovedKS;
    bool castled = (row == homeRow && (col == 3 || col == 7));
    bool canStillCastle = !kingMoved && (!rookMovedQS || !rookMovedKS);
    enum pieceColor opponent = (color == WHITE) ? BLACK : WHITE;
    bool opponentQueenPresent = false;

    for (int scanRow = 0; scanRow < 8 && !opponentQueenPresent; scanRow++)
    {
        for (int scanCol = 0; scanCol < 10; scanCol++)
        {
            struct piece* p = gs->board[scanRow][scanCol];
            if (p && p->color == opponent && p->piece == QUEEN)
            {
                opponentQueenPresent = true;
                break;
            }
        }
    }

    if (!endgame)
    {
        if (castled)
            score += 95;
        else if (row == homeRow && col == homeKingCol)
            score -= canStillCastle ? 20 : 75;
        else
            score -= 110;

        if (!castled)
        {
            if (kingMoved)
                score -= 25;

            if (col >= 3 && col <= 6)
                score -= 15;

            if (row != homeRow)
                score -= 15;

            if (opponentQueenPresent)
                score -= 20;
        }

        int shieldRow = row + ((color == WHITE) ? 1 : -1);
        if (shieldRow >= 0 && shieldRow < 8)
        {
            for (int dc = -1; dc <= 1; dc++)
            {
                int shieldCol = col + dc;
                if (shieldCol < 0 || shieldCol >= 10) continue;

                struct piece* shield = gs->board[shieldRow][shieldCol];
                if (shield && shield->color == color && shield->piece == ANT)
                    score += 14;
                else
                    score -= 12;
            }
        }
    }
    else
    {
        //In endgames, active kings are stronger, so reward centralisation.
        if (col >= 3 && col <= 6 && row >= 2 && row <= 5)
            score += 25;
        else if (col >= 2 && col <= 7 && row >= 1 && row <= 6)
            score += 10;
    }

    return score;
}

static int getMobility(const struct gameState* gs, enum pieceColor color)
{
    //capture temp state of current board
    struct gameState temp_state = *gs;
    uint32_t moves[MAX_MOVES];
    int moveCount = 0;

    temp_state.currentPlayer = color;
    getPseudoLegalMoves(&temp_state, moves, &moveCount);

    //return legal moves from that position
    return moveCount;
}

int getScore(const struct gameState* gs)
{
    int score = 0;
    int white_no_ant = 0;
    int black_no_ant = 0;
    //int checkmate = 1e8;

    //ranks = rows (row)
    //file  = columns (col)

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 10; col++)
        {
            struct piece* p = gs->board[row][col];
            if (!p || p->piece == KING || p->piece == ANT) continue;

            if (p->color == WHITE) white_no_ant += weight[p->piece];
            else                   black_no_ant += weight[p->piece];
        }
    }

    bool endgame = (white_no_ant + black_no_ant <= 2200);

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
                int row_bonus[] = {0, 5, 10, 20, 35, 55, 80, 120};

                if (tilPromo < 0) tilPromo = 0;
                if (tilPromo > 7) tilPromo = 7;
                value += row_bonus[tilPromo];

            }

            if (p->piece == QUEEN) value += 20;
            
            //discourage these pieces from staying near their home row
            if (p->piece == KNIGHT || p->piece == BISHOP || p->piece == ANTEATER)
                if (!((p->color == WHITE) ? (row == 0) : (row == 7))) 
                    value += 35;

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

            if (p->piece == KING)
                value += kingSafetyScore(gs, p->color, row, col, endgame);

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

    //encourage having more legal moves available
    int whiteMobility = getMobility(gs, WHITE);
    int blackMobility = getMobility(gs, BLACK);
    score += (whiteMobility - blackMobility) * 2;

    // return relative to side to move
    return (gs->currentPlayer == WHITE) ? score : -score;
}

int negaMax(struct gameState* gs, int depth, int alpha, int beta)
{
    nodeCount++;
    //find all pseudolegal moves and filter out self-checks in-search

    //only sample every 2^11 nodes, to reduce get elasped time checks (sort of expensive)
    if (nodeCount % 2048 == 0) 
    { 
        if (get_elapsed_time(time_start) >= time_allot) 
            stop_search = true;
    }
    if (stop_search) return 0;

    int moveCount = 0;
    uint32_t moves[MAX_MOVES];
    //rather than using getMoves to return all legal moves, now just get psuedo legal moves
    getPseudoLegalMoves(gs, moves, &moveCount);

    //only use getScore after ensuring no checkmate condition
    //base condition:
    //run q-search on leaves, will be expensive, but results in better tactics
    if (depth <= 0) return Quiesce(gs, alpha, beta);

    //sort according to MVV-LVA, history, and killer-move bonuses
    preSort(gs, moves, moveCount, depth);

    int bestScore = -INF;
    bool legalMoveFound = false;
    enum pieceColor movingColor = gs->currentPlayer;
    for(int i = 0; i < moveCount; i++)
    {
        struct MoveUndo u;
        applyMove(gs, moves[i], &u);

        //place the legality check here instead
        //prevents having to check legality of all moves each run as it will most likely cutoff early
        if (isColorInCheck(gs, movingColor))
        {
            undoMove(gs, &u);
            continue;
        }

        //reset legal move tracker
        legalMoveFound = true;

        //flip the board for next player
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

    //if no legal moves
    if(!legalMoveFound)
    {
        //and in check
        if(inCheck(gs)) return -INF+depth; //then checkmate
        //and not in check
        else return 0;  //stalemate
    }

    return bestScore;
}

uint32_t depthSearch(struct gameState* gs, int depth, uint32_t pvMove)
{
    int maxScore = -INF;

    //get pseudolegal moves and filter out self-checks in-search
    int moveCount = 0;
    uint32_t moves[MAX_MOVES];
    getPseudoLegalMoves(gs, moves, &moveCount);

    //at the beginning, there is no previous best move
    bool pvFound = false;

    //pick an initial move, maybe add randomness to this
    if(moveCount == 0) return 0; //<---if no pseudolegal moves, don't access moves[0]

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
                pvFound = true;
                break;
            }
        }
    }

    //index offset so the previous best move remains in front
    if (pvFound && moveCount > 1) {
        //sort according to MVV-LVA + History
        preSort(gs, &moves[1], moveCount - 1, depth);
    }
    //start of depth search
    else if (moveCount > 0)
    {
        preSort(gs, moves, moveCount, depth);
    }


    uint32_t bestMove = 0;
    bool legalMoveFound = false;
    
    int alpha = -INF;
    int beta = INF;
    enum pieceColor movingColor = gs->currentPlayer;
    for(int i = 0 ; i < moveCount; i++)
    {
        struct MoveUndo u;
        applyMove(gs, moves[i], &u);

        //skip pseudolegal moves that leave the moving color in check
        if (isColorInCheck(gs, movingColor))
        {
            undoMove(gs, &u);
            continue;
        }

        legalMoveFound = true;

        int score = -negaMax(gs, depth - 1, -beta, -alpha);

        undoMove(gs, &u);

        if(bestMove == 0 || score > maxScore)
        {
            maxScore = score;
            bestMove = moves[i];
        }
        alpha = MAX(alpha, maxScore);
        if(beta <= alpha) break;
    }

    if (!legalMoveFound) return 0;

    return bestMove;
}


uint32_t findBestMove(struct gameState* gs, int depth)
{
    //reset killer moves and history
    memset(K_MOVES, 0, sizeof(K_MOVES));
    memset(HISTORY, 0, sizeof(HISTORY));
    //reset the time trackers
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
    if (bestMove == 0) return;
    applyMove(gs, bestMove, NULL);
}

int MVV_LVA(const struct gameState* gs, uint32_t move, int depth)
{

    //access the specific piece from board
    struct piece* attacker = gs->board[getFromRow(move)][getFromCol(move)];
    struct piece* victim = gs->board[getToRow(move)][getToCol(move)];
    int flags = getFlags(move);

    if (flags == MOVE_EN_PASSANT || flags == MOVE_ANTEATING) {
        return 10100; //tune this, just make it above 10k so q-funct doesnt skip it
    }


    //if not a capture:
    if(!victim)
    {
        int score = HISTORY[getFromRow(move)][getFromCol(move)][getToRow(move)][getToCol(move)];
        int fromRow = getFromRow(move), toRow = getToRow(move);
        int toCol = getToCol(move);

        //have 2 killer moves, the better move is slightly more valued
        if (depth > 0)
        {
            if (move == K_MOVES[depth][0]) return 9500;
            if (move == K_MOVES[depth][1]) return 9400;
        }

        //encourage castling early (TUNE THIS)
        if (flags == MOVE_CASTLE_KS || flags == MOVE_CASTLE_QS)
            score += 9800;

        //to encourage advancement/less repetitive moves
        if (attacker)
        {
            //encourage the following pieces to leave the homerow
            if (attacker->piece == KNIGHT || attacker->piece == BISHOP || attacker->piece == ANTEATER)
            {
                int homeRow = (attacker->color == WHITE) ? 0 : 7;
                if (fromRow == homeRow && toRow != homeRow)
                    score += 80;
            }

            //encourage developement (TUNE)
            if (attacker->piece == ANT)
            {
                int startRow = (attacker->color == WHITE) ? 1 : 6;
                if (fromRow == startRow)
                {
                    //near the center, give a bonus
                    if (toCol >= 3 && toCol <= 6) score += 35;
                    //on the sides, remove some score
                    if (toCol == 0 || toCol == 9) score -= 20;
                }
            }

            //penalty for not castling(tune) if you ahve chance to castle
            if (attacker->piece == KING && flags != MOVE_CASTLE_KS && flags != MOVE_CASTLE_QS)
                score -= 120;
        }

        //seperated the bonuses for center
        //inner center gives more
        if (toCol >= 3 && toCol <= 6 && toRow >= 2 && toRow <= 5)
            score += 12;
        //a bit further outside, it still gives a bonus, but much less
        else if (toCol >= 2 && toCol <= 7 && toRow >= 1 && toRow <= 6)
            score += 6;

        return score;
    }


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
    //if capture, subtract the victim's value by attackers value
    //goal is to get the highest victim value, lowest attacker value
    //ex: pawn->queen
    return 10000 + weight[victim->piece] - weight[attacker->piece];

}

void preSort(const struct gameState* gs, uint32_t* moves, int moveCount, int depth)
{
    //compute weights
    int scores[MAX_MOVES];
    for(int i = 0 ; i<moveCount; i++)
        scores[i] = MVV_LVA(gs, moves[i], depth);

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

    //once again, this is used for optimization, sample every so often instead of every node(becomes very expensive)
    if (nodeCount % 2048 == 0) 
    { 
        if (get_elapsed_time(time_start) >= time_allot) 
            stop_search = true;
    }
    if (stop_search) return alpha;

    bool currentPlayerInCheck = inCheck(gs);
    int static_score = getScore(gs);

    int best_score = static_score;
    if (!currentPlayerInCheck)
    {
        if(best_score >= beta)
            return best_score;
        if(best_score > alpha)
            alpha = best_score;
    }
    else
    {
        best_score = -INF;
    }
    
    uint32_t moves[MAX_MOVES];
    int moveCount = 0;
    getPseudoLegalMoves(gs, moves, &moveCount);
    preSort(gs, moves, moveCount, 0); //give better moves first for earlier beta cutoffs

    bool legalMoveFound = false;
    enum pieceColor movingColor = gs->currentPlayer;
    for(int i = 0; i< moveCount ; i++)
    {
        uint32_t move = moves[i];
        bool captureOrPromo = isCapture(gs, move) || isPromotion(move);

        struct MoveUndo u;
        applyMove(gs, move, &u);

        //skip pseudolegal moves that leave moving piece in check (similar to negamax)
        if (isColorInCheck(gs, movingColor))
        {
            undoMove(gs, &u);
            continue;
        }

        legalMoveFound = true;

        //When not in check, skip non quiet moves
        if (!currentPlayerInCheck && !captureOrPromo)
        {
            undoMove(gs, &u);
            continue;
        }

        int score = -Quiesce(gs, -beta, -alpha);

        undoMove(gs, &u);

        if(score >= beta)
            return score;
        if(score > best_score)
            best_score = score;
        if(score > alpha)
            alpha = score;
    }

    if (!legalMoveFound)
        return currentPlayerInCheck ? -INF : 0;

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
