// Copyright (c) 2017 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#include <iostream>
#include <QuickTrace/QuickTrace.h>
#include <assert.h>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <unistd.h>
#include <sys/wait.h> // waitpid

// Verify multiple threads writing to thread specific trace files.
// QuickTrace file output validated in QuickTraceMultipleThreadsTest.py

static constexpr const uint threadCount = 10;
static const char * qtStringFormatBuf[ threadCount ];
static QuickTrace::TraceHandle * thDef;
static QuickTrace::TraceHandle * th1;
static QuickTrace::TraceHandle * th2;
static std::string thDefName;
static std::string th1Name;
static std::string th2Name;
static QuickTrace::TraceFile * traceFileDef[ threadCount ];
static QuickTrace::TraceFile * traceFile1[ threadCount ];
static QuickTrace::TraceFile * traceFile2[ threadCount ];

static std::mutex threadsReadyForForkMutex;
static std::condition_variable threadReadyForForkCv;
static uint threadsReadyForFork;
static std::mutex forkCompleteMutex;
static std::condition_variable forkCompleteCv;
static bool forkComplete;

static constexpr const uint messageIdCount = 500;
static QuickTrace::MsgId messageId[ messageIdCount ];
static std::atomic_uint threads_completed_tracefile_creation = 0;

static void
setThreadName( uint index ) {
   std::string name = "QtMtTest-";
   name += std::to_string( index );
   int ret;
   ret = pthread_setname_np( pthread_self(), name.c_str() );
   assert( 0 == ret );
}

static void
qtTestThreadWaitForFork() {
   // Let the main thread know we are waiting for it to perform
   // the fork test.
   {
      std::lock_guard< std::mutex > readyLock( threadsReadyForForkMutex );
      threadsReadyForFork += 1;
   }
   threadReadyForForkCv.notify_one();
   // Wait for main thread to perform fork test and then resume
   // our ouput.
   std::unique_lock< std::mutex > forkLock( forkCompleteMutex );
   while( !forkComplete ) {
      forkCompleteCv.wait( forkLock );
   }
}

static void
generateTrace( uint index ) {
   for( uint msgIdIndex = 0; msgIdIndex < messageIdCount; ++msgIdIndex ) {
      QuickTrace::MsgId * msgIdPtr = &messageId[ msgIdIndex ];
      if( msgIdIndex == 2 ) {
         // Do the fork early in the cycle as post-fork all the
         // threads will be released simultaneously making sure we get
         // concurrent traces. The first batch of traces may end up
         // being serialised because the amount of time it takes to
         // open the thread-specific TraceFiles is longer than the
         // amount of time taken by the tracing.
         qtTestThreadWaitForFork();
      }
      for( uint level = 0; level < QuickTrace::TraceFile::NumTraceLevels; ++level ) {
         for( uint i = 0; i < 4; ++i ) {
            QTPROF_H_MSGID( QuickTrace::theTraceFile, *msgIdPtr, level,
                            "D-" << QVAR << "-" << QVAR << "-" << QVAR,
                            index << msgIdIndex << i );
            QTPROF_H_MSGID( th1->getFile(), *msgIdPtr, level,
                            "1-" << QVAR << "-" << QVAR << "-" << QVAR,
                            index << msgIdIndex << i );
            QTPROF_H_MSGID( th2->getFile(), *msgIdPtr, level,
                            "2-" << QVAR << "-" << QVAR << "-" << QVAR,
                            index << msgIdIndex << i );
         }
      }
   }
}

static void *
qtTestThread( void *arg ) {
   uint index = *( uint * )arg;
   setThreadName( index );
   qtStringFormatBuf[ index ] = QuickTrace::QtString::str();

   // Verify that no trace files are generated when we trace before calling
   // maybeCreateMtTraceFile()
   generateTrace( index );
   assert( QuickTrace::defaultQuickTraceFile() == NULL );
   assert( th1->traceFile() == NULL );
   assert( th2->traceFile() == NULL );

   // Create the trace files and verify that traces are generated
   QuickTrace::defaultQuickTraceHandle->maybeCreateMtTraceFile();
   traceFileDef[ index ] = QuickTrace::defaultQuickTraceFile();
   // std::cerr << traceFileDef[ index ]->fileName() << std::endl;
   th1->maybeCreateMtTraceFile();
   traceFile1[ index ] = th1->traceFile();
   th2->maybeCreateMtTraceFile();
   traceFile2[ index ] = th2->traceFile();

   threads_completed_tracefile_creation++;
   // wait for all the other threads to create their files to prevent
   // a race deleting the QuickTrace::TraceFile objects returned by the
   // maybeCreate... functions above (this function creates the
   // QuickTrace::TraceFile object on the heap and registers a deletion
   // handler which is called when the thread exits)
   // If we loose the race it can result in the same
   // address for the QuickTrace::TraceFile pointer being returned on multiple
   // threads.
   // This is actually OK, but this test has an assert at the end to
   // verify all the pointer values are different (yes this could be
   // improved but that is not the whole issue).
   // Additionally, the way the name of the QT file is generated only
   // guarentees uniqueness while the thread is alive, adding this check
   // ensures also all the threads are alive when they create their trace
   // files, ensuring they have unique filenames.
   while ( threads_completed_tracefile_creation < threadCount ) {
      sleep( 1 );
   }

   generateTrace( index );
   assert( QuickTrace::defaultQuickTraceFile() != NULL );
   assert( th1->traceFile() != NULL );
   assert( th2->traceFile() != NULL );

   // std::cerr << index << " done" << std::endl;
   return NULL;
}

