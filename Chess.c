#include <stdio.h>
#include <stdbool.h>

    enum pieceType {
        KING,
        QUEEN,
        KNIGHT,
        BISHOP,
        ROOK,
        ANT,
        ANTEATER
    };

    enum pieceColor {
       BLACK,
       WHITE
    };

    struct piece {
        enum pieceType piece;
        enum pieceColor color;
    };

    char pieceToChar(const struct piece* p) {
        if (p == NULL) return '.';

        char base = '?';
        switch (p->piece) {
            case KING: base = 'K'; break;
            case QUEEN: base = 'Q'; break;
            case KNIGHT: base = 'N'; break;
            case BISHOP: base = 'B'; break;
            case ROOK: base = 'R'; break;
            case ANT: base = 'P'; break;
            case ANTEATER: base = 'A'; break;
        }

        if (p->color == BLACK && base >= 'A' && base <= 'Z') {
            base = (char)(base + ('a' - 'A'));
        }

        return base;
    }

    void printBoard(struct piece* board[8][10]) {
        printf("\n    a b c d e f g h i j\n");
        printf("   ---------------------\n");

        for (int row = 7; row >= 0; row--) {
            printf("%d | ", row + 1);
            for (int col = 0; col < 10; col++) {
                printf("%c", pieceToChar(board[row][col]));
                if (col < 9) printf(" ");
            }
            printf(" |\n");
        }

        printf("   ---------------------\n");
    }

    void initializeBoard(struct piece* board[8][10]) {

        //Initialize pieces
        static struct piece whiteKing = {.color = WHITE, .piece = KING};
        static struct piece whiteQueen = {.color = WHITE, .piece = QUEEN};
        static struct piece whiteKnight = {.color = WHITE, .piece = KNIGHT};
        static struct piece whiteBishop = {.color = WHITE, .piece = BISHOP};
        static struct piece whiteRook = {.color = WHITE, .piece = ROOK};
        static struct piece whiteAnt = {.color = WHITE, .piece = ANT};
        static struct piece whiteAnteater = {.color = WHITE, .piece = ANTEATER};

        static struct piece blackKing = {.color = BLACK, .piece = KING};
        static struct piece blackQueen = {.color = BLACK, .piece = QUEEN};
        static struct piece blackKnight = {.color = BLACK, .piece = KNIGHT};
        static struct piece blackBishop = {.color = BLACK, .piece = BISHOP};
        static struct piece blackRook = {.color = BLACK, .piece = ROOK};
        static struct piece blackAnt = {.color = BLACK, .piece = ANT};
        static struct piece blackAnteater = {.color = BLACK, .piece = ANTEATER};

        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 10; j++) {
                board[i][j] = NULL;
            }
        }

        board[0][0] = &whiteRook;
        board[0][1] = &whiteKnight;
        board[0][2] = &whiteBishop;
        board[0][3] = &whiteAnteater;
        board[0][4] = &whiteQueen;
        board[0][5] = &whiteKing;
        board[0][6] = &whiteAnteater;
        board[0][7] = &whiteBishop;
        board[0][8] = &whiteKnight;
        board[0][9] = &whiteRook;

        board[7][0] = &blackRook;
        board[7][1] = &blackKnight;
        board[7][2] = &blackBishop;
        board[7][3] = &blackAnteater;
        board[7][4] = &blackQueen;
        board[7][5] = &blackKing;
        board[7][6] = &blackAnteater;
        board[7][7] = &blackBishop;
        board[7][8] = &blackKnight;
        board[7][9] = &blackRook;

        for (int col = 0; col < 10; col++) {
            board[1][col] = &whiteAnt;
            board[6][col] = &blackAnt;
        }
    }


    /*
     *
     *  int countLegalMoves(board, color)
     *      - Generates all legal moves for 'color' on 'board'
     *      - A move is legal only if it does not leave that
     *        color's own king in check after the move
     *      - Returns the count (0 means no legal moves exist)
     *
     *  bool makeMove(board, fromRow, fromCol, toRow, toCol)
     *      - Applies a validated move to the board in-place
     *      - Handles special moves: castling, en passant,
     *        pawn promotion, ant eating
     *      - Returns true on success, false if the move is
     *        illegal (caller should re-prompt)
     *
     *  void getHumanMove(board, color, fromRow, fromCol,
     *                    toRow, toCol)
     *      - Prompts the human player for input (e.g. "E2 E4")
     *      - Parses algebraic notation into row/col indices
     *      - Owned by the UI team member
     *
     *  void getComputerMove(board, color, fromRow, fromCol,
     *                       toRow, toCol)
     *      - Selects the best move for the computer player
     *      - Owned by the AI/strategy team member
     *
     *  void logMove(logFile, moveNumber, color,
     *               fromRow, fromCol, toRow, toCol)
     *      - Appends a human-readable move record to the log
     *      - Owned by the log-file team member
     * =========================================================
     */

    /*
     * findKing — scan the board and return the row/col of the
     * king belonging to 'color'.  Writes results through the
     * out-parameters kingRow and kingCol.
     * Returns true if the king was found, false otherwise
     * (should never be false in a valid game state).
     */
    bool findKing(struct piece* board[8][10],
                  enum pieceColor color,
                  int* kingRow, int* kingCol) {
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 10; c++) {
                if (board[r][c] != NULL &&
                    board[r][c]->piece == KING &&
                    board[r][c]->color == color) {
                    *kingRow = r;
                    *kingCol = c;
                    return true;
                }
            }
        }
        return false;
    }

    /*
     * isSquareAttackedBy — returns true if the square at
     * (row, col) is attacked by any piece belonging to
     * 'attackerColor'.
     *
     * Covers every attacking piece type:
     *   Rook / Queen  — horizontal and vertical rays
     *   Bishop / Queen — diagonal rays
     *   Knight         — eight L-shaped jumps
     *   King           — one step in any direction
     *   Ant (pawn)     — diagonal forward captures only
     *   Anteater       — per spec, NOT a threat to the king
     */
    bool isSquareAttackedBy(struct piece* board[8][10],
                            int row, int col,
                            enum pieceColor attackerColor) {

        //Rook and Queen: horizontal / vertical rays
        int rookDirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
        for (int d = 0; d < 4; d++) {
            int r = row + rookDirs[d][0];
            int c = col + rookDirs[d][1];
            while (r >= 0 && r < 8 && c >= 0 && c < 10) {
                if (board[r][c] != NULL) {
                    if (board[r][c]->color == attackerColor &&
                        (board[r][c]->piece == ROOK ||
                         board[r][c]->piece == QUEEN)) {
                        return true;
                    }
                    break; /* any piece blocks the ray */
                }
                r += rookDirs[d][0];
                c += rookDirs[d][1];
            }
        }

        //Bishop and Queen: diagonal rays
        int bishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
        for (int d = 0; d < 4; d++) {
            int r = row + bishopDirs[d][0];
            int c = col + bishopDirs[d][1];
            while (r >= 0 && r < 8 && c >= 0 && c < 10) {
                if (board[r][c] != NULL) {
                    if (board[r][c]->color == attackerColor &&
                        (board[r][c]->piece == BISHOP ||
                         board[r][c]->piece == QUEEN)) {
                        return true;
                    }
                    break;
                }
                r += bishopDirs[d][0];
                c += bishopDirs[d][1];
            }
        }

        //Knight: eight L-shaped jumps
        int knightMoves[8][2] = {
            {2,1},{2,-1},{-2,1},{-2,-1},
            {1,2},{1,-2},{-1,2},{-1,-2}
        };
        for (int i = 0; i < 8; i++) {
            int r = row + knightMoves[i][0];
            int c = col + knightMoves[i][1];
            if (r >= 0 && r < 8 && c >= 0 && c < 10 &&
                board[r][c] != NULL &&
                board[r][c]->color == attackerColor &&
                board[r][c]->piece == KNIGHT) {
                return true;
            }
        }

        //King: one step in any direction
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0) continue;
                int r = row + dr;
                int c = col + dc;
                if (r >= 0 && r < 8 && c >= 0 && c < 10 &&
                    board[r][c] != NULL &&
                    board[r][c]->color == attackerColor &&
                    board[r][c]->piece == KING) {
                    return true;
                }
            }
        }

        /* --- Ant (pawn): captures diagonally forward ---
         * White ants move up (increasing row), so they attack
         * from below; a white ant at (row-1, col±1) attacks (row,col).
         * Black ants move down, so a black ant at (row+1, col±1)
         * attacks (row,col). */
        int antDirection = (attackerColor == WHITE) ? -1 : 1;
        int antRow = row + antDirection;
        if (antRow >= 0 && antRow < 8) {
            for (int dc = -1; dc <= 1; dc += 2) {
                int c = col + dc;
                if (c >= 0 && c < 10 &&
                    board[antRow][c] != NULL &&
                    board[antRow][c]->color == attackerColor &&
                    board[antRow][c]->piece == ANT) {
                    return true;
                }
            }
        }

        /* Anteater: per spec "it is no threat to the opposing king" */

        return false;
    }

    /*
     * isKingInCheck — returns true if the king belonging to
     * 'color' is currently attacked by any opponent piece.
     */
    bool isKingInCheck(struct piece* board[8][10],
                       enum pieceColor color) {
        int kingRow, kingCol;
        if (!findKing(board, color, &kingRow, &kingCol)) {
            return false; /* king not found — treat as not in check */
        }

        enum pieceColor opponent = (color == WHITE) ? BLACK : WHITE;
        return isSquareAttackedBy(board, kingRow, kingCol, opponent);
    }

    /*
     * isCheckmate — returns true when:
     *   1. The king of 'color' is currently in check, AND
     *   2. There are no legal moves that would get it out.
     *
     * NOTE: countLegalMoves() is owned by the move-generation
     * team member (see stub comment above).  Replace the
     * pseudo-call below with the real function once available.
     */
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

    /*
     * isStalemate — returns true when:
     *   1. The king of 'color' is NOT in check, AND
     *   2. There are still no legal moves available.
     *
     * NOTE: countLegalMoves() is owned by the move-generation
     * team member (see stub comment above).
     */
    bool isStalemate(struct piece* board[8][10],
                     enum pieceColor color) {
        if (isKingInCheck(board, color)) {
            return false;
        }

        /* PSEUDO: int moves = countLegalMoves(board, color); */
        /* PSEUDO: return (moves == 0);                       */

        return false;
    }

    bool isDraw(struct piece* board[8][10]) {
        /* Placeholder — draw conditions (50-move rule,
         * threefold repetition, etc.) to be added later.    */
        return false;
    }

    int moveNumber = 1;

