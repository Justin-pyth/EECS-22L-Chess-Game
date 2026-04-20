#ifndef TERMINAL_TESTING_FUNCTIONS_H
#define TERMINAL_TESTING_FUNCTIONS_H

#include "types.h"
#include "Moves.h"

/* ------------------------------------------------------------------ */
/* Function prototypes                                                  */
/* ------------------------------------------------------------------ */
char            pieceToChar(const struct piece* p);
void            printBoard(struct piece* board[8][10]);

enum gameMode   promptGameMode(void);
enum pieceColor promptColorChoice(void);
int             promptDifficulty(void);   /* returns 0=easy, 1=medium, 2=hard */

/* Prompts human for a move, validates legality, returns encoded Move (0 on EOF). */
Move            getHumanMove(struct gameState* gs);

#endif /* TERMINAL_TESTING_FUNCTIONS_H */
