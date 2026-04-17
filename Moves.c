#include "Moves.h"
#include "Chess.c" // TODO: is this allowed? to get the enums and structs from Chess.c

void getMoves(struct gameState* gs, struct move* moves, int* moveCount)
{
    ;
}


/*
   Returns a list of legal moves that the specified piece can perform
*/
struct move* getLegalMoves(struct piece p, int* moveCount)
{
   ;
}


/*
   Checks if the specified move for that piece is legal
*/
bool isLegalMove(struct move moveMade, const struct gameState* gs)
{
   ;
   // go through list of moves for a certain piece
        // if piece is in check, then the list of moves for that piece may or may not be a legal move
}


/*
    doMove()
    Performs the move by using pos1 and pos2 from the move struct
*/
void makeMove(struct piece* board[8][10], struct move thisMove, bool isCapture)
{
    // get startingPos = pos1 and endingPos = pos2 from the move struct

    // set the endingPos to the piece that is being moved

    // set the startingPos to NULL
}


/*
    getAntMoves()
    Returns the moves that a given Ant (pawn) piece can make
*/
struct move* getAntMoves(struct piece* board[8][10], int row, int col, int* moveCount)
{
    struct move* moves = malloc(4 * sizeof(struct move)); // an ant piece has at most 4 possible moves
    struct piece* p = board[row][col];
    moveCount = 0;

    int direction = 0;
    if (p->color == WHITE)
    {
        direction = 1;
    }
    else if (p->color == BLACK)
    {
        direction = -1;
    }

    int oneStep = row + direction;

    // If ant in starting position, can move (one or) two squares forward
    if (p->color == WHITE && row == 1 || p->color == BLACK && row == 6)
    {
        int twoSteps = row + (2 * direction); // can only move two steps if in starting position

        if (board[row+oneStep][col] == NULL && board[row+twoSteps][col] == NULL)
        {
            // add possible move to moves
            char fromRank = row + 1;
            char fromFile = 'a' + col;
            char toRank = twoSteps + 1;
            char toFile = 'a' + col;

            struct pos fromPos;
            struct pos toPos;

            fromPos.rank = fromRank;
            fromPos.file = fromFile;
            toPos.rank = toRank;
            toPos.file = toFile;

            moves[*moveCount].pos1 = fromPos;
            moves[*moveCount].pos2 = toPos;

            (*moveCount)++;
        }
    }

    // Otherwise, ant can only move one square directly forward
    if (board[row+oneStep][col] == NULL && (row+oneStep) >= 0 && (row+oneStep) < 8)
    {
        // add possible move to moves
        char fromRank = row + 1;
        char fromFile = 'a' + col;
        char toRank = oneStep + 1;
        char toFile = 'a' + col;

        struct pos fromPos;
        struct pos toPos;

        fromPos.rank = fromRank;
        fromPos.file = fromFile;
        toPos.rank = toRank;
        toPos.file = toFile;

        moves[*moveCount].pos1 = fromPos;
        moves[*moveCount].pos2 = toPos;

        (*moveCount)++;
    }

    // Capture


    // Promotion
    // insert helper function specifically for pawn promotion here
    // doPromotion()

    // En passant
    // insert helper function specifically for en passant here
    // doEnPassant()
}


/*
    getBishopMoves()
    Returns the moves that a given Bishop piece can make
*/
struct move* getBishopMoves(struct piece p, int* moveCount)
{

}


/*
    getKnightMoves()
    Returns the moves that a given Knight piece can make
*/
struct move* getKnightMoves(struct piece p, int* moveCount)
{

}


/*
    getRookMoves()
    Returns the moves that a given Rook piece can make
*/
struct move* getRookMoves(struct piece p, int* moveCount)
{

}


/*
    getQueenMoves()
    Returns the moves that a given Queen piece can make
*/
struct move* getQueenMoves(struct piece p, int* moveCount)
{

}


/*
    getKingMoves()
    Returns the moves that a given King piece can make
*/
struct move* getKingMoves(struct piece p, int* moveCount)
{

}


/*
    getAnteaterMoves()
    Returns the moves that a given Anteater piece can make
*/
struct move* getAnteaterMoves(struct piece p, int* moveCount)
{

}

/*
    getCapture()
    Performs a capture by replacing the captured piece with the capturing piece on the board
*/
struct move* getCapture(struct piece pCaptured, struct piece pCapturing)
{

}


/*
    Promotion
    tbd
*/
struct move* doPromotion()
{

}


/*
    En Passant
    tbd
*/
struct move* doEnPassant()
{

}
