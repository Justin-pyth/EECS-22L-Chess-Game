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
    makeMove()
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

    // TODO: Capture


    // Promotion
    // insert helper function specifically for pawn promotion here
    // doPromotion()

    // En passant
    // insert helper function specifically for en passant here
    // doEnPassant()

    return moves;
}


/*
    getBishopMoves()
    Returns the moves that a given Bishop piece can make (diagonal rays)
*/
struct move* getBishopMoves(struct piece* board[8][10], int row, int col, int* moveCount)
{
    struct move* moves = malloc(13 * sizeof(struct move)); // a bishop piece has at most 13 possible moves
    struct piece* p = board[row][col];
    moveCount = 0;
    
    // checks for diagonal free spaces, not including captures
    int bishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};

    for (int d = 0; d < 4; d++) {
        int r = row + bishopDirs[d][0];
        int c = col + bishopDirs[d][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 10) 
        {
            if (board[r][c] == NULL) 
            {
                char fromRank = row + 1;
                char fromFile = 'a' + col;
                char toRank = r + 1;
                char toFile = 'a' + c;

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
            r += bishopDirs[d][0];
            c += bishopDirs[d][1];
        }
    }

    // TODO: Capture

    return moves;
}


/*
    getKnightMoves()
    Returns the moves that a given Knight piece can make (eight L-shaped jumps)
*/
struct move* getKnightMoves(struct piece* board[8][10], int row, int col, int* moveCount)
{
    struct move* moves = malloc(8 * sizeof(struct move)); // a bishop piece has at most 8 possible moves
    struct piece* p = board[row][col];
    moveCount = 0;

    // checks eight L-shaped jumps for free spaces, not including captures
    int knightMoves[8][2] = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};

    for (int i = 0; i < 8; i++) 
    {
        int r = row + knightMoves[i][0];
        int c = col + knightMoves[i][1];
        if (r >= 0 && r < 8 && c >= 0 && c < 10 && board[r][c] == NULL) 
        {
            char fromRank = row + 1;
            char fromFile = 'a' + col;
            char toRank = r + 1;
            char toFile = 'a' + c;

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

    // TODO: Capture

    return moves;
}


/*
    getRookMoves()
    Returns the moves that a given Rook piece can make (horizontal and vertical rays)
*/
struct move* getRookMoves(struct piece* board[8][10], int row, int col, int* moveCount)
{
    struct move* moves = malloc(14 * sizeof(struct move)); // a bishop piece has at most 8 possible moves
    struct piece* p = board[row][col];
    moveCount = 0;

    // checks for horizontal and vertical free spaces, not including captures
    int rookDirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

    for (int d = 0; d < 4; d++) 
    {
        int r = row + rookDirs[d][0];
        int c = col + rookDirs[d][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 10) 
        {
            if (board[r][c] == NULL) 
            {
                char fromRank = row + 1;
                char fromFile = 'a' + col;
                char toRank = r + 1;
                char toFile = 'a' + c;

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
            r += rookDirs[d][0];
            c += rookDirs[d][1];
        }
    }

    // TODO: Capture

    return moves;
}


/*
    getQueenMoves()
    Returns the moves that a given Queen piece can make (horizontal, vertical, and diagonal rays)
*/
struct move* getQueenMoves(struct piece p, int* moveCount)
{

}


/*
    getKingMoves()
    Returns the moves that a given King piece can make (one step in any direction)
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
