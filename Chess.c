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

    bool isKingInCheck(struct piece* board[8][10]) {
        // Placeholder for check detection logic
        return false;
    }

    bool isCheckmate(struct piece* board[8][10]) {
        // Placeholder for checkmate detection logic
        return false;
    }

    bool isStalemate(struct piece* board[8][10]) {
        // Placeholder for stalemate detection logic
        return false;
    }

    bool isDraw(struct piece* board[8][10]) {
        // Placeholder for draw detection logic
        return false;
    }

    int moveNumber = 1;


int main() {

    //Game control flag
    bool gameContinue = true;

    struct piece* board [8][10];



    initializeBoard(board);
    printBoard(board);

    //Main Game Loop
    while (gameContinue) {

        //Check for game status (check, checkmate, continue)
        gameContinue = false;

    }




    return 0;
}

