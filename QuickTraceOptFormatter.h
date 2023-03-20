// Copyright (c) 2022 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#ifndef QUICKTRACE_QUICKTRACEOPTFORMATTER_H
#define QUICKTRACE_QUICKTRACEOPTFORMATTER_H

#include <optional>
#include <QuickTrace/QuickTraceFormatString.h>

namespace QuickTrace {

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
