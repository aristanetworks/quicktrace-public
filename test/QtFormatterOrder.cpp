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

#include <iostream>
#include <unistd.h>

#include <QuickTrace/QuickTrace.h>

namespace {

struct Bar {
   Bar( int a ) : a_{ a } {}
   int a_;
};

} // namespace

namespace QuickTrace {

const char *
formatString( MsgFormatStringAdlTag const *, Bar const & ) {
   return "i";
}

void
put( RingBuf * rb, const Bar & bar ) {
   rb->push( bar.a_ );
}

} // namespace QuickTrace


int
main( int argc, char ** argv ) {
   char const * outfile = getenv( "QTFILE" ) ?: "qt_formatter_order_test.qt";
   QuickTrace::initialize( outfile );

   static_assert( QuickTrace::HasFormatStringOverloadWithArgForAdl< Bar > );
   const auto bar = Bar( 42 );
   QTRACE0( "Bar: " << QVAR, bar );

   // Test for optionals.
   const std::optional< uint32_t > & opt = 42;
   QTRACE0( "Opt: " << QVAR, opt );

   // Test for bitfields.
   enum ExampleEnum { enum1 };
   struct {
      int i1 : 3;
      ExampleEnum l1 : 3;
   } bitfield = {};
   QTRACE0( "Bitfield: " << QVAR << " " << QVAR, bitfield.i1 << bitfield.l1 );

   // Test for VLAs.
   size_t len = 42;
   char vla[ len ];
   QTRACE0( "strAddr: " << QVAR, vla );
}
