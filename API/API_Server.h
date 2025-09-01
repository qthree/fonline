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

#endif // __API_SERVER__