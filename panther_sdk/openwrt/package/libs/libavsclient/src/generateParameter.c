/*
 * genreateParameter.c
 *
 * 生成APP端登录amazon alexa时所需要的参数
 * 需要参数：ProductId、Dsn、CodeChallenge、CodeChallengeMethod、code_verifier
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/file.h>
#include <sys/stat.h> 
#include <assert.h>
#include <openssl/evp.h>  
#include <openssl/bio.h>  
#include <openssl/buffer.h> 
#include <openssl/sha.h>   
#include <openssl/crypto.h>
#include "generateParameter.h"
#include "configuration.h"

#include <sys/socket.h>  
#include <sys/ioctl.h>  
#include <netinet/if_ether.h>  
#include <net/if.h>  
#include <linux/sockios.h>  
#define CODECHALLENGEMETHOD "S256"


/**
	* Base64 index table.
	*/
#if 0
	// 标准base 64
	 static const char b64_table[] = {
	  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	  'w', 'x', 'y', 'z', '0', '1', '2', '3',
	  '4', '5', '6', '7', '8', '9', '+', '/'
	};
#else
	static const char b64_table[] = {
	  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	  'w', 'x', 'y', 'z', '0', '1', '2', '3',
	  '4', '5', '6', '7', '8', '9', '-', '_'
	};
#endif



char *
base64Encode (unsigned char *src, size_t len) {
  int i = 0;
  int j = 0;
  int k = 0;
  char *enc = NULL;
  size_t size = 0;
  unsigned char buf[4];
  unsigned char tmp[3];

	// alloc
	enc = (char *) malloc(0);
	if (NULL == enc) { return NULL; }

	// parse until end of source
	while (len--) {
		// read up to 3 bytes at a time into `tmp'
		//tmp[i++] = *(src++);
		tmp[i++] = src[k++];

		// if 3 bytes read then encode into `buf'
		if (3 == i) {
		  buf[0] = (tmp[0] & 0xfc) >> 2;
		  buf[1] = ((tmp[0] & 0x03) << 4) | ((tmp[1] & 0xf0) >> 4);
		  buf[2] = ((tmp[1] & 0x0f) << 2) | ((tmp[2] & 0xc0) >> 6);
		  buf[3] = tmp[2] & 0x3f;
		  // allocate 4 new byts for `enc` and
		  // then translate each encoded buffer
		  // part by index from the base 64 index table
		  // into `enc' unsigned char array
		  enc = (char *) realloc(enc, size + 4);
		  for (i = 0; i < 4; ++i) {
			enc[size++] = b64_table[buf[i]];
		  }
		  // reset index
		  i = 0;
		}
	}

	// remainder
	if (i > 0) {
	// fill `tmp' with `\0' at most 3 times
	for (j = i; j < 3; ++j) {
	  tmp[j] = '\0';
	}

	// perform same codec as above
	buf[0] = (tmp[0] & 0xfc) >> 2;
	buf[1] = ((tmp[0] & 0x03) << 4) + ((tmp[1] & 0xf0) >> 4);
	buf[2] = ((tmp[1] & 0x0f) << 2) + ((tmp[2] & 0xc0) >> 6);
	buf[3] = tmp[2] & 0x3f;

	// perform same write to `enc` with new allocation
	for (j = 0; (j < i + 1); ++j) {
	  enc = (char *) realloc(enc, size + 1);
	  enc[size++] = b64_table[buf[j]];
	}

    // while there is still a remainder
    // append `=' to `enc'
    while ((i++ < 3)) {
      enc = (char *) realloc(enc, size + 1);
      enc[size++] = '=';
    }
  }

	// Make sure we have enough space to add '\0' character at end.
	enc = (char *) realloc(enc, size + 1);
	enc[size] = '\0';

    return enc;
}


char * hash256(char *orgStr)  
{  
    SHA256_CTX c;  
    char md[SHA256_DIGEST_LENGTH];  
    SHA256((char *)orgStr, strlen(orgStr)-1, md);  
  
    SHA256_Init(&c);  
    SHA256_Update(&c, orgStr, strlen(orgStr)-1);  
    SHA256_Final(md, &c);  
    OPENSSL_cleanse(&c, sizeof(c));  	
	char *CodeVerifier = base64Encode(md, 32);
	
	return CodeVerifier;
}  

void del_chr( char *s, char ch )
{
    char *t=s; //目标指针先指向原串头
    while( *s != '\0' ) //遍历字符串s
    {
        if ( *s != ch ) //如果当前字符不是要删除的，则保存到目标串中
            *t++=*s;
        s++ ; //检查下一个字符
    }
    *t='\0'; //置目标串结束符。
}

char* generateCodeVerifier() {
	int i;
	char byFlag = 1;
	char number[32] = {0};
	char *tempCodeVerifier = NULL;

	srand((unsigned) time(NULL));

	for (i = 0; i < 32; i++)
	{
		number[i] = rand() % 255;
	}

	tempCodeVerifier = base64Encode(number, 32);

	return tempCodeVerifier;
}

char* generateCodeChallenge(char *CodeVerifier, char *CodeChallengeMethod) {
	return hash256(CodeVerifier);
}

static unsigned long get_file_size(const char *path)
{
	int iRet = -1;
	struct stat buf;

	if (0 == stat(path, &buf))
	{
		return buf.st_size;
	}

	return iRet;
}

