#include "Hash.h"

//zobrist[row][col][pieceType]
static uint64_t zobrist[8][10][14];
static bool z_isInit = false;

extern bool wouldLeaveKingInCheck(struct piece* board[8][10],
                                  int fromRow, int fromCol,
                                  int toRow,   int toCol,
                                  enum pieceColor color, bool isEnPassant);

uint64_t HASHES[MAX_REPETITION_HISTORY];
int currentPly = 0;

static void initZ(void)
{
    //if initialized, stop
    if (z_isInit)
        return;

    //loop through the board
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 10; col++)
        {
            //loop through the piece
            for (int piece = 0; piece < 14; piece++)
                //generate a 64 bit number to represent the current board position
                //note* rand had to be run on 2 different 32 bit numbers because it has a limit of 31 effective bits
                //effectively places a 62 bit int into the table (still more than enough to represent)
                //large to avoid collisions
                zobrist[row][col][piece] = ((uint64_t)rand() << 32) | (uint64_t)rand();
        }
    }

    z_isInit = true;
}

static bool hasLegalEnPassant(const struct gameState* gs)
{
    //only hash en passant if a legal capture actually exists
    if (gs->enPassantCol < 0 || gs->enPassantCol >= 10 ||
        gs->enPassantRow < 0 || gs->enPassantRow >= 8)
        return false;

    int fromRow = gs->enPassantRow;
    int toRow = fromRow + ((gs->currentPlayer == WHITE) ? 1 : -1);
    if (toRow < 0 || toRow >= 8)
        return false;

    for (int deltaCol = -1; deltaCol <= 1; deltaCol += 2)
    {
        int fromCol = gs->enPassantCol + deltaCol;
        if (fromCol < 0 || fromCol >= 10)
            continue;

        struct piece* p = gs->board[fromRow][fromCol];
        if (!p || p->piece != ANT || p->color != gs->currentPlayer)
            continue;

        if (!wouldLeaveKingInCheck((struct piece* (*)[10])gs->board,
                                   fromRow, fromCol,
                                   toRow, gs->enPassantCol,
                                   gs->currentPlayer, true))
            return true;
    }

    return false;
}

//standard zorbrist hash funciton, necessary to check for repetitions and transposition table if needed
uint64_t positionHash(const struct gameState* gs)
{
    if (!z_isInit)
        initZ();

    uint64_t hash = 0;

    //loop through the board
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 10; col++)
        {
            //find pieces, if it exists
            struct piece* p = gs->board[row][col];
            if (!p)
                continue;

            //further encode according to color
            //each piece gets 2 slots
            //ex : black ant vs white ant, WHITE is 1, BLACK is 0
            //doing p->piece * 2 + 0/1 allows an easy encode and allows the use of 3d array instead of 4d
            int encodeColor = p->piece * 2 + p->color;
            //xor allows lightweight switch (according to chess programming wiki)
            hash ^= zobrist[row][col][encodeColor];
        }
    }

    //add the side to move
    if (gs->currentPlayer == BLACK)
        //apparently good number for cache?
        hash ^= 0x9e3779b97f4a7c15ULL;

    //castling rights
    if (gs->whiteKingMoved) hash ^= 0x1;
    if (gs->blackKingMoved) hash ^= 0x2;
    if (gs->whiteRookMovedQS) hash ^= 0x4;
    if (gs->whiteRookMovedKS) hash ^= 0x8;
    if (gs->blackRookMovedQS) hash ^= 0x10;
    if (gs->blackRookMovedKS) hash ^= 0x20;

    //en passant
    if (hasLegalEnPassant(gs))
        hash ^= ((uint64_t)gs->enPassantCol << 32);

    return hash;
}

bool isThreeFoldDraw(uint64_t hash, int curPly)
{
    int repetitions = 0;

    //loop through until it reachs the current half move
    for (int i = 0; i < curPly; i++)
    {
        //incremenet repeitions as you go
        if (HASHES[i] == hash)
        {
            repetitions++;
            //threefold draw condition
            if (repetitions >= 3)
                return true;
        }
    }

    return false;
}

void storePositionHash(const struct gameState* gs)
{
    if (currentPly < MAX_REPETITION_HISTORY)
    {
        //hash the current move and increment the counter
        HASHES[currentPly] = positionHash(gs);
        currentPly++;
    }
}

//reset for new games
void resetRepetitionTracking(void)
{
    currentPly = 0;
    memset(HASHES, 0, sizeof(HASHES));
}
