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

#include <QuickTrace/QuickTrace.h>
#include <QuickTrace/Registration.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <cassert>
#include <climits>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctype.h>
#include <cstring>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <fstream>
#include <unordered_set>
#ifdef QT_USE_ATFORK_LIB
#include <AtFork/AtFork.h>
#else
#include <pthread.h>
#endif

// A QuickTrace file has the following format:
// --------------------------------
// - TraceFileHeader (starts with file version)
// - 512 MsgCounters by default (can be configured via initialize)
// - 10 circular RingBufs for the different trace levels
// - The QuickTraceDict, variable size, containing a sequence of MsgDescs.
//   Each one looks like this:
//     "<timestamp> __FILE__ __LINE__ " (ascii)
//     "<message-id> " (when file version >= 3)
//     4 byte length of message string
//     null-terminated format string like this:
//     "This is the message my int param is %d my other params are %s and %s"
//     byte length of key string
//     deserialization key, of the form "ddd" for a message with 3 integer params

namespace QuickTrace {

TraceHandle * defaultQuickTraceHandle;
static std::mutex traceHandleMutex = {};
static std::unordered_map< std::string, QuickTrace::TraceHandle * > traceHandleMap;

thread_local char QtString::formatBuf_[ QtString::formatBufSize ] = {};

thread_local std::vector< uint64_t > threadSubFuncStack = { 0 };

static constexpr uint32_t DEFAULT_MAX_SIZE = 32 * 1024;

#ifndef SMALL_MEMORY_MAX_SIZE
// Limit total ring-buffers to 512KB max on small-memory systems
static constexpr uint32_t SMALL_MEMORY_MAX_SIZE = 512;
#endif

#ifndef SMALL_SYSTEM_MEM_SIZE
// Small systems have at most 4G RAM, specify this in KB.
static constexpr uint64_t SMALL_SYSTEM_MEM_SIZE = 0x100000000ULL / 1024;
#endif

// Touch every page in some file by writing one byte of zero to it and
// return errno if we failed, 0 otherwise.
int
touchEveryPage( int fd, int sz ) noexcept {
   int data = 0;
   const int pagesize = 4 * 1024;
   for( int i = 0; i < sz; i += pagesize ) {
      lseek( fd, i, SEEK_SET );
      int err = write( fd, &data, 1 );
      if( err <= 0 )  return errno;
   }
   lseek( fd, sz-1, SEEK_SET );
   int err = write( fd, &data, 1 );
   return (err <= 0) ? errno : 0;
}

static int getfile( char const * filename, int sz ) noexcept {
   int fd = open( filename, O_RDWR|O_CREAT, 0666 );
   if( fd < 0 ) {
      std::cerr << "open " << filename << ": "
                << errno << " " << strerror( errno ) << std::endl;
      return -1;
   }
   
   // We leave a little buffer between the end of the last Ring Buffer
   // and the start of the first message descriptor ftruncate to 0
   // first gives me clean pages, so if there's some Ring Buffer I'm
   // not using then it's backed by holes in the file that are not
   // actually allocated.  I think.
   int err = ftruncate( fd, 0 );
   if( err < 0 ) {
      std::cerr << "Failed to truncate quicktrace file " << filename << " " << errno
                << ":" << strerror( errno ) << std::endl;
   }
   int realSize = sz+TraceFile::FileTrailerSize;
   err = ftruncate( fd, realSize );
   if( err < 0 ) {
      std::cerr << "Failed to truncate quicktrace file " << filename << " " << errno
                << ":" << strerror( errno ) << std::endl;
   }

   // If the filesystem is full, then touchEveryPage will fail.
   // Touching every page up front ensures that we don't get a sigbus
   // later when we attempt to allocate memory for the page.
   err = touchEveryPage( fd, realSize );
   if( err ) {
      std::cerr << "Filesystem full touchEveryPage failed" << std::endl;
      int rc = ::close( fd );
      assert( rc == 0 );
      unlink( filename );
      return -1;
   }

   lseek( fd, realSize, SEEK_SET );
   return fd;
}

void MsgFormatString::put( char const * ss ) noexcept {
   if( ptr_ != key_ ) { *ptr_++ = ','; }
   ptr_ = (char*)memccpy( ptr_, ss, 0, key_ + keySize - ptr_ );
   ptr_--;
}

MsgDesc::MsgDesc( TraceFile * tf, MsgId *msgIdPtr,
                  char const * file, int line ) noexcept
      : tf_( tf ),
        buf_( tf->msgDescBuf_ ),
        ptr_( tf->msgDescBuf_ ),
        end_( tf->msgDescBuf_ + MsgDesc::bufSize ),
        formatString_( tf->msgFormatStringKey_ ) {
   // Message IDs are shared across threads so another thread may have
   // already allocated one for this message.
   if( 0 == *msgIdPtr ) {
      tf->traceHandle_->allocateMsgId( msgIdPtr );
   }
   id_ = *msgIdPtr;

   // Store the message Id, timestamp, file, and line
   int n = sprintf( ptr_, "%" PRId64 " %s %d %d ", rdtsc(), file, line, id_ );
   ptr_ += n;

   // save room to store the length
   pstr_ = (PStringL*)ptr_;
   ptr_ = pstr_->data;
}

void
MsgDesc::finish() noexcept {
   // null-terminate the message string
   *ptr_++ = '\0';

   // backfill the length pointer so the message string is prepended
   // by a 4 byte length value.  The length includes the null terminator
   // (maybe it should not)
   pstr_->len = ptr_ - pstr_->data;

   PStringL * p = (PStringL*)ptr_;
   ptr_ = p->data;
   ptr_ = (char*)memccpy( ptr_, formatString_.key(), 0, end_-ptr_);
   assert( ptr_ );              // include null terminator on format
   p->len = ptr_ - p->data;

   // Write it all into the file
   int fd = tf_->fd();
   int n = write( fd, buf_, ptr_ - buf_ );
   // int where = lseek( fd, 0, SEEK_CUR );
   if( n <= 0 ) {
      static bool once;
      if( !once ) {
         std::cerr << "QuickTrace failed to create message(" << errno << "): "
                   << strerror( errno ) << std::endl;
         once = 1;
      }
   }
   tf_->msgIdInitializedIs( id_ );
}

MsgDesc &
MsgDesc::operator<<( const std::string_view & x ) noexcept {
   assert( ( x.size() < static_cast< size_t >( end_ - ptr_ ) ) &&
           "format string too long" );
   auto lenToCopy = x.size();
   ptr_ = ( char * )memcpy( ptr_, x.data(), lenToCopy );
   ptr_ += lenToCopy;
   return *this;
}

MsgDesc & 
MsgDesc::operator<<( char const * x ) noexcept {
   ptr_ = (char*) memccpy( ptr_, x, 0, end_ - ptr_ );
   ptr_--;
   return *this;
}

MsgDesc & 
MsgDesc::operator<<( Varg x ) noexcept {
   ptr_[0] = '%';
   ptr_[1] = 's';
   ptr_+=2;
   return *this;
}

MsgDesc & 
MsgDesc::operator<<( HexVarg x ) noexcept {
   ptr_[0] = '%';
   ptr_[1] = 'x';
   ptr_+=2;
   return *this;
}

static double monotime() noexcept {
   struct timespec ts;
   clock_gettime( CLOCK_MONOTONIC, &ts );
   return (double)ts.tv_sec + ts.tv_nsec / (double)1000000000;
}

static double utc() noexcept {
   struct timeval tv;
   gettimeofday( &tv, 0 );
   // in reality, gettimeofday never fails, so I'm not checking the
   // error code.
   return tv.tv_sec + 0.000001 * tv.tv_usec;
}

int qtMaxStringLen = 24;

static SizeSpec defaultTraceFileSizes = { 8,8,8,8,8,8,8,8,8,8 }; // in Kilobytes

void saveOldFiles( char const * path ) noexcept {
   char old[256];
   char oldest[256];
   int old_len = snprintf( old, sizeof(old), "%s.1.gz", path );
   int oldest_len = snprintf( oldest, sizeof(oldest), "%s.2.gz", path );

   bool old_is_gz = access( old, F_OK ) == 0; // Is the old file (.1) compressed ?

   // Remove .gz extension
   if( !old_is_gz ) {
      old[ old_len - 3 ] = '\0';
   }

   if( access( oldest, F_OK ) == -1 ) {
      // The oldest file (.2) is not compressed
      oldest[ oldest_len - 3 ] = '\0';

      if( access( oldest, F_OK ) == -1 ) {
         // The oldest file (.2) does not exist
         if( old_is_gz ) {
            oldest[ oldest_len - 3 ] = '.';
         }

         // We rename the old (.1) to the oldest (.2) only if the last doesn't exist
         // to keep the first log file if the agent restarts many times.
         // (Please refer to the README, Section 2.5: Turning QuickTrace on)
         rename( old, oldest ); // may get ENOENT, but we don't care        
      }
   }

   if( old_is_gz ) {
      // Remove the old compressed file (.1.gz)
      unlink( old ); // may get ENOENT, but we don't care
      old[ old_len - 3 ] = '\0';
   }

   rename( path, old );         // may get ENOENT, but we don't care
}


void
TraceFile::maybeBackupBuffer() noexcept {
   if( !traceHandle_->foreverLog() ) {
      return;
   }

   const std::string &foreverLogPath = traceHandle_->foreverLogPath();
   if( foreverLogPath.empty() ) {
      return;
   }

   if( foreverLogPath.find( "null" ) != std::string::npos ) {
      std::cerr << "EventMon Buffer Full.  Backups Disabled. Rolling Over";
      return;
   }
   std::cerr << "BACKING UP BUFFER to " << foreverLogPath << " " <<
      traceHandle_->foreverLogIndex() << std::endl;
   char newPath[256];
   snprintf( newPath, sizeof( newPath ), "%s.%d", foreverLogPath.c_str(),
             traceHandle_->foreverLogIndex() );

   // copy the file to new location
   errno = 0;
   int srcFd = open( fileName_.c_str(), O_RDONLY, 0 );
   if ( srcFd == -1 ) {
     // failed to open file - log error and continue
      std::cerr << __PRETTY_FUNCTION__ <<  "open failed for " << 
         fileName_.c_str() << " error " << strerror( errno ) << std::endl;
      return;
   }

   // determine size of source file
   struct stat statSrc;
   if ( fstat( srcFd, &statSrc ) == -1 ) {
      // failed to determine file status - clean up, log error, and continue
      ::close( srcFd );
      std::cerr << __PRETTY_FUNCTION__ << "fstat failed for " << fileName_.c_str() <<
         " error " << strerror( errno ) << std::endl;
      return;
   }

   // get file descriptor for destination file - create if doesn't exist, open as
   // write-only
   int destFd = open( newPath, O_WRONLY | O_CREAT, 0644 );
   if ( destFd == -1 ) {
      // failed to open file - clean up, log error, and continue
      ::close( srcFd );
      std::cerr << __PRETTY_FUNCTION__ <<  "open failed for " << 
         newPath << " error " << strerror( errno ) << std::endl;
      return;
   }

   // copy source to destination
   if ( sendfile( destFd, srcFd, 0, statSrc.st_size ) == -1 ) {
      // failed to copy file - throw exception
      ::close( srcFd );
      ::close( destFd );
      // log error and give up
      std::cerr << __PRETTY_FUNCTION__ <<  " sendfile failed for " <<
              fileName_.c_str() << " error " << strerror( errno ) << std::endl;
      return;
   }
   ::close( srcFd );
   ::close( destFd );

   traceHandle_->foreverLogIndexInc( 1 );
}


bool
initialize( char const * filename, SizeSpec * sizesInKilobytes, 
            char const *foreverLogPath, int foreverLogIndex,
            int maxStringLen, MultiThreading multiThreading,
            bool rotateLogFile, uint32_t numMsgCounters ) noexcept {
   if( !filename || filename[0] == '\0' ) return false;

   // We don't expect the filename to be just .qt in single thread case
   // In multi-threaded scenario the qt file name comes from the thread name
   if ( multiThreading != MultiThreading::enabled ) {
      assert( "Filename cannot be .qt" && strcmp( filename, ".qt" ) );
   }

   // The maximum length of a string in the dynamic portion of QuickTrace.
   // Longer strings will be truncated to fit this limit. 80 was picked as the 
   // limit because it struck a balance between avoiding the max buffer size of
   // 256 while allowing longer strings (i.e. pathnames) to be stored.
   if( maxStringLen > 80 ) {
      qtMaxStringLen = 80;
   } else {
      qtMaxStringLen = maxStringLen;
   }

   // Even if trace file is already created, we should update the
   // forever log parameters
   if( NULL != defaultQuickTraceHandle ) {
      if( defaultQuickTraceHandle->fileNameFormat() != filename ) {
         std::cerr << "Tracing has already been enabled to " <<
            defaultQuickTraceHandle->fileNameFormat() << std::endl;
         return false;
      }
   } else {
      defaultQuickTraceHandle = new TraceHandle( filename,
                                                 sizesInKilobytes,
                                                 foreverLogPath,
                                                 foreverLogIndex,
                                                 multiThreading,
                                                 rotateLogFile,
                                                 numMsgCounters,
                                                 true );
      if( !defaultQuickTraceHandle->isInitialized() ) {
         close();
         return false;
      }
   }
   return true;
}

void
close() noexcept {
   delete defaultQuickTraceHandle;
   // The TraceHandle desctructor sets the defaultQuickTraceHandle
   // pointer to NULL
   assert( NULL == defaultQuickTraceHandle );
}

TraceHandle *initialize_handle( char const *filename, SizeSpec *sizesInKilobytes,
                                char const *foreverLogPath, int foreverLogIndex,
                                MultiThreading multiThreading ) noexcept {
   TraceHandle *th = new TraceHandle( filename, sizesInKilobytes, foreverLogPath,
                                      foreverLogIndex, multiThreading );
   // We could check if isInitialized() is false and delete the
   // TraceHandle returning NULL but there are QuickTrace users
   // (PolicyMap...) that assume they always get back a non-NULL
   // handle.
   return th;
}

bool enableContextSwitchTracking();

uint32_t addAndLimitSizes( SizeSpec * sizeSpec, bool limit ) noexcept {
   uint32_t sz = 0;
   for( int i = 0; i < 10; ++i ) {
      uint32_t szi = sizeSpec->sz[i];
      if( szi > 16384 && limit ) {
         szi = 16384;
      } else if( szi == 0 ) {
         szi = 1;
      }
      sz += szi;
      sizeSpec->sz[i] = szi;
   }
   return sz;
}

uint32_t addLevelSizes( SizeSpec * sizeSpec, int end ) noexcept {
   uint32_t sum = 0;
   for( int i = 0; i < end; i++ ) {
      sum += sizeSpec->sz[i];
   }
   return sum;
}

uint32_t scaleDownSizes( SizeSpec * sizeSpec, float factor ) noexcept {
   // Trims each trace level by the same percentage
   // Except if the resulting size is < 1. Instead 1 is assigned.
   uint32_t sum = 0;
   for( int i = 0; i < 10; i++ ) {
      uint32_t newSize = sizeSpec->sz[i] * factor;
      // Zero size levels are not supported
      // This could cause the total file size to be a few KB over the maximum,
      // But it shouldn't be an issue.
      sizeSpec->sz[i] = newSize < 1 ? 1 : newSize;
      sum += sizeSpec->sz[i];
   }
   return sum;
}

TraceHandle *
traceHandle( const std::string & traceFilename ) noexcept {
   std::lock_guard< std::mutex > lock( traceHandleMutex );
   auto iter = traceHandleMap.find( traceFilename );
   if ( iter == traceHandleMap.end() ) {
      return nullptr;
   }
   return iter->second;
}

#ifndef __i386__
static size_t
getPhysicalMemorySize() noexcept {
   FILE * fp = fopen( "/proc/meminfo", "r" );
   assert( fp != nullptr );

   char labelStr[ 32 ];
   char unitsStr[ 32 ];
   size_t memTotal;
   int rc = fscanf( fp, "%s %zu %s", labelStr, &memTotal, unitsStr );
   assert( rc == 3 );
   rc = fclose( fp );
   assert( rc == 0 );

   // make sure we're reading the right thing
   assert( strcmp( labelStr, "MemTotal:" ) == 0 );
   assert( memTotal != 0 );
   assert( memTotal != SIZE_MAX );

   return memTotal;
}
#endif

void
TraceHandle::adjustSizeSpec( bool limit ) noexcept{
   uint32_t sz = addAndLimitSizes( &sizeSpec_, limit );
   uint32_t maxSize = DEFAULT_MAX_SIZE;

#ifndef __i386__
   // On 64-bit small memory systems (e.g. 4GB RAM), automatically scale down
   // the maximum total QT ring-buffer size to SMALL_MEMORY_MAX_SIZE
   static size_t memorySize = getPhysicalMemorySize();
   if ( memorySize <= SMALL_SYSTEM_MEM_SIZE ) {
      maxSize = SMALL_MEMORY_MAX_SIZE;
   }
#endif

   if ( limit && sz > maxSize ) {
      sz = scaleDownSizes( &sizeSpec_, ( maxSize * 1.0 ) / sz );
   }
   sz = sz * 1024 + sizeof( TraceFileHeader ) +
        ( numMsgCounters_ * sizeof( MsgCounter ) );
   mappedTraceFileSize_ = sz;
}

bool
TraceHandle::resize( const SizeSpec &newSizeSpecInKilobytes ) noexcept{
   // make sure resize is called only on a single threaded process
   if( multiThreading_ == MultiThreading::enabled ) {
      std::cerr << "Resizing multi-thread quicktrace not supported" << std::endl;
      return false;
   }

   // check that sizespec to resize to is not the same as old sizespec
   if( sizeSpec_ == newSizeSpecInKilobytes ){
      std::cerr << "QuickTrace resize not required, identical SizeSpec" << std::endl;
      return false;
   }

   bool deleteAndCreate = ( nonMtTraceFile_ != nullptr );
   if ( deleteAndCreate ) {
      // delete existing traceFile(s) - only one for single thread
      close();
   }

   // change sizeSpec
   sizeSpec_ = newSizeSpecInKilobytes;
   adjustSizeSpec( false );

   if ( deleteAndCreate ) {
      // create new file and rotate old one
      bool logRotate = true;
      nonMtTraceFile_ = newTraceFile( logRotate, numMsgCounters_ );

      // set other parts correctly
      initialized_ = true;
      traceFilesClosed_ = false;
   }

   return true;
}

TraceFile::TraceFile( TraceHandle * traceHandle, bool rotateLogFile,
                      uint32_t numMsgCounters ) noexcept
      : traceHandle_( traceHandle ),
        numMsgCounters_( numMsgCounters ),
        buf_( 0 ),
        initialized_( false ) {
   multiThreading_ = traceHandle_->multiThreading_;
   fileName_ = traceHandle_->qtdir();
   if( fileName_ != "" ) {
      fileName_ += "/";
   }
   if( traceHandle_->multiThreading_ == MultiThreading::enabled ) {
      // Obtain the thread name that must have been previously been
      // set through pthread_setname_np() to construct a unique
      // thread-specific trace filename.
      char threadName[ 100 ];
      pthread_t myTid = pthread_self();
      int ret = pthread_getname_np( myTid, threadName, sizeof( threadName ) );
      assert( ret == 0 );
      fileName_ += threadName;
   }
   fileName_ += traceHandle_->fileNameSuffix();
   for( auto iter = traceHandle_->traceFiles_.cbegin();
        iter != traceHandle_->traceFiles_.cend(); ++iter ) {
      TraceFile * otherTraceFile = *iter;
      assert( fileName_ != otherTraceFile->fileName_ );
   }

   // Rename the old .qt file.
   if( rotateLogFile ) {
      saveOldFiles( fileName_.c_str() );
   }

   // Open file and memory map
   uint32_t mappedSize = traceHandle_->mappedTraceFileSize();
   fd_ = getfile( fileName_.c_str(), mappedSize );
   if( fd_ < 0 ) return;
   void * m = mmap( 0, mappedSize, PROT_WRITE, MAP_SHARED, fd_, 0 );
   if( m == MAP_FAILED ) {
      std::cerr << "mmap " << fileName_ << ": "
                << errno << " " << strerror( errno ) << std::endl;
      closeIfNeeded();
      return;
   }
   int r = madvise( m, mappedSize, MADV_DONTDUMP ); // Don't include qt files in core
   assert( r == 0 && "madvise() MADV_DONTDUMP failed" );
   memset( m, 0, mappedSize ); // is this necessary?
   buf_ = m;

   TraceFileHeader* sfh = (TraceFileHeader*) m;
   sfh->version = 5;
   sfh->fileSize = mappedSize;
   sfh->fileHeaderSize = 
      sizeof( TraceFileHeader ) + ( numMsgCounters_ * sizeof( MsgCounter ) );
   sfh->fileTrailerSize = FileTrailerSize;
   sfh->firstMsgOffset = sizeof( TraceFileHeader );
   sfh->logCount = NumTraceLevels;
   SizeSpec sizeSpec = traceHandle_->sizeSpec();
   sfh->logSizes = sizeSpec;

   MsgCounter * msgCounters = ( MsgCounter * )( sfh + 1 );
   char * logStart = ( ( char * )m ) + sizeof( TraceFileHeader ) + 
                     ( numMsgCounters_ * sizeof( MsgCounter ) );
   for( int i=0; i<NumTraceLevels; ++i ) {
      int levelOffset = addLevelSizes( &sizeSpec, i ) * 1024;
      log_[i].bufIs( logStart + levelOffset, sizeSpec.sz[ i ] * 1024 );
      log_[i].qtFileIs( this );
      log_[i].msgCounterIs( msgCounters );
      log_[i].numMsgCountersIs( numMsgCounters_ );
   }

   // Insert TraceFile into the set maintained by the TraceHandle
   traceHandle_->traceFiles_.insert( this );
   initialized_ = true;

   sfh->tsc0 = rdtsc();
   sfh->monotime0 = monotime();
   do {
      takeTimestamp();
   } while( sfh->monotime1 - sfh->monotime0 < 0.1 );
}

TraceFile::~TraceFile() noexcept {
   if( buf_ ){
      munmap( buf_, traceHandle_->mappedTraceFileSize() );
   }

   // Close the associated file descriptor
   closeIfNeeded();

   // Remove TraceFile from the set maintained by the TraceHandle
   traceHandle_->traceFiles_.erase( this );
}

void TraceFile::closeIfNeeded() noexcept {
   if( fd_ < 0 ) return;
   int rc = ::close( fd_ );
   assert( rc == 0 );
   fd_ = -1;
}

void
TraceFile::sync() noexcept {
   msync( buf_, traceHandle_->mappedTraceFileSize(), MS_SYNC );
}

void 
TraceFile::takeTimestamp() noexcept {
   TraceFileHeader* sfh = (TraceFileHeader*) buf_;
   uint64_t tsc = rdtsc();
   // Only take a new timestamp every 1 million cycles It's pointless
   // to go much faster and this way we guarantee that we don't spend
   // a lot of time monotime() and in gettimeofday(), which each take
   // around a microsecond.
   if( tsc - sfh->tsc1 > 1000000 ) {
      sfh->tsc1 = tsc;
      sfh->monotime1 = monotime();
      sfh->utc1 = utc();
   }
}

void
TraceFile::msgIdInitializedIs( MsgId msgId ) noexcept {
   // Record that the necessary message descriptor information for
   // this ID has been added to the trace file.
   if( msgIdInitialized_.size() <= ( size_t )msgId ) {
      msgIdInitialized_.resize( ( size_t )msgId + 32 );
   }
   msgIdInitialized_[ msgId ] = true;
}

// The wall-clock timestamp returned by gettimeofday
struct timeval
TraceFile::wallClockTimestamp() const noexcept {
   struct timeval tv;
   gettimeofday( &tv, 0 );
   return tv;
}

struct RingBufHeader {
   uint32_t tailPtr;
};

inline void
RingBuf::maybeWrap( TraceFile *tf ) noexcept {
   if( unlikely(ptr_ >= bufEnd_) ) {
      doWrap();
      qtFile_->maybeBackupBuffer();
   }
}

void
RingBuf::bufIs( void * buf, int bufSize ) noexcept {
   buf_ = (char*)buf;
   bufEnd_ = buf_ + bufSize - TrailerSize;
   // initialize trailer to 0xff's to help qttail synchronize with messages
   // that spill over into the trailer
   memset( bufEnd_, -1, TrailerSize );
   ptr_ = buf_;
   doWrap();
   // mark the buffer as empty by writing a zero tsc
   memset( ptr_, 0, sizeof( uint64_t ) );
}

RingBuf::RingBuf() noexcept {
   memset( this, 0, sizeof( *this ));
}


void
RingBuf::doWrap() noexcept {
   memset( ptr_, -1, sizeof( uint64_t ) ); //  restore the trailer's 0xff's
   RingBufHeader * hdr = (RingBufHeader*) buf_;
   hdr->tailPtr = ptr_ - buf_;  // distance to byte after end of last message
   ptr_ = (char*)(hdr+1);
   if( qtFile_ )
      qtFile_->takeTimestamp();

}

void
RingBuf::qtFileIs( TraceFile * f ) noexcept { qtFile_ = f; }

#ifndef SUPERFAST
uint64_t
RingBuf::startMsg( TraceFile *tf, MsgId id ) noexcept {
   MsgCounter * mc = msgCounter( id );
   __builtin_prefetch( mc, 1, 1 ); // This seems to make a small difference
   maybeWrap(tf);

   uint64_t tsc;
   tsc = rdtsc();

   // Don't touch the message if the 'off' bit is set
   uint32_t off = mc->lastTsc & 0x80000000;
   if( off ) {
      msgStart_ = 0;
   } else {
      msgStart_ = ptr_;
      memcpy( ptr_, &tsc, sizeof( tsc ) );
      ptr_ += sizeof( uint64_t );
      memcpy( ptr_, &id, sizeof( id ) );
      ptr_ += sizeof( id );
   }

   // Update the hit counters.  The high 4 bits of TSC take more
   // than 8.5 years to become non-zero on a processor whose TSC
   // advances at 2**32 ticks/second.  So we'll not worry about the
   // case where the last-use timer is more than 8 years ago.

   // We make sure the 32nd bit is never set by masking it unless we set it manually.
   // With a high enough tsc value, the 32nd bit can end up being true,
   // and we don't want to disable the qt message when that becomes true.
   mc->lastTsc = ( ( tsc >> 28 ) & 0x7FFFFFFF ) | off;
   mc->count++;

   return tsc;
}

void
RingBuf::endMsg() noexcept {
   if ( enabled() ) {
      *ptr_ = (char) (ptr_-msgStart_);
      ptr_++;
      memset( ptr_, 0, sizeof( uint64_t ) );
   }
}
#endif

BlockTimer::~BlockTimer() noexcept {
   if( likely( qtFile_ != 0 ) ) {
      uint64_t now = rdtsc();
      uint64_t delta = now - tsc_;
      MsgCounter * mc = qtFile_->msgCounter( msgId_ );
      mc->tscCount += delta;
      mc->count++;
   }
}

BlockTimerMsg::~BlockTimerMsg() noexcept {
   if( likely( qtFile_ != 0 ) ) {
      uint64_t now = rdtsc();
      uint64_t delta = now - tsc_;
      MsgCounter * mc = qtFile_->msgCounter( msgId_ );
      mc->tscCount += delta;
   }
}

BlockTimerSelf::BlockTimerSelf( TraceFile * sf, MsgId mid ) noexcept :
      qtFile_( sf ), msgId_( mid ), tsc_( rdtsc() ) {
   if ( qtFile_ != 0 ) {
     threadSubFuncStack.push_back( 0 );
   }
}

BlockTimerSelf::~BlockTimerSelf() noexcept {
   if( likely( qtFile_ != 0 ) ) {
      uint64_t now = rdtsc();
      uint64_t delta = now - tsc_;
      MsgCounter * mc = qtFile_->msgCounter( msgId_ );
      mc->lastTsc = ( now >> 28 ) | ( mc->lastTsc & 0x80000000 );
      mc->tscCount += delta;
      mc->count++;
      mc->tscSelfCount += delta;
      mc->tscSelfCount -= threadSubFuncStack.back();
      threadSubFuncStack.pop_back();
      threadSubFuncStack.back() += delta;
   }
}

BlockTimerSelfMsg::BlockTimerSelfMsg( TraceFile * sf, MsgId mid,
                                      uint64_t now ) noexcept :
      qtFile_( sf ), msgId_( mid ), tsc_( now ) {
   if( qtFile_ != 0 ) {
      threadSubFuncStack.push_back( 0 );
   }
}

BlockTimerSelfMsg::~BlockTimerSelfMsg() noexcept {
   if( likely( qtFile_ != 0 ) ) {
      uint64_t now = rdtsc();
      uint64_t delta = now - tsc_;
      MsgCounter * mc = qtFile_->msgCounter( msgId_ );
      mc->tscCount += delta;
      mc->tscSelfCount += delta;
      mc->tscSelfCount -= threadSubFuncStack.back();
      threadSubFuncStack.pop_back();
      threadSubFuncStack.back() += delta;
   }
}
void put( RingBuf * log,
          char const * x ) noexcept __attribute__ ( ( optimize( 3 ) ) );

void
put( RingBuf * log, char const * x ) noexcept {
   // pascal-style string: len byte followed by data
   char * ptr = ((char*) log->ptr());
   char * ptr1 = ptr + 1;
   int i;
   for( i = 0 ; i < qtMaxStringLen ; ++i ) {
      if( !x[i] ) { break; }
      ptr1[i] = x[i];
   }
   *ptr = i;
   log->ptrIs( ptr1 + i );
}

// Invoked when a thread is destroyed and the pthread TLS is being
// cleaned up.
static void
deleteTraceFile( void * tf ) noexcept {
   // Deleting the TraceFile will modify the TraceHandle as we will be
   // removing the TraceFile from the traceFiles_ set maintained in
   // the TraceHandle.
   std::lock_guard< std::mutex > lock( traceHandleMutex );
   delete ( TraceFile * )tf;
}

// Controls whether we simply close the TraceHandles on fork or if we
// actually delete them. Default is to just close to support existing
// users that attempt to use the TraceHandle in the child process.
static bool deleteTraceHandlesOnFork;

void
setDeleteTraceHandlesOnFork() noexcept {
   deleteTraceHandlesOnFork = true;
}

static void
processForkPrepare() noexcept {
   // Block TraceHandle or TraceFile creation while we are forking
   traceHandleMutex.lock();
}

static void
processForkParent() noexcept {
   // Release the lock acquired in processForkPrepare()
   traceHandleMutex.unlock();
}

static void
processForkChild() noexcept {
   // The old mutex was locked by the parent process and we cannot
   // release the lock as we have a different thread-id. Re-initalize
   // the mutex by destroying the old mutex and constructing a new one
   // in its place.
   traceHandleMutex.~mutex();
   new ( &traceHandleMutex ) std::mutex();
   // At this point we are the only thread in the new child process.
   if( deleteTraceHandlesOnFork ) {
      while ( !traceHandleMap.empty() ) {
         TraceHandle * th = traceHandleMap.begin()->second;
         delete th;
      }
   } else {
      // Closing the TraceHandle means that anyone holding onto a
      // pointer can still access it. Tracing will be prevented as the
      // user will get a NULL TraceFile when they attempt to issue a
      // trace message. The NULL TraceFile is handled by the
      // QuickTrace macros.
      for ( auto iter = traceHandleMap.begin(); iter != traceHandleMap.end();
            ++iter ) {
         TraceHandle * th = iter->second;
         th->close();
      }
   }
}

static void
registerPostForkCleanup() noexcept {
   static bool initialized;
   if( initialized ) {
      return;
   }
   initialized = true;
   // Register handlers to safely handle a fork and to cleanup
   // quicktrace on the child process after a fork.
   // We shouldn't be holding any other custom locks inside quicktrace so pick
   // a large priority value
#ifdef QT_USE_ATFORK_LIB
   registerAtForkHandler( processForkPrepare, processForkParent, processForkChild,
                          100 );
#else
   pthread_atfork( processForkPrepare, processForkParent, processForkChild );
#endif
}

TraceHandle::TraceHandle( char const * fileNameFormat,
                          SizeSpec * sizeSpec,
                          char const * foreverLogPath,
                          int foreverLogIndex,
                          MultiThreading multiThreading,
                          bool rotateLogFile,
                          uint32_t numMsgCounters,
                          bool defaultHandle ) noexcept
      : multiThreading_( multiThreading ),
        traceFilesClosed_( false ),
        numMsgCounters_( numMsgCounters ),
        nonMtTraceFile_( NULL ),
        traceFileThreadLocalKey_( invalidPthreadKey ),
        foreverLogIndex_( foreverLogIndex ),
        foreverLog_( false ),
        initialized_( false ) {
   std::lock_guard< std::mutex > lock( traceHandleMutex );
   registerPostForkCleanup();
   if( foreverLogPath ) {
      // Support for backup of thread specific files and management of
      // the index has not been added.
      assert( multiThreading_ == MultiThreading::disabled );
      foreverLogPath_ = foreverLogPath;
      foreverLog_ = true;
   }

   nextMsgId_ = 1;

   assert( NULL != fileNameFormat );
   assert( ( multiThreading_ == MultiThreading::enabled ) ||
           fileNameFormat[ 0 ] );

   // format the PID.
   fileNameFormat_ = fileNameFormat;
   char fnPid[ 256 ];
   snprintf( fnPid, sizeof( fnPid ), fileNameFormat, getpid() );
   fileNameSuffix_ = fnPid;

   auto qtd = getQtDir( multiThreading_, fileNameFormat_ );
   if ( !qtd.has_value() ) {
      std::cerr << "Error: Could not create .qt directory" << std::endl;
      return;
   } else if ( qtd.value().compare( "" ) != 0 ) {
      qtdir_ = qtd.value();
   }

   if( NULL != sizeSpec ) {
      sizeSpec_ = *sizeSpec;
   } else {
      sizeSpec_ = defaultTraceFileSizes;
   }
   adjustSizeSpec();

   if( multiThreading_  == MultiThreading::enabled ) {
      int ret = pthread_key_create( &traceFileThreadLocalKey_, deleteTraceFile );
      assert( ret == 0 );

      // Thead specific TraceFiles will be created by the client's individual
      // threads when they call maybeCreateMtTraceFile().
   } else {
      // Register the handle first, as this may adjust the sizeSpec if
      // there is existing config. We want to do this before creating
      // a trace file, to avoid needing to rotate it immediately
      registerHandle( *this, defaultHandle );

      // Create the single TraceFile in advance without waiting for
      // the first trace to be issued.
      nonMtTraceFile_ = newTraceFile( rotateLogFile, numMsgCounters_ );
      if( nonMtTraceFile_ == NULL ) {
         std::cerr << "Error: Could not create trace file" << std::endl;
         return;
      }

   }

   // Maintain a set of all the TraceHandles in the process so that we
   // can close them on fork.
   traceHandleMap[ fileNameSuffix_ ] = this;

   initialized_ = true;
}

std::optional< std::string >
TraceHandle::getQtDir( MultiThreading & multiThreading,
                       std::string & fileNameFormat ) noexcept {
   // In cases where the filename starts with / or ./ or ../ we skip the retrieving
   // QUICKTRACEDIR
   if ( ( multiThreading == MultiThreading::enabled ) ||
        ( fileNameFormat.rfind( "/", 0 ) && fileNameFormat.rfind( "./", 0 ) &&
          fileNameFormat.rfind( "../", 0 ) ) ) {
      const char * qtd{ getenv( "QUICKTRACEDIR" ) };
      if ( NULL == qtd ) {
         qtd = ".qt";
         if ( access( qtd, F_OK ) && mkdir( qtd, 0777 ) ) {
            return std::nullopt;
         }
      }
      return std::string( qtd );
   }
   return "";
}

TraceFile *
TraceHandle::maybeCreateMtTraceFile( bool rotateLogFile ) noexcept {
   assert( initialized_ );
   if ( multiThreading_ != MultiThreading::enabled ) {
      // Summary: There are scenarios where we might have a mt-mode mismatch.
      //
      // Scenario 1: nonMt agents, which have already initialized their
      // QuickTrace::defaultQuickTraceHandle (in nonMt mode), will call this method.
      // This can occur if these agents load other libraries which instantiate their
      // own QuickTraceFile instances in mt mode depending on their configurations.
      // This function is called but since the trace handle is already initialized in
      // nonMt mode, there is a mismatch.
      //
      // In scenarios like such, we believe the reasonable behavior is to return
      // the nonMt trace file.
      return nonMtTraceFile_;
   }
   TraceFile *tf = ( TraceFile * )pthread_getspecific( traceFileThreadLocalKey_ );
   if ( likely( NULL != tf ) ) {
      return tf;
   }
   // TraceFile initialization modifies TraceHandle state and also calls
   // non thread-safe APIs
   std::lock_guard< std::mutex >lock( traceHandleMutex );
   tf = newTraceFile( rotateLogFile, numMsgCounters_ );
   if ( NULL == tf ) {
      std::cerr << "Error: Could not create trace file" << std::endl;
   }
   int ret = pthread_setspecific( traceFileThreadLocalKey_, tf );
   assert( 0 == ret );
   assert( ( TraceFile * )pthread_getspecific( traceFileThreadLocalKey_ ) == tf );
   return tf;
}

TraceFile *
TraceHandle::newTraceFile( bool rotateLogFile, uint32_t numMsgCounters ) noexcept {
   TraceFile * tf = new TraceFile( this, rotateLogFile, numMsgCounters );
   if( tf->initialized() ) {
      return tf;
   }
   delete tf;
   return NULL;
}

TraceFile *
TraceHandle::mtTraceFile() noexcept {
   if( unlikely( !initialized_ ) ) {
      return NULL;
   }
   return ( TraceFile * )pthread_getspecific( traceFileThreadLocalKey_ );
}

inline void
TraceHandle::allocateMsgId( MsgId * msgIdPtr ) noexcept {
   if ( multiThreading_ == MultiThreading::enabled ) {
      msgIdAllocMutex.lock();
   } else {
      // This is not an optimisation for non-MT clients. It is a
      // workaround for BUG214186. Incorrect uses of QuickTrace
      // in signal handlers may result in re-entrancy in this
      // code. By not holding the lock we prevent the signal handler
      // from hanging and revert to the pre-MT behaviour of a corrupt
      // QuickTrace file.
   }
   if( 0 != *msgIdPtr ) {
      if ( multiThreading_ == MultiThreading::enabled ) {
         msgIdAllocMutex.unlock();
      }
      return;
   }
   if ( !nextMsgId_ ) { //overflow, skip 0 again
      nextMsgId_++;
   }
   // Allocate new ID and store return value
   *msgIdPtr = nextMsgId_++;

   if ( multiThreading_ == MultiThreading::enabled ) {
      msgIdAllocMutex.unlock();
   }
}

TraceHandle::~TraceHandle() noexcept {
   std::lock_guard< std::mutex > lock( traceHandleMutex );
   disableTracing();
   traceHandleMap.erase( fileNameSuffix_ );
   if( this == defaultQuickTraceHandle ) {
      defaultQuickTraceHandle = NULL;
   }
}

void
TraceHandle::close() noexcept {
   std::lock_guard< std::mutex > lock( traceHandleMutex );
   disableTracing();
}

void
TraceHandle::disableTracing() noexcept {
   if( traceFilesClosed_ ) {
      assert( traceFiles_.empty() );
      return;
   }
   // Prevent creation of new TraceFiles
   initialized_ = false;
   traceFilesClosed_ = true;
   nonMtTraceFile_ = NULL;

   while( !traceFiles_.empty() ) {
      TraceFile *tf = *traceFiles_.cbegin();
      delete tf;
      // TraceFile destructor removes it from the set
   }
}

bool
traceHandleExists( const std::string & filenameSuffix ) noexcept {
   std::lock_guard< std::mutex > lock( traceHandleMutex );
   return traceHandleMap.find( filenameSuffix ) != traceHandleMap.cend();
}

} // namespace QuickTrace

