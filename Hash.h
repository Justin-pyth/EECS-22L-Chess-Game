#ifndef HASH_H
#define HASH_H

#include "Moves.h"

#define MAX_REPETITION_HISTORY 4096

extern uint64_t HASHES[MAX_REPETITION_HISTORY];
extern int currentPly;

uint64_t positionHash(const struct gameState* gs);
void storePositionHash(const struct gameState* gs);
void resetRepetitionTracking(void);
bool isThreeFoldDraw(uint64_t hash, int curPly);

#endif
