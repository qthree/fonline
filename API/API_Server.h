#ifndef __API_SERVER__
#define __API_SERVER__

#include "API_Common.h"

#ifdef __API_IMPL__

#else
struct ServerGameOptions;
#endif //__API_IMPL__

EXPORT uint Timer_GameTick();
EXPORT uint Timer_FastTick();
EXPORT ServerGameOptions* Server_GameOptions();
EXPORT ServerScriptFunctions* Server_ServerFunctions();

#ifdef SERVER_LIB
EXPORT int Global_StartServerLib( int argc, const void* argv );
#endif

#endif // __API_SERVER__