// Copyright (c) 2019, Arista Networks, Inc.
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

#include "QuickTraceFormatTest.h"
#include <cstdlib>
#include <iostream>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

namespace {

pid_t pid = -1;

}
void
verifyLine( const std::string & stage, const std::string & actual,
            const std::string & expected ) {
   std::string::size_type spos = actual.find( SEP1 );
   std::string::size_type epos = actual.rfind( SEP2 );
   if ( spos == std::string::npos || epos == std::string::npos ||
         actual.substr( spos + 2, epos - spos - 2 ) != expected ) {
      std::cerr << "expected: " << expected << std::endl;
      std::cerr << "actual: " << actual << std::endl;
      pexit( ( stage + ": parameter format differs in trace line" ).c_str() );
   } else {
      spos = actual.find_last_of( " \"", spos );
      std::cout << stage << ' ' << actual.substr( spos + 1, epos - spos + 1 )
                << " ... ok" << std::endl;
   }
}

std::list< std::string >
testQtTail( const char * qtFileName, TraceFunc * traceFuncs,
            unsigned numTraceFuncs ) {
   int fd = runQtTail( qtFileName );
   std::string line = readQtLine( fd );
   std::cout << line;
   std::list< std::string > expectedLines;
   auto verify = [&]( const TraceFuncReturn & expectedTraces ) {
      for ( const std::string & expected : expectedTraces ) {
         std::string actual = readQtLine( fd );
         verifyLine( "qttail", actual, expected );
         expectedLines.push_back( expected );
      }
   };
   for ( unsigned  i = 0; i < numTraceFuncs; i++ ) {
      TraceFuncReturn expectedTraces = ( traceFuncs[ i ] )();
      verify( expectedTraces );
   }
   TraceFuncReturn expectedTraces = traceStatic();
   verify( expectedTraces );
   close( fd );
   killProcess();
   return expectedLines;
}

std::string
initializeQuickTrace( const char * fileName, unsigned bufferSize,
                      bool multiThreading ) {
   QuickTrace::SizeSpec sizeSpec = { bufferSize, bufferSize, bufferSize, bufferSize,
                                     bufferSize, bufferSize, bufferSize, bufferSize,
                                     bufferSize, bufferSize };
   if ( multiThreading ) {
      setenv( "QUICKTRACEDIR", "/tmp", 1 );
      QuickTrace::initializeMt( fileName, &sizeSpec );
      QuickTrace::defaultQuickTraceHandle->maybeCreateMtTraceFile();
      return "";
   } else {
      std::string qtFileName = std::string( "/tmp/" ) + fileName;
      QuickTrace::initialize( qtFileName.c_str(), &sizeSpec );
      return qtFileName;
   }
}

void
killProcess() {
   if ( pid > 0 ) {
      signalProcess( SIGKILL );
      pid = -1;
   }
}

void
pabort( const char * msg ) {
   perror( msg );
   exit( EXIT_FAILURE );
}

void
pexit( const char * msg ) {
   std::cerr << msg << std::endl;
   exit( EXIT_FAILURE );
}

std::string
readQtLine( int fd, bool failOnEof, unsigned timeoutMs ) {
   static std::string lbuf;
   std::string::size_type p = lbuf.find( '\n' );
   while ( p == std::string::npos ) {
      char buf[ 4096 ];
      ssize_t n = read( fd, buf, sizeof( buf ) );
      if ( n < 0 ) {
         if ( errno == EAGAIN ) {
            pollfd fds;
            fds.fd = fd;
            fds.events = POLLIN;
            n = poll( &fds, 1, timeoutMs );
            if ( n < 0 ) {
               pabort( "poll" );
            } else if ( n == 0 ) {
               pexit( "unexpected timeout" );
            } else {
               continue;
            }
         } else {
            pabort( "read" );
         }
      }
      if ( n == 0 ) {
         if ( failOnEof ) {
            pexit( "unexpected EOF" );
         } else {
            return "";
         }
      }
      lbuf.append( buf, n );
      p = lbuf.find( '\n' );
   }
   std::string line = lbuf.substr( 0, p + 1 );
   lbuf.erase( 0, p + 1 );
   return line;
}

std::string
randomAsciiString( unsigned minLength, unsigned maxLength ) {
   unsigned length = maxLength == 0 ? minLength : randomInt( minLength, maxLength );
   std::string s;
   for ( unsigned i = 0; i < length; i++ ) {
      char ch = randomInt( ' ', '\x7f' );
      while ( ch == '"' || ch == '%' ) {
         ch = randomInt( ' ', '\x7f' );
      }
      s += ch;
   }
   return s;
}

unsigned
randomInt( unsigned minVal, unsigned maxVal ) {
   unsigned range = maxVal - minVal;
   if ( range == 0 ) {
      return minVal;
   } else if ( range <= INT32_MAX ) {
      // 31 bits of randomness are sufficient
      return minVal + lrand48() % ( ++range );
   } else {
      // need 32 bits of randomness
      unsigned randNum = ( lrand48() << 1 ) ^ lrand48();
      if ( ++range != 0 ) {
         randNum = minVal + randNum % range;
      }
      return randNum;
   }
}

int
runProcess( const char * file, const char ** argv ) {
   int pipefd[ 2 ];
   if ( pipe( pipefd ) < 0 ) {
      pabort( "pipe" );
   }
   pid = fork();
   if ( pid < 0 ) {
      pabort( "fork" );
   }
   if ( pid == 0 ) {
      close( pipefd[ 0 ] );
      dup2( pipefd[ 1 ], STDOUT_FILENO );
      dup2( pipefd[ 1 ], STDERR_FILENO );
      close( pipefd[ 1 ] );
      execvp( file, (char * const *) argv );
      pabort( "execvp" );
   } else {
      close( pipefd[ 1 ] );
   }
   int flags = fcntl( pipefd[ 0 ], F_GETFL );
   if ( flags < 0 ) {
      pabort( "fcntl" );
   }
   fcntl( pipefd[ 0 ], F_SETFL, flags | O_NONBLOCK );
   return pipefd[ 0 ];
}

int
runQtTail( const char * qtFileName, bool debug ) {
   const char * argv[ 4 ] = { "qttail", debug ? "-dx" : "-x", qtFileName, nullptr };
   return runProcess( argv[ 0 ], argv );
}

void
signalProcess( int signum ) {
   if ( pid > 0 ) {
      kill( pid, signum );
   }
}

TraceFuncReturn
traceStatic() {
   std::string str1 = randomAsciiString(), str2 = randomAsciiString();
   QTRACE0( __FUNCTION__ << SEP1 << str1.c_str() << SEP2, QNULL );
   QTRACE0( __FUNCTION__ << SEP1 << str2.c_str() << SEP2, QNULL );
   return { str1, str2 };
}
