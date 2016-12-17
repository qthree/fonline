# CMake initial cache

if( NOT DEFINED "ENV{FO_SOURCE}" )
	message( FATAL_ERROR "Define FO_SOURCE" )
endif()
if( NOT DEFINED "ENV{EMSCRIPTEN}" )
	message( FATAL_ERROR "Define EMSCRIPTEN" )
endif()

set( CMAKE_TOOLCHAIN_FILE "$ENV{EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake" CACHE PATH "" FORCE )
set( EMSCRIPTEN $ENV{EMSCRIPTEN} CACHE PATH "" FORCE )
