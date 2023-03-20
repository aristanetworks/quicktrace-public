// Copyright (c) 2021 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

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

const char *
formatString( const CorruptU8 & ) {
   return "u";
}

void
put( QuickTrace::RingBuf * rb, const CorruptU8 & labelOp ) {
   switch ( labelOp.what ) {
    case 1: // wrong datatype
      rb->push( 1.0 );
      break;
    case 2: // wrong datatype
      rb->push( "world" );
      break;
   }
}

} // namespace

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
