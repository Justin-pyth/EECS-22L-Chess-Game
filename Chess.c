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

    void initalizeBoard (struct piece* board) {
        struct piece whiteKing;

        whiteKing.color = BLACK;
        whiteKing.piece = KING;

        
    }

int main() {

    //Board Structure
    

    //Game control flag
    bool gameContinue = true;

    struct piece* board [8][10];

    

    initializeBoard(board);

    //Main Game Loop
    while (gameContinue) {

        //Check for game status (check, checkmate, continue)

    }




    return 0;
}