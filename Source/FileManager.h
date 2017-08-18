#ifndef __FILE_MANAGER__
#define __FILE_MANAGER__

#include "Defines.h"
#include "Log.h"
#include "DataFile.h"

#ifdef FO_MSVC
# undef CopyFile
# undef DeleteFile
#endif

struct FindData
{
    string FileName;
    uint   FileSize;
    uint64 WriteTime;
    bool   IsDirectory;
};
typedef vector< FindData > FindDataVec;

class FileManager
{
public:
    static void InitDataFiles( const string& path, bool set_write_dir = true );
    static bool LoadDataFile( const string& path, bool skip_inner = false );
    static void ClearDataFiles();

    FileManager( const string& path, bool no_read = false );
    FileManager( const uchar* stream, uint length );

    bool   LoadFile( const string& path, bool no_read = false );
    bool   LoadStream( const uchar* stream, uint length );
    void   UnloadFile();
    uchar* ReleaseBuffer();

    void SetCurPos( uint pos );
    void GoForward( uint offs );
    void GoBack( uint offs );
    bool FindFragment( const uchar* fragment, uint fragment_len, uint begin_offs );

    string GetNonEmptyLine();
    bool   CopyMem( void* ptr, uint size );
    string GetStrNT(); // Null terminated
    uchar  GetUChar();
    ushort GetBEUShort();
    short  GetBEShort() { return (short) GetBEUShort(); }
    ushort GetLEUShort();
    short  GetLEShort() { return (short) GetLEUShort(); }
    uint   GetBEUInt();
    uint   GetLEUInt();
    uint   GetLE3UChar();
    float  GetBEFloat();
    float  GetLEFloat();

    void   SwitchToRead();
    void   SwitchToWrite();
    bool   ResizeOutBuf();
    void   SetPosOutBuf( uint pos );
    uchar* GetOutBuf()    { return dataOutBuf; }
    uint   GetOutBufLen() { return endOutBuf; }
    bool   SaveFile( const string& fname );

    void SetData( void* data, uint len );
    void SetStr( const char* fmt, ... );
    void SetStrNT( const char* fmt, ... );
    void SetUChar( uchar data );
    void SetBEUShort( ushort data );
    void SetBEShort( short data ) { SetBEUShort( (ushort) data ); }
    void SetLEUShort( ushort data );
    void SetBEUInt( uint data );
    void SetLEUInt( uint data );

    static void   ResetCurrentDir();
    static void   SetCurrentDir( const string& dir, const string& write_dir );
    static string GetWritePath( const string& fname );
    static void   FormatPath( string& path );
    static string ExtractDir( const string& path );
    static string ExtractFileName( const string& path );
    static string CombinePath( const string& base_path, const string& path );
    static string GetExtension( const string& path ); // Extension without dot
    static void   EraseExtension( string& path );     // Erase extension with dot
    static string ForwardPath( const string& path, const string& relative_dir );
    static bool   IsFileExists( const string& fname );
    static bool   CopyFile( const string& from, const string& to );
    static bool   RenameFile( const string& from, const string& to );
    static bool   DeleteFile( const string& fname );
    static void   DeleteDir( const string& dir );
    static void   CreateDirectoryTree( const string& path );
    static void   ResolvePath( string& path );
    static void   NormalizePathSlashes( string& path );

    bool        IsLoaded()     { return fileSize != 0; }
    uchar*      GetBuf()       { return fileBuf; }
    const char* GetCStr()      { return (const char*) fileBuf; }
    uchar*      GetCurBuf()    { return fileBuf + curPos; }
    uint        GetCurPos()    { return curPos; }
    uint        GetFsize()     { return fileSize; }
    bool        IsEOF()        { return curPos >= fileSize; }
    uint64      GetWriteTime() { return writeTime; }

    static DataFileVec& GetDataFiles() { return dataFiles; }
    static void         GetFolderFileNames( const string& path, bool include_subdirs, const string& ext, StrVec& files_path, FindDataVec* files = nullptr, StrVec* dirs_path = nullptr, FindDataVec* dirs = nullptr );
    static void         GetDataFileNames( const string& path, bool include_subdirs, const string& ext, StrVec& result );

    FileManager();
    ~FileManager();

private:
    static DataFileVec dataFiles;
    static string      writeDir;

    uint               fileSize;
    uchar*             fileBuf;
    uint               curPos;

    uchar*             dataOutBuf;
    uint               posOutBuf;
    uint               endOutBuf;
    uint               lenOutBuf;

    uint64             writeTime;

    static void RecursiveDirLook( const string& base_dir, const string& cur_dir, bool include_subdirs, const string& ext, StrVec& files_path, FindDataVec* files, StrVec* dirs_path, FindDataVec* dirs );
};

class FilesCollection
{
public:
    FilesCollection( const string& ext, const string& fixed_dir = "" );
    bool         IsNextFile();
    FileManager& GetNextFile( string* name = nullptr, string* path = nullptr, string* relative_path = nullptr, bool no_read_data = false );
    FileManager& FindFile( const string& name, string* path = nullptr, string* relative_path = nullptr, bool no_read_data = false );
    uint         GetFilesCount();
    void         ResetCounter();

private:
    StrVec      fileNames;
    StrVec      filePaths;
    StrVec      fileRelativePaths;
    uint        curFileIndex;
    FileManager curFile;
};

#endif // __FILE_MANAGER__
