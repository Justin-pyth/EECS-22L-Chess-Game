#include <stdio.h>
#include <ctype.h>
#include "terminalTestingFunctions.h"

extern enum pieceType promptPromotion(void);

/* ------------------------------------------------------------------ */
/* pieceToChar                                                          */
/* ------------------------------------------------------------------ */

//USED FOR TESTING PURPOSES ONLY
char pieceToChar(const struct piece* p) {
    if (p == NULL) return '.';
    char base = '?';
    switch (p->piece) {
        case KING:     base = 'K'; break;
        case QUEEN:    base = 'Q'; break;
        case KNIGHT:   base = 'N'; break;
        case BISHOP:   base = 'B'; break;
        case ROOK:     base = 'R'; break;
        case ANT:      base = 'P'; break;
        case ANTEATER: base = 'A'; break;
    }
    if (p->color == BLACK && base >= 'A' && base <= 'Z')
        base = (char)(base + ('a' - 'A'));
    return base;
}

/* ------------------------------------------------------------------ */
/* printBoard                                                           */
/* ------------------------------------------------------------------ */

//USED FOR TESTING PURPOSES ONLY
void printBoard(struct piece* board[8][10]) {
    printf("\n    a b c d e f g h i j\n");
    printf("   ---------------------\n");
    for (int row = 7; row >= 0; row--) {
        printf("%d | ", row + 1);
        for (int col = 0; col < 10; col++) {
            printf("%c", pieceToChar(board[row][col]));
            if (col < 9) printf(" ");
        }
        printf(" |\n");
    }
    printf("   ---------------------\n");
}

//Game setup prompts                                                   


/*
 * promptGameMode — presents a mode menu and returns the selection.
 *   1. Human vs Human  — both players type moves at the terminal.
 *   2. Human vs AI     — one human player; the other is the computer.
 *   3. AI vs AI        — both sides played by the computer (demo).
 */
enum gameMode promptGameMode(void) {
    char input[8];
    printf("\nSelect game mode:\n");
    printf("  1. Human vs Human\n");
    printf("  2. Human vs AI\n");
    printf("  3. AI vs AI\n");
    printf("Choice (1-3): ");
    fflush(stdout);
    while (1) {
        if (fgets(input, sizeof(input), stdin) == NULL) return HUMAN_VS_AI;
        if (input[0] == '1') return HUMAN_VS_HUMAN;
        if (input[0] == '2') return HUMAN_VS_AI;
        if (input[0] == '3') return AI_VS_AI;
        printf("Invalid. Enter 1, 2, or 3: ");
        fflush(stdout);
    }
}

/*
 * promptColorChoice — asks which color the human wants to play.
 * Only called in HUMAN_VS_AI mode.
 */
enum pieceColor promptColorChoice(void) {
    char input[8];
    printf("Play as (W=White, B=Black): ");
    fflush(stdout);
    while (1) {
        if (fgets(input, sizeof(input), stdin) == NULL) return WHITE;
        char c = (char)toupper((unsigned char)input[0]);
        if (c == 'W') return WHITE;
        if (c == 'B') return BLACK;
        printf("Invalid. Enter W or B: ");
        fflush(stdout);
    }
}

/*
 * promptDifficulty — asks the user to choose an AI difficulty level.
 * Returns 0 (easy), 1 (medium), or 2 (hard).
 */
int promptDifficulty(void) {
    char input[16];
    printf("Select AI difficulty:\n");
    printf("  1. Easy\n");
    printf("  2. Medium\n");
    printf("  3. Hard\n");
    printf("Choice (1-3): ");
    fflush(stdout);
    while (1) {
        if (fgets(input, sizeof(input), stdin) == NULL) return 1;
        if (input[0] == '1') return 0;
        if (input[0] == '2') return 1;
        if (input[0] == '3') return 2;
        /* Hidden: "testingX" sets engine to exact depth X (negative sentinel) */
        int testDepth = 0;
        if (sscanf(input, "testing%d", &testDepth) == 1 && testDepth > 0)
            return -testDepth;
        printf("Invalid. Enter 1, 2, or 3: ");
        fflush(stdout);
    }
}

/*
 * getHumanMove — prompts for a move in "f2 f4" format, validates legality,
 * and returns the encoded Move. Returns 0 on EOF.
 */
Move getHumanMove(struct gameState* gs)
{
    char buf[32];
    while (1) {
        printf("Enter move (e.g. f2 f4): ");
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) return 0;

        char fc, tc;
        int  fr, tr;
        if (sscanf(buf, " %c%d %c%d", &fc, &fr, &tc, &tr) != 4) {
            printf("Bad format. Use <file><rank> <file><rank>  (e.g. f2 f4)\n");
            continue;
        }
        fc = (char)tolower((unsigned char)fc);
        tc = (char)tolower((unsigned char)tc);

        if (fc < 'a' || fc > 'j' || tc < 'a' || tc > 'j' ||
            fr < 1   || fr > 8   || tr < 1   || tr > 8) {
            printf("Out of range. Files a-j, ranks 1-8.\n");
            continue;
        }

        struct move m;
        m.pos1.file = fc; m.pos1.rank = (char)fr;
        m.pos2.file = tc; m.pos2.rank = (char)tr;

        if (!isLegalMove(m, gs)) {
            printf("Illegal move. Try again.\n");
            continue;
        }

        /* Find matching encoded Move to preserve flags (en passant, castling, etc.) */
        Move encoded[MAX_MOVES];
        int count = 0;
        getMoves(gs, encoded, &count);
        int fromRow = fr - 1, fromCol = fc - 'a';
        int toRow   = tr - 1, toCol   = tc - 'a';

        for (int i = 0; i < count; i++) {
            if (getFromRow(encoded[i]) == fromRow && getFromCol(encoded[i]) == fromCol &&
                getToRow  (encoded[i]) == toRow   && getToCol  (encoded[i]) == toCol) {
                int flags = getFlags(encoded[i]);
                if (flags >= MOVE_PROMO_QUEEN && flags <= MOVE_PROMO_ANTEATER) {
                    enum pieceType choice = promptPromotion();
                    int pf;
                    switch (choice) {
                        case QUEEN:    pf = MOVE_PROMO_QUEEN;    break;
                        case ROOK:     pf = MOVE_PROMO_ROOK;     break;
                        case BISHOP:   pf = MOVE_PROMO_BISHOP;   break;
                        case KNIGHT:   pf = MOVE_PROMO_KNIGHT;   break;
                        case ANTEATER: pf = MOVE_PROMO_ANTEATER; break;
                        default:       pf = MOVE_PROMO_QUEEN;    break;
                    }
                    return createMove(fromRow, fromCol, toRow, toCol, pf);
                }
                return encoded[i];
            }
        }
        return 0;
    }
}