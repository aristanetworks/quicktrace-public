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
