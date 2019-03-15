//
// Created by nick on 17-12-6.
//

#include <string>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include "LoggerUtils/DcsSdkLogger.h"
#include "DCSApp/DuerLinkCchipInstance.h"
#include "DCSApp/Configuration.h"
#include "DCSApp/DeviceIoWrapper.h"
#include "duer_link/duer_link_sdk.h"
#include "duer_link/network_define_public.h"

#include <DeviceTools/PrintTickCount.h>

namespace duerOSDcsApp {
namespace application {

using namespace duerLink;

using std::string;
using std::vector;
using std::ifstream;

static string m_target_ssid;
static string m_target_pwd;
static string m_target_ssid_prefix;
static bool m_ble_is_opened = false;
static int m_ping_interval = 1;
static int m_network_status = 0;

DuerLinkCchipInstance* DuerLinkCchipInstance::m_duerLink_cchip_instance = nullptr;

bool system_command(const char* cmd) {
	trc(__func__);

    pid_t status = 0;
    bool ret_value = false;

    APP_INFO("System [%s]", cmd);

    status = system(cmd);

    if (-1 == status) {
        APP_ERROR("system failed!");
    } else {
        if (WIFEXITED(status)) {
            if (0 == WEXITSTATUS(status)) {
                ret_value = true;
            } else {
                APP_ERROR("System shell script failed:[%d].", WEXITSTATUS(status));
            }
        } else {
            APP_INFO("System status = [%d]", WEXITSTATUS(status));
        }
    }

    return ret_value;
}

bool check_br0_interface_status(string ap) {
	trc(__func__);

        int sockfd;
        bool ret = false;
        struct ifreq ifr_mac;

        if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
            APP_ERROR("socket create failed.");
            return false;
        }

        memset(&ifr_mac,0,sizeof(ifr_mac));
        strncpy(ifr_mac.ifr_name, ap.c_str(), sizeof(ifr_mac.ifr_name)-1);

        if ((ioctl(sockfd, SIOCGIFHWADDR, &ifr_mac)) < 0) {
            APP_ERROR("Mac ioctl failed.");
        } else {
            APP_INFO("Mac ioctl suceess.");
            ret = true;
        }
        close(sockfd);

