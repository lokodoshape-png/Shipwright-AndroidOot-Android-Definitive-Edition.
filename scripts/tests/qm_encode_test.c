// OOT D.E.: host test of QM_Encode (soh/src/overlays/misc/ovl_kaleido_scope/questmenu/questmenu_gfx.c).
// scripts/tests/run_qm_encode_test.sh extracts the function from the game source, so the test always runs the real
// code. Checks the output limit (never one byte past `max`, also when a letter needs two codes), the terminator, the
// Portuguese letters and broken UTF-8, then encodes every text of questmenu_data.c and reports any that would be cut.
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef int32_t s32;
typedef uint8_t u8;
typedef uint32_t u32;

#define QM_ACUTE 0x10
#define QM_TILDE 0x11
#define QM_DIAERESIS 0x12
#define QM_DOTLESS_I 0x13

#include "qm_encode_extracted.inc"

static int failures = 0;
#define CHECK(cond, ...)                         \
    do {                                         \
        if (!(cond)) {                           \
            failures++;                          \
            printf("FAIL %s:%d ", __FILE__, __LINE__); \
            printf(__VA_ARGS__);                 \
            printf("\n");                        \
        }                                        \
    } while (0)

// Encodes into a buffer with guard bytes around it; returns the length and checks nothing outside [0, max) changed.
static s32 Encode(const char* text, u8* out, s32 max) {
    u8 buf[1024];
    s32 n;
    s32 i;

    memset(buf, 0xEE, sizeof(buf));
    n = QM_Encode(text, buf + 16, max);
    for (i = 0; i < 16; i++) {
        CHECK(buf[i] == 0xEE, "underflow writing \"%s\" max %d", text, max);
    }
    for (i = 16 + max; i < (s32)sizeof(buf); i++) {
        CHECK(buf[i] == 0xEE, "overflow at +%d writing \"%s\" max %d", i - 16 - max, text, max);
        if (buf[i] != 0xEE) break;
    }
    CHECK(n >= 0 && n <= max, "length %d out of 0..%d", n, max);
    if (n < max) {
        CHECK(buf[16 + n] == 0, "missing terminator after %d codes", n);
    }
    if (out != NULL) memcpy(out, buf + 16, max);
    return n;
}

int main(int argc, char** argv) {
    u8 out[600];
    s32 n;
    s32 max;

    // The case of the audit: "Ã" needs two codes and only one is left.
    n = Encode("ABÃ", out, 3);
    CHECK(n == 2 && out[0] == 'A' && out[1] == 'B', "two-code letter cut at the end: n=%d", n);
    n = Encode("Ã", out, 1);
    CHECK(n == 0, "two-code letter with one slot: n=%d", n);
    n = Encode("Ã", out, 2);
    CHECK(n == 2 && out[0] == 'A' && out[1] == QM_TILDE, "Ã with room");
    // Every size from 0 to the full length, for a text full of two-code letters.
    for (max = 0; max <= 64; max++) {
        Encode("Ação, Missões, Região: Índio Ópera Úmido Ü í ó ú õ ã ü", NULL, max);
    }
    CHECK(QM_Encode("abc", out, 0) == 0, "max 0");
    CHECK(QM_Encode(NULL, out, 4) == 0 && out[0] == 0, "NULL text");
    CHECK(QM_Encode("abc", NULL, 4) == 0, "NULL output");

    // Portuguese letters.
    n = Encode("àáâãçéêíóôõúü ÀÁÂÃÇÉÊÍÓÔÕÚÜ", out, 100);
    {
        static const u8 expect[] = { 0x90, 0x91, 0x92, 'a', QM_TILDE, 0x94, 0x96, 0x97, QM_DOTLESS_I, QM_ACUTE,
                                     'o', QM_ACUTE, 0x9A, 'o', QM_TILDE, 'u', QM_ACUTE, 'u', QM_DIAERESIS, ' ',
                                     0x80, 'A', QM_ACUTE, 0x82, 'A', QM_TILDE, 0x84, 0x86, 0x87, 'I', QM_ACUTE,
                                     'O', QM_ACUTE, 0x8A, 'O', QM_TILDE, 'U', QM_ACUTE, 'U', QM_DIAERESIS };
        CHECK(n == (s32)sizeof(expect) && memcmp(out, expect, sizeof(expect)) == 0, "Portuguese letters (n=%d)", n);
    }
    // Button glyphs.
    n = Encode("\x01\x05\x06\x07", out, 10);
    CHECK(n == 4 && out[0] == 0xA5 && out[1] == 0xA0 && out[2] == 0xA2 && out[3] == 0xA3, "glyphs");
    // Broken UTF-8: a lead byte at the end, a lone continuation byte, a 3-byte sequence, an overlong lead byte.
    n = Encode("a\xC3", out, 10);
    CHECK(n == 2 && out[0] == 'a' && out[1] == '?', "lead byte at the end");
    n = Encode("\x80z", out, 10);
    CHECK(n == 2 && out[0] == '?' && out[1] == 'z', "lone continuation byte");
    n = Encode("\xE2\x80\x94z", out, 10);
    CHECK(n == 2 && out[0] == '?' && out[1] == 'z', "3-byte sequence (em dash)");
    n = Encode("\xC0\xA0z", out, 10);
    CHECK(n == 2 && out[0] == '?' && out[1] == 'z', "overlong sequence n=%d", n);
    n = Encode("\xE2\x80", out, 10);
    CHECK(n == 1 && out[0] == '?', "cut 3-byte sequence at the end");

    // Every text of the quest tables (one per line in the file given), with the largest buffer used for it.
    if (argc > 1) {
        FILE* f = fopen(argv[1], "r");
        char line[2048];
        int texts = 0;
        int cut = 0;

        if (f == NULL) {
            printf("FAIL cannot open %s\n", argv[1]);
            return 1;
        }
        while (fgets(line, sizeof(line), f)) {
            s32 len;
            line[strcspn(line, "\n")] = 0;
            texts++;
            len = Encode(line, out, 320);
            // Names and short texts are drawn through 128/160 code buffers, descriptions through 320.
            if (len >= 320 || (strlen(line) < 120 && len >= 128)) {
                cut++;
                printf("CUT  %s\n", line);
            }
        }
        fclose(f);
        printf("%d texts of questmenu_data.c encoded, %d would be cut\n", texts, cut);
        CHECK(cut == 0, "texts cut");
    }

    printf(failures == 0 ? "ALL TESTS PASSED\n" : "%d TEST(S) FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
