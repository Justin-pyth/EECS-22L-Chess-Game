#include "Moves.h"
#include "Ant.h"
#include <stdlib.h>
#include <string.h>

extern bool isKingInCheck(struct piece* board[8][10], enum pieceColor color);
extern bool wouldLeaveKingInCheck(struct piece* board[8][10],
                                  uint32_t move,
                                  enum pieceColor color);
extern bool isCastlingValid(struct piece* board[8][10], enum pieceColor color,
                             bool kingSide, struct gameState* state);
extern struct piece* allocatePromotion(enum pieceType type, enum pieceColor color);
extern int getPromotionCount(void);
extern void setPromotionCount(int count);

/* ── inCheck ────────────────────────────────────────────────────────── */

bool inCheck(const struct gameState* gs)
{
    return isColorInCheck(gs, gs->currentPlayer);
}

static inline void decrementAntCount(struct gameState* gs, struct piece* p)
{
    if (!p || p->piece != ANT)
        return;

    if (p->color == WHITE) gs->whiteAntCount--;
    else                   gs->blackAntCount--;
}

/*
    getAntMoves
    Returns all possible moves that a given Ant (pawn) piece can make
*/
void getAntMoves(struct piece* board[8][10], int row, int col, uint32_t* moves, int* moveCount)
{
    struct piece* p = board[row][col];
    *moveCount = 0;
    
    int dir      = (p->color == WHITE) ?  1 : -1;
    int startRow = (p->color == WHITE) ?  1 :  6;
    enum pieceColor opp = (p->color == WHITE) ? BLACK : WHITE;

    int nr = row + dir;
    if (nr >= 0 && nr < 8 && board[nr][col] == NULL) 
    {
        moves[(*moveCount)++] = createMove(row, col, nr, col, MOVE_NORMAL);
        
        if (row == startRow) 
        {
            int nr2 = row + 2 * dir;
            if (nr2 >= 0 && nr2 < 8 && board[nr2][col] == NULL)
                moves[(*moveCount)++] = createMove(row, col, nr2, col, MOVE_NORMAL);
        }
    }
    for (int dc = -1; dc <= 1; dc += 2) 
    {
        int nc = col + dc;
        nr = row + dir;
        if (nr >= 0 && nr < 8 && nc >= 0 && nc < 10 &&
            board[nr][nc] != NULL && board[nr][nc]->color == opp)
            moves[(*moveCount)++] = createMove(row, col, nr, nc, MOVE_NORMAL);
    }
}

/*
    getBishopMoves
    Returns all possible moves that a given Bishop piece can make
*/
void getBishopMoves(struct piece* board[8][10], int row, int col, uint32_t* moves, int* moveCount)
{
    struct piece* p = board[row][col];
    *moveCount = 0;
    enum pieceColor opp = (p->color == WHITE) ? BLACK : WHITE;

    // checks diagonal squares
    int dirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};

    for (int d = 0; d < 4; d++) 
    {
        int r = row + dirs[d][0], c = col + dirs[d][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 10) 
        {
            if (board[r][c] == NULL) 
            {
                moves[(*moveCount)++] = createMove(row, col, r, c, MOVE_NORMAL);
            } 
            else 
            {
                if (board[r][c]->color == opp)
                moves[(*moveCount)++] = createMove(row, col, r, c, MOVE_NORMAL);
                break;
            }
            r += dirs[d][0]; c += dirs[d][1];
        }
    }
}

/*
    getKnightMoves
    Returns all possible moves that a given Knight piece can make
*/
void getKnightMoves(struct piece* board[8][10], int row, int col, uint32_t* moves, int* moveCount)
{
    struct piece* p = board[row][col];
    *moveCount = 0;

    // checks L-shaped jumps
    int jumps[8][2] = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};

    for (int i = 0; i < 8; i++) 
    {
        int r = row + jumps[i][0], c = col + jumps[i][1];
        if (r >= 0 && r < 8 && c >= 0 && c < 10 &&
            (board[r][c] == NULL || board[r][c]->color != p->color))
            moves[(*moveCount)++] = createMove(row, col, r, c, MOVE_NORMAL);
    }
}

