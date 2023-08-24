// Copyright (c) 2011, Arista Networks, Inc.
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

#ifndef QUICKTRACE_QUICKTRACE_H
#define QUICKTRACE_QUICKTRACE_H

#ifdef __cplusplus
#include <optional>
#include <string.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <assert.h>

/*
  To do to clean this all up:
  - Make it easy to get the TraceFile for an agent.
  - /var/log/agents/qt.agent
  - make it bulletproof so tracing can't crash the agent
    even if the filesys is full, etc.  Corrupting the file
    is ok, but crashing is bad.
  - Tests
  - Robustness
  - Align message counters to cache lines
*/
#include <stdint.h>
#include <iomanip>
#include <string>
#include <QuickTrace/QuickTraceCommon.h>
#include <QuickTrace/QuickTraceRingBuf.h> // provides the RingBuffer to put the messages
#include <QuickTrace/QuickTraceFormatString.h>
#include <QuickTrace/QuickTraceOptFormatter.h>

#define DEFAULT_NUM_MSG_COUNTERS 512

namespace QuickTrace {

static constexpr pthread_key_t invalidPthreadKey = 0;

// The following structs, with no members and a trivial constructor,
// serve as place holders so you can shift them into a MsgDesc.
struct Varg {};                 // %s-formatted argument
struct HexVarg {};                  // hex-formatted argument

class MsgFormatString {
 public:
   MsgFormatString( char * keyBuf ) noexcept
         : key_( keyBuf ), ptr_( keyBuf ) { key_[0] = 0; }
   template < class T >
   MsgFormatString & operator<<( T t ) noexcept {
      if constexpr ( has_qtformatter< T > ) {
         put( QtFormatter< T >::formatString() );
      } else {
         put( formatString( t ) );
      }
      return *this;
   }
   MsgFormatString & operator<<( QNull t ) noexcept {
      return *this;
   }
   char const * key() noexcept { return key_; }
   void put( const char * s ) noexcept;
   static int const keySize = 256;
 private:
   void put( QNull s ) noexcept {}
   char * key_;
   char * ptr_;
};

//  "Pascal" string (long) with an integer length followed by data.
// Pascal string format is used in the output file
// Since it is used primarily for serialization and deserialization, use pack to
// lower the structs alignment to 1 from the default of 4 to avoid UB surrounding
// misaligned pointer accesses.
#pragma pack( push, 1 )
struct PStringL {
   int len;
   char data[0];
};
#pragma pack( pop )

class TraceFile;
// A MsgDesc describes the static parameters of a QuickTrace
// statement, and is created the first time a QuickTrace statement is
// executed.  It contains the timestamp that the message was first
// recorded, the __FILE__ and __LINE__ of the message, a message
// string suitable for passing to sprintf with any 'int' parameters
// replaced by %d, any char * parameters replaced by their value, and
// any other parameter replaced by "<>".
//
// MsgDescs are added at the end of the QuickTrace file after the
// memory mapped section containing the RingBufs. In a version 2
// QuickTrace file the position of the MsgDesc in the list of
// descriptors at the end of the file conveys its MsgId. From version
// 3 onwards, a thread-specific file only contains the descriptors for
// traces issued against that file and the MsgId is encoded explicitly
// in the entry.
class MsgDesc {
 public:
   MsgDesc( TraceFile *tf, MsgId *msgIdPtr, char const * file,
            int line ) noexcept;
   void finish() noexcept;
   const char * buf() noexcept { return buf_; }
   MsgDesc & operator<<( const std::string_view & x ) noexcept;
   MsgDesc & operator<<( char const * x ) noexcept;
   MsgDesc & operator<<( Varg x ) noexcept;
   MsgDesc & operator<<( HexVarg x ) noexcept;
   MsgId id() noexcept { return id_; }
   MsgFormatString & formatString() noexcept { return formatString_; }
   static int const bufSize = 1024;

   // For testing purposes
   const char * ptr() const noexcept { return ptr_; }

 private:
   TraceFile * tf_;
   char * buf_;
   char *ptr_, *end_;
   MsgFormatString formatString_;
   PStringL * pstr_;
   MsgId id_;
};

struct SizeSpec {
   // Sizes, in kilobytes of the individual trace buffers
   // So for example, if you construct one  like this:
   //   SizeSpec s = {1000,100,1, 1,1,1,1,1,1,1000};
   // you'll allocate a megabyte for 0 and 9, 100K for 1, and 1K for
   // everything else
   static constexpr int SIZE = 10;
   uint32_t sz[ SIZE ];

