#include <stdio.h>
#include <string.h>
#include "product.h"
#include "product_config.h"

//TODO: update these product info
#ifdef CONFIG_SDS
#define product_model           "OPENALINK_LIVING_LIGHT_SDS_TEST"
#else
#define product_model           "ALINKTEST_LIVING_LIGHT_ALINK_TEST"
#endif
#define product_key             "1L6ueddLqnRORAQ2sGOL"
#define product_secret          "qfxCLoc1yXEk9aLdx5F74tl1pdxl0W0q7eYOvvuo"
#define product_debug_key       "dpZZEpm9eBfqzK7yVeLq"
#define product_debug_secret    "THnfRRsU5vu6g6m9X6uFyAjUWflgZ0iyGjdEneKm"
//get device_key & device_secret from https://iot.aliyun.com/
//each device have unique device_key & device_secret

/*SN:1234567896860*/
//#define device_key              "hWhW6TUmnZ7Oe4xIexla"
//#define device_secret           "O0ykSCxcWxcyw7AayYx8yIQWSZKYf9ni"


/*SN:8005dfc3fce0*/
//#define device_key              "3XPOlDt8n8KKEDHgE1tz"
//#define device_secret           "5IZSOziiPXp4B5sRtUWsaVdEVMAdiE15"


/*SN:8005dfc3fd01*/
#define device_key              "gONfSjtktMjpG1xzsEIQ"
#define device_secret           "53BqNwdHTMGdI1VEqa3jrOyVwDSZvWFc"


/*SN:8005dfc3fce9*/
//#define device_key              "uiajIVz7Whu9LoTPXTfR"
//#define device_secret          "BPdDs79ZyfxWTA63lJYO1Pd6hlIlPPmZ"


/*SN:8c882b001172*/
#define device_key1              "jlU0ZzSqaJvxsN4RFr48"
#define device_secret1           "QSq34gBCVYUDOqJFkNlcPUTAb0xHZYLy"

/*SN:8c882b001175*/
#define device_key2              "VxtUMTAtJa4prMj4miXg"
#define device_secret2          "655up5P3akcivY12qYax7IgeWZxhNMo9"

/*SN:8c882b001178*/
#define device_key3              "SVxWwCYbZNl8rAplcPJP"
#define device_secret3          "nppecfgyNEJIcf8OJcCwnLGnnK8ouHD8"

/*SN:8c882b00117b*/
#define device_key4              "ZMNbR5B9ecF6mRKQtXsV"
#define device_secret4          "1H2bmxRrG6uuiRpLlVwnqnfrEAfHW7XF"

/*SN:8c882b001187*/
#define device_key5              "OzuYKGp5Gp9afRUcERYs"
#define device_secret5           "4Nb0uhB8fLPa3MpPIoimFBnUl43c4Y1T"


unsigned char wifi_mac[18] = {0};


#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"


char *product_get_name(char name_str[PRODUCT_NAME_LEN])
{
	return strncpy(name_str, "alink_product", PRODUCT_NAME_LEN);
}

char *product_get_version(char ver_str[PRODUCT_VERSION_LEN])
{
    char buf[32] = {0};
    cdb_get("$sw_build",buf);
	return strncpy(ver_str, buf, PRODUCT_VERSION_LEN);
}

char *product_get_model(char model_str[PRODUCT_MODEL_LEN])
{
	return strncpy(model_str, product_model, PRODUCT_MODEL_LEN);
}

char *product_get_key(char key_str[PRODUCT_KEY_LEN])
{
	return strncpy(key_str, product_key, PRODUCT_KEY_LEN);
}

char *product_get_device_key(char key_str[DEVICE_KEY_LEN])
{
 	if (0 == strcmp(wifi_mac,"8c882b001172"))
		return strncpy(key_str, device_key1, DEVICE_KEY_LEN);
	else if (0 == strcmp(wifi_mac,"8c882b001175"))
		return strncpy(key_str, device_key2, DEVICE_KEY_LEN);
	if (0 == strcmp(wifi_mac,"8c882b001178"))
		return strncpy(key_str, device_key3, DEVICE_KEY_LEN);
	else if (0 == strcmp(wifi_mac,"8c882b00117b"))
		return strncpy(key_str, device_key4, DEVICE_KEY_LEN);
	else if (0 == strcmp(wifi_mac,"8c882b001187"))
		return strncpy(key_str, device_key5, DEVICE_KEY_LEN);
	else	
		return strncpy(key_str, device_key, DEVICE_KEY_LEN);
}

char *product_get_device_secret(char secret_str[DEVICE_SECRET_LEN])
{
 	if (0 == strcmp(wifi_mac,"8c882b001172"))
		return strncpy(secret_str, device_secret1, DEVICE_SECRET_LEN);
	else if (0 == strcmp(wifi_mac,"8c882b001175"))
		return strncpy(secret_str, device_secret2, DEVICE_SECRET_LEN);
	if (0 == strcmp(wifi_mac,"8c882b001178"))
		return strncpy(secret_str, device_secret3, DEVICE_SECRET_LEN);
	else if (0 == strcmp(wifi_mac,"8c882b00117b"))
		return strncpy(secret_str, device_secret4, DEVICE_SECRET_LEN);
	else if (0 == strcmp(wifi_mac,"8c882b001187"))
		return strncpy(secret_str, device_secret5, DEVICE_SECRET_LEN);
	else
		return strncpy(secret_str, device_secret, DEVICE_SECRET_LEN);
}

char *product_get_secret(char secret_str[PRODUCT_SECRET_LEN])
{
	return strncpy(secret_str, product_secret, PRODUCT_SECRET_LEN);
}

char *product_get_debug_key(char key_str[PRODUCT_KEY_LEN])
{
	return strncpy(key_str, product_debug_key, PRODUCT_KEY_LEN);
}

char *product_get_debug_secret(char secret_str[PRODUCT_SECRET_LEN])
{
	return strncpy(secret_str, product_debug_secret, PRODUCT_SECRET_LEN);
}

#if 1

char *product_get_sn(char sn_str[PRODUCT_SN_LEN])
{
	return strncpy(sn_str, "8005dfc3fd01", PRODUCT_SN_LEN);
}
#else
char *product_get_sn(char sn_str[PRODUCT_SN_LEN])
{
	char *getMac = NULL;
	unsigned char mac[6] = {0};

	///getMac = WIFIAudio_UciConfig_SearchValueString("xzxwifiaudio.config.mac_addr");
	if(getMac != NULL)
	{
		sscanf(getMac,  "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    	sprintf(wifi_mac, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4],mac[5]);
       printf( "getMac = %s -----mac %s\n", getMac,wifi_mac);
		return strncpy(sn_str, wifi_mac, PRODUCT_SN_LEN);	
	}
	
	return strncpy(sn_str, "1234567896860", PRODUCT_SN_LEN);
}
#endif