        return ret;
}

bool tmp_wpa_conf(const string ssid, const string pwd, const string path) {
	trc(__func__);

    string tmp_conf_path;
    string bk_conf_path;
    string file_conf_path;

    if (ssid.empty() || pwd.empty() || path.empty()){
        APP_ERROR("Input param is empty.");
        return false;
    }

    tmp_conf_path = path + ".tmp";
    bk_conf_path = path + ".bk";
    file_conf_path = path;

    //如果空密码,修改此处
    string insert_ct = NETWORK_WPA_CONF_HEAD;
    insert_ct += ssid;
    insert_ct += NETWORK_WPA_CONF_MIDDLE;
    insert_ct += pwd;
    insert_ct += NETWORK_WPA_CONF_END;

    APP_INFO("Wpa_supplicant.conf insert CT [%s]", insert_ct.c_str());

    deviceCommonLib::deviceTools::printTickCount("network_config cp wpa.conf begin.");

    string cmd_buff = "cp ";
    cmd_buff += file_conf_path + " " + bk_conf_path;
    cmd_buff += " &";

    if (!system_command(cmd_buff.c_str())) {
        APP_ERROR("Bake Wpa_supplicant.conf failed.");
        return false;
    }

    deviceCommonLib::deviceTools::printTickCount("network_config cp wpa.conf end.");

    sync();

    deviceCommonLib::deviceTools::printTickCount("network_config sync1 wpa.conf end.");

    APP_INFO("Bake Wpa_supplicant.conf successed.");

    string line;
    std::ifstream in(file_conf_path);
    std::ofstream out(tmp_conf_path);

    while (getline(in, line)) {
        if (strstr(line.c_str(), DUERLINK_WPA_INSERT_FLAG)){
            out << line << std::endl;
            out << insert_ct << std::endl;
            break;
        } else {
            out << line << std::endl;
        }
    }

    in.close();
    out.close();

    deviceCommonLib::deviceTools::printTickCount("network_config check wpa.conf end.");

    cmd_buff.clear();
    cmd_buff = "mv ";
    cmd_buff += tmp_conf_path + " " + file_conf_path;
    cmd_buff += " &";

    if (!system_command(cmd_buff.c_str())) {
        APP_ERROR("Update wpa_supplicant.conf failed.");
        return false;
    }

    deviceCommonLib::deviceTools::printTickCount("network_config update wpa.conf end.");

    sync();

    deviceCommonLib::deviceTools::printTickCount("network_config sync2 wpa.conf end.");

    APP_INFO("Update wpa_supplicant.conf successed.");

    return true;
}

bool start_wpa_supplicant() {
	trc(__func__);


    deviceCommonLib::deviceTools::printTickCount("network_config connect_ap begin.");

    system_command("echo 1 > /dev/wmtWifi &");

    sleep(1);

    system_command("/usr/bin/connect_ap.sh &");

    deviceCommonLib::deviceTools::printTickCount("network_config connect_ap end.");

    return true;
}

bool stop_wpa_supplicant() {
	trc(__func__);

    return system_command("killall wpa_supplicant &");
}

bool set_ap_tmp_ipaddr() {
	trc(__func__);

    if (!check_br0_interface_status(DUERLINK_NETWORK_DEVICE_CCHIP_FOR_AP)) {
        APP_INFO("%s is down.", DUERLINK_NETWORK_DEVICE_CCHIP_FOR_AP);
        return false;
    }

    return system_command("ifconfig br0 192.168.1.1 netmask 255.255.255.0 &");
}

bool delete_tmp_addr() {
	trc(__func__);

    if (!check_br0_interface_status(DUERLINK_NETWORK_DEVICE_CCHIP_FOR_AP)) {
        APP_INFO("%s is down.", DUERLINK_NETWORK_DEVICE_CCHIP_FOR_AP);
        return true;
    }

    return system_command("ip addr delete 192.168.1.1 dev br0 &");
}

bool echo_ap_to_devwifi() {
	trc(__func__);

    return system_command("echo AP > /dev/wmtWifi &");
}

bool stop_ap_mode() {
	trc(__func__);

    return system_command("killall hostapd &");
}

bool start_ap_mode() {
	trc(__func__);

    string cmd = " ";
    cmd += DUERLINK_WPA_HOSTAPD_CONFIG_FILE_PATH_CCHIP;
    cmd += " &";

    if (stop_ap_mode()) {
        APP_INFO("[start_ap_mode] hostapd is ruuning.");
    }
    sleep(1);

    string tmp = "hostapd ";
    tmp += cmd;

    return system_command(tmp.c_str());
}

bool starup_ap_interface() {
	trc(__func__);

    if (check_br0_interface_status(DUERLINK_NETWORK_DEVICE_CCHIP_FOR_AP)) {
        APP_INFO("%s is up.", DUERLINK_NETWORK_DEVICE_CCHIP_FOR_AP);

        return true;
    }

    return system_command("ifconfig br0 up &");
}

bool down_ap_interface() {
	trc(__func__);

    if (!check_br0_interface_status(DUERLINK_NETWORK_DEVICE_CCHIP_FOR_AP)) {
        APP_INFO("%s is down.", DUERLINK_NETWORK_DEVICE_CCHIP_FOR_AP);
        return true;
    }

    return system_command("ifconfig br0 down &");
}

bool starup_br1_interface() {
	trc(__func__);
    return system_command("ifconfig br1 up &");
}

bool down_br1_interface() {
	trc(__func__);

    if (!check_br0_interface_status(DUERLINK_NETWORK_DEVICE_CCHIP_FOR_AP)) {
        APP_INFO("%s is down.", DUERLINK_NETWORK_DEVICE_CCHIP_FOR_AP);
        return true;
    }

    system_command("ifconfig br1 0.0.0.0");

    return system_command("ifconfig br1 down &");
}

bool stop_dhcp_server() {
	trc(__func__);

    return system_command("killall dnsmasq &");
}

bool start_dhcp_server() {
	trc(__func__);

    if (stop_dhcp_server()) {
        APP_INFO("[Start_dhcp_server] dnsmasq is killed.");
    }
    sleep(1);

    return system_command("dnsmasq &");
}

bool change_hostapd_conf(const string ssid) {
	trc(__func__);

    string conf_ct_all = NETWORK_SSID_CONFIG_HEAD;
    conf_ct_all += ssid;
    conf_ct_all += "\n";
    conf_ct_all += NETWORK_SSID_CONFIG_MIDDLE;
    conf_ct_all += "\n";
    conf_ct_all += NETWORK_SSID_CONFIG_END;

    APP_INFO("Hostapd:\n%s", conf_ct_all.c_str());

    string ap_hstapd_temp_confile_path = DUERLINK_WPA_HOSTAPD_CONFIG_FILE_PATH_CCHIP;
    ap_hstapd_temp_confile_path += ".tmp";

    std::ofstream out(ap_hstapd_temp_confile_path.c_str());
    out << conf_ct_all << std::endl;
    out.close();

    string command_mv_string = ap_hstapd_temp_confile_path + " " + DUERLINK_WPA_HOSTAPD_CONFIG_FILE_PATH_CCHIP;
    string tmp = "mv ";
    tmp += command_mv_string;
    tmp += " &";

    return system_command(tmp.c_str());
}

bool get_device_interface_mac(string &mac_address) {
	trc(__func__);

    int sock_mac;
    struct ifreq ifr_mac;
    char mac_addr[30] = {0};

    sock_mac = socket( AF_INET, SOCK_STREAM, 0 );
    if (sock_mac == -1) {
        APP_ERROR("create mac socket failed.");
        return false;
    }

    memset(&ifr_mac,0,sizeof(ifr_mac));
    strncpy(ifr_mac.ifr_name, DUERLINK_NETWORK_DEVICE_CCHIP_FOR_WORK, sizeof(ifr_mac.ifr_name)-1);

    if ((ioctl( sock_mac, SIOCGIFHWADDR, &ifr_mac)) < 0) {
        APP_ERROR("Mac socket ioctl failed.");
        close(sock_mac);
        return false;
    }

    sprintf(mac_addr,"%02X%02X",
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[4],
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[5]);

    APP_INFO("local mac:%s",mac_addr);

    close(sock_mac);

    mac_address = mac_addr;

    std::transform(mac_address.begin(), mac_address.end(), mac_address.begin(), toupper);

    return true;
}

void set_softAp_ssid_and_pwd(string hostapd_config_file_path) {
	trc(__func__);

    // hostapd configuration file path
    string m_ap_hostapd_conffile_path = hostapd_config_file_path;

    string ssid;
    string suffix;
    string mac_address;

    m_target_ssid_prefix = Configuration::getInstance()->getSsidPrefix();

    APP_INFO("Configure SSID Prefix: [%s].", m_target_ssid_prefix.c_str());

    get_device_interface_mac(mac_address);
    suffix += mac_address;

    ssid = m_target_ssid_prefix + suffix;
    change_hostapd_conf(ssid);

    return ;
}

bool softap_prepare_env_cb() {
	trc(__func__);
	my_popen("cdb set dhcps_start 192.168.1.2");
	my_popen("cdb set dhcps_end 192.168.1.254");
	my_popen("cdb set lan_gw 192.168.1.1");
	my_popen("cdb set lan_ip 192.168.1.1");
	my_popen("cdb set op_work_mode 1");
	my_popen("mtc commit");
	return true;
}
 