   bool operator==( const SizeSpec &other ) const{
      for ( int i = 0; i < SIZE; i++ ){
         if( sz[i] != other.sz[i] ){
            return false;
         }
      }
      return true;
   }
};

struct TraceFileHeader {
   uint32_t version;
   uint32_t fileSize;
   uint32_t fileHeaderSize;
   uint32_t fileTrailerSize;
   uint32_t firstMsgOffset;
   uint32_t logCount;
   uint64_t tsc0;
   double monotime0;
   uint64_t tsc1;
   double monotime1;
   double utc1;
   SizeSpec logSizes;
};

enum class MultiThreading {
   disabled,
   enabled,
};

class TraceHandle;

// The TraceFile class manages a single QuickTrace file. For
// multi-threaded processes, a separate TraceFile is created by the
// TraceHandle for each thread issuing traces. This makes the
// TraceFile thread-specific.
class TraceFile {
 public:
   TraceFile( TraceHandle * traceHandle, bool rotateLogFile=true,
              uint32_t numMsgCounters = DEFAULT_NUM_MSG_COUNTERS ) noexcept;
   ~TraceFile() noexcept;
   bool initialized() noexcept { return initialized_; }
   void sync() noexcept;
   int fd() const noexcept { return fd_; }
   RingBuf & log( int i ) noexcept {
      return log_[i];
   }
   void takeTimestamp() noexcept;
   MsgCounter * msgCounter( int msgId ) noexcept{
      TraceFileHeader* sfh = (TraceFileHeader*) buf_;
      MsgCounter * m = (MsgCounter*) (sfh+1);
      return &(m[msgId % numMsgCounters_]);
   }
   char const * fileName() noexcept { return fileName_.c_str(); }
   void closeIfNeeded() noexcept;
   static int const FileTrailerSize = 1024;
   bool msgIdInitialized( MsgId msgId_ ) noexcept{
      size_t msgId = ( size_t )msgId_;
      // When the msgId is non-zero, it means that we have allocated
      // an ID for this trace. However, if the ID was allocated by a
      // different thread or a previous instance of the TraceFile, and
      // never used by the current thread, then msgIdInitialized_ will
      // be false for this thread-specific TraceFile.
      return( ( 0 != msgId ) &&
                ( ( msgIdInitialized_.size() > msgId ) &&
                  msgIdInitialized_[ msgId ] ) );
   }
   void msgIdInitializedIs( MsgId msgId ) noexcept;
   void maybeBackupBuffer() noexcept;
   enum {
      NumTraceLevels = 10
   };
   struct timeval wallClockTimestamp() const noexcept;

 private:
   friend class MsgDesc;
   MultiThreading multiThreading_;
   TraceHandle * traceHandle_;
   uint32_t numMsgCounters_;

   // Thread specific buffers to support operations during message
   // initialisation
   char msgDescBuf_[ MsgDesc::bufSize ];
   char msgFormatStringKey_[ MsgFormatString::keySize ];

   // Vector of booleans indicating if a specific messageId has been
   // used against this TraceFile.
   std::vector< uint8_t >msgIdInitialized_;
   RingBuf log_[NumTraceLevels];
   void * buf_;
   int fd_;
   std::string fileName_;
   bool initialized_;
};

// A single TraceHandle is created for each call to
// QuickTrace::initialize() or QuickTrace::initialize_handle(). For
// non multi-threaded processes, a single TraceFile is created for
// each TraceHandle. For multi-threaded processes, a per-thread
// TraceFile will be created by the TraceHandle for each thread
// issuing traces.
class TraceHandle {
 public:
   TraceHandle( char const * fileNameFormat,
                SizeSpec * sizeSpec = NULL,
                char const * foreverLogPath = NULL,
                int foreverLogIndex = 0,
                MultiThreading multiThreading = MultiThreading::disabled,
                bool rotateLogFile = true,
                uint32_t numMsgCounters = DEFAULT_NUM_MSG_COUNTERS,
                bool defaultHandle = false ) noexcept;
   ~TraceHandle() noexcept;

   std::string qtdir() const noexcept { return qtdir_; }
   std::string fileNameFormat() const noexcept { return fileNameFormat_; }
   std::string fileNameSuffix() const noexcept { return fileNameSuffix_; }
   uint32_t mappedTraceFileSize() const noexcept { return mappedTraceFileSize_; }
   SizeSpec sizeSpec() const noexcept { return sizeSpec_; }
   bool resize( const SizeSpec &newSizeSpecInKilobytes ) noexcept;
   static std::optional< std::string > getQtDir(
      MultiThreading & multiThreading, std::string & fileNameFormat ) noexcept;

   // Allocate and retrieve message Ids.
   // When multi-threaded the message ID space is common across all
   // the TraceFiles for this handle.
   inline void allocateMsgId( MsgId * msgIdPtr ) noexcept;

   // Obtain the thread specific TraceFile
   inline TraceFile * traceFile() noexcept {
      if( multiThreading_ == MultiThreading::disabled ) {
         return nonMtTraceFile_;
      }
      return mtTraceFile();
   }
   inline TraceFile * getFile() noexcept { return traceFile(); }
   TraceFile * maybeCreateMtTraceFile( bool rotateLogFile = true ) noexcept;

   const std::string &foreverLogPath() noexcept { return foreverLogPath_; }
   void foreverLogIndexInc( int delta ) noexcept { foreverLogIndex_ += delta; }
   int foreverLogIndex() noexcept { return foreverLogIndex_; }
   bool foreverLog() noexcept { return foreverLog_; }

   inline bool isInitialized() noexcept {
      return initialized_ && !traceFilesClosed_;
   }
   void close() noexcept;
 private:
   friend class TraceFile;
   MultiThreading multiThreading_;
   bool traceFilesClosed_;
   uint32_t numMsgCounters_;

   TraceFile * newTraceFile(
         bool rotateLogFile = true,
         uint32_t numMsgCounters = DEFAULT_NUM_MSG_COUNTERS ) noexcept;
   TraceFile * mtTraceFile() noexcept;
   // Close existing TraceFiles for this TraceHandle and prevent
   // creation of new ones.
   void disableTracing() noexcept;

