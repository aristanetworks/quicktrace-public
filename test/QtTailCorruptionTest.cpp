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

// Verfifies that qttail recovers (with potential message loss) when the writer
// produces a corrupt message.

#include <iostream>
#include <unistd.h>
#include "QuickTraceFormatTest.h"

namespace {

struct CorruptU8 {
   CorruptU8( int what ) : what(what) {}
   int what;
};

} // namespace

template<>
struct QuickTrace::QtFormatter< CorruptU8 > {
   static inline void put( RingBuf * rb, CorruptU8 labelOp ) noexcept {
      switch ( labelOp.what ) {
       case 1: // wrong datatype
         rb->push( 1.0 );
         break;
       case 2: // wrong datatype
         rb->push( "world" );
         break;
      }
   }
   static inline char const * formatString() noexcept {
      return "u";
   }
};

int
main( int argc, char ** argv ) {
   atexit( &killProcess );
   std::string qtFileName = initializeQuickTrace( "qttail_corruption_test.qt", 8 );
   int qtTailFd = runQtTail( qtFileName.c_str() );
   std::string line = readQtLine( qtTailFd );
   int testsPassed = 0;
   for ( int test = 1; test <= 2; test++ ) {
      // must have a good message first. if the corrupt message is at the beginning
      // of the ring buffer, qttail will not reset the buffer (because it is already
      // reset)
      switch ( test ) {
       case 1:
         QTRACE0( "a good message", QNULL );
         QTRACE0( "corrupt msg: bad datatype of float " << QVAR, CorruptU8( 1 ) );
         break;
       case 2:
         QTRACE1( "a good message", QNULL );
         QTRACE1( "corrupt msg: bad datatype of string " << QVAR, CorruptU8( 2 ) );
         break;
      }
      int i, matchedLines = 0;
      for ( i = 0; matchedLines != 0x03; i++ ) {
         line = readQtLine( qtTailFd, false );
         if ( line.empty() ) {
            break; // qttail exited or timeout
         }
         if ( !isdigit( line[ 0 ] ) ) {
            std::cout << "[i=" << i << "] " << line;
            if ( line.compare( 0, 31, "---------- resetting log buffer" ) == 0 ) {
               matchedLines |= 0x01;
            } else if ( line.compare( 0, 30, "---------- corruption detected" )
                           == 0 ) {
               matchedLines |= 0x02;
            }
         }
      }
      if ( matchedLines == 0x03 ) {
         std::cout << "corruption detected after " << i << " lines" << std::endl;
         testsPassed |= 1 << ( test - 1 );
      } else {
         std::cout << "corruption not detected after " << i << " lines" << std::endl;
      }
   }
   close( qtTailFd );
   return testsPassed == 0x03 ? EXIT_SUCCESS : EXIT_FAILURE;
}