 bool softap_destroy_env_cb() {
 	trc(__func__);
 	my_popen("cdb set dhcps_start 10.10.10.1");
 	my_popen("cdb set dhcps_end 10.10.10.253");
	my_popen("cdb set lan_gw 10.10.10.254");
 	my_popen("cdb set lan_ip 10.10.10.254");
	my_popen("mtc lan restart");
 	return true;
 }

bool platform_connect_wifi_cb(const char *ssid,
                              const int ssid_len,
                              const char *pwd,
                              const int pwd_len) {
	trc(__func__);
	my_popen("/lib/wdk/omnicfg_apply 9 '%s' '%s'",ssid,pwd);
	return true;

}

bool ble_prepare_env_cb() {
	trc(__func__);

    APP_INFO("prepare ble environment resource.");

    m_ble_is_opened = framework::DeviceIo::getInstance()->controlBt(framework::BtControl::BT_IS_OPENED);
    if (!m_ble_is_opened) {
        APP_ERROR("Open bluetooth.");
        framework::DeviceIo::getInstance()->controlBt(framework::BtControl::BT_OPEN);
    }

    framework::DeviceIo::getInstance()->controlBt(framework::BtControl::BLE_OPEN_SERVER);

    APP_INFO("End prepare ble environment resource.");

    return true;
}

bool ble_destroy_env_cb() {
	trc(__func__);

    APP_INFO("destroy ble environment resource.");

    framework::DeviceIo::getInstance()->controlBt(framework::BtControl::BLE_CLOSE_SERVER);

    if (!m_ble_is_opened) {
        APP_INFO("Close bluetooth.");
        framework::DeviceIo::getInstance()->controlBt(framework::BtControl::BT_CLOSE);
        APP_INFO("Close led.");
//        DeviceIoWrapper::getInstance()->ledBtOff();
    }

    APP_INFO("End destroy ble environment resource.");

    return true;
}

int ble_send_data_cb(char *data, unsigned short data_length) {
	trc(__func__);
	int i = 0;


    APP_INFO("ble send data: %s, len: %d", data, data_length);
	printf("sending:");
	for(i = 0; i < data_length; i++){
		printf("%hhx ", data[i]);
	}
	printf("\n");

    framework::DeviceIo::getInstance()->controlBt(framework::BtControl::BLE_SERVER_SEND,
                                                  data,
                                                  data_length);

    APP_INFO("End ble send data.");

    return 0;
}

DuerLinkCchipInstance::DuerLinkCchipInstance() {
	trc(__func__);

    m_maxPacketSize = MAX_PACKETS_COUNT;
    m_datalen = 56;
    m_nsend = 0;
    m_nreceived = 0;
    m_icmp_seq = 0;
    m_pCchipConfigObserver = NULL;

    m_operation_type = operation_type::EOperationStart;

    pthread_mutex_init(&m_ping_lock, NULL);

    init_network_config_timeout_alarm();
}

DuerLinkCchipInstance::~DuerLinkCchipInstance() {
    pthread_mutex_destroy(&m_ping_lock);

    duerLinkSdk::get_instance()->destroy();
}

DuerLinkCchipInstance* DuerLinkCchipInstance::get_instance() {
    if (NULL == m_duerLink_cchip_instance) {
        m_duerLink_cchip_instance = new DuerLinkCchipInstance();
    }

    return m_duerLink_cchip_instance;
}

void DuerLinkCchipInstance::destroy() {
	trc(__func__);

    if (NULL != m_duerLink_cchip_instance) {
        delete m_duerLink_cchip_instance;
        m_duerLink_cchip_instance = NULL;
    }
}

bool DuerLinkCchipInstance::set_wpa_conf(bool change_file) {
	trc(__func__);
	return true;
//    int skip_line = 0;
//    string tmp_conf_path;
//    string bk_conf_path;
//    string file_conf_path;
//    string cmd_buff;
//
//    if (m_target_ssid.empty() || m_target_pwd.empty()){
//        APP_ERROR("Input param is empty.");
//        return false;
//    }
//
//    tmp_conf_path = DUERLINK_WPA_CONFIG_FILE_CCHIP;
//    tmp_conf_path += ".tmp";
//    bk_conf_path = DUERLINK_WPA_CONFIG_FILE_CCHIP;
//    bk_conf_path += ".bk";
//    file_conf_path = DUERLINK_WPA_CONFIG_FILE_CCHIP;
//
//    if (!change_file) {
//        cmd_buff.clear();
//        cmd_buff = "mv ";
//        cmd_buff += bk_conf_path + " " + file_conf_path;
//        cmd_buff += " &";
//
//        if (!system_command(cmd_buff.c_str())) {
//            APP_ERROR("Reset wpa_supplicant.conf failed.");
//            return false;
//        }
//        sync();
//
//        APP_INFO("Reset wpa_supplicant.conf successed.");
//
//        return false;
//    }
//
//    string insert_ct = NETWORK_WPA_CONF_HEAD;
//    insert_ct += m_target_ssid;
//    insert_ct += NETWORK_WPA_CONF_MIDDLE;
//    insert_ct += m_target_pwd;
//    insert_ct += NETWORK_WPA_CONF_END;
//
//    string line;
//    std::ifstream in(bk_conf_path);
//    std::ofstream out(tmp_conf_path);
//
//    //read file head
//    while (getline (in, line)) {
//        if (strstr(line.c_str(), DUERLINK_WPA_INSERT_FLAG)){
//            out << line << std::endl;
//            out << insert_ct << std::endl;
//            break;
//        } else {
//            out << line << std::endl;
//        }
//    }
//
//    //read file body
//    vector<string> body;
//    string tmp_ssid = "\"" + m_target_ssid + "\"";
//    while (getline (in, line)) {
//        if (strstr(line.c_str(), tmp_ssid.c_str())) {
//            APP_ERROR("Set ssid is already exists.");
//            body.pop_back();
//            skip_line = 3;
//            continue;
//        }
//
//        if (skip_line != 0) {
//            skip_line --;
//            continue;
//        } else {
//            body.push_back(line);
//        }
//    }
//
//    for (unsigned int i = 0; i < body.size(); i++) {
//        out << body[i] << std::endl;
//    }
//
//    in.close();
//    out.close();
//
//    cmd_buff.clear();
//    cmd_buff = "mv ";
//    cmd_buff += tmp_conf_path + " " + file_conf_path;
//    cmd_buff += " &";
//
//    if (!system_command(cmd_buff.c_str())) {
//        APP_ERROR("Update Wpa_supplicant.conf failed.");
//        return false;
//    }
//    sync();
//
//    APP_INFO("Update Wpa_supplicant.conf successed.");
//
//    return true;
}

bool DuerLinkCchipInstance::is_first_network_config(string path) {
	trc(__func__);

    ifstream it_stream;
    int length = 0;
    string wpa_config_file = path;

    it_stream.open(wpa_config_file.c_str());
    if (!it_stream.is_open()) {
        APP_ERROR("wpa config file open error.");
        return false;
    }

    it_stream.seekg(0,std::ios::end);
    length = it_stream.tellg();
    it_stream.seekg(0,std::ios::beg);

    char *buffer = new char[length];
    it_stream.read(buffer, length);
    it_stream.close();

    char * position = nullptr;
    position = strstr(buffer,"ssid");
    delete [] buffer;
    buffer = nullptr;

    if (nullptr == position) {
        APP_ERROR("First network config.");
        return true;
    }

    APP_INFO("Not first network config.");

    return false;
}

void DuerLinkCchipInstance::sigalrm_fn(int sig) {
	trc(__func__);

    APP_INFO("alarm is run.");

    get_instance()->m_operation_type = operation_type::EAutoEnd;

    get_instance()->stop_network_config_timeout_alarm();
    duerLinkSdk::get_instance()->stop_network_config();
}

void DuerLinkCchipInstance::init_network_config_timeout_alarm() {
	trc(__func__);

    APP_INFO("set alarm.");

    signal(SIGALRM, sigalrm_fn);
}

void DuerLinkCchipInstance::start_network_config_timeout_alarm(int timeout) {
	trc(__func__);

    APP_INFO("start alarm.");

    alarm(timeout);
}

void DuerLinkCchipInstance::stop_network_config_timeout_alarm() {
	trc(__func__);

    APP_INFO("stop alarm.");

    alarm(0);
}

void DuerLinkCchipInstance::init_softAp_env_cb() {
	trc(__func__);

    duerLinkSdk::get_instance()->init_platform_softAp_control(softap_prepare_env_cb,
                                                              softap_destroy_env_cb);
}

void DuerLinkCchipInstance::init_ble_env_cb() {
	trc(__func__);

    duerLinkSdk::get_instance()->init_platform_ble_control(ble_prepare_env_cb,
                                                           ble_destroy_env_cb,
                                                           ble_send_data_cb);
}

void DuerLinkCchipInstance::init_wifi_connect_cb() {
	trc(__func__);

    duerLinkSdk::get_instance()->init_platform_connect_control(platform_connect_wifi_cb);
}

void DuerLinkCchipInstance::start_network_recovery() {
	trc(__func__);

#ifdef Box86
    check_recovery_network_status();
#else
    if (is_first_network_config(DUERLINK_WPA_CONFIG_FILE_CCHIP)) {
        get_instance()->m_operation_type = operation_type::EAutoConfig;

        start_network_config_timeout_alarm(DUERLINK_AUTO_CONFIG_TIMEOUT);
        duerLinkSdk::get_instance()->start_network_config();
    } else {
        check_recovery_network_status();
    }
#endif
}

void DuerLinkCchipInstance::start_discover_and_bound() {
	trc(__func__);

    duerLinkSdk::get_instance()->start_device_discover_and_bound();
}

void DuerLinkCchipInstance::init_config_network_parameter(platform_type speaker_type,
                                                        auto_config_network_type autoType,
                                                        string devicedID,
                                                        string interface) {
    duerLinkSdk::get_instance()->init_config_network_parameter(speaker_type,
                                                               autoType,
                                                               devicedID,
                                                               interface);
}

void DuerLinkCchipInstance::init_discovery_parameter(string devicedID,
                                                   string appId,
                                                   string interface) {
    //test cuid and access token for linkplay
//    duerLinkSdk::get_instance()->set_config_json_file("/data/duer/duerLink_config.json");

    duerLinkSdk::get_instance()->init_discovery_parameter(devicedID, appId, interface, "./bduss.txt");

}

void DuerLinkCchipInstance::set_networkConfig_observer(NetworkConfigStatusObserver* config_listener) {
	trc(__func__);

    if (config_listener) {
        m_pCchipConfigObserver = config_listener;
    }
    duerLinkSdk::get_instance()->set_networkConfig_observer(config_listener);
}

void DuerLinkCchipInstance::set_monitor_observer(NetWorkPingStatusObserver *ping_listener) {
	trc(__func__);

    if (ping_listener) {
        m_pObserver = ping_listener;
    }

    APP_INFO("Set monitor observer.");
}

bool DuerLinkCchipInstance::check_recovery_network_status() {
	trc(__func__);

#ifdef CCHIP8198
    deviceCommonLib::deviceTools::printTickCount("network_config main_thread recovery begin.");
#endif

    if (nullptr != m_pCchipConfigObserver) {
        (m_pCchipConfigObserver)->notify_network_config_status(duerLink::ENetworkRecoveryStart);
    }

    int network_check_count = 0;
    bool recovery = false;

    while(!(recovery = ping_network(false))) {
        if (network_check_count == DUERLINK_NETWORK_CONFIGURE_PING_COUNT) {
            APP_ERROR("Network recovery ping failed.");

            break;
        }

        sleep(1);
        network_check_count++;
    }

    if (recovery) {
        if (nullptr != m_pCchipConfigObserver) {
            APP_INFO("Network recovery succed.");

            (m_pCchipConfigObserver)->notify_network_config_status(duerLink::ENetworkRecoverySucceed);
        }
        return true;
    } else {
        if (nullptr != m_pCchipConfigObserver) {
            APP_ERROR("Network recovery failed.");

            (m_pCchipConfigObserver)->notify_network_config_status(duerLink::ENetworkRecoveryFailed);
        }

        return false;
    }
}

void DuerLinkCchipInstance::ble_client_connected() {
	trc(__func__);

    duerLinkSdk::get_instance()->ble_client_connected();
}

void DuerLinkCchipInstance::ble_client_disconnected() {
	trc(__func__);

    duerLinkSdk::get_instance()->ble_client_disconnected();
}

bool DuerLinkCchipInstance::ble_recv_data(void *data, int len) {
	trc(__func__);
	int i = 0;
	printf("recving:");
	for(i = 0; i < len; i++){
		printf("%hhx ", ((char *)data)[i]);
	}
	printf("\n");

    return duerLinkSdk::get_instance()->ble_recv_data(data, len);
}

bool DuerLinkCchipInstance::start_network_config(int timeout) {
	trc(__func__);
	if(m_operation_type!=operation_type::EAutoEnd){
			stop_network_config();
	}
    APP_INFO("start configing %d.", timeout);

    m_operation_type = operation_type::EManualConfig;

    start_network_config_timeout_alarm(timeout);
	duerLinkSdk* pduerLinkSdk_ins=duerLinkSdk::get_instance();
    return pduerLinkSdk_ins->start_network_config();
}

void DuerLinkCchipInstance::stop_network_config() {
	trc(__func__);

    APP_INFO("start stopping.");

    m_operation_type = operation_type::EAutoEnd;

    stop_network_config_timeout_alarm();
    duerLinkSdk::get_instance()->stop_network_config();
}

void DuerLinkCchipInstance::set_dlp_data_observer(DuerLinkReceivedDataObserver* observer) {
	trc(__func__);

    duerLinkSdk::get_instance()->set_dlp_data_observer(observer);
}

bool DuerLinkCchipInstance::send_dlp_msg_to_all_clients(const string& sendBuffer) {
	trc(__func__);

    return duerLinkSdk::get_instance()->send_dlp_msg_to_all_clients(sendBuffer);
}

bool DuerLinkCchipInstance::send_dlp_msg_by_specific_session_id(
    const string& message, unsigned short sessionId) {
    return duerLinkSdk::get_instance()->send_dlp_msg_to_client_by_specific_id(message, sessionId);
}

bool DuerLinkCchipInstance::start_loudspeaker_ctrl_devices_service(
    device_type device_type_value, const string& client_id, const std::string& message) {
    return duerLinkSdk::get_instance()->start_loudspeaker_ctrl_devices_service(device_type_value,
                                                                               client_id,
                                                                               message);
}

bool DuerLinkCchipInstance::start_loudspeaker_ctrl_devices_service_by_device_id(const string& device_id) {
	trc(__func__);

    return duerLinkSdk::get_instance()->start_loudspeaker_ctrl_devices_service_by_device_id(device_id);
}

bool DuerLinkCchipInstance::send_msg_to_devices_by_spec_type(const string& msg_buf,
                                                           device_type device_type_value) {
    return duerLinkSdk::get_instance()->send_msg_to_devices_by_spec_type(msg_buf,
                                                                         device_type_value);
}

bool DuerLinkCchipInstance::disconnect_devices_connections_by_spe_type(
        device_type device_type_value, const string& message) {
    APP_DEBUG("device_type_value = [%d], message = [%s]", device_type_value, message.c_str());
    return duerLinkSdk::get_instance()->disconnect_devices_connections_by_spe_type(device_type_value,
                                                                                   message);
}

bool DuerLinkCchipInstance::get_curret_connect_device_info(string& client_id,
                                                         string& device_id,
                                                         device_type device_type_value) {
    return duerLinkSdk::get_instance()->get_curret_connect_device_info(client_id,
                                                                       device_id,
                                                                       device_type_value);
}

void DuerLinkCchipInstance::set_network_observer(NetWorkPingStatusObserver *observer) {
	trc(__func__);

    if (observer != nullptr){
        m_pObserver = observer;
    }
}
    
unsigned short DuerLinkCchipInstance::getChksum(unsigned short *addr,int len) {
	trc(__func__);

    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft-= 2;
    }