   //adjust the mapped size spec after setting sizeSpec_
   void adjustSizeSpec( bool limit = true ) noexcept;

   // Per TraceHandle mutex to prevent concurrent MsgId allocation
   // from the thread-specific TraceFiles.
   std::mutex msgIdAllocMutex;

   MsgId nextMsgId_;

   // When not multiThreaded we use this single TraceFile for
   // efficiency
   TraceFile * nonMtTraceFile_;

   // Set of all the active TraceFiles. Only includes the
   // nonMtTraceFile_ in the non-MT case.
   std::unordered_set< TraceFile * >traceFiles_;

   // When multiThreaded we use thread local storage to keep track of
   // the thread specific TraceFiles.
   pthread_key_t traceFileThreadLocalKey_;

   // The size of the trace file without any message descriptors.
   // Incudes the TraceFileHeader, MsgCounters and RingBufs.
   uint32_t mappedTraceFileSize_;

   std::string fileNameFormat_;
   std::string fileNameSuffix_;
   std::string qtdir_;
   int foreverLogIndex_;
   std::string foreverLogPath_;
   bool foreverLog_;
   SizeSpec sizeSpec_;
   bool initialized_;
};

extern TraceHandle * defaultQuickTraceHandle;
// Get the thread-specific default TraceFile.
static inline TraceFile * defaultQuickTraceFile() noexcept {
   if( likely( NULL != ::QuickTrace::defaultQuickTraceHandle ) ) {
      return ::QuickTrace::defaultQuickTraceHandle->traceFile();
   }
   return NULL;
}
#define theTraceFile defaultQuickTraceFile()
bool traceHandleExists( const std::string & filenameSuffix ) noexcept;

class BlockTimer {
 public:
   BlockTimer( TraceFile * sf, MsgId mid ) noexcept
         : qtFile_( sf ), msgId_( mid ), tsc_( rdtsc() ) {}
   ~BlockTimer() noexcept;
 private:
   TraceFile * qtFile_;
   MsgId msgId_;
   uint64_t tsc_;
};

class BlockTimerMsg {
 public:
   BlockTimerMsg( TraceFile * sf, MsgId mid, uint64_t now ) noexcept
         : qtFile_( sf ), msgId_( mid ), tsc_( now ) {}
   ~BlockTimerMsg() noexcept;
 private:
   TraceFile * qtFile_;
   MsgId msgId_;
   uint64_t tsc_;
};

class BlockTimerSelf {
 public:
   BlockTimerSelf( TraceFile * sf, MsgId mid ) noexcept;
   ~BlockTimerSelf() noexcept;
 private:
   TraceFile * qtFile_;
   MsgId msgId_;
   uint64_t tsc_;
};

class BlockTimerSelfMsg {
 public:
   BlockTimerSelfMsg( TraceFile * sf, MsgId mid, uint64_t now ) noexcept;
   ~BlockTimerSelfMsg() noexcept;
 private:
   TraceFile * qtFile_;
   MsgId msgId_;
   uint64_t tsc_;
};


class QtString  {
 public:
   QtString() noexcept :
      ptr_( formatBuf_ ), end_( ptr_ + formatBufSize - 1 ), hex_( false ) {
      end_[0] = 0;
   }
   template < class T > QtString & operator<<( T t ) noexcept {
      ptr_ = stpncpy( ptr_, hex_ ? "%x" : "%s", end_ - ptr_ );
      return *this;
   }
   QtString & operator<<( std::basic_ostream<char, std::char_traits<char> >&
                          ( * manipulator )( std::basic_ostream<char,
                                             std::char_traits< char > > & )
      ) noexcept {
      *this << " ";
      return *this;
   }

   QtString & operator<<( decltype( std::hex ) h ) noexcept {
      if( h == std::hex ) { hex_ = true; }
      return *this;
   }
   QtString & operator<<( char const * s ) noexcept {
      ptr_ = stpncpy( ptr_, s, end_ - ptr_ );
      return *this;
   }
   static constexpr const size_t formatBufSize = 256;
   static char const * str() noexcept { return formatBuf_; }
 private:
   static thread_local char formatBuf_[ formatBufSize ];
   char * ptr_;
   char * end_;
   bool hex_;
};

// The maximum length of a string in the dynamic portion of QuickTrace. 
// Defaults to 24. Any string longer than qtMaxStringLen will be truncated to
// fit this limit. Cannot be set higher than 80.
extern int qtMaxStringLen;

// Request that QuickTrace closes and deletes all TraceHandles when
// the process forks.
void setDeleteTraceHandlesOnFork() noexcept;

// Existing and new agents that would like to have a single trace log file can
// continue using the initialize() function, which creates a global instance 
// of TraceFile.
bool initialize( char const * prefix, SizeSpec * sizesInKilobytes=0,
                 char const *foreverLogPath=NULL, int foreverLogIndex=0,
                 int maxStringLen=24,
                 MultiThreading multiThreading=MultiThreading::disabled,
                 bool rotateLogFile=true,
                 uint32_t numMsgCounters = DEFAULT_NUM_MSG_COUNTERS ) noexcept;
static inline bool
initializeMt( char const * prefix, SizeSpec * sizesInKilobytes=0,
              char const *foreverLogPath=NULL, int foreverLogIndex=0,
              int maxStringLen=24, 
              uint32_t numMsgCounters = DEFAULT_NUM_MSG_COUNTERS ) noexcept {
   return initialize( prefix, sizesInKilobytes, foreverLogPath, foreverLogIndex,
                      maxStringLen, MultiThreading::enabled, true, numMsgCounters );
}
void close() noexcept;

TraceHandle * traceHandle( const std::string & traceFilename ) noexcept;

// Processes that require multiple quicktrace log files can create
// multiple TraceHandles. initialize_handle() can be used in
// combination with initialize() in which case the
// defaultQuickTraceHandle will be used in addition to the
// TraceHandles created through initialize_handle().
TraceHandle *
initialize_handle( char const * prefix, SizeSpec * ss=0,
                   char const *foreverLogPath=NULL, int foreverLogIndex=0,
                   MultiThreading multiThreading = MultiThreading::disabled
   ) noexcept;
static inline TraceHandle *
initializeHandleMt( char const * prefix, SizeSpec * ss=0,
                    char const *foreverLogPath = NULL,
                    int foreverLogIndex = 0 ) noexcept {
   return initialize_handle( prefix, ss, foreverLogPath, foreverLogIndex,
                             MultiThreading::enabled );
}

} // namespace QuickTrace