extern "C" void *QuickTrace_initialize( char const * filename,
                                        char const * sizeStr,
                                        int maxStringLen ) {
   int ss[10] = {8,8,8,8,8,8,8,8,8,8};
   if( sizeStr ) {
      char * s2 = strdup( sizeStr );
      char * freeMe = s2;
      char * c;
      int i = 0;
      char * saveptr = NULL;
      do {
         c = strtok_r( s2, ", ", &saveptr );
         s2 = NULL;
         if( !c ) break;
         ss[ i++ ] = atoi( c ) ?: 1;
      } while ( i < 10 );
      free( freeMe );
   }
   QuickTrace::initialize(
         filename, ( QuickTrace::SizeSpec * )ss, NULL, 0, maxStringLen );
   return QuickTrace::defaultQuickTraceHandle;
}

extern "C" void *QuickTrace_initialize_with_logging( char const * filename,
                                                     char const * sizeStr,
                                                     char const * foreverLogPath,
                                                     int foreverLogIndex,
                                                     int maxStringLen,
                                                     bool multiThreadingEnabled,
                                                     uint32_t numMsgCounters ) {
   int ss[10] = {8,8,8,8,8,8,8,8,8,8};
   if( sizeStr ) {
      char * s2 = strdup( sizeStr );
      char * freeMe = s2;
      char * c;
      int i = 0;
      char * saveptr = NULL;
      do {
         c = strtok_r( s2, ", ", &saveptr );
         s2 = NULL;
         if( !c ) break;
         ss[ i++ ] = atoi( c ) ?: 1;
      } while ( i < 10 );
      free( freeMe );
   }
   
   QuickTrace::MultiThreading multiThreading = multiThreadingEnabled ? 
      QuickTrace::MultiThreading::enabled : QuickTrace::MultiThreading::disabled;

   QuickTrace::initialize( filename, ( QuickTrace::SizeSpec * )ss,
                           foreverLogPath, foreverLogIndex,
                           maxStringLen, multiThreading,
                           true, numMsgCounters );
   return QuickTrace::defaultQuickTraceHandle;
}

