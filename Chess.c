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

    struct piece BlackKing;

    BlackKing.color = Black;
    BlackKing.piece = King;

    board [0][0] = King;

    return 0;
}