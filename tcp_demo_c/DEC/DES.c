
//LINUX下编译命令：g++ -g -o Des DES.cpp 32位应用
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
typedef unsigned char uint8;
typedef unsigned long long uint16;

//CRC
#define		CRC_START_16		0x0000
#define		CRC_POLY_16		0xA001

static void init_crc16_tab( void );
static char crc_tab16_init = 0;
static uint16 crc_tab16[256];

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
//CRC END

uint8 IP[64] = {
	58 , 50 , 42 , 34 , 26 , 18 , 10 ,  2 ,
	60 , 52 , 44 , 36 , 28 , 20 , 12 ,  4 ,
	62 , 54 , 46 , 38 , 30 , 22 , 14 ,  6 ,
	64 , 56 , 48 , 40 , 32 , 24 , 16 ,  8 ,
	57 , 49 , 41 , 33 , 25 , 17 ,  9 ,  1 ,
	59 , 51 , 43 , 35 , 27 , 19 , 11 ,  3 ,
	61 , 53 , 45 , 37 , 29 , 21 , 13 ,  5 ,
	63 , 55 , 47 , 39 , 31 , 23 , 15 ,  7 };
	
uint8 FP[64] = {
	40 ,  8 , 48 , 16 , 56 , 24 , 64 , 32 ,
	39 ,  7 , 47 , 15 , 55 , 23 , 63 , 31 ,
	38 ,  6 , 46 , 14 , 54 , 22 , 62 , 30 ,
	37 ,  5 , 45 , 13 , 53 , 21 , 61 , 29 ,
	36 ,  4 , 44 , 12 , 52 , 20 , 60 , 28 ,
	35 ,  3 , 43 , 11 , 51 , 19 , 59 , 27 ,
	34 ,  2 , 42 , 10 , 50 , 18 , 58 , 26 ,
	33 ,  1 , 41 ,  9 , 49 , 17 , 57 , 25 };
	
uint8 KM[16] = {
	1 ,  1 ,  2 ,  2 ,  2 ,  2 ,  2 ,  2 ,  1 ,  2 ,  2 ,  2 ,  2 ,  2 ,  2 ,  1 };
	
uint8 PC1[56] = {
	57 , 49 , 41 , 33 , 25 , 17 ,  9 ,
	1 , 58 , 50 , 42 , 34 , 26 , 18 ,
	10 ,  2 , 59 , 51 , 43 , 35 , 27 ,
	19 , 11 ,  3 , 60 , 52 , 44 , 36 ,
	63 , 55 , 47 , 39 , 31 , 23 , 15 ,
	7 , 62 , 54 , 46 , 38 , 30 , 22 ,
	14 ,  6 , 61 , 53 , 45 , 37 , 29 ,
	21 , 13 ,  5 , 28 , 20 , 12 ,  4
};
  
uint8 PC2[48] = {
	14 , 17 , 11 , 24 ,  1 ,  5 ,  3 , 28 ,
	15 ,  6 , 21 , 10 , 23 , 19 , 12 ,  4 ,
	26 ,  8 , 16 ,  7 , 27 , 20 , 13 ,  2 ,
	41 , 52 , 31 , 37 , 47 , 55 , 30 , 40 ,
	51 , 45 , 33 , 48 , 44 , 49 , 39 , 56 ,
	34 , 53 , 46 , 42 , 50 , 36 , 29 , 32 };

uint8	E[48] = {
	32 ,  1 ,  2 ,  3 ,  4 ,  5 ,  4 ,  5 ,
	6 ,  7 ,  8 ,  9 ,  8 ,  9 , 10 , 11 ,
	12 , 13 , 12 , 13 , 14 , 15 , 16 , 17 ,
	16 , 17 , 18 , 19 , 20 , 21 , 20 , 21 ,
	22 , 23 , 24 , 25 , 24 , 25 , 26 , 27 ,
	28 , 29 , 28 , 29 , 30 , 31 , 32 ,  1 };


uint8	s_boxes[32][16] = {
	//S1     
		{ 14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7 },
		{ 0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8 },
		{ 4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0 },
		{ 15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13 },
	//S2  
		{ 15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10 },
		{ 3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5 },
		{ 0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15 },
		{ 13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9 },
	//S3  
		{ 10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8 },
		{ 13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1 },
		{ 13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7 },
		{ 1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12 },
	//S4  
		{ 7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15 },
		{ 13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9 },
		{ 10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4 },
		{ 3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14 },
	//S5  
		{ 2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9 },
		{ 14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6 },
		{ 4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14 },
		{ 11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3 },
	//S6  
		{ 12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11 },
		{ 10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8 },
		{ 9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6 },
		{ 4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13 },
	//S7  
		{ 4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1 },
		{ 13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6 },
		{ 1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2 },
		{ 6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12 },
	//S8  
		{ 13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7 },
		{ 1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2 },
		{ 7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8 },
		{ 2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11 }
};

