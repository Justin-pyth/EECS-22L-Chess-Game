#ifndef LOG_H
#define LOG_H

struct log {
    string move; // "a1 - j10"
    int piece; // Black or White
    static int moveNumber; // 1-1000 for # of moves in the game
    int history[1000];
};

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
void logMove (struct move moves, struct piece* Piece, const struct log* logOfMoves);

// Converts encoded move from history and returns it as a readable string
char* convertLogMove(const struct log* Log, int moveIndex);

#endif