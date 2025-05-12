// Copyright (c) 2018, Arista Networks, Inc.
// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:

// 	* Redistributions of source code must retain the above copyright notice,
//  	  this list of conditions and the following disclaimer.
// 	* Redistributions in binary form must reproduce the above copyright notice,
// 	  this list of conditions and the following disclaimer in the documentation
// 	  and/or other materials provided with the distribution.
// 	* Neither the name of Arista Networks nor the names of its contributors may
// 	  be used to endorse or promote products derived from this software without
// 	  specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL ARISTA NETWORKS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

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
inline char const * formatString( void const * ) noexcept {
   return sizeof( void * ) == sizeof( uint64_t ) ? "q" : "i";
}

} // namespace QuickTrace

#endif // QUICKTRACE_QUICKTRACEFORMATSTRING_H
