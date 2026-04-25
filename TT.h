#ifndef TT_H
#define TT_H

#include <stdbool.h>
#include <stdint.h>

#define TT_SIZE 1000000
#define TT_FLAG_EXACT 0
#define TT_FLAG_LOWER 1
#define TT_FLAG_UPPER 2

bool lookupTT(uint64_t hash, int depth, int alpha, int beta, int* score, int ply);
void storeTT(uint64_t hash, int score, int depth, int flag, int ply);
void clearTT(void);

#endif
