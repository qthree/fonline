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

#ifdef SERVER_LIB
#include "Version.h" 
#include <sys/stat.h>
FOServer Server;
int Global_StartServerLib( int argc, const void* argv )
{
    // Stuff
    setlocale( LC_ALL, "Russian" );
    SetCommandLine( argc, argv );
    RestoreMainDirectory();
    CatchExceptions( "FOnlineServer", SERVER_VERSION );
    Timer::Init();
    Thread::SetCurrentName( "Daemon" );
    LogToFile( "./FOnlineServerDaemon.log" );

    // Config
    IniParser cfg;
    cfg.LoadFile( GetConfigFileName(), PT_SERVER_ROOT );

    // Logging
    LogWithTime( cfg.GetInt( "LoggingTime", 1 ) == 0 ? false : true );
    LogWithThread( cfg.GetInt( "LoggingThread", 1 ) == 0 ? false : true );
    if( strstr( CommandLine, "-logdebugoutput" ) || cfg.GetInt( "LoggingDebugOutput", 0 ) != 0 )
        LogToDebugOutput();

    // Log version
    WriteLog( "FOnline server daemon, version %04X-%02X.\n", SERVER_VERSION, FO_PROTOCOL_VERSION & 0xFF );
    if( CommandLineArgCount > 1 )
        WriteLog( "Command line<%s>.\n", CommandLine );

    umask( 0 );

    GetServerOptions();

    if( Server.Init() )
    {
        FOQuit = false;
        Server.MainLoop();
        Server.Finish();
    }
    else
    {
        WriteLog( "Initialization fail!\n" );
    }
    return 0;
}
#endif // SERVER_LIB
