#include "Engine.h"

//FORWARD DECLARATIONS==================================================================================START
static bool isSearchDraw(const struct gameState* gs, uint64_t posHash, int ply);
static int negaMax(struct gameState* gs, int depth, int alpha, int beta, int ply);
static int Quiesce(struct gameState* gs, int alpha, int beta, int ply);
static inline bool isCapture(const struct gameState* gs, uint32_t move);
static inline double get_current_time(void);
static inline double get_elapsed_time(double start);
static inline bool isPromotion(uint32_t move);
//FORWARD DECLARATIONS==================================================================================END

//TRACKING VARIABLES AND DEBUG==========================================================================START

int nodeCount = 0;

bool stop_search = false;
double time_start = 0;
double time_allot = 2000; //2 seconds alloted per move (can change)
static uint64_t SEARCH_HASHES[SEARCH_STACK_SIZE];

//start of killer move implementation
uint32_t K_MOVES[MAX_DEPTH][2];    //max depth, 2 killer moves stored per depth
//history implementation
int HISTORY[8][10][8][10];  //row col row col

//TRACKING VARIABLES AND DEBUG==========================================================================END

#define ASPIRATION_WINDOW 50

static int negaMax(struct gameState* gs, int depth, int alpha, int beta, int ply)
{
    nodeCount++;
    int originalAlpha = alpha; //store for TT flag

    //sample every 2048 nodes
    if (nodeCount % 2048 == 0) 
    { 
        if (get_elapsed_time(time_start) >= time_allot) 
            stop_search = true;
    }
    if (stop_search) return originalAlpha;

    //always check the t_table before running this to exit earlier
    uint64_t posHash = positionHash(gs);
    if (isSearchDraw(gs, posHash, ply))
        return 0;

    if (ply > 0 && ply - 1 < SEARCH_STACK_SIZE)
        SEARCH_HASHES[ply - 1] = posHash;

    int ttScore;
    if (lookupTT(posHash, depth, alpha, beta, &ttScore, ply))
        return ttScore;

    //only use getScore after ensuring no checkmate condition
    //base condition:
    //run q-search on leaves, will be expensive, but results in better tactics
    if (depth <= 0) return Quiesce(gs, alpha, beta, ply);

    int moveCount = 0;
    uint32_t moves[MAX_MOVES];
    //gather all psuedo legal moves
    getPseudoLegalMoves(gs, moves, &moveCount);

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

        //search from other player's perspective
        int score = -negaMax(gs, depth - 1, -beta, -alpha, ply + 1);

        undoMove(gs, &u);

        //premature exit
        if (stop_search) return alpha;


        if(score > bestScore)
            bestScore = score;

        //recompute alpha(lower bound), if bestScore is higher, then that is the new low bound
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
        if(inCheck(gs)) return -INF + ply; //then checkmate
        //and not in check
        else return 0;  //stalemate
    }

    //store score, depth, flag(as bounds)
    int flag;
    if (bestScore <= originalAlpha)
        flag = TT_FLAG_UPPER;
    else if (bestScore >= beta)
        flag = TT_FLAG_LOWER;
    else
        flag = TT_FLAG_EXACT;
    
    storeTT(posHash, bestScore, depth, flag, ply);
    
    return bestScore;
}

uint32_t depthSearch(struct gameState* gs, int depth, uint32_t pvMove, int alpha, int beta, int* outputScore)
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
    //else order the moves
    else if (moveCount > 0)
    {
        preSort(gs, moves, moveCount, depth);
    }


    uint32_t bestMove = 0;
    bool legalMoveFound = false;
    
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


        //apply negamax to get score of move
        int score = -negaMax(gs, depth - 1, -beta, -alpha, 1);

        undoMove(gs, &u);

        if (stop_search) return bestMove;

        //if this is not the first run, and the score is max
        if(bestMove == 0 || score > maxScore)
        {
            //set new max and assign as best move
            maxScore = score;
            bestMove = moves[i];
        }
        //reassign the lower bound
        alpha = MAX(alpha, maxScore);
        if(beta <= alpha) break; //lower bound > upper bound? exit
    }

    //if not legal moves were found, return 0 for draw/stalemate
    if (!legalMoveFound)
    {
        *outputScore = inCheck(gs) ? -INF : 0;
        return 0;
    }

    *outputScore = maxScore;

    return bestMove;
}


