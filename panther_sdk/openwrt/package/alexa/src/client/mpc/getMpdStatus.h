#ifndef __GET_MPD_STATUS_H__
#define __GET_MPD_STATUS_H__

//extern void GetMpdStatus(void);

extern int GetMpcVolume(void);
extern int QueryMpdStatus(void);
extern void CreateGetMpdStatusPthread(void);
extern struct mpd_connection *setup_connection(void);
extern int printErrorAndExit(struct mpd_connection *conn);
extern void getMpdStatusthread_wait(void);
extern int SetMpcVolume(int iVolume);

#endif



