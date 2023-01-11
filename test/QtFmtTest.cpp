// Copyright (c) 2021, Arista Networks, Inc.
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
#include <QuickTrace/QtFmtGeneric.h>

// This would normally be defined by the non-generic user of QTFMT
#define qtraceClassName "ClassName"

void
testQtFmtClass() {
   QTFMT0( "QtFmtTest noparams extra" );
   QTFMT0( "QtFmtTest lvl0 {} {:x}", 42, 0xcafe );
   QTFMT1( "QtFmtTest lvl1 {} {:x}", 42, 0xcafe );
   QTFMT2( "QtFmtTest lvl2 {} {:x}", 42, 0xcafe );
   QTFMT3( "QtFmtTest lvl3 {} {:x}", 42, 0xcafe );
   QTFMT4( "QtFmtTest lvl4 {} {:x}", 42, 0xcafe );
   QTFMT5( "QtFmtTest lvl5 {} {:x}", 42, 0xcafe );
   QTFMT6( "QtFmtTest lvl6 {} {:x}", 42, 0xcafe );
   QTFMT7( "QtFmtTest lvl7 {} {:x}", 42, 0xcafe );
   QTFMT8( "QtFmtTest lvl8 {} {:x}", 42, 0xcafe );
   QTFMT9( "QtFmtTest lvl9 {} {:x}", 42, 0xcafe );

   QTFMT_PROF0( "QtFmtTest noparamsprof extra" );
   QTFMT_PROF0( "QtFmtTest prof0 {} {:x}", 42, 0xcafe );
   QTFMT_PROF1( "QtFmtTest prof1 {} {:x}", 42, 0xcafe );
   QTFMT_PROF2( "QtFmtTest prof2 {} {:x}", 42, 0xcafe );
   QTFMT_PROF3( "QtFmtTest prof3 {} {:x}", 42, 0xcafe );
   QTFMT_PROF4( "QtFmtTest prof4 {} {:x}", 42, 0xcafe );
   QTFMT_PROF5( "QtFmtTest prof5 {} {:x}", 42, 0xcafe );
   QTFMT_PROF6( "QtFmtTest prof6 {} {:x}", 42, 0xcafe );
   QTFMT_PROF7( "QtFmtTest prof7 {} {:x}", 42, 0xcafe );
   QTFMT_PROF8( "QtFmtTest prof8 {} {:x}", 42, 0xcafe );
   QTFMT_PROF9( "QtFmtTest prof9 {} {:x}", 42, 0xcafe );
}

// Validate that the QTFMT and QTFMT_PROF macros behave as expected.
// This is the driver which will populate the .qt file, and we'll validate the
// output of qttail/qtprof using QtFmtTest
void
testQtFmtFunc() {
   QTFMT0_FUNC( "QtFmtTest noparams extra" );
   QTFMT0_FUNC( "QtFmtTest lvl0 {} {:x}", 42, 0xcafe );
   QTFMT1_FUNC( "QtFmtTest lvl1 {} {:x}", 42, 0xcafe );
   QTFMT2_FUNC( "QtFmtTest lvl2 {} {:x}", 42, 0xcafe );
   QTFMT3_FUNC( "QtFmtTest lvl3 {} {:x}", 42, 0xcafe );
   QTFMT4_FUNC( "QtFmtTest lvl4 {} {:x}", 42, 0xcafe );
   QTFMT5_FUNC( "QtFmtTest lvl5 {} {:x}", 42, 0xcafe );
   QTFMT6_FUNC( "QtFmtTest lvl6 {} {:x}", 42, 0xcafe );
   QTFMT7_FUNC( "QtFmtTest lvl7 {} {:x}", 42, 0xcafe );
   QTFMT8_FUNC( "QtFmtTest lvl8 {} {:x}", 42, 0xcafe );
   QTFMT9_FUNC( "QtFmtTest lvl9 {} {:x}", 42, 0xcafe );

   QTFMT_PROF0_FUNC( "QtFmtTest noparamsprof extra" );
   QTFMT_PROF0_FUNC( "QtFmtTest prof0 {} {:x}", 42, 0xcafe );
   QTFMT_PROF1_FUNC( "QtFmtTest prof1 {} {:x}", 42, 0xcafe );
   QTFMT_PROF2_FUNC( "QtFmtTest prof2 {} {:x}", 42, 0xcafe );
   QTFMT_PROF3_FUNC( "QtFmtTest prof3 {} {:x}", 42, 0xcafe );
   QTFMT_PROF4_FUNC( "QtFmtTest prof4 {} {:x}", 42, 0xcafe );
   QTFMT_PROF5_FUNC( "QtFmtTest prof5 {} {:x}", 42, 0xcafe );
   QTFMT_PROF6_FUNC( "QtFmtTest prof6 {} {:x}", 42, 0xcafe );
   QTFMT_PROF7_FUNC( "QtFmtTest prof7 {} {:x}", 42, 0xcafe );
   QTFMT_PROF8_FUNC( "QtFmtTest prof8 {} {:x}", 42, 0xcafe );
   QTFMT_PROF9_FUNC( "QtFmtTest prof9 {} {:x}", 42, 0xcafe );
}

