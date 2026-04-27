/*
 * Chess_core.c
 *
 * This is Chess.c with the terminal main() wrapped in #ifndef GUI_BUILD.
 * When compiling with -DGUI_BUILD, only the game logic functions are compiled
 * (initializeBoard, initGameState, findKing, isSquareAttackedBy,
 *  wouldLeaveKingInCheck, isCastlingValid, allocatePromotion, promptPromotion,
 *  isKingInCheck, isCheckmate, isStalemate, hasInsufficientMaterial).
 *
 * The GUI entry point is in chess_gui.c.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * HOW TO USE:
 *   Replace Chess.c with this file in your build, OR rename this and update
 *   the Makefile to use Chess_core.c instead of Chess.c for the GUI target.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "types.h"
#include "Engine.h"
#include "Moves.h"
#include "Ant.h"

#ifndef GUI_BUILD
#include "terminalTestingFunctions.h"
#endif

bool isKingInCheck(struct piece* board[8][10], enum pieceColor color);

int moveNumber = 1;
int timedMoves = 0;
double totalTime = 0.0;

void printStats(double t, int tm, int tn) {
    if (tm == 0) return;
    printf("AverageTime: %.3f , TotalMoves: %d , AverageNodes: %d\n",
           t / tm, tm, tn / tm);
}

/* Promotion pool */
static struct piece promotionPool[40];
static int promotionCount = 0;

int getPromotionCount(void) {
    return promotionCount;
}

void setPromotionCount(int count) {
    promotionCount = count;
}

void initializeBoard(struct piece* board[8][10]) {
    static struct piece whiteKing     = {.color = WHITE, .piece = KING};
    static struct piece whiteQueen    = {.color = WHITE, .piece = QUEEN};
    static struct piece whiteKnight   = {.color = WHITE, .piece = KNIGHT};
    static struct piece whiteBishop   = {.color = WHITE, .piece = BISHOP};
    static struct piece whiteRook     = {.color = WHITE, .piece = ROOK};
    static struct piece whiteAnt      = {.color = WHITE, .piece = ANT};
    static struct piece whiteAnteater = {.color = WHITE, .piece = ANTEATER};

    static struct piece blackKing     = {.color = BLACK, .piece = KING};
    static struct piece blackQueen    = {.color = BLACK, .piece = QUEEN};
    static struct piece blackKnight   = {.color = BLACK, .piece = KNIGHT};
    static struct piece blackBishop   = {.color = BLACK, .piece = BISHOP};
    static struct piece blackRook     = {.color = BLACK, .piece = ROOK};
    static struct piece blackAnt      = {.color = BLACK, .piece = ANT};
    static struct piece blackAnteater = {.color = BLACK, .piece = ANTEATER};

    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 10; j++)
            board[i][j] = NULL;

    board[0][0] = &whiteRook;     board[0][1] = &whiteKnight;
    board[0][2] = &whiteBishop;   board[0][3] = &whiteAnteater;
    board[0][4] = &whiteQueen;    board[0][5] = &whiteKing;
    board[0][6] = &whiteAnteater; board[0][7] = &whiteBishop;
    board[0][8] = &whiteKnight;   board[0][9] = &whiteRook;

    board[7][0] = &blackRook;     board[7][1] = &blackKnight;
    board[7][2] = &blackBishop;   board[7][3] = &blackAnteater;
    board[7][4] = &blackQueen;    board[7][5] = &blackKing;
    board[7][6] = &blackAnteater; board[7][7] = &blackBishop;
    board[7][8] = &blackKnight;   board[7][9] = &blackRook;

    for (int col = 0; col < 10; col++) {
        board[1][col] = &whiteAnt;
        board[6][col] = &blackAnt;
    }
}

void initGameState(struct gameState* state) {
    setPromotionCount(0);
    moveNumber            = 1;
    timedMoves            = 0;
    totalTime             = 0.0;
    nodeCount             = 0;
    state->whiteAntCount    = 10;
    state->blackAntCount    = 10;
    state->whiteKingMoved   = false;
    state->blackKingMoved   = false;
    state->whiteRookMovedQS = false;
    state->whiteRookMovedKS = false;
    state->blackRookMovedQS = false;
    state->blackRookMovedKS = false;
    state->enPassantCol     = -1;
    state->enPassantRow     = -1;
    state->currentPlayer    = WHITE;
    state->halfMove_count   = 0;
    state->fullMove_count   = 1;
    memset(&state->move_log, 0, sizeof(state->move_log));
}

