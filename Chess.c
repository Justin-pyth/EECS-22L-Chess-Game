#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "types.h"
#include "terminalTestingFunctions.h"
#include "Engine.h"
#include "Moves.h"


    //isValidMove is declared in terminalTestingFunctions.h               
    bool isKingInCheck(struct piece* board[8][10], enum pieceColor color);

    int moveNumber = 1;

    //Promotion piece pool — up to 40 promotions per game                 
    static struct piece promotionPool[40];
    static int promotionCount = 0;

    // Board / state initialisation                                         
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
        state->whiteKingMoved   = false;
        state->blackKingMoved   = false;
        state->whiteRookMovedQS = false;
        state->whiteRookMovedKS = false;
        state->blackRookMovedQS = false;
        state->blackRookMovedKS = false;
        state->enPassantCol     = -1;
        state->enPassantRow     = -1;
    }

    //HELPER FUNCTIONS

    //Returns pointers to the kings location; false if king is missing (should never happen in a valid game)
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

    /* ONLY VALID FOR KING
     * isSquareAttackedBy — returns true if (row, col) is attacked by
     * any piece of 'attackerColor'.
     *
     *   Rook  / Queen  — horizontal and vertical rays
     *   Bishop / Queen — diagonal rays
     *   Knight         — eight L-shaped jumps
     *   King           — one step in any direction
     *   Ant (pawn)     — diagonal forward captures only
     *   Anteater       — NOT a threat to the king (per spec)
     */
    bool isSquareAttackedBy(struct piece* board[8][10],
                            int row, int col,
                            enum pieceColor attackerColor) {
        // Rook and Queen: horizontal / vertical rays
        int rookDirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
        for (int d = 0; d < 4; d++) {
            int r = row + rookDirs[d][0], c = col + rookDirs[d][1];
            while (r >= 0 && r < 8 && c >= 0 && c < 10) {
                if (board[r][c] != NULL) {
                    if (board[r][c]->color == attackerColor &&
                        (board[r][c]->piece == ROOK ||
                         board[r][c]->piece == QUEEN)) return true;
                    break;
                }
                r += rookDirs[d][0]; c += rookDirs[d][1];
            }
        }

        // Bishop and Queen: diagonal rays
        int bishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
        for (int d = 0; d < 4; d++) {
            int r = row + bishopDirs[d][0], c = col + bishopDirs[d][1];
            while (r >= 0 && r < 8 && c >= 0 && c < 10) {
                if (board[r][c] != NULL) {
                    if (board[r][c]->color == attackerColor &&
                        (board[r][c]->piece == BISHOP ||
                         board[r][c]->piece == QUEEN)) return true;
                    break;
                }
                r += bishopDirs[d][0]; c += bishopDirs[d][1];
            }
        }

        // Knight: eight L-shaped jumps
        int knightMoves[8][2] = {
            {2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}
        };
        for (int i = 0; i < 8; i++) {
            int r = row + knightMoves[i][0], c = col + knightMoves[i][1];
            if (r >= 0 && r < 8 && c >= 0 && c < 10 &&
                board[r][c] != NULL &&
                board[r][c]->color == attackerColor &&
                board[r][c]->piece == KNIGHT) return true;
        }

        // King: one step in any direction
        for (int dr = -1; dr <= 1; dr++)
            for (int dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0) continue;
                int r = row + dr, c = col + dc;
                if (r >= 0 && r < 8 && c >= 0 && c < 10 &&
                    board[r][c] != NULL &&
                    board[r][c]->color == attackerColor &&
                    board[r][c]->piece == KING) return true;
            }

        // Ant (pawn): diagonal forward captures only
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

        // Anteater: NOT a threat to the king (per spec)
        return false;
    }

    /*
     * wouldLeaveKingInCheck — temporarily applies a move and tests
     * whether 'color's king ends up in check, then undoes it.
     *
     * isEnPassant: when true, also removes the captured ant at
     * (fromRow, toCol) — the square occupied by the ant that double-
     * advanced on the previous turn.
     */
    bool wouldLeaveKingInCheck(struct piece* board[8][10],
                               int fromRow, int fromCol,
                               int toRow,   int toCol,
                               enum pieceColor color,
                               bool isEnPassant) {
        struct piece* moving   = board[fromRow][fromCol];
        struct piece* captured = board[toRow][toCol];
        struct piece* epPawn   = NULL;

        if (isEnPassant) {
            epPawn = board[fromRow][toCol];
            board[fromRow][toCol] = NULL;
        }
        board[toRow][toCol]     = moving;
        board[fromRow][fromCol] = NULL;

        bool inCheck = isKingInCheck(board, color);

        board[fromRow][fromCol] = moving;
        board[toRow][toCol]     = captured;
        if (isEnPassant) board[fromRow][toCol] = epPawn;

        return inCheck;
    }

    /*
     * isCastlingValid — verifies every castling rule:
     *   1. King and the relevant rook have not previously moved.
     *   2. All squares between king and rook are empty.
     *   3. King is not currently in check.
     *   4. King does not pass through or land on an attacked square.
     *
     * Board layout (row 0 = white, row 7 = black):
     *   King col 5 (F), queenside rook col 0 (A), kingside rook col 9 (J)
     *   Kingside:  king F→H (5→7), rook moves J(9)→G(6)
     *   Queenside: king F→D (5→3), rook moves A(0)→E(4)
     */
    bool isCastlingValid(struct piece* board[8][10],
                         enum pieceColor color,
                         bool kingSide,
                         struct gameState* state) {
        int row     = (color == WHITE) ? 0 : 7;
        int kingCol = 5;

        // King must not have moved
        bool kingMoved = (color == WHITE) ? state->whiteKingMoved
                                           : state->blackKingMoved;
        if (kingMoved) return false;

        // Rook must not have moved; identify which rook and destination squares
        bool rookMoved;
        int rookCol, kingDestCol;

        if (kingSide) {
            rookMoved   = (color == WHITE) ? state->whiteRookMovedKS
                                           : state->blackRookMovedKS;
            rookCol     = 9;
            kingDestCol = 7;
        } else {
            rookMoved   = (color == WHITE) ? state->whiteRookMovedQS
                                           : state->blackRookMovedQS;
            rookCol     = 0;
            kingDestCol = 3;
        }
        if (rookMoved) return false;

        // Rook must still be on its original square
        if (board[row][rookCol] == NULL ||
            board[row][rookCol]->piece != ROOK ||
            board[row][rookCol]->color != color) return false;

        // Every square between king and rook must be empty
        int step = kingSide ? 1 : -1;
        for (int c = kingCol + step; c != rookCol; c += step)
            if (board[row][c] != NULL) return false;

        // King must not be in check in its current position
        if (isKingInCheck(board, color)) return false;

        // King must not pass through or land on an attacked square.
        // Simulate the king at each square from kingCol+step through kingDestCol.
        enum pieceColor opp    = (color == WHITE) ? BLACK : WHITE;
        struct piece*   kPiece = board[row][kingCol];

        for (int c = kingCol + step; c != kingDestCol + step; c += step) {
            struct piece* orig  = board[row][c];
            board[row][kingCol] = NULL;
            board[row][c]       = kPiece;

            bool attacked = isSquareAttackedBy(board, row, c, opp);

            board[row][c]       = orig;
            board[row][kingCol] = kPiece;

            if (attacked) return false;
        }

        return true;
    }

    /* ------------------------------------------------------------------ */
    /* Promotion                                                            */
    /* ------------------------------------------------------------------ */

    struct piece* allocatePromotion(enum pieceType type, enum pieceColor color) {
        if (promotionCount >= 40) return NULL;  /* should never happen */
        promotionPool[promotionCount] = (struct piece){.piece = type, .color = color};
        return &promotionPool[promotionCount++];
    }

    /*
     * promptPromotion — asks which piece to promote to.
     * Returns QUEEN on EOF (safe default).
     */
    enum pieceType promptPromotion(void) {
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
    }

    

    //Game State
    bool isKingInCheck(struct piece* board[8][10], enum pieceColor color) {
        int kingRow, kingCol;
        if (!findKing(board, color, &kingRow, &kingCol)) return false;
        enum pieceColor opp = (color == WHITE) ? BLACK : WHITE;
        return isSquareAttackedBy(board, kingRow, kingCol, opp);
    }

    bool isCheckmate(struct piece* board[8][10],
                     enum pieceColor color) {
        if (!isKingInCheck(board, color)) {
            return false;
        }

        /* PSEUDO: int moves = countLegalMoves(board, color); */
        /* PSEUDO: return (moves == 0);                       */

        /* Temporary stub — always returns false until
         * countLegalMoves() is integrated.                   */
        return false;
    }

    bool isStalemate(struct piece* board[8][10],
                     enum pieceColor color) {
        if (isKingInCheck(board, color)) {
            return false;
        }

        /* PSEUDO: int moves = countLegalMoves(board, color); */
        /* PSEUDO: return (moves == 0);                       */

        return false;
    }


    // hasInsufficientMaterial — returns true when neither side can
    // ever deliver checkmate regardless of play:
    //   - King vs King
    //   - King + Bishop vs King
    //   - King + Knight vs King
    // (In this variant the anteater is also considered minor material.)
    static bool hasInsufficientMaterial(struct piece* board[8][10]) {
        int whiteMajor = 0, whiteMate = 0;  /* queens, rooks, ants */
        int blackMajor = 0, blackMate = 0;
        int whiteMinor = 0, blackMinor = 0; /* bishops, knights, anteaters */

        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 10; c++) {
                struct piece* p = board[r][c];
                if (p == NULL || p->piece == KING) continue;
                int* major = (p->color == WHITE) ? &whiteMajor : &blackMajor;
                int* minor = (p->color == WHITE) ? &whiteMinor : &blackMinor;
                int* mate  = (p->color == WHITE) ? &whiteMate  : &blackMate;
                switch (p->piece) {
                    case QUEEN: case ROOK: case ANT: (*major)++; (*mate)++; break;
                    case BISHOP: case KNIGHT: case ANTEATER: (*minor)++; break;
                    default: break;
                }
                (void)mate;
            }

        /* Either side has a mating piece → material is sufficient */
        if (whiteMajor > 0 || blackMajor > 0) return false;
        /* King + 2 minors can force mate; King + 1 minor cannot    */
        if (whiteMinor >= 2 || blackMinor >= 2) return false;
        return true;
    }

