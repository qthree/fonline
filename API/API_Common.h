#ifdef FO_WINDOWS
# ifdef FO_MSVC
#  define EXPORT                    extern "C" __declspec( dllexport )
#  define EXPORT_UNINITIALIZED      extern "C" __declspec( dllexport ) extern
# else // FO_GCC
#  define EXPORT                    extern "C" __attribute__( ( dllexport ) )
#  define EXPORT_UNINITIALIZED      extern "C" __attribute__( ( dllexport ) ) extern
# endif
#else
# define EXPORT                     extern "C" __attribute__( ( visibility( "default" ) ) )
# define EXPORT_UNINITIALIZED       extern "C" __attribute__( ( visibility( "default" ) ) )
#endif

#ifndef __API_IMPL__
typedef unsigned int     uint;
typedef unsigned char    uchar;
typedef unsigned short   ushort;
#ifdef FO_X86
    typedef unsigned int        size_t;
#elif defined(FO_X64)
    typedef __SIZE_TYPE__       size_t;
#endif

#endif //__API_IMPL__
