//
// Created by nick on 17-12-6.
//

#ifndef DCS_DUERLINKCCHIPINSTANCE_H
#define DCS_DUERLINKCCHIPINSTANCE_H

#include <string>
#include <vector>
#include <netinet/in.h>
#include "duer_link/network_define_public.h"
#include "network_observer/network_status_observer.h"
#include "device_ctrl_observer/duer_link_received_data_observer.h"

#define NETWORK_SSID_CONFIG_HEAD        "#Basic configuration\n\ninterface=br0\nssid="
#define NETWORK_SSID_CONFIG_MIDDLE      "channel=6\nctrl_interface=/var/run/hostapd\n\n"\
                                        "#WPA and WPA2 configuration\n"\
                                        "macaddr_acl=0\nauth_algs=1\nignore_broadcast_ssid=0\n#wpa=2\n"\
                                        "#wpa_key_mgmt=WPA-PSK\n#wpa_passphrase="
#define NETWORK_SSID_CONFIG_END         "#rsn_pairwise=CCMP\n#wpa_pairwise=TKIP\n\n\n"\
                                        "#Hardware configuration\n\ndriver=nl80211\n"\
                                        "hw_mode=g\nieee80211n=1\n\n"

#define NETWORK_WPA_CONF_HEAD           "\nnetwork={\n\tssid=\""
#define NETWORK_WPA_CONF_MIDDLE         "\"\n\tpsk=\""
#define NETWORK_WPA_CONF_END            "\"\n\tkey_mgmt=WPA-PSK\n}\n"
#define DUERLINK_WPA_INSERT_FLAG        "country=GB"

#define DUERLINK_NETWORK_DEVICE_CCHIP_FOR_AP "br0"
#define DUERLINK_NETWORK_DEVICE_CCHIP_FOR_WORK "br1"

#define DUERLINK_WPA_HOSTAPD_CONFIG_FILE_PATH_CCHIP "/var/run/hostapd.conf"
#define DUERLINK_WPA_CONFIG_FILE_CCHIP "/var/run/wpa.conf"

#define SSID_PREFIX_HODOR       "Hodor"
#define SSID_PREFIX_STANDARD    "Standard"
#define SSID_PREFIX_INDIVIDUAL  "Individual"

#define DUERLINK_AUTO_CONFIG_TIMEOUT 1*60

#define PACKET_SIZE         4096
#define MAX_WAIT_TIME       1;//5
#define MAX_PACKETS_COUNT   4
#define MAX_PING_INTERVAL   300
#define PING_DEST_HOST1       "114.114.114.114"
#define PING_DEST_HOST2       "8.8.8.8"
#define DUERLINK_NETWORK_CONFIGURE_PING_COUNT 18

namespace duerOSDcsApp {
namespace application {

struct IcmpEchoReply {
    int icmpSeq;
    int icmpLen;
    int ipTtl;
    double rtt;
    std::string fromAddr;
    bool isReply;
};

struct PingResult {
    int dataLen;
    int nsend;
    int nreceived;
    char ip[32];
    std::string error;
    std::vector<IcmpEchoReply> icmpEchoReplys;
};

//network status
enum InternetConnectivity {
    UNAVAILABLE = 0,
    AVAILABLE,
    UNKNOW
};

//operation type
enum operation_type {
    EOperationStart = 10,
    EAutoEnd,
    EAutoConfig,
    EManualConfig
};

class NetWorkPingStatusObserver {
public:
    virtual void network_status_changed(InternetConnectivity status, bool wakeupTrigger) = 0;
};

class DuerLinkCchipInstance {
public:
    static DuerLinkCchipInstance* get_instance();

    static void destroy();

    void init_config_network_parameter(duerLink::platform_type speaker_type,
                                       duerLink::auto_config_network_type autoType,
                                       std::string devicedID,
                                       std::string interface);
    void init_softAp_env_cb();
    void init_ble_env_cb();
    void init_wifi_connect_cb();
    void init_discovery_parameter(std::string devicedID, std::string appId, std::string interface);

    bool set_wpa_conf(bool change_file);

    void set_network_observer(NetWorkPingStatusObserver *observer);
    bool ping_network(bool wakeupTrigger);
    void start_network_monitor();

    void set_networkConfig_observer(duerLink::NetworkConfigStatusObserver* config_listener);
    void set_monitor_observer(NetWorkPingStatusObserver *ping_listener);

    void start_network_recovery();
    void start_discover_and_bound();

    void set_dlp_data_observer(DuerLinkReceivedDataObserver* observer);

    void ble_client_connected();
    void ble_client_disconnected();
    bool ble_recv_data(void *data, int len);

    bool start_network_config(int timeout);
    void stop_network_config();

    bool send_dlp_msg_to_all_clients(const std::string& sendBuffer);

    bool send_dlp_msg_by_specific_session_id(const std::string& message, unsigned short sessionId);

    bool start_loudspeaker_ctrl_devices_service(duerLink::device_type device_type_value,
                                                const std::string& client_id,
                                                const std::string& message);

    bool start_loudspeaker_ctrl_devices_service_by_device_id(const std::string& device_id);

    bool send_msg_to_devices_by_spec_type(const std::string& msg_buf,
                                          duerLink::device_type device_type_value);
    bool disconnect_devices_connections_by_spe_type(duerLink::device_type device_type_value,
                                                    const std::string& message);

    bool get_curret_connect_device_info(std::string& client_id,
                                        std::string& device_id,
                                        duerLink::device_type device_type_value);

    void init_network_config_timeout_alarm();
    void start_network_config_timeout_alarm(int timeout);
    void stop_network_config_timeout_alarm();

    inline operation_type get_operation_type() {return m_operation_type;};
    inline void set_operation_type(operation_type type) {m_operation_type = type;};

private:
    DuerLinkCchipInstance();
    virtual ~DuerLinkCchipInstance();

    bool is_first_network_config(std::string path);

    static void sigalrm_fn(int sig);

    unsigned short getChksum(unsigned short *addr,int len);
    int packIcmp(int pack_no, struct icmp* icmp);
    bool unpackIcmp(char *buf,int len, struct IcmpEchoReply *icmpEchoReply);
    struct timeval timevalSub(struct timeval timeval1,struct timeval timval2);
    bool getsockaddr(const char * hostOrIp, sockaddr_in* sockaddr);
    bool sendPacket();
    bool recvPacket(PingResult &pingResult);
    bool ping(std::string host, int count, PingResult& pingResult);
    static void *monitor_work_routine(void *arg);
    bool check_recovery_network_status();

private:
    static DuerLinkCchipInstance* m_duerLink_cchip_instance;

    char m_sendpacket[PACKET_SIZE];
    char m_recvpacket[PACKET_SIZE];
    int m_maxPacketSize;
    int m_sockfd;
    int m_datalen;
    int m_nsend;
    int m_nreceived;
    int m_icmp_seq;
    struct sockaddr_in m_dest_addr;
    struct sockaddr_in m_from_addr;
    pid_t m_pid;
    pthread_mutex_t m_ping_lock;

    NetWorkPingStatusObserver *m_pObserver;
    duerLink::NetworkConfigStatusObserver *m_pCchipConfigObserver;

    operation_type m_operation_type;
};

}
}

#endif //DCS_DUERLINKCCHIPINSTANCE_H
