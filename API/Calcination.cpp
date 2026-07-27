//#include "Types.h"
//#include "Log.h"
//#include "Timer.h"
//#include "Version.h"
//#include "NetProtocol.h"
#include "Common.h"
#include "Client.h"
#include "Version.h"

#define __API_IMPL__
#include "Calcination.h"

extern "C" void client_lib_log( char* str );

void init_client_lib()
{
    Timer::Init();

    // Logging
    LogToFunc(client_lib_log);
    LogWithTime( false );
    LogWithThread( true );

    // Log version
    WriteLog( "FOnline client library, version %04X-%02X.\n", CLIENT_VERSION, FO_PROTOCOL_VERSION & 0xFF );
}

FOWindow* MainWindow = NULL;
FOClient* FOEngine = NULL;

uchar Global_StartClientLib( /* const ServerConfig cfg */ )
{
    init_client_lib();
    // Threading
    //#ifdef FO_WINDOWS
    //pthread_win32_process_attach_np();
    //#elif FO_LINUX
    //XInitThreads();
    //#endif

    // Options
    GetClientOptions();

    // Create window
    MainWindow = new FOWindow();
    //MainWindow->label( GetWindowName() );
    //MainWindow->position( ( Fl::w() - MODE_WIDTH ) / 2, ( Fl::h() - MODE_HEIGHT ) / 2 );
    //MainWindow->size( MODE_WIDTH, MODE_HEIGHT );


    // OpenGL parameters
    //#ifndef FO_D3D
    //Fl::gl_visual( FL_RGB | FL_RGB8 | FL_DOUBLE | FL_DEPTH | FL_STENCIL );
    //#endif

    FOEngine = new FOClient();
    if( !FOEngine || !FOEngine->Init() )
    {
        WriteLog( "FOnline engine initialization fail.\n" );
        GameOpt.Quit = true;
        return 1;
    }

    // Loop
    while( !GameOpt.Quit )
    {
        if( !FOEngine->MainLoop() )
            Sleep( 100 );
    }

    // Finish
    FOEngine->Finish();
    delete FOEngine;
    //delete MainWindow;

    WriteLog( "FOnline finished.\n" );
    return 0;
}

uchar* Global_AllocBytes(size_t size) 
{
    return new uchar[ size ];
}
