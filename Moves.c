#include "Moves.h"
#include <stdlib.h>
#include <string.h>

extern bool isKingInCheck(struct piece* board[8][10], enum pieceColor color);
extern bool wouldLeaveKingInCheck(struct piece* board[8][10],
                                   int fromRow, int fromCol,
                                   int toRow,   int toCol,
                                   enum pieceColor color, bool isEnPassant);
extern bool isCastlingValid(struct piece* board[8][10], enum pieceColor color,
                             bool kingSide, struct gameState* state);
extern struct piece* allocatePromotion(enum pieceType type, enum pieceColor color);
extern int getPromotionCount(void);
extern void setPromotionCount(int count);

/* ── inCheck ────────────────────────────────────────────────────────── */

bool inCheck(const struct gameState* gs)
{
    return isKingInCheck((struct piece*(*)[10])gs->board, gs->currentPlayer);
}

/* ── Shared appendMove helper ───────────────────────────────────────── */

static struct move* appendMove(struct move* arr, int* cnt, int* cap,
                                int fr, int fc, int tr, int tc)
{
    if (*cnt >= *cap) {
        *cap *= 2;
        arr = realloc(arr, (size_t)(*cap) * sizeof(struct move));
    }
    arr[*cnt].pos1.rank = (char)(fr + 1);
    arr[*cnt].pos1.file = (char)('a' + fc);
    arr[*cnt].pos2.rank = (char)(tr + 1);
    arr[*cnt].pos2.file = (char)('a' + tc);
    (*cnt)++;
    return arr;
}

/* ── Decode a struct move back to row/col integers ──────────────────── */

static void decodeMoveCoords(struct move m,
                              int* fr, int* fc, int* tr, int* tc)
{
    *fr = m.pos1.rank - 1;
    *fc = m.pos1.file - 'a';
    *tr = m.pos2.rank - 1;
    *tc = m.pos2.file - 'a';
}

/* ── Convert a legal struct move to Move and append to Move[] ───────── */

