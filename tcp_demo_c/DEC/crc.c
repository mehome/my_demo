#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#define		CRC_START_16		0x0000
#define		CRC_POLY_16		0xA001

typedef unsigned int uint16;

static void init_crc16_tab( void );
static char crc_tab16_init = 0;
static uint16 crc_tab16[256];
extern void CRC16_Str(const unsigned char *input_str, int num_bytes,unsigned char *output_str);

/* * static void init_crc16_tab( void ); 
* * For optimal performance uses the CRC16 routine a lookup table with values 
* that can be used directly in the XOR arithmetic in the algorithm. This 
* lookup table is calculated by the init_crc16_tab() routine, the first time 
* the CRC function is called. */
static void init_crc16_tab( void ) 
{	
	uint16 i;	
	uint16 j;
	uint16 crc;	
	uint16 c;	
	for (i=0; i<256; i++) {
		crc = 0;		
	c   = i;		
	for (j=0; j<8; j++) 
	{			
		if ( (crc ^ c) & 0x0001 ) 
			crc = ( crc >> 1 ) ^ CRC_POLY_16;			
		else                      
			crc =   crc >> 1;			
		c = c >> 1;		
	}		
	crc_tab16[i] = crc;	
	}	
	crc_tab16_init = 1;
}

/* * uint16_t crc_16( const unsigned char *input_str, size_t num_bytes ); 
* * The function crc_16() calculates the 16 bits CRC16 in one pass for a byte 
* string of which the beginning has been passed to the function. The number of 
* bytes to check is also a parameter. The number of the bytes in the string is 
* limited by the constant SIZE_MAX. */
uint16 CRC16( const unsigned char *input_str, int num_bytes ) 
{	
	uint16 crc;	
	const unsigned char *ptr;	
	int a;	
	if ( ! crc_tab16_init ) init_crc16_tab();	
	crc = CRC_START_16;	
	ptr = input_str;	
	if ( ptr != NULL ) 
		for (a=0; a<num_bytes; a++) 
		{		
			crc = (crc >> 8) ^ crc_tab16[ (crc ^ (uint16) *ptr++) & 0x00FF ];	
		}	
	return crc;
}  /* crc_16 */



/*
void main(const int argc,const char *argv[])
{
	char a[]="cmd_typ=log_crpy,cmd_id=00000000000000000_00000002,cryp=ad3191f456be685805267764998ff29dec4d";
	uint16 res = 0;
	res = CRC16(a,strlen(a));
	printf("res=%d\n",res);

	return ;
}

*/

