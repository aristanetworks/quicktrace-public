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

#ifndef QUICKTRACE_QUICKTRACEFORMATSTRINGTRAITS_H
#define QUICKTRACE_QUICKTRACEFORMATSTRINGTRAITS_H

#include <type_traits>
#include <utility>

namespace QuickTrace {

// Declaring everything needed for custom formatter definitions here, so that files
// that need to declare formatters don't have to include QuickTrace.h, which causes
// issues downstream for other formatter definitions that are not yet using the ADL
// definition style.
class MsgFormatStringAdlTag {};

template< typename T >
concept HasFormatStringOverloadWithArgForAdl = requires( T & t ) {
   { formatString( std::declval< const MsgFormatStringAdlTag * >(), t ) };
};

// We need this to detect type that do not allow taking a reference, typically
// bitfields, or VLAs.
template< typename T >
concept CantTakeRef = ( std::is_integral_v< std::remove_cvref_t< T > > ||
                        std::is_enum_v< std::remove_cvref_t< T > > ||
                        std::is_pointer_v< std::decay_t<T> > );

template< typename T >
concept CanTakeRef = !CantTakeRef< T >;

// Users may use this trait to force the resolution for a type to use the legacy
// formatString overload. This is provided as a workaround for cases where ADL-style
// overload is not yet available for a type Foo, but Foo happens to be implicitly
// convertible to a type that has an ADL-style overload and thus
// HasFormatStringOverloadWithArgForAdl is a false positive, leading to incorrect
// resolution.
template< typename T >
struct ForceLegacyFormatString : std::false_type
{};

template< typename T >
inline constexpr bool ForceLegacyFormatStringVal =
   ForceLegacyFormatString< T >::value;

} // namespace QuickTrace

#endif // QUICKTRACE_QUICKTRACEFORMATSTRINGTRAITS_H
