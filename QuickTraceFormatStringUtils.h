// Copyright (c) 2025, Arista Networks, Inc.
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

#ifndef QUICKTRACE_QUICKTRACEFORMATSTRINGUTILS_H
#define QUICKTRACE_QUICKTRACEFORMATSTRINGUTILS_H

#include <QuickTrace/QuickTraceFormatStringTraits.h>

namespace QuickTrace {

template < CanTakeRef T >
inline char const *
formatStringDispatch( T && t ) {
   if constexpr ( HasFormatStringOverloadWithArgForAdl< T > &&
                  !ForceLegacyFormatStringVal< std::remove_cvref_t< T > > ) {
      const MsgFormatStringAdlTag * adlTag = nullptr;
      return formatString( adlTag, std::forward< T >( t ) );
   } else {
      // This is provided for backward compatibility only; this overload form
      // should not be used for new code as it causes header include order issues.
      // The format above with the first argument `MsgFormatString const *` should
      // be preferred as it avoids the ordering issue (using ADL from the point of
      // template instantiation).
      return formatString( std::forward< T >( t ) );
   }
}

// We need this overload for users that do not allow taking a reference, typically
// bitfields, or VLAs.
template < CantTakeRef T >
inline char const *
formatStringDispatch( T t ) {
   if constexpr ( HasFormatStringOverloadWithArgForAdl< T > &&
                  !ForceLegacyFormatStringVal< std::remove_cvref_t< T > > ) {
      const MsgFormatStringAdlTag * adlTag = nullptr;
      return formatString( adlTag, std::forward< T >( t ) );
   } else {
      // This is provided for backward compatibility only; this overload form
      // should not be used for new code as it causes header include order issues.
      // The format above with the first argument `MsgFormatString const *` should
      // be preferred as it avoids the ordering issue (using ADL from the point of
      // template instantiation).
      return formatString( std::forward< T >( t ) );
   }
}

} // namespace QuickTrace

#endif // QUICKTRACE_QUICKTRACEFORMATSTRINGUTILS_H