bool findKing(struct piece* board[8][10],
              enum pieceColor color,
              int* kingRow, int* kingCol) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 10; c++)
            if (board[r][c] != NULL &&
                board[r][c]->piece == KING &&
                board[r][c]->color == color) {
                *kingRow = r; *kingCol = c;
                return true;
            }
    return false;
}

bool isSquareAttackedBy(struct piece* board[8][10],
                        int row, int col,
                        enum pieceColor attackerColor) {
    int rookDirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (int d = 0; d < 4; d++) {
        int r = row + rookDirs[d][0], c = col + rookDirs[d][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 10) {
            if (board[r][c] != NULL) {
                if (board[r][c]->color == attackerColor &&
                    (board[r][c]->piece == ROOK || board[r][c]->piece == QUEEN))
                    return true;
                break;
            }
            r += rookDirs[d][0]; c += rookDirs[d][1];
        }
    }
    int bishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (int d = 0; d < 4; d++) {
        int r = row + bishopDirs[d][0], c = col + bishopDirs[d][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 10) {
            if (board[r][c] != NULL) {
                if (board[r][c]->color == attackerColor &&
                    (board[r][c]->piece == BISHOP || board[r][c]->piece == QUEEN))
                    return true;
                break;
            }
            r += bishopDirs[d][0]; c += bishopDirs[d][1];
        }
    }
    int knightMoves[8][2] = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
    for (int i = 0; i < 8; i++) {
        int r = row + knightMoves[i][0], c = col + knightMoves[i][1];
        if (r >= 0 && r < 8 && c >= 0 && c < 10 &&
            board[r][c] != NULL &&
            board[r][c]->color == attackerColor &&
            board[r][c]->piece == KNIGHT) return true;
    }
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int r = row + dr, c = col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 10 &&
                board[r][c] != NULL &&
                board[r][c]->color == attackerColor &&
                board[r][c]->piece == KING) return true;
        }
    int antDir = (attackerColor == WHITE) ? -1 : 1;
    int antRow = row + antDir;
    if (antRow >= 0 && antRow < 8)
        for (int dc = -1; dc <= 1; dc += 2) {
            int c = col + dc;
            if (c >= 0 && c < 10 &&
                board[antRow][c] != NULL &&
                board[antRow][c]->color == attackerColor &&
                board[antRow][c]->piece == ANT) return true;
        }
    return false;
}

bool wouldLeaveKingInCheck(struct piece* board[8][10],
                           uint32_t move,
                           enum pieceColor color) {
    int fromRow = getFromRow(move);
    int fromCol = getFromCol(move);
    int toRow = getToRow(move);
    int toCol = getToCol(move);
    int flags = getFlags(move);
    bool isEnPassant = (flags == MOVE_EN_PASSANT);
    bool isAnteating = (flags == MOVE_ANTEATING);
    bool isCastleKS = (flags == MOVE_CASTLE_KS);
    bool isCastleQS = (flags == MOVE_CASTLE_QS);
    struct piece* moving   = board[fromRow][fromCol];
    struct piece* captured = board[toRow][toCol];
    struct piece* epPawn   = NULL;
    struct piece* rook     = NULL;
    int rookFromCol = -1, rookToCol = -1;
    struct location removed[10];
    struct piece* removedPieces[10];
    int removedCount = 0;

    if (isEnPassant) { epPawn = board[fromRow][toCol]; board[fromRow][toCol] = NULL; }
    if (isAnteating) {
        int eatRow = getEatRow(move);
        int eatCol = getEatCol(move);
        struct location path[80];
        int pathCount = 0;

        if (!buildAnteaterPath(board, fromRow, fromCol, eatRow, eatCol, toRow, toCol, color, path, &pathCount))
            return true;

        for (int i = 0; i < pathCount; i++) {
            int row = path[i].row;
            int col = path[i].col;
            removed[removedCount] = (struct location){row, col};
            removedPieces[removedCount] = board[row][col];
            board[row][col] = NULL;
            removedCount++;
        }
        captured = NULL;
    }
    if (isCastleKS || isCastleQS) {
        rookFromCol = isCastleKS ? 9 : 0;
        rookToCol = isCastleKS ? 6 : 4;
        rook = board[fromRow][rookFromCol];
        board[fromRow][rookFromCol] = NULL;
        board[fromRow][rookToCol] = rook;
    }
    board[toRow][toCol]     = moving;
    board[fromRow][fromCol] = NULL;

    bool inCheck = isKingInCheck(board, color);

    board[fromRow][fromCol] = moving;
    board[toRow][toCol]     = captured;
    if (isCastleKS || isCastleQS) {
        board[fromRow][rookToCol] = NULL;
        board[fromRow][rookFromCol] = rook;
    }
    if (isEnPassant) board[fromRow][toCol] = epPawn;
    for (int i = 0; i < removedCount; i++)
        board[removed[i].row][removed[i].col] = removedPieces[i];
    return inCheck;
}

bool isCastlingValid(struct piece* board[8][10], enum pieceColor color,
                     bool kingSide, struct gameState* state) {
    int row       = (color == WHITE) ? 0 : 7;
    int kingCol   = 5;
    bool kingMoved = (color == WHITE) ? state->whiteKingMoved : state->blackKingMoved;
    bool rookMoved = kingSide
        ? ((color == WHITE) ? state->whiteRookMovedKS : state->blackRookMovedKS)
        : ((color == WHITE) ? state->whiteRookMovedQS : state->blackRookMovedQS);

    if (kingMoved || rookMoved) return false;

    int rookCol = kingSide ? 9 : 0;
    if (!board[row][rookCol] || board[row][rookCol]->piece != ROOK ||
        board[row][rookCol]->color != color) return false;

    int lo = (rookCol < kingCol) ? rookCol + 1 : kingCol + 1;
    int hi = (rookCol < kingCol) ? kingCol - 1 : rookCol - 1;
    for (int c = lo; c <= hi; c++)
        if (board[row][c]) return false;

    enum pieceColor opp  = (color == WHITE) ? BLACK : WHITE;
    int destCol = kingSide ? 7 : 3;
    int step = (destCol > kingCol) ? 1 : -1;
    struct piece* kPiece = board[row][kingCol];
    int transitCol = kingCol + step;

    board[row][kingCol] = NULL;
    board[row][transitCol] = kPiece;
    bool transitAttacked = isSquareAttackedBy(board, row, transitCol, opp);
    board[row][transitCol] = NULL;
    board[row][kingCol] = kPiece;
    if (transitAttacked) return false;

    int rookToCol = kingSide ? 6 : 4;
    struct piece* rook = board[row][rookCol];

    board[row][kingCol] = NULL;
    board[row][rookCol] = NULL;
    board[row][destCol] = kPiece;
    board[row][rookToCol] = rook;
    bool destAttacked = isSquareAttackedBy(board, row, destCol, opp);
    board[row][destCol] = NULL;
    board[row][rookToCol] = NULL;
    board[row][kingCol] = kPiece;
    board[row][rookCol] = rook;
    if (destAttacked) return false;

    return true;
}

struct piece* allocatePromotion(enum pieceType type, enum pieceColor color) {
    if (promotionCount >= 40) return NULL;
    promotionPool[promotionCount] = (struct piece){.piece = type, .color = color};
    return &promotionPool[promotionCount++];
}

enum pieceType promptPromotion(void) {
#ifdef GUI_BUILD
    return QUEEN; /* GUI handles this via dialog */
#else
    char input[8];
    printf("Promote to (Q=Queen, R=Rook, B=Bishop, N=Knight, A=Anteater): ");
    fflush(stdout);
    while (1) {
        if (fgets(input, sizeof(input), stdin) == NULL) return QUEEN;
        switch (toupper((unsigned char)input[0])) {
            case 'Q': return QUEEN;
            case 'R': return ROOK;
            case 'B': return BISHOP;
            case 'N': return KNIGHT;
            case 'A': return ANTEATER;
        }
        printf("Invalid. Enter Q, R, B, N, or A: ");
        fflush(stdout);
    }
#endif
}

bool isKingInCheck(struct piece* board[8][10], enum pieceColor color) {
    int kingRow, kingCol;
    if (!findKing(board, color, &kingRow, &kingCol)) return false;
    enum pieceColor opp = (color == WHITE) ? BLACK : WHITE;
    return isSquareAttackedBy(board, kingRow, kingCol, opp);
}

