//#include<iostream>
//#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//using namespace std;
//extern unsigned long g_dwCnt ;

/*****************************************************************************
*			DES																 *
*												by	wuxin	2016-10-22		 *
*																			 *
*****************************************************************************/
int
//初置换   
IP[64] = {
	58 , 50 , 42 , 34 , 26 , 18 , 10 ,  2 ,
	60 , 52 , 44 , 36 , 28 , 20 , 12 ,  4 ,
	62 , 54 , 46 , 38 , 30 , 22 , 14 ,  6 ,
	64 , 56 , 48 , 40 , 32 , 24 , 16 ,  8 ,
	57 , 49 , 41 , 33 , 25 , 17 ,  9 ,  1 ,
	59 , 51 , 43 , 35 , 27 , 19 , 11 ,  3 ,
	61 , 53 , 45 , 37 , 29 , 21 , 13 ,  5 ,
	63 , 55 , 47 , 39 , 31 , 23 , 15 ,  7 };
	//逆初始置换   
int	FP[64] = {
	40 ,  8 , 48 , 16 , 56 , 24 , 64 , 32 ,
	39 ,  7 , 47 , 15 , 55 , 23 , 63 , 31 ,
	38 ,  6 , 46 , 14 , 54 , 22 , 62 , 30 ,
	37 ,  5 , 45 , 13 , 53 , 21 , 61 , 29 ,
	36 ,  4 , 44 , 12 , 52 , 20 , 60 , 28 ,
	35 ,  3 , 43 , 11 , 51 , 19 , 59 , 27 ,
	34 ,  2 , 42 , 10 , 50 , 18 , 58 , 26 ,
	33 ,  1 , 41 ,  9 , 49 , 17 , 57 , 25 };
	//密钥位移(左移次数的确定)
int	KM[16] = {
	1 ,  1 ,  2 ,  2 ,  2 ,  2 ,  2 ,  2 ,  1 ,  2 ,  2 ,  2 ,  2 ,  2 ,  2 ,  1 };
	//置换选择1
int	PC1[56] = {
	57 , 49 , 41 , 33 , 25 , 17 ,  9 ,
	1 , 58 , 50 , 42 , 34 , 26 , 18 ,
	10 ,  2 , 59 , 51 , 43 , 35 , 27 ,
	19 , 11 ,  3 , 60 , 52 , 44 , 36 ,
	63 , 55 , 47 , 39 , 31 , 23 , 15 ,
	7 , 62 , 54 , 46 , 38 , 30 , 22 ,
	14 ,  6 , 61 , 53 , 45 , 37 , 29 ,
	21 , 13 ,  5 , 28 , 20 , 12 ,  4
};
//置换选择2   
int PC2[48] = {
	14 , 17 , 11 , 24 ,  1 ,  5 ,  3 , 28 ,
	15 ,  6 , 21 , 10 , 23 , 19 , 12 ,  4 ,
	26 ,  8 , 16 ,  7 , 27 , 20 , 13 ,  2 ,
	41 , 52 , 31 , 37 , 47 , 55 , 30 , 40 ,
	51 , 45 , 33 , 48 , 44 , 49 , 39 , 56 ,
	34 , 53 , 46 , 42 , 50 , 36 , 29 , 32 };
	//E表
int	E[48] = {
	32 ,  1 ,  2 ,  3 ,  4 ,  5 ,  4 ,  5 ,
	6 ,  7 ,  8 ,  9 ,  8 ,  9 , 10 , 11 ,
	12 , 13 , 12 , 13 , 14 , 15 , 16 , 17 ,
	16 , 17 , 18 , 19 , 20 , 21 , 20 , 21 ,
	22 , 23 , 24 , 25 , 24 , 25 , 26 , 27 ,
	28 , 29 , 28 , 29 , 30 , 31 , 32 ,  1 };

	//S盒