// qtvar is clever macro hackery to get a variable name that is unique
// to each instance of a macro within the file.  It achieves this by
// leveraging the fact that __LINE__ is the same for the entire body
// of a single MACRO and then mangling the line number into the
// variable name.  You actually *need* three levels of nested macros
// to trick the macro preprocessor into actually expanding __LINE__ in
// the body of the macro, and thus your variable becomes "x342".  With
// fewer levels of macros, it gets named "x__LINE__".
#define qtvar__(a,x) _qt_##a##x
#define qtvar_(a,x) qtvar__(a,x)
#define qtvar( a ) qtvar_(a,__LINE__)

// A single invocation is 287 bytes with -Os, 449 with -O3 or -O2
// The actual macro that gets inlined at the point of call to record a
// message into the buffer.  It allocates a new MsgDesc the first
// time it is called

// Some simple benchmarks of cycles/ns and byte size of the code
//                  Core2(2.53Mhz)   K7(1.2Mhz)  Bytes
// QPROF0():           43/16           70/40     252
// QTRACE one int:     52/19           71/39     175
// QPROF1 one int      80/30          108/60     252
// QPROF only          46/17           44/24     133
// QTRACE 13 byte str  80/30          125/70     200
// QTRACE 24 byte str 105/43          162/90     200
// QTRACE 8 ints       65/26           95/53     350
//
// Both the cycles and byte counters were taken from user code
// compiled -O2 and with libQuickTrace.so compiled -O2 with gcc-4.4.3

// The goal of the _VAR macros below is to make sure that we only
// invoke qtvar() a single time for each variable. This seems to be
// needed as in some cases gcc advances __LINE__ in the middle of a
// macro.

// Base macro for tracing events
#define QTRACE_H( _qtf, _n, _x, _y )                                    \
   do {                                                                 \
      static QuickTrace::MsgId _msgId;                                  \
      if( likely( !!(_qtf) ) ) {                                        \
         QTRACE_H_MSGID( _qtf, _msgId, _n, _x, _y );                    \
      }                                                                 \
   } while(0)

#define QTRACE_H_MSGID( _qtf, _msgId, _n, _x, _y )      \
   QTRACE_H_MSGID_VAR( _qtf, _msgId, qtvar(_rb), _n, _x, _y )

#define QTRACE_H_MSGID_INIT_BASIC( _qtf, _msgId, _x )                      \
   if( unlikely( !!( _qtf ) && !( _qtf )->msgIdInitialized( _msgId ) ) ) { \
      QuickTrace::MsgDesc _qtmd( _qtf, &_msgId, __FILE__, __LINE__ );      \
      _qtmd << _x;                                                         \
      _qtmd.finish();                                                      \
   }

#define QTRACE_H_MSGID_INIT_FMT( _qtf, _msgId, _x, _y )                    \
   if( unlikely( !!( _qtf ) && !( _qtf )->msgIdInitialized( _msgId ) ) ) { \
      QuickTrace::MsgDesc _qtmd( _qtf, &_msgId, __FILE__, __LINE__ );      \
      _qtmd.formatString() << _y;                                          \
      _qtmd << _x;                                                         \
      _qtmd.finish();                                                      \
   }

#define QTRACE_H_MSGID_VAR( _qtf, _msgId, _rb, _n, _x, _y )             \
   QTRACE_H_MSGID_INIT_FMT( _qtf, _msgId, _x, _y );                     \
   QuickTrace::RingBuf & _rb = ( _qtf )->log( _n );                     \
   _rb.startMsg( _qtf, _msgId );                                        \
   _rb << _y;                                                           \
   _rb.endMsg()

// Base macro for tracing events and profiling
#define QTPROF_H( _qtf, _n, _x, _y )                                    \
   QTPROF_H_VAR( _qtf, qtvar(msgid), _n, _x, _y )

#define QTPROF_H_VAR( _qtf, _msgId, _n, _x, _y )                        \
   static QuickTrace::MsgId _msgId;                                     \
   QTPROF_H_MSGID( _qtf, _msgId, _n, _x, _y )

#define QTPROF_H_MSGID( _qtf, _msgId, _n, _x, _y )                      \
   QTPROF_H_MSGID_VAR( _qtf, _msgId,                                    \
                       qtvar(tsc),  _n, _x, _y )

