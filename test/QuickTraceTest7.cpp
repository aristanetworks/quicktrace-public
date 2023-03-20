// Copyright (c) 2011 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#include <stdlib.h>
#include <QuickTrace/QuickTrace.h>

char const *outfile = getenv( "QTFILE" ) ?: "qtt7";

using namespace QuickTrace;

// Purpose of this is to test QuickTrace for multiple trace files.
// Test QTRACEx_F and QPROFx_F macros.

int main (int argc, char const ** argv)
{
   char outfile1[100];
   char outfile2[100];
   uint64_t n = (argc > 1) ? atoll( argv[1] ) : 1000 * 1000;

   // Test initialize_handle API.
   snprintf(outfile1, 100, "%s-1.out", outfile);
   snprintf(outfile2, 100, "%s-2.out", outfile);
   TraceHandle *th1 = QuickTrace::initialize_handle(outfile1); 
   TraceHandle *th2 = QuickTrace::initialize_handle(outfile2); 

   // Test all the QTRACEx_F macros.
   for( uint64_t i = 0; i < n; ++i ) {
      QTRACE0_F( th1, "Sample test %d", i );
      QTRACE1_F( th1, "Sample test %d", i );
      QTRACE2_F( th1, "Sample test %d", i );
      QTRACE3_F( th1, "Sample test %d", i );
      QTRACE4_F( th1, "Sample test %d", i );
      QTRACE5_F( th1, "Sample test %d", i );
      QTRACE6_F( th1, "Sample test %d", i );
      QTRACE7_F( th1, "Sample test %d", i );
      QTRACE8_F( th1, "Sample test %d", i );
      QTRACE9_F( th1, "Sample test %d", i );

      QTRACE0_F( th2, "Sample test %d", i );
      QTRACE1_F( th2, "Sample test %d", i );
      QTRACE2_F( th2, "Sample test %d", i );
      QTRACE3_F( th2, "Sample test %d", i );
      QTRACE4_F( th2, "Sample test %d", i );
      QTRACE5_F( th2, "Sample test %d", i );
      QTRACE6_F( th2, "Sample test %d", i );
      QTRACE7_F( th2, "Sample test %d", i );
      QTRACE8_F( th2, "Sample test %d", i );
      QTRACE9_F( th2, "Sample test %d", i );
   }

   // Test all the QPROFx_F macros.
   for ( uint32_t i = 0; i < 10; i++ ) {
      QPROF0_F( th1, "Profiling %d", i );
      QPROF0_F( th2, "Profiling %d", i );
      for ( uint32_t j = 0; j < n; j++ ) {
         // busy work
      }
   }

   delete th1;
   delete th2;
}
