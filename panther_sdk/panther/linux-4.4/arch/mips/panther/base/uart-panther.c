#include <asm/mach-panther/panther.h>

void putc(char c)
{
    int panther_console_index = 0;
    volatile int *p = (unsigned int *) (UR_BASE + (0x100 * panther_console_index));

    while (p[URCS>>2] & URCS_TF)
        ;

    p[URBR>>2] = (int)c<<URBR_DTSHFT;
}
