#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdbool.h>
#include <libutils/wlutils.h>
#include "wdk.h"

int wdk_ap(int argc, char **argv) 
{
	if (argc < 1)
		return -1;
	
	if (str_equal(argv[0], "scan"))
		wifi_scan();
	else if (str_equal(argv[0], "result"))
	{
		if (f_exists(SCANRESULT))
			dump_file(SCANRESULT);
	}
	else if (str_equal(argv[0], "apply"))
	{
		char ret, cnt;
		
		/*No SSID*/
		if(argc < 2)
			return -1;
		for (cnt = 0; cnt < SCAN_RETRY_CNT; cnt++)
		{
			char *ssid, *pass=NULL;
			
			/*command: /lib/wdk/ap apply ssid*/
			ssid = argv[1];
			if(argc > 2) {
				pass = argv[2];
				if(strlen(pass) == 0)
					pass= NULL;
			}
			ret = wifi_set_ssid_pwd(ssid, pass);
			if (ret == SCAN_AP_NOT_FOUND)
				;
			else
				return ret;
		}
	}
	
	return 0;
}