#define QTPROF_H_MSGID_VAR( _qtf, _msgId, _tsc, _n, _x, _y )                        \
   uint64_t _tsc;                                                                   \
   if ( likely( !!( _qtf ) ) ) {                                                    \
      QTRACE_H_MSGID_INIT_FMT( _qtf, _msgId, _x, _y );                              \
      QuickTrace::RingBuf & _rb = ( _qtf )->log( _n );                              \
      qtvar( tsc ) = _rb.startMsg( _qtf, _msgId );                                  \
      _rb << _y;                                                                    \
      _rb.endMsg();                                                                 \
   } else {                                                                         \
      _tsc = 0;                                                                     \
   }                                                                                \
   QuickTrace::BlockTimerMsg qtvar( bt )( ( _qtf ), _msgId, _tsc )

// Base macro for tracing events and profiling (including self profiling)
#define QTPROF_H_S( _qtf, _n, _x, _y )                                    \
   QTPROF_H_S_VAR( _qtf, qtvar(msgid), _n, _x, _y )

#define QTPROF_H_S_VAR( _qtf, _msgId, _n, _x, _y )                        \
   static QuickTrace::MsgId _msgId;                                     \
   QTPROF_H_S_MSGID( _qtf, _msgId, _n, _x, _y )

#define QTPROF_H_S_MSGID( _qtf, _msgId, _n, _x, _y )                      \
   QTPROF_H_S_MSGID_VAR( _qtf, _msgId,                                    \
                       qtvar(tsc),  _n, _x, _y )

#define QTPROF_H_S_MSGID_VAR( _qtf, _msgId, _tsc, _n, _x, _y )                      \
   uint64_t _tsc;                                                                   \
   if ( likely( !!( _qtf ) ) ) {                                                    \
      QTRACE_H_MSGID_INIT_FMT( _qtf, _msgId, _x, _y );                              \
      QuickTrace::RingBuf & _rb = ( _qtf )->log( _n );                              \
      qtvar( tsc ) = _rb.startMsg( _qtf, _msgId );                                  \
      _rb << _y;                                                                    \
      _rb.endMsg();                                                                 \
   } else {                                                                         \
      _tsc = 0;                                                                     \
   }                                                                                \
   QuickTrace::BlockTimerSelfMsg qtvar( bt )( ( _qtf ), _msgId, _tsc )

// Base macro for profiling without adding a trace in the log
#define QPROF_H( _qtf, _x )                                             \
   QPROF_H_VAR( _qtf, qtvar(msgid), _x )

#define QPROF_H_VAR( _qtf, _msgId, _x )                                 \
   static QuickTrace::MsgId _msgId;                                     \
   QPROF_H_MSGID( _qtf, _msgId, _x )

#define QPROF_H_MSGID( _qtf, _msgId, _x )                               \
   QPROF_H_MSGID_VAR( _qtf,                                             \
                      _msgId, _x )

#define QPROF_H_MSGID_VAR( _qtf, _msgId, _x )                                       \
   QTRACE_H_MSGID_INIT_BASIC( _qtf, _msgId, _x );                                   \
   QuickTrace::BlockTimer qtvar( bt )( ( _qtf ), _msgId )

// Base macro for profiling (including self profiling) without adding
// a trace in the log
#define QPROF_H_S( _qtf, _x )                                             \
   QPROF_H_S_VAR( _qtf, qtvar(msgid), _x )

#define QPROF_H_S_VAR( _qtf, _msgId, _x )                                 \
   static QuickTrace::MsgId _msgId;                                     \
   QPROF_H_S_MSGID( _qtf, _msgId, _x )

#define QPROF_H_S_MSGID( _qtf, _msgId, _x )                               \
   QPROF_H_S_MSGID_VAR( _qtf,                                             \
                      _msgId, _x )

#define QPROF_H_S_MSGID_VAR( _qtf, _msgId, _x )                                     \
   QTRACE_H_MSGID_INIT_BASIC( _qtf, _msgId, _x );                                   \
   QuickTrace::BlockTimerSelf qtvar( bt )( ( _qtf ), _msgId )

#define QTRACE0(_fixed,_dynamic) \
        QTRACE_H( QuickTrace::theTraceFile, 0, _fixed, _dynamic )
#define QTRACE1(_fixed,_dynamic) \
        QTRACE_H( QuickTrace::theTraceFile, 1, _fixed, _dynamic )
#define QTRACE2(_fixed,_dynamic) \
        QTRACE_H( QuickTrace::theTraceFile, 2, _fixed, _dynamic )
#define QTRACE3(_fixed,_dynamic) \
        QTRACE_H( QuickTrace::theTraceFile, 3, _fixed, _dynamic )
#define QTRACE4(_fixed,_dynamic) \
        QTRACE_H( QuickTrace::theTraceFile, 4, _fixed, _dynamic )
#define QTRACE5(_fixed,_dynamic) \
        QTRACE_H( QuickTrace::theTraceFile, 5, _fixed, _dynamic )
#define QTRACE6(_fixed,_dynamic) \
        QTRACE_H( QuickTrace::theTraceFile, 6, _fixed, _dynamic )
#define QTRACE7(_fixed,_dynamic) \
        QTRACE_H( QuickTrace::theTraceFile, 7, _fixed, _dynamic )
