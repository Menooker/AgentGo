#include "Config.h"
#ifndef _H_DBGPIPE_
#define _H_DBGPIPE_

void InitDbgConsole();
int DpPrintf(char* str,...);
#ifdef AG_DBG
#define dinitdbg InitDbgConsole
#define dprintf DpPrintf
#else
#define dinitdbg
#define dprintf 
#endif
#endif
