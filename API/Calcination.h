#ifndef __CALCINATION__
#define __CALCINATION__

#include "API_Common.h"

EXPORT uchar Global_StartClientLib();
EXPORT uchar* Global_AllocBytes(size_t size);

//struct ServerGameOptions;

//EXPORT uint Timer_GameTick();
//EXPORT uint Timer_FastTick();
///EXPORT ServerGameOptions* Server_GameOptions();
//EXPORT ServerScriptFunctions* Server_ServerFunctions();

//EXPORT uchar Global_StartServerLib( const ServerConfig cfg );
//EXPORT void Global_StopServerLib( uchar code );
//EXPORT uchar Global_CompileClientScripts( const ServerConfig cfg );

#endif // __CALCINATION__