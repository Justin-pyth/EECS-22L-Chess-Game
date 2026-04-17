#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

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

struct GameState {
    bool whiteKingMoved;
    bool blackKingMoved;
    bool whiteRookMovedQS;   /* queenside rook — column A (index 0) */
    bool whiteRookMovedKS;   /* kingside  rook — column J (index 9) */
    bool blackRookMovedQS;
    bool blackRookMovedKS;
    int  enPassantCol;       /* -1 if unavailable                    */
    int  enPassantRow;       /* row of the double-advanced ant       */
};

#endif /* TYPES_H */
