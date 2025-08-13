#ifndef CORRODED_NET
#ifndef __BUFFER_MANAGER__
#define __BUFFER_MANAGER__

#include "Common.h"

#define CRYPT_KEYS_COUNT    ( 50 )

class BufferManager
{
private:
    bool isError;
    Mutex bufLocker;
    char* bufData;
    uint bufLen;
    uint bufEndPos;
    uint bufReadPos;
    bool encryptActive;
    int encryptKeyPos;
    uint encryptKeys[ CRYPT_KEYS_COUNT ];

    void CopyBuf( const char* from, char* to, const char* mask, uint crypt_key, uint len );
    bool IsValidMsg( uint msg );

public:
    BufferManager();
    BufferManager( uint alen );
    BufferManager& operator=( const BufferManager& r );
    ~BufferManager();

    void SetEncryptKey( uint seed );
    void Lock();
    void Unlock();
    void Refresh();
    void Reset();
    void LockReset();
    void Push( const char* buf, uint len, bool no_crypt = false );
    void Push( const char* buf, const char* mask, uint len );
    void Pop( char* buf, uint len );
    void Cut( uint len );
    void GrowBuf( uint len );

    char* GetData()             { return bufData; }
    char* GetCurData()          { return bufData + bufReadPos; }
    uint  GetLen()              { return bufLen; }
    uint  GetCurPos()           { return bufReadPos; }
    void  SetEndPos( uint pos ) { bufEndPos = pos; }
    uint  GetEndPos() const     { return bufEndPos; }
    void  MoveReadPos( int val )
    {
        bufReadPos += val;
        EncryptKey( val );
    }
    bool IsError() const               { return isError; }
    bool IsEmpty() const               { return bufReadPos >= bufEndPos; }
    bool IsHaveSize( uint size ) const { return bufReadPos + size <= bufEndPos; }

    #if ( defined ( FONLINE_SERVER ) ) || ( defined ( FONLINE_CLIENT ) )
    bool NeedProcess();
    void SkipMsg( uint msg );
    void SeekValidMsg();
    #else
    bool NeedProcess() { return ( bufReadPos < bufEndPos ); }
    #endif

    BufferManager& operator<<( uint i );
    BufferManager& operator>>( uint& i );
    BufferManager& operator<<( int i );
    BufferManager& operator>>( int& i );
    BufferManager& operator<<( ushort i );
    BufferManager& operator>>( ushort& i );
    BufferManager& operator<<( short i );
    BufferManager& operator>>( short& i );
    BufferManager& operator<<( uchar i );
    BufferManager& operator>>( uchar& i );
    BufferManager& operator<<( char i );
    BufferManager& operator>>( char& i );
    BufferManager& operator<<( bool i );
    BufferManager& operator>>( bool& i );
#ifndef DISABLE_AVATARS
	BufferManager& operator>>( string& i);
	BufferManager& operator<<(string& i);
#endif

private:
    inline uint EncryptKey( int move )
    {
        if( !encryptActive ) return 0;
        uint key = encryptKeys[ encryptKeyPos ];
        encryptKeyPos += move;
        if( encryptKeyPos < 0 || encryptKeyPos >= CRYPT_KEYS_COUNT )
        {
            encryptKeyPos %= CRYPT_KEYS_COUNT;
            if( encryptKeyPos < 0 ) encryptKeyPos += CRYPT_KEYS_COUNT;
        }
        return key;
    }
};

#if !defined(FONLINE_MAPPER) && !defined(DISABLE_AVATARS)
struct FileSendBuffer
{
private:
	size_t realsize;
    int refcounter;

public:
    char* buffer;

    size_t filesize; 
	int type;
    std::string MD5;
	std::string Extension;

    struct State 
    {
        unsigned short int packetsize;
		size_t bytework;

		int params[ 3 ];
		std::string path;

		void Drop( )
		{
			packetsize = 0;
			bytework = 0;
			params[ 0 ] = 0;
			params[ 1 ] = 0;
			params[ 2 ] = 0;
			path = "";
		}

        void Finish( )
        {
            packetsize = 0;
        }

		State( )
		{
			Drop( );
		}
    };

private:
#ifdef FONLINE_CLIENT
    State _state;
#endif
#ifdef FONLINE_SERVER
    std::map<uint, State*> _state;
#endif
public:

    FileSendBuffer( ): filesize( 0 ), realsize( 0 ), buffer( nullptr ), type(0)
    {
        MD5 = "";
		Extension = "";
        refcounter = 1;
    }
#ifdef FONLINE_CLIENT
    State* GetState( uint clientid )
    {
        return &_state;
    }
#endif
#ifdef FONLINE_SERVER
    State* GetState( uint clientid )
    {
		auto stateit = _state.find(clientid);
		if (stateit == _state.end())
		{
			return nullptr;
		}
        return stateit->second;
    }
#endif

