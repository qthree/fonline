#include "StdAfx.h"
#include "Server.h"
#include "AngelScript/Preprocessor/preprocess.h"
#include "ScriptPragmas.h"

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

void init_server_lib(const ServerConfig &cfg)
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
}

uchar Global_StartServerLib( const ServerConfig cfg )
{
    init_server_lib(cfg);

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

#define BIND_CLIENT
#define BIND_CLASS    BindClass::
#define BIND_ASSERT( x )    if( ( x ) < 0 ) { WriteLogF( _FUNC_, " - Bind error, line<%d>.\n", __LINE__ ); bind_errors++; }
namespace ClientScriptsCompiler
{
    #include "DummyData.h"

    static int Bind( asIScriptEngine* engine )
    {
        int bind_errors = 0;
        #include "ScriptBind.h"
        return bind_errors;
    }
}
#undef BIND_CLIENT
#undef BIND_CLASS
#undef BIND_ASSERT

bool compile_client_scripts(const ServerConfig &cfg )
{
    WriteLog( "Compile client scripts...\n" );

    if( !FOServer::InitLangPacks( FOServer::LangPacks, cfg ) )
    {
        WriteLog( "Can't init lang packs.\n" );
        return false;
    }

    // Get config file
    FileManager scripts_cfg;
    scripts_cfg.LoadFile( SCRIPTS_LST, PT_SERVER_SCRIPTS );
    if( !scripts_cfg.IsLoaded() )
    {
        WriteLog( "Config file<%s> not found.\n", SCRIPTS_LST );
        return false;
    }

    asIScriptEngine* engine = Script::CreateEngine( new ScriptPragmaCallback( PRAGMA_CLIENT ), "CLIENT" );
    if( engine )
        Script::SetEngine( engine );

    // Bind vars and functions
    int bind_errors = 0;
    if( engine )
        bind_errors = ClientScriptsCompiler::Bind( engine );

    // Check errors
    if( !engine || bind_errors )
    {
        if( !engine )
            WriteLogF( _FUNC_, " - asCreateScriptEngine fail.\n" );
        else
            WriteLog( "Bind fail, errors<%d>.\n", bind_errors );
        Script::FinishEngine( engine );

        return false;
    }

    // Load script modules
    Script::Define( "__CLIENT" );

    Script::SetLoadLibraryCompiler( true );

    int    num = STR_INTERNAL_SCRIPT_MODULES;
    int    errors = 0;
    char   buf[ MAX_FOTEXT ];
    string value, config;
    StrVec pragmas;
    while( scripts_cfg.GetLine( buf, MAX_FOTEXT ) )
    {
        if( buf[ 0 ] != '@' )
            continue;
        istrstream str( &buf[ 1 ] );
        str >> value;
        if( str.fail() || value != "client" )
            continue;
        str >> value;
        if( str.fail() || ( value != "module" && value != "bind" ) )
            continue;

        if( value == "module" )
        {
            str >> value;
            if( str.fail() )
                continue;

            if( !Script::LoadScript( value.c_str(), NULL, false, "CLIENT_" ) )
            {
                WriteLogF( _FUNC_, " - Unable to load client script<%s>.\n", value.c_str() );
                errors++;
                continue;
            }

            asIScriptModule* module = engine->GetModule( value.c_str(), asGM_ONLY_IF_EXISTS );
            CBytecodeStream  binary;
            if( !module || module->SaveByteCode( &binary ) < 0 )
            {
                WriteLogF( _FUNC_, " - Unable to save bytecode of client script<%s>.\n", value.c_str() );
                errors++;
                continue;
            }
            std::vector< asBYTE >& buf = binary.GetBuf();

            // Pragmas
            const StrVec& pr = Preprocessor::GetParsedPragmas();
            for( size_t i = 0, j = pr.size(); i < j; i += 2 )
            {
                bool found = false;
                for( size_t k = 0, l = pragmas.size(); k < l; k += 2 )
                {
                    if( pragmas[ k ] == pr[ i ] && pragmas[ k + 1 ] == pr[ i + 1 ] )
                    {
                        found = true;
                        break;
                    }
                }
                if( !found )
                {
                    pragmas.push_back( pr[ i ] );
                    pragmas.push_back( pr[ i + 1 ] );
                }
            }

            // Add module name and bytecode
            for( auto it = FOServer::LangPacks.begin(), end = FOServer::LangPacks.end(); it != end; ++it )
            {
                LanguagePack& lang = *it;
                FOMsg&        msg_script = lang.Msg[ TEXTMSG_INTERNAL ];

                for( int i = 0; i < 10; i++ )
                    msg_script.EraseStr( num + i );
                msg_script.AddStr( num, value.c_str() );
                msg_script.AddBinary( num + 1, (uchar*) &buf[ 0 ], (uint) buf.size() );
            }
            num += 2;
        }
        else
        {
            // Make bind line
            string config_ = "@ client bind ";
            str >> value;
            if( str.fail() )
                continue;
            config_ += value + " ";
            str >> value;
            if( str.fail() )
                continue;
            config_ += value;
            config += config_ + "\n";
        }
    }

    // Imported functions
    Script::BindImportedFunctions();

    // Add native dlls to MSG
    int         dll_num = STR_INTERNAL_SCRIPT_DLLS;
    EngineData* ed = (EngineData*) engine->GetUserData();
    for( auto it = ed->LoadedDlls.begin(), end = ed->LoadedDlls.end(); it != end; ++it )
    {
        const string& dll_name = ( *it ).first;
        const string& dll_path = ( *it ).second.first;

        // Load libraries for all platforms
        // Windows, Linux
        for( int d = 0; d < 2; d++ )
        {
            // Make file name
            const char* extensions[] = { ".dll", ".so" };
            char        fname[ MAX_FOPATH ];
            Str::Copy( fname, dll_path.c_str() );
            FileManager::EraseExtension( fname );
            Str::Append( fname, extensions[ d ] );

            // Erase first './'
            if( Str::CompareCount( fname, DIR_SLASH_SD, Str::Length( DIR_SLASH_SD ) ) )
                Str::EraseInterval( fname, Str::Length( DIR_SLASH_SD ) );

            // Load dll
            FileManager dll;
            if( !dll.LoadFile( fname, -1 ) )
            {
                if( !d )
                {
                    WriteLogF( _FUNC_, " - Can't load dll<%s>.\n", dll_name.c_str() );
                    errors++;
                }
                continue;
            }

            // Add dll name and binary
            for( auto it = FOServer::LangPacks.begin(), end = FOServer::LangPacks.end(); it != end; ++it )
            {
                LanguagePack& lang = *it;
                FOMsg&        msg_script = lang.Msg[ TEXTMSG_INTERNAL ];

                for( int i = 0; i < 10; i++ )
                    msg_script.EraseStr( dll_num + i );
                msg_script.AddStr( dll_num, fname );
                msg_script.AddBinary( dll_num + 1, dll.GetBuf(), dll.GetFsize() );
            }
            dll_num += 2;
        }
    }

    // Finish
    Script::FinishEngine( engine );
    Script::Undefine( "__CLIENT" );

    // Add config text and pragmas, calculate hash
    for( auto it = FOServer::LangPacks.begin(), end = FOServer::LangPacks.end(); it != end; ++it )
    {
        LanguagePack& lang = *it;
        FOMsg&        msg_script = lang.Msg[ TEXTMSG_INTERNAL ];

        msg_script.EraseStr( STR_INTERNAL_SCRIPT_CONFIG );
        msg_script.AddStr( STR_INTERNAL_SCRIPT_CONFIG, config.c_str() );
        msg_script.EraseStr( STR_INTERNAL_SCRIPT_VERSION );
        msg_script.AddStr( STR_INTERNAL_SCRIPT_VERSION, Str::FormatBuf( "%d", CLIENT_SCRIPT_BINARY_VERSION ) );

        for( uint i = 0, j = (uint) pragmas.size(); i < j; i++ )
        {
            msg_script.EraseStr( STR_INTERNAL_SCRIPT_PRAGMAS + i );
            msg_script.AddStr( STR_INTERNAL_SCRIPT_PRAGMAS + i, pragmas[ i ].c_str() );
        }
        for( uint i = 0, j = 10; i < j; i++ )
            msg_script.EraseStr( STR_INTERNAL_SCRIPT_PRAGMAS + (uint) pragmas.size() + i );

        msg_script.CalculateHash();

        msg_script.SaveMsgFile(Str::FormatBuf("%s\\%s", lang.NameStr, TextMsgFileName[ TEXTMSG_INTERNAL ]), PT_TEXTS);
    }

    WriteLog( "Client scripts compiled\n" );
    return true;
    
}

uchar Global_CompileClientScripts(const ServerConfig cfg)
{
    init_server_lib(cfg);
    
    if (!compile_client_scripts(cfg)) {
        exit_code = 1;
    }

    return exit_code;
}

void Global_StopServerLib( uchar code ) {
    exit_code = code;
    FOQuit = true;
}
#endif // SERVER_LIB