uint8 PF[32] = {
	16 ,  7 , 20 , 21 , 29 , 12 , 28 , 17 ,
	1 , 15 , 23 , 26 ,  5 , 18 , 31 , 10 ,
	2 ,  8 , 24 , 14 , 32 , 27 ,  3 ,  9 ,
	19 , 13 , 30 ,  6 , 22 , 11 ,  4 , 25 };



void initial_p(int *plaint_text) 
{
	int tmp[64];
	int i;
	for (i = 0;i < 64;i++)
	{
		tmp[i] = plaint_text[IP[i] - 1];
	}	
	memcpy(plaint_text, tmp, sizeof(tmp));
}

void final_p(int *cipher_text) {
	int tmp[64];
	int i ;
	for (i= 0;i < 64;i++)
		tmp[i] = cipher_text[FP[i] - 1];
	memcpy(cipher_text, tmp, sizeof(tmp));
}

void extend_p(int *text, int *output) {
	int i ;
	for (i= 0;i < 48;i++)
		output[i] = text[E[i] - 1];
}
void permutation1(int *key, int *o) {
	//short tmp[56];
	int i;
	for ( i= 0;i < 56;i++)
		o[i] = key[PC1[i] - 1];
	//memcpy(output, tmp, sizeof(tmp));
}


void permutationP(int *text) {
	int tmp[32];
	int i ;
	for (i= 0;i < 32;i++) {
		tmp[i] = text[PF[i] - 1];
	}
	memcpy(text, tmp, sizeof(tmp));
}

void F_DES(int *R, int *key) {
	int R_48[48];

	int S_in[48];
	int S_out[32];
	int i;
	int j;
		extend_p(R, R_48);
	for ( i= 0;i < 48;i++) {
		S_in[i] = R_48[i] ^ key[i];
	}


	
	for ( i = 0;i < 8;i++) {
		int a = (S_in[i * 6] << 1) + S_in[i * 6 + 5];
		int b = (S_in[i * 6 + 1] << 3) + (S_in[i * 6 + 2] << 2) + (S_in[i * 6 + 3] << 1) + S_in[i * 6 + 4];
		int s = s_boxes[i * 4 + a][b];
		for (j = 0;j < 4;j++) {
			S_out[i * 4 + j] = (s >> (3 - j)) & 1;
		}
	}

	permutationP(S_out);
	memcpy(R, S_out, sizeof(S_out));

}

void Round(int *L, int *R, int *key) {
	int R_1[32];
	int i;
	for (i = 0;i < 32;i++) {
		R_1[i] = R[i];
	}
	F_DES(R_1, key);

	//L XOR R_1
	for ( i = 0;i < 32;i++) {
		R_1[i] ^= L[i];
	}


	for ( i = 0;i < 32;i++) {
		L[i] = R[i];
		R[i] = R_1[i];
	}
	

}
void setK(uint8 *keyC,int (*K)[56]) {

	int keyP[64];
	int key[56];
	int C[84], D[84];
	int i ;
	int j;
	int shift_len = 0;

	for (i= 0;i < 8;i++) {
		for (j = 0;j < 8;j++) {
			keyP[i * 8 + j] = (keyC[i] >> (7 - j)) & 1;
		}
	}

	permutation1(keyP, key);



	for ( i = 0;i < 28;i++) {
		C[56 + i] = C[28 + i] = C[i] = key[i];
		D[56 + i] = D[28 + i] = D[i] = key[28 + i];
	}

	
	for ( i = 0;i < 16;i++) {
		shift_len += KM[i];
		for (j = 0;j < 48;j++) {
			K[i][j] = (PC2[j]<28) ? C[PC2[j] - 1 + shift_len] : D[PC2[j] - 29 + shift_len];
		}
	}
}

void Enc(uint8 *txt, uint8 *enc,int (*K)[56]) {

	//
	int plain[64];
	int L[32], R[32];
	int i;
	int j;
	for ( i = 0;i < 8;i++) {
		for ( j = 0;j < 8;j++) {
			plain[i * 8 + j] = (txt[i] >> (7 - j)) & 1;
		}
	}

	initial_p(plain);


	for ( i = 0;i < 32;i++) {
		L[i] = plain[i];
		R[i] = plain[i + 32];
	}

	for ( i = 0;i < 16;i++) {
		Round(L, R, K[i]);
	}

	for ( i = 0;i < 32;i++) {
		plain[i] = R[i];
		plain[i + 32] = L[i];
	}

	final_p(plain);
	//for (int i = 0;i < 64;i++) {		cout << plain[i];	}	cout << endl;
	for ( i = 0;i < 8;i++)
		for ( j = 7;j >= 0;j--) {
			enc[i] |= plain[i * 8 + (7 - j)] << j;//+还是|？
		}


}
void Dec(uint8 *txt, uint8 *dec,int (*K)[56]) {


	int plain[64];
	int L[32], R[32];
	int i;
	int j;
	for ( i = 0;i < 8;i++) {
		for ( j = 0;j < 8;j++) {
			plain[i * 8 + j] = (txt[i] >> (7 - j)) & 1;//plain[i * 8 + j]保存0或者1
		}
	}
	initial_p(plain);


	for ( i = 0;i < 32;i++) {
		L[i] = plain[i];
		R[i] = plain[i + 32];
	}
	for ( i = 0;i < 16;i++) {
		Round(L, R, K[15 - i]);
	}

	for ( i = 0;i < 32;i++) {
		plain[i] = R[i];
		plain[i + 32] = L[i];
	}
	final_p(plain);

	for ( i = 0;i < 8;i++)
		for ( j = 7;j >= 0;j--) {
			dec[i] |= plain[i * 8 + (7 - j)] << j;
		}
	
}







