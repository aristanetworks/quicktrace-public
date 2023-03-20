// Copyright (c) 2019 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#include "QuickTraceFormatTest.h"
#include "QuickTraceFormatBasicTestFunctions.h"
#include <cstdlib>
#include <iostream>

int
runTest( const char * fileName, TraceFunc * traceFuncs, unsigned numTraceFuncs ) {
   srand48( time( nullptr ) );
   atexit( &killProcess );
   std::string qtFileName = initializeQuickTrace( fileName, 16 );
   std::list< std::string > expectedLines =
      testQtTail( qtFileName.c_str(), traceFuncs, numTraceFuncs );
   std::cout << "*** test passed ***" << std::endl;
   return EXIT_SUCCESS;
}

int
main( int /*argc*/, const char ** /*argv*/ ) {
   return runTest(
      "qtformat_basic_test.qt",
      QuickTrace::traceFuncs,
      sizeof( QuickTrace::traceFuncs ) / sizeof( QuickTrace::traceFuncs[ 0 ] ) );
}
