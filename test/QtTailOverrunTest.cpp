// Copyright (c) 2019 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

// Verfifies that qttail recovers (with potential message loss) when the writer
// produces messages at a faster rate than what qttail can consume.

#include <iostream>
#include <thread>
#include <signal.h>
#include <unistd.h>
#include "QuickTraceFormatTest.h"

namespace {

void
emitTraceMsgs( int count, int messageLength, int msgNum ) {
   // enforce message length by padding it with a random ASCII string
   // baseLength is 13 + 4 + 1:
   // 13: 8 bytes tsc, 4 bytes message id, 1 byte message length
   // 4: size of traced integer
   // 1: size of encoded string length
   constexpr int baseLength = 13 + 4 + 1;
   assert( messageLength >= baseLength );
   for ( int i = 0; i < count; i++, msgNum++ ) {
      std::string s = randomAsciiString( messageLength - baseLength );
      QTRACE0( "message number " << QVAR << ": " << QVAR, msgNum << s.c_str() );
   }
}

bool
expectOverrun( int qtTailFd ) {
   bool gotOverrun = false;
   for ( int i = 0; i < 10; i++ ) {
      std::string line = readQtLine( qtTailFd, false );
      if ( line.empty() ) {
         std::cout << "*** unexpected EOF from qttail" << std::endl;
         return false;
      }
      if ( isdigit( line[ 0 ] ) ) {
         std::cout << "*** unexpected message from qttail: " << line;
         return false;
      } else {
         if ( line.compare( 0, 31, "---------- resetting log buffer" ) == 0 ) {
            gotOverrun = true;
            break;
         }
      }
   }
   if ( !gotOverrun ) {
      std::cout << "*** missing overrun message from qttail" << std::endl;
   }
   return gotOverrun;
}

bool
expectTraceMsgs( int qtTailFd, int count, int msgNum ) {
   for ( int i = 0; i < count; i++ ) {
      std::string line = readQtLine( qtTailFd, false );
      if ( line.empty() ) {
         std::cout << "*** unexpected EOF from qttail" << std::endl;
         return false;
      }
      std::string expected = "message number " + std::to_string( msgNum++ ) + ":";
      if ( !isdigit( line[ 0 ] ) || line.find( expected ) == std::string::npos ) {
         std::cout << "*** unexpected message from qttail: " << line;
         return false;
      }
   }
   return true;
}

bool
testOverrun( int qtTailFd ) {
   int bufSize = 1024, trailerLen = 256;
   int sizeToFill = bufSize - trailerLen;
   int msgLength = 21; // start with messages that are 21 bytes long
   int totalMsgsNeeded = ( sizeToFill / msgLength ) + 1;
   int firstMsgNum = 1;
   int numMsgs = 3; // start with tracing 3 messages
   emitTraceMsgs( numMsgs, msgLength, firstMsgNum );
   if ( !expectTraceMsgs( qtTailFd, numMsgs, firstMsgNum ) ) {
      return false;
   }

   // qttail is at offset 63 : 3 * 21, stop it now
   signalProcess( SIGSTOP );

   // emit a certain number of messages so that it ends up in the trailer
   firstMsgNum += numMsgs;
   numMsgs = totalMsgsNeeded - numMsgs;
   emitTraceMsgs( numMsgs, msgLength, firstMsgNum );

   // emit additional messages, which wrap the buffer and start at offset 0
   // each message is 25 bytes, so when it overwrites the first 75 bytes of the
   // ring buffer it overruns qttail which is at offset 63
   firstMsgNum += numMsgs;
   msgLength = 25; // overwrite with messages that are 25 bytes long
   totalMsgsNeeded = ( sizeToFill / msgLength ) + 1;
   emitTraceMsgs( totalMsgsNeeded, msgLength, firstMsgNum );

   // let qttail continue and detect the overrun
   signalProcess( SIGCONT );
   if ( !expectOverrun( qtTailFd ) ) {
      return false;
   }

   // qttail resets to offset 0 of the ring buffer; expect all messages
   if ( !expectTraceMsgs( qtTailFd, totalMsgsNeeded, firstMsgNum ) ) {
      return false;
   }

   return true;
}

} // namespace

int
main( int argc, char ** argv ) {
   srand48( time( nullptr ) );
   atexit( &killProcess );
   std::string qtFileName = initializeQuickTrace( "qttail_overrun_test.qt", 1 );
   int qtTailFd = runQtTail( qtFileName.c_str() );
   std::string line = readQtLine( qtTailFd ); // skip output from -x
   bool ok = testOverrun( qtTailFd );
   close( qtTailFd );
   return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