int test_Enc48(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut,int (*K)[56]) 
{
	

	setK((uint8*)pi_ucKey1,K);
	Enc((uint8*)(pi_ucTxt+ 0), (uint8*)(pi_ucOut+ 0) ,K);
	Enc((uint8*)(pi_ucTxt+ 8), (uint8*)(pi_ucOut+ 8) ,K);
	Enc((uint8*)(pi_ucTxt+16), (uint8*)(pi_ucOut+16) ,K);
	Enc((uint8*)(pi_ucTxt+24), (uint8*)(pi_ucOut+24) ,K);
	Enc((uint8*)(pi_ucTxt+32), (uint8*)(pi_ucOut+32) ,K);
	Enc((uint8*)(pi_ucTxt+40), (uint8*)(pi_ucOut+40) ,K);



	return 1;
}


int test_Dec48(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut,int (*K)[56]) 
{
	

	setK((uint8*)pi_ucKey1,K);

	Dec((uint8*)(pi_ucTxt+ 0), (uint8*)(pi_ucOut+ 0) ,K);
	Dec((uint8*)(pi_ucTxt+ 8), (uint8*)(pi_ucOut+ 8) ,K);
	Dec((uint8*)(pi_ucTxt+16), (uint8*)(pi_ucOut+16) ,K);
	Dec((uint8*)(pi_ucTxt+24), (uint8*)(pi_ucOut+24) ,K);
	Dec((uint8*)(pi_ucTxt+32), (uint8*)(pi_ucOut+32) ,K);
	Dec((uint8*)(pi_ucTxt+40), (uint8*)(pi_ucOut+40) ,K);

	

	return 1;
}


void des3enc(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut,int (*K)[56]) 
{
//	int i;
	memset(pi_ucOut,0,48);
	test_Enc48(pi_ucTxt,pi_ucKey1,pi_ucOut,K);
//	for (i = 0;i < 48;i++)	printf("%02x,", (unsigned char )pi_ucOut[i] );
	
			

}

void des3dec(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut,int (*K)[56]) 
{

	memset(pi_ucOut,0,48);

	test_Dec48(pi_ucTxt,pi_ucKey1,pi_ucOut,K);
}


#if 0
//add tmp
int main()
{
	int iRet = 0;
	int i;
	unsigned char trans_buf[48*4]={1};
	unsigned char tmp_buf2[]= "cmd_typ=log_req,cmd_id=00000000000000000_00000000,meid=012345678912345,imsi=460001234567890,brand=huawei,hardver=v2.39,softver=v01.00,net=4,sig=31";
	int K_po[100][56];
	printf("Before Des3enc:%s\n",tmp_buf2);
	uint8 loginkey[]="lqw2xazzi1h9lqw2xazzi1h9";
	des3enc(tmp_buf2 + 0,loginkey,trans_buf + 0,K_po);
	for (i = 0;i < 48;i++)	printf("%02x,", (unsigned char )trans_buf[i] );
//	des3enc(tmp_buf2 + 48,loginkey,trans_buf + 48,K_po);
//	des3enc(tmp_buf2 + 96,loginkey,trans_buf + 96,K_po);
//	des3enc(tmp_buf2 + 144,loginkey,trans_buf + 144,K_po);
	printf("Des3enc result:");
	for(i=0;i<50;i++)
	{
	   printf("%02x",trans_buf[i]);
	}
	printf("\n");
	unsigned char dec[]="529ae39ea4bdb31fd4f28f4a4255537398d961bfff5ba7ee867dcac311ce838cdc3ab5f01c86a8977b3cbf0207ad520d";
	unsigned char enc[256]={0};
	
	des3dec(trans_buf + 0,loginkey,enc + 0,K_po);
//	des3dec(trans_buf + 48,loginkey,enc + 48,K_po);
//	des3dec(trans_buf + 96,loginkey,enc + 96,K_po);
//	des3dec(trans_buf + 144,loginkey,enc + 144,K_po);
	
	printf("Des3dec result:");
	printf("%s\n",enc);
	/*for(i=0;i<50;i++)
	{
	   printf("%02x",enc[i]);
	}*/
	
	//crc
	unsigned char crc_in_put[]= "wd,00000000,460001711930796_861477032135319/";
	char crc_out_put[256]={0};
	int number_byte ;
	uint16 crc;
	crc = CRC16(crc_in_put,strlen((char*)crc_in_put));
	printf("crc:%d\n",crc);
	sprintf(crc_out_put,"%04x",crc);
	printf("Crc result:%s\n",crc_out_put);
	printf("\n");
	return iRet;
}
#endif

