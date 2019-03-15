#ifndef UPDATE_H
#define UPDATE_H 1

#define CONEXANTSFS_PATHNAME "/tmp/oem_image.sfs"
#define CONEXANTBIN_PATHNAME "/tmp/iflash.bin"
#define DOWNLOAD_PROGRESS_DIR "/tmp/wifi_bt_download_progress"
#define BURN_PROGRESS_DIR "/tmp/progress"

int conexant_update(char *sfs,char *bin);
char *get_conexant_version(void);
int xzxhtml_btup(char *pPath);
int xzxhtml_wifiup(char *pPath);
int app_update_noparmeter(char *pPath);
int get_wifi_bt_update_flage(void);
int set_wifi_bt_update_flage(int status);
int get_wifi_bt_burn_progress(void);
int get_wifi_bt_download_progress(void);
int is_charge_plug(void);

#endif