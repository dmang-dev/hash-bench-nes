/*
 * hash-bench-nes — NES (Famicom) port of the multi-platform hash
 * benchmark. cc65 toolchain, NROM mapper (mapper 0), 32 KB PRG +
 * 8 KB CHR.
 *
 * V2: simpler structure to debug a hang seen in FCEUX after a few
 * algorithms.  The previous version used a function-pointer table
 * with `(hash_fn_t)` casts across diverging digest-array signatures
 * and rendered a "RUN…" preview row before each benchmark.  Either
 * the cast or the live preview (or some interaction with cc65's
 * 768-byte parameter stack and the PPU vblank-flushed write buffer)
 * killed the run.  This rewrite avoids both:
 *
 *   - Each algorithm is invoked by name in a switch — no function
 *     pointers, no casts.  cc65 inlines the call straight to the
 *     fastcall entry point with no extra glue.
 *
 *   - All benchmarks run first, then results render in one pass.
 *     The PPU write buffer ($0200-$0500, 768 B) is touched at most
 *     ~30 chars per algo at the very end; NMI flushes it between
 *     iterations of the render loop, with no intermediate prints
 *     racing the long-running uint32_t algorithms.
 *
 *   - `digest[]` is a file-scope static (BSS in WRAM at $6000+),
 *     not a function local — keeps the per-call cc65 parameter
 *     stack tiny.
 *
 * Why CLOCKS_PER_SEC=50 on NES (per I:\cc65\include\time.h):  cc65
 * NES nes.lib increments the clock from its NMI hook.  At NTSC
 * 60 Hz that's 60 ticks/sec, but the header still defines 50; the
 * value is informational only and we display ms = ticks*20 to match.
 *
 * Controls:
 *   any button : rerun the entire sweep
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <joystick.h>
#include <nes.h>

#include "hashes.h"

/* Tier classification — '.' / '*' / '#' is the visible marker since
** cc65's NES conio palette is constrained to 4 background colors and
** textcolor() doesn't reliably remap them per-character. */
#define TIER_CHK  0u
#define TIER_NC   1u
#define TIER_CR   2u

/* Bench budget. CLOCKS_PER_SEC=50 nominally; 25 ticks ≈ 500 ms. */
#define BUDGET_TICKS 25u

/* Hard cap on per-algo iterations to keep a stuck clock() from running
** the bench forever — even the cheapest algo (CRC-8 on 64 B) tops
** out around a few hundred iters/500ms here. */
#define ITER_CAP 9999u

/* Workload buffer + digest staging — both in BSS at $6000+. */
static uint8_t buf[BENCH_BUF_LEN];
static uint8_t digest[HASH_MAX_DIGEST];

/* Rolling result for one algorithm. Kept tiny — 6 bytes per entry —
** so the whole results[] table stays well under 256 bytes even with
** the full algo set. */
typedef struct {
    uint16_t iters;
    uint16_t ticks;
    uint8_t  digest0;        /* first byte of the digest (sanity ID) */
    uint8_t  tier;
} algo_result;

#define NUM_ALGOS 18u
static algo_result results[NUM_ALGOS];

/* Six-character labels — one per algo, kept in RODATA. */
static const char * const ALGO_LABEL[NUM_ALGOS] = {
    "CRC8 ", "CRC16", "CRC32", "ADL32", "FLT16", "FLT32", "PRSN8",
    "KNUTH", "OAT  ", "PJW  ", "SDBM ", "DJB2 ", "FNV1A", "MMUR3",
    "XXH32", "MD4  ", "MD5  ", "SHA1 "
};

/* Tier per algo — same indexing as above. */
static const uint8_t ALGO_TIER[NUM_ALGOS] = {
    TIER_CHK, TIER_CHK, TIER_CHK, TIER_CHK, TIER_CHK, TIER_CHK, TIER_CHK,
    TIER_NC,  TIER_NC,  TIER_NC,  TIER_NC,  TIER_NC,  TIER_NC,  TIER_NC,
    TIER_NC,
    TIER_CR,  TIER_CR,  TIER_CR
};

/* Same byte pattern as every other port. */
static void fill_buffer(void) {
    uint16_t i;
    for (i = 0; i < BENCH_BUF_LEN; i++) {
        buf[i] = (uint8_t)((i * 31u + 7u) & 0xFFu);
    }
}

