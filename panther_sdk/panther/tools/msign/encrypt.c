#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(COMPACT_VERSION)

#define BYTE unsigned char

void printBytes(BYTE b[], int len);
void AES_SubBytes(BYTE state[], BYTE sbox[]);
void AES_AddRoundKey(BYTE state[], BYTE rkey[]);
void AES_ShiftRows(BYTE state[], BYTE shifttab[]);
void AES_MixColumns(BYTE state[]);
void AES_MixColumns_Inv(BYTE state[]);
void AES_Init(void);
void AES_Done(void);
void AES_ECB_Encrypt(BYTE *data, int datalen, BYTE *key, int keylen);
void AES_ECB_Decrypt(BYTE *data, int datalen, BYTE *key, int keylen);
int AES_ExpandKey(BYTE key[], int keyLen);
void AES_Encrypt(BYTE block[], BYTE key[], int keyLen);
void AES_Decrypt(BYTE block[], BYTE key[], int keyLen);
void AES_CBC_Encrypt(BYTE *data, int datalen, BYTE *key, int keylen);
void AES_CBC_Decrypt(BYTE *data, int datalen, BYTE *key, int keylen);
int gen_decrypt_erased_page_header(const char *outfile, int page_size);
int encrypt(const char *infile, const char *outfile, int page_size, int start_offset);

void printBytes(BYTE b[], int len)
{
   int i;
   for (i = 0; i < len; i++) printf("%02x ", b[i]);
   printf("\n");
}

/******************************************************************************/

// The following lookup tables and functions are for internal use only!
BYTE AES_Sbox[] = { 99, 124, 119, 123, 242, 107, 111, 197, 48, 1, 103, 43, 254, 215, 171,
   118, 202, 130, 201, 125, 250, 89, 71, 240, 173, 212, 162, 175, 156, 164, 114, 192, 183, 253,
   147, 38, 54, 63, 247, 204, 52, 165, 229, 241, 113, 216, 49, 21, 4, 199, 35, 195, 24, 150, 5, 154,
   7, 18, 128, 226, 235, 39, 178, 117, 9, 131, 44, 26, 27, 110, 90, 160, 82, 59, 214, 179, 41, 227,
   47, 132, 83, 209, 0, 237, 32, 252, 177, 91, 106, 203, 190, 57, 74, 76, 88, 207, 208, 239, 170,
   251, 67, 77, 51, 133, 69, 249, 2, 127, 80, 60, 159, 168, 81, 163, 64, 143, 146, 157, 56, 245,
   188, 182, 218, 33, 16, 255, 243, 210, 205, 12, 19, 236, 95, 151, 68, 23, 196, 167, 126, 61,
   100, 93, 25, 115, 96, 129, 79, 220, 34, 42, 144, 136, 70, 238, 184, 20, 222, 94, 11, 219, 224,
   50, 58, 10, 73, 6, 36, 92, 194, 211, 172, 98, 145, 149, 228, 121, 231, 200, 55, 109, 141, 213,
   78, 169, 108, 86, 244, 234, 101, 122, 174, 8, 186, 120, 37, 46, 28, 166, 180, 198, 232, 221,
   116, 31, 75, 189, 139, 138, 112, 62, 181, 102, 72, 3, 246, 14, 97, 53, 87, 185, 134, 193, 29,
   158, 225, 248, 152, 17, 105, 217, 142, 148, 155, 30, 135, 233, 206, 85, 40, 223, 140, 161,
   137, 13, 191, 230, 66, 104, 65, 153, 45, 15, 176, 84, 187, 22 };

BYTE AES_ShiftRowTab[] = { 0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, 1, 6, 11 };

BYTE AES_Sbox_Inv[256];
BYTE AES_ShiftRowTab_Inv[16];
BYTE AES_xtime[256];

void AES_SubBytes(BYTE state[], BYTE sbox[])
{
   int i;
   for (i = 0; i < 16; i++) state[i] = sbox[state[i]];
}

void AES_AddRoundKey(BYTE state[], BYTE rkey[])
{
   int i;
   for (i = 0; i < 16; i++) state[i] ^= rkey[i];
}

void AES_ShiftRows(BYTE state[], BYTE shifttab[])
{
   BYTE h[16];
   memcpy(h, state, 16);
   int i;
   for (i = 0; i < 16; i++) state[i] = h[shifttab[i]];
}