// Hook to allow Golang users to create a MsgId to cache locally,
// allowing minimal (expensive) calls into Cgo
extern "C" QuickTrace::MsgId
GoQuickTrace_create_msg_id(
   void * hdl, const char * file, int line, const char * fmt, const char * msg ) {
   assert( hdl );

   auto th = ( QuickTrace::TraceHandle * )hdl;
   auto tf = th->traceFile();
   QuickTrace::MsgId msgId = 0;
   QuickTrace::MsgDesc qtmd( tf, &msgId, file, line );
   qtmd.formatString().put( fmt );
   qtmd << msg;
   qtmd.finish();

   assert( msgId != 0 );

   return msgId;
}

// Hook to allow Golang users to pass all relevant information for a
// message to C in a single call.
extern "C" void
GoQuickTrace_fullMsg( void * hdl,
                      uint64_t * tsc,
                      int id,
                      int level,
                      int numArgs,
                      uint32_t * argLens,
                      void * args ) {
   // All types are serialized into the args buffer. The argLens array
   // contains either the length (for a string), or a bit-mask
   // describing the type. If you update any of these fields, you'll
   // need to modify the associated Go mask as well
   static constexpr uint32_t MASK_SHIFT = 10;
   static constexpr uint32_t U8_MASK = 1;
   static constexpr uint32_t U16_MASK = U8_MASK + 1;
   static constexpr uint32_t U32_MASK = U16_MASK + 1;
   static constexpr uint32_t U64_MASK = U32_MASK + 1;
   static constexpr uint32_t FLOAT_MASK = U64_MASK + 1;
   static constexpr uint32_t DOUBLE_MASK = FLOAT_MASK + 1;
   static constexpr uint32_t BOOL_MASK = DOUBLE_MASK + 1;

   unsigned int offset = 0;
   uintptr_t byteBuf = reinterpret_cast< uintptr_t >( args );

   QuickTrace::TraceHandle * th = ( QuickTrace::TraceHandle * )hdl;
   QuickTrace::RingBuf & sl = ( ( th )->getFile() )->log( level );

   *tsc = sl.startMsg( th->getFile(), id );

   if ( unlikely( !sl.enabled() ) ) {
      return;
   }

   for ( auto i = 0; i < numArgs; ++i ) {
      uint32_t mask = argLens[ i ] >> MASK_SHIFT;
      auto argStart = byteBuf + offset;
      switch ( mask ) {
       case U8_MASK: {
         auto * u8Arg = reinterpret_cast< const uint8_t * >( argStart );
         put( &sl, *u8Arg );
         offset += sizeof( uint8_t );
         break;
       }
       case U16_MASK: {
         auto * u16Arg = reinterpret_cast< const uint16_t * >( argStart );
         put( &sl, *u16Arg );
         offset += sizeof( uint16_t );
         break;
       }
       case U32_MASK: {
         auto * u32Arg = reinterpret_cast< const uint32_t * >( argStart );
         put( &sl, *u32Arg );
         offset += sizeof( uint32_t );
         break;
       }
       case U64_MASK: {
         auto * u64Arg = reinterpret_cast< const uint64_t * >( argStart );
         put( &sl, *u64Arg );
         offset += sizeof( uint64_t );
         break;
       }
       case FLOAT_MASK: {
         auto * floatArg = reinterpret_cast< const float * >( argStart );
         put( &sl, *floatArg );
         offset += sizeof( float );
         break;
       }
       case DOUBLE_MASK: {
         auto * doubleArg = reinterpret_cast< const double * >( argStart );
         put( &sl, *doubleArg );
         offset += sizeof( double );
         break;
       }
       case BOOL_MASK: {
         auto * boolArg = reinterpret_cast< const bool * >( argStart );
         put( &sl, *boolArg );
         offset += sizeof( bool );
         break;
       }
       default: {
         // No mask, must be a string
         const auto * stringArg = reinterpret_cast< const char * >( argStart );
         put( &sl, stringArg );

         // Add our substring length + 1 to our offset to account for the null char
         offset += argLens[ i ];
         offset += 1;
       }
      }
   }

   sl.endMsg();
}
