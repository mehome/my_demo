#ifndef __MONTAGE_API_H__
#define __MONTAGE_API_H__
#include <stdbool.h>
#include <unistd.h>



#define VERIFY_DEVICE "br1"


#ifdef __cplusplus
extern "C"
{
#endif



#define cprintf(fmt, args...) \
do { \
    FILE *fp = fopen("/dev/console", "w"); \
    if (fp) { \
        fprintf(fp, fmt , ## args); \
        fclose(fp); \
    } \
} while (0)

#define printf cprintf




typedef enum e_network_status
{
	m_start_config_network				= 0x1,
	m_get_passwd_start_connect_router,
	m_connect_router_complete,
	m_connect_router_failed,
	m_passwd_error_reconnect,
	m_cannot_get_ip,
	m_get_ip_success,
	m_add_device_complete,
	m_add_device_fail,
	m_upgrade_start,
	m_upgrade_success,
	m_upgrade_fail


}network_status;

int play_notice_voice(network_status status);


#ifdef __cplusplus
}
#endif
#endif
