// Copyright (c) 2022, Arista Networks, Inc.
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

#ifndef QUICKTRACE_QUICKTRACEOPTFORMATTER_H
#define QUICKTRACE_QUICKTRACEOPTFORMATTER_H

#include <optional>
#include <QuickTrace/QuickTraceFormatString.h>
#include <QuickTrace/QuickTraceFormatter.h>

namespace QuickTrace {

template< typename T >
struct QtFormatter< std::optional< T > > {
   static inline void put( RingBuf * log, const std::optional< T > & opt ) noexcept {
      if ( opt ) {
         log->push( static_cast< uint8_t >( 1 ) );
         put( log, *opt );
      } else {
         log->push( static_cast< uint8_t >( 0 ) );
      }
   }
   static inline char const * formatString() {
      static constexpr size_t formatStrMaxLen = 16;
      // Use a statically sized buffer to store our runtime generated formatString
      thread_local char formatStr[ formatStrMaxLen ] = {
         0,
      };
      if ( formatStr[ 0 ] == '\0' ) {
         static constexpr std::string_view prefix( "OPT" );

         // Overloads of formatString shouldn't actually care about the underlying
         // instance, and are only used for overload resolution, so default construct
         // the value to pick the right overload, since our optional type might be
         // empty
         char const * typeFormat = QtFormatter< T >::formatString();

         // Craft a format string along the lines of "OPT<FORMAT>". This will be
         // specially decoded in qtcat.
         memcpy( formatStr, prefix.data(), prefix.size() );
         // Only copy the remaining bytes. This could result in a truncated string, but
         // this is better than crashing
         auto remainingSize = formatStrMaxLen - prefix.size() - 1;
         memcpy( formatStr + prefix.size(),
               typeFormat,
               std::min( remainingSize, strlen( typeFormat ) ) );
      }
      return formatStr;
   }
};

template< typename T >
void
put( RingBuf * log, const std::optional< T > & opt ) noexcept {
   if ( opt ) {
      log->push( static_cast< uint8_t >( 1 ) );
      put( log, *opt );
   } else {
      log->push( static_cast< uint8_t >( 0 ) );
   }
}

template< typename T >
char const *
formatString( const std::optional< T > & opt ) {
   static constexpr size_t formatStrMaxLen = 16;
   // Use a statically sized buffer to store our runtime generated formatString
   thread_local char formatStr[ formatStrMaxLen ] = {
      0,
   };
   if ( formatStr[ 0 ] == '\0' ) {
      static constexpr std::string_view prefix( "OPT" );

      // Overloads of formatString shouldn't actually care about the underlying
      // instance, and are only used for overload resolution, so default construct
      // the value to pick the right overload, since our optional type might be
      // empty
      char const * typeFormat = formatString( T{} );

      // Craft a format string along the lines of "OPT<FORMAT>". This will be
      // specially decoded in qttail.
      memcpy( formatStr, prefix.data(), prefix.size() );
      // Only copy the remaining bytes. This could result in a truncated string, but
      // this is better than crashing
      auto remainingSize = formatStrMaxLen - prefix.size() - 1;
      memcpy( formatStr + prefix.size(),
              typeFormat,
              std::min( remainingSize, strlen( typeFormat ) ) );
   }
   return formatStr;
}

} // namespace QuickTrace

#endif // QUICKTRACE_QUICKTRACEOPTFORMATTER_H
