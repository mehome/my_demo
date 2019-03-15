#if defined(CONFIG_ASP_VEP) || defined(CONFIG_ASP_VEP_OPT)
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#define READER_NEEDS_SIZES

#include "spf-postapi.h"
#include "cmdlutils.h"

void
report_error(const char *f, ...)
{
/*    va_list ap;

    err_count++;

    if (quiet)
    {
        if (minority_report == 0)
        {
#if defined (_WIN32) && !defined(__GNUC__)
            if (int_on_error)
                __asm int 3;
            else
#endif
                error_exit(-4);
        }
        return;
    }

    fprintf(stderr,"ERROR: ");
    va_start(ap,f);
    vfprintf(stderr,f,ap);
    va_end(ap);

    if (minority_report == 0)
    {
        fprintf(stderr,"Exiting...\n");
        delete_outputs_on_error();
#if defined (_WIN32) && !defined(__GNUC__)
        if (int_on_error)
            __asm int 3;
        else
#endif
            error_exit(-4);
    }*/
}

void
report_warning(const char *f, ...)
{
/*    va_list ap;

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__GNUC__)
    if (int_on_error == 2)
        __asm int 3;
#endif
    if (quiet)
        return;

    fprintf(stderr,"WARNING: ");
    va_start(ap,f);
    vfprintf(stderr,f,ap);
    va_end(ap);
*/
}

static int sample_global = 0;

int
get_sample_rate_global(void)
{
    return sample_global;
}

void set_sample_rate_global(int s)
{
    sample_global = s;
}



static PROFILE_TYPE(t) profile;

PROFILE_TYPE(t) *
base_profile()
{
    return &profile;
}

void 
init_profile(void)
{
    memset(&profile, 0, sizeof(profile));
}

static uitem_t
update_crc16(uitem_t *pcBlock, uitem_t len, uitem_t crc)
{
    //	unsigned short crc = 0xFFFF;
    int i;
    while (len--)
    {
        crc ^= *pcBlock++;// << 8;

        for (i = 0; i < 16; i++)
            crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }

    return crc;
}


void
set_dump_adm_flag(void *b)
{
    size_t **member = (size_t **)&profile;
    pitem_t *mem = (pitem_t *)b;
    uitem_t crc = 0xffff;

    profile.p_gen->flags |= 1 << GENERAL_DUMP_MIXER_BIT;
    crc = update_crc16((uitem_t *)mem + 4, mem[3] - 1 - 4, crc);
#if 0  // __PANTHER__
    (uitem_t)mem[mem[3] - 1] = crc;
#endif

}

void
assign_profile(void *b)
{
    int i = 0;
    size_t **member = (size_t **)&profile;
    pitem_t *mem = (pitem_t *)b;
    int num_ena;
    uitem_t crc = 0xffff;

    crc = update_crc16((uitem_t *)mem + 4, mem[3] - 1 - 4, crc);

    if (crc != (uitem_t)mem[mem[3]-1])
    {
        report_error("CRC does not match\n");
        return;
    }

 // clear profile entries ->
    init_profile();
    num_ena = mem[1];
    // read profile entries:
    for (i = 0; i < num_ena; i++)
    {
        pitem_t id = mem[4 + i * 2];
        pitem_t pos = mem[4 + i * 2 + 1];
        member[id] = (size_t *)&mem[pos];
    }


    if (mem[2])
    {
        char *x = (char *)mem;
        report_warning("\nProfile has some extra information: (...later)\n");
    }

}

void *
read_profile(char *name)
{
    struct stat sb;
    int len;
    void *buf;
    FILE *fd;

    len = stat(name, &sb);
    if (len)
        return 0;

    len = sb.st_size;

    buf = malloc(len + 1);

    if (!buf)
        return 0;

    fd = fopen(name, "rb");

    if (fread(buf, 1, len, fd) != len)
    {
        report_error("Cannot read file %s", name);
        return 0;
    }

    assign_profile(buf);

    return buf;
}


void
extra_profile_check(void)
{

    PROFILE_TYPE(t) *prof = base_profile();

    //check if sampling rate is Ok.

    if (!get_sample_rate_global())
        set_sample_rate_global(prof->p_gen->sr);

    if (get_sample_rate_global() != prof->p_gen->sr)
    {
        report_error("Sampling rate does not match!\n"
            "File's sample rate is %d, profile's one %d.\n",
            get_sample_rate_global(), prof->p_gen->sr);
        return;

    }

}

/*
void extra_profile_afl_check(void)
{

    PROFILE_TYPE(t) *prof = base_profile();

    

    if (prof->p_ch1_af[1] && prof->p_ch2_af[2])
    {
        if (prof->p_ch1_af[1]->afl1 != prof->p_ch2_af[2]->afl1)
        {
            report_error("AFL1 in AF#1 in ch#1 should be equal to AFL1 in AF#2 in ch#2\n",
                "AFL1 in AF1CH1 = %d and AFL1 in AF2CH2 = %d\n",
                prof->p_ch1_af[1]->afl1, prof->p_ch2_af[2]->afl1);
            return;

        }
        if (prof->p_ch1_af[1]->afl2 != prof->p_ch2_af[2]->afl2)
        {
            report_error("AFL2 in AF#1 in ch#1 should be equal to AFL2 in AF#2 in ch#2\n",
                "AFL1 in AF1CH1 = %d and AFL1 in AF2CH2 = %d\n",
                prof->p_ch1_af[1]->afl2, prof->p_ch2_af[2]->afl2);
            return;

        }

    }

    
    if (prof->p_ch1_af[2] && prof->p_ch2_af[1])
    {
        if (prof->p_ch1_af[2]->afl1 != prof->p_ch2_af[1]->afl1)
        {
            report_error("AFL1 in AF#2 in ch#1 should be equal to AFL1 in AF#1 in ch#2\n",
                "AFL1 in AF2CH1 = %d and AFL1 in AF1CH2 = %d\n",
                prof->p_ch1_af[2]->afl1, prof->p_ch2_af[1]->afl1);
            return;

        }
        if (prof->p_ch1_af[2]->afl2 != prof->p_ch2_af[1]->afl2)
        {
            report_error("AFL2 in AF#2 in ch#1 should be equal to AFL2 in AF#1 in ch#2\n",
                "AFL1 in AF2CH1 = %d and AFL1 in AF1CH2 = %d\n",
                prof->p_ch1_af[2]->afl2, prof->p_ch2_af[1]->afl2);
            return;
        }
    }


    

    if (prof->p_ch1_sf[0] && prof->p_ch2_sf[1])
    {
        if (prof->p_ch1_sf[0]->sfl != prof->p_ch2_sf[1]->sfl)
        {
            report_error("SFL in SF#1 in ch#1 should be equal to SFL in SF#2 in ch#2\n",
                "SFL in SF1CH1 = %d and SFL in SF2CH2 = %d\n",
                prof->p_ch1_sf[0]->sfl, prof->p_ch2_sf[1]->sfl);
            return;

        }
    }

    if (prof->p_ch1_sf[1] && prof->p_ch2_sf[0])
    {
        if (prof->p_ch1_sf[1]->sfl != prof->p_ch2_sf[0]->sfl)
        {
            report_error("SFL in SF#2 in ch#1 should be equal to SFL in SF#1 in ch#2\n",
                "SFL in SF2CH1 = %d and SFL in SF1CH2 = %d\n",
                prof->p_ch1_sf[1]->sfl, prof->p_ch2_sf[0]->sfl);
            return;
        }
    }



}
*/
#endif
