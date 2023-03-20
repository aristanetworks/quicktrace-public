// Copyright (c) 2018 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#ifndef QUICKTRACE_QUICKTRACEFORMATSTRING_H
#define QUICKTRACE_QUICKTRACEFORMATSTRING_H

#include <stdint.h>

namespace QuickTrace {

inline char const * formatString( char ) noexcept { return "c"; }
inline char const * formatString( uint8_t ) noexcept { return "u"; }
inline char const * formatString( int8_t ) noexcept { return "u"; }
inline char const * formatString( uint16_t ) noexcept { return "s"; }
inline char const * formatString( int16_t ) noexcept { return "s"; }
inline char const * formatString( uint32_t ) noexcept { return "i"; }
inline char const * formatString( int32_t ) noexcept { return "i"; }
inline char const * formatString( uint64_t ) noexcept { return "q"; }
inline char const * formatString( int64_t ) noexcept { return "q"; }
#ifdef __LP64__
inline char const * formatString( long long int ) noexcept { return "q"; }
inline char const * formatString( long long unsigned int ) noexcept { return "q"; }
#endif
#ifndef __LP64__
inline char const * formatString( long ) noexcept { return "i"; }
inline char const * formatString( unsigned long x ) noexcept { return "i"; }
#endif
inline char const * formatString( float ) noexcept { return "f"; }
inline char const * formatString( double ) noexcept { return "d"; }
inline char const * formatString( char const * ) noexcept { return "p"; }
inline char const * formatString( bool ) noexcept { return "b"; }
inline char const * formatString( void * ) noexcept {
   return sizeof( void * ) == sizeof( uint64_t ) ? "q" : "i";
}

} // namespace QuickTrace

#endif // QUICKTRACE_QUICKTRACEFORMATSTRING_H
