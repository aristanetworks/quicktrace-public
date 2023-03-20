// Copyright (c) 2021 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

// Test that the registerHandle hook is invoked on QuickTrace initialize
// when the registerHandle extern function is defined. Simple test that defines
// the register handle method to set a boolean to true and verifies that
// the boolean is set when the test is run.

#include <QuickTrace/QuickTrace.h>
#include <QuickTrace/Registration.h>
#include <iostream>

char const * outfile = getenv( "QTFILE" ) ?: "QtRegisterTest.qt";
bool registered = false;
bool isDefaultHandle = false;

int main( int argc, char const ** argv ) {
   // implementation of register handle method used as callback in QuickTrace
   QuickTrace::registerHandle = []( const QuickTrace::TraceHandle & handle,
                                    bool defaultHandle ) {
      std::cout<< "TraceHandle registered by test lambda method" << std::endl;
      registered = true;
      isDefaultHandle = defaultHandle;
   };

   // initialize quicktrace file and write to it, close
   QuickTrace::initialize( outfile );
   QTRACE1( "Hola", 10.3 );
   std::cout << "Registered: " << registered << " default: " << isDefaultHandle
             << std::endl;
   assert( registered == true );
   assert( isDefaultHandle == true );
   QuickTrace::close();
}
