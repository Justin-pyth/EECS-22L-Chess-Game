#ifndef MOVES_H
#define MOVES_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

//TEMPORARY DEFINITIONS
//=======================================================
typedef char* string;

struct location { // use to log moves
    int rank; // 1-10
    int file; // 1-8
};
struct move {
    string pos1; // "a4" 
    string pos2; // "a5"
};
struct log {
    string move; // "a1 - j10"
    int piece; // Black or White
    int moveNumber; // 1-1000 for # of moves in the game
    int history[1000];
};

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
        enum PieceType piece_;
        enum PieceColor color;
    };

struct gameState {
    struct piece* board[8][10];
    enum PieceColor currentPlayer;
    struct location enPassantTile;
    
    //sides that castling is allowed (CastlingRights)
    bool whiteKing_side;
    bool whiteQueen_side;
    bool blackKing_side;
    bool blackQueen_side;

    //trackers for move log + 50 turn draw rule
    int halfMove_count;
    int fullMove_count;
    struct log move_log;
};
//=======================================================

struct move* getMoves(int* moveCount);

struct move* getLegalMoves(struct piece p, int* moveCount);

bool isLegalMove(struct move, const struct gameState* gs);

#endif