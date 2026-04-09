#include <stdio.h>

enum PieceType {
        King,
        Queen,
        Knight,
        Bishop,
        Rook,
        Ant,
        Anteater
    };

    enum PieceColor {
       Black,
       White 
    };

    struct piece {
        enum PieceType piece;
        enum PieceColor color;
    };

int main() {

    struct piece* board [8][10];

    return 0;
}