int main() {

    struct piece* board[8][10];

    //Setup
    initializeBoard(board);

    /* PSEUDO: enum pieceColor humanColor = promptPlayerColor();
     *         Asks "Play as white or black?" and returns the choice. */
    enum pieceColor humanColor  = WHITE; /* default until UI is ready */
    enum pieceColor computerColor = (humanColor == WHITE) ? BLACK : WHITE;

    //logFile = openLogFile("chess_game.log");
    //Creates / opens the move log file.             

    printBoard(board);

    //Main Game Loop
    enum pieceColor currentTurn = WHITE;
    bool gameContinue = true;

    while (gameContinue) {

        //Determine whose turn it is
        if (currentTurn == humanColor) {

            //GET MOVE HERE
            printf("\nYour turn (%s). Enter move (e.g. E2 E4): ",
                   (humanColor == WHITE) ? "White" : "Black");

            /* PSEUDO: int fromRow, fromCol, toRow, toCol;
             *         getHumanMove(board, humanColor,
             *                      &fromRow, &fromCol,
             *                      &toRow, &toCol);
             *         Reads and validates input from stdin.  */

            /* PSEUDO: bool ok = makeMove(board, fromRow, fromCol,
             *                            toRow, toCol);
             *         if (!ok) { printf("Illegal move.\n"); continue; } */

            /* PSEUDO: logMove(logFile, moveNumber, humanColor,
             *                 fromRow, fromCol, toRow, toCol); */


                gameContinue = false;

        } else {
            printf("\nComputer (%s) is thinking...\n",
                   (computerColor == WHITE) ? "White" : "Black");

            /* PSEUDO: int fromRow, fromCol, toRow, toCol;
             *         getComputerMove(board, computerColor,
             *                         &fromRow, &fromCol,
             *                         &toRow, &toCol);
             *         AI selects and returns the best move.  */

            //makeMove(board, fromRow, fromCol,toRow, toCol);               

            //logMove(logFile, moveNumber, computerColor,fromRow, fromCol, toRow, toCol); 
        }

        printBoard(board);

        //Check game-ending conditions after every move
        enum pieceColor justMoved = currentTurn;
        enum pieceColor nextTurn  = (currentTurn == WHITE) ? BLACK : WHITE;

        if (isCheckmate(board, nextTurn)) {
            printf("\nCheckmate! %s wins!\n",
                   (justMoved == WHITE) ? "White" : "Black");
            gameContinue = false;

        } else if (isStalemate(board, nextTurn)) {
            printf("\nStalemate! The game is a draw.\n");
            gameContinue = false;

        } else if (isDraw(board)) {
            printf("\nDraw!\n");
            gameContinue = false;

        } else {
            //Notify the next player if their king is in check
            if (isKingInCheck(board, nextTurn)) {
                printf("\n%s is in check!\n",
                       (nextTurn == WHITE) ? "White" : "Black");
            }
            //Advance turn counter once both players have moved
            if (currentTurn == BLACK) {
                moveNumber++;
            }
            currentTurn = nextTurn;
        }
    }

    //closeLogFile(logFile);

    return 0;
}