    if (nleft == 1) {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }

    sum = ((sum>>16) + (sum&0xffff));
    sum += (sum>>16);
    answer = ~sum;

    return answer;
}

int DuerLinkCchipInstance::packIcmp(int pack_no, struct icmp* icmp) {
	trc(__func__);

    int packsize;
    struct icmp *picmp;
    struct timeval *tval;

    picmp = icmp;
    picmp->icmp_type = ICMP_ECHO;
    picmp->icmp_code = 0;
    picmp->icmp_cksum = 0;
    picmp->icmp_seq = pack_no;
    picmp->icmp_id = m_pid;
    packsize = (8 + m_datalen);
    tval= (struct timeval *)icmp->icmp_data;
    gettimeofday(tval, nullptr);
    picmp->icmp_cksum = getChksum((unsigned short *)icmp, packsize);

    return packsize;
}

bool DuerLinkCchipInstance::unpackIcmp(char *buf, int len, struct IcmpEchoReply *icmpEchoReply) {
	trc(__func__);

    int iphdrlen;
    struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend, tvrecv, tvresult;
    double rtt;

    ip = (struct ip *)buf;
    iphdrlen = ip->ip_hl << 2;
    icmp = (struct icmp *)(buf + iphdrlen);
    len -= iphdrlen;

    if (len < 8) {
        APP_ERROR("ICMP packets's length is less than 8.");
        return false;
    }

    if( (icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == m_pid) ) {
        tvsend = (struct timeval *)icmp->icmp_data;
        gettimeofday(&tvrecv, nullptr);
        tvresult = timevalSub(tvrecv, *tvsend);
        rtt = tvresult.tv_sec*1000 + tvresult.tv_usec/1000;  //ms
        icmpEchoReply->rtt = rtt;
        icmpEchoReply->icmpSeq = icmp->icmp_seq;
        icmpEchoReply->ipTtl = ip->ip_ttl;
        icmpEchoReply->icmpLen = len;

        return true;
    } else {
        return false;
    }
}

