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

// This tests ForceLegacyFormatString to force the resolution of the legacy
// formatString overload, used to prevent incorrect resolution to an implicitly
// convertible type.

#include <assert.h>
#include <iostream>
#include <unistd.h>

#include <QuickTrace/QuickTraceRingBuf.h>

namespace NS {

struct Foo1 {
   Foo1( int a ) : a_{ a } {}
   int a_;
};

struct Bar1 {
   Bar1( int a ) : a_{ a } {}
   int a_;

   operator Foo1() { return Foo1( a_ ); }
};

struct Foo2 {
   Foo2( int a ) : a_{ a } {}
   int a_;
};

struct Bar2 {
   Bar2( int a ) : a_{ a } {}
   int a_;

   operator Foo2() { return Foo2( a_ ); }
};

} // namespace NS

namespace QuickTrace {

const char *
formatString( const MsgFormatStringAdlTag *, const NS::Foo1 & ) {
   // formatString for Foo1 should not be called since we're qtracing only Bar1.
   assert( false );
   return "i";
}

void
put( RingBuf * rb, const NS::Foo1 & foo ) {
   rb->push( foo.a_ );
}

template<>
struct ForceLegacyFormatString< NS::Bar1 > : std::true_type
{};

const char *
formatString( const NS::Bar1 & ) {
   return "i";
}

void
put( RingBuf * rb, const NS::Bar1 & bar ) {
   rb->push( bar.a_ );
}

const char *
formatString( const MsgFormatStringAdlTag *, const NS::Foo2 & ) {
   return "i";
}

void
put( RingBuf * rb, const NS::Foo2 & foo ) {
   rb->push( foo.a_ );
}

const char *
formatString( const NS::Bar2 & ) {
   // formatString for Bar2 should not be called even if we're qtracing Bar2 because
   // it is implicitly convertible to Foo1 and ForceLegacyFormatString is not used
   // for Bar2.
   assert( false );
   return "i";
}

void
put( RingBuf * rb, const NS::Bar2 & bar ) {
   rb->push( bar.a_ );
}

} // namespace QuickTrace

#include <QuickTrace/QuickTrace.h>

int
main( int argc, char ** argv ) {
   char const * outfile = getenv( "QTFILE" ) ?: "qt_formatter_order_test.qt";
   QuickTrace::initialize( outfile );
   auto bar1 = NS::Bar1( 42 );
   QTRACE0( "Bar1: " << QVAR, bar1 );

   auto bar2 = NS::Bar2( 42 );
   QTRACE0( "Bar2: " << QVAR, bar2 );
}