int	s_boxes[32][16] = {
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
//P置换
int PF[32] = {
	16 ,  7 , 20 , 21 , 29 , 12 , 28 , 17 ,
	1 , 15 , 23 , 26 ,  5 , 18 , 31 , 10 ,
	2 ,  8 , 24 , 14 , 32 , 27 ,  3 ,  9 ,
	19 , 13 , 30 ,  6 , 22 , 11 ,  4 , 25 };
short K[16][56];
void initial_p(short plaint_text[64]) {
	short tmp[64];
	int i;
	for (i = 0;i < 64;i++)
		tmp[i] = plaint_text[IP[i] - 1];
	memcpy(plaint_text, tmp, sizeof(tmp));
}

void final_p(short cipher_text[64]) {
	short tmp[64];
	int i;
	for (i = 0;i < 64;i++)
		tmp[i] = cipher_text[FP[i] - 1];
	memcpy(cipher_text, tmp, sizeof(tmp));
}

void extend_p(short text[32], short output[48]) {
	int i;
	for (i = 0;i < 48;i++)
		output[i] = text[E[i] - 1];
}
void permutation1(short key[64], short o[56]) {
	//short tmp[56];
	int i;
	for (i = 0;i < 56;i++)
		o[i] = key[PC1[i] - 1];
	//memcpy(output, tmp, sizeof(tmp));
}


void permutationP(short text[32]) {
	short tmp[32];
	int i;
	for ( i = 0;i < 32;i++) {
		tmp[i] = text[PF[i] - 1];
	}
	memcpy(text, tmp, sizeof(tmp));
}
void F(short R[32], short key[48]) {
	int i;
	int j;
	short R_48[48];
	//表E扩展
	extend_p(R, R_48);

	//XOR运算
	short S_in[48];
	for ( i = 0;i < 48;i++) {
		S_in[i] = R_48[i] ^ key[i];//^按位异或
	}

	//S盒置换
	short S_out[32];
	
	for (i = 0;i < 8;i++) {
		int a = (S_in[i * 6] << 1) + S_in[i * 6 + 5];//第一位+最后一位
		int b = (S_in[i * 6 + 1] << 3) + (S_in[i * 6 + 2] << 2) + (S_in[i * 6 + 3] << 1) + S_in[i * 6 + 4];//中间4位
		int s = s_boxes[i * 4 + a][b];
		for (j = 0;j < 4;j++) {
			S_out[i * 4 + j] = (s >> (3 - j)) & 1;
		}
	}
	//P置换
	permutationP(S_out);
	memcpy(R, S_out, sizeof(S_out));

}

void Round(short L[32], short R[32], short key[48]) {
	short R_1[32];
	int i;
	for ( i = 0;i < 32;i++) {
		R_1[i] = R[i];
	}
	F(R_1, key);

	//L XOR R_1
	for ( i = 0;i < 32;i++) {
		R_1[i] ^= L[i];
	}

	//交换L/R
	for ( i = 0;i < 32;i++) {
		L[i] = R[i];
		R[i] = R_1[i];
	}
	//memcpy(R,R_1,sizeof(R_1));

}
void setK(char keyC[8]) {		//09876543
	//计算key
	short keyP[64];
	short key[56];
	short C[84], D[84];
	int i;
	int j;
	for ( i = 0;i < 8;i++)
	{
		for ( j = 0;j < 8;j++)
		{
			keyP[i * 8 + j] = (keyC[i] >> (7 - j)) & 1;
		}
	}

	//转置1,获得56位key
	permutation1(keyP, key);
	//C D分组

	for ( i = 0;i < 28;i++)
	{
		C[56 + i] = C[28 + i] = C[i] = key[i];
		D[56 + i] = D[28 + i] = D[i] = key[28 + i];
	}
	//转置2的同时生成16轮密钥的矩阵
	int shift_len = 0;
	for ( i = 0;i < 16;i++) 
	{
		shift_len += KM[i];
		for ( j = 0;j < 48;j++) 
		{
			K[i][j] = (PC2[j]<28) ? C[PC2[j] - 1 + shift_len] : D[PC2[j] - 29 + shift_len];
		}
	}
}

void Enc(char txt[8], char enc[8]) {

	//
	short plain[64];
	short L[32], R[32];
	int i;
	int j;
	for ( i = 0;i < 8;i++) 
	{
		for ( j = 0;j < 8;j++) 
		{
			plain[i * 8 + j] = (txt[i] >> (7 - j)) & 1;
		}
	}

	initial_p(plain);
	//分组
	for ( i = 0;i < 32;i++)
	{
		L[i] = plain[i];
		R[i] = plain[i + 32];
	}

	for ( i = 0;i < 16;i++)
	{
		Round(L, R, K[i]);
	}
	//合并 & L\R交换
	for ( i = 0;i < 32;i++) 
	{
		plain[i] = R[i];
		plain[i + 32] = L[i];
	}

	final_p(plain);
	//for (int i = 0;i < 64;i++) {		cout << plain[i];	}	cout << endl;
	for ( i = 0;i < 8;i++)
		for ( j = 7;j >= 0;j--) 
		{
			enc[i] |= plain[i * 8 + (7 - j)] << j;//+还是|？
		}
//	printf("enc:%s",enc);
}
void Dec(char txt[8], char dec[8]) {

	//生成明文二进制(用长度64的数组代替)
	short plain[64];
	short L[32], R[32];
	int i;
	int j;
	for ( i = 0;i < 8;i++) {
		for ( j = 0;j < 8;j++) {
			plain[i * 8 + j] = (txt[i] >> (7 - j)) & 1;//plain[i * 8 + j]保存0或者1
		}
	}
	initial_p(plain);

	//分组
	for ( i = 0;i < 32;i++) {
		L[i] = plain[i];
		R[i] = plain[i + 32];
	}
	for ( i = 0;i < 16;i++) {
		Round(L, R, K[15 - i]);
	}
	//合并
	for ( i = 0;i < 32;i++) {
		plain[i] = R[i];
		plain[i + 32] = L[i];
	}
	final_p(plain);

	for ( i = 0;i < 8;i++)
		for ( j = 7;j >= 0;j--) {
			dec[i] |= plain[i * 8 + (7 - j)] << j;//+还是|？
		}
	//for ( i = 0;i < 64;i++) {
	//	printf("%d",plain[i]);	
	//}
}
void Des() {
	char txt[9] = "12345678";//保存明文
	char keyC[9] = "09876543";//保存密码09876543
	char enc[9] = "";
	char dec[9] = "";
	int i;
	setK(keyC);		//该keyC变量的值会通过转变后放入K[16][56]
	Enc(txt, enc);	//将keyC，txt经过处理后，得到enc
	printf("\r\n");
	for ( i = 0;i < 8;i++) {
		printf("%02x,", (unsigned char )enc[i] );
	}
	printf("\r\n");
	Dec(enc, dec);		//通过enc经过处理后，得到dec
	printf("\r\n");
	for ( i = 0;i < 8;i++) {
		printf("%2x,", (unsigned char )dec[i] );
	}
	printf("\r\n");
}
int test_Des(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut);
int test_Des2(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut); 
int test_Enc48(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut);
int test_Dec48(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut); 

/*encode/decode(编码/解码)*/
#if 0
int main(int argc ,char *argv[]) 
{
	unsigned char txt[ ] = "cmd_typ=heart_data_send,cmd_id=00000000000000000_00000005,dev_time=20181212010203,sig=31,net=3,tag_open=1,tag_move=1,cell_v=4.1,cell_pwr=35,lost_pwr=0";//保存明文,字符串+\0
	unsigned char keyC[9] = "09876543";//保存密码
	unsigned char pi_ucOut[48*4]={0};
	unsigned char pi[48*4] ={0};	
	
	printf("sizeof:%ld\r\n",(sizeof(txt)-1));
	test_Enc48((unsigned char *)(txt + 0),keyC,(unsigned char *)(pi_ucOut + 0));
	test_Enc48((unsigned char *)(txt + 48),keyC,(unsigned char *)(pi_ucOut + 48));
	test_Enc48((unsigned char *)(txt + 96),keyC,(unsigned char *)(pi_ucOut + 96));
	test_Enc48((unsigned char *)(txt + 144),keyC,(unsigned char *)(pi_ucOut + 144));
//	test_Enc48((unsigned char *)(txt + 192),keyC,(unsigned char *)(pi_ucOut + 192));
//	unsigned char pi[sizeof(txt)] = {0};

//	printf("pi_ucOut:%s\n",pi_ucOut);
	test_Dec48((pi_ucOut + 0),keyC,(pi + 0));
	test_Dec48((pi_ucOut + 48),keyC,(pi + 48));
	test_Dec48((pi_ucOut + 96),keyC,(pi + 96));
	test_Dec48((pi_ucOut + 144),keyC,(pi + 144));
//	test_Dec48((pi_ucOut + 192),keyC,(pi + 192));
	printf("\rtest_Dec48 pi length:%ld\r\n",(sizeof(pi)-1));
	printf("%s\n",pi);
	return 0;
}
#endif

/*解码方式测试*/
int test_Des(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut) 
{
	//char txt[9] = "12345678";//保存明文
	//char keyC[9] = "09876543";//保存密码
	char enc[9] = "";
	//char dec[9] = "";
	int i;
	memset( (void*)enc, 0x00, sizeof(enc) );
	//memset( (void*)dec, 0x00, sizeof(dec) );
	printf("\r\ntest_Des() data\r\n" );	
	for ( i = 0;i < 8;i++)	
		printf("%02x,", (unsigned char )pi_ucTxt[i] );
	printf("\r\ntest_Des() key\r\n" );	
	for ( i = 0;i < 8;i++)	
		printf("%02x,", (unsigned char )pi_ucKey1[i] );

	setK((char *)pi_ucKey1);
	Enc((char *)pi_ucTxt, enc);
//	for ( i = 0;i < 8;i++)	
//		pi_ucOut[i]= (unsigned char )enc[i] ;
	printf("\r\ntest_Des() enc\r\n");	
	for ( i = 0;i < 8;i++)	
		printf("%02x,", (unsigned char )enc[i] );

	//Dec(enc, dec);
	//printf("\r dec:");	for (int i = 0;i < 8;i++)	printf("%02x,", (unsigned char )dec[i] );
	//printf("\r");

	return 1;
}

/*解码方式测试*/
int test_Des2(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut) 
{
	//char txt[9] = "12345678";//保存明文
	//char keyC[9] = "09876543";//保存密码
	//char enc[9] = "";
	char dec[9] = "";
	int i;
	//memset( (void*)enc, 0x00, sizeof(enc) );
	memset( (void*)dec, 0x00, sizeof(dec) );

	printf("\r test_Des2() data");	for ( i = 0;i < 8;i++)	printf("%02x,", (unsigned char )pi_ucTxt[i] );
	printf("\r test_Des2() key");	for ( i = 0;i < 8;i++)	printf("%02x,", (unsigned char )pi_ucKey1[i] );

	setK((char*)pi_ucKey1);
	//Enc((char*)pi_ucTxt, enc);
	Dec((char*)pi_ucTxt, dec);

	//for ( i = 0;i < 8;i++)	pi_ucOut[i]= (unsigned char )dec[i] ;
	//printf("\r enc:");	for (int i = 0;i < 8;i++)	printf("%02x,", (unsigned char )enc[i] );

	printf("\r test_Des2() dec");	for ( i = 0;i < 8;i++)	printf("%02x,", (unsigned char )dec[i] );
	//printf("\r");

	return 1;
}




/*编码方式测试*/
int test_Enc48(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut) 
{
	int i;
	char txt[] = "12345678";//保存明文
	char keyC[9] = "09876543";//保存密码
	char enc[9] = "";
	char dec[9] = "";
	char enc_hex[4096];
	memset( (void*)enc, 0x00, sizeof(enc) );
	memset( (void*)dec, 0x00, sizeof(dec) );
	//48字节数据
//	printf("\r test_Enc48() data\n");	for ( i = 0;i < 48;i++)	printf("%02x,", (unsigned char )pi_ucTxt[i] );
	//8字节key
//	printf("\r test_Enc48() key");	for ( i = 0;i < 8;i++)	printf("%02x,", (unsigned char )pi_ucKey1[i] );

	setK((char*)pi_ucKey1);
	Enc((char*)(pi_ucTxt+ 0), (char*)(pi_ucOut+ 0) );
	Enc((char*)(pi_ucTxt+ 8), (char*)(pi_ucOut+ 8) );
	Enc((char*)(pi_ucTxt+16), (char*)(pi_ucOut+16) );
	Enc((char*)(pi_ucTxt+24), (char*)(pi_ucOut+24) );
	Enc((char*)(pi_ucTxt+32), (char*)(pi_ucOut+32) );
	Enc((char*)(pi_ucTxt+40), (char*)(pi_ucOut+40) );

	//for ( i = 0;i < 8;i++)	pi_ucOut[i]= (unsigned char )enc[i] ;
	/*printf("\r test_Enc48() enc\n");*/
	
	for ( i = 0;i < 48;i++)	
	{
		//printf("%02x,", (unsigned char )pi_ucOut[i]);
		//sprintf(enc_hex+i*2,"%02x",(unsigned char )pi_ucOut[i]);
	}
	//printf("\n");
	//printf("pi_ucOut:[%s]\n",pi_ucOut);


	//Dec(enc, dec);
	//printf("\r dec:");	for (int i = 0;i < 8;i++)	printf("%02x,", (unsigned char )dec[i] );
	//printf("\r");

	return 1;
}


/*解码方式测试*/
int test_Dec48(unsigned char * pi_ucTxt, unsigned char * pi_ucKey1, unsigned char * pi_ucOut) 
{
	int i;
	//char txt[9] = "12345678";//保存明文
	//char keyC[9] = "09876543";//保存密码
	//char enc[9] = "";
	//char dec[9] = "";

	//memset( (void*)enc, 0x00, sizeof(enc) );
	//memset( (void*)dec, 0x00, sizeof(dec) );

	//48字节数据
	//printf("\r test_Dec48() data\n");	for ( i = 0;i < 48;i++)	printf("%02x,", (unsigned char )pi_ucTxt[i] );
	//8字节key
	//printf("\r test_Dec48() key");	for (int i = 0;i < 8;i++)	printf("%02x,", (unsigned char )pi_ucKey1[i] );

	setK((char*)pi_ucKey1);

	Dec((char*)(pi_ucTxt+ 0), (char*)(pi_ucOut+ 0) );
	Dec((char*)(pi_ucTxt+ 8), (char*)(pi_ucOut+ 8) );
	Dec((char*)(pi_ucTxt+16), (char*)(pi_ucOut+16) );
	Dec((char*)(pi_ucTxt+24), (char*)(pi_ucOut+24) );
	Dec((char*)(pi_ucTxt+32), (char*)(pi_ucOut+32) );
	Dec((char*)(pi_ucTxt+40), (char*)(pi_ucOut+40) );

//	for (int i = 0;i < 8;i++)	pi_ucOut[i]= (unsigned char )dec[i] ;
//	printf("\r enc:");	for (int i = 0;i < 8;i++)	printf("%02x,", (unsigned char )enc[i] );

//	/*printf("\r test_Dec48() dec\n");*/	for ( i = 0;i <48;i++)	printf("%02x.", (unsigned char )pi_ucOut[i] );
//	printf("\n");

	return 1;
}