struct timeval DuerLinkCchipInstance::timevalSub(struct timeval timeval1, struct timeval timeval2) {
	trc(__func__);

    struct timeval result;

    result = timeval1;

    if ((result.tv_usec < timeval2.tv_usec) && (timeval2.tv_usec < 0)) {
        -- result.tv_sec;
        result.tv_usec += 1000000;
    }

    result.tv_sec -= timeval2.tv_sec;

    return result;
}

bool DuerLinkCchipInstance::sendPacket() {
	trc(__func__);

    size_t packetsize;
    while( m_nsend < m_maxPacketSize) {
        m_nsend ++;
        m_icmp_seq ++;
        packetsize = packIcmp(m_icmp_seq, (struct icmp*)m_sendpacket);

        if (sendto(m_sockfd,m_sendpacket, packetsize, 0, (struct sockaddr *) &m_dest_addr, sizeof(m_dest_addr)) < 0) {
            APP_ERROR("Ping sendto failed:%s.", strerror(errno));
            continue;
        }
    }

    return true;
}

bool DuerLinkCchipInstance::recvPacket(PingResult &pingResult) {
	trc(__func__);

    int len = 0;
    struct IcmpEchoReply icmpEchoReply;
    int maxfds = m_sockfd + 1;
    int nfd  = 0;
    fd_set rset;
    struct timeval timeout;
    socklen_t fromlen = sizeof(m_from_addr);

    timeout.tv_sec = MAX_WAIT_TIME;
    timeout.tv_usec = 0;

    FD_ZERO(&rset);

    for (int recvCount = 0; recvCount < m_maxPacketSize; recvCount ++) {
        FD_SET(m_sockfd, &rset);
        if ((nfd = select(maxfds, &rset, nullptr, nullptr, &timeout)) == -1) {
            APP_ERROR("Ping recv select failed:%s.", strerror(errno));
            continue;
        }

        if (nfd == 0) {
            icmpEchoReply.isReply = false;
            pingResult.icmpEchoReplys.push_back(icmpEchoReply);
            continue;
        }

        if (FD_ISSET(m_sockfd, &rset)) {
            if ((len = recvfrom(m_sockfd,
                                m_recvpacket,
                                sizeof(m_recvpacket),
                                0,
                                (struct sockaddr *)&m_from_addr,
                                &fromlen)) <0) {
                if(errno == EINTR) {
                    continue;
                }
                APP_ERROR("Ping recvfrom failed: %s.", strerror(errno));
                continue;
            }

            icmpEchoReply.fromAddr = inet_ntoa(m_from_addr.sin_addr) ;
            if (strncmp(icmpEchoReply.fromAddr.c_str(), pingResult.ip, strlen(pingResult.ip)) != 0) {
                recvCount--;
                continue;
            }
        }

        if (!unpackIcmp(m_recvpacket, len, &icmpEchoReply)) {
            recvCount--;
            continue;
        }

        icmpEchoReply.isReply = true;
        pingResult.icmpEchoReplys.push_back(icmpEchoReply);
        m_nreceived ++;
    }

    return true;

}