void AES_MixColumns(BYTE state[])
{
   int i;
   for (i = 0; i < 16; i += 4)
   {
      BYTE s0 = state[i + 0], s1 = state[i + 1];
      BYTE s2 = state[i + 2], s3 = state[i + 3];
      BYTE h = s0 ^ s1 ^ s2 ^ s3;
      state[i + 0] ^= h ^ AES_xtime[s0 ^ s1];
      state[i + 1] ^= h ^ AES_xtime[s1 ^ s2];
      state[i + 2] ^= h ^ AES_xtime[s2 ^ s3];
      state[i + 3] ^= h ^ AES_xtime[s3 ^ s0];
   }
}

void AES_MixColumns_Inv(BYTE state[])
{
   int i;
   for (i = 0; i < 16; i += 4)
   {
      BYTE s0 = state[i + 0], s1 = state[i + 1];
      BYTE s2 = state[i + 2], s3 = state[i + 3];
      BYTE h = s0 ^ s1 ^ s2 ^ s3;
      BYTE xh = AES_xtime[h];
      BYTE h1 = AES_xtime[AES_xtime[xh ^ s0 ^ s2]] ^ h;
      BYTE h2 = AES_xtime[AES_xtime[xh ^ s1 ^ s3]] ^ h;
      state[i + 0] ^= h1 ^ AES_xtime[s0 ^ s1];
      state[i + 1] ^= h2 ^ AES_xtime[s1 ^ s2];
      state[i + 2] ^= h1 ^ AES_xtime[s2 ^ s3];
      state[i + 3] ^= h2 ^ AES_xtime[s3 ^ s0];
   }
}

// AES_Init: initialize the tables needed at runtime.
// Call this function before the (first) key expansion.
void AES_Init(void)
{
   int i;
   for (i = 0; i < 256; i++) AES_Sbox_Inv[AES_Sbox[i]] = i;

   for (i = 0; i < 16; i++) AES_ShiftRowTab_Inv[AES_ShiftRowTab[i]] = i;

   for (i = 0; i < 128; i++)
   {
      AES_xtime[i] = i << 1;
      AES_xtime[128 + i] = (i << 1) ^ 0x1b;
   }
}

// AES_Done: release memory reserved by AES_Init.
// Call this function after the last encryption/decryption operation.
void AES_Done(void) {
}

/* AES_ExpandKey: expand a cipher key. Depending on the desired encryption 
   strength of 128, 192 or 256 bits 'key' has to be a byte array of length 
   16, 24 or 32, respectively. The key expansion is done "in place", meaning 
   that the array 'key' is modified.
*/
int AES_ExpandKey(BYTE key[], int keyLen)
{
   int kl = keyLen, ks, Rcon = 1, i, j;
   BYTE temp[4], temp2[4];
   switch (kl)
   {
   case 16:
      ks = 16 * (10 + 1); break;
   case 24:
      ks = 16 * (12 + 1); break;
   case 32:
      ks = 16 * (14 + 1); break;
   default:
      printf("AES_ExpandKey: Only key lengths of 16, 24 or 32 bytes allowed!");
   }
   for (i = kl; i < ks; i += 4)
   {
      memcpy(temp, &key[i - 4], 4);
      if (i % kl == 0)
      {
         temp2[0] = AES_Sbox[temp[1]] ^ Rcon;
         temp2[1] = AES_Sbox[temp[2]];
         temp2[2] = AES_Sbox[temp[3]];
         temp2[3] = AES_Sbox[temp[0]];
         memcpy(temp, temp2, 4);
         if ((Rcon <<= 1) >= 256) Rcon ^= 0x11b;
      }
      else if ((kl > 24) && (i % kl == 16))
      {
         temp2[0] = AES_Sbox[temp[0]];
         temp2[1] = AES_Sbox[temp[1]];
         temp2[2] = AES_Sbox[temp[2]];
         temp2[3] = AES_Sbox[temp[3]];
         memcpy(temp, temp2, 4);
      }
      for (j = 0; j < 4; j++) key[i + j] = key[i + j - kl] ^ temp[j];
   }
   return ks;
}

