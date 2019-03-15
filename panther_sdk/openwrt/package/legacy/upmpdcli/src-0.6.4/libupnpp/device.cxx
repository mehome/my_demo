/* Copyright (C) 2014 J.F.Dockes
 *	 This program is free software; you can redistribute it and/or modify
 *	 it under the terms of the GNU General Public License as published by
 *	 the Free Software Foundation; either version 2 of the License, or
 *	 (at your option) any later version.
 *
 *	 This program is distributed in the hope that it will be useful,
 *	 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	 GNU General Public License for more details.
 *
 *	 You should have received a copy of the GNU General Public License
 *	 along with this program; if not, write to the
 *	 Free Software Foundation, Inc.,
 *	 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "config.h"

#include <time.h>
#include <sys/time.h>
#include <string.h>

#include <iostream>
#include <cerrno>
#include <signal.h>
#include <stdlib.h>
using namespace std;

#include "upnpplib.hxx"
#include "vdir.hxx"
#include "device.hxx"
#include "log.hxx"

static LibUPnP *m_lib_g;
unsigned int connect_ip;
//#define DEBUG
unordered_map<std::string, UpnpDevice *> UpnpDevice::o_devices;

static string xmlquote(const string& in)
{
    string out;
    for (unsigned int i = 0; i < in.size(); i++) {
        switch(in[i]) {
        case '"': out += "&quot;";break;
        case '&': out += "&amp;";break;
        case '<': out += "&lt;";break;
        case '>': out += "&gt;";break;
        case '\'': out += "&apos;";break;
        default: out += in[i];
        }
    }
    return out;
}

static bool vectorstoargslists(const vector<string>& names, 
                               const vector<string>& values,
                               vector<string>& qvalues,
                               vector<const char *>& cnames,
                               vector<const char *>& cvalues)
{
    if (names.size() != values.size()) {
        LOGERR("vectorstoargslists: bad sizes" << endl);
        return false;
    }

    cnames.reserve(names.size());
    for (unsigned int i = 0; i < names.size(); i++) {
        cnames.push_back(names[i].c_str());
    }
    qvalues.clear();
    qvalues.reserve(values.size());
    cvalues.reserve(values.size());
    for (unsigned int i = 0; i < values.size(); i++) {
        qvalues.push_back(xmlquote(values[i]));
        cvalues.push_back(qvalues[i].c_str());
    }
    return true;
}

UpnpDevice::UpnpDevice(const string& deviceId, 
                       const unordered_map<string, string>& xmlfiles)
    : m_deviceId(deviceId), connectip(0)
{
    //LOGDEB("UpnpDevice::UpnpDevice(" << m_deviceId << ")" << endl);

    m_lib_g = m_lib = LibUPnP::getLibUPnP(true);
    if (!m_lib) {
        LOGFAT(" Can't get LibUPnP" << endl);
        return;
    }
    if (!m_lib->ok()) {
        LOGFAT("Lib init failed: " <<
               m_lib->errAsString("main", m_lib->getInitError()) << endl);
        m_lib = 0;
        return;
    }

    if (o_devices.empty()) {
        // First call: init callbacks
        m_lib->registerHandler(UPNP_CONTROL_ACTION_REQUEST, sCallBack, this);
		m_lib->registerHandler(UPNP_CONTROL_GET_VAR_REQUEST, sCallBack, this);
		m_lib->registerHandler(UPNP_EVENT_SUBSCRIPTION_REQUEST, sCallBack,this);
    }

    VirtualDir* theVD = VirtualDir::getVirtualDir();
    if (theVD == 0) {
        LOGFAT("UpnpDevice::UpnpDevice: can't get VirtualDir" << endl);
        return;
    }

    unordered_map<string,string>::const_iterator it = 
        xmlfiles.find("description.xml");
    if (it == xmlfiles.end()) {
        LOGFAT("UpnpDevice::UpnpDevice: no description.xml found in xmlfiles"
               << endl);
        return;
    } 

    const string& description = it->second;

    for (it = xmlfiles.begin(); it != xmlfiles.end(); it++) {
        theVD->addFile("/", it->first, it->second, "application/xml");
    }

    // Start up the web server for sending out description files
    m_lib->setupWebServer(description);

/*
    if (m_lib_g->ok()) {
		UpnpUnRegisterRootDevice(m_lib_g->getdvh());
	}
	m_lib->setupWebServer(description);
*/

    o_devices[m_deviceId] = this;
}

