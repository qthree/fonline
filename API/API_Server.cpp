#include "StdAfx.h"
#include "Server.h"

#define __API_IMPL__
#include "API_Server.h"

uint Timer_GameTick() {
	return Timer::GameTick();
}
uint Timer_FastTick() {
	return Timer::FastTick();
}

ServerGameOptions* Server_GameOptions() {
    return &GameOpt;
}

ServerScriptFunctions* Server_ServerFunctions() {
    return &ServerFunctions;
}

#ifdef SERVER_LIB
#include "Version.h"
#include "ServerConfig.h"
#include <sys/stat.h>
FOServer Server;

#ifdef CORRODED_NET
extern "C" void start_corroded_net();
#endif
extern "C" void server_lib_log( char* str );

static volatile uchar exit_code = 0;

uchar Global_StartServerLib( ServerConfig cfg )
{
    // Stuff
    Timer::Init();
    Thread::SetCurrentName( "ServerLib" );
    LogToFunc(server_lib_log);

    // Logging
    LogWithTime( cfg.LoggingTime );
    LogWithThread( cfg.LoggingThread );
    if( cfg.LoggingDebugOutput ) {
        LogToDebugOutput();
    }

    // Log version
    WriteLog( "FOnline server library, version %04X-%02X.\n", SERVER_VERSION, FO_PROTOCOL_VERSION & 0xFF );

    if( Server.Init(cfg) )
    {
        FOQuit = false;
        #ifdef CORRODED_NET
        start_corroded_net();
        #endif
        if( !FOQuit )
        {
            Server.MainLoop();
        }
        Server.Finish();
    }
    else
    {
        WriteLog( "Initialization fail!\n" );
    }
    return exit_code;
}

void Global_StopServerLib( uchar code ) {
    exit_code = code;
    FOQuit = true;
}
#endif // SERVER_LIB