// AES_Encrypt: encrypt the 16 byte array 'block' with the previously expanded key 'key'.
void AES_Encrypt(BYTE block[], BYTE key[], int keyLen)
{
   int l = keyLen, i;
   //printBytes(block, 16);
   AES_AddRoundKey(block, &key[0]);
   for (i = 16; i < l - 16; i += 16)
   {
      AES_SubBytes(block, AES_Sbox);
      AES_ShiftRows(block, AES_ShiftRowTab);
      AES_MixColumns(block);
      AES_AddRoundKey(block, &key[i]);
   }
   AES_SubBytes(block, AES_Sbox);
   AES_ShiftRows(block, AES_ShiftRowTab);
   AES_AddRoundKey(block, &key[i]);
}

// AES_Decrypt: decrypt the 16 byte array 'block' with the previously expanded key 'key'.
void AES_Decrypt(BYTE block[], BYTE key[], int keyLen)
{
   int l = keyLen, i;
   AES_AddRoundKey(block, &key[l - 16]);
   AES_ShiftRows(block, AES_ShiftRowTab_Inv);
   AES_SubBytes(block, AES_Sbox_Inv);
   for (i = l - 32; i >= 16; i -= 16)
   {
      AES_AddRoundKey(block, &key[i]);
      AES_MixColumns_Inv(block);
      AES_ShiftRows(block, AES_ShiftRowTab_Inv);
      AES_SubBytes(block, AES_Sbox_Inv);
   }
   AES_AddRoundKey(block, &key[0]);
}

void AES_ECB_Encrypt(BYTE *data, int datalen, BYTE *key, int keylen)
{
   BYTE expandKey[16 * (14 + 1)];
   int expandKeyLen;
   int i;

   memcpy(expandKey, key, keylen);
   expandKeyLen = AES_ExpandKey(expandKey, keylen);

   for (i = 0; i < datalen / 16; i++)
   {
      AES_Encrypt(&data[i * 16], expandKey, expandKeyLen);
   }
}

void AES_ECB_Decrypt(BYTE *data, int datalen, BYTE *key, int keylen)
{
   BYTE expandKey[16 * (14 + 1)];
   int expandKeyLen;
   int i;

   memcpy(expandKey, key, keylen);
   expandKeyLen = AES_ExpandKey(expandKey, keylen);

   for (i = 0; i < datalen / 16; i++)
   {
      AES_Decrypt(&data[i * 16], expandKey, expandKeyLen);
   }
}


void AES_CBC_Encrypt(BYTE *data, int datalen, BYTE *key, int keylen)
{
   BYTE expandKey[16 * (14 + 1)];
   int expandKeyLen;
   int i;
   int j;
   BYTE last_block[16];

   memcpy(expandKey, key, keylen);
   expandKeyLen = AES_ExpandKey(expandKey, keylen);

   for (i = 0; i < datalen / 16; i++)
   {
      if (i != 0)
      {
         for (j = 0; j < 16; j++) data[j + i * 16] ^= last_block[j];
      }
      AES_Encrypt(&data[i * 16], expandKey, expandKeyLen);
      memcpy(last_block, &data[i * 16], 16);
   }
}

void AES_CBC_Decrypt(BYTE *data, int datalen, BYTE *key, int keylen)
{
   BYTE expandKey[16 * (14 + 1)];
   int expandKeyLen;
   int i;
   int j;
   BYTE last_block[16];
   BYTE curr_block[16];

   memcpy(expandKey, key, keylen);
   expandKeyLen = AES_ExpandKey(expandKey, keylen);

   memset(last_block, 0, 16);
   for (i = 0; i < datalen / 16; i++)
   {
      memcpy(curr_block, &data[i * 16], 16);

      AES_Decrypt(&data[i * 16], expandKey, expandKeyLen);

      if (i != 0)
      {
         for (j = 0; j < 16; j++) data[j + i * 16] ^= last_block[j];
      }
      memcpy(last_block, curr_block, 16);
   }
}

// ===================== test ============================================
#if 0

#define DATA_LEN    64
#define KEY_LEN     24