static PTMutexInit cblock;

// Main libupnp callback: use the device id and call the right device
int UpnpDevice::sCallBack(Upnp_EventType et, void* evp, void* tok)
{
    //LOGDEB("UpnpDevice::sCallBack" << endl);
    PTMutexLocker lock(cblock);

    string deviceid;
    switch (et) {
    case UPNP_CONTROL_ACTION_REQUEST:
        deviceid = ((struct Upnp_Action_Request *)evp)->DevUDN;
    break;

    case UPNP_CONTROL_GET_VAR_REQUEST:
        deviceid = ((struct Upnp_State_Var_Request *)evp)->DevUDN;
    break;

    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        deviceid = ((struct  Upnp_Subscription_Request*)evp)->UDN;
    break;

    default:
        LOGERR("UpnpDevice::sCallBack: unknown event " << et << endl);
        return UPNP_E_INVALID_PARAM;
    }
    // LOGDEB("UpnpDevice::sCallBack: deviceid[" << deviceid << "]" << endl);

    unordered_map<std::string, UpnpDevice *>::iterator it =
        o_devices.find(deviceid);

    if (it == o_devices.end()) {
        LOGERR("UpnpDevice::sCallBack: Device not found: [" << 
               deviceid << "]" << endl);
        return UPNP_E_INVALID_PARAM;
    }
    // LOGDEB("UpnpDevice::sCallBack: device found: [" << it->second 
    // << "]" << endl);
    return (it->second)->callBack(et, evp);
}

