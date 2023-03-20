// Copyright (c) 2021 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

// Verfifies that qttail handles message ids appearing out of order.

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <gtest/gtest.h>
#include "QuickTraceFormatTest.h"

namespace {

class Thread {
public:
   Thread( std::string name ) :
      name_( std::move( name ) ),
      request_( 0 ),
      response_( 0 )
   {}

   ~Thread() {
      if ( thread_.joinable() ) {
         stop();
      }
   }

   int getRequest() {
      std::unique_lock< std::mutex > lock( lock_ );
      requestReady_.wait( lock, [ this ]{ return request_ != 0; } );
      return request_;
   }

   void setRequest( int request ) {
      {
         std::lock_guard< std::mutex > lock( lock_ );
         request_ = request;
         response_ = 0;
      }
      requestReady_.notify_one();
      std::unique_lock< std::mutex > lock( lock_ );
      responseReady_.wait( lock, [ this ]{ return response_ != 0; } );
   }

   void setResponse( int response ) {
      {
         std::lock_guard< std::mutex > lock( lock_ );
         request_ = 0;
         response_ = response;
      }
      responseReady_.notify_one();
   }

   void trace1() {
      QTRACE0( "message one", QNULL );
   }

   void trace2() {
      QTRACE0( "message two", QNULL );
   }

   void trace3() {
      QTRACE0( "message three", QNULL );
   }

   void run() {
      pthread_setname_np( pthread_self(), name_.c_str() );
      QuickTrace::defaultQuickTraceHandle->maybeCreateMtTraceFile();
      int request;
      while ( ( request = getRequest() ) > 0 ) {
         switch ( request ) {
          case 1:
            trace1();
            break;
          case 2:
            trace2();
            break;
          case 3:
            trace3();
            break;
         }
         setResponse( request );
      }
      setResponse( request );
   }

   void start() {
      thread_ = std::thread( &Thread::run, this );
   }

   void stop() {
      setRequest( -1 );
      thread_.join();
   }

private:
   std::string name_;
   std::mutex lock_;
   int request_;
   int response_;
   std::condition_variable requestReady_;
   std::condition_variable responseReady_;
   std::thread thread_;
};

} // namespace

TEST( QtTailTest, LogMessageOutOfOrder ) {
   Thread thread1( "threadOne" );
   thread1.start();
   Thread thread2( "threadTwo" );
   thread2.start();
   thread1.setRequest( 1 ); // trace 3 messages, with message ids 1, 2 and 3
   thread1.setRequest( 2 );
   thread1.setRequest( 3 );
   thread2.setRequest( 2 ); // trace message id 2

   std::string qtFileName = "/tmp/threadTwo_qttail_newlogmessage_order_test.qt";
   int qtTailFd = runQtTail( qtFileName.c_str() );
   std::string line = readQtLine( qtTailFd ); // skip output from -x

   thread2.setRequest( 3 ); // trace message id 3
   line = readQtLine( qtTailFd );
   ASSERT_FALSE( line.find( "message three" ) == std::string::npos );

   // trace message id 1, which will now be after message ids 2 and 3 in the
   // thread-specific qt file
   thread2.setRequest( 1 );
   line = readQtLine( qtTailFd );
   ASSERT_FALSE( line.find( "message one" ) == std::string::npos );

   close( qtTailFd );
}

int main( int argc, char **argv ) {
   atexit( &killProcess );
   initializeQuickTrace( "_qttail_newlogmessage_order_test.qt", 1, true );
   ::testing::InitGoogleTest( &argc, argv );
   return RUN_ALL_TESTS();
}