    int PushToBout( BufferManager& bout, uint clientid )
    {
		int result = 0;
        State* state = GetState( clientid );
        if( state )
        { 
            uint s = state->packetsize;
			if( ( s + state->bytework ) > filesize )
			{
				s = filesize - state->bytework;
				result = -1;
			}
			else result = 1;
			bout.Push( &buffer[ state->bytework ], s );
			state->bytework += s;
        }
		return result;
    }

	int PopToBin( BufferManager& bin, uint clientid )
	{ 
		int result = 0;
		State* state = GetState( clientid );
		if( state )
		{
			uint s = state->packetsize;
			if( ( s + state->bytework ) > filesize )
			{
				s = filesize - state->bytework;
				result = -1;
			}
			else result = 1;
			bin.Pop( &buffer[ state->bytework ], s );
			state->bytework += s;
		}
		return result;
	}

    void Resize( size_t newsize )
    {
        if( newsize != filesize )
        {
            if( newsize > realsize )
            {
                if( realsize != 0 )
                {
                    delete[] buffer;
                }
                realsize = newsize + 1;
                buffer = new char[ realsize ];
				memzero( buffer, realsize );
            }

            filesize = newsize;
        }
    }

    void AddRef( )
    {
        refcounter++;
    }

	void Drop( )
	{
		memzero( buffer, realsize );
		filesize = 0;
#ifdef FONLINE_CLIENT
		_state.Drop( );
#endif
		type = 0;
		MD5 = "";
		Extension = "";
	}

    void Release( )
    {
		if( --refcounter == 0 )
		{
			Drop( );
			if( buffer )
			{
				delete[] buffer;
				buffer = nullptr;
			}
			realsize = 0;
			buffer = nullptr;
			delete this;
		}
    }

	inline int GetRefCounter( )
	{
		return refcounter;
	}

	static map< uint, FileSendBuffer*> lib;
	static map< uint, FileSendBuffer*> DownloadLib;
	static map< uint, FileSendBuffer*> UploadLib;

	static FileSendBuffer* FileSendBuffer::GetDownloadBuffer( string md5 )
	{
		auto r = DownloadLib.find( Str::GetHash( md5.c_str( ) ) );
		if( r != DownloadLib.end( ) )
			return r->second;
		return nullptr;
	}

	static FileSendBuffer* FileSendBuffer::GetFileBuffer( string md5 )
	{
		auto r = lib.find( Str::GetHash( md5.c_str( ) ) );
		if( r != lib.end( ) )
			return r->second;
		return nullptr;
	}

	static FileSendBuffer* FileSendBuffer::GetDownloadFileBuffer( uint clientid )
	{
		auto r = DownloadLib.find( clientid );
		if( r != DownloadLib.end( ) )
			return r->second;
		return nullptr;
	}

	static FileSendBuffer* FileSendBuffer::GetUploadFileBuffer(uint clientid)
	{
		auto r = UploadLib.find(clientid);
		if (r != UploadLib.end())
			return r->second;
		return nullptr;
	}

	void ReserveForUpload(uint clientid)
	{
		AddRef();
		FreeUpload(clientid);
		UploadLib.insert(PAIR(clientid, this));
	}

	void FreeUpload(uint clientid)
	{
		auto r = UploadLib.find(clientid);
		if (r != UploadLib.end())
		{
			UploadLib.erase(r);
			Release();
		}
	}

	static FileSendBuffer* FileSendBuffer::CreateDownloadFileBuffer( string md5, uint clientid )
	{
		if( GetFileBuffer( md5 ) )
			return nullptr;

		auto result = new FileSendBuffer( );
		result->MD5 = md5;
		DownloadLib.insert( PAIR( clientid, result ) );
		return result;
	}
	
	bool FinishDownload( uint clientid, bool isSuccess = true )
	{
		auto it = DownloadLib.find( clientid );
		if( it == DownloadLib.end( ) )
			return false;

		DownloadLib.erase( it );

#ifdef FONLINE_CLIENT
		_state.Drop();
#endif
#ifdef FONLINE_SERVER
		auto stateit = _state.find( clientid );
		if( stateit != _state.end( ) )
		{
			stateit->second->Drop( );
			delete stateit->second;
			_state.erase( stateit );
		}
#endif
		if( isSuccess )
		{
			if( GetFileBuffer( MD5 ) )
				return false;

			lib.insert( PAIR( Str::GetHash( MD5.c_str( ) ), this ) );
			AddRef( );
		}
		return true;
	}

	State* CreateState( uint clientid )
	{
#ifdef FONLINE_CLIENT
		return &_state;
#endif
#ifdef FONLINE_SERVER
		auto state = new State;
		_state.insert(PAIR(clientid, state));
		return state;
#endif
	}

};

#endif // ! FONLINE_MAPPER && ! DISABLE_AVATARS

#endif // __BUFFER_MANAGER__
#endif // CORRODED_NET