#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

enum pieceType {
    ANT, ROOK, KNIGHT, BISHOP, QUEEN, KING, ANTEATER, 
};

enum pieceColor {
    WHITE, BLACK
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

struct gameState {
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