uint32_t findBestMove(struct gameState* gs, int depth)
{
    //reset killer moves and history
    memset(K_MOVES, 0, sizeof(K_MOVES));
    memset(HISTORY, 0, sizeof(HISTORY));
    memset(SEARCH_HASHES, 0, sizeof(SEARCH_HASHES));
    //reset the time trackers
    stop_search = false;

    //always keep a legal fallback move in case the search times out
    uint32_t legalMoves[MAX_MOVES];
    int legalMoveCount = 0;
    getMoves(gs, legalMoves, &legalMoveCount);
    if (legalMoveCount == 0)
        return 0;

    time_start = get_current_time();

    uint32_t bestMove = legalMoves[0];
    uint32_t previousBestMove = bestMove;
    int previousScore = 0;

    //iterate at each depth
    for(int i = 1 ; i <= depth; i++)
    {
        int alpha = -INF;
        int beta = INF;
        int score = 0;

        if (i > 1)
        {
            alpha = previousScore - ASPIRATION_WINDOW;
            beta = previousScore + ASPIRATION_WINDOW;
        }

        //return the best move at the depth
        uint32_t move = depthSearch(gs, i, previousBestMove, alpha, beta, &score);

        if (stop_search)
            break;

        //if the score fell outside the window, research with the full bounds
        if (i > 1 && (score <= alpha || score >= beta))
        {
            move = depthSearch(gs, i, previousBestMove, -INF, INF, &score);
            if (stop_search)
                break;
        }

        //if move is valid, then set it as best
        previousBestMove = move;
        bestMove = move;
        previousScore = score;
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
    storePositionHash(gs);
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

    if (isPromotion(move)) 
    {
        int promoValue = 0;
        switch (flags)
        {
            case MOVE_PROMO_QUEEN:    promoValue = weight[QUEEN]; break;
            case MOVE_PROMO_ROOK:     promoValue = weight[ROOK]; break;
            case MOVE_PROMO_BISHOP:   promoValue = weight[BISHOP]; break;
            case MOVE_PROMO_KNIGHT:   promoValue = weight[KNIGHT]; break;
            case MOVE_PROMO_ANTEATER: promoValue = weight[ANTEATER]; break;
        }

        return 10000 + promoValue + (victim ? weight[victim->piece] : 0);
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
            score += 800;

        //to encourage advancement/less repetitive moves
        if (attacker)
        {
            //encourage the following pieces to leave the homerow
            if (attacker->piece == KNIGHT || attacker->piece == BISHOP || attacker->piece == ANTEATER)
            {
                int homeRow = (attacker->color == WHITE) ? 0 : 7;
                if (fromRow == homeRow && toRow != homeRow)
                    score += 35;
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
                score -= 50;
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

static int Quiesce(struct gameState* gs, int alpha, int beta, int ply)
{
    nodeCount++;

    bool currentPlayerInCheck = inCheck(gs);
    uint64_t posHash = positionHash(gs);
    if (isSearchDraw(gs, posHash, ply))
        return 0;

    if (ply > 0 && ply - 1 < SEARCH_STACK_SIZE)
        SEARCH_HASHES[ply - 1] = posHash;

    //once again, this is used for optimization, sample every so often instead of every node(becomes very expensive)
    if (nodeCount % 2048 == 0) 
    { 
        if (get_elapsed_time(time_start) >= time_allot) 
            stop_search = true;
    }
    if (stop_search) return alpha;

    int static_score = getScore(gs);

    //if in check, look to uncheck -inf is high penalty
    int best_score = currentPlayerInCheck ? -INF : static_score;

    if (!currentPlayerInCheck)
    {
        if(best_score >= beta)
            return best_score;
        if(best_score > alpha)
            alpha = best_score;
    }
    
    uint32_t moves[MAX_MOVES];
    int moveCount = 0;
    getPseudoLegalMoves(gs, moves, &moveCount);
    preSort(gs, moves, moveCount, 0); //give better moves first for earlier beta cutoffs

    bool anyLegalMove = false;
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

        anyLegalMove = true;

        if (!currentPlayerInCheck && !captureOrPromo)
        {
            undoMove(gs, &u);
            continue;
        }

        int score = -Quiesce(gs, -beta, -alpha, ply + 1);

        undoMove(gs, &u);

        if(score >= beta)
            return score;
        if(score > best_score)
            best_score = score;
        if(score > alpha)
            alpha = score;
    }

    if (!anyLegalMove)
    {
        if (currentPlayerInCheck)
            return -INF + ply;
        else
            return 0;
    }

    return best_score;
}

static inline bool isCapture(const struct gameState* gs, uint32_t move)
{
    int toRow = getToRow(move); int toCol = getToCol(move); int flags = getFlags(move);

    if (toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 10) return false;

    if (flags == MOVE_ANTEATING || flags == MOVE_EN_PASSANT) return true;

    //check if theres a piece and if that piece is an enemy piece (return it)
    struct piece* victim = gs->board[toRow][toCol];

    //return true if there is a opponent piece that can be captured
    return (victim && victim->color != gs->currentPlayer);
}

static inline double get_current_time(void) 
{
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}

static inline double get_elapsed_time(double start) 
{
    return get_current_time() - start;
}

static inline bool isPromotion(uint32_t move)
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

static bool isSearchDraw(const struct gameState* gs, uint64_t posHash, int ply)
{
    //repetition count, if inputted ply is > 0, current node needs to be counted
    int repetitions = (ply > 0) ? 1 : 0;

    //50 move rule
    if (gs->halfMove_count >= 100)
        return true;

    //repeat hashes until reaching current ply (half move)
    for (int i = 0; i < currentPly; i++)
    {
        //if the exact position was found
        if (HASHES[i] == posHash)
        {
            //increment and check for 3 fold draw rule
            repetitions++;
            if (repetitions >= 3)
                return true;
        }
    }

    //if its one move down, start comparing ancestors
    if (ply > 1)
    {
        //if u are on move 5, there are 4 moves before that
        int ancestorCount = ply - 1;
        if (ancestorCount > SEARCH_STACK_SIZE) //out of bounds fall back
            ancestorCount = SEARCH_STACK_SIZE;

        //check all the moves that led up to the current
        for (int i = 0; i < ancestorCount; i++)
        {
            //if any of them match, then increment repetition counter
            if (SEARCH_HASHES[i] == posHash)
            {
                repetitions++;
                //if 3 or more matches, then its 3 fold draw rule
                if (repetitions >= 3)
                    return true;
            }
        }
    }

    return false;
}
