#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WIFIAudio_GetCommand.h"

int WIFIAudio_GetCommand_freeConectAp(WAGC_pConectAp_t *ppconectap)
{
	int ret = 0;
	
	if ((ppconectap == NULL) || (*ppconectap == NULL))
	{
		ret = -1;
	} else {
		if((*ppconectap)->pSSID != NULL)
		{
			free((*ppconectap)->pSSID);
			(*ppconectap)->pSSID = NULL;
		}
		if((*ppconectap)->pPassword != NULL)
		{
			free((*ppconectap)->pPassword);
			(*ppconectap)->pPassword = NULL;
		}
		
		free(*ppconectap);
		*ppconectap = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeConnectAp(WAGC_pConnectAp_t *ppconnectap)
{
	int ret = 0;
	
	if ((ppconnectap == NULL) || (*ppconnectap == NULL))
	{
		ret = -1;
	} else {
		if((*ppconnectap)->pSSID != NULL)
		{
			free((*ppconnectap)->pSSID);
			(*ppconnectap)->pSSID = NULL;
		}
		if((*ppconnectap)->pPassword != NULL)
		{
			free((*ppconnectap)->pPassword);
			(*ppconnectap)->pPassword = NULL;
		}
		
		free(*ppconnectap);
		*ppconnectap = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeSubConnectAp(WAGC_pConnectAp_t *ppsubconnectap)
{
	int ret = 0;
	
	if ((ppsubconnectap == NULL) || (*ppsubconnectap == NULL))
	{
		ret = -1;
	} else {
		if((*ppsubconnectap)->pSSID != NULL)
		{
			free((*ppsubconnectap)->pSSID);
			(*ppsubconnectap)->pSSID = NULL;
		}
		if((*ppsubconnectap)->pPassword != NULL)
		{
			free((*ppsubconnectap)->pPassword);
			(*ppsubconnectap)->pPassword = NULL;
		}
		
		free(*ppsubconnectap);
		*ppsubconnectap = NULL;
	}
	
	return ret;
}


int WIFIAudio_GetCommand_freeConnectApEx(WAGC_pConnectApEx_t *ppconnectapex)
{
	int ret = 0;
	
	if ((ppconnectapex == NULL) || (*ppconnectapex == NULL))
	{
		ret = -1;
	} else {
		if((*ppconnectapex)->pSSID != NULL)
		{
			free((*ppconnectapex)->pSSID);
			(*ppconnectapex)->pSSID = NULL;
		}
		if((*ppconnectapex)->pPassword != NULL)
		{
			free((*ppconnectapex)->pPassword);
			(*ppconnectapex)->pPassword = NULL;
		}
		
		free(*ppconnectapex);
		*ppconnectapex = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeOneTouchState(WAGC_pOneTouchState_t *pponetouchstate)
{
	int ret = 0;
	
	if ((pponetouchstate == NULL) || (*pponetouchstate == NULL))
	{
		ret = -1;
	} else {
		free(*pponetouchstate);
		*pponetouchstate = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeOneTouchNameState(WAGC_pOneTouchNameState_t *pponetouchnamestate)
{
	int ret = 0;
	
	if ((pponetouchnamestate == NULL) || (*pponetouchnamestate == NULL))
	{
		ret = -1;
	} else {
		free(*pponetouchnamestate);
		*pponetouchnamestate = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeSetPlayerCmd(WAGC_pSetPlayerCmd_t *ppsetplayercmd)
{
	int ret = 0;
	
	if ((ppsetplayercmd == NULL) || (*ppsetplayercmd == NULL))
	{
		ret = -1;
	} else {
		if((*ppsetplayercmd)->pURI != NULL)
		{
			free((*ppsetplayercmd)->pURI);
			(*ppsetplayercmd)->pURI = NULL;
		}
		
		free(*ppsetplayercmd);
		*ppsetplayercmd = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeMultiroom(WAGC_pMultiroom_t *ppmultiroom)
{
	int ret = 0;
	if ((ppmultiroom == NULL) || (*ppmultiroom == NULL))
	{
		ret = -1;
	} else {
		free(*ppmultiroom);
		*ppmultiroom = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeGetPlayerCmd(WAGC_pGetPlayerCmd_t *ppgetplayercmd)
{
	int ret = 0;
	
	if ((ppgetplayercmd == NULL) || (*ppgetplayercmd == NULL))
	{
		ret = -1;
	} else {
		free(*ppgetplayercmd);
		*ppgetplayercmd = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeSSID(WAGC_pSSID_t *ppssid)
{
	int ret = 0;
	
	if ((ppssid == NULL) || (ppssid == NULL))
	{
		ret = -1;
	} else {
		if((*ppssid)->pSSID != NULL)
		{
			free((*ppssid)->pSSID);
			(*ppssid)->pSSID = NULL;
		}
		
		free(*ppssid);
		*ppssid = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeNetwork(WAGC_pNetwork_t *ppnetwork)
{
	int ret = 0;
	
	if ((ppnetwork == NULL) || (*ppnetwork == NULL))
	{
		ret = -1;
	} else {
		if((*ppnetwork)->pPassword != NULL)
		{
			free((*ppnetwork)->pPassword);
			(*ppnetwork)->pPassword = NULL;
		}
		
		free(*ppnetwork);
		*ppnetwork = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeName(WAGC_pName_t *ppname)
{
	int ret = 0;
	
	if ((ppname == NULL) || (*ppname == NULL))
	{
		ret = -1;
	} else {
		if((*ppname)->pName != NULL)
		{
			free((*ppname)->pName);
			(*ppname)->pName = NULL;
		}
		
		free(*ppname);
		*ppname = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeShutdown(WAGC_pShutdown_t *ppshutdown)
{
	int ret = 0;
	
	if ((ppshutdown == NULL) || (*ppshutdown == NULL))
	{
		ret = -1;
	} else {
		free(*ppshutdown);
		*ppshutdown = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeUpdateURL(WAGC_pUpdateURL_t *ppupdateurl)
{
	int ret = 0;
	
	if ((ppupdateurl == NULL) || (*ppupdateurl == NULL))
	{
		ret = -1;
	} else {
		if((*ppupdateurl)->pURL != NULL)
		{
			free((*ppupdateurl)->pURL);
			(*ppupdateurl)->pURL = NULL;
		}
		
		free(*ppupdateurl);
		*ppupdateurl = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeDateTime(WAGC_pDateTime_t *ppdatetime)
{
	int ret = 0;
	
	if ((ppdatetime == NULL) || (*ppdatetime == NULL))
	{
		ret = -1;
	} else {
		free(*ppdatetime);
		*ppdatetime = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeClockNum(WAGC_pClockNum_t *ppclocknum)
{
	int ret = 0;
	
	if ((ppclocknum == NULL) || (*ppclocknum == NULL))
	{
		ret = -1;
	} else {
		free(*ppclocknum);
		*ppclocknum = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeSelectLanguage(WAGC_pSelectLanguage_t *ppselectlanguage)
{
	int ret = 0;
	
	if ((ppselectlanguage == NULL) || (*ppselectlanguage == NULL))
	{
		ret = -1;
	} else {
		free(*ppselectlanguage);
		*ppselectlanguage = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freePlaylistIndex(WAGC_pPlaylistIndex_t *ppplaylistindex)
{
	int ret = 0;
	
	if ((ppplaylistindex == NULL) || (*ppplaylistindex == NULL))
	{
		ret = -1;
	} else {
		free(*ppplaylistindex);
		*ppplaylistindex = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freexzxAction(WAGC_pxzxAction_t *ppxzxAction)
{
	int ret = 0;
	
	if ((ppxzxAction == NULL) || (*ppxzxAction == NULL))
	{
		ret = -1;
	} else {
		if((*ppxzxAction)->pUrl != NULL)
		{
			free((*ppxzxAction)->pUrl);
			(*ppxzxAction)->pUrl = NULL;
		}
		
		if((*ppxzxAction)->pCardName != NULL)
		{
			free((*ppxzxAction)->pCardName);
			(*ppxzxAction)->pCardName = NULL;
		}
		
		free(*ppxzxAction);
		*ppxzxAction = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freePlayTfCard(WAGC_pPlayTfCard_t *ppplaytfcard)
{
	int ret = 0;
	
	if ((ppplaytfcard == NULL) || (*ppplaytfcard == NULL))
	{
		ret = -1;
	} else {
		free(*ppplaytfcard);
		*ppplaytfcard = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freePlayUsbDisk(WAGC_pPlayUsbDisk_t *ppplayusbdisk)
{
	int ret = 0;
	
	if ((ppplayusbdisk == NULL) || (*ppplayusbdisk == NULL))
	{
		ret = -1;
	} else {
		free(*ppplayusbdisk);
		*ppplayusbdisk = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeRemoveSynchronousEx(WAGC_pRemoveSynchronousEx_t *ppremovesynchronousex)
{
	int ret = 0;
	
	if ((ppremovesynchronousex == NULL) || (*ppremovesynchronousex == NULL))
	{
		ret = -1;
	} else {
		if((*ppremovesynchronousex)->pMac != NULL)
		{
			free((*ppremovesynchronousex)->pMac);
			(*ppremovesynchronousex)->pMac = NULL;
		}
		
		free(*ppremovesynchronousex);
		*ppremovesynchronousex = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeSetSynchronousEx(WAGC_pSetSynchronousEx_t *ppsetsynchronousex)
{
	int ret = 0;
	
	if ((ppsetsynchronousex == NULL) || (ppsetsynchronousex == NULL))
	{
		ret = -1;
	} else {
		if((*ppsetsynchronousex)->pMac != NULL)
		{
			free((*ppsetsynchronousex)->pMac);
			(*ppsetsynchronousex)->pMac = NULL;
		}
		
		free(*ppsetsynchronousex);
		*ppsetsynchronousex = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeBluetooth(WAGC_pBluetooth_t *ppbluetooth)
{
	int ret = 0;
	
	if ((ppbluetooth == NULL) || (ppbluetooth == NULL))
	{
		ret = -1;
	} else {
		if((*ppbluetooth)->pAddress != NULL)
		{
			free((*ppbluetooth)->pAddress);
			(*ppbluetooth)->pAddress = NULL;
		}
		
		free(*ppbluetooth);
		*ppbluetooth = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeSetSpeechRecognition(WAGC_pSetSpeechRecognition_t *ppsetspeechrecognition)
{
	int ret = 0;
	
	if ((ppsetspeechrecognition == NULL) || (ppsetspeechrecognition == NULL))
	{
		ret = -1;
	} else {
		free(*ppsetspeechrecognition);
		*ppsetspeechrecognition = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeSetSingleLight(WAGC_pSetSingleLight_t *ppsetsinglelight)
{
	int ret = 0;
	
	if ((ppsetsinglelight == NULL) || (ppsetsinglelight == NULL))
	{
		ret = -1;
	} else {
		free(*ppsetsinglelight);
		*ppsetsinglelight = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeSetAlexaLanguage(WAGC_pSetAlexaLanguage_t *ppsetalexalanguage)
{
	int ret = 0;
	
	if ((ppsetalexalanguage == NULL) || (ppsetalexalanguage == NULL))
	{
		ret = -1;
	} else {
		if((*ppsetalexalanguage)->pLanguage != NULL)
		{
			free((*ppsetalexalanguage)->pLanguage);
			(*ppsetalexalanguage)->pLanguage = NULL;
		}
		
		free(*ppsetalexalanguage);
		*ppsetalexalanguage = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeSetTestMode(WAGC_pSetTestMode_t *ppsettestmode)
{
	int ret = 0;
	
	if ((ppsettestmode == NULL) || (ppsettestmode == NULL))
	{
		ret = -1;
	} else {
		free(*ppsettestmode);
		*ppsettestmode = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeSetEnable(WAGC_pSetEnable_t *ppSetEnable)
{
	int ret = 0;
	
	if ((ppSetEnable == NULL) || (ppSetEnable == NULL))
	{
		ret = -1;
	} else {
		free(*ppSetEnable);
		*ppSetEnable = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeAlarmAndPlan(WAGC_pAlarmAndPlan_t *ppalarmandplan)
{
	int ret = 0;
	
	if ((ppalarmandplan == NULL) || (ppalarmandplan == NULL))
	{
		ret = -1;
	} else {
		free(*ppalarmandplan);
		*ppalarmandplan = NULL;
	}
	
	return ret;
}

int WIFIAudio_GetCommand_freeShortcutKey(WAGC_pShortcutKey_t *ppShortcutKey)
{
	int ret = 0;
	
	if ((ppShortcutKey == NULL) || (ppShortcutKey == NULL))
	{
		ret = -1;
	} else {
		free(*ppShortcutKey);
		*ppShortcutKey = NULL;
	}
	
	return ret;
}


int WIFIAudio_GetCommand_freeComParm(WAGC_pComParm *pComParm)
{
	int ret = 0;
	
	if (pComParm == NULL)
	{
		ret = -1;
	} else {
		switch ((*pComParm)->Command)
		{
			case WIFIAUDIO_GETCOMMAND_WLANCONECTAP:
				if((*pComParm)->Parameter.pConectAp != NULL)
				{
					ret = WIFIAudio_GetCommand_freeConectAp(&((*pComParm)->Parameter.pConectAp));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_WLANCONNECTAP:
				if((*pComParm)->Parameter.pConnectAp != NULL)
				{
					ret = WIFIAudio_GetCommand_freeConnectAp(&((*pComParm)->Parameter.pConnectAp));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_WLANSUBCONNECTAP:
				if((*pComParm)->Parameter.pSubConnectAp != NULL)
				{
					ret = WIFIAudio_GetCommand_freeSubConnectAp(&((*pComParm)->Parameter.pSubConnectAp));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_WLANCONNECTAPEX:
				if((*pComParm)->Parameter.pConnectApEx != NULL)
				{
					ret = WIFIAudio_GetCommand_freeConnectApEx(&((*pComParm)->Parameter.pConnectApEx));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETONETOUCHCONFIGURE:
				if((*pComParm)->Parameter.pOneTouchState != NULL)
				{
					ret = WIFIAudio_GetCommand_freeOneTouchState(&((*pComParm)->Parameter.pOneTouchState));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETONETOUCHCONFIGURESETDEVICENAME:
				if((*pComParm)->Parameter.pOneTouchNameState != NULL)
				{
					ret = WIFIAudio_GetCommand_freeOneTouchNameState(&((*pComParm)->Parameter.pOneTouchNameState));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETPLAYERCMD:
				if((*pComParm)->Parameter.pSetPlayerCmd != NULL)
				{
					ret = WIFIAudio_GetCommand_freeSetPlayerCmd(&((*pComParm)->Parameter.pSetPlayerCmd));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_MULTIROOM:
				if((*pComParm)->Parameter.pMultiroom != NULL)
				{
					ret = WIFIAudio_GetCommand_freeMultiroom(&((*pComParm)->Parameter.pMultiroom));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_GETPLAYERCMD:
				if((*pComParm)->Parameter.pGetPlayerCmd != NULL)
				{
					ret = WIFIAudio_GetCommand_freeGetPlayerCmd(&((*pComParm)->Parameter.pGetPlayerCmd));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETSSID:
				if((*pComParm)->Parameter.pSSID != NULL)
				{
					ret = WIFIAudio_GetCommand_freeSSID(&((*pComParm)->Parameter.pSSID));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETNETWORK:
				if((*pComParm)->Parameter.pNetwork != NULL)
				{
					ret = WIFIAudio_GetCommand_freeNetwork(&((*pComParm)->Parameter.pNetwork));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETDEVICENAME:
				if((*pComParm)->Parameter.pName != NULL)
				{
					ret = WIFIAudio_GetCommand_freeName(&((*pComParm)->Parameter.pName));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETSHUTDOWN:
				if((*pComParm)->Parameter.pShutdown != NULL)
				{
					ret = WIFIAudio_GetCommand_freeShutdown(&((*pComParm)->Parameter.pShutdown));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_GETMVREMOTEUPDATE:
				if((*pComParm)->Parameter.pUpdateURL != NULL)
				{
					ret = WIFIAudio_GetCommand_freeUpdateURL(&((*pComParm)->Parameter.pUpdateURL));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETTIMESYNC:
				if((*pComParm)->Parameter.pDateTime != NULL)
				{
					ret = WIFIAudio_GetCommand_freeDateTime(&((*pComParm)->Parameter.pDateTime));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_GETALARMCLOCK:
				if((*pComParm)->Parameter.pClockNum != NULL)
				{
					ret = WIFIAudio_GetCommand_freeClockNum(&((*pComParm)->Parameter.pClockNum));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SELECTLANGUAGE:
				if((*pComParm)->Parameter.pSelectLanguage != NULL)
				{
					ret = WIFIAudio_GetCommand_freeSelectLanguage(&((*pComParm)->Parameter.pSelectLanguage));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_DELETEPLAYLISTMUSIC:
				if((*pComParm)->Parameter.pPlaylistIndex != NULL)
				{
					ret = WIFIAudio_GetCommand_freePlaylistIndex(&((*pComParm)->Parameter.pPlaylistIndex));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_XZXACTION:
				if((*pComParm)->Parameter.pxzxAction != NULL)
				{
					ret = WIFIAudio_GetCommand_freexzxAction(&((*pComParm)->Parameter.pxzxAction));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_PLAYTFCARD:
				if((*pComParm)->Parameter.pPlayTfCard != NULL)
				{
					ret = WIFIAudio_GetCommand_freePlayTfCard(&((*pComParm)->Parameter.pPlayTfCard));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_PLAYUSBDISK:
				if((*pComParm)->Parameter.pPlayUsbDisk != NULL)
				{
					ret = WIFIAudio_GetCommand_freePlayUsbDisk(&((*pComParm)->Parameter.pPlayUsbDisk));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_REMOVESYNCHRONOUSEX:
				if((*pComParm)->Parameter.pRemoveSynchronousEx != NULL)
				{
					ret = WIFIAudio_GetCommand_freeRemoveSynchronousEx(&((*pComParm)->Parameter.pRemoveSynchronousEx));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETSYNCHRONOUSEX:
				if((*pComParm)->Parameter.pSetSynchronousEx != NULL)
				{
					ret = WIFIAudio_GetCommand_freeSetSynchronousEx(&((*pComParm)->Parameter.pSetSynchronousEx));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_BLUETOOTH:
				if((*pComParm)->Parameter.pBluetooth != NULL)
				{
					ret = WIFIAudio_GetCommand_freeBluetooth(&((*pComParm)->Parameter.pBluetooth));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETSPEECHRECOGNITION:
				if((*pComParm)->Parameter.pSetSpeechRecognition != NULL)
				{
					ret = WIFIAudio_GetCommand_freeSetSpeechRecognition(&((*pComParm)->Parameter.pSetSpeechRecognition));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETSINGLELIGHT:
				if((*pComParm)->Parameter.pSetSingleLight != NULL)
				{
					ret = WIFIAudio_GetCommand_freeSetSingleLight(&((*pComParm)->Parameter.pSetSingleLight));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETALEXALANGUAGE:
				if((*pComParm)->Parameter.pSetAlexaLanguage != NULL)
				{
					ret = WIFIAudio_GetCommand_freeSetAlexaLanguage(&((*pComParm)->Parameter.pSetAlexaLanguage));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_SETTESTMODE:
				if((*pComParm)->Parameter.pSetTestMode != NULL)
				{
					ret = WIFIAudio_GetCommand_freeSetTestMode(&((*pComParm)->Parameter.pSetTestMode));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_POWERONAUTOPLAY:
			case WIFIAUDIO_GETCOMMAND_ENABLEAUTOTIMESYNC:
				if((*pComParm)->Parameter.pSetEnable != NULL)
				{
					ret = WIFIAudio_GetCommand_freeSetEnable(&((*pComParm)->Parameter.pSetEnable));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_REMOVEALARMCLOCK:
			case WIFIAUDIO_GETCOMMAND_ENABLEALARMCLOCK:
			case WIFIAUDIO_GETCOMMAND_REMOVEPLAYPLAN:
			case WIFIAUDIO_GETCOMMAND_GETPLANMUSICLIST:
			case WIFIAUDIO_GETCOMMAND_ENABLEPLAYMUSICASPLAN:
			case WIFIAUDIO_GETCOMMAND_GETPLANMIXEDLIST:
				if((*pComParm)->Parameter.pAlarmAndPlan != NULL)
				{
					ret = WIFIAudio_GetCommand_freeAlarmAndPlan(&((*pComParm)->Parameter.pAlarmAndPlan));
				}
				break;
			case WIFIAUDIO_GETCOMMAND_GETSHORTCUTKEYLIST:
				if((*pComParm)->Parameter.pShortcutKey != NULL)
				{
					ret = WIFIAudio_GetCommand_freeShortcutKey(&((*pComParm)->Parameter.pShortcutKey));
				}
				break;
			default:
				break;
		}
		free(*pComParm);
		*pComParm = NULL;
	}
	
	return ret;
}

WAGC_pConectAp_t WIFIAudio_GetCommand_getConectApStr(const char *pBuf)
{
	WAGC_pConectAp_t pconectap = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pconectap = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pconectap = NULL;
		} else {
			strcpy(pBufCpy, pBuf);
			strtok(pBufCpy, ":");
			pconectap = (WAGC_pConectAp_t)calloc(1, sizeof(WAGC_ConectAp_t));
			if(pconectap == NULL)
			{
			} else {
				pTmp = strtok(NULL, ":");
				pconectap->pSSID = (char *)calloc(1,strlen(pTmp)+1);
				if(pconectap->pSSID == NULL)
				{
					free(pconectap);
					pconectap = NULL;
				} else {
					strcpy(pconectap->pSSID,pTmp);
					
					pTmp = strtok(NULL, ":");
					pconectap->Channel = atoi(pTmp);
					
					pTmp = strtok(NULL, ":");
					pconectap->Auth = atoi(pTmp);
					
					pTmp = strtok(NULL, ":");
					pconectap->Encty = atoi(pTmp);
					
					pTmp = strtok(NULL, ":");
					pconectap->pPassword = (char *)calloc(1,strlen(pTmp)+1);
					if(pconectap->pPassword == NULL)
					{
						free(pconectap->pSSID);
						pconectap->pSSID = NULL;
						free(pconectap);
						pconectap = NULL;
					}
					else
					{
						strcpy(pconectap->pPassword,pTmp);
						
						pTmp = strtok(NULL, ":");
						pconectap->Extch = atoi(pTmp);
					}
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pconectap;
}

int WIFIAudio_GetCommand_UftStringToUftvalue(char *out_value,char *in_string)
{
	int ret = 0;
	int len = 0;
	int i = 0;
	int j = 0;
	char temp[10];
	
	if((NULL == out_value) || (NULL == in_string))
	{
	} else {
		len = strlen(in_string);
		
		for(i = 0,j = 0; i<len; i++,j++)
		{
			if('%' == in_string[i])
			{
				temp[0] = in_string[i + 1];
				temp[1] = in_string[i + 2];
				temp[2] = '\0';
				i = i + 2;
				out_value[j] = (unsigned char)strtol(temp, NULL, 16);
			} else {
				out_value[j] = in_string[i];
				if('+' == out_value[j])
				{
					out_value[j] = ' ';
				}
			}
		}
	}
	
	return ret;
}

WAGC_pConnectAp_t WIFIAudio_GetCommand_getConnectApStr(const char *pBuf)
{
	WAGC_pConnectAp_t pconnectap = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	char ssid[128];
	char temp[256];
	
	if (pBuf == NULL)
	{
		pconnectap = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pconnectap = NULL;
		} else {
			strcpy(pBufCpy, pBuf);
			strtok(pBufCpy, ":");
			
			pconnectap = (WAGC_pConnectAp_t)calloc(1, sizeof(WAGC_ConnectAp_t));
			if(pconnectap == NULL)
			{
			} else {
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					strcpy(ssid, pTmp);	
				}
				
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pconnectap->Channel = atoi(pTmp);
				}
				
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pconnectap->Auth = atoi(pTmp);
				}
				
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pconnectap->Encty = atoi(pTmp);
				}
				
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pconnectap->pPassword = strdup(pTmp);
				}
				
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pconnectap->Extch = atoi(pTmp);
				}
				
				memset(temp, 0, sizeof(temp)/sizeof(char));
				WIFIAudio_GetCommand_UftStringToUftvalue(temp, ssid);
				pconnectap->pSSID = strdup(temp);
			}
			
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pconnectap;
}

WAGC_pSubConnectAp_t WIFIAudio_GetCommand_getSubConnectApStr(const char *pBuf)
{
	WAGC_pSubConnectAp_t psubconnectap = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		psubconnectap = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			psubconnectap = NULL;
		} else {
			strcpy(pBufCpy, pBuf);
			strtok(pBufCpy, ":");
			
			psubconnectap = (WAGC_pSubConnectAp_t)calloc(1, sizeof(WAGC_SubConnectAp_t));
			if(psubconnectap == NULL)
			{
			} else {
				pTmp = strtok(NULL, ":");
				psubconnectap->pSSID = (char *)calloc(1,strlen(pTmp)+1);
				if(psubconnectap->pSSID == NULL)
				{
					free(psubconnectap);
					psubconnectap = NULL;
				} else {
					strcpy(psubconnectap->pSSID,pTmp);
					
					pTmp = strtok(NULL, ":");
					psubconnectap->Channel = atoi(pTmp);
					
					pTmp = strtok(NULL, ":");
					psubconnectap->Auth = atoi(pTmp);
					
					pTmp = strtok(NULL, ":");
					psubconnectap->Encty = atoi(pTmp);
					
					pTmp = strtok(NULL, ":");
					psubconnectap->pPassword = (char *)calloc(1,strlen(pTmp)+1);
					if(psubconnectap->pPassword == NULL)
					{
						free(psubconnectap->pSSID);
						psubconnectap->pSSID = NULL;
						free(psubconnectap);
						psubconnectap = NULL;
					} else {
						strcpy(psubconnectap->pPassword,pTmp);
						
						pTmp = strtok(NULL, ":");
						psubconnectap->Extch = atoi(pTmp);
					}
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return psubconnectap;
}

WAGC_pConnectApEx_t WIFIAudio_GetCommand_getConnectApExStr(const char *pBuf)
{
	WAGC_pConnectApEx_t pconnectapex = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pconnectapex = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pconnectapex = NULL;
		} else {
			strcpy(pBufCpy, pBuf);
			strtok(pBufCpy, ":=;");	
			
			pconnectapex = (WAGC_pConnectApEx_t)calloc(1, sizeof(WAGC_ConnectApEx_t));
			if(pconnectapex == NULL)
			{
			} else {
				strtok(NULL, ":=;");
				pTmp = strtok(NULL, ":=;");
				pconnectapex->pSSID = (char *)calloc(1,strlen(pTmp)+1);
				if(pconnectapex->pSSID == NULL)
				{
					free(pconnectapex);
					pconnectapex = NULL;
				} else {
					strcpy(pconnectapex->pSSID,pTmp);
					
					strtok(NULL, ":=;");
					pTmp = strtok(NULL, ":=;");
					pconnectapex->Channel = atoi(pTmp);
					
					pTmp = strtok(NULL, ":=;");
					pconnectapex->Auth = atoi(pTmp);
					
					pTmp = strtok(NULL, ":=;");
					pconnectapex->Encty = atoi(pTmp);
					
					strtok(NULL, ":=;");
					pTmp = strtok(NULL, ":=;");
					pconnectapex->pPassword = (char *)calloc(1,strlen(pTmp)+1);
					if(pconnectapex->pPassword == NULL)
					{
						free(pconnectapex->pSSID);
						pconnectapex->pSSID = NULL;
						free(pconnectapex);
						pconnectapex = NULL;
					} else {
						strcpy(pconnectapex->pPassword,pTmp);
						
						strtok(NULL, ":=;");
						pTmp = strtok(NULL, ":=;");
						pconnectapex->Extch = atoi(pTmp);
					}
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pconnectapex;
}

WAGC_pOneTouchState_t WIFIAudio_GetCommand_setOneTouchConfigureStr(const char *pBuf)
{
	WAGC_pOneTouchState_t ponetouchstate = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		ponetouchstate = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			ponetouchstate = NULL;
		} else {
			ponetouchstate = (WAGC_pOneTouchState_t)calloc(1, sizeof(WAGC_OneTouchState_t));
			if(ponetouchstate == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					ponetouchstate->Value = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return ponetouchstate;
}

WAGC_pOneTouchNameState_t WIFIAudio_GetCommand_setOneTouchConfigureSetDeviceNameStr(const char *pBuf)
{
	WAGC_pOneTouchNameState_t ponetouchnamestate = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		ponetouchnamestate = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			ponetouchnamestate = NULL;
		} else {
			ponetouchnamestate = (WAGC_pOneTouchNameState_t)calloc(1, sizeof(WAGC_OneTouchNameState_t));
			if(ponetouchnamestate == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					ponetouchnamestate->Value = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return ponetouchnamestate;
}

WAGC_pSetAlexaLanguage_t WIFIAudio_GetCommand_SetAlexaLanguageStr(const char *pBuf)
{
	WAGC_pSetAlexaLanguage_t psetalexalanguage = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		psetalexalanguage = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			psetalexalanguage = NULL;
		} else {
			psetalexalanguage = (WAGC_pSetAlexaLanguage_t)calloc(1, sizeof(WAGC_SetAlexaLanguage_t));
			if(psetalexalanguage == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					psetalexalanguage->pLanguage = strdup(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return psetalexalanguage;
}

WAGC_pSetPlayerCmd_t WIFIAudio_GetCommand_getSetPlayerCmdStr(const char *pBuf)
{
	WAGC_pSetPlayerCmd_t psetplayercmd = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int num = 0;
	int i;
	if (pBuf == NULL)
	{
		psetplayercmd = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			psetplayercmd = NULL;
		} else {
			strcpy(pBufCpy, pBuf);
			strtok(pBufCpy, ":");
			
			psetplayercmd = (WAGC_pSetPlayerCmd_t)calloc(1, sizeof(WAGC_SetPlayerCmd_t));
			if(psetplayercmd == NULL)
			{
			} else {
				pTmp = strtok(NULL, ":");
				if(!strcmp(pTmp,"playlist"))
				{
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_PLAYLIST;
					pTmp = strtok(NULL, ":");
					psetplayercmd->Value = atoi(pTmp);
				} else if(!strcmp(pTmp,"pause")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_PAUSE;
				} else if(!strcmp(pTmp,"play")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_PLAY;
				} else if(!strcmp(pTmp,"prev")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_PREV;
				} else if(!strcmp(pTmp,"next")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_NEXT;
				} else if(!strcmp(pTmp,"seek")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_SEEK;
					pTmp = strtok(NULL, ":");
					psetplayercmd->Value = atoi(pTmp);
				} else if(!strcmp(pTmp,"stop")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_STOP;
				} else if(!strcmp(pTmp,"vol")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_VOL;
					pTmp = strtok(NULL, ":");
					psetplayercmd->Value = atoi(pTmp);
				} else if(!strcmp(pTmp,"mute")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_MUTE;
					pTmp = strtok(NULL, ":");
					psetplayercmd->Value = atoi(pTmp);
				} else if(!strcmp(pTmp,"loopmode")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_LOOPMODE;
					pTmp = strtok(NULL, ":");
					psetplayercmd->Value = atoi(pTmp);
				} else if(!strcmp(pTmp,"equalizer")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_EQUALIZER;
					pTmp = strtok(NULL, ":");
					psetplayercmd->Value = atoi(pTmp);
				} else if(!strcmp(pTmp,"slave_vol")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_SLAVE_VOL;
					pTmp = strtok(NULL, ":");
					psetplayercmd->Value = atoi(pTmp);
				} else if(!strcmp(pTmp,"slave_channel")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_SLAVE_CHANNEL;
					pTmp = strtok(NULL, ":");
					psetplayercmd->Value = atoi(pTmp);
				} else if(!strcmp(pTmp,"switchmode")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_SWITCHMODE;
					pTmp = strtok(NULL, ":");
					psetplayercmd->Value = atoi(pTmp);
				} else if(!strcmp(pTmp,"slave_mute")) {
					psetplayercmd->Attr = WAGC_COMMAND_SETPLAYERATTR_TYPE_SLAVE_MUTE;
					pTmp = strtok(NULL, ":");
					psetplayercmd->Value = atoi(pTmp);
				} else {
					free(psetplayercmd);
					psetplayercmd = NULL;
				}
			}
			
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return psetplayercmd;
}

WAGC_pMultiroom_t WIFIAudio_GetCommand_getMultiroomStr(const char *pBuf)
{
	WAGC_pMultiroom_t pmultiroom = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pmultiroom = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pmultiroom = NULL;
		} else {
			strcpy(pBufCpy, pBuf);
			strtok(pBufCpy, ":");
			
			pmultiroom = (WAGC_pMultiroom_t)calloc(1, sizeof(WAGC_Multiroom_t));
			if(pmultiroom == NULL)
			{
			} else {
				pTmp = strtok(NULL, ":");
				if(!strcmp(pTmp,"SlaveMute"))
				{
					pmultiroom->Attr = WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEMUTE;
					pTmp = strtok(NULL, ":");
					inet_aton(pTmp,&(pmultiroom->IP));
					pTmp = strtok(NULL, ":");
					pmultiroom->Value = atoi(pTmp);
				} else if(!strcmp(pTmp,"getSlaveList")) {
					pmultiroom->Attr = WAGC_COMMAND_MULTIROOMATTR_TYPE_GETSLAVELIST;
				} else if(!strcmp(pTmp,"SlaveKickout")) {
					pmultiroom->Attr = WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEKICKOUT;
					pTmp = strtok(NULL, ":");
					inet_aton(pTmp,&(pmultiroom->IP));
				} else if(!strcmp(pTmp,"SlaveMask")) {
					pmultiroom->Attr = WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEMASK;
					pTmp = strtok(NULL, ":");
					inet_aton(pTmp,&(pmultiroom->IP));
				} else if(!strcmp(pTmp,"SlaveUnMask")) {
					pmultiroom->Attr = WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEUNMASK;
					pTmp = strtok(NULL, ":");
					inet_aton(pTmp,&(pmultiroom->IP));
				} else if(!strcmp(pTmp,"SlaveVolume")) {
					pmultiroom->Attr = WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVEVOLUME;
					pTmp = strtok(NULL, ":");
					inet_aton(pTmp,&(pmultiroom->IP));
					pTmp = strtok(NULL, ":");
					pmultiroom->Value = atoi(pTmp);
				} else if(!strcmp(pTmp,"SlaveChannel")) {
					pmultiroom->Attr = WAGC_COMMAND_MULTIROOMATTR_TYPE_SLAVECHANNEL;
					pTmp = strtok(NULL, ":");
					inet_aton(pTmp,&(pmultiroom->IP));
					pTmp = strtok(NULL, ":");
					pmultiroom->Value = atoi(pTmp);
				} else {
					free(pmultiroom);
					pmultiroom = NULL;
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pmultiroom;
}

WAGC_pGetPlayerCmd_t WIFIAudio_GetCommand_getGetPlayerCmdStr(const char *pBuf)
{
	WAGC_pGetPlayerCmd_t pgetplayercmd = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	if (pBuf == NULL)
	{
		pgetplayercmd = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pgetplayercmd = NULL;
		} else {
			strcpy(pBufCpy, pBuf);
			strtok(pBufCpy, ":");
			
			pgetplayercmd = (WAGC_pGetPlayerCmd_t)calloc(1, sizeof(WAGC_GetPlayerCmd_t));
			if(pgetplayercmd == NULL)
			{
			} else {
				pTmp = strtok(NULL, ":");
				if(!strcmp(pTmp,"vol"))
				{
					pgetplayercmd->Attr = WAGC_COMMAND_GETPLAYERATTR_TYPE_VOLUME;
				} else if(!strcmp(pTmp,"mute")) {
					pgetplayercmd->Attr = WAGC_COMMAND_GETPLAYERATTR_TYPE_MUTE;
				} else if(!strcmp(pTmp,"slave_channel")) {
					pgetplayercmd->Attr = WAGC_COMMAND_GETPLAYERATTR_TYPE_SLAVE_CHANNEL;
				} else if(!strcmp(pTmp,"switchmode")) {
					pgetplayercmd->Attr = WAGC_COMMAND_GETPLAYERATTR_TYPE_SWITCHMODE;
				} else {
					free(pgetplayercmd);
					pgetplayercmd = NULL;
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pgetplayercmd;
}

WAGC_pSSID_t WIFIAudio_GetCommand_getSSIDStr(const char *pBuf)
{
	WAGC_pSSID_t pssid = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int num = 0;
	char ssidstr[256];
	int i = 0;
	
	if (pBuf == NULL)
	{
		pssid = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pssid = NULL;
		} else {
			pssid = (WAGC_pSSID_t)calloc(1, sizeof(WAGC_SSID_t));
			if(pssid == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");	
				
				num = 0;
				
				while(1)
				{
					if(NULL == strtok(NULL, ":"))
					{
						num--;
						break;
					} else {
						num++;
					}
				}
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				memset(ssidstr, 0, 256);
				for(i = 0; i<num+1; i++)
				{
					if(0 != i)
					{
						strcat(ssidstr, ":");
					}
					strcat(ssidstr, strtok(NULL, ":"));
				}

				pssid->pSSID = (char *)calloc(1, strlen(ssidstr) + 1);
				if(NULL == pssid->pSSID)
				{
					free(pssid);
					pssid = NULL;
				} else {
					strcpy(pssid->pSSID, ssidstr);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pssid;
}

WAGC_pNetwork_t WIFIAudio_GetCommand_getNetworkStr(const char *pBuf)
{
	WAGC_pNetwork_t pnetwork = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int num = 0;
	char passwordstr[256];
	int i = 0;
	
	if (pBuf == NULL)
	{
		pnetwork = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pnetwork = NULL;
		} else {
			memset(pBufCpy, 0, strlen(pBuf) + 1);
			strcpy(pBufCpy, pBuf);
			strtok(pBufCpy, ":");
			
			pnetwork = (WAGC_pNetwork_t)calloc(1, sizeof(WAGC_Network_t));
			if(pnetwork == NULL)
			{
			} else {
				pTmp = strtok(NULL, ":");
				pnetwork->Auth = atoi(pTmp);
				
				num = 0;
				
				while(1)
				{
					if(NULL == strtok(NULL, ":"))
					{
						num--;
						break;
					} else {
						num++;
					}
				}
				
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				strtok(NULL, ":");
				
				memset(passwordstr, 0, 256);
				for(i = 0; i<num+1; i++)
				{
					if(0 != i)
					{
						strcat(passwordstr, ":");
					}
					strcat(passwordstr, strtok(NULL, ":"));
				}

				pnetwork->pPassword = (char *)calloc(1, strlen(passwordstr) + 1);
				if(NULL == pnetwork->pPassword)
				{
					free(pnetwork);
					pnetwork = NULL;
				} else {
					strcpy(pnetwork->pPassword, passwordstr);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pnetwork;
}

WAGC_pName_t WIFIAudio_GetCommand_setDeviceNameStr(const char *pBuf)
{
	WAGC_pName_t pdevicename = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int num = 0;
	char namestr[256];
	char temp[256];
	int i = 0;
	
	if (pBuf == NULL)
	{
		pdevicename = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pdevicename = NULL;
		} else {
			pdevicename = (WAGC_pName_t)calloc(1, sizeof(WAGC_Name_t));
			if(pdevicename == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				if(NULL == strtok(NULL, ":"))
				{
					pdevicename->pName = strdup(" ");
				} else {
					num = 1;
					
					while(1)
					{
						if(NULL == strtok(NULL, ":"))
						{
							num--;
							break;
						} else {
							num++;
						}
					}
					
					memset(pBufCpy, 0, strlen(pBuf) + 1);
					strcpy(pBufCpy, pBuf);
					strtok(pBufCpy, ":");
					memset(namestr, 0, sizeof(temp)/sizeof(namestr));
					for(i = 0; i<num+1; i++)
					{
						if(0 != i)
						{
							strcat(namestr, ":");
						}
						strcat(namestr, strtok(NULL, ":"));
					}
					
					memset(temp, 0, sizeof(temp)/sizeof(char));
					WIFIAudio_GetCommand_UftStringToUftvalue(temp, namestr);
					pdevicename->pName = strdup(temp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pdevicename;
}

WAGC_pShutdown_t WIFIAudio_GetCommand_setShutdownStr(const char *pBuf)
{
	WAGC_pShutdown_t pShutdown = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pShutdown = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pShutdown = NULL;
		} else {
			pShutdown = (WAGC_pShutdown_t)calloc(1, sizeof(WAGC_Shutdown_t));
			if(pShutdown == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					pShutdown->Second = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pShutdown;
}

WAGC_pUpdateURL_t WIFIAudio_GetCommand_getRemoteUpdateStr(const char *pBuf)
{
	WAGC_pUpdateURL_t pupdateurl = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int num = 0;
	char urlstr[512];
	int i = 0;
	
	if (pBuf == NULL)
	{
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
		} else {
			pupdateurl = (WAGC_pUpdateURL_t)calloc(1, sizeof(WAGC_UpdateURL_t));
			if(pupdateurl == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				num = 0;
				
				while(1)
				{
					if(NULL == strtok(NULL, ":"))
					{
						num--;
						break;
					} else {
						num++;
					}
				}
				
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				memset(urlstr, 0, 512);
				for(i = 0; i<num+1; i++)
				{
					if(0 != i)
					{
						strcat(urlstr, ":");
					}
					strcat(urlstr, strtok(NULL, ":"));
				}
				
				if(strlen(urlstr) == 0)
				{
					pupdateurl->pURL = NULL;
				} else {
					pupdateurl->pURL = strdup(urlstr);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pupdateurl;
}

WAGC_pDateTime_t WIFIAudio_GetCommand_setTimeSyncStr(const char *pBuf)
{
	WAGC_pDateTime_t pdatetime = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int num = 0;
	int i = 0;
	
	if (pBuf == NULL)
	{
		pdatetime = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pdatetime = NULL;
		} else {
			pdatetime = (WAGC_pDateTime_t)calloc(1, sizeof(WAGC_DateTime_t));
			if(pdatetime == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");	
				pTmp = strtok(NULL, ":");
				
				if(6 != sscanf(pTmp, "%04d%02d%02d%02d%02d%02d", &(pdatetime->Year), &(pdatetime->Month), \
				&(pdatetime->Day), &(pdatetime->Hour), &(pdatetime->Minute), &(pdatetime->Second)))
				{
					free(pdatetime);
					pdatetime = NULL;
				}
				
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pdatetime->Zone = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pdatetime;
}

WAGC_pClockNum_t WIFIAudio_GetCommand_getAlarmClockStr(const char *pBuf)
{
	WAGC_pClockNum_t pclocknum = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pclocknum = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pclocknum = NULL;
		} else {
			pclocknum = (WAGC_pClockNum_t)calloc(1, sizeof(WAGC_ClockNum_t));
			if(pclocknum == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					pclocknum->Num = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pclocknum;
}

WAGC_pSelectLanguage_t WIFIAudio_GetCommand_SelectLanguageStr(const char *pBuf)
{
	WAGC_pSelectLanguage_t pselectlanguage = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pselectlanguage = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pselectlanguage = NULL;
		} else {
			pselectlanguage = (WAGC_pSelectLanguage_t)calloc(1, sizeof(WAGC_SelectLanguage_t));
			if(pselectlanguage == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					pselectlanguage->Language = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pselectlanguage;
}

WAGC_pPlaylistIndex_t WIFIAudio_GetCommand_DeletePlaylistMusicStr(const char *pBuf)
{
	WAGC_pPlaylistIndex_t pplaylistindex = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pplaylistindex = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pplaylistindex = NULL;
		} else {
			pplaylistindex = (WAGC_pPlaylistIndex_t)calloc(1, sizeof(WAGC_PlaylistIndex_t));
			if(pplaylistindex == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					pplaylistindex->Index = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pplaylistindex;
}

WAGC_pxzxAction_t WIFIAudio_GetCommand_xzxActionStr(const char *pBuf)
{
	WAGC_pxzxAction_t pxzxaction = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int num = 0;
	int i;
	
	if (pBuf == NULL)
	{
		pxzxaction = NULL;
	} else {
		pBufCpy = strdup(pBuf);
		strtok(pBufCpy, ":");	//MicarrayRecord:micarray:mic:5
		
		pxzxaction = (WAGC_pxzxAction_t)calloc(1, sizeof(WAGC_xzxAction_t));
		if(pxzxaction == NULL)
		{
		} else {
			pTmp = strtok(NULL, ":");
			if(!strcmp(pTmp,"setrt"))
			{
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_SETRT;
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pxzxaction->Value = atoi(pTmp);
				} else {
					pxzxaction->Value = 0;
				}
			} else if(!strcmp(pTmp,"playrec")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_PLAYREC;
			} else if(!strcmp(pTmp,"playurlrec")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_PLAYURLREC;
				pTmp = pTmp + strlen("playurlrec") + 1;
				if(strlen(pTmp)>0)
				{
					pxzxaction->pUrl = strdup(pTmp);
				} else {
					pxzxaction->pUrl = NULL;
				}
			} else if(!strcmp(pTmp,"startrecord")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_STARTRECORD;
			} else if(!strcmp(pTmp,"stoprecord")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_STOPRECORD;
			} else if(!strcmp(pTmp,"fixedrecord")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_FIXEDRECORD;
			} else if(!strcmp(pTmp,"getrecordfileurl")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_GETRECORDFILEURL;
			} else if(!strcmp(pTmp,"vad")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_VOICEACTIVITYDETECTION;
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pxzxaction->Value = atoi(pTmp);
				} else {
					pxzxaction->Value = 0;
				}
				
				pTmp = pTmp + strlen(pTmp) + 1;
				if(strlen(pTmp)>0)
				{
					pxzxaction->pCardName = strdup(pTmp);
				} else {
					pxzxaction->pCardName = NULL;
				}
			} else if(!strcmp(pTmp,"compare")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CHANNELCOMPARE;
			} else if(!strcmp(pTmp,"normalrecord")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_NORMALRECORD;
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pxzxaction->Value = atoi(pTmp);
				} else {
					pxzxaction->Value = 0;
				}
			} else if(!strcmp(pTmp,"getnormalurl")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_GETNORMALURL;
			} else if(!strcmp(pTmp,"micrecord")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921MICRECORD;
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pxzxaction->Value = atoi(pTmp);
				} else {
					pxzxaction->Value = 0;
				}
			} else if(!strcmp(pTmp,"getmicurl")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921GETMICURL;
			} else if(!strcmp(pTmp,"refrecord")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921REFRECORD;
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pxzxaction->Value = atoi(pTmp);
				} else {
					pxzxaction->Value = 0;
				}
			} else if(!strcmp(pTmp,"getrefurl")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921GETREFURL;
			} else if(!strcmp(pTmp,"stopModeRecord")) {
				pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_STOPMODERECORD;
			}else if(!strcmp(pTmp,"micarray")){		//doing
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{									
					if(!strcmp(pTmp,"mic")){		//doing
						pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_MICARRAYMICRECORD;
						pTmp = strtok(NULL, ":");
						if(NULL != pTmp)
						{
							pxzxaction->Value = atoi(pTmp);
						} else {
							pxzxaction->Value = 0;
						}
					} else if(!strcmp(pTmp,"ref")){
		
						pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_MICARRAYMICRECORD;
						pTmp = strtok(NULL, ":");
						if(NULL != pTmp)
						{
							pxzxaction->Value = atoi(pTmp);
						} else {
							pxzxaction->Value = 0;
						}
		
					}

				}			
			//	pxzxaction = WIFIAudio_GetCommand_getmicarraydate(pBuf);
			}else if(!strcmp(pTmp,"cx20921")){

				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					if(!strcmp(pTmp,"mic")){
						pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921MICRECORD;
						pTmp = strtok(NULL, ":");
						if(NULL != pTmp)
						{
							pxzxaction->Value = atoi(pTmp);
						} else {
							pxzxaction->Value = 0;
						}
					} else if(!strcmp(pTmp,"ref")){

						pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921REFRECORD;
						pTmp = strtok(NULL, ":");
						if(NULL != pTmp)
						{
							pxzxaction->Value = atoi(pTmp);
						} else {
							pxzxaction->Value = 0;
						}

					}else if(!strcmp(pTmp,"denoise")){

						pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921DENOISERECORD;
						pTmp = strtok(NULL, ":");
						if(NULL != pTmp)
						{
							pxzxaction->Value = atoi(pTmp);
						} else {
							pxzxaction->Value = 0;
						}

					}				
					//	pxzxaction = WIFIAudio_GetCommand_getcx20921date(pBuf);
				}
			}else {
				free(pxzxaction);
				pxzxaction = NULL;
			}
		}
		free(pBufCpy);
		pBufCpy = NULL;
	}
	
	return pxzxaction;
}

WAGC_pxzxAction_t WIFIAudio_GetCommand_ALLMICSTOP(const char *pBuf)
{
	WAGC_pxzxAction_t pxzxaction = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int num = 0;
	int i;
	
	if (pBuf == NULL)
	{
		pxzxaction = NULL;
	} else {
		pBufCpy = strdup(pBuf);
		strtok(pBufCpy, ":");	//MicarrayStop:micarray
		
		pxzxaction = (WAGC_pxzxAction_t)calloc(1, sizeof(WAGC_xzxAction_t));
		if(pxzxaction == NULL){
			
		} else {
			pTmp = strtok(NULL, ":");
			if (pTmp != NULL){
				if (!strcmp(pTmp,"micarray")){		//doing 
					pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_MICARRAYRECORDSTOP;
					pTmp = strtok(NULL, ":");
					if(NULL != pTmp)
					{
						pxzxaction->Value = atoi(pTmp);
					} else {
						pxzxaction->Value = 0;
					}
				} else if(!strcmp(pTmp,"cx20921")){
					pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921RECORDSTOP;
					pTmp = strtok(NULL, ":");
					if(NULL != pTmp)
					{
						pxzxaction->Value = atoi(pTmp);
					} else {
						pxzxaction->Value = 0;
					}
						
			 	}else {
					free(pxzxaction);
					pxzxaction = NULL;				
				} 	
			}
		}
		free(pBufCpy);
		pBufCpy = NULL;
	}
	
	return pxzxaction;
}

WAGC_pxzxAction_t WIFIAudio_GetCommand_ALLMICFILE(const char *pBuf)
{
	WAGC_pxzxAction_t pxzxaction = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int num = 0;
	int i;
	
	if (pBuf == NULL)
	{
		pxzxaction = NULL;
	} else {
		pBufCpy = strdup(pBuf);
		strtok(pBufCpy, ":");
		
		pxzxaction = (WAGC_pxzxAction_t)calloc(1, sizeof(WAGC_xzxAction_t));
		if(pxzxaction == NULL){
			
		} else {
			pTmp = strtok(NULL, ":");
			if (pTmp != NULL){
				if (!strcmp(pTmp,"micarray")){
				//	pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_MICARRAYRECORDFILE;
					pTmp = strtok(NULL, ":");
					if(NULL != pTmp)
					{
						if(!strcmp(pTmp,"mic")){
							pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_MICARRAYRECORDFILE;
							pTmp = strtok(NULL, ":");
							if(NULL != pTmp)
							{
								pxzxaction->Value = atoi(pTmp);
							} else {
								pxzxaction->Value = 0;
							}
						} else if(!strcmp(pTmp,"denoise")){

							pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_MICARRAYDENOISERECORDFILE;
							pTmp = strtok(NULL, ":");
							if(NULL != pTmp)
							{
								pxzxaction->Value = atoi(pTmp);
							} else {
								pxzxaction->Value = 0;
							}

						}					
						//pxzxaction->Value = atoi(pTmp);
					} 
				} else if(!strcmp(pTmp,"cx20921")){
					pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921RECORDFILE;
					pTmp = strtok(NULL, ":");
					if(NULL != pTmp)
					{
						if(!strcmp(pTmp,"mic")){
							pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921RECORDFILE;
							pTmp = strtok(NULL, ":");
							if(NULL != pTmp)
							{
								pxzxaction->Value = atoi(pTmp);
							} else {
								pxzxaction->Value = 0;
							}
						} else if(!strcmp(pTmp,"denoise")){

							pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921DENOISERECORDFILE;
							pTmp = strtok(NULL, ":");
							if(NULL != pTmp)
							{
								pxzxaction->Value = atoi(pTmp);
							} else {
								pxzxaction->Value = 0;
							}

						}					
					//	pxzxaction->Value = atoi(pTmp);
					} 	
			 	}else {
					free(pxzxaction);
					pxzxaction = NULL;				
				} 	
			}
		}
		free(pBufCpy);
		pBufCpy = NULL;
	}
	
	return pxzxaction;
}

WAGC_pxzxAction_t WIFIAudio_GetCommand_ALLREFFILE(const char *pBuf)
{
	WAGC_pxzxAction_t pxzxaction = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int num = 0;
	int i;
	
	if (pBuf == NULL)
	{
		pxzxaction = NULL;
	} else {
		pBufCpy = strdup(pBuf);
		strtok(pBufCpy, ":");
		
		pxzxaction = (WAGC_pxzxAction_t)calloc(1, sizeof(WAGC_xzxAction_t));
		if(pxzxaction == NULL){
			
		} else {
			pTmp = strtok(NULL, ":");
			if (pTmp != NULL){
				if (!strcmp(pTmp,"micarray")){
					pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_MICARRAYREFRECORDFILE;
					pTmp = strtok(NULL, ":");
					if(NULL != pTmp)
					{
						pxzxaction->Value = atoi(pTmp);
					} else {
						pxzxaction->Value = 0;
					}
				} else if(!strcmp(pTmp,"cx20921")){
					pxzxaction->Attr = WAGC_COMMAND_XZXACTIONATTR_TYPE_CX20921REFRECORDFILE;
					pTmp = strtok(NULL, ":");
					if(NULL != pTmp)
					{
						pxzxaction->Value = atoi(pTmp);
					} else {
						pxzxaction->Value = 0;
					}
						
			 	}else {
					free(pxzxaction);
					pxzxaction = NULL;				
				} 	
			}
		}
		free(pBufCpy);
		pBufCpy = NULL;
	}
	
	return pxzxaction;
}


WAGC_pPlayTfCard_t WIFIAudio_GetCommand_PlayTfCardStr(const char *pBuf)
{
	WAGC_pPlayTfCard_t pPlayTfCard = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pPlayTfCard = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pPlayTfCard = NULL;
		} else {
			pPlayTfCard = (WAGC_pPlayTfCard_t)calloc(1, sizeof(WAGC_PlayTfCard_t));
			if(pPlayTfCard == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					pPlayTfCard->Index = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pPlayTfCard;
}

WAGC_pPlayUsbDisk_t WIFIAudio_GetCommand_PlayUsbDiskStr(const char *pBuf)
{
	WAGC_pPlayUsbDisk_t pPlayUsbDisk = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pPlayUsbDisk = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pPlayUsbDisk = NULL;
		} else {
			pPlayUsbDisk = (WAGC_pPlayUsbDisk_t)calloc(1, sizeof(WAGC_PlayUsbDisk_t));
			if(pPlayUsbDisk == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					pPlayUsbDisk->Index = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pPlayUsbDisk;
}

WAGC_pRemoveSynchronousEx_t WIFIAudio_GetCommand_RemoveSynchronousExStr(const char *pBuf)
{
	WAGC_pRemoveSynchronousEx_t pRemoveSynchronousEx = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int i = 0;
	
	if (pBuf == NULL)
	{
		pRemoveSynchronousEx = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pRemoveSynchronousEx = NULL;
		} else {
			pRemoveSynchronousEx = (WAGC_pRemoveSynchronousEx_t)calloc(1, sizeof(WAGC_RemoveSynchronousEx_t));
			if(pRemoveSynchronousEx == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				pTmp = strtok(pBufCpy, ":");
				
				pRemoveSynchronousEx->pMac = strdup(pTmp + strlen(pTmp) + 1);
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pRemoveSynchronousEx;
}

WAGC_pSetSynchronousEx_t WIFIAudio_GetCommand_SetSynchronousExStr(const char *pBuf)
{
	WAGC_pSetSynchronousEx_t psetsynchronousex = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		psetsynchronousex = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			psetsynchronousex = NULL;
		} else {
			psetsynchronousex = (WAGC_pSetSynchronousEx_t)calloc(1, sizeof(WAGC_SetSynchronousEx_t));
			if(psetsynchronousex == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(!strcmp(pTmp,"vol"))
				{
					psetsynchronousex->Attr = WAGC_COMMAND_SETSYNCHRONOUSATTREX_TYPE_VOLUME;
					pTmp = strtok(NULL, ":");
					psetsynchronousex->Value = atoi(pTmp);
					
					psetsynchronousex->pMac = strdup(pTmp + strlen(pTmp) + 1);
				} else {
					free(psetsynchronousex);
					psetsynchronousex = NULL;
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return psetsynchronousex;
}

WAGC_pBluetooth_t WIFIAudio_GetCommand_BluetoothStr(const char *pBuf)
{
	WAGC_pBluetooth_t pbluetooth = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pbluetooth = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pbluetooth = NULL;
		} else {
			pbluetooth = (WAGC_pBluetooth_t)calloc(1, sizeof(WAGC_Bluetooth_t));
			if(pbluetooth == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				pTmp = strtok(NULL, ":");
				if(!strcmp(pTmp, "Search"))
				{
					pbluetooth->Attr = WAGC_COMMAND_BLUETOOTHATTR_TYPE_SEARCH;
				} else if(!strcmp(pTmp, "GetDeviceList")) {
					pbluetooth->Attr = WAGC_COMMAND_BLUETOOTHATTR_TYPE_GETDEVICELIST;
				} else if(!strcmp(pTmp, "Connect")) {
					pbluetooth->Attr = WAGC_COMMAND_BLUETOOTHATTR_TYPE_CONNECT;
					pTmp = strtok(NULL, ":");
					pbluetooth->pAddress = strdup(pTmp);
				} else if(!strcmp(pTmp, "Disconnect")) {
					pbluetooth->Attr = WAGC_COMMAND_BLUETOOTHATTR_TYPE_DISCONNECT;
				} else {
					free(pbluetooth);
					pbluetooth = NULL;
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pbluetooth;
}

WAGC_pSetSpeechRecognition_t WIFIAudio_GetCommand_SetSpeechRecognitionStr(const char *pBuf)
{
	WAGC_pSetSpeechRecognition_t pSetSpeechRecognition = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pSetSpeechRecognition = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pSetSpeechRecognition = NULL;
		} else {
			pSetSpeechRecognition = (WAGC_pSetSpeechRecognition_t)calloc(1, sizeof(WAGC_SetSpeechRecognition_t));
			if(pSetSpeechRecognition == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					pSetSpeechRecognition->Platform = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pSetSpeechRecognition;
}

WAGC_pSetSingleLight_t WIFIAudio_GetCommand_SetSingleLightStr(const char *pBuf)
{
	WAGC_pSetSingleLight_t pSetSingleLight = NULL;
	char *pBufCpy = NULL;
	
	if (pBuf == NULL)
	{
		pSetSingleLight = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pSetSingleLight = NULL;
		} else {
			pSetSingleLight = (WAGC_pSetSingleLight_t)calloc(1, sizeof(WAGC_SetSingleLight_t));
			if(pSetSingleLight == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				pSetSingleLight->Enable = atoi(strtok(NULL, ":"));
				pSetSingleLight->Brightness = atoi(strtok(NULL, ":"));
				pSetSingleLight->Red = atoi(strtok(NULL, ":"));
				pSetSingleLight->Green = atoi(strtok(NULL, ":"));
				pSetSingleLight->Blue = atoi(strtok(NULL, ":"));
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pSetSingleLight;
}

WAGC_pSetTestMode_t WIFIAudio_GetCommand_SetTestModeStr(const char *pBuf)
{
	WAGC_pSetTestMode_t pSetTestMode = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pSetTestMode = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pSetTestMode = NULL;
		} else {
			pSetTestMode = (WAGC_pSetTestMode_t)calloc(1, sizeof(WAGC_SetTestMode_t));
			if(pSetTestMode == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					pSetTestMode->Mode = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pSetTestMode;
}

WAGC_pSetEnable_t WIFIAudio_GetCommand_SetEnableStr(const char *pBuf)
{
	WAGC_pSetEnable_t pSetEnable = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pSetEnable = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pSetEnable = NULL;
		} else {
			pSetEnable = (WAGC_pSetEnable_t)calloc(1, sizeof(WAGC_SetEnable_t));
			if(pSetEnable == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					pSetEnable->Enable = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pSetEnable;
}

WAGC_pPlayTestMusic_t WIFIAudio_GetCommand_PlayTestMusicStr(const char *pBuf)
{
	WAGC_pPlayTestMusic_t pPlayTestMusic = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	int num = 0;
	char urlstr[512];
	int i = 0;
	
	if (pBuf == NULL)
	{
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
		} else {
			pPlayTestMusic = (WAGC_pPlayTestMusic_t)calloc(1, sizeof(WAGC_PlayTestMusic_t));
			if (pPlayTestMusic == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				num = 0;
				
				while(1)
				{
					if(NULL == strtok(NULL, ":"))
					{
						num--;
						break;
					} else {
						num++;
					}
				}
				
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				memset(urlstr, 0, 512);
				for(i = 0; i<num+1; i++)
				{
					if(0 != i)
					{
						strcat(urlstr, ":");
					}
					strcat(urlstr, strtok(NULL, ":"));
				}
				if(strlen(urlstr) == 0)
				{
					pPlayTestMusic->pURL = NULL;
				} else {
					pPlayTestMusic->pURL = strdup(urlstr);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pPlayTestMusic;
}

WAGC_pAlarmAndPlan_t WIFIAudio_GetCommand_AlarmAndPlanStr(const char *pBuf)
{
	WAGC_pAlarmAndPlan_t pAlarmAndPlan = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pAlarmAndPlan = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pAlarmAndPlan = NULL;
		} else {
			pAlarmAndPlan = (WAGC_pAlarmAndPlan_t)calloc(1, sizeof(WAGC_AlarmAndPlan_t));
			if(pAlarmAndPlan == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				pAlarmAndPlan->Num = atoi(strtok(NULL, ":"));
				pTmp = strtok(NULL, ":");
				if(NULL != pTmp)
				{
					pAlarmAndPlan->Enable = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pAlarmAndPlan;
}

WAGC_pShortcutKey_t WIFIAudio_GetCommand_ShortcutKeyStr(const char *pBuf)
{
	WAGC_pShortcutKey_t pShortcutKey = NULL;
	char *pBufCpy = NULL;
	char *pTmp = NULL;
	
	if (pBuf == NULL)
	{
		pShortcutKey = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pShortcutKey = NULL;
		} else {
			pShortcutKey = (WAGC_pShortcutKey_t)calloc(1, sizeof(WAGC_ShortcutKey_t));
			if(pShortcutKey == NULL)
			{
			} else {
				memset(pBufCpy, 0, strlen(pBuf) + 1);
				strcpy(pBufCpy, pBuf);
				strtok(pBufCpy, ":");
				
				pTmp = strtok(NULL, ":");
				if(pTmp != NULL)
				{
					pShortcutKey->Num = atoi(pTmp);
				}
			}
			free(pBufCpy);
			pBufCpy = NULL;
		}
	}
	
	return pShortcutKey;
}

WAGC_pComParm WIFIAudio_GetCommand_newComParm(const char *pBuf)
{
	WAGC_pComParm pComParm = NULL;
	char *pBufCpy = NULL;
	char *pCommand = NULL;
	
	if (pBuf == NULL)
	{
		pComParm = NULL;
	} else {
		pBufCpy = (char *)calloc(1, strlen(pBuf) + 1);
		if(pBufCpy == NULL)
		{
			pComParm = NULL;
		} else {
			strcpy(pBufCpy,pBuf);
			pCommand = strtok(pBufCpy, ":");
			pComParm = (WAGC_pComParm)calloc(1, sizeof(WAGC_ComParm));
			if (pComParm == NULL)
			{
			} else {
				if(pCommand != NULL)
				{
					if (!strcmp(pCommand, "wlanGetApList"))
					{
						pComParm->Command = WIFIAUDIO_GETCOMMAND_WLANGETAPLIST; 
					} else if (!strcmp(pCommand, "wlanConnectAp")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_WLANCONNECTAP; 
						pComParm->Parameter.pConnectAp = WIFIAudio_GetCommand_getConnectApStr(pBuf); 
					} else if (!strcmp(pCommand, "GetCurrentWirelessConnect")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETCURRENTWIRELESSCONNECT; 
					} else if (!strcmp(pCommand, "GetDeviceInfo")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETDEVICEINFO; 
					}else if (!strcmp(pCommand, "getUsbStatus")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETUSBSTATUS; 	//USB
					}else if (!strcmp(pCommand, "getSdStatus")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETSDSTATUS; 	//SD卡
					}else if (!strcmp(pCommand, "getOneTouchConfigure")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETONETOUCHCONFIGURE; 
					} else if (!strcmp(pCommand, "setOneTouchConfigure")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETONETOUCHCONFIGURE; 
						pComParm->Parameter.pOneTouchState = WIFIAudio_GetCommand_setOneTouchConfigureStr(pBuf);
					} else if (!strcmp(pCommand, "getOneTouchConfigureSetDeviceName")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETONETOUCHCONFIGURESETDEVICENAME; 
					} else if (!strcmp(pCommand, "setOneTouchConfigureSetDeviceName")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETONETOUCHCONFIGURESETDEVICENAME; 
						pComParm->Parameter.pOneTouchNameState = WIFIAudio_GetCommand_setOneTouchConfigureSetDeviceNameStr(pBuf);
					} else if (!strcmp(pCommand, "getLoginAmazonStatus")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETLOGINAMAZONSTATUS; 
					} else if (!strcmp(pCommand, "LogoutAmazon")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_LOGOUTAMAZON; 
					} else if (!strcmp(pCommand, "SetAlexaLanguage")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETALEXALANGUAGE; 
						pComParm->Parameter.pSetAlexaLanguage = WIFIAudio_GetCommand_SetAlexaLanguageStr(pBuf);
					} else if (!strcmp(pCommand, "GetAlexaLanguage")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETALEXALANGUAGE; 
					} else if (!strcmp(pCommand, "ChangeNetworkConnect")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_CHANGENETWORKCONNECT; 
					} else if (!strcmp(pCommand, "DisconnectWiFi")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_DISCONNCTWIFI;
					} else if (!strcmp(pCommand, "GetBatteryLevel")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETBATTERYLEVEL;
					} else if (!strcmp(pCommand, "IsChargePlug")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_ISCHARGEPLUG;
					} else if (!strcmp(pCommand, "GetWakeupTimes")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETWAKEUPTIMES;
					} else if (!strcmp(pCommand, "getPlayerStatus")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETPLAYERSTATUS; 
					} else if (!strcmp(pCommand, "setPlayerCmd")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETPLAYERCMD; 
						pComParm->Parameter.pSetPlayerCmd = WIFIAudio_GetCommand_getSetPlayerCmdStr(pBuf);
					} else if (!strcmp(pCommand, "getPlayerCmd")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETPLAYERCMD; 
						pComParm->Parameter.pGetPlayerCmd = WIFIAudio_GetCommand_getGetPlayerCmdStr(pBuf);
					} else if (!strcmp(pCommand, "getEqualizer")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETEQUALIZER; 
					} else if (!strcmp(pCommand, "getSynchronousInfoEx")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETSYNCHRONOUSINFOEX; 
					} else if (!strcmp(pCommand, "removeSynchronousEx")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_REMOVESYNCHRONOUSEX; 
						pComParm->Parameter.pRemoveSynchronousEx = WIFIAudio_GetCommand_RemoveSynchronousExStr(pBuf);
					} else if (!strcmp(pCommand, "setSynchronousEx")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETSYNCHRONOUSEX; 
						pComParm->Parameter.pSetSynchronousEx = WIFIAudio_GetCommand_SetSynchronousExStr(pBuf);
					} else if (!strcmp(pCommand, "setSynchronousChannel")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETSYNCHRONOUSCHANNEL;
					} else if (!strcmp(pCommand, "setSSID")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETSSID; 
						pComParm->Parameter.pSSID = WIFIAudio_GetCommand_getSSIDStr(pBuf);
					} else if (!strcmp(pCommand, "setNetwork")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETNETWORK; 
						pComParm->Parameter.pNetwork = WIFIAudio_GetCommand_getNetworkStr(pBuf);
					} else if (!strcmp(pCommand, "restoreToDefault")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_RESTORETODEFAULT; 
					} else if (!strcmp(pCommand, "reboot")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_REBOOT; 
					} else if (!strcmp(pCommand, "setDeviceName")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETDEVICENAME; 
						pComParm->Parameter.pName = WIFIAudio_GetCommand_setDeviceNameStr(pBuf);
					} else if (!strcmp(pCommand, "setShutdown")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETSHUTDOWN; 
						pComParm->Parameter.pShutdown = WIFIAudio_GetCommand_setShutdownStr(pBuf);
					} else if (!strcmp(pCommand, "getShutdown")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETSHUTDOWN; 
					} else if (!strcmp(pCommand, "setPowerWifiDown")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETPOWERWIFIDOWN; 
					} else if (!strcmp(pCommand, "getManuFacturerInfo")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETMANUFACTURERINFO; 
					} else if (!strcmp(pCommand, "PowerOnAutoPlay")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_POWERONAUTOPLAY; 
						pComParm->Parameter.pSetEnable = WIFIAudio_GetCommand_SetEnableStr(pBuf);
					} else if (!strcmp(pCommand, "SetTestMode")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETTESTMODE; 
						pComParm->Parameter.pSetTestMode = WIFIAudio_GetCommand_SetTestModeStr(pBuf);
					} else if (!strcmp(pCommand, "GetTestMode")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETTESTMODE; 
					} else if (!strcmp(pCommand, "PlayTestMusic")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_PLAYTESTMUSIC;
						pComParm->Parameter.pPlayTestMusic = WIFIAudio_GetCommand_PlayTestMusicStr(pBuf);
					} else if (!strcmp(pCommand, "StopTestMusic")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_STOPTESTMUSIC;
					} else if (!strcmp(pCommand, "GetTheLatestVersionNumberOfFirmware")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETTHELATESTVERSIONNUMBEROFFIRMWARE; 
					} else if (!strcmp(pCommand, "UpgradeWiFiAndBluetooth")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_UPGRADEWIFIANDBLUETOOTH; 
						pComParm->Parameter.pUpdateURL = WIFIAudio_GetCommand_getRemoteUpdateStr(pBuf);
					} else if (!strcmp(pCommand, "UpgradeWiFiAndBluetoothState")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_UPGRADEWIFIANDBLUETOOTHSTATE; 
					} else if (!strcmp(pCommand, "setTimeSync")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETTIMESYNC; 
						pComParm->Parameter.pDateTime = WIFIAudio_GetCommand_setTimeSyncStr(pBuf);
					} else if (!strcmp(pCommand, "getDeviceTime")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETDEVICETIME; 
					} else if (!strcmp(pCommand, "getAlarmClock")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETALARMCLOCK; 
						pComParm->Parameter.pClockNum = WIFIAudio_GetCommand_getAlarmClockStr(pBuf);
					} else if (!strcmp(pCommand, "alarmStop")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_ALARMSTOP; 
					} else if (!strcmp(pCommand, "RemoveAlarmClock")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_REMOVEALARMCLOCK; 
						pComParm->Parameter.pAlarmAndPlan = WIFIAudio_GetCommand_AlarmAndPlanStr(pBuf);
					} else if (!strcmp(pCommand, "EnableAlarmClock")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_ENABLEALARMCLOCK; 
						pComParm->Parameter.pAlarmAndPlan = WIFIAudio_GetCommand_AlarmAndPlanStr(pBuf);
					} else if (!strcmp(pCommand, "RemovePlayPlan")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_REMOVEPLAYPLAN; 
						pComParm->Parameter.pAlarmAndPlan = WIFIAudio_GetCommand_AlarmAndPlanStr(pBuf);
					} else if (!strcmp(pCommand, "GetAlarmClockAndPlayPlan")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETALARMCLOCKANDPLAYPLAN; 
					} else if (!strcmp(pCommand, "GetPlanMusicList")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETPLANMUSICLIST; 
						pComParm->Parameter.pAlarmAndPlan = WIFIAudio_GetCommand_AlarmAndPlanStr(pBuf);
					} else if (!strcmp(pCommand, "EnablePlayMusicAsPlan")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_ENABLEPLAYMUSICASPLAN; 
						pComParm->Parameter.pAlarmAndPlan = WIFIAudio_GetCommand_AlarmAndPlanStr(pBuf);
					} else if (!strcmp(pCommand, "GetPlanMixedList")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETPLANMIXEDLIST; 
						pComParm->Parameter.pAlarmAndPlan = WIFIAudio_GetCommand_AlarmAndPlanStr(pBuf);
					} else if (!strcmp(pCommand, "enableAutoTimeSync")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_ENABLEAUTOTIMESYNC; 
						pComParm->Parameter.pSetEnable = WIFIAudio_GetCommand_SetEnableStr(pBuf);
					} else if (!strcmp(pCommand, "SelectLanguage")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SELECTLANGUAGE; 
						pComParm->Parameter.pSelectLanguage = WIFIAudio_GetCommand_SelectLanguageStr(pBuf);
					} else if (!strcmp(pCommand, "getLanguage")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETLANGUAGE; 
					} else if (!strcmp(pCommand, "GetCurrentPlaylist")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETCURRENTPLAYLIST; 
					} else if (!strcmp(pCommand, "DeletePlaylistMusic")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_DELETEPLAYLISTMUSIC; 
						pComParm->Parameter.pPlaylistIndex = WIFIAudio_GetCommand_DeletePlaylistMusicStr(pBuf);
					} else if (!strcmp(pCommand, "RemoveShortcutKeyList")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_REMOVESHORTCUTKEYLIST; 
						pComParm->Parameter.pShortcutKey = WIFIAudio_GetCommand_ShortcutKeyStr(pBuf);
					} else if (!strcmp(pCommand, "GetShortcutKeyList")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETSHORTCUTKEYLIST; 
						pComParm->Parameter.pShortcutKey = WIFIAudio_GetCommand_ShortcutKeyStr(pBuf);
					} else if (!strcmp(pCommand, "xzxAction")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_XZXACTION; 
						pComParm->Parameter.pxzxAction = WIFIAudio_GetCommand_xzxActionStr(pBuf);
					} else if (!strcmp(pCommand, "GetMicarrayInfo")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETMICARRAYINFO;		//远场录音1
					} else if (!strcmp(pCommand, "MicarrayRecord")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_MICARRAYRECORD;		//远场录音2
						pComParm->Parameter.pxzxAction = WIFIAudio_GetCommand_xzxActionStr(pBuf);
					} else if (!strcmp(pCommand, "MicarrayNormal")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_MICARRAYNORMAL;
					} else if (!strcmp(pCommand, "MicarrayMic")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETRECORDFILE;
						pComParm->Parameter.pxzxAction = WIFIAudio_GetCommand_ALLMICFILE(pBuf);						
					} else if (!strcmp(pCommand, "MicarrayRef")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_MICARRAYREF;
						pComParm->Parameter.pxzxAction = WIFIAudio_GetCommand_ALLREFFILE(pBuf);	
					} else if (!strcmp(pCommand, "MicarrayStop")) {	
						pComParm->Command = WIFIAUDIO_GETCOMMAND_MICARRAYSTOP;			//远场录音3
						pComParm->Parameter.pxzxAction = WIFIAudio_GetCommand_ALLMICSTOP(pBuf);
					} else if (!strcmp(pCommand, "StorageDeviceOnlineState")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_STORAGEDEVICEONLINESTATE; 
					} else if (!strcmp(pCommand, "GetTfCardPlaylistInfo")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETTFCARDPLAYLISTINFO; 
					} else if (!strcmp(pCommand, "PlayTfCard")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_PLAYTFCARD; 
						pComParm->Parameter.pPlayTfCard = WIFIAudio_GetCommand_PlayTfCardStr(pBuf);
					} else if (!strcmp(pCommand, "GetUsbDiskPlaylistInfo")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETUSBDISKPLAYLISTINFO; 
					} else if (!strcmp(pCommand, "PlayUsbDisk")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_PLAYUSBDISK; 
						pComParm->Parameter.pPlayUsbDisk = WIFIAudio_GetCommand_PlayUsbDiskStr(pBuf);
					} else if (!strcmp(pCommand, "Bluetooth")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_BLUETOOTH; 
						pComParm->Parameter.pBluetooth = WIFIAudio_GetCommand_BluetoothStr(pBuf);
					} else if (!strcmp(pCommand, "GetSpeechRecognition")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETSPEECHRECOGNITION; 
					} else if (!strcmp(pCommand, "SetSpeechRecognition")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETSPEECHRECOGNITION; 
						pComParm->Parameter.pSetSpeechRecognition = WIFIAudio_GetCommand_SetSpeechRecognitionStr(pBuf);
					} else if (!strcmp(pCommand, "UpdataConexant")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_UPDATACONEXANT;
					} else if (!strcmp(pCommand, "GetConexantVersion")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETCONEXANTVERSION;
					} else if (!strcmp(pCommand, "LogCaptureEnable")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_LOGCAPTUREENABLE;
					} else if (!strcmp(pCommand, "LogCaptureDisable")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_LOGCAPTUREDISABLE;
					} else if (!strcmp(pCommand, "getStatus")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETSTATUS; 
					} else if (!strcmp(pCommand, "getStatusEx")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETSTATUSEX; 
					} else if (!strcmp(pCommand, "wlanGetConnectState")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_WLANGETCONNECTSTATE; 
					} else if (!strcmp(pCommand, "GetCurrentWirelessConnectEx")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETCURRENTWIRELESSCONNECTEX; 
					} else if (!strcmp(pCommand, "getLoginAmazonNeedInfo")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETLOGINAMAZONNEEDINFO; 
					} else if (!strcmp(pCommand, "wlanGetApListEx")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_WLANGETAPLISTEX; 
					} else if (!strcmp(pCommand, "wpsservermode")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_WPSSERVERMODE; 
					} else if (!strcmp(pCommand, "wpscancel")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_WPSCANCEL; 
					} else if (!strcmp(pCommand, "GetDownloadProgress")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETDOWNLOADPROGRESS; 
					} else if (!strcmp(pCommand, "GetBluetoothDownloadProgress")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETBLUETOOTHDOWNLOADPROGRESS; 
					} else if (!strcmp(pCommand, "wlanConectAp")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_WLANCONECTAP; 
						pComParm->Parameter.pConectAp = WIFIAudio_GetCommand_getConectApStr(pBuf);  
					} else if (!strcmp(pCommand, "wlanSubConnectAp")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_WLANSUBCONNECTAP; 
						pComParm->Parameter.pSubConnectAp = WIFIAudio_GetCommand_getSubConnectApStr(pBuf); 
					} else if (!strcmp(pCommand, "wlanConnectApEx")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_WLANCONNECTAPEX; 
						pComParm->Parameter.pConnectApEx = WIFIAudio_GetCommand_getConnectApExStr(pBuf);
					} else if (!strcmp(pCommand, "multiroom")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_MULTIROOM; 
						pComParm->Parameter.pMultiroom = WIFIAudio_GetCommand_getMultiroomStr(pBuf);
					} else if (!strcmp(pCommand, "getMvRemoteUpdate")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_GETMVREMOTEUPDATE; 
						pComParm->Parameter.pUpdateURL = WIFIAudio_GetCommand_getRemoteUpdateStr(pBuf);
					} else if (!strcmp(pCommand, "SetSingleLight")) {
						pComParm->Command = WIFIAUDIO_GETCOMMAND_SETSINGLELIGHT; 
						pComParm->Parameter.pSetSingleLight = WIFIAudio_GetCommand_SetSingleLightStr(pBuf);
					} else {
						free(pComParm);
						pComParm = NULL;
					}
				}
			}
		}
	}
	
	free(pBufCpy);
	pBufCpy = NULL;
	
	return pComParm;
}

