#ifndef __SERVER_CONFIG__
#define __SERVER_CONFIG__

#ifndef CORRODED_CONFIG
#include "IniParser.h"
#endif

#define SERVER_CONFIG_LANGUAGE_LENGTH_WITH_NUL (5)
#define SERVER_CONFIG_LANGUAGE_MAX_COUNT       (8)

struct ServerConfig
{
    int  MemoryDebugLevel;
    bool LoggingTime;
    bool LoggingThread;
    bool LoggingDebugOutput;

    int  GameSleep;
    bool ScriptConcurrentExecution;
    int WorldSaveManager;

    uint ProfilerSampleInterval;
    uint ProfilerMode;
    int DetailedCallStackInfo;
    bool LogicThreadSetAffinity;
    uint LogicThreadCount;

    ushort Port;
    uint NetWorkThread;
    uint WorldSaveTime;

    uchar LanguageCount;
    char Languages[SERVER_CONFIG_LANGUAGE_LENGTH_WITH_NUL * SERVER_CONFIG_LANGUAGE_MAX_COUNT];

    #ifndef CORRODED_CONFIG
    ServerConfig(IniParser &cfg) {
        MemoryDebugLevel = cfg.GetInt( "MemoryDebugLevel", 0 );
        LoggingTime = cfg.GetInt( "LoggingTime", 1 ) != 0;
        LoggingThread = cfg.GetInt( "LoggingThread", 1 ) != 0;
        LoggingDebugOutput = cfg.GetInt( "LoggingDebugOutput", 0 ) != 0;

        GameSleep = cfg.GetInt( "GameSleep", 10 );
        ScriptConcurrentExecution = cfg.GetInt( "ScriptConcurrentExecution", 0 ) != 0;
        WorldSaveManager = cfg.GetInt( "WorldSaveManager", 1 );

        ProfilerSampleInterval = cfg.GetInt( "ProfilerSampleInterval", 0 );
        ProfilerMode = cfg.GetInt( "ProfilerMode", 0 );
        DetailedCallStackInfo = cfg.GetInt( "DetailedCallStackInfo", 0 );
        LogicThreadSetAffinity = cfg.GetInt( "LogicThreadSetAffinity", 0 ) != 0;
        LogicThreadCount = cfg.GetInt( "LogicThreadCount", 0 );

        Port = cfg.GetInt( "Port", 4000 );
        NetWorkThread = cfg.GetInt( "NetWorkThread", 0 );
        WorldSaveTime = cfg.GetInt( "WorldSaveTime", 60 );

        LanguageCount = 0;
        for( char cur_lang = 0; cur_lang < SERVER_CONFIG_LANGUAGE_MAX_COUNT; ++cur_lang ) {
            char cur_str_lang[ MAX_FOTEXT ];
            char lang_name[ MAX_FOTEXT ];
            Str::Format( cur_str_lang, "Language_%u", cur_lang );

            if( !cfg.GetStr( cur_str_lang, "", lang_name ) ) {
                break;
            }
            
            if( Str::Length( lang_name ) != 4 ) {
                WriteLog( "Language name not equal to four letters.\n" );
                continue;
            }

            memcpy(
                &Languages[SERVER_CONFIG_LANGUAGE_LENGTH_WITH_NUL*LanguageCount],
                lang_name,
                SERVER_CONFIG_LANGUAGE_LENGTH_WITH_NUL
            );
            LanguageCount += 1;
        }
    }
    const char* GetLanguage(uchar index) const {
        return &Languages[SERVER_CONFIG_LANGUAGE_LENGTH_WITH_NUL * index];
    }

    static ServerConfig LoadConfigFile();
    #endif // CORRODED_CONFIG
};

#endif // __SERVER_CONFIG__