bool DuerLinkCchipInstance::getsockaddr(const char * hostOrIp, struct sockaddr_in* sockaddr) {
	trc(__func__);

    struct hostent *host;
    struct sockaddr_in dest_addr;
    unsigned long inaddr = 0l;

    bzero(&dest_addr,sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;

    inaddr = inet_addr(hostOrIp);
    if (inaddr == INADDR_NONE) {
        host = gethostbyname(hostOrIp);
        if (host == nullptr) {
            return false;
        }
        memcpy( (char *)&dest_addr.sin_addr,host->h_addr, host->h_length);
    } else if (!inet_aton(hostOrIp, &dest_addr.sin_addr)) {
        return false;
    }

    *sockaddr = dest_addr;

    return true;
}

bool DuerLinkCchipInstance::ping(string host, int count, PingResult& pingResult) {
	trc(__func__);

    int size = 50 * 1024;
    IcmpEchoReply icmpEchoReply;

    m_nsend = 0;
    m_nreceived = 0;
    pingResult.icmpEchoReplys.clear();
    m_maxPacketSize = count;
    m_pid = getpid();

    pingResult.dataLen = m_datalen;

    if ((m_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        APP_ERROR("Ping socket failed:%s.", strerror(errno));
        pingResult.error = strerror(errno);
        return false;
    }

    if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) != 0) {
        APP_ERROR("Setsockopt SO_RCVBUF failed:%s.", strerror(errno));
        close(m_sockfd);
        return false;
    }

    if (!getsockaddr(host.c_str(), &m_dest_addr)) {
        pingResult.error = "unknow host " + host;
        close(m_sockfd);
        return false;
    }

    strcpy(pingResult.ip, inet_ntoa(m_dest_addr.sin_addr));

    sendPacket();
    recvPacket(pingResult);

    pingResult.nsend = m_nsend;
    pingResult.nreceived = m_nreceived;

    close(m_sockfd);

    return true;
}

bool DuerLinkCchipInstance::ping_network(bool wakeupTrigger) {
	trc(__func__);

    string hostOrIp = PING_DEST_HOST1;
    int nsend = 0, nreceived = 0;
    bool ret;
    PingResult pingResult;
    InternetConnectivity networkResult = UNAVAILABLE;

    pthread_mutex_lock(&m_ping_lock);

    for (int count = 1; count <= MAX_PACKETS_COUNT; count ++) {
        memset(&pingResult.ip, 0x0, 32);
        ret = ping(hostOrIp, 1, pingResult);

        if (!ret) {
            APP_ERROR("Ping error:%s", pingResult.error.c_str());
        } else {
            nsend += pingResult.nsend;
            nreceived += pingResult.nreceived;
            if (nreceived > 0)
                break;
        }

        if (count == 2) {
            hostOrIp = PING_DEST_HOST2;
        }
    }

    if (nreceived > 0) {
        ret = true;
        networkResult = AVAILABLE;
        if (m_network_status == (int)UNAVAILABLE) {
            m_ping_interval = 1;
        } else {
            if (m_ping_interval < MAX_PING_INTERVAL) {
                m_ping_interval = m_ping_interval * 2;
                if (m_ping_interval > MAX_PING_INTERVAL) {
                    m_ping_interval = MAX_PING_INTERVAL;
                }
            }
        }
        m_network_status = 1;
    } else {
        ret = false;
        networkResult = UNAVAILABLE;
        m_network_status = 0;
        m_ping_interval = 1;
    }

    if (m_pObserver) {
        m_pObserver->network_status_changed(networkResult, wakeupTrigger);
    }

    pthread_mutex_unlock(&m_ping_lock);

    return ret;
}