/*
    getRookMoves
    Returns all possible moves that a given Rook piece can make
*/
void getRookMoves(struct piece* board[8][10], int row, int col, uint32_t* moves, int* moveCount)
{
    struct piece* p = board[row][col];
    *moveCount = 0;
    enum pieceColor opp = (p->color == WHITE) ? BLACK : WHITE;

    // checks horizontal ranks and vertical files
    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

    for (int d = 0; d < 4; d++) 
    {
        int r = row + dirs[d][0], c = col + dirs[d][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 10) 
        {
            if (board[r][c] == NULL) 
            {
                moves[(*moveCount)++] = createMove(row, col, r, c, MOVE_NORMAL);
            } 
            else 
            {
                if (board[r][c]->color == opp)
                    moves[(*moveCount)++] = createMove(row, col, r, c, MOVE_NORMAL);
                break;
            }
            r += dirs[d][0]; c += dirs[d][1];
        }
    }
}

/*
    getQueenMoves
    Returns all possible moves that a given Queen piece can make
*/
void getQueenMoves(struct piece* board[8][10], int row, int col, uint32_t* moves, int* moveCount)
{
    struct piece* p = board[row][col];
    *moveCount = 0;
    enum pieceColor opp = (p->color == WHITE) ? BLACK : WHITE;

    // checks diagonals, ranks, and files
    int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};

    for (int d = 0; d < 8; d++) 
    {
        int r = row + dirs[d][0], c = col + dirs[d][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 10) 
        {
            if (board[r][c] == NULL) 
            {
                moves[(*moveCount)++] = createMove(row, col, r, c, MOVE_NORMAL);
            } 
            else 
            {
                if (board[r][c]->color == opp)
                    moves[(*moveCount)++] = createMove(row, col, r, c, MOVE_NORMAL);
                break;
            }
            r += dirs[d][0]; c += dirs[d][1];
        }
    }
}

/*
    getKingMoves
    Returns all possible moves that a given King piece can make
*/
void getKingMoves(struct piece* board[8][10], int row, int col, uint32_t* moves, int* moveCount)
{
    struct piece* p = board[row][col];
    *moveCount = 0;

    for (int dr = -1; dr <= 1; dr++) 
    {
        for (int dc = -1; dc <= 1; dc++) 
        {
            if (dr == 0 && dc == 0) continue;
            int r = row + dr, c = col + dc;

            if (r >= 0 && r < 8 && c >= 0 && c < 10 &&
                (board[r][c] == NULL || board[r][c]->color != p->color))
                moves[(*moveCount)++] = createMove(row, col, r, c, MOVE_NORMAL);
        }
    }
}

/*  getPseudoLegalMoves — calls get*Moves, converts results, appends special moves */

void getPseudoLegalMoves(struct gameState* gs, Move* moves, int* moveCount)
{
    *moveCount = 0;
    enum pieceColor color = gs->currentPlayer;
    int dir      = (color == WHITE) ?  1 : -1;
    int promoRow = (color == WHITE) ?  7 :  0;
    static const int promoFlags[5] = {
        MOVE_PROMO_QUEEN, MOVE_PROMO_ROOK, MOVE_PROMO_BISHOP,
        MOVE_PROMO_KNIGHT, MOVE_PROMO_ANTEATER
    };

    // temp buffer for raw moves before legality check
    Move raw[MAX_MOVES];
    int rawCount = 0;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 10; c++) {
            struct piece* p = gs->board[r][c];
            if (!p || p->color != color) continue;

            rawCount = 0;

            switch (p->piece) {
                case ANT:      getAntMoves     (gs->board, r, c, raw, &rawCount); break;
                case BISHOP:   getBishopMoves  (gs->board, r, c, raw, &rawCount); break;
                case KNIGHT:   getKnightMoves  (gs->board, r, c, raw, &rawCount); break;
                case ROOK:     getRookMoves    (gs->board, r, c, raw, &rawCount); break;
                case QUEEN:    getQueenMoves   (gs->board, r, c, raw, &rawCount); break;
                case KING:     getKingMoves    (gs->board, r, c, raw, &rawCount); break;
                case ANTEATER: getAnteaterMoves(gs->board, r, c, raw, &rawCount); break;
            }

            for (int i = 0; i < rawCount; i++) {
                int fr = getFromRow(raw[i]), fc = getFromCol(raw[i]);
                int tr = getToRow  (raw[i]), tc = getToCol  (raw[i]);
                int flags = getFlags(raw[i]);

                // pawn promotion
                if (p->piece == ANT && tr == promoRow) {
                    for (int pi = 0; pi < 5; pi++)
                        moves[(*moveCount)++] = createMove(fr, fc, tr, tc, promoFlags[pi]);
                    continue;
                }

                // anteater eating flag
                if (p->piece == ANTEATER && flags == MOVE_ANTEATING) {
                    moves[(*moveCount)++] = raw[i];
                    continue;
                }

                // normal move
                moves[(*moveCount)++] = createMove(fr, fc, tr, tc, MOVE_NORMAL);
            }

            // en passant
            if (p->piece == ANT && gs->enPassantCol >= 0 && gs->enPassantRow == r) {
                int dc = gs->enPassantCol - c;
                if (dc == 1 || dc == -1) {
                    int epToRow = r + dir;
                    moves[(*moveCount)++] = createMove(r, c, epToRow,
                                                       gs->enPassantCol,
                                                       MOVE_EN_PASSANT);
                }
            }

            // castling
            if (p->piece == KING) {
                if (isCastlingValid(gs->board, color, true,  gs))
                    moves[(*moveCount)++] = createMove(r, c, r, 7, MOVE_CASTLE_KS);
                if (isCastlingValid(gs->board, color, false, gs))
                    moves[(*moveCount)++] = createMove(r, c, r, 3, MOVE_CASTLE_QS);
            }
        }
    }
}

