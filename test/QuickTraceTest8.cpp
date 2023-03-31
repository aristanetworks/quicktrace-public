// Copyright (c) 2017, Arista Networks, Inc.
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

// Test trace buffers of different sizes

// Each trace level should hold more messages than the last.
// We can verify this with qtcat, by counting the number of times
// each trace appears in th output.

// Each trace is 21 bytes:
//   U64 tsc
//   U32 msgId
//   U64 custom parameter
//   U8 len/

// so each trace ring should store an aditional ~48 messages over the
// previous one

#include <QuickTrace/QuickTrace.h>
#include <stdlib.h>

using namespace QuickTrace;

static const char * outfile = getenv( "QTFILE" ) ?: "qtt8.out";
static const uint64_t NUM_LOOPS = 1000;

int
main( int argc, char * argv[] ) {
   QuickTrace::SizeSpec ss = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 };
   QuickTrace::initialize( outfile, &ss );

   for ( uint64_t i = 0; i < NUM_LOOPS; ++i ) {
      QTRACE0( "Level0 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE1( "Level1 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE2( "Level2 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE3( "Level3 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE4( "Level4 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE5( "Level5 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE6( "Level6 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE7( "Level7 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE8( "Level8 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE9( "Level9 regex match this!" << QVAR, i * 10 + 1 );
   }

   return 0;
}
