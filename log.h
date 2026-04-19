#ifndef LOG_H
#define LOG_H

struct log {
    string move; // "a1 - j10"
    int piece; // Black or White
    static int moveNumber; // 1-1000 for # of moves in the game
    int history[1000];
};

//Logs move made by chess user into the log
void logMove (struct pos position, struct piece* Piece, const struct log logOfMoves);

//Converts move in the log to chess notation for the user
char[] convertLogMove (const struct log* Log);

//Converts the position characters into ints to be used on the board
void convertPos (struct pos position);

#endif