//clean config file and write new data
int writeConfigJsonData(const char *pJson)
{
	int iRet;
	unsigned int lock_file_times = 0;
	if(0 == access(CONFIG_PATH, 0)){
		FILE *fp = NULL;
		unsigned long wFilesize = 0;
		if((fp = fopen(CONFIG_PATH, "w")) == NULL){
			printf("file %s open error!\n", CONFIG_PATH);
			return -1;
		}
		
		while(flock(fileno(fp), LOCK_EX | LOCK_NB) < 0){
			if(++lock_file_times < LOCK_FILE_TRY_TIMES){
				usleep(10*1000);
			}else{
				return -1;
			}
		}
		
		int iLength = strlen(pJson);
		if (fwrite(pJson, 1, iLength, fp) != iLength) {
			printf("文件写入错误。!\n");
		} else {
			iRet = 0;
		}
		
		flock(fileno(fp), LOCK_UN);
		fclose(fp);
	}
	
	return iRet;
}


char* getConfigJsonData(char *pPath) 
{
	char *pData = NULL;
	unsigned int lock_file_times = 0;
	if(0 == access(pPath, 0)){
		FILE *fp = NULL;
		unsigned long wFilesize = 0;
		if((fp = fopen(pPath, "r")) == NULL){
			printf("file %s open error!\n", pPath);
			return NULL;
		}
LOCK_FILE:
		while(flock(fileno(fp), LOCK_SH | LOCK_NB) < 0){
			if(++lock_file_times < LOCK_FILE_TRY_TIMES){
				usleep(10*1000);
			}else{
				return NULL;
			}
		}
		wFilesize = get_file_size(pPath);
		if(wFilesize == 0 || wFilesize == -1){
			flock(fileno(fp), LOCK_UN);
			switch(wFilesize){
				case 0:
					if(++lock_file_times < LOCK_FILE_TRY_TIMES){
						usleep(10*1000);
						goto LOCK_FILE;
					}
					break;
				case -1:
					printf("file is not exist!\n");
					break;
				default:
					printf("===\n");
					break;
			}
			//printf("WARNING !!!!\nWARNING !!!!\nWARNING !!!!\nfile is empty %d!\n", ++read_fail_count);

			return NULL;
		}
		if((pData = malloc(wFilesize)) == NULL){		
			printf("malloc error!\n");
			flock(fileno(fp), LOCK_UN);
			return NULL;
		}
		fread(pData, 1, wFilesize, fp);

		flock(fileno(fp), LOCK_UN);
		fclose(fp);
	}
	
	return pData;
}

#if 0
int getConfigInfo(char *pJsonData, char **pProductId, char **pDevicesSerialNumber){
	int iRet = -1;
	struct json_object *root = NULL, *authDelegate = NULL, *ProductID = NULL, *DevicesSerialNumber = NULL;
	root = json_tokener_parse(pJsonData);
	/*if (is_error(root))
	{
		printf("json_tokener_parse root failed");
		return iRet;
	}*/
	authDelegate = json_object_object_get(root, "authDelegate");
	if (NULL != authDelegate){
		ProductID = json_object_object_get(authDelegate, "productId");
		if (NULL == ProductID){
			iRet = -2;
		} else {
			iRet = 0;
			*pProductId = strdup(json_object_get_string(ProductID));
		}

		DevicesSerialNumber = json_object_object_get(authDelegate, "deviceSerialNumber");
		if (NULL == DevicesSerialNumber){
			iRet = -2;
		} else {
			iRet = 0;
			*pDevicesSerialNumber = strdup(json_object_get_string(DevicesSerialNumber));
		}
	}

	json_object_put(root);
	free(pJsonData);

	return iRet;
}
#endif

void logInInfo_free(LOGIN_INFO_STRUCT *loginfo){
	if (loginfo->pCodeVerifier){
		free(loginfo->pCodeVerifier);
	}

	if (loginfo->pCodeChallenge){
		free(loginfo->pCodeChallenge);
	}

	if (loginfo->pProductId){
		free(loginfo->pProductId);
	}

	if (loginfo->pDeviceSerialNumber){
		free(loginfo->pDeviceSerialNumber);
	}

	if (loginfo){
		free(loginfo);
	}
}

// pDevice是网卡设备名  
char *getDevoceSerialNumber(char *pDevice) {
	unsigned char *pMac = NULL;
	unsigned char macaddr[ETH_ALEN];
	int i,s;  
	s = socket(AF_INET,SOCK_DGRAM,0);
	struct ifreq req;  
	int err,rc;  
	rc = strcpy(req.ifr_name, pDevice);
	err = ioctl(s, SIOCGIFHWADDR, &req); 
	close(s);  
	if (err != -1) {  
	 	memcpy(macaddr, req.ifr_hwaddr.sa_data, ETH_ALEN); //取输出的MAC地址  
		pMac = (char *)malloc(32);
		if (NULL != pMac) {
			for(i = 0; i < ETH_ALEN; i++) {
				sprintf(pMac+3*i, "%02X:", (unsigned char)req.ifr_hwaddr.sa_data[i]);
			}
			pMac[strlen(pMac) - 1] = 0;
		}
		else {
			return "123456";
		}
	}  
	return pMac;
}
LOGIN_INFO_STRUCT* logInInfo_new(){
	LOGIN_INFO_STRUCT* loginfo;
	loginfo = (LOGIN_INFO_STRUCT*)malloc(sizeof(LOGIN_INFO_STRUCT));
	if (loginfo != NULL){
		loginfo->pCodeChallengeMethod = CODECHALLENGEMETHOD;
		loginfo->pCodeVerifier = generateCodeVerifier();
		loginfo->pCodeChallenge = generateCodeChallenge(loginfo->pCodeVerifier, loginfo->pCodeChallengeMethod);
		del_chr(loginfo->pCodeVerifier, '=');
		del_chr(loginfo->pCodeChallenge, '=');

		loginfo->pProductId     = ReadConfigValueString("productId");
		loginfo->pDeviceSerialNumber = getDevoceSerialNumber("eth0");
	}

	return loginfo;
ERROR:
	logInInfo_free(loginfo);
	loginfo = NULL;
}

