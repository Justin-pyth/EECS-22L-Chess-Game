#ifndef LOG_H
#define LOG_H

#include "log.h"
#include "Moves.h"
#include "Engine.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct log {
    char move[16]; // "a1 - j10"
    int piece; // Black or White
    int moveNumber; // 1-1000 for # of moves in the game
    int history[1000];
};

// Starting board layout for reference
const enum pieceType backRow[10] = {
    ROOK, KNIGHT, BISHOP, ANTEATER, QUEEN,
    KING, ANTEATER, BISHOP, KNIGHT, ROOK
};

//Piece names
const char* pieceNames[] = {
    "Ant", "Rook", "Knight", "Bishop", "Queen", "King", "Anteater"
};

//Piece character lookup
const char pieceChars[] = { 'A', 'R', 'N', 'B', 'Q', 'K', 'T' };

//Encodes the board position fof a piece + piece type into a 3-digit int
int encodeLocation(struct location loc, enum pieceType type);

//Decodes encoded position back into piece type, row, and column
void decodeLocation(int encoded, enum pieceType* type, struct location* loc);

//Converts the position characters into ints to be used on the board
void locationToNotation(struct location loc, char* buf, int bufSize);

//Encodes the origin to destination into a 6-digit int
int encodeMoveLocation(struct location from, struct location to, struct piece* Piece);

//Logs move made by chess user into the log
//Odd moves = from, even moves = to
void logMove(struct location from, struct location to, struct piece* Piece, struct log* logOfMoves);

// Converts encoded move from history and returns it as a readable string
char* convertLogMove(const struct log* Log, int moveIndex);

#endif