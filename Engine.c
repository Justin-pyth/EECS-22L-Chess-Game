#include <stdio.h>
#include <stdbool.h>
#include <string.h>
//#include "Chess.c"


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
//=======================================================


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

struct Move* getMoves(int* moveCount);

struct Move* getLegalMoves(struct piece p, int* moveCount);

//probably belongs in another file
bool isLegalMove(struct Move, const struct gameState* gs);

int getScore(const struct gameState* gs)
{
    int score = 0;
    //int checkmate = 1e8;

    static const int weight[7]=
    {
        [King] = 100000000,
        [Queen] = 900,
        [Knight] = 300,
        [Bishop] = 350,
        [Rook] = 500,
        [Ant] = 100,
        [Anteater] = 330
    };

    //ranks = rows (row)
    //file  = columns (col)

    for(int row = 0; row < 8; row++)
    {   
        //find each piece on board
        for(int col = 0 ; col < 10; col++)
        {
            struct piece* p = gs->board[row][col];
            if (p) {
                //get value of piece
                int value = weight[p->piece_];
                //if White, add the score, else if Black, subtract the score
                score += (p->color == White) ? value : -value;
        }
    }

    return score;
}

int miniMax(const struct gameState* gs, int depth, int alpha, int beta, bool playerColor)
{
    if (depth == 0) return getScore(gs);

    int moveCount = 0;
    struct Move* moves = getMoves
}

int movePiece_Computer(struct gameState* gs);