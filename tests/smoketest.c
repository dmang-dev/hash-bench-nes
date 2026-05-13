/* smoketest — strip everything down to "does cc65 conio render on FCEUX?"
**
** Build via   build-smoketest.bat   (which links *only* this file, not the
** full algo set).  If this displays text and the counter ticks, the
** toolchain and emulator are fine and the bug is somewhere in the bench
** harness.  If even this is a black screen, we have an init/emulator
** problem to chase down first.
*/
#include <stdint.h>
#include <conio.h>
#include <time.h>
#include <joystick.h>
#include <nes.h>

int main(void) {
    uint16_t counter = 0u;
    clock_t  last;

    joy_install(joy_static_stddrv);
    (void)textcolor(COLOR_WHITE);
    (void)bgcolor(COLOR_BLACK);
    (void)bordercolor(COLOR_BLACK);

    clrscr();
    gotoxy(0, 0); cputs("hash-bench-nes SMOKETEST");
    gotoxy(0, 2); cputs("if you see this, conio works.");
    gotoxy(0, 3); cputs("counter ticks once / second:");

    last = clock();
    for (;;) {
        clock_t now = clock();
        if (now - last >= CLOCKS_PER_SEC) {
            last = now;
            counter++;
            gotoxy(0, 5);
            cprintf("count = %u  clock = %lu  ", counter, (unsigned long)now);
        }
        /* press any button to clear and restart */
        if (joy_read(JOY_1)) {
            counter = 0u;
            clrscr();
            gotoxy(0, 0); cputs("RESET");
            last = clock();
        }
    }
}