static void commitIfLegal(struct piece* board[8][10],
                           struct move m, int flags,
                           enum pieceColor color, bool isEnPassant,
                           Move* mv, int* n)
{
    int fr, fc, tr, tc;
    decodeMoveCoords(m, &fr, &fc, &tr, &tc);
    if (!wouldLeaveKingInCheck(board, fr, fc, tr, tc, color, isEnPassant))
        mv[(*n)++] = createMove(fr, fc, tr, tc, flags);
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

/*
    getAnteaterMoves
    Returns all possible moves that a given Anteater piece can make
*/
void getAnteaterMoves(struct piece* board[8][10], int row, int col, uint32_t* moves, int* moveCount)
{
    struct piece* p = board[row][col];
    *moveCount = 0;
    enum pieceColor opp = (p->color == WHITE) ? BLACK : WHITE;

    for (int dr = -1; dr <= 1; dr++) 
    {
        for (int dc = -1; dc <= 1; dc++) 
        {
            if (dr == 0 && dc == 0) continue;
            int r = row + dr, c = col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 10 && board[r][c] == NULL)
                moves[(*moveCount)++] = createMove(row, col, r, c, MOVE_NORMAL);
        }
    }
    for (int dr = -1; dr <= 1; dr++) 
    {
        for (int dc = -1; dc <= 1; dc++) 
        {
            if (dr == 0 && dc == 0) continue;
            int r = row + dr, c = col + dc;
            if (r < 0 || r >= 8 || c < 0 || c >= 10) continue;
            if (!board[r][c] || board[r][c]->piece != ANT ||
                board[r][c]->color != opp) continue;
                moves[(*moveCount)++] = createMove(row, col, r, c, MOVE_NORMAL);
            int edirs[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
            
            for (int d = 0; d < 4; d++) 
            {
                int nr = r + edirs[d][0], nc = c + edirs[d][1];
                while (nr >= 0 && nr < 8 && nc >= 0 && nc < 10 &&
                       board[nr][nc] && board[nr][nc]->piece == ANT &&
                       board[nr][nc]->color == opp) 
                {
                    moves[(*moveCount)++] = createMove(row, col, nr, nc, MOVE_NORMAL);
                    nr += edirs[d][0]; nc += edirs[d][1];
                }
            }
        }
    }
}

/*  getMoves — calls get*Moves, converts results, appends special moves */

void getMoves(struct gameState* gs, Move* moves, int* moveCount)
{
    *moveCount = 0;
    enum pieceColor color = gs->currentPlayer;
    enum pieceColor opp   = (color == WHITE) ? BLACK : WHITE;
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
                    for (int pi = 0; pi < 5; pi++) {
                        if (!wouldLeaveKingInCheck(gs->board, fr, fc, tr, tc, color, false))
                            moves[(*moveCount)++] = createMove(fr, fc, tr, tc, promoFlags[pi]);
                    }
                    continue;
                }

                // anteater eating flag
                if (p->piece == ANTEATER && flags == MOVE_ANTEATING) {
                    if (!wouldLeaveKingInCheck(gs->board, fr, fc, tr, tc, color, false))
                        moves[(*moveCount)++] = createMove(fr, fc, tr, tc, MOVE_ANTEATING);
                    continue;
                }

                // normal move legality check
                if (!wouldLeaveKingInCheck(gs->board, fr, fc, tr, tc, color, false))
                    moves[(*moveCount)++] = createMove(fr, fc, tr, tc, MOVE_NORMAL);
            }

            // en passant
            if (p->piece == ANT && gs->enPassantCol >= 0 && gs->enPassantRow == r) {
                int dc = gs->enPassantCol - c;
                if (dc == 1 || dc == -1) {
                    int epToRow = r + dir;
                    if (!wouldLeaveKingInCheck(gs->board, r, c, epToRow,
                                               gs->enPassantCol, color, true))
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

static void saveUndo(const struct gameState* gs, struct MoveUndo* u)
{
    memcpy(u->board, gs->board, sizeof(gs->board));
    u->currentPlayer    = gs->currentPlayer;
    u->whiteKingMoved   = gs->whiteKingMoved;
    u->blackKingMoved   = gs->blackKingMoved;
    u->whiteRookMovedQS = gs->whiteRookMovedQS;
    u->whiteRookMovedKS = gs->whiteRookMovedKS;
    u->blackRookMovedQS = gs->blackRookMovedQS;
    u->blackRookMovedKS = gs->blackRookMovedKS;
    u->enPassantCol     = gs->enPassantCol;
    u->enPassantRow     = gs->enPassantRow;
    u->halfMove_count   = gs->halfMove_count;
    u->promotionCount   = getPromotionCount();
}

void undoMove(struct gameState* gs, const struct MoveUndo* u)
{
    memcpy(gs->board, u->board, sizeof(gs->board));
    gs->currentPlayer    = u->currentPlayer;
    gs->whiteKingMoved   = u->whiteKingMoved;
    gs->blackKingMoved   = u->blackKingMoved;
    gs->whiteRookMovedQS = u->whiteRookMovedQS;
    gs->whiteRookMovedKS = u->whiteRookMovedKS;
    gs->blackRookMovedQS = u->blackRookMovedQS;
    gs->blackRookMovedKS = u->blackRookMovedKS;
    gs->enPassantCol     = u->enPassantCol;
    gs->enPassantRow     = u->enPassantRow;
    gs->halfMove_count   = u->halfMove_count;
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
            gs->board[tr][tc] = moving;
            gs->board[fr][fc] = NULL;
            break;
        case MOVE_EN_PASSANT:
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
            gs->board[tr][tc] = allocatePromotion(tbl[flags], color);
            gs->board[fr][fc] = NULL;
            isPawnMove = true;
            break;
        }
        case MOVE_ANTEATING: {
            gs->board[tr][tc] = moving;
            gs->board[fr][fc] = NULL;
            int dr = (tr > fr) ? 1 : (tr < fr) ? -1 : 0;
            int dc = (tc > fc) ? 1 : (tc < fc) ? -1 : 0;
            for (int row = fr + dr, col = fc + dc;
                (row != tr || col != tc) && row >= 0 && row < 8 && col >= 0 && col < 10;
                row += dr, col += dc) {
                if (gs->board[row][col] && gs->board[row][col]->piece == ANT)
                    gs->board[row][col] = NULL;
            }
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

