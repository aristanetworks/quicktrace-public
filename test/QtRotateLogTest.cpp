// Copyright (c) 2018, Arista Networks, Inc.
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

// Test that the rotateLogFile parameter to QuickTrace::initialize() correctly 
// turns on and off log rotation. Specifically, it tests that when rotateLogFile
// is false, new log files overwrite old ones. When rotateLogFile is true, it 
// verifies that old log files are rotated out to a new file before creating 
// a new one. This test will interchangeably call QuickTrace::initialize() 
// with rotateLogFile set to true and false. It will test with both 
// multithreading enabled and disabled, to ensure it works for both variants.
// Make sure QtGzRotate test pass before looking into this test.


#include <unistd.h>
#include <string>
#include <QuickTrace/QuickTrace.h>
#include <filesystem>

char const * outfile = getenv( "QTFILE" ) ?: "QtRotateTest.qt";
char const * filedir = getenv( "QUICKTRACEDIR" ) ?: ".qt";
const std::string threadName = "MT-";

// Creates and immediately closes a new log file
void createLogFile( bool rotateLogFile, bool multithread ) {
   if( multithread ) { 
      QuickTrace::initialize( outfile, NULL, NULL, 0, 24,
                              QuickTrace::MultiThreading::enabled,
                              rotateLogFile );
      QuickTrace::defaultQuickTraceHandle->maybeCreateMtTraceFile( 
                                                         rotateLogFile );
   } else {
      QuickTrace::initialize( outfile, NULL, NULL, 0, 24,
                              QuickTrace::MultiThreading::disabled,
                              rotateLogFile );
   }

   QuickTrace::close();
}

// check if file exists
bool fileExists( std::string fileName ) {
  return access( fileName.c_str(), F_OK ) == 0;
}

// check if the existence of each file matches the expectation.
void checkResult( bool multithread, int expectedNumLogFiles ) { 
  // Create the file paths, which is different depending on multithreading status
  std::string outfileName;
  if( multithread ) {
     outfileName = threadName + outfile;
  } else {
     outfileName = outfile;
  }
  std::string outfile0 = std::string( filedir ) + '/' + std::string( outfileName );  
  std::string outfile1 = outfile0 + ".1";
  std::string outfile2 = outfile0 + ".2";

  assert( fileExists( outfile0 ) == ( expectedNumLogFiles >= 1 )  &&
            "qt file does not match expected output\n" );
  assert( fileExists( outfile1 ) == ( expectedNumLogFiles >= 2 ) &&
            "qt1 file does not match expected output\n" );
  assert( fileExists( outfile2 ) == ( expectedNumLogFiles >= 3 ) &&
            "qt2 file does not match expected output\n" );
}

// run test with rotateLogFile, expected file exist result set accrodingly.
void testLogFileCreation( bool rotateLogFile, bool multithread,
                          int expectedNumLogFiles ) {
  // Create the log file
  createLogFile( rotateLogFile, multithread );

  // check result
  checkResult( multithread, expectedNumLogFiles );
}

void setup() {
   // Clear out any leftover files from a previous run
   for ( const auto & fname : { "QtRotateTest.qt", "MT-QtRotateTest.qt" } ) {
      for ( const auto & suffix : { "", ".1", ".2" } ) {
         std::string path = std::string(filedir) + "/" + fname + suffix;
         std::filesystem::remove( path );
      }
   }

   // Multithreaded tracefiles use the thread name as part of the filename.
   int ret;
   ret = pthread_setname_np( pthread_self(), threadName.c_str() );
   assert( 0 == ret );

   // Create the initial log files, which should be overwritten when tracing
   // with rotateFileLog false.
   createLogFile( false, false );
   createLogFile( false, true );
}

void runTests( bool multithread ) {
   for( int i = 1; i < 3; i++ ) {
      // Test that with rotateLogFile off, the number of log files remains the same
      testLogFileCreation( false, multithread, i );
      // With rotateLogFile on, the number of log files should increase by 1 (max 3)
      testLogFileCreation( true, multithread, i + 1 );
   }

   // Verify that if rotateLogFile is off and there are already 3 files,
   // creating a new log file still leaves 3 total log files
   testLogFileCreation( false, multithread, 3 );
}

int main() {
   // Set up the environment for testing
   setup();

   // Test functionality for single threaded tracing
   runTests( false );

   // Test functionality for multi threaded tracing
   runTests( true );

   return 0;
}