//getMoves still outputs only legal moves
void getMoves(struct gameState* gs, Move* moves, int* moveCount)
{
    //create a buffer to store psuedolegal moves
    Move pseudoMoves[MAX_MOVES];
    int cnt = 0;
    enum pieceColor color = gs->currentPlayer;

    //get psuedolegal moves and the count of psuedolegal moves
    getPseudoLegalMoves(gs, pseudoMoves, &cnt);

    *moveCount = 0;
    //iterate through the list of psuedo legal moves
    for (int i = 0; i < cnt; i++) 
    {
        //if that move doesn't leave king in check, LEGAL
        if (!wouldLeaveKingInCheck(gs->board, pseudoMoves[i], color))
            moves[(*moveCount)++] = pseudoMoves[i];
    }
}

static void saveUndo(const struct gameState* gs, struct MoveUndo* u)
{
    memcpy(u->board, gs->board, sizeof(gs->board));
    u->currentPlayer    = gs->currentPlayer;
    u->whiteAntCount    = gs->whiteAntCount;
    u->blackAntCount    = gs->blackAntCount;
    u->whiteKingMoved   = gs->whiteKingMoved;
    u->blackKingMoved   = gs->blackKingMoved;
    u->whiteRookMovedQS = gs->whiteRookMovedQS;
    u->whiteRookMovedKS = gs->whiteRookMovedKS;
    u->blackRookMovedQS = gs->blackRookMovedQS;
    u->blackRookMovedKS = gs->blackRookMovedKS;
    u->enPassantCol     = gs->enPassantCol;
    u->enPassantRow     = gs->enPassantRow;
    u->halfMove_count   = gs->halfMove_count;
    u->fullMove_count   = gs->fullMove_count;
    u->promotionCount   = getPromotionCount();
}

void undoMove(struct gameState* gs, const struct MoveUndo* u)
{
    memcpy(gs->board, u->board, sizeof(gs->board));
    gs->currentPlayer    = u->currentPlayer;
    gs->whiteAntCount    = u->whiteAntCount;
    gs->blackAntCount    = u->blackAntCount;
    gs->whiteKingMoved   = u->whiteKingMoved;
    gs->blackKingMoved   = u->blackKingMoved;
    gs->whiteRookMovedQS = u->whiteRookMovedQS;
    gs->whiteRookMovedKS = u->whiteRookMovedKS;
    gs->blackRookMovedQS = u->blackRookMovedQS;
    gs->blackRookMovedKS = u->blackRookMovedKS;
    gs->enPassantCol     = u->enPassantCol;
    gs->enPassantRow     = u->enPassantRow;
    gs->halfMove_count   = u->halfMove_count;
    gs->fullMove_count   = u->fullMove_count;
    setPromotionCount(u->promotionCount);
}

