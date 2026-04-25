#include "Ant.h"

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


bool buildAnteaterPath(struct piece* board[8][10],
                       int fromRow, int fromCol,
                       int eatRow, int eatCol,
                       int toRow, int toCol,
                       enum pieceColor attackerColor,
                       struct location* path,
                       int* pathCount)
{
    enum pieceColor opponent = (attackerColor == WHITE) ? BLACK : WHITE;

    //needed to rebuild path
    int parentRow[8][10];
    int parentCol[8][10];
    bool visited[8][10];
    //BFS queue
    struct location queue[80];
    int head = 0, tail = 0;

    //reset visited
    memset(visited, 0, sizeof(visited));

    //go through board initializing as -1 for no parent
    for (int row = 0; row < 8; row++)
        for (int col = 0; col < 10; col++) 
        {
            parentRow[row][col] = -1;
            parentCol[row][col] = -1;
        }

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

    //set visited and add to BFS queue
    visited[eatRow][eatCol] = true;
    queue[tail++] = (struct location){eatRow, eatCol};

    //
    while (head < tail)
    {
        //find current location and increment head count
        struct location current = queue[head++];
        
        //if final ant is reached, stop the search
        if (current.row == toRow && current.col == toCol) break;

        //orthogonal directions(no diagonals)
        int orth_dir[4][2] = {{0,1}, {0,-1}, {1,0}, {-1,0}}; //Ex: {0,1} 1 col right or 1 tile right
        for (int d = 0; d < 4; d++)
        {
            int nr = current.row + orth_dir[d][0]; //new row = curr row + row_in_pair
            int nc = current.col + orth_dir[d][1]; //new col = curr col + col_in_pair

            //check bounds
            if (nr < 0 || nr >= 8 || nc < 0 || nc >= 10 || visited[nr][nc]) continue;

            //if enemy ant in location
            if (board[nr][nc] && board[nr][nc]->piece == ANT && board[nr][nc]->color == opponent)
            {
                visited[nr][nc] = true; //visit
                parentRow[nr][nc] = current.row; //tile was reached from current row
                parentCol[nr][nc] = current.col; //tile was reached from current col
                queue[tail++] = (struct location){nr, nc}; //store in queue and increment tail(to be explored)
            }
        }
    }

    //no valid path if never reached destination
    if (!visited[toRow][toCol]) return false;

    //store the path backwards in a temp array 
    struct location temp[80];
    int count = 0;
    //start from the the destination tile, walking backwards
    for (int r = toRow, c = toCol; r != -1 && c != -1; )
    {
        temp[count++] = (struct location){r, c}; //store current tile in backwards path 
        if (r == eatRow && c == eatCol) break;  //until we reach the first ant eaten

        int nextR = parentRow[r][c];//next row
        int nextC = parentCol[r][c];//next col
        r = nextR; //move backwards 1 row
        c = nextC; //move backwards 1 col
    }

    //check if path is empty, and last ant in backward path is first eaten ant
    if (count == 0 || temp[count - 1].row != eatRow || temp[count - 1].col != eatCol)
        return false;

    //create the forward path by reversing the backwards temp array
    for (int i = 0; i < count; i++)
        path[i] = temp[count - 1 - i];

    //how many ants were taken
    *pathCount = count;
    return true;
}