int UpnpDevice::callBack(Upnp_EventType et, void* evp)
{
    switch (et) {
    case UPNP_CONTROL_ACTION_REQUEST:
    {
        struct Upnp_Action_Request *act = (struct Upnp_Action_Request *)evp;
        LOGDEB("UPNP_CONTROL_ACTION_REQUEST: " << act->ActionName <<
               ". Params: " << ixmlPrintDocument(act->ActionRequest) << endl);

		struct sockaddr_in *addr_v4 = (struct sockaddr_in *)&act->CtrlPtIPAddr;
		unsigned int linkip = get_linkip();
		dlna_log("Action: [%s] from:%x\n",act->ActionName, addr_v4->sin_addr.s_addr);

        unordered_map<string, string>::const_iterator servit = 
            m_serviceTypes.find(act->ServiceID);
        if (servit == m_serviceTypes.end()) {
            LOGERR("Bad serviceID" << endl);
            return UPNP_E_INVALID_PARAM;
        }

		/*For different device check*/	
        unordered_map<string, char>::iterator checkit = 
            m_checks.find(act->ActionName);
        if (checkit == m_checks.end()) {
            LOGINF("No such action: " << act->ActionName << endl);
            return UPNP_E_INVALID_PARAM;
        }

		if(m_checks[act->ActionName] == SETUP) {
			set_linkip(addr_v4->sin_addr.s_addr);
			linkip = addr_v4->sin_addr.s_addr;
			dlna_log("Action:%s and its policy:%d ip:%x\n",act->ActionName,m_checks[act->ActionName]);
		}
		else if(m_checks[act->ActionName] == SKIP)
			goto todo; 

		if( (linkip!=0) && (linkip != addr_v4->sin_addr.s_addr)) {
			dlna_log("Drop action because device changed\n");
            return UPNP_E_INVALID_PARAM;
		}
        /*End*/
todo:
		const string& servicetype = servit->second;

        unordered_map<string, soapfun>::iterator callit = 
            m_calls.find(act->ActionName);
        if (callit == m_calls.end()) {
            LOGINF("No such action: " << act->ActionName << endl);
            return UPNP_E_INVALID_PARAM;
        }

        SoapArgs sc;
        if (!decodeSoapBody(act->ActionName, act->ActionRequest, &sc)) {
            LOGERR("Error decoding Action call arguments" << endl);
            return UPNP_E_INVALID_PARAM;
        }
        SoapData dt;
        dt.name = act->ActionName;
        dt.serviceType = servicetype;
		dt.pktip = addr_v4->sin_addr.s_addr;

        // Call the action routine
        int ret = callit->second(sc, dt);
        if (ret != UPNP_E_SUCCESS) {
            LOGERR("Action failed: " << sc.name << endl);
            return ret;
        }

        // Encode result data
        act->ActionResult = buildSoapBody(dt);

/*Print our Response data*/
#ifdef DEBUG
		LOGDEB("Response data: " << 
           ixmlPrintDocument(act->ActionResult) << endl);
#endif
        return ret;
    }
    break;

    case UPNP_CONTROL_GET_VAR_REQUEST:
        // Note that the "Control: query for variable" action is
        // deprecated (upnp arch v1), and we should never get these.
    {
        struct Upnp_State_Var_Request *act = 
            (struct Upnp_State_Var_Request *)evp;
        LOGDEB("UPNP_CONTROL_GET_VAR__REQUEST?: " << act->StateVarName << endl);
    }
    break;

    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
    {
        struct Upnp_Subscription_Request *act = 
            (struct  Upnp_Subscription_Request*)evp;
        LOGDEB("UPNP_EVENT_SUBSCRIPTION_REQUEST: " << act->ServiceId << endl);

        vector<string> names, values, qvalues;
		
		/*QQ ATTACH: first event will reject, then QQ another subscription request*/
        if (!getEventData(true, act->ServiceId, names, values)) {
            break;
        }
        vector<const char *> cnames, cvalues;
        vectorstoargslists(names, values, qvalues, cnames, cvalues);
        int ret = 
            UpnpAcceptSubscription(m_lib->getdvh(), act->UDN, act->ServiceId,
                                   &cnames[0], &cvalues[0],
                                   int(cnames.size()), act->Sid);
        if (ret != UPNP_E_SUCCESS) {
            LOGERR("UpnpDevice::callBack: UpnpAcceptSubscription failed: " 
                   << ret << endl);
        }

        return ret;
    }
    break;

    default:
        LOGINF("UpnpDevice::callBack: unknown libupnp event type: " <<
               LibUPnP::evTypeAsString(et).c_str() << endl);
        return UPNP_E_INVALID_PARAM;
    }
    return UPNP_E_INVALID_PARAM;
}

void UpnpDevice::addServiceType(const std::string& serviceId, 
                                const std::string& serviceType)
{
    //LOGDEB("UpnpDevice::addServiceType: [" << 
    //    serviceId << "] -> [" << serviceType << endl);
    m_serviceTypes[serviceId] = serviceType;
}

void UpnpDevice::addActionMapping(const std::string& actName, soapfun fun, char action)
{
    // LOGDEB("UpnpDevice::addActionMapping:" << actName << endl);
	m_calls[actName] = fun;
    m_checks[actName] = action;
}

void UpnpDevice::notifyEvent(const string& serviceId,
                             const vector<string>& names, 
                             const vector<string>& values)
{
    LOGDEB("UpnpDevice::notifyEvent " << serviceId << " " <<
           (names.empty()?"Empty names??":names[0]) << endl);
    if (names.empty())
        return;
    vector<const char *> cnames, cvalues;
    vector<string> qvalues;
    vectorstoargslists(names, values, qvalues, cnames, cvalues);

    int ret = UpnpNotify(m_lib->getdvh(), m_deviceId.c_str(), 
                         serviceId.c_str(), &cnames[0], &cvalues[0],
                         int(cnames.size()));
    if (ret != UPNP_E_SUCCESS) {
        LOGERR("UpnpDevice::notifyEvent: UpnpNotify failed: " << ret << endl);
    }
}