void *DuerLinkCchipInstance::monitor_work_routine(void *arg) {
	showProcessThreadId("");

    auto thread = static_cast<DuerLinkCchipInstance*>(arg);
    int time_interval = 1;
    int time_count = 1;
    while(1) {
        thread->ping_network(false);
        time_count = time_interval = m_ping_interval;
        APP_INFO("monitor_work_routine m_ping_interval:%d", m_ping_interval);

        while (time_count > 0) {
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            ::select(0, NULL, NULL, NULL, &tv);
            time_count--;
            if (time_interval != m_ping_interval) {
                APP_INFO("monitor_work_routine m_ping_interval:%d, time_interval:%d", m_ping_interval, time_interval);
                break;
            }
        }
    }

    return nullptr;
}

void DuerLinkCchipInstance::start_network_monitor() {
	trc(__func__);

    pthread_t network_config_threadId;
#if 1
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,PTHREAD_STACK_MIN);
    pthread_create(&network_config_threadId, &attr, monitor_work_routine, this);
    pthread_detach(network_config_threadId);
	pthread_attr_destroy(&attr);
#else
    pthread_create(&network_config_threadId,nullptr, monitor_work_routine, this);
    pthread_detach(network_config_threadId);
#endif
}

}  // namespace application
}  // namespace duerOSDcsApp
