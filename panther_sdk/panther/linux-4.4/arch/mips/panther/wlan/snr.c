#ifndef __GENERATE_SNR_TABLE__

#define USE_SNR_TABLE
#include "snr.h"

int link_quality_b[8] =
{
    -6,
    -5,
    -4,
    0,
    -1,
    2,
    5,
    8,
};

int snr_calc(unsigned char snr, unsigned char part1, unsigned char part2)
{
    unsigned long _snr = part1_to_x[part1] / part2_to_y[part2];
    int lo = 0;
    int hi = LOG_TBL_SIZE - 1;
    int next_idx = LOG_TBL_SIZE / 2;
    int idx;

    if((part1==0)&&(part2==0))
    {
        return link_quality_b[(snr & 0xf) - 8];
    }

    do
    {
        idx = next_idx;
        //printf(" %d %d %d %ld %ld\n", hi, lo, idx, _snr, table_10_log10[idx]);
        if (_snr >= table_10_log10[idx])
        {
            lo = idx;
            next_idx = (idx + hi) >> 1;
        }
        else
        {
            hi = idx;
            next_idx = (idx + lo) >> 1;
        }
    } while (next_idx!=idx);

    //printf("====> idx %d\n", idx);
    return idx;
}

#else

// gcc -D__GENERATE_SNR_TABLE__ -o snr snr.c -lm
// ./snr > ./include/snr.h
// ./snr test (for testing)
/*
 X = SNR_part1[7:0]/1024
 Y = (SNR_part2[7:5]+8)/64 *2^(- SNR_part2[4:0])
 SNR_dB = 10*log10( 0.5*(X/Y-1) )
 snr_x = (double)rx_phy_info_storage[idx].snr_p1 / 1024;
 snr_y = ((double)(((rx_phy_info_storage[idx].snr_p2 >> 5) & 0x07) + 8)/64) * pow(2, -1*(rx_phy_info_storage[idx].snr_p2 & 0x1F));
 snr_db = 10*log10(0.5 * (snr_x/snr_y - 1));
*/

#include <stdio.h>
#include <math.h>

#define LOG_TBL_SIZE 64
unsigned long part1_to_x[256];
unsigned long part2_to_y[256];
unsigned long table_10_log10[LOG_TBL_SIZE];

void snr_calc(unsigned long x, unsigned long y)
{
    unsigned long _snr = x / y;
    int lo = 0;
    int hi = LOG_TBL_SIZE - 1;
    int next_idx = LOG_TBL_SIZE / 2;
    int idx;
    do
    {
        idx = next_idx;
        printf(" %d %d %d %ld %ld\n", hi, lo, idx, _snr, table_10_log10[idx]);
        if (_snr >= table_10_log10[idx])
        {
            lo = idx;
            next_idx = (idx + hi) >> 1;
        }
        else
        {
            hi = idx;
            next_idx = (idx + lo) >> 1;
        }
    } while (next_idx!=idx);

    printf("====> idx %d\n", idx);
}
void snr_db(unsigned char part1, unsigned char part2)
{
    double x = part1_to_x[part1];
    double y = part2_to_y[part2];
    double snr;
    double snr_x, snr_y, snr_db;

    snr = 10*log10(x/y);
    snr_calc(x,y);
    printf("%x %x %f %f %f\n", part1, part2, x, y, snr);

    snr_x = (double)part1 / 1024;
    snr_y = ((double)(((part2 >> 5) & 0x07) + 8)/64) * pow(2, -1*(part2 & 0x1F));
    snr_db = 10*log10(0.5 * (snr_x/snr_y - 1));
    printf("%f %f %f\n", snr_x, snr_y, snr_db);
}
int main(int argc, char **argv)
{
    int i;
    double x;
    double y;
    for (i=0;i<256;i++)
    {
        x = (double)i / 1024;
        y = ((double)(((i >> 5) & 0x07) + 8)/64) * pow(2, -1*(i & 0x1F));
        //printf("%d %f %f %f %f\n", i, x, y, x *180000*100000, y*180000*100000);
        //printf("* %8.8f %8.8f *\n", 0xffffffff/(x *180000*100000),0xffffffff/(y*180000*100000));
        part1_to_x[i] = x *  90000 * 100000;
        part2_to_y[i] = y * 180000 * 100000;
        //printf("%d %08x %08x %8.8f %8.8f\n", i, part1_to_x[i], part2_to_y[i], x, y);
    }

    for (i=0;i<64;i++)
    {
        //printf("%d %f\n", i, pow((double)10, (double)i/10));
        table_10_log10[i] = pow((double)10, (double)i/10);
    }

    if (argc >= 2)
    {
        snr_db(0x31,0x8f);
    }
    else
    {
        printf("#ifndef __DRAONITE_SNR_H__\n");
        printf("#define __DRAONITE_SNR_H__\n");

        printf("#define LOG_TBL_SIZE %d\n", LOG_TBL_SIZE);

        printf("int snr_calc(unsigned char part1, unsigned char part2);\n");

        printf("#if defined(USE_SNR_TABLE)\n");
        printf("unsigned long part1_to_x[256] = {\n");
        for (i=0;i<256;i++)
            printf("0x%08lx,\n", part1_to_x[i]);
        printf("};\n");

        printf("unsigned long part2_to_y[256] = {\n");
        for (i=0;i<256;i++)
            printf("0x%08lx,\n", part2_to_y[i]);
        printf("};\n");

        printf("unsigned long table_10_log10[LOG_TBL_SIZE] = {\n");
        for (i=0;i<LOG_TBL_SIZE;i++)
            printf("0x%08lx,\n", table_10_log10[i]);
        printf("};\n");
        printf("#endif\n");

        printf("#endif //__DRAONITE_SNR_H__\n");
    }

    return 0;
}
#endif