void applyMove(struct gameState* gs, Move m, struct MoveUndo* u)
{
    if (u) saveUndo(gs, u);

    int fr = getFromRow(m), fc = getFromCol(m);
    int tr = getToRow(m),   tc = getToCol(m);
    int flags = getFlags(m);

    struct piece* moving   = gs->board[fr][fc];
    struct piece* captured = gs->board[tr][tc];
    enum pieceColor color  = moving->color;
    bool isCapture  = (captured != NULL);
    bool isPawnMove = (moving->piece == ANT);

    gs->enPassantCol = -1;
    gs->enPassantRow = -1;

    switch (flags) {
        case MOVE_NORMAL:
            decrementAntCount(gs, captured);
            gs->board[tr][tc] = moving;
            gs->board[fr][fc] = NULL;
            break;
        case MOVE_EN_PASSANT:
            decrementAntCount(gs, gs->board[fr][tc]);
            gs->board[tr][tc] = moving;
            gs->board[fr][fc] = NULL;
            gs->board[fr][tc] = NULL;
            isCapture = true;
            break;
        case MOVE_CASTLE_KS: {
            int row = fr;
            gs->board[row][7] = moving;
            gs->board[row][5] = NULL;
            gs->board[row][6] = gs->board[row][9];
            gs->board[row][9] = NULL;
            break;
        }
        case MOVE_CASTLE_QS: {
            int row = fr;
            gs->board[row][3] = moving;
            gs->board[row][5] = NULL;
            gs->board[row][4] = gs->board[row][0];
            gs->board[row][0] = NULL;
            break;
        }
        case MOVE_PROMO_QUEEN:
        case MOVE_PROMO_ROOK:
        case MOVE_PROMO_BISHOP:
        case MOVE_PROMO_KNIGHT:
        case MOVE_PROMO_ANTEATER: {
            static const enum pieceType tbl[10] = {
                [MOVE_PROMO_QUEEN]    = QUEEN,
                [MOVE_PROMO_ROOK]     = ROOK,
                [MOVE_PROMO_BISHOP]   = BISHOP,
                [MOVE_PROMO_KNIGHT]   = KNIGHT,
                [MOVE_PROMO_ANTEATER] = ANTEATER
            };
            decrementAntCount(gs, captured);
            if (color == WHITE) gs->whiteAntCount--;
            else                gs->blackAntCount--;
            gs->board[tr][tc] = allocatePromotion(tbl[flags], color);
            gs->board[fr][fc] = NULL;
            isPawnMove = true;
            break;
        }
        case MOVE_ANTEATING: {
            int eatRow = getEatRow(m);
            int eatCol = getEatCol(m);
            struct location path[80];
            int pathCount = 0;

            if (buildAnteaterPath(gs->board, fr, fc, eatRow, eatCol, tr, tc, color, path, &pathCount))
            {
                for (int i = 0; i < pathCount; i++)
                {
                    int row = path[i].row;
                    int col = path[i].col;
                    decrementAntCount(gs, gs->board[row][col]);
                    gs->board[row][col] = NULL;
                }
            }

            gs->board[tr][tc] = moving;
            gs->board[fr][fc] = NULL;
            isCapture = true;
            break;
}
    }

    if (moving->piece == KING) {
        if (color == WHITE) gs->whiteKingMoved = true;
        else                gs->blackKingMoved = true;
    }
    if (moving->piece == ROOK) {
        if (color == WHITE) {
            if (fc == 0) gs->whiteRookMovedQS = true;
            if (fc == 9) gs->whiteRookMovedKS = true;
        } else {
            if (fc == 0) gs->blackRookMovedQS = true;
            if (fc == 9) gs->blackRookMovedKS = true;
        }
    }

    if (isPawnMove && abs(tr - fr) == 2) {
        gs->enPassantCol = fc;
        gs->enPassantRow = tr;
    }

    gs->halfMove_count = (isPawnMove || isCapture) ? 0 : gs->halfMove_count + 1;
    if (color == BLACK)
        gs->fullMove_count++;
    gs->currentPlayer  = (color == WHITE) ? BLACK : WHITE;
}

bool isLegalMove(struct move moveMade, const struct gameState* gs)
{
    int fr = moveMade.pos1.rank - 1;
    int fc = moveMade.pos1.file - 'a';
    int tr = moveMade.pos2.rank - 1;
    int tc = moveMade.pos2.file - 'a';

    Move moves[MAX_MOVES];
    int count = 0;
    getMoves((struct gameState*)gs, moves, &count);

    for (int i = 0; i < count; i++) {
        if (getFromRow(moves[i]) == fr && getFromCol(moves[i]) == fc &&
            getToRow  (moves[i]) == tr && getToCol  (moves[i]) == tc)
            return true;
    }
    return false;
}

