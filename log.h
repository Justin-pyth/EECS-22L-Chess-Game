#ifndef LOG_H
#define LOG_H

void logMove (struct pos position, struct piece* Piece, struct log logOfMoves);


char[] convertLogMove (struct log Log);

void convertPos (struct pos position);



#endif