#define QTRACE8(_fixed,_dynamic) \
        QTRACE_H( QuickTrace::theTraceFile, 8, _fixed, _dynamic )
#define QTRACE9(_fixed,_dynamic) \
        QTRACE_H( QuickTrace::theTraceFile, 9, _fixed, _dynamic )

#define QTRACE0_F(hdl,_fixed,_dynamic) \
        QTRACE_H( (hdl)->getFile(), 0, _fixed, _dynamic )
#define QTRACE1_F(hdl,_fixed,_dynamic) \
        QTRACE_H( (hdl)->getFile(), 1, _fixed, _dynamic )
#define QTRACE2_F(hdl,_fixed,_dynamic) \
        QTRACE_H( (hdl)->getFile(), 2, _fixed, _dynamic )
#define QTRACE3_F(hdl,_fixed,_dynamic) \
        QTRACE_H( (hdl)->getFile(), 3, _fixed, _dynamic )
#define QTRACE4_F(hdl,_fixed,_dynamic) \
        QTRACE_H( (hdl)->getFile(), 4, _fixed, _dynamic )
#define QTRACE5_F(hdl,_fixed,_dynamic) \
        QTRACE_H( (hdl)->getFile(), 5, _fixed, _dynamic )
#define QTRACE6_F(hdl,_fixed,_dynamic) \
        QTRACE_H( (hdl)->getFile(), 6, _fixed, _dynamic )
#define QTRACE7_F(hdl,_fixed,_dynamic) \
        QTRACE_H( (hdl)->getFile(), 7, _fixed, _dynamic )
#define QTRACE8_F(hdl,_fixed,_dynamic) \
        QTRACE_H( (hdl)->getFile(), 8, _fixed, _dynamic )
#define QTRACE9_F(hdl,_fixed,_dynamic) \
        QTRACE_H( (hdl)->getFile(), 9, _fixed, _dynamic )

#define QASSERT( _cond, _fixed, _dynamic ) \
   if( unlikely( !( _cond ) ) ) {          \
      QTRACE0( _fixed, _dynamic );         \
      assert( _cond );                     \
   }

#define QVAR QuickTrace::Varg()
#define QHEX QuickTrace::HexVarg()
#define QNULL QuickTrace::QNull()

// 10 QPROF statements in a tight loop take about 120 cycles each, or
// about 40-70 ns to run.  The generated code is about 454 bytes if
// not inlined With inlined macros, it is about 651 bytes or.  With
// -Os, we get 412 bytes.  With plain old -S, we get 337
// bytes each, and about 100 cycles or 51 ns per call.
#define QPROF0(_fixed,_dynamic) \
        QTPROF_H( QuickTrace::theTraceFile, 0, _fixed, _dynamic )
#define QPROF1(_fixed,_dynamic) \
        QTPROF_H( QuickTrace::theTraceFile, 1, _fixed, _dynamic )
#define QPROF2(_fixed,_dynamic) \
        QTPROF_H( QuickTrace::theTraceFile, 2, _fixed, _dynamic )
#define QPROF3(_fixed,_dynamic) \
        QTPROF_H( QuickTrace::theTraceFile, 3, _fixed, _dynamic )
#define QPROF4(_fixed,_dynamic) \
        QTPROF_H( QuickTrace::theTraceFile, 4, _fixed, _dynamic )
#define QPROF5(_fixed,_dynamic) \
        QTPROF_H( QuickTrace::theTraceFile, 5, _fixed, _dynamic )
#define QPROF6(_fixed,_dynamic) \
        QTPROF_H( QuickTrace::theTraceFile, 6, _fixed, _dynamic )
#define QPROF7(_fixed,_dynamic) \
        QTPROF_H( QuickTrace::theTraceFile, 7, _fixed, _dynamic )
#define QPROF8(_fixed,_dynamic) \
        QTPROF_H( QuickTrace::theTraceFile, 8, _fixed, _dynamic )
#define QPROF9(_fixed,_dynamic) \
        QTPROF_H( QuickTrace::theTraceFile, 9, _fixed, _dynamic )

#define QPROF0_F(hdl,_fixed,_dynamic) \
        QTPROF_H( (hdl)->getFile(), 0, _fixed, _dynamic )
#define QPROF1_F(hdl,_fixed,_dynamic) \
        QTPROF_H( (hdl)->getFile(), 1, _fixed, _dynamic )
#define QPROF2_F(hdl,_fixed,_dynamic) \
        QTPROF_H( (hdl)->getFile(), 2, _fixed, _dynamic )
#define QPROF3_F(hdl,_fixed,_dynamic) \
        QTPROF_H( (hdl)->getFile(), 3, _fixed, _dynamic )
#define QPROF4_F(hdl,_fixed,_dynamic) \
        QTPROF_H( (hdl)->getFile(), 4, _fixed, _dynamic )
#define QPROF5_F(hdl,_fixed,_dynamic) \
        QTPROF_H( (hdl)->getFile(), 5, _fixed, _dynamic )
#define QPROF6_F(hdl,_fixed,_dynamic) \
        QTPROF_H( (hdl)->getFile(), 6, _fixed, _dynamic )
#define QPROF7_F(hdl,_fixed,_dynamic) \
        QTPROF_H( (hdl)->getFile(), 7, _fixed, _dynamic )
#define QPROF8_F(hdl,_fixed,_dynamic) \
        QTPROF_H( (hdl)->getFile(), 8, _fixed, _dynamic )
#define QPROF9_F(hdl,_fixed,_dynamic) \
        QTPROF_H( (hdl)->getFile(), 9, _fixed, _dynamic )

#define QPROF0_S( _fixed, _dynamic ) \
        QTPROF_H_S( QuickTrace::theTraceFile, 0, _fixed, _dynamic )
#define QPROF1_S( _fixed, _dynamic ) \
        QTPROF_H_S( QuickTrace::theTraceFile, 1, _fixed, _dynamic )
#define QPROF2_S( _fixed, _dynamic)  \
        QTPROF_H_S( QuickTrace::theTraceFile, 2, _fixed, _dynamic )
#define QPROF3_S( _fixed, _dynamic ) \
        QTPROF_H_S( QuickTrace::theTraceFile, 3, _fixed, _dynamic )
#define QPROF4_S( _fixed, _dynamic ) \
        QTPROF_H_S( QuickTrace::theTraceFile, 4, _fixed, _dynamic )
#define QPROF5_S( _fixed, _dynamic ) \
        QTPROF_H_S( QuickTrace::theTraceFile, 5, _fixed, _dynamic )
#define QPROF6_S( _fixed, _dynamic ) \
        QTPROF_H_S( QuickTrace::theTraceFile, 6, _fixed, _dynamic )
#define QPROF7_S( _fixed, _dynamic ) \
        QTPROF_H_S( QuickTrace::theTraceFile, 7, _fixed, _dynamic )
#define QPROF8_S( _fixed, _dynamic ) \
        QTPROF_H_S( QuickTrace::theTraceFile, 8, _fixed, _dynamic )
#define QPROF9_S( _fixed, _dynamic)  \
        QTPROF_H_S( QuickTrace::theTraceFile, 9, _fixed, _dynamic )

#define QPROF0_F_S( hdl, _fixed, _dynamic ) \
        QTPROF_H_S( (hdl)->getFile(), 0, _fixed, _dynamic )
#define QPROF1_F_S( hdl, _fixed, _dynamic) \
        QTPROF_H_S( (hdl)->getFile(), 1, _fixed, _dynamic )
#define QPROF2_F_S( hdl, _fixed, _dynamic ) \
        QTPROF_H_S( (hdl)->getFile(), 2, _fixed, _dynamic )
#define QPROF3_F_S( hdl, _fixed, _dynamic ) \
        QTPROF_H_S( (hdl)->getFile(), 3, _fixed, _dynamic )
#define QPROF4_F_S( hdl, _fixed, _dynamic ) \
        QTPROF_H_S( (hdl)->getFile(), 4, _fixed, _dynamic )
#define QPROF5_F_S( hdl, _fixed, _dynamic ) \
        QTPROF_H_S( (hdl)->getFile(), 5, _fixed, _dynamic )
#define QPROF6_F_S( hdl, _fixed, _dynamic ) \
        QTPROF_H_S( (hdl)->getFile(), 6, _fixed, _dynamic )
#define QPROF7_F_S( hdl, _fixed, _dynamic ) \
        QTPROF_H_S( (hdl)->getFile(), 7, _fixed, _dynamic )
#define QPROF8_F_S( hdl, _fixed, _dynamic ) \
        QTPROF_H_S( (hdl)->getFile(), 8, _fixed, _dynamic )
#define QPROF9_F_S( hdl, _fixed, _dynamic ) \
        QTPROF_H_S( (hdl)->getFile(), 9, _fixed, _dynamic )



// 374 bytes per call with -O2, 280 if compiled -Os
#define QPROF(_fixed) QPROF_H( QuickTrace::theTraceFile, _fixed )
#define QPROF_F(hdl,_fixed) QPROF_H( (hdl)->getFile(), _fixed )
#define QPROF_S( _fixed ) QPROF_H_S( QuickTrace::theTraceFile, _fixed )
#define QPROF_F_S( hdl, _fixed ) QPROF_H_S( (hdl)->getFile(), _fixed )

// If you get this compiler error:
// 
// ... error: ‘QPROF_is_not_a_single_line_statement_see_QuickTrace_h’ 
//     was not declared in this scope
// 
// it is because QPROF, QPROF0,... are not single-line statements and
// cannot and should not be used in code that requires a single-line
// statement, like this:
//
//    if( test )
//      QPROF( "here I am" );  // This is a bug!
//
// It's meaningless to use QPROF like this anyway because the whole
// point of QPROF is to profile The statements that come after it in
// the same basic block.  If the QPROF statement is standalone in its
// own basic block, then all you're doing is measuring is two
// back-to-back calls to rdtsc().
// 
// In this situation you typically want QTRACE instead.


