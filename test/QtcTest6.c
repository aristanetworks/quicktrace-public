// Copyright (c) 2010, Arista Networks, Inc.
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

#include <stdlib.h>
#include <QuickTrace/QuickTrace.h>

char const *outfile1 = "qtct6.out";
char const *outfile2 = "qtct6-2.out";
char const *outfile3 = "qtct6-3.out";

int 
main ( int argc, char const ** argv ) 
{
   void *hdl1 = qt_initialize_handle(
         outfile1, "16, 16, 16, 16, 16, 16, 16, 16, 16, 16");
   void *hdl2 = qt_initialize_handle(
         outfile2, "16, 16, 16, 16, 16, 16, 16, 16, 16, 16");
   void *hdl3 = qt_initialize_handle(
         outfile3, "16, 16, 16, 16, 16, 16, 16, 16, 16, 16");
   uint64_t N = 1000000, i;

   if( argc > 1 ) { N = strtoll( argv[1], 0, 0 ); }

   // Test writing to file 1
   for( i = 0; i < N; ++i ) {
      QTRACEF0_F( hdl1, "qt0%ld", i*10 );
      QTRACEF1_F( hdl1, "qt0%ld", i*10 );
      QTRACEF2_F( hdl1, "qt0%ld", i*10 );
      QTRACEF3_F( hdl1, "qt0%ld", i*10 );
      QTRACEF4_F( hdl1, "qt0%ld", i*10 );
      QTRACEF5_F( hdl1, "qt0%ld", i*10 );
      QTRACEF6_F( hdl1, "qt0%ld", i*10 );
      QTRACEF7_F( hdl1, "qt0%ld", i*10 );
      QTRACEF8_F( hdl1, "qt0%ld", i*10 );
      QTRACEF9_F( hdl1, "qt0%ld", i*10 );
   }

   for( i = 0; i < N; ++i ) {
      QPROF_F( hdl1, "time to make ten QPROF of int" );
      QPROFF0_F( hdl1, "profInt%ld", i*10 );
      QPROFF1_F( hdl1, "qt0%ld", i*10 );
      QPROFF2_F( hdl1, "qt0%ld", i*10 );
      QPROFF3_F( hdl1, "qt0%ld", i*10 );
      QPROFF4_F( hdl1, "qt0%ld", i*10 );
      QPROFF5_F( hdl1, "qt0%ld", i*10 );
      QPROFF6_F( hdl1, "qt0%ld", i*10 );
      QPROFF7_F( hdl1, "qt0%ld", i*10 );
      QPROFF8_F( hdl1, "qt0%ld", i*10 );
      QPROFF9_F( hdl1, "qt0%ld", i*10 );
   }

   // Test writing to file 2
   for( i = 0; i < N; ++i ) {
      QTRACEF0_F( hdl2, "qt0%ld", i*10 );
      QTRACEF1_F( hdl2, "qt0%ld", i*10 );
      QTRACEF2_F( hdl2, "qt0%ld", i*10 );
      QTRACEF3_F( hdl2, "qt0%ld", i*10 );
      QTRACEF4_F( hdl2, "qt0%ld", i*10 );
      QTRACEF5_F( hdl2, "qt0%ld", i*10 );
      QTRACEF6_F( hdl2, "qt0%ld", i*10 );
      QTRACEF7_F( hdl2, "qt0%ld", i*10 );
      QTRACEF8_F( hdl2, "qt0%ld", i*10 );
      QTRACEF9_F( hdl2, "qt0%ld", i*10 );
   }

   for( i = 0; i < N; ++i ) {
      QPROF_F( hdl2, "time to make ten QPROF of int" );
      QPROFF0_F( hdl2, "profInt%ld", i*10 );
      QPROFF1_F( hdl2, "qt0%ld", i*10 );
      QPROFF2_F( hdl2, "qt0%ld", i*10 );
      QPROFF3_F( hdl2, "qt0%ld", i*10 );
      QPROFF4_F( hdl2, "qt0%ld", i*10 );
      QPROFF5_F( hdl2, "qt0%ld", i*10 );
      QPROFF6_F( hdl2, "qt0%ld", i*10 );
      QPROFF7_F( hdl2, "qt0%ld", i*10 );
      QPROFF8_F( hdl2, "qt0%ld", i*10 );
      QPROFF9_F( hdl2, "qt0%ld", i*10 );
   }

   // Test writing to file 3
   for( i = 0; i < N; ++i ) {
      QTRACEF0_F( hdl3, "qt0%ld", i*10 );
      QTRACEF1_F( hdl3, "qt0%ld", i*10 );
      QTRACEF2_F( hdl3, "qt0%ld", i*10 );
      QTRACEF3_F( hdl3, "qt0%ld", i*10 );
      QTRACEF4_F( hdl3, "qt0%ld", i*10 );
      QTRACEF5_F( hdl3, "qt0%ld", i*10 );
      QTRACEF6_F( hdl3, "qt0%ld", i*10 );
      QTRACEF7_F( hdl3, "qt0%ld", i*10 );
      QTRACEF8_F( hdl3, "qt0%ld", i*10 );
      QTRACEF9_F( hdl3, "qt0%ld", i*10 );
   }

   for( i = 0; i < N; ++i ) {
      QPROF_F( hdl3, "time to make ten QPROF of int" );
      QPROFF0_F( hdl3, "profInt%ld", i*10 );
      QPROFF1_F( hdl3, "qt0%ld", i*10 );
      QPROFF2_F( hdl3, "qt0%ld", i*10 );
      QPROFF3_F( hdl3, "qt0%ld", i*10 );
      QPROFF4_F( hdl3, "qt0%ld", i*10 );
      QPROFF5_F( hdl3, "qt0%ld", i*10 );
      QPROFF6_F( hdl3, "qt0%ld", i*10 );
      QPROFF7_F( hdl3, "qt0%ld", i*10 );
      QPROFF8_F( hdl3, "qt0%ld", i*10 );
      QPROFF9_F( hdl3, "qt0%ld", i*10 );
   }

   return 0;
}
