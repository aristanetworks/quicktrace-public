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

#include <cstdint>
#include <iostream>
#include <stdlib.h>
#include <QuickTrace/QuickTrace.h>
#include <QuickTrace/QuickTraceCommon.h>

uint64_t qt_rdtsc_mocker;

char const * outfile = getenv( "QTFILE" ) ?: "QtOffBitTest.out";

void
AddQt( float ft, int i ) {
   QTRACE1( "float ft" << QVAR << " " << QVAR, i << ft );
}

int
main( int argc, char const ** argv ) {
   qt_rdtsc_mocker = 0;
   QuickTrace::initialize( outfile );

   float f1 = 1.0;
   float f2 = 3.14;
   float f3 = 4.14;
   float f4 = 5.14;

   AddQt( f1, 1 );
   AddQt( f2, 2 );

   // We set 60th bit to 1. we'll shift right by 28 bits, so this resulted in the
   // 32nd bit being set in the lastTsc, leading to the message being switched off
   qt_rdtsc_mocker = 0x800000000000000; // 60th bit is 1

   AddQt( f3, 3 );
   AddQt( f4, 4 );
}
