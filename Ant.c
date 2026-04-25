#include "Ant.h"

static void dfsAnteaterPath(struct piece* board[8][10],
                            int row, int col,
                            int toRow, int toCol,
                            enum pieceColor opponent,
                            bool visited[8][10],
                            struct location* currentPath,
                            int currentCount,
                            struct location* bestPath,
                            int* bestCount);

Move chooseBestAnteaterMove(const struct gameState* gs, Move* firstAnt, int count)
{
    if (count <= 0) return 0;

    Move bestMove = firstAnt[0];
    int bestPathCount = -1;

    for (int i = 0; i < count; i++)
    {
        if (getFlags(firstAnt[i]) != MOVE_ANTEATING)
            return firstAnt[i];

        //FROM TILE
        int fr = getFromRow(firstAnt[i]);   int fc = getFromCol(firstAnt[i]);
        //TO TILE
        int tr = getToRow(firstAnt[i]);     int tc = getToCol(firstAnt[i]);
        //EAT TILE
        int eatRow = getEatRow(firstAnt[i]);int eatCol = getEatCol(firstAnt[i]);

        //moving piece
        struct piece* moving = gs->board[fr][fc];
        struct location path[80];
        int pathCount = 0;

        //check if the piece exists, can create a path of more than 1 eaten ant, and if the new pathcount is greater than old
        if (moving && buildAnteaterPath((struct piece* (*)[10])gs->board, fr, fc, eatRow, eatCol,
            tr, tc, moving->color, path, &pathCount) &&
            pathCount > bestPathCount)
        {
            //set new best path count, and new best starting ant to eat
            bestPathCount = pathCount;
            bestMove = firstAnt[i];
        }
    }

    return bestMove;
}


/*
    getAnteaterMoves
    Returns all possible moves that a given Anteater piece can make
*/
void getAnteaterMoves(struct piece* board[8][10], int row, int col, uint32_t* moves, int* moveCount)
{
    struct piece* p = board[row][col];
    *moveCount = 0;
    enum pieceColor opp = (p->color == WHITE) ? BLACK : WHITE;

    //create a 3x3 centered around anteater for quiet move validation
    for (int dr = -1; dr <= 1; dr++)
    {
        for (int dc = -1; dc <= 1; dc++)
        {
            if (dr == 0 && dc == 0) continue;
            int r = row + dr, c = col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 10 && board[r][c] == NULL)
                moves[(*moveCount)++] = createMove(row, col, r, c, MOVE_NORMAL);
        }
    }
    //anteating portion
    for (int dr = -1; dr <= 1; dr++)
    {
        for (int dc = -1; dc <= 1; dc++)
        {
            //skip center tile(where anteater starts)
            if (dr == 0 && dc == 0) continue;
            //set new row and new col
            int r = row + dr, c = col + dc;
            //bounds
            if (r < 0 || r >= 8 || c < 0 || c >= 10) continue;
            //check if an enemy any piece exists at target tile
            if (!board[r][c] || board[r][c]->piece != ANT || board[r][c]->color != opp) continue;

            bool visited[8][10] = {0}; //init visited
            struct location queue[80]; //create BFS queue
            int head = 0, tail = 0; 

            visited[r][c] = true; //first ant eaten
            //store pos at tail and increment count
            queue[tail++] = (struct location){r, c};

            //check until queue is explored
            while (head < tail)
            {
                //store current location in the queue and create an encoded anteater move
                struct location current = queue[head++];
                moves[(*moveCount)++] = createAnteaterMove(row, col, current.row, current.col, r, c);

                //all possible orthogonal directions
                int orth_dir[4][2] = {{0,1}, {0,-1}, {1,0}, {-1,0}};
                for (int d = 0; d < 4; d++)
                {
                    //check each tile orthogonally from current tile
                    int nr = current.row + orth_dir[d][0];
                    int nc = current.col + orth_dir[d][1];
                    //bounds check and ignore if visited
                    if (nr < 0 || nr >= 8 || nc < 0 || nc >= 10 || visited[nr][nc]) continue;

                    //if enemy ant
                    if (board[nr][nc] && board[nr][nc]->piece == ANT && board[nr][nc]->color == opp)
                    {
                        visited[nr][nc] = true;
                        queue[tail++] = (struct location){nr, nc}; //add to eaten queue
                    }
                }
            }
        }
    }
}


//helper function to create the path
bool buildAnteaterPath(struct piece* board[8][10],
                       int fromRow, int fromCol,
                       int eatRow, int eatCol,
                       int toRow, int toCol,
                       enum pieceColor attackerColor,
                       struct location* path,
                       int* pathCount)
{
    enum pieceColor opponent = (attackerColor == WHITE) ? BLACK : WHITE;

    bool visited[8][10] = {0};
    struct location currentPath[80];
    struct location bestPath[80];
    int bestCount = 0;

    //check if any available units  to be eaten 1 unit way
    if (abs(eatRow - fromRow) > 1 || abs(eatCol - fromCol) > 1)
        return false;   //if not, return false for illegal

    //bounds
    if (eatRow < 0 || eatRow >= 8 || eatCol < 0 || eatCol >= 10 ||
        toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 10)
        return false;

    //check the first square, it must be an ant to be eaten
    if (!board[eatRow][eatCol] || board[eatRow][eatCol]->piece != ANT ||
        board[eatRow][eatCol]->color != opponent)
        return false;

    //check the final square, it must be an ant to be eaten
    if (!board[toRow][toCol] || board[toRow][toCol]->piece != ANT ||
        board[toRow][toCol]->color != opponent)
        return false;

    //start a dfs search to find the path that eats the most ants on current path
    //only the best path will be chosen to avoid human ambiguoity in multiple legal paths to destination
    visited[eatRow][eatCol] = true;
    dfsAnteaterPath(board, eatRow, eatCol, toRow, toCol, opponent, visited, currentPath, 0, bestPath, &bestCount);

    if (bestCount == 0)
        return false;

    for (int i = 0; i < bestCount; i++)
        path[i] = bestPath[i];

    //how many ants were taken
    *pathCount = bestCount;
    return true;
}



//dfs search to return the path that eats the most ants
static void dfsAnteaterPath(struct piece* board[8][10],
                            int row, int col,
                            int toRow, int toCol,
                            enum pieceColor opponent,
                            bool visited[8][10],
                            struct location* currentPath,
                            int currentCount,
                            struct location* bestPath,
                            int* bestCount)
{
    currentPath[currentCount++] = (struct location){row, col};

    //if final ant is reached, keep the longest valid path found so far
    if (row == toRow && col == toCol)
    {
        if (currentCount > *bestCount)
        {
            for (int i = 0; i < currentCount; i++)
                bestPath[i] = currentPath[i];
            *bestCount = currentCount;
        }
        return;
    }

    //orthogonal directions(no diagonals)
    int orth_dir[4][2] = {{0,1}, {0,-1}, {1,0}, {-1,0}};
    for (int d = 0; d < 4; d++)
    {
        int nr = row + orth_dir[d][0];
        int nc = col + orth_dir[d][1];

        //check bounds
        if (nr < 0 || nr >= 8 || nc < 0 || nc >= 10 || visited[nr][nc]) continue;

        //if enemy ant in location
        if (board[nr][nc] && board[nr][nc]->piece == ANT && board[nr][nc]->color == opponent)
        {
            visited[nr][nc] = true;
            dfsAnteaterPath(board, nr, nc, toRow, toCol, opponent,
                            visited, currentPath, currentCount, bestPath, bestCount);
            visited[nr][nc] = false;
        }
    }
}
