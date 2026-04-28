#include "Engine.h"
#include "Ant.h"

//weight table
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

//to prevent risking king unnessarily
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

    //checkmates are harder without queen so detect it first
    bool oppQueen_alive = false;
    //scan through the board
    for (int scanRow = 0; scanRow < 8 && !oppQueen_alive; scanRow++)
    {
        for (int scanCol = 0; scanCol < 10; scanCol++)
        {
            struct piece* p = gs->board[scanRow][scanCol];
            //check for queen piece
            if (p && p->color == opponent && p->piece == QUEEN)
            {
                //if queen found, exit
                oppQueen_alive = true;
                break;
            }
        }
    }

    if (!endgame)
    {
        //promote castling / sheltered king positions
        if (castled)
            score += 70;
        else if (row == homeRow && col == homeKingCol)
            score -= canStillCastle ? 15 : 55;
        else
            score -= oppQueen_alive ? 70 : 50;

        int pawnShieldRow = row + ((color == WHITE) ? 1 : -1);
        if (pawnShieldRow >= 0 && pawnShieldRow < 8)
        {
            //check the movement range of king
            for (int deltaCol = -1; deltaCol <= 1; deltaCol++)
            {
                int pawnShieldCol = col + deltaCol;
                if (pawnShieldCol < 0 || pawnShieldCol >= 10)
                    continue;

                //detect potential pawns around king
                struct piece* isPawn = gs->board[pawnShieldRow][pawnShieldCol];
                //if there exists friendly pawns, add a bonus
                if (isPawn && isPawn->color == color && isPawn->piece == ANT)
                    score += 10;
                else
                    //if there are no friendly pawns, or near other pieces, lower the score
                    score -= 8;
            }
        }

        //a king in the center is riskier before the endgame
        if (!castled && col >= 3 && col <= 6)
            score -= oppQueen_alive ? 12 : 8;

        //slight penalty if the king left home early without castling
        if (!castled && kingMoved && row != homeRow)
            score -= 10;
    }
    else
    {
        //if in endgame, try to make king centralize
        if (col >= 3 && col <= 6 && row >= 2 && row <= 5)
            score += 20;
        else if (col >= 2 && col <= 7 && row >= 1 && row <= 6)
            score += 8;
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

    //return pseudolegal moves from that position
    return moveCount;
}

//include anteater attack patterns when evaluating ant safety
static bool isEvalSquareAttackedBy(const struct gameState* gs, int row, int col, enum pieceColor attackerColor)
{
    if (isSquareAttackedBy((struct piece* (*)[10])gs->board, row, col, attackerColor))
        return true;

    struct piece* target = gs->board[row][col];
    if (!target || target->piece != ANT)
        return false;

    for (int scanRow = 0; scanRow < 8; scanRow++)
    {
        for (int scanCol = 0; scanCol < 10; scanCol++)
        {
            struct piece* p = gs->board[scanRow][scanCol];
            if (!p || p->piece != ANTEATER || p->color != attackerColor)
                continue;

            uint32_t moves[MAX_MOVES];
            int moveCount = 0;
            getAnteaterMoves((struct piece* (*)[10])gs->board, scanRow, scanCol, moves, &moveCount);

            for (int i = 0; i < moveCount; i++)
            {
                if (getFlags(moves[i]) == MOVE_ANTEATING &&
                    getToRow(moves[i]) == row &&
                    getToCol(moves[i]) == col)
                    return true;
            }
        }
    }

    return false;
}