static pthread_cond_t evloopcond = PTHREAD_COND_INITIALIZER;

static void timespec_addnanos(struct timespec *ts, int nanos)
{
    ts->tv_nsec += nanos;
    if (ts->tv_nsec > 1000 * 1000 * 1000) {
        int secs = ts->tv_nsec / (1000 * 1000 * 1000);
        ts->tv_sec += secs;
        ts->tv_nsec -= secs * (1000 * 1000 * 1000);
    } 
}
int timespec_diffms(const struct timespec& old, const struct timespec& recent)
{
    return (recent.tv_sec - old.tv_sec) * 1000 + 
        (recent.tv_nsec - old.tv_nsec) / (1000 * 1000);
}

#ifndef CLOCK_REALTIME
// Mac OS X for one does not have clock_gettime. Gettimeofday is more than
// enough for our needs.
#define CLOCK_REALTIME 0
int clock_gettime(int /*clk_id*/, struct timespec* t) {
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#endif // ! CLOCK_REALTIME

// Loop on services, and poll each for changed data. Generate event
// only if changed data exists. Every 3 S we generate an artificial
// event with all the current state.
void UpnpDevice::eventloop()
{
    int count = 0;
    const int loopwait_ms = 1000; // Polling mpd every 1 S
    const int nloopstofull = 10;  // Full state every 10 S
    struct timespec wkuptime, earlytime;
    bool didearly = false;
    bool all = false;

    for (;;) {
        clock_gettime(CLOCK_REALTIME, &wkuptime);
        timespec_addnanos(&wkuptime, loopwait_ms * 1000 * 1000);

        //LOGDEB("eventloop: now " << time(0) << " wkup at "<< 
        //    wkuptime.tv_sec << " S " << wkuptime.tv_nsec << " ns" << endl);

        PTMutexLocker lock(cblock);
        int err = pthread_cond_timedwait(&evloopcond, lock.getMutex(), 
                                         &wkuptime);
        if (err && err != ETIMEDOUT) {
            LOGINF("UpnpDevice:eventloop: wait errno " << errno << endl);
            break;
        } else if (err == 0) {
            // Early wakeup. Only does something if it did not already
            // happen recently
            if (didearly) {
                int millis = timespec_diffms(earlytime, wkuptime);
                if (millis < loopwait_ms) {
                    // Do nothing. didearly stays true
                    // LOGDEB("eventloop: early, previous too close "<<endl);
                    continue;
                } else {
                    // had an early wakeup previously, but it was a
                    // long time ago. Update state and wakeup
                    // LOGDEB("eventloop: early, previous is old "<<endl);
                    earlytime = wkuptime;
                }
            } else {
                // Early wakeup, previous one was normal. Remember.
                // LOGDEB("eventloop: early, no previous" << endl);
                didearly = true;
                earlytime = wkuptime;
				all = true;
            }
        } else {
            // Normal wakeup
            // LOGDEB("eventloop: normal wakeup" << endl);
            didearly = false;
        }

        count++;
        if(!all)
			all = count && ((count % nloopstofull) == 0);
        //LOGDEB("UpnpDevice::eventloop count "<<count<<" all "<<all<<endl);

        for (unordered_map<string, string>::const_iterator it = 
                 m_serviceTypes.begin(); it != m_serviceTypes.end(); it++) {
            vector<string> names, values;
            if (!getEventData(all, it->first, names, values) || names.empty()) {
                continue;
            }
            notifyEvent(it->first, names, values);
        }
		all = false;
    }
}

void UpnpDevice::loopWakeup()
{
    pthread_cond_broadcast(&evloopcond);
}

void UpnpDevice::myHandler(int signum)
{
	if(signum == SIGUSR1) {
		if (m_lib_g->ok()) {
			UpnpUnRegisterRootDevice(m_lib_g->getdvh());
		}
	}
	return;
}