#define MAX_KEY_LEN 32
BYTE buf[DATA_LEN];
BYTE key[MAX_KEY_LEN];
int main()
{
   int i;

   AES_Init();

   printf("=== ECB mode ===\n");

   for(i = 0; i < DATA_LEN; i++)
   buf[i] = i;

   for(i = 0; i < MAX_KEY_LEN; i++)
   key[i] = i;

   printf("Orig data:"); printBytes(buf, DATA_LEN);

   AES_ECB_Encrypt(buf, DATA_LEN, key, KEY_LEN);
   printf("Encrypted data:"); printBytes(buf, DATA_LEN);

   AES_ECB_Decrypt(buf, DATA_LEN, key, KEY_LEN);
   printf("Decrypted data:"); printBytes(buf, DATA_LEN);

   printf("=== CBC mode ===\n");


   for(i = 0; i < DATA_LEN; i++)
   buf[i] = 0;

   for(i = 0; i < MAX_KEY_LEN; i++)
   key[i] = 0;

   printf("Orig data:"); printBytes(buf, DATA_LEN);

   AES_CBC_Encrypt(buf, DATA_LEN, key, KEY_LEN);
   printf("Encrypted data:"); printBytes(buf, DATA_LEN);

   AES_CBC_Decrypt(buf, DATA_LEN, key, KEY_LEN);
   printf("Decrypted data:"); printBytes(buf, DATA_LEN);

}
#endif
#if 0
int main()
{
   int i;
   AES_Init();

   BYTE block[16];
   for(i = 0; i < 16; i++)
   block[i] = i;

   printf("192bits AES key encrypt/decrypt test\n");

   printf("Orig. message:"); printBytes(block, 16);

   BYTE key[16 * (14 + 1)];
   int keyLen = 24, maxKeyLen=16 * (14 + 1), blockLen = 16;
   for(i = 0; i < 16; i++)
   key[i] = 16+i;
   for(i = 16; i< 32; i++)
   key[i] = 0;

   printf("Orig. key:"); printBytes(key, keyLen);

   int expandKeyLen = AES_ExpandKey(key, keyLen);

   printf("Expanded key:"); printBytes(key, expandKeyLen);

   AES_Encrypt(block, key, expandKeyLen);

   printf("Encrypted data:"); printBytes(block, blockLen);

   AES_Decrypt(block, key, expandKeyLen);

   printf("Decrypted data:"); printBytes(block, blockLen);

   AES_Done();
}
/*  Expected result

192bits AES key encrypt/decrypt test
Orig. message:00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
Orig. key:10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 00 00 00 00 00 00 00 00
Expanded key:10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 00 00 00 00 00 00 00 00 72 72 71 70 66 67 67 67 7e 7e 7d 7c 62 63 63 63 62 63 63 63 62 63 63 63 8b 89 8a da ed ee ed bd 93 90 90 c1 f1 f3 f3 a2 93 90 90 c1 f1 f3 f3 a2 82 84 b0 7b 6f 6a 5d c6 fc fa cd 07 0d 09 3e a5 9e 99 ae 64 6f 6a 5d c6 88 c8 04 d3 e7 a2 59 15 1b 58 94 12 16 51 aa b7 88 c8 04 d3 e7 a2 59 15 a2 03 5d 47 45 a1 04 52 5e f9 90 40 48 a8 3a f7 c0 60 3e 24 27 c2 67 31 a7 86 9a 8b e2 27 9e d9 bc de 0e 99 f4 76 34 6e 34 16 0a 4a 13 d4 6d 7b af ba bb f6 4d 9d 25 2f f1 43 2b b6 05 35 1f d8 31 23 15 92 22 f7 78 e9 47 06 a5 65 0a 9b 80 4a fb d8 ab fc fe ed b4 24
00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
Encrypted data:98 1f 23 05 a4 68 b8 1c 77 6f 29 3b 4c 14 60 f6
Decrypted data:00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f

*/
#endif

#define KEY_LENGTH (256 / 8)           // IPL uses AES-256

extern int get_aes_key(unsigned char *key);

