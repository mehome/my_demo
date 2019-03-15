#ifndef __WAKEUP_DEBUG_H__
#define __WAKEUP_DEBUG_H__

#include <libcchip/platform.h>
#define WAKEUP_DEBUG  
   

#ifdef WAKEUP_DEBUG  
#define WAKEUP_RAW(x...)		raw(x)
#define WAKEUP_ERR(x...)		err(x)
#define WAKEUP_WAR(x...)		war(x)
#define WAKEUP_INFO(x...)		inf(x)
#define WAKEUP_DBG(x...)		dbg(x)
#define WAKEUP_TRACE(x...)		trc(x)
#define WAKEUP_PERR(x...)		show_errno(0,x)
#else
#define WAKEUP_RAW(x...)		
#define WAKEUP_ERR(x...)
#define WAKEUP_WAR(x...)
#define WAKEUP_INFO(x...) 
#define WAKEUP_DBG(x...)
#define WAKEUP_TRACE(x...)
#define WAKEUP_PERR(x...)
#endif

			

#endif