int getScore(const struct gameState* gs)
{
    int score = 0;
    //to check for endgame
    int white_exclude_ants = 0;
    int black_exclude_ants = 0;

    //scan through board
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 10; col++)
        {
            struct piece* p = gs->board[row][col];
            //skip king and ants
            if (!p || p->piece == KING || p->piece == ANT)
                continue;

            //non-ant piece adds to the weight count
            if (p->color == WHITE)
                white_exclude_ants += weight[p->piece];
            else
                black_exclude_ants += weight[p->piece];
        }
    }

    //to check for endgame return the weights of non ant pieces true if less than 2200 (TUNE THIS)
    bool endgame = (white_exclude_ants + black_exclude_ants <= 2200);

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 10; col++)
        {
            struct piece* p = gs->board[row][col];
            if (!p)
                continue;

            //get value of piece
            int value = weight[p->piece];

            //due to the engine picking the same pawn to move in a straight line
            //positional multipliers were added
            if (p->piece == ANT)
            {
                //distance traveled by pawn from starting pos
                int progress = (p->color == WHITE) ? (row - 1) : (6 - row);
                int row_bonus[] = {0, 10, 20, 40, 70, 110, 170, 250};

                if (progress < 0)
                    progress = 0;
                if (progress > 7)
                    progress = 7;
                value += row_bonus[progress];
            }


            if (p->piece == ANTEATER)
            {
                int near_Ant = 0;
                int chainAnt = 0;
                enum pieceColor opponent_color = (p->color == WHITE) ? BLACK : WHITE;
                int opponent_ants = (p->color == WHITE) ? gs->blackAntCount : gs->whiteAntCount;
                value = 40 + (290 * opponent_ants / 10);; //lower the base value depending on remaining enemy pawns
                //ex: at 10 pawns, valued at 330, but becomes less than a pawn from 2 remaining ants and lower. (TUNE)


                //(deltaR, deltaC) <--- (change in row, change in col)
                //iterate through all directions aka 1 move in orthogonal or diagonal
                for (int deltaRow = -1; deltaRow <= 1; deltaRow++)
                {
                    for (int deltaCol = -1; deltaCol <= 1; deltaCol++)
                    {
                        //ignore self
                        if (deltaRow == 0 && deltaCol == 0)
                            continue;

                        //find new row and col
                        int newRow = row + deltaRow;
                        int newCol = col + deltaCol;
                        //make sure its not out of bounds ... segfault
                        if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 10)
                            continue;

                        //identify the victim
                        struct piece* victim = gs->board[newRow][newCol];
                        //check if theres a piece there, check if its an ant, check if its the color of opponent
                        if (victim && victim->piece == ANT && victim->color == opponent_color)
                        {
                            //theres an ant nearby
                            near_Ant++;

                            int edirs[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};

                            //scan through chaining
                            for (int d = 0; d < 4; d++)
                            {
                                int chainRow = newRow + edirs[d][0];
                                int chainCol = newCol + edirs[d][1];

                                //while in bounds
                                while (chainRow >= 0 && chainRow < 8 && chainCol >= 0 && chainCol < 10)
                                {
                                    struct piece* chainVictim = gs->board[chainRow][chainCol];
                                    if (!chainVictim || chainVictim->piece != ANT || chainVictim->color != opponent_color)
                                        break;

                                    //new ant to chain
                                    chainAnt++;
                                    //check next
                                    chainRow += edirs[d][0];
                                    chainCol += edirs[d][1];
                                }
                            }
                        }
                    }
                }

                //bonuses (arbitrary TO ADJUST LATER)
                value += near_Ant * (weight[ANT] / 20);
                value += MIN(chainAnt, 4) * (weight[ANT] / 30);

                //increase value for ability to capture on current turn
                if (near_Ant > 0)
                    value += 10;
            }

            //discourage these pieces from staying near their home row
            if (p->piece == KNIGHT || p->piece == BISHOP || p->piece == ANTEATER)
            {
                if (!((p->color == WHITE) ? (row == 0) : (row == 7)))
                    value += 35;
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

    //penalize hanging
    //loop through board
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 10; col++)
        {
            struct piece* p = gs->board[row][col];
            //skip empty squares and king (offloaded to kingsafety fxn)
            if (!p || p->piece == KING)
                continue;

            //find if the square can be attacked by opponent
            enum pieceColor opponent = (p->color == WHITE) ? BLACK : WHITE;
            //if the square can be attacked
            if (isEvalSquareAttackedBy(gs, row, col, opponent))
            {
                bool isDefended = isEvalSquareAttackedBy(gs, row, col, p->color);
                //if no friendly pieces detected, assign as hanging
                if (!isDefended)
                {
                    //reduce the square by 1/4 if hanging
                    int penalty = weight[p->piece] / 4;
                    score += (p->color == WHITE) ? -penalty : penalty;
                }
            }
        }
    }

    //encourage having more legal moves available
    int whiteMobility = getMobility(gs, WHITE);
    int blackMobility = getMobility(gs, BLACK);
    score += (whiteMobility - blackMobility) * 2;

    // return relative to side to move
    return (gs->currentPlayer == WHITE) ? score : -score;
}