int gen_decrypt_erased_page_header(const char *outfile, int page_size)
{
   FILE *filew;
   unsigned char key[32];
   unsigned char *buffer;
   int i;

   if(get_aes_key(key))
   {
      fprintf(stderr, "Read secret key file error.\n");
      return 1;
   }

   if ((filew = fopen(outfile, "wb")) == NULL)
   {
      fprintf(stderr, "Open output header file '%s' error.\n", outfile);
      return 1;
   }

   buffer = malloc(page_size);
   if (NULL==buffer)
   {
      fprintf(stderr, "Malloc failed\n");
      return 1;
   }
   memset(buffer, 0xff, page_size);

   AES_Init();

   AES_CBC_Decrypt(buffer, page_size, key, KEY_LENGTH);

   fprintf(filew, "#ifndef __ERASED_PAGE_H__\n");
   fprintf(filew, "#define __ERASED_PAGE_H__\n\n");
   fprintf(filew, "#define ERASED_PAGE_SIZE %d\n\n", page_size);
   fprintf(filew, "const unsigned char __erased_page_data_first[] = {\n");
   for(i=0;i<16;i++)
   {
      if (i==0)
      {
         fprintf(filew, "0x%02X", buffer[i]);
      }
      else
      {
         fprintf(filew, ",0x%02X", buffer[i]);
      }
      if(((i+1)%16)==0)
         fprintf(filew, "\n");
   }
   fprintf(filew, "};\n\n");
   fprintf(filew, "const unsigned char __erased_page_data_next[] = {\n");
   for(i=16;i<32;i++)
   {
      if (i==16)
      {
         fprintf(filew, "0x%02X", buffer[i]);
      }
      else
      {
         fprintf(filew, ",0x%02X", buffer[i]);
      }
      if(((i+1)%16)==0)
         fprintf(filew, "\n");
   }
   fprintf(filew, "};\n\n");
   fprintf(filew, "\nstatic inline int is_erased_page(unsigned char *data, int size)\n");
   fprintf(filew, "{\n");
   fprintf(filew, "  int i;\n");
   fprintf(filew, "  if(memcmp(__erased_page_data_first, &data[0], 16))\n");
   fprintf(filew, "     return 0;\n");
   fprintf(filew, "  for(i=16;i<size;i+=16)\n");
   fprintf(filew, "  {\n");
   fprintf(filew, "     if(memcmp(__erased_page_data_next, &data[i], 16))\n");
   fprintf(filew, "        return 0;\n");
   fprintf(filew, "  }\n");
   fprintf(filew, "  return 1;\n");
   fprintf(filew, "}\n\n");
#if 0
   fprintf(filew, "const unsigned char *__erased_page_data[] = {\n");
   for(i=0;i<page_size;i++)
   {
      fprintf(filew, "0x%02X,", buffer[i]);
      if(((i+1)%16)==0)
         fprintf(filew, "\n");
   }
   fprintf(filew, "};\n");
#endif
   fprintf(filew, "#endif // __ERASED_PAGE_H__\n");

   fclose(filew);
   free(buffer);

   return 0;
}

int encrypt(const char *infile, const char *outfile, int page_size, int start_offset)
{
   FILE * filer,*filew;
   int numr, numw;
   unsigned char *buffer;
   unsigned char key[32];
   int skip_num = 0;

   if(start_offset)
   {
      skip_num = start_offset / page_size;
   }

   if(get_aes_key(key))
   {
      fprintf(stderr, "Read secret key file error.\n");
      return 1;
   }
   
   if ((filer = fopen(infile, "rb")) == NULL)
   {
      fprintf(stderr, "Open input image file '%s' error.\n", infile);
      return 1;
   }

   buffer = malloc(page_size);
   if (NULL==buffer)
   {
      fprintf(stderr, "Malloc failed\n");
      return 1;
   }

   if ((filew = fopen(outfile, "wb")) == NULL)
   {
      fprintf(stderr, "Open output image file '%s' error.\n", outfile);
      return 1;
   }

   AES_Init();

   while (feof(filer) == 0)
   {
      if ((numr = fread(buffer, 1, page_size, filer)) != page_size)
      {
         if (ferror(filer) != 0)
         {
            fprintf(stderr, "Read input image file error.\n");
            return 1;
         }
         
         if(numr==0)
            break;

         memset(&buffer[numr], 0, page_size - numr);
         numr = page_size;
      }

      if(skip_num)
      {
         skip_num--;
      }
      else
      {
         AES_CBC_Encrypt(buffer, page_size, key, KEY_LENGTH);
      }

      if ((numw = fwrite(buffer, 1, numr, filew)) != numr)
      {
         fprintf(stderr, "Write output image file error.\n");
         return 1;
      }
   }

   fclose(filer);
   fclose(filew);
   free(buffer);

   return 0;
}

#endif

