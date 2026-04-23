#include "log.h"
#include "Moves.h"
#include "Engine.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int encodeLocation(struct location loc, enum pieceType type) {
    return (int)type * 100 + loc.col * 10 + loc.row;
}

void decodeLocation(int encoded, enum pieceType* type, struct location* loc) {
    *type = (enum pieceType)(encoded / 100);
    encoded %= 100;
    loc->col = encoded / 10;
    loc->row = encoded % 10;
}

void locationToNotation(struct location loc, char* buf, int bufSize) {
    char fileLetter = 'a' + loc.row; // row 0→'a', 7→'h'
    int  rankNumber = loc.col + 1;   // col 0→1,   9→10

    snprintf(buf, bufSize, "%c%d", fileLetter, rankNumber);
}

int encodeMoveLocation(struct location from, struct location to, struct piece* Piece) {
    int fromEncoded = encodeLocation(from, Piece->type);
    int toEncoded   = to.col * 10 + to.row;
    return fromEncoded * 1000 + toEncoded;
}

void logMove(struct location from, struct location to, struct piece* Piece, struct log* logOfMoves) {
    int encoded = encodeMoveLocation(from, to, Piece);

    int idx = logOfMoves->moveNumber - 1;
    if (idx >= 0 && idx < 1000) {
        logOfMoves->history[idx] = encoded;
    }

    // Human-readable: "a1 - h10"
    char fromBuf[4], toBuf[4];
    locationToNotation(from, fromBuf, sizeof(fromBuf));
    locationToNotation(to, toBuf, sizeof(toBuf));
    snprintf(logOfMoves->move, sizeof(logOfMoves->move), "%s - %s", fromBuf, toBuf);
}


char* convertLogMove(const struct log* Log, int moveIndex) {
    if (moveIndex < 0 || moveIndex >= 1000) return NULL;

    int encoded = Log->history[moveIndex];

    // Split upper 3 (from) and lower 3 (to)
    int fromEncoded = encoded / 1000;
    int toEncoded   = encoded % 1000;

    // Decode from
    enum pieceType  type;
    struct location fromLoc, toLoc;
    decodeLocation(fromEncoded, &type, &fromLoc);

    // Decode to (no piece digit)
    toLoc.col = toEncoded / 10;
    toLoc.row = toEncoded % 10;

    // Convert to notation strings
    char fromBuf[4], toBuf[4];
    locationToNotation(fromLoc, fromBuf, sizeof(fromBuf));
    locationToNotation(toLoc,   toBuf,   sizeof(toBuf));

    char* result = (char*)malloc(12 * sizeof(char));
    if (!result) return NULL;

    snprintf(result, 12, "%c%s-%s", pieceChars[(int)type], fromBuf, toBuf);
    return result; // caller must free()
}