/* Direct-call dispatch — no function pointers. cc65 emits a normal
** call straight to the fastcall entry, which is what we want.  The
** previous version's `(hash_fn_t)` cast through a wider signature
** generated unreliable code on a few algos. */
static void invoke(uint8_t idx) {
    switch (idx) {
        case  0: hash_crc8       (buf, BENCH_BUF_LEN, digest); break;
        case  1: hash_crc16      (buf, BENCH_BUF_LEN, digest); break;
        case  2: hash_crc32      (buf, BENCH_BUF_LEN, digest); break;
        case  3: hash_adler32    (buf, BENCH_BUF_LEN, digest); break;
        case  4: hash_fletcher16 (buf, BENCH_BUF_LEN, digest); break;
        case  5: hash_fletcher32 (buf, BENCH_BUF_LEN, digest); break;
        case  6: hash_pearson    (buf, BENCH_BUF_LEN, digest); break;
        case  7: hash_knuth      (buf, BENCH_BUF_LEN, digest); break;
        case  8: hash_jenkins_oat(buf, BENCH_BUF_LEN, digest); break;
        case  9: hash_pjw_elf    (buf, BENCH_BUF_LEN, digest); break;
        case 10: hash_sdbm       (buf, BENCH_BUF_LEN, digest); break;
        case 11: hash_djb2       (buf, BENCH_BUF_LEN, digest); break;
        case 12: hash_fnv1a32    (buf, BENCH_BUF_LEN, digest); break;
        case 13: hash_murmur3    (buf, BENCH_BUF_LEN, digest); break;
        case 14: hash_xxh32      (buf, BENCH_BUF_LEN, digest); break;
        case 15: hash_md4        (buf, BENCH_BUF_LEN, digest); break;
        case 16: hash_md5        (buf, BENCH_BUF_LEN, digest); break;
        case 17: hash_sha1       (buf, BENCH_BUF_LEN, digest); break;
        default: break;
    }
}

/* Run one algorithm until BUDGET_TICKS have elapsed (or ITER_CAP hits). */
static void run_one(uint8_t idx) {
    clock_t  t0, dt;
    uint16_t iters = 0u;

    t0 = clock();
    do {
        invoke(idx);
        iters++;
        if (iters >= ITER_CAP) break;
        dt = clock() - t0;
    } while ((uint16_t)dt < BUDGET_TICKS);

    results[idx].iters   = iters;
    results[idx].ticks   = (uint16_t)dt;
    results[idx].digest0 = digest[0];
    results[idx].tier    = ALGO_TIER[idx];
}

/* Render one row.  `slot` is the screen Y position (0-based within
** the data area); `idx` indexes into results[] / ALGO_LABEL[]. */
static void draw_row(uint8_t slot, uint8_t idx) {
    uint16_t          ms_per_iter;
    const algo_result *r = &results[idx];
    char               mark = '.';

    if (r->tier == TIER_NC) mark = '*';
    else if (r->tier == TIER_CR) mark = '#';

    /* Header row offset = 4. Row Y = 4 + slot. */
    gotoxy(0, (unsigned char)(4u + slot));
    cputc(mark);
    cputc(' ');
    cprintf("%s ", ALGO_LABEL[idx]);

    if (r->iters == 0u) {
        cputs("    ---       --");
        return;
    }

    /* ms_per_iter = ticks * 20 / iters; safe in uint32_t. */
    ms_per_iter = (uint16_t)(((uint32_t)r->ticks * 20uL) / r->iters);
    cprintf("%5u %5u %02X",
            (unsigned)r->iters,
            (unsigned)ms_per_iter,
            (unsigned)r->digest0);
}

static void render_all(void) {
    uint8_t i;

    clrscr();
    gotoxy(0, 0);
    cputs("hash-bench-nes v2");
    gotoxy(0, 1);
    cputs("64B  budget=500ms");
    gotoxy(0, 2);
    cputs("ALGO    ITER MS/IT H");
    gotoxy(0, 3);
    cputs("--------------------");

    for (i = 0; i < NUM_ALGOS; i++) {
        draw_row(i, i);
    }

    gotoxy(0, (unsigned char)(4u + NUM_ALGOS + 1u));
    cputs("any btn: rerun");
}

/* Wait for release-then-press of any joystick button. */
static void wait_press(void) {
    uint8_t v;
    do { v = joy_read(JOY_1); } while (v != 0u);
    do { v = joy_read(JOY_1); } while (v == 0u);
}

int main(void) {
    uint8_t i;

    joy_install(joy_static_stddrv);
    (void)textcolor(COLOR_WHITE);
    (void)bgcolor(COLOR_BLACK);
    (void)bordercolor(COLOR_BLACK);

    fill_buffer();

    clrscr();
    cputs("hash-bench-nes v2");

    for (;;) {
        for (i = 0; i < NUM_ALGOS; i++) {
            /* Show which algo is currently running.  If the screen
            ** freezes on N/18 the user knows ALGOS[N] is the culprit
            ** — much easier to triage than a blank hang. */
            gotoxy(0, 1);
            cprintf("running %2u/%2u %s    ",
                    (unsigned)(i + 1u), (unsigned)NUM_ALGOS,
                    ALGO_LABEL[i]);

            results[i].iters = 0u;
            results[i].ticks = 0u;
            run_one(i);
        }
        render_all();
        wait_press();
    }
    /* not reached */
}
