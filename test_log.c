#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

void printSeparator() {
    printf("--------------------------------------------------\n");
}
 
void printHeader(const char* title) {
    printSeparator();
    printf("  %s\n", title);
    printSeparator();
}
 
// Makes a move and prints a full breakdown of what was encoded
void testMove(struct log* gameLog, struct piece* p,
              struct location from, struct location to,
              const char* description) {
 
    logMove(from, to, p, gameLog);
 
    int moveIdx     = gameLog->moveNumber - 1;
    int encoded     = gameLog->history[moveIdx];
    char* notation  = convertLogMove(gameLog, moveIdx);
 
    char fromBuf[4], toBuf[4];
    locationToNotation(from, fromBuf, sizeof(fromBuf));
    locationToNotation(to,   toBuf,   sizeof(toBuf));
 
    printf("Move #%d — %s\n", gameLog->moveNumber, description);
    printf("  Piece       : %s (%s)\n",
           pieceNames[(int)p->pieceType],
           p->color == WHITE ? "White" : "Black");
    printf("  From        : board[%d][%d] → %s\n", from.row, from.col, fromBuf);
    printf("  To          : board[%d][%d] → %s\n", to.row,   to.col,   toBuf);
    printf("  Encoded int : %d\n", encoded);
    printf("  Notation    : %s\n", notation ? notation : "ERROR");
    printf("  Log string  : \"%s\"\n\n", gameLog->move);
 
    free(notation);
    gameLog->moveNumber++;
}
 
/* -----------------------------------------------------------
 * Tests
 * ----------------------------------------------------------- */
 
// Test 1: Basic ant (pawn) opening moves
void testAntOpenings(struct log* gameLog) {
    printHeader("TEST 1: Ant (Pawn) Opening Moves");
 
    struct piece whiteAnt = { ANT, WHITE };
    struct piece blackAnt = { ANT, BLACK };
 
    // White ant e2→e4 (row=4, col=1 → row=4, col=3)
    testMove(gameLog, &whiteAnt,
             (struct location){4, 1}, (struct location){4, 3},
             "White Ant e2-e4");
 
    // Black ant e7→e5 (row=4, col=6 → row=4, col=4)
    testMove(gameLog, &blackAnt,
             (struct location){4, 6}, (struct location){4, 4},
             "Black Ant e7-e5");
 
    // White ant d2→d4
    testMove(gameLog, &whiteAnt,
             (struct location){3, 1}, (struct location){3, 3},
             "White Ant d2-d4");
}
 
// Test 2: Back row pieces including Anteater
void testBackRowPieces(struct log* gameLog) {
    printHeader("TEST 2: Back Row Pieces (Knight, Anteater, Queen, King)");
 
    struct piece whiteKnight   = { KNIGHT,   WHITE };
    struct piece whiteAnteater = { ANTEATER, WHITE };
    struct piece whiteQueen    = { QUEEN,    WHITE };
    struct piece blackAnteater = { ANTEATER, BLACK };
 
    // White knight b1→c3 (row=1, col=0 → row=2, col=2)
    testMove(gameLog, &whiteKnight,
             (struct location){1, 0}, (struct location){2, 2},
             "White Knight b1-c3");
 
    // White anteater d1→d3 (row=3, col=0 → row=3, col=2)
    testMove(gameLog, &whiteAnteater,
             (struct location){3, 0}, (struct location){3, 2},
             "White Anteater d1-d3");
 
    // Black anteater d8→d6 (row=3, col=7 → row=3, col=5)
    testMove(gameLog, &blackAnteater,
             (struct location){3, 7}, (struct location){3, 5},
             "Black Anteater d8-d6");
 
    // White queen e1→e3 (row=4, col=0 → row=4, col=2)
    testMove(gameLog, &whiteQueen,
             (struct location){4, 0}, (struct location){4, 2},
             "White Queen e1-e3");
}
 
// Test 3: Corner and edge positions
void testEdgePositions(struct log* gameLog) {
    printHeader("TEST 3: Edge and Corner Positions");
 
    struct piece whiteRook = { ROOK, WHITE };
    struct piece blackRook = { ROOK, BLACK };
 
    // White rook a1→a5 (row=0, col=0 → row=0, col=4)
    testMove(gameLog, &whiteRook,
             (struct location){0, 0}, (struct location){0, 4},
             "White Rook a1-a5 (corner move)");
 
    // Black rook j8→j4 (row=7 wait — j is col 9) 
    // h10→h6: row=7, col=9 → row=7, col=5
    testMove(gameLog, &blackRook,
             (struct location){7, 9}, (struct location){7, 5},
             "Black Rook h10-h6 (far corner move)");
}
 
// Test 4: Replay the full log
void testReplayLog(const struct log* gameLog) {
    printHeader("TEST 4: Full Game Log Replay");
 
    printf("Total moves recorded: %d\n\n", gameLog->moveNumber - 1);
 
    for (int i = 0; i < gameLog->moveNumber - 1; i++) {
        char* notation = convertLogMove(gameLog, i);
        printf("  Move %2d : encoded=%-8d  notation=%s\n",
               i + 1, gameLog->history[i], notation ? notation : "ERROR");
        free(notation);
    }
    printf("\n");
}
 
// Test 5: encodeLocation and decodeLocation round-trip
void testEncodeDecode() {
    printHeader("TEST 5: encodeLocation / decodeLocation Round-Trip");
 
    struct location testLocs[] = {
        {0, 0}, {7, 9}, {4, 4}, {3, 0}, {6, 7}
    };
    enum pieceType testPieces[] = {
        ANT, ROOK, QUEEN, ANTEATER, KING
    };
    int numTests = 5;
 
    for (int i = 0; i < numTests; i++) {
        struct location orig  = testLocs[i];
        enum pieceType  ptype = testPieces[i];
 
        int encoded = encodeLocation(orig, ptype);
 
        enum pieceType  decodedType;
        struct location decodedLoc;
        decodeLocation(encoded, &decodedType, &decodedLoc);
 
        char buf[4];
        locationToNotation(orig, buf, sizeof(buf));
 
        int pass = (decodedType == ptype &&
                    decodedLoc.row == orig.row &&
                    decodedLoc.col == orig.col);
 
        printf("  board[%d][%d] %-8s encoded=%-4d  decoded=%-8s row=%d col=%d  [%s]\n",
               orig.row, orig.col,
               pieceNames[(int)ptype],
               encoded,
               pieceNames[(int)decodedType],
               decodedLoc.row, decodedLoc.col,
               pass ? "PASS" : "FAIL");
    }
    printf("\n");
}