#include "msg_handler.h"
#include "ipc_message.h"
#include "log.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "spotify.h"
#include <spotify_embedded.h>
#include <errno.h>
#include <libcchip/platform.h>

struct cmd_table cmds[];
static void  ctrl_spotify_set_volume_handler(struct spotify_ipc_param *msg){
	unsigned volume = atoi(msg->data)%201;
	if(volume > 200){
		return;
	}
	if(isLogined) {
		if(volume<=100){
			SpPlaybackUpdateVolume(VOL_RA_TO_SP_SCALE(volume));
			set_system_volume(volume);
		}else if(volume <= 200) {
			SpPlaybackUpdateVolume(VOL_RA_TO_SP_SCALE(volume-100));
		}		
	}
}

// set command table name and handler
// must let index as same as NOTIFY enum
void ipc_init() {

    // *** Do Not Modify These Two Function! ***
    register_ipc_handles(cmds);
    start_ipc_thread();

    // start coding from here ...

    return;
}


void set_display_name_handler(struct spotify_ipc_param *msg)
{
    button_pressed = kButtonDisplayName;
}

void ctrl_spotify_stop_handler(struct spotify_ipc_param *msg)
{
    LOG("[%s:%d]", __func__, __LINE__);

    if(isLogined) {
        button_pressed = kButtonChangeSource;
    }
}

void ctrl_spotify_toggle_pause_handler(struct spotify_ipc_param *msg)
{
    LOG("[%s:%d]", __func__, __LINE__);

    if(isLogined) {
        button_pressed = kButtonToggle;
    }
}
struct cmd_table cmds[] = {
    [NOTIFY_SET_DISPLAY_NAME]={"set-display-name",set_display_name_handler},
    [NOTIFY_SPOTIFY_STOP	]={"spotify-stop",    ctrl_spotify_stop_handler},
    [NOTIFY_SPOTIFY_TOGGLE	]={"spotify-toggle",  ctrl_spotify_toggle_pause_handler},
    [NOTIFY_SPOTIFY_VOLUME	]={"spotify-volume",  ctrl_spotify_set_volume_handler},
};

