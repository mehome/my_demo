#ifdef CONFIG_CLK_32K_CAL
#include <common.h>
#include <lib.h>
//#define GPIO_INDICATE_RESULT

int do_cal(int argc, char *argv[])
{
    int interval;
    unsigned long regs;

    if(argc == 1)
    {
        interval = atoi(argv[0]);
        printf("32k calibrate for %d(ms) interval\n", interval);

        *(volatile unsigned long *)0xbf004808 = 0xfeUL;
        *(volatile unsigned long *)0xbf004804 = 0xf5UL;
        *(volatile unsigned long *)0xbf004814 = 0x01100000UL;
#ifdef GPIO_INDICATE_RESULT
        *(volatile unsigned long *)0xbf00483c = 0xffffffffUL;
#endif
        mdelay(interval);
        regs = *(volatile unsigned long *)0xbf004814;
#ifdef GPIO_INDICATE_RESULT
        *(volatile unsigned long *)0xbf00483c = 0;
#endif
        regs = 0x01100000UL - regs;

        printf("finish %ld (ticks)\n", regs);
    }
    else
        goto err;

    return 0;
err:
    return ERR_HELP;
}

cmdt cmdt_clk_32k[] __attribute__ ((section("cmdt"))) =
{
    { "clk_32k", do_cal, "clk_32k [interval (ms)]\n"} ,
};
#endif
