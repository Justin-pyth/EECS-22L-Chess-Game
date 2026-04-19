#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

enum pieceType {
    KING, QUEEN, KNIGHT, BISHOP, ROOK, ANT, ANTEATER
};

enum pieceColor {
    BLACK, WHITE
};

enum gameMode {
    HUMAN_VS_HUMAN,
    HUMAN_VS_AI,
    AI_VS_AI
};

struct piece {
    enum pieceType  piece;
    enum pieceColor color;
};

typedef uint32_t Move;

struct log {
    Move move; // encoded move (first 7 bits are fromTile, next 7 bits are toTile)
    int piece; // Black or White
    int moveNumber; // 1-1000 for # of moves in the game
    int history[1000];
};

struct GameState {
    struct piece* board[8][10];
    enum pieceColor currentPlayer;

    bool whiteKingMoved;
    bool blackKingMoved;
    bool whiteRookMovedQS;   /* queenside rook — column A (index 0) */
    bool whiteRookMovedKS;   /* kingside  rook — column J (index 9) */
    bool blackRookMovedQS;
    bool blackRookMovedKS;
    int  enPassantCol;       /* -1 if unavailable                    */
    int  enPassantRow;       /* row of the double-advanced ant       */

    //trackers for move log + 50 turn draw rule
    int halfMove_count;
    int fullMove_count;
    struct log move_log;
};

#endif /* TYPES_H */