static void
assertTraceFiles( bool exist, bool initialized ) {
   assert( exist == ( NULL != QuickTrace::defaultQuickTraceHandle ) );
   assert( exist == ( thDef == QuickTrace::defaultQuickTraceHandle ) );
   assert( exist == QuickTrace::traceHandleExists( thDefName ) );
   assert( exist == QuickTrace::traceHandleExists( th1Name ) );
   assert( exist == QuickTrace::traceHandleExists( th2Name ) );
   if( !exist ) {
      return;
   }
   assert( initialized == QuickTrace::defaultQuickTraceHandle->isInitialized() );
   assert( initialized == th1->isInitialized() );
   assert( initialized == th2->isInitialized() );
}

static void
forkTest( bool traceHandlesDeleted ) {
   pid_t pid = fork();
   switch( pid ) {
    case -1:
      // Fork failed
      assert( false );
    case 0:
      // Child. Verify TraceHandles were closed
      assertTraceFiles( !traceHandlesDeleted, false );
      _exit( 0 );
    default:
      // Parent
      break;
   }
   // Verify QuickTrace is still functional
   assertTraceFiles( true, true );
   // Verify the forked child process didn't encounter any issues.
   int status;
   waitpid( pid, &status, 0 );
   assert( 0 == status );
}

int
main( int argc, char * const * argv ) {
   pthread_t thread[ threadCount ];
   uint threadArg[ threadCount ];
   int ret;

   // Create the default trace file and two non-default trace files
   // per thread.
   QuickTrace::SizeSpec sizeSpec = {
      256, 256, 256, 256, 256, 256, 256, 256, 256, 256
   };
   bool ok;
   ok = QuickTrace::initializeMt( "-default", &sizeSpec );
   assert( ok );
   thDef = QuickTrace::defaultQuickTraceHandle;
   thDefName = thDef->fileNameSuffix();
   th1 = QuickTrace::initializeHandleMt( "-1", &sizeSpec );
   assert( NULL != th1 );
   th1Name = th1->fileNameSuffix();
   th2 = QuickTrace::initializeHandleMt( "-2", &sizeSpec );
   assert( NULL != th2 );
   th2Name = th2->fileNameSuffix();

   // Run multiple parallel threads producing trace output
   for( uint i = 0; i < threadCount; ++i ) {
      threadArg[ i ] = i;
      ret = pthread_create( &thread[ i ], NULL, qtTestThread, &threadArg[ i ] );
      assert( 0 == ret );
   }

   // We want the threads to be active when we perform the fork
   // test so we stop them before completeing. Wait for all the
   // threads to get to the point where they are ready for us to
   // perform the fork test.
   {
      std::unique_lock< std::mutex > readyLock( threadsReadyForForkMutex );
      while( threadsReadyForFork != threadCount ) {
         threadReadyForForkCv.wait( readyLock );
      }
   }

   // Perform fork tests.
   forkTest( false );
   QuickTrace::setDeleteTraceHandlesOnFork();
   forkTest( true );

   // Let the threads know that we are done with the fork test and
   // they can produce more output to make sure there are no issues
   // post-fork.
   {
      std::lock_guard< std::mutex > forkLock( forkCompleteMutex );
      forkComplete = true;
   }
   forkCompleteCv.notify_all();

   // Wait for the threads to complete
   for( uint i = 0; i < threadCount; ++i ) {
      ret = pthread_join( thread[ threadCount - 1 - i ], NULL );
      assert( 0 == ret );
   }

   // Make sure each thread gets a unique buffer
   for( uint i = 0; i < threadCount; ++i ) {
      assert( qtStringFormatBuf[ i ] != NULL );
      assert( traceFileDef[ i ] != NULL );
      assert( traceFile1[ i ] != NULL );
      assert( traceFile2[ i ] != NULL );
      for( uint j = 0; j < i; ++j ) {
         assert( qtStringFormatBuf[ i ] != qtStringFormatBuf[ j ] );
         assert( traceFileDef[ i ] != traceFileDef[ j ] );
         assert( traceFile1[ i ] != traceFile1[ j ] );
         assert( traceFile2[ i ] != traceFile2[ j ] );
      }
   }
   return 0;
}
