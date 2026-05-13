/* smoketest2 — copy of main.c's init flow, but with a no-op bench
** body. If this displays "running 1/3 ..." through "running 3/3 ..."
** rotating once a second, then the issue is in run_one() / the algos.
** If this is *also* a black screen, the issue is in the init flow
** itself (joy_install + textcolor/bg/border + clrscr + the for-loop
** structure).
*/
#include <stdint.h>
#include <conio.h>
#include <time.h>
#include <joystick.h>
#include <nes.h>

static const char * const LABELS[3] = { "FAKE-A", "FAKE-B", "FAKE-C" };

static void fake_run(void) {
    /* Burn ~500 ms by spinning on clock(). No 32-bit math, no algo
    ** runtime — if this fails to advance, the bug is in the spin
    ** loop / clock() reading, not any specific hash. */
    clock_t t0 = clock();
    while (clock() - t0 < 25u) { /* nothing */ }
}

int main(void) {
    uint8_t i;

    joy_install(joy_static_stddrv);
    (void)textcolor(COLOR_WHITE);
    (void)bgcolor(COLOR_BLACK);
    (void)bordercolor(COLOR_BLACK);

    clrscr();
    cputs("hash-bench-nes SMOKE2");

    for (;;) {
        for (i = 0; i < 3u; i++) {
            gotoxy(0, 1);
            cprintf("running %u/3 %s     ",
                    (unsigned)(i + 1u), LABELS[i]);
            fake_run();
        }
        gotoxy(0, 3);
        cputs("press any btn to loop");
        while (joy_read(JOY_1) == 0u) { /* wait */ }
        gotoxy(0, 3);
        cputs("                     ");
        while (joy_read(JOY_1) != 0u) { /* drain */ }
    }
}