int
main() {
   char const * outfile = getenv( "QTFILE" ) ?: "QtFmtTest.out";
   QuickTrace::initialize( outfile );

   // First test the raw version
   QTFMT0_RAW( "QtFmtTest noparams extra" );
   QTFMT0_RAW( "QtFmtTest lvl0 {} {:x}", 42, 0xcafe );
   QTFMT1_RAW( "QtFmtTest lvl1 {} {:x}", 42, 0xcafe );
   QTFMT2_RAW( "QtFmtTest lvl2 {} {:x}", 42, 0xcafe );
   QTFMT3_RAW( "QtFmtTest lvl3 {} {:x}", 42, 0xcafe );
   QTFMT4_RAW( "QtFmtTest lvl4 {} {:x}", 42, 0xcafe );
   QTFMT5_RAW( "QtFmtTest lvl5 {} {:x}", 42, 0xcafe );
   QTFMT6_RAW( "QtFmtTest lvl6 {} {:x}", 42, 0xcafe );
   QTFMT7_RAW( "QtFmtTest lvl7 {} {:x}", 42, 0xcafe );
   QTFMT8_RAW( "QtFmtTest lvl8 {} {:x}", 42, 0xcafe );
   QTFMT9_RAW( "QtFmtTest lvl9 {} {:x}", 42, 0xcafe );

   QTFMT_PROF0_RAW( "QtFmtTest noparamsprof extra" );
   QTFMT_PROF0_RAW( "QtFmtTest prof0 {} {:x}", 42, 0xcafe );
   QTFMT_PROF1_RAW( "QtFmtTest prof1 {} {:x}", 42, 0xcafe );
   QTFMT_PROF2_RAW( "QtFmtTest prof2 {} {:x}", 42, 0xcafe );
   QTFMT_PROF3_RAW( "QtFmtTest prof3 {} {:x}", 42, 0xcafe );
   QTFMT_PROF4_RAW( "QtFmtTest prof4 {} {:x}", 42, 0xcafe );
   QTFMT_PROF5_RAW( "QtFmtTest prof5 {} {:x}", 42, 0xcafe );
   QTFMT_PROF6_RAW( "QtFmtTest prof6 {} {:x}", 42, 0xcafe );
   QTFMT_PROF7_RAW( "QtFmtTest prof7 {} {:x}", 42, 0xcafe );
   QTFMT_PROF8_RAW( "QtFmtTest prof8 {} {:x}", 42, 0xcafe );
   QTFMT_PROF9_RAW( "QtFmtTest prof9 {} {:x}", 42, 0xcafe );

   testQtFmtFunc();
   testQtFmtClass();

   std::cout << QuickTrace::theTraceFile->fileName();

   return 0;
}
