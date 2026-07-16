#include "StdAfx.h"
#include "Common.h"
#include "Exception.h"

#ifdef DISABLE_EXCEPTION_HANDLING

void SetupExceptionHandler( const string& app_name, int app_ver ) {}
void CreateDump( const string& appendix, const string& message ) {}
void BT_SetTerminate() {}

#else // DISABLE_EXCEPTION_HANDLING
#include "BugTrap/BugTrap.h"
#include "Script.h"
#include "Version.h"

#ifdef FONLINE_SERVER
#include "Jobs.h"
#endif

static string AppName;
static string AppVer;
static string ManualDumpAppendix;
static string ManualDumpMessage;

#if defined ( FO_WINDOWS )

# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
# include <stdio.h>
# pragma warning( disable : 4091 )
# pragma warning( disable : 4996 )
# include <DbgHelp.h>
# include "Timer.h"
# include "FileManager.h"

static LONG TopLevelFilterReadableDump();

static void DumpAngelScript( FILE* f );

void CALLBACK PreErrorHandler	(INT_PTR)
{
	char        dump_path[ MAX_FOPATH ];
	char        dump_path_dir[ MAX_FOPATH ];

	DateTime    dt;
	Timer::GetCurrentDateTime( dt );
	FileManager::GetFullPath( NULL, PT_SERVER_DUMPS, dump_path_dir );
	Str::Format( dump_path, "%s%s_%s_%s_%04d.%02d.%02d_%02d-%02d-%02d.txt",
				 dump_path_dir, "BugTrap", AppName.c_str(), AppVer.c_str( ), dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second );
	BT_AddLogFile(dump_path);
}

void SetupExceptionHandler( const string& app_name, int app_ver )
{
	BT_InstallSehFilter();
	if ( !Str::Substring(CommandLine, "-SilentErrorMode") )
		BT_SetActivityType(BTA_SHOWUI);
	else
		BT_SetActivityType(BTA_SAVEREPORT);

	BT_SetDialogMessage(
	BTDM_INTRO2,
	"\
This is FOnline Engine crash reporting client. \
To help the development process, \
please Submit Bug or save report and email it manually (button More...).\
\r\nMany thanks in advance and sorry for the inconvenience."
	);
	BT_SetPreErrHandler(PreErrorHandler,0);

	BT_SetAppName(app_name.c_str());
	BT_SetReportFormat(BTRF_TEXT);
	BT_SetSupportEMail("support@fonline.ru");
	BT_SetFlags(BTF_DETAILEDMODE | BTF_ATTACHREPORT);
	BT_SetDumpType(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithFullMemory | MiniDumpIgnoreInaccessibleMemory);

	AppName = app_name;
	AppVer = Str::FormatBuf( "%i", app_ver );
}

# ifdef FO_X86
#  pragma warning( disable : 4748 )
# endif
static LONG TopLevelFilterReadableDump()
{
	string mess;
	char        dump_path[ MAX_FOPATH ];
	char        dump_path_dir[ MAX_FOPATH ];

	DateTime    dt;
	Timer::GetCurrentDateTime( dt );
	const char* dump_str = ManualDumpAppendix.c_str();
	FileManager::GetFullPath( NULL, PT_SERVER_DUMPS, dump_path_dir );
	Str::Format( dump_path, "%s%s_%s_%s_%04d.%02d.%02d_%02d-%02d-%02d.txt",
				 dump_path_dir, dump_str, AppName.c_str(), AppVer.c_str( ), dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second );

	FILE* f = fopen( dump_path, "wt" );
	if( f )
	{
		// Generic info
		fprintf( f, "Message\n" );
		fprintf( f, "%s\n", ManualDumpMessage.c_str( ) );
		fprintf( f, "\n" );
		fprintf( f, "Application\n" );
		fprintf( f, "\tName        %s\n", AppName.c_str( ) );
		fprintf( f, "\tVersion     %s\n", AppVer.c_str( ) );
		OSVERSIONINFO ver;
		memset( &ver, 0, sizeof( OSVERSIONINFO ) );
		ver.dwOSVersionInfoSize = sizeof( ver );
		if( GetVersionEx( ( OSVERSIONINFO* )&ver ) )
		{
			fprintf( f, "\tOS          %d.%d.%d (%s)\n",
					 ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber, ver.szCSDVersion );
		}
		fprintf( f, "\tTimestamp   %04d.%02d.%02d %02d:%02d:%02d\n", dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, dt.Second );
		fprintf( f, "\n" );

#if defined(FONLINE_SERVER) && !defined(DISABLE_CALLSTACK)
		fprintf( f, "\nCallStack:\n%s\n", Script::FormatCallstackInfo( true ).c_str() );
#endif

		// AngelScript dump
		DumpAngelScript( f );

		fclose( f );
	}
	else
	{
		WriteLog( "Error create dump: <%s>\n", dump_path );
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void CreateDump( const string& appendix, const string& message )
{	
	ManualDumpAppendix = appendix;
	ManualDumpMessage = message;

	TopLevelFilterReadableDump();
}

#else
# pragma MESSAGE( "Exception handling is disabled" )

void SetupExceptionHandler( const string& app_name, int app_ver )
{
	//
}

void CreateDump( const string& appendix, const string& message )
{
	//
}

#endif

static void DumpAngelScript( FILE* f )
{
	string tb = Script::GetTraceback( );
	if( !tb.empty( ) )
		fprintf( f, "AngelScript\n%s", tb.c_str() );
}

#endif // DISABLE_EXCEPTION_HANDLING