// The motivation for the QPROF_EVT macros was to record performance data about
// items that are tracked across multiple events (e.g. handleTimer expirations).
//
// The QPROF_EVT macros are an alternative to the normal QPROF macros that allow
// for profiling of things that are not driven by frames being pushed/popped from
// the stack. They do NOT use the BlockTimer objects to recorded the updates to
// the QuickTrace file. Instead the caller provides a "symbol" that is recorded
// as the "last called time", the delta time is calculated, and the hit count is
// incremented. The startTime should be set with rdtsc().
//
// Here's a pseudo-code example of usage:
//
//    // these macros declare static variables, and should be in file scope if
//    // other QPROF_EVT macros are used from multiple functions
//    QPROF_EVT_INIT( startTime, "Foo event time" );
//    QPROF_EVT_INIT( processTime, "Foo processing time" );
//
//    void Foo:handleTimer() {
//       if ( startEvent() ) {
//          QPROF_EVT_START( startTime );
//          QPROF_EVT_START( proceessTime );
//       }
//
//       QPROF_EVT_RESUME( processTime );
//       // more Foo "processTime" work goes here
//       QPROF_EVT_PAUSE( processTime );
//
//       if ( endEvent() ) {
//          QPROF_EVT_STOP( startTime );
//          QPROF_EVT_STOP( processTime );
//       } else {
//          rescheduleTimer( notDoneYetTime() );
//       }
//    }
//

// NOTE: cannot use "normal" qtvar(), since it includes line # and we need access
//       to the same variables from several macros (on different lines).
#define qtVarStart( _msgId ) qtvar__( _msgId, Start )
#define qtVarRestart( _msgId ) qtvar__( _msgId, Restart )
#define qtVarSum( _msgId ) qtvar__( _msgId, Sum )
#define qtVarText( _msgId ) qtvar__( _msgId, Text )
#define qtVarId( _msgId ) qtvar__( _msgId, Id )

// This macro declares/initializes static variables. It should be used in the file
// scope if other QPROF_EVT macros are used from multiple functions.
#define QPROF_EVT_INIT( _msgId, _msgText ) \
   static QuickTrace::MsgId qtVarId( _msgId );                                  \
   static uint64_t qtVarStart( _msgId ) = 0;                                    \
   static uint64_t qtVarSum( _msgId ) = 0;                                      \
   static uint64_t qtVarRestart( _msgId ) = 0;                                  \
   static const char *qtVarText( _msgId ) = _msgText;

#define QPROF_EVT_START( _msgId ) \
   QPROF_EVT_START_VALUE( qtVarStart( _msgId ), qtVarRestart( _msgId ), \
                          qtVarSum( _msgId ) )

#define QPROF_EVT_START_VALUE( _startVar, _restartVar, _sumVar ) \
   _startVar = rdtsc(); \
   _restartVar = _sumVar = 0;

#define QPROF_EVT_MAYBE_START( _msgId, _startValue ) \
   QPROF_EVT_MAYBE_START_VALUE( qtVarStart( _msgId ), qtVarRestart( _msgId ), \
                                qtVarSum( _msgId ), _startValue )

#define QPROF_EVT_MAYBE_START_VALUE( _startVar, _restartVar, _sumVar, _startValue ) \
   if ( _startVar == 0 ) {                                                          \
      _startVar = _startValue;                                                      \
      _restartVar = _sumVar = 0;                                                    \
   }

#define QPROF_EVT_PAUSE( _msgId ) \
   QPROF_EVT_PAUSE_VALUE( qtVarRestart( _msgId ), qtVarSum( _msgId ) )

#define QPROF_EVT_PAUSE_VALUE( _restartVar, _sumVar ) \
   if ( _restartVar != 0 ) {                                                    \
      _sumVar += rdtsc() - _restartVar;                                         \
      _restartVar = 0;                                                          \
   }

#define QPROF_EVT_RESUME( _msgId ) \
   QPROF_EVT_RESUME_VALUE( qtVarRestart( _msgId ) )

#define QPROF_EVT_RESUME_VALUE( _restartVar ) \
   _restartVar = rdtsc();

#define QPROF_EVT_RESET( _msgId ) \
   QPROF_EVT_RESET_VALUE( qtVarStart( _msgId ), qtVarRestart( _msgId ), \
                          qtVarSum( _msgId ) )

#define QPROF_EVT_RESET_VALUE( _startVar, _restartVar, _sumVar ) \
   _startVar = _restartVar = _sumVar = 0;

#define QPROF_EVT_STOP( _msgId ) \
   QPROF_EVT_STOP_VALUE( qtVarId( _msgId ), qtVarStart( _msgId ), \
                         qtVarRestart( _msgId ), qtVarSum( _msgId ), \
                         qtVarText( _msgId ) )

#define QPROF_EVT_STOP_VALUE( _idVar, _startVar, _restartVar, _sumVar, _textVar ) \
   QPROF_EVT_PAUSE_VALUE( _restartVar, _sumVar );                                 \
   uint64_t qtvar( diff ) = ( _sumVar != 0 ? _sumVar : rdtsc() - _startVar );     \
   QPROF_EVT_REPORT( QuickTrace::theTraceFile, _idVar, _textVar, _startVar, \
                     qtvar( diff ) )                                         \
   QPROF_EVT_RESET_VALUE( _startVar, _restartVar, _sumVar )

#define QPROF_EVT_REPORT( _qtf, _idVar, _text, _startTime, _diff ) \
   if ( likely( !!_qtf ) ) {                                                    \
      QTRACE_H_MSGID_INIT_BASIC( _qtf, _idVar, _text );                         \
      QuickTrace::MsgCounter *qtmc = _qtf->msgCounter( _idVar );                \
      qtmc->lastTsc = ( _startTime >> 28 ) | ( qtmc->lastTsc & 0x80000000 );    \
      qtmc->tscCount += _diff;                                                  \
      qtmc->count++;                                                            \
   }

#else
#include <QuickTrace/Qtc.h>
#endif
#endif // QUICKTRACE_QUICKTRACE_H
