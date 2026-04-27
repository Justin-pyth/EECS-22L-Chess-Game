#include "TT.h"
#include "Engine.h"
#include <string.h>

//transposition table

struct TTEntry {
    uint64_t hash;
    int score;
    int depth;
    int flag;
};
struct TTEntry tt[TT_SIZE];

static int scoreToTT(int score, int ply)
{
    if (score > INF - MAX_DEPTH)
        return score + ply;
    if (score < -INF + MAX_DEPTH)
        return score - ply;

    return score;
}

static int scoreFromTT(int score, int ply)
{
    if (score > INF - MAX_DEPTH)
        return score - ply;
    if (score < -INF + MAX_DEPTH)
        return score + ply;

    return score;
}

//store the position to the t_table
void storeTT(uint64_t hash, int score, int depth, int flag, int ply)
{
    //keep shared bits and assign as index
    unsigned int i = hash % TT_SIZE;
    //store metadata within that specific index
    tt[i] = (struct TTEntry){hash, scoreToTT(score, ply), depth, flag};
}

//look up function for t_table (basic implementation from chess programming)
bool lookupTT(uint64_t hash, int depth, int alpha, int beta, int* score, int ply)
{
    //go where it might be stored
    unsigned int i = hash % TT_SIZE;
    struct TTEntry* entry = &tt[i];
    int entryScore;

    //check the position and depth
    if (entry->hash != hash || entry->depth < depth)
        return false;

    entryScore = scoreFromTT(entry->score, ply);

    //case : this path was evaluated before so the score is the exact same
    if (entry->flag == TT_FLAG_EXACT)
    {
        *score = entryScore;
        return true;
    }
    //case : raise the alpha, as score was atleast this value
    else if (entry->flag == TT_FLAG_LOWER)
    {
        alpha = (entryScore > alpha) ? entryScore : alpha;
    }
    //case : score was lower than this, so need to lower beta
    else if (entry->flag == TT_FLAG_UPPER)
    {
        beta = (entryScore < beta) ? entryScore : beta;
    }

    //at some point, the correct path would be bounded
    if (alpha >= beta)
    {
        *score = entryScore;
        return true;
    }

    //did not reduce the bounds, go back to normal search
    return false;
}

void clearTT(void)
{
    memset(tt, 0, sizeof(tt));
}
