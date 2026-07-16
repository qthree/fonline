#ifndef ___EXCEPTION___
#define ___EXCEPTION___

// Uncomment for use minidumps (dmp) instead readable dumps (txt)
// #define EXCEPTION_MINIDUMP

#ifndef DISABLE_EXCEPTION_HANDLING
#pragma comment(lib, "BugTrap.lib")
#endif// DISABLE_EXCEPTION_HANDLING

void SetupExceptionHandler( const string& app_name, int app_ver );
void CreateDump( const string& appendix, const string& message );

#endif // ___EXCEPTION___