int main(void) {

    struct piece*    board[8][10];
    struct gameState state;

    initializeBoard(board);
    initGameState(&state);

    enum gameMode mode = promptGameMode();

    enum pieceColor humanColor = WHITE;
    int aiDifficulty = 1;   
    if (mode == HUMAN_VS_AI) {
        humanColor = promptColorChoice();
        aiDifficulty = promptDifficulty();
    } else if (mode == AI_VS_AI) {
        aiDifficulty = promptDifficulty();
    }

    // logFile = openLogFile("chess_game.log");

    printBoard(board);

    enum pieceColor currentTurn = WHITE;   // White always moves first
    bool gameContinue = true;

    while (gameContinue) {
        int fromRow, fromCol, toRow, toCol;

        // Determine whether the current player is human or AI
        bool isHuman;
        if      (mode == HUMAN_VS_HUMAN) isHuman = true;
        else if (mode == AI_VS_AI)       isHuman = false;
        else                             isHuman = (currentTurn == humanColor);

        const char* colorName = (currentTurn == WHITE) ? "White" : "Black";

        if (isHuman) {
            printf("\n%s's turn.\n", colorName);

            /*
            if (!getHumanMove(board, currentTurn, &state,
                              &fromRow, &fromCol, &toRow, &toCol)) {
                printf("\nNo input — game ended.\n");
                break;
            }
             */

            //makeMove(board, fromRow, fromCol, toRow, toCol,currentTurn, &state);

            /* PSEUDO: logMove(logFile, moveNumber, currentTurn,
             *                 fromRow, fromCol, toRow, toCol);   */

        } else {
            printf("\n%s's turn (AI). Thinking...\n", colorName);
            //movePiece_Computer(board, &state, currentTurn, aiDifficulty);
        }

        enum pieceColor nextTurn = (currentTurn == WHITE) ? BLACK : WHITE;

        printBoard(board);

        enum pieceColor justMoved = currentTurn;

    
        if (currentTurn == BLACK) moveNumber++;
        currentTurn = nextTurn;
    }

    // closeLogFile(logFile);
    return 0;
}