bool isColorInCheck(const struct gameState* gs, enum pieceColor color) {
    return isKingInCheck((struct piece*(*)[10])gs->board, color);
}

bool isCheckmate(struct gameState* gs) {
    if (!inCheck(gs)) return false;
    Move moves[MAX_MOVES]; int count = 0;
    getMoves(gs, moves, &count);
    return count == 0;
}

bool isStalemate(struct gameState* gs) {
    if (inCheck(gs)) return false;
    Move moves[MAX_MOVES]; int count = 0;
    getMoves(gs, moves, &count);
    return count == 0;
}

static bool hasInsufficientMaterial(struct piece* board[8][10]) {
    int whiteMajor = 0, blackMajor = 0;
    int whiteMinor = 0, blackMinor = 0;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 10; c++) {
            struct piece* p = board[r][c];
            if (p == NULL || p->piece == KING) continue;
            int* major = (p->color == WHITE) ? &whiteMajor : &blackMajor;
            int* minor = (p->color == WHITE) ? &whiteMinor : &blackMinor;
            switch (p->piece) {
                case QUEEN: case ROOK: case ANT: (*major)++; break;
                case BISHOP: case KNIGHT: case ANTEATER: (*minor)++; break;
                default: break;
            }
        }
    if (whiteMajor > 0 || blackMajor > 0) return false;
    if (whiteMinor >= 2 || blackMinor >= 2) return false;
    return true;
}

/* ───────────────────────────────────────────────────────────────────────────
   Terminal main — compiled only when NOT building the GUI
   ─────────────────────────────────────────────────────────────────────────── */
#ifndef 0

int main(void) {
    srand(time(NULL));
    struct gameState state;
    initGameState(&state);
    resetRepetitionTracking();
    clearTT();
    initializeBoard(state.board);
    storePositionHash(&state);

    enum gameMode mode = promptGameMode();
    enum pieceColor humanColor = WHITE;
    int aiDifficulty = 1;
    if (mode == HUMAN_VS_AI) {
        humanColor   = promptColorChoice();
        aiDifficulty = promptDifficulty();
    } else if (mode == AI_VS_AI) {
        aiDifficulty = promptDifficulty();
    }

    printBoard(state.board);

    while (1) {
        const char* colorName = (state.currentPlayer == WHITE) ? "White" : "Black";
        bool isHuman;
        if      (mode == HUMAN_VS_HUMAN) isHuman = true;
        else if (mode == AI_VS_AI)       isHuman = false;
        else                             isHuman = (state.currentPlayer == humanColor);

        if (isHuman) {
            printf("\n%s's turn.\n", colorName);
            Move chosen = getHumanMove(&state);
            if (!chosen) { printf("\nNo input — game ended.\n"); break; }
            applyMove(&state, chosen, NULL);
            storePositionHash(&state);
        } else {
            printf("\n%s's turn (AI). Thinking...\n", colorName);
            clock_t start = clock();
            movePiece_Computer(&state, aiDifficulty);
            clock_t end = clock();
            totalTime += (double)(end - start) / CLOCKS_PER_SEC;
            timedMoves++;
            printf("MoveTime: %.3f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
        }

        if (state.currentPlayer == BLACK) moveNumber++;
        printBoard(state.board);

        if (isCheckmate(&state)) {
            const char* winner = (state.currentPlayer == WHITE) ? "Black" : "White";
            printf("\nCheckmate! %s wins.\n", winner);
            printStats(totalTime, timedMoves, nodeCount);
            break;
        }
        if (isStalemate(&state)) { printf("\nStalemate! Draw.\n"); printStats(totalTime, timedMoves, nodeCount); break; }
        if (state.halfMove_count >= 100) { printf("\nDraw by fifty-move rule.\n"); printStats(totalTime, timedMoves, nodeCount); break; }
        if (isThreeFoldDraw(HASHES[currentPly-1], currentPly)) { printf("\nDraw by threefold repetition.\n"); printStats(totalTime, timedMoves, nodeCount); break; }
        if (hasInsufficientMaterial(state.board)) { printf("\nDraw by insufficient material.\n"); printStats(totalTime, timedMoves, nodeCount); break; }
    }
    return 0;
}

#endif /* GUI_BUILD */
