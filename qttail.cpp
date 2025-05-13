// Copyright (c) 2020, Arista Networks, Inc.
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

#include <array>
#include <map>
#include <cstdarg>
#include <getopt.h>
#include <inttypes.h>
#include <poll.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <iostream>
#include <list>
#include <QuickTrace/QuickTrace.h>
#include <QuickTrace/MessageParser.h>
#include <QuickTrace/MessageFormatter.h>

// How to read a 64 bit timestamp counter. Use memcpy to prevent undefined behavior
// as the addresses are not naturally aligned.
#define LOAD_TSC( DST, SRC ) memcpy( &DST, SRC, sizeof( DST ) )

namespace {

// Give up if file version is less than that
constexpr uint32_t minimumVersionSupported = 2;

// This should be updated every time a new file version is added
// Emit a warning if a newer file version is found
constexpr uint32_t mostRecentVersionSupported = 5;

void pabort( const std::string & message ) {
   if ( errno == 0 ) {
      std::cerr << message << std::endl;
   } else {
      perror( message.c_str() );
   }
   abort();
}

void pexit( const std::string & message ) {
   if ( errno == 0 ) {
      std::cerr << message << std::endl;
   } else {
      perror( message.c_str() );
   }
   exit( EXIT_FAILURE );
}

} // namespace

namespace QuickTrace {
class Messages {
public:
   MessageFormatter * get( uint32_t msgId ) {
      auto i = messages_.find( msgId );
      if ( i == messages_.end() ) {
         parse();
         i = messages_.find( msgId );
         if ( i == messages_.end() ) {
            return nullptr;
         }
      }
      auto ret = &( *i ).second;
      return ret->msg().empty() ? nullptr : ret;
   }

   void initialize( const void * fpp, int fd ) {
      messages_.clear();
      parser_.initialize( fpp, fd );
   }

   void parse() {
      parser_.recheck();
      while ( parser_.more() ) {
         Message msg = parser_.parse();
         messages_.try_emplace( msg.msgId(),
                                msg.msgId(),
                                msg.filename(),
                                msg.lineno(),
                                msg.msg(),
                                msg.fmt() );
      };
   }

private:
  std::unordered_map< uint32_t, MessageFormatter > messages_;
  MessageParser parser_;
};

// formats timestamps in tsc ticks to human-readable time
class TimestampFormatter {
public:
   TimestampFormatter() :
         tsc0_( 0 ), ticksPerSecond_( 0 ), utc0_( 0 ) {
   }

   // format timestamp to os
   void format( uint64_t ts, std::ostream & os ) {
      double t = utc0_ + ( ts - tsc0_ ) / ticksPerSecond_;
      if ( lastT_ != static_cast< time_t >( t ) ) {
         nextSecond_ = true;
         lastT_ = static_cast< time_t >( t );
         strftime( lastTbuf_, sizeof( lastTbuf_ ), "%Y-%m-%d %H:%M:%S.",
                   localtime( &lastT_ ) );
      }
      char usec[ 8 ];
      // round to the nearest usec for compatibility with qtcat
      sprintf( usec, "%06u",
               static_cast< unsigned >( ( t - lastT_ + 0.0000005 ) * 1000000 ) );
      os << lastTbuf_ << usec;
   }

   void initialize( const QuickTrace::TraceFileHeader & hdr ) {
      tsc0_ = hdr.tsc0;
      uint64_t tscDelta = hdr.tsc1 - hdr.tsc0;
      double timeDelta = hdr.monotime1 - hdr.monotime0;
      ticksPerSecond_ = tscDelta / timeDelta;
      utc0_ = hdr.utc1 - timeDelta;
   }

   static bool nextSecond_; // time has wrapped to the next second

private:
   uint64_t tsc0_; // counter in ticks when qt file was created
   double ticksPerSecond_; // how many ticks are in one second
   double utc0_; // utc time when qt file was created
   static time_t lastT_; // last timestamp formatted (entire seconds, unix time)
   static char lastTbuf_[ 24 ]; // formatted string representation of lastT_
};

bool TimestampFormatter::nextSecond_ = false;
time_t TimestampFormatter::lastT_;
char TimestampFormatter::lastTbuf_[ 24 ];

class CorruptionError : public std::exception {
public:
   CorruptionError( Messages & messages, uint32_t msgId, const char * fmt, ... ) {
      msgId_ = msgId;
      MessageFormatter * formatter = messages.get( msgId );
      if ( formatter != nullptr ) {
         msg_ = formatter->msg();
         fmt_ = formatter->fmt();
      }
      char buf[ 256 ];
      va_list ap;
      va_start( ap, fmt );
      vsnprintf( buf, sizeof( buf ) - 1, fmt, ap );
      va_end( ap );
      buf[ sizeof( buf ) - 1 ] = '\0';
      what_ = buf;
   }

   virtual const char * what() const noexcept {
      return what_.c_str();
   }

   const std::string & fmt() const { return fmt_; }
   const std::string & msg() const { return msg_; }
   uint32_t msgId() const { return msgId_; }

private:
   std::string fmt_;
   std::string msg_;
   std::string what_;
   uint32_t msgId_;
};

struct Options {
   enum {
      LEVEL_0 = 0x01,
      LEVEL_1 = 0x02,
      LEVEL_2 = 0x04,
      LEVEL_3 = 0x08,
      LEVEL_4 = 0x10,
      LEVEL_5 = 0x20,
      LEVEL_6 = 0x40,
      LEVEL_7 = 0x80,
      LEVEL_8 = 0x100,
      LEVEL_9 = 0x200,
      LEVEL_ALL = 0x3ff,
      CHECK_LEVEL = 0x400,
      DEBUG = 0x800,
      TAIL = 0x1000,
      PRINT_TSC = 0x2000,
      PRINT_FILE_LINE = 0x4000,
      PRINT_QT_FILE_NAME = 0x8000,
      PRINT_QT_FILE_EVENTS = 0x10000,
      PRINT_WALL_CLOCK_TIME = 0x20000
   };
};

//  a ring buffer containing log messages of a particular log level
class RingBuffer {
public:
   RingBuffer() : corruption_( 0 ), level_( 0 ), start_( nullptr ), end_( nullptr ),
                  cur_( nullptr ), lastTsc_( 0 ) {}

   RingBuffer( unsigned level, const unsigned char * start,
               const unsigned char * end ) {
      corruption_ = 0;
      level_ = level;
      start_ = start + sizeof( uint32_t ); // skip the end pointer
      end_ = end - 256; // exclude the trailer
      cur_ = start_;
      lastTsc_ = 0;
   }

   // dump the current message and advance to next one
   bool dump( Messages & msgs, TimestampFormatter & tsf, uint64_t orderTsc,
              uint64_t curTsc, int options, const char * qtName ) {
      uint32_t msgId;
      uint64_t tsc = next( msgs, curTsc, options, msgId );
      if ( tsc != 0 ) {
         if ( tsc != orderTsc ) {
            // the tsc made for the ordering decision is different from the read tsc
            // which means the wrong buffer might have been selected.
            // therefore don't output anything but instead go through another round
            // of buffer selection
            corruption_++;
            return true; // pretend the message has been consumed so that the caller
                         // resets its state and re-evaluates all buffers
         }
         MessageFormatter * formatter = msgs.get( msgId );
         if ( ( options & Options::PRINT_WALL_CLOCK_TIME ) != 0 &&
              formatter->lineno() != 0 ) {
            // Message with wall-clock timestamp are using 0 as line number.
            // Skip printing other messages without wall-clock timestamp.
            // Even if we skip printing output, we still need to advance the
            // cur_ pointer in RingBuf as per the formatter data.
            // One workaround to disable output on std ostream is to set failbit
            // which indicates logical error on i/o operation.
            std::cout.setstate( std::ios::failbit );
         }
         if ( formatter->lineno() == 0 &&
              ( options & Options::PRINT_WALL_CLOCK_TIME ) != 0 ) {
            // If the line number is 0, the first two fields in RingBuf data
            // contains wall-clock timestamp (tv_sec & tv_usec).
            // If wallClock option is enabled, use those fields to replace
            // message timestamp. Otherwise ignore those fields.
            formatter->formatWallClock( cur_ + 12, std::cout );
         } else {
            tsf.format( tsc, std::cout );
         }
         std::cout << ' ' << level_;
         if ( ( options & Options::DEBUG ) != 0 ) {
            std::cout << " 0x" << std::hex << std::setfill( '0' ) << std::setw( 16 )
                      << orderTsc << std::dec;
         }
         if ( ( options & Options::PRINT_TSC ) != 0 ) {
            std::cout << " 0x" << std::hex << std::setfill( '0' ) << std::setw( 16 )
                      << tsc << std::dec;
         }
         if ( ( options & Options::PRINT_QT_FILE_NAME ) != 0 ) {
            std::cout << ' ' << qtName;
         }
         std::cout << " +" << ( tsc - lastPrintedTsc_ ) << ' ';
         if ( ( options & Options::PRINT_FILE_LINE ) != 0 ) {
            std::cout << formatter->filename() << ':' << formatter->lineno() << ' ';
         }
         std::cout << '"';
         int n = formatter->format( cur_ + 12, std::cout );
         if ( n < 0 ) {
            // corruption even after message has been validated already;
            // fail right away
            throw CorruptionError(
                     msgs, msgId, "failed to dump message after successful decode" );
         }
         std::cout << "\"\n";
         int expectedLength = cur_[ n + 12 ]; // what the writer thinks
         if ( n + 12 != expectedLength ) {
            // invalid length
            throw CorruptionError( msgs, msgId,
                                   "invalid length after dump: %d (expected: %d)",
                                   static_cast< int >( expectedLength ), n + 12 );
         }
         cur_ += 13 + n;
         if ( cur_ >= end_ ) {
            // need to wrap
            cur_ = start_;
         }
         lastTsc_ = tsc;
         corruption_ = 0;
         if ( ( options & Options::PRINT_WALL_CLOCK_TIME ) != 0 &&
              formatter->lineno() != 0 ) {
            // Clear the failbit error so that the next message gets printed.
            std::cout.clear();
         } else {
            lastPrintedTsc_ = tsc;
         }
         return true;
      } else {
         // message failed to decode, increment the corruption counter
         corruption_++;
         return false;
      }
   }

   // move to the end of the current buffer
   void fastforward( Messages & msgs, int options ) {
      while ( skip( msgs, options ) ) {}
      if ( cur_ >= end_ ) {
         // need to wrap
         cur_ = start_;
      }
   }

   // go to the start of the ring buffer (after corruption was detected)
   bool reset() {
      if ( cur_ == start_ ) {
         // already at the beginning of the ring buffer, reset not possible
         return false;
      }
      corruption_ = 0;
      cur_ = start_;
      lastTsc_ += 1; // ensure last message is not dumped again
      return true;
   }

   // go to the very first message in the buffer
   bool rewind( Messages & msgs, int options ) {
      corruption_ = corruptionThreshold_; // fail immediately if corruption is found
      uint32_t e = reinterpret_cast< const uint32_t * >( start_ )[ -1 ];
      if ( e == 0 ) {
         // the ring buffer never wrapped, there is nothing to do as the
         // current pointer is already at the start of the buffer
      } else {
         // the ring buffer did wrap, go forward and find the split point
         // i.e. the point where it emitted the last message
         uint64_t tsc;
         uint32_t msgId;
         try {
            while ( cur_ < end_ &&
                    ( tsc = next( msgs, UINT64_MAX, options, msgId ) ) != 0 ) {
               MessageFormatter * formatter = msgs.get( msgId );
               int n = formatter->length( cur_ + 12 );
               if ( n < 0 ) {
                  // failed to decode message; give up immediately
                  return false;
               }
               cur_ += 13 + n;
               lastTsc_ = tsc;
            }
         } catch ( const CorruptionError & ) {
            // found corruption; give up immediately
            return false;
         }
         const unsigned char * splitPoint = cur_ + 8;
         const unsigned char * p = start_ - sizeof( uint32_t ) + e;
         if ( p <= splitPoint ) {
            // there is no split point, just take all the messages from the
            // start of the buffer
            cur_ = start_;
         } else {
            // there is a split point, take all messages after this point
            // need to walk backwards from the end to find the first message.
            // cond should be 'p >= splitPoint' but keep 'p > splitPoint' for
            // compatibility with qtcat. because of this, qtcat might not
            // recognize the first valid message
            uint32_t movedBackNumMsgs = 0;
            while ( p > splitPoint ) {
               movedBackNumMsgs++;
               cur_ = p;
               p -= 1 + p[ -1 ];
            }
            if ( p < splitPoint && movedBackNumMsgs == 1 ) {
               // moved back one message but then found that it already went before
               // the split point. in this case,it needs to go back to the start
               // of the buffer.
               cur_ = start_;
            }
         }
      }
      // initialize lastTsc so that the tsc delta of the first message is zero
      LOAD_TSC( lastTsc_, cur_ );
      corruption_ = 0;
      return true;
   }

   // skip the current message and advance to next one
   bool skip( Messages & msgs, int options ) {
      uint32_t msgId;
      uint64_t tsc = next( msgs, UINT64_MAX, options, msgId );
      if ( tsc != 0 ) {
         MessageFormatter * formatter = msgs.get( msgId );
         int n = formatter->length( cur_ + 12 );
         if ( n < 0 ) {
            // corruption; give up immediately
            // as 'skip' is only used in tailing mode, just return to the caller and
            // let it resume tailing. If corruption persists, it will eventually
            // deal with it
            return false;
         }
         cur_ += 13 + n;
         lastTsc_ = tsc;
         return true;
      } else {
         return false;
      }
   }

   // get information about next message
   // return: 0 if there is no next message
   uint64_t next( Messages & msgs, uint64_t curTsc, int options,
                  uint32_t & msgId ) const {
      // check if valid message is there
      uint64_t tsc = nextTsc();
      if ( tsc == 0 ) {
         // zero tsc; not valid but not a corruption either
         // it is simply an indicator that there is no message yet
         return 0;
      }
      if ( !isValidTsc( tsc, curTsc ) ) {
         // not a valid timestamp
         if ( corruption_ >= corruptionThreshold_ ) {
            throw CorruptionError( msgs, 0,
                                   "invalid tsc: %" PRIu64 " (last: %" PRIu64 ")",
                                   tsc, lastTsc_ );
         }
         return 0;
      }
      memcpy( &msgId, cur_ + 8, sizeof( msgId ) );
      MessageFormatter * formatter = msgs.get( msgId );
      if ( formatter == nullptr ) {
         // not a valid message id
         if ( corruption_ >= corruptionThreshold_ ) {
            throw CorruptionError( msgs, msgId, "invalid message id: %" PRIu32,
                                   msgId );
         }
         return 0;
      }
      int length = formatter->length( cur_ + 12 );
      if ( length < 0 ) {
         // corrupt parameter data, could be temporary due to concurrent write
         if ( corruption_ >= corruptionThreshold_ ) {
            throw CorruptionError( msgs, msgId, "invalid parameter data" );
         }
         return 0;
      }
      int expectedLength = cur_[ length + 12 ]; // what the writer thinks
      if ( length + 12 != expectedLength ) {
         // mismatching length
         if ( corruption_ >= corruptionThreshold_ ) {
            throw CorruptionError( msgs, msgId, "invalid length: %d (expected: %d)",
                                   expectedLength, length + 12 );
         }
         return 0;
      }
      uint64_t nextTsc;
      LOAD_TSC( nextTsc, cur_ + 13 + length );
      if ( nextTsc != 0 && cur_ + 13 + length >= end_ ) {
         // got a non-zero next timestamp when the message extends into the trailer.
         // do additional checks
         if ( nextTsc == UINT64_MAX ) {
            // got an all-one timestamp
            if ( ( options & Options::TAIL ) != 0 ) {
               // when tailing, an all-one timestamp could be either because
               // the message is incomplete or the buffer has already wrapped.
               // therefore check if there is a new timestamp at the start of
               // the buffer to find out
               LOAD_TSC( nextTsc, start_ );
            } else {
               // in qtcat mode this means the buffer has rolled over, so just accept
               // the message (we can not check the timestamp at the start of the
               // buffer in this case as it will be from an older message)
               nextTsc = 0;
            }
         } else {
            // got a non-zero timestamp in the trailer that is not all-one
            // this could be corruption if persistent
            if ( corruption_ >= corruptionThreshold_ ) {
               throw CorruptionError( msgs, msgId, "invalid next tsc: %" PRIu64
                                      " (tsc: %" PRIu64 ")", nextTsc, tsc );
            }
            return 0;
         }
      }
      if ( nextTsc != 0 && ( nextTsc < tsc || nextTsc > curTsc ) ) {
         // not a valid next timestamp
         if ( corruption_ >= corruptionThreshold_ ) {
            throw CorruptionError( msgs, msgId, "invalid next tsc: %" PRIu64
                                   " (tsc: %" PRIu64 ")", nextTsc, tsc );
         }
         return 0;
      }
      return tsc;
   }

   // return the next tsc only, without validating if the message is complete
   // for quickly determining the next buffer to look at
   uint64_t nextTsc() const {
      // read the tsc from the current position
      uint64_t tsc;
      LOAD_TSC( tsc, cur_ );
      if ( cur_ == start_ && tsc < lastTsc_ ) {
         // ignore timestamp being less than last when at the beginning of a ring
         // buffer, because qttail could have gone there after a corruption, but
         // then it should not trace old messages, instead wait until the next
         // larger timestamp shows up
         tsc = 0;
      }
      return tsc;
   }

   bool isValidTsc( uint64_t tsc, uint64_t curTsc ) const {
      // the tsc can not go backwards or into the future. if the tsc is less than the
      // tsc of the last message, or larger than the current tsc from the CPU, then
      // it has not read the complete new tsc due to an incomplete write, or it
      // has read an old tsc from the start of the buffer.
      return tsc > 0 && tsc >= lastTsc_ && tsc < curTsc;
   }

   // number of times to try decoding the current message, if decoding still
   // fails the file is deemed corrupt
   static constexpr unsigned corruptionThreshold_ = 1024;

private:
   unsigned corruption_; // number of times the current message failed to decode
   unsigned level_; // log level
   const unsigned char * start_; // start of usable area in ring buffer
   const unsigned char * end_; // one past the end of usable area in ring buffer
   const unsigned char * cur_; // current position in ring buffer
   uint64_t lastTsc_; // timestamp of last message from this ring buffer
   static uint64_t lastPrintedTsc_; // timestamp of last printed message across
                                    // all ring buffers
};

uint64_t RingBuffer::lastPrintedTsc_ = 0;

// tails a single file
class Tail {
public:
   enum Status {
      INITIALIZING, // Initializing the file, waiting for suitable monotime* in the
                    // header
      ACTIVE, // actively tracing the file
      DELETE_PENDING, // The file was deleted but we still need to trace the
                      // remaining messages
      REINIT_PENDING, // The file was re-created due to an agent restart
                      // but the agent has not yet written anything to the file.
                      // qttail might still be consuming remaining messages from
                      // the previously mapped file.
      REINIT_READY    // The file was re-created and the agent has written something
                      // to the file, so now qttail can start tracing.
                      // qttail might still be consuming remaining messages from
                      // the previously mapped file.
                      // Also used as initial state after qttail starts. At that
                      // time there are no remaining messages to consume so it can
                      // transition to INITIALIZING immediately
   };

   // The reason for having these states is that the qt file can be deleted or
   // re-created (agent restart) while qttail is still processing messages from
   // the file.
   // File deleted: qttail will exit when all the files that is tailing have been
   //               deleted and it has consumed all messages from all of those files
   // File re-created: qttail will re-map the new file only after it has consumed
   //                  all messages from the old file
   // When re-creating a file qttail receives two filesystem notifications:
   // 1. IN_CREATE meaning the file has been created
   // 2. IN_MODIFY meaning the file has been modified
   // qttail waits for the second notification before starting to tail, because
   // the agent needs to write the qt header first, and initialize all ring buffers
   // and after the IN_CREATE event this has not happened yet.
   //
   //      (qttail starts)
   //            |                             +---------+
   //            v       qttail started        v         | monotime not usable
   //       REINIT_READY ---------------> INITIALIZING --+
   //            ^       or all messages       |
   //            |       consumed              | monotime usable
   //  IN_MODIFY |                             v
   //  event     |      +------------------- ACTIVE ---+
   //            |      | IN_CREATE                    | IN_DELETE
   //            |      | event                        | event
   //            |      v                              v
   //            +--- REINIT_PENDING <-------------- DELETE_PENDING
   //                                  IN_CREATE       | all messages consumed and
   //                                  event           | no other files to tail
   //                                                  v
   //                                            (qttail exits)

   Tail( std::string filename, bool skipToEnd, int options ) {
      fd_ = -1;
      filename_ = std::move( filename );
      options_ = options;
      next_ = std::make_pair( UINT64_MAX, -1 );
      qtname_ = filename_;
      qtname_.erase( 0, qtname_.rfind( '/' ) + 1 );
      status_ = REINIT_READY;
      initialize( true, skipToEnd );
   }

   Tail( Tail && rhs ) :
         fd_( rhs.fd_ ), filename_( std::move( rhs.filename_ ) ),
         msgs_( std::move( rhs.msgs_ ) ), options_( rhs.options_ ),
         next_( rhs.next_ ), qtname_( std::move( rhs.qtname_ ) ), rbs_( rhs.rbs_ ),
         size_( rhs.size_ ), status_( rhs.status_ ), tfh_( rhs.tfh_ ),
         tsc1_( rhs.tsc1_ ), tsf_( rhs.tsf_ ) {
      rhs.fd_ = -1;
      rhs.tfh_ = nullptr;
   }

   ~Tail() {
      if ( tfh_ != nullptr ) {
         munmap( const_cast< QuickTrace::TraceFileHeader * >( tfh_ ), size_ );
      }
      if ( fd_ >= 0 ) {
         ::close( fd_ );
      }
   }

   void cleanup() {
      if ( tfh_ != nullptr ) {
         if ( munmap( const_cast< QuickTrace::TraceFileHeader * >( tfh_ ),
                      size_ ) != 0 ) {
            pabort( "munmap" );
         }
         tfh_ = nullptr;
      }
      if ( fd_ >= 0 ) {
         ::close( fd_ );
         fd_ = -1;
      }
   }

   bool initialize( bool initial, bool skipToEnd ) {
      statusIs( INITIALIZING );
      if ( fd_ < 0 ) {
         fd_ = open( filename_.c_str(), O_RDONLY );
         if ( fd_ < 0 ) {
            if ( initial ) {
               pexit( ( "open(" + filename_ + ")" ).c_str() );
            } else {
               pabort( ( "open(" + filename_ + ")" ).c_str() );
            }
         }
         size_ = lseek( fd_, 0, SEEK_END );
         if ( size_ == -1 ) {
            pabort( "lseek" );
         } else if ( size_== 0 ) {
            std::cerr << "Empty file not supported by qttail" << std::endl;
            exit( EXIT_FAILURE );
         }
         size_ += 128 * 1024;
         const void * m = mmap( 0, size_, PROT_READ, MAP_SHARED, fd_, 0 );
         if ( m == MAP_FAILED ) {
            pabort( ( "mmap(" + filename_ + ")" ).c_str() );
         }
         tfh_ = static_cast< const QuickTrace::TraceFileHeader * >( m );
         if ( ( options_ & Options::PRINT_QT_FILE_EVENTS ) != 0 ) {
            std::cerr << "--- " << ( initial ? "" : "re-" ) << "opened " << filename_
                      << std::endl;
         }
      }
      // wait for qt file to be fully initialized
      if ( tfh_->monotime1 - tfh_->monotime0 < 0.1 ) {
         return false;
      }
      // check the file version version after ensuring that the qt file is fully
      // initialized. Otherwise QtTailReopen test fails as it detects a file version
      // of zero
      if ( tfh_->version < minimumVersionSupported ) {
         std::cerr << filename_ << " is version " << tfh_->version << ".\n"
                   << "This version of qttail only supports file versions from "
                   << minimumVersionSupported << " on.";
         errno = 0;
         pexit( "" );
      }
      if ( tfh_->version > mostRecentVersionSupported ) {
         std::cerr << filename_ << " is version " << tfh_->version << ".\n"
                   << "This version of qttail only supports up to file version "
                   << mostRecentVersionSupported << ", so the output may be "
                   << "incorrect.\nPlease use a newer version of qttail to "
                   << "ensure correct output." << std::endl;
      }
      // initialize and parse the messages
      msgs_.initialize( tfh_, fd_ );
      msgs_.parse();
      // initialize the ring buffers
      assert( QuickTrace::TraceFile::NumTraceLevels >= tfh_->logCount );
      const unsigned char * logStart =
            reinterpret_cast< const unsigned char * >( tfh_ ) + tfh_->fileHeaderSize;
      for ( unsigned i = 0; i < tfh_->logCount; ++i ) {
         unsigned logSize = tfh_->logSizes.sz[ i ] * 1024;
         rbs_[ i ] = RingBuffer( i, logStart, logStart + logSize );
         logStart += logSize;
      }
      if ( skipToEnd ) {
         // skip all existing messages
         for ( unsigned i = 0; i < tfh_->logCount; i++ ) {
            if ( levelEnabled( i ) ) {
               rbs_[ i ].fastforward( msgs_, options_ );
            }
         }
      } else {
         // go to the very first message
         for ( unsigned i = 0; i < tfh_->logCount; i++ ) {
            if ( levelEnabled( i ) ) {
              if ( !rbs_[ i ].rewind( msgs_, options_ ) ) {
                 errno = 0;
                 pexit( "File changed while reading, try again" );
              }
            }
         }
      }
      // initialize formatter
      tsc1_ = tfh_->tsc1;
      tsf_.initialize( *tfh_ );
      statusIs( ACTIVE );
      return true;
   }

   std::pair< uint64_t, int > nextTsc( uint64_t curTsc ) {
      if ( next_.second >= 0 ) {
         return next_;
      }
      if ( status_ == INITIALIZING && !initialize( false, false ) ) {
         // already opened and memory-mapped the file. Just waiting for monotime
         // to become usable, which is still not the case.
         // Therefore return next_ which atm means 'no message available'
         return next_;
      }
      if ( tfh_ == nullptr ) {
         // The file has been unmapped from memory. When status is REINIT_READY,
         // try to initialize. If that is not successful, or in any other cases
         // just return next_ which atm means 'no message available'
         if ( status_ != REINIT_READY || !initialize( false, false ) ) {
            return next_;
         }
      }
      // determine ring buffer that has the next message
      // i.e. the message with the lowest timestamp counter
      uint64_t prevMinTsc = UINT64_MAX;
      for ( ;; ) {
         next_ = std::make_pair( UINT64_MAX, -1 );
         for ( unsigned i = 0; i < tfh_->logCount; i++ ) {
            if ( levelEnabled( i ) ) {
               uint64_t tsc = rbs_[ i ].nextTsc();
               if ( tsc != 0 && tsc < next_.first ) {
                  // found a candidate
                  next_.first = tsc;
                  next_.second = i;
               }
            }
         }
         if ( next_.first == prevMinTsc ) {
            // the minimum tsc found in this round is the same as the one that
            // has been found in the last round. consider this the final result
            break;
         }
         // a new minimum tsc has been found in this round, this could be due to
         // timing issues or if an incomplete tsc has been read in the current or
         // the previous round. do one more interation to sort it out.
         prevMinTsc = next_.first;
      }
      if ( next_.second < 0 ) {
         // no next message available, time to do other work
         if ( status_ == DELETE_PENDING || status_ == REINIT_PENDING ||
              status_ == REINIT_READY ) {
            cleanup();
         } else if ( tsc1_ != tfh_->tsc1 ) {
            // re-initialize timestamp formatter as the timestamps in the header
            // have been updated after a buffer has wrapped
            tsc1_ = tfh_->tsc1;
            tsf_.initialize( *tfh_ );
         }
      }
      return next_;
   }

   Status status() const {
      return status_;
   }

   void statusIs( Status status ) {
      if ( status != status_ ) {
         switch ( status ) {
          case INITIALIZING: // initialize pending
            assert( status_ == REINIT_READY );
            break;
          case ACTIVE: // file successfully initialized
            assert( status_ == INITIALIZING );
            break;
          case DELETE_PENDING: // got IN_DELETE event
             if ( ( options_ & Options::PRINT_QT_FILE_EVENTS ) != 0 ) {
                std::cerr << "--- " << "deleted " << filename_ << std::endl;
             }
             // fallthrough
          case REINIT_PENDING: // got IN_CREATE event
            if ( status_ == INITIALIZING ) {
               // qttail is initializing but hasn't traced any messages
               // it can discard the file immediately as it has been either deleted
               // or is being in re-created.
               // In all other states it needs to keep tracing messages until there
               // are no more messages so it can not call cleanup yet
               cleanup();
            }
            break;
          case REINIT_READY: // got IN_MODIFY event
            if ( status_ != REINIT_PENDING ) {
               // IN_MODIFY means the agent has written something to the file
               // qttail must ignore this except for when it is in the REINIT_PENDING
               // state. In this state, it is the signal to start tailing the new
               // file.
               return;
            }
            break;
         }
      }
      status_ = status;
   }

   bool tail( uint64_t curTsc ) {
      uint64_t minTsc;
      int bufNum;
      std::tie( minTsc, bufNum ) = nextTsc( curTsc );
      if ( bufNum >= 0 ) {
         return tailBuffer( minTsc, curTsc, bufNum );
      } else {
         return false;
      }
   }

   bool tailBuffer( uint64_t tsc, uint64_t curTsc, int bufNum ) {
      // dump next message available in ring buffer bufNum
      // return true if a message has been consumed, or false if it as not been
      // consumed due to corruption (the source of the corruption could be a
      // concurrent write to the message data)
      try {
         bool ok = rbs_[ bufNum ].dump( msgs_, tsf_, tsc, curTsc, options_,
                                        qtname_.c_str() );
         if ( ok ) {
            // message has been consumed, reset next_
            next_ = std::make_pair( UINT64_MAX, -1 );
         }
         return ok;
      } catch ( const CorruptionError & e ) {
         std::cout << "---------- corruption detected in " << filename_ << std::endl;
         std::cout << "log level: " << bufNum << std::endl;
         if ( !e.msg().empty() ) {
            std::cout << "message id: " << e.msgId() << std::endl;
            std::cout << "message: " << e.msg() << std::endl;
            std::cout << "format: " << e.fmt() << std::endl;
         }
         std::cout << "reason: " << e.what() << std::endl;
         if ( ( options_ & Options::TAIL ) != 0 ) {
            // when tailing mode, go back to the start of the buffer
            if ( rbs_[ bufNum ].reset() ) {
               std::cout << "---------- resetting log buffer " << bufNum << " of "
                         << qtname_ << " due to corruption - potential message loss"
                         << std::endl;
            }
            next_ = std::make_pair( UINT64_MAX, -1 );
            return false;
         } else {
            // when in qtcat mode, just exit (there is no need to keep running)
            exit( EXIT_FAILURE );
         }
      }
   }

private:
   inline bool levelEnabled( unsigned level ) const {
      return ( options_ & Options::CHECK_LEVEL ) == 0 ||
             ( options_ & ( 1 << level ) ) != 0;
   }

   int fd_;
   std::string filename_; // full file name as passed from the command line
   Messages msgs_;
   int options_;
   std::pair< uint64_t, int > next_;
   std::string qtname_; // short file name (without path)
   std::array<RingBuffer, QuickTrace::TraceFile::NumTraceLevels> rbs_;
   off_t size_;
   Status status_;
   const QuickTrace::TraceFileHeader * tfh_;
   uint64_t tsc1_;
   TimestampFormatter tsf_;
};

// Watches a directory for modifications and maintains a list of tailed qt files
class Watch {
public:
   Watch( int fd, std::string dirname ) {
      fd_ = fd;
      wd_ = inotify_add_watch( fd_, dirname.c_str(), IN_CREATE | IN_DELETE );
      if ( wd_ < 0 ) {
         pexit( ( "inotify_add_watch(" + dirname + ", IN_CREATE)" ).c_str() );
      }
      dirname_ = std::move( dirname );
   }

   Watch( Watch && rhs ) :
         dirname_( std::move( rhs.dirname_ ) ), fd_( rhs.fd_ ), wd_( rhs.wd_ ),
         tails_( std::move( rhs.tails_ ) ) {
      rhs.wd_ = -1;
   }

   ~Watch() {
      if ( wd_ >= 0 ) {
         inotify_rm_watch( fd_, wd_ );
      }
   }

   void process( const inotify_event * event ) {
      if ( ( event->mask & IN_CREATE ) != 0 ) {
         auto tail = tails_.find( event->name );
         if ( tail != tails_.end() ) {
            watchForModificationsIs( true );
            tail->second.statusIs( Tail::REINIT_PENDING );
         }
      } else if ( ( event->mask & IN_DELETE ) != 0 ) {
         auto tail = tails_.find( event->name );
         if ( tail != tails_.end() ) {
            watchForModificationsIs( false );
            tail->second.statusIs( Tail::DELETE_PENDING );
         }
      } else if ( ( event->mask & IN_MODIFY ) != 0 ) {
         auto tail = tails_.find( event->name );
         if ( tail != tails_.end() ) {
            tail->second.statusIs( Tail::REINIT_READY );
            watchForModificationsIs( false );
         }
      }
   }

   std::map< std::string, Tail > & tails() {
      return tails_;
   }

   void watchForModificationsIs( bool on ) {
      for ( auto & t : tails_ ) {
         // when any file is in REINIT_PENDING it is still waiting for the
         // IN_MODIFY event so we do not want to turn it off
         if ( t.second.status() == Tail::REINIT_PENDING ) {
            return;
         }
      }
      if ( on ) {
         wd_ = inotify_add_watch( fd_, dirname_.c_str(),
                                  IN_CREATE | IN_DELETE | IN_MODIFY );
         if ( wd_ < 0 ) {
            pabort( ( "inotify_add_watch(" + dirname_ + ", IN_MODIFY)" ).c_str() );
         }
      } else {
         wd_ = inotify_add_watch( fd_, dirname_.c_str(), IN_CREATE | IN_DELETE );
         if ( wd_ < 0 ) {
            pabort( ( "inotify_add_watch(" + dirname_ + ", IN_CREATE)" ).c_str() );
         }
      }
   }

   int wd() const {
      return wd_;
   }

private:
   std::string dirname_; // watched directory
   int fd_; // inotify file descriptor (not owned)
   int wd_; // watch descriptor for create and modify events
   std::map< std::string, Tail > tails_; // qt file name to tail object
};

// controls concatenating of one or more files
class CatControl {
public:
   CatControl( char * const * filenames, int n, int options ) {
      nFiles_ = n;
      options_ = options;
      if ( n > 1 ) {
         options_ |= Options::PRINT_QT_FILE_NAME;
      }
      for ( int i = 0; i < n; i++ ) {
         files_.push_back( Tail( filenames[ i ], false, options_ ) );
      }
   }

   void cat() {
      if ( nFiles_ == 1 ) {
         catOne();
      } else {
         catMany();
      }
   }

private:
   void catOne() {
      Tail & file = *files_.begin();
      while ( file.tail( UINT64_MAX ) ) {}
      std::cout << std::flush;
   }

   void catMany() {
      populateQueue();
      while ( !fileQueue_.empty() ) {
         auto file = fileQueue_.begin();
         Tail * tail = file->second.tail;
         uint64_t tsc = file->first.tsc;
         int bufNum = file->second.bufNum;
         tail->tailBuffer( tsc, UINT64_MAX, bufNum );
         std::tie( tsc, bufNum ) = tail->nextTsc( UINT64_MAX );
         if ( bufNum < 0 ) {
            // all messages from this file have been dumped,
            // remove it from the queue
            fileQueue_.erase( file );
         } else {
            auto nextFile = file;
            ++nextFile;
            if ( nextFile != fileQueue_.end() ) {
               uint64_t nextFileTsc;
               std::tie( nextFileTsc, std::ignore ) =
                     nextFile->second.tail->nextTsc( UINT64_MAX );
               if ( nextFileTsc < tsc ) {
                  // the next file in line has a tsc that is before the current file
                  // therefore dequeue the current file and re-queue it
                  QueueKey key( tsc, file->first.fileNum );
                  QueueEntry entry( tail, bufNum );
                  fileQueue_.erase( file );
                  fileQueue_.insert( std::make_pair( key, entry ) );
               } else {
                  // update the tsc in the key. does not change order, but correct
                  // tsc is needed for subsequent dump
                  const_cast< QueueKey & >( file->first ).tsc = tsc;
                  file->second.bufNum = bufNum;
               }
            } else {
               // update the tsc in the key. does not change order, but correct
               // tsc is needed for subsequent dump
               const_cast< QueueKey & >( file->first ).tsc = tsc;
               file->second.bufNum = bufNum;
            }
         }
      }
      std::cout << std::flush;
   }

   void populateQueue() {
      uint32_t fileNum = 0;
      for ( auto end = files_.end(), file = files_.begin(); file != end; ) {
         uint64_t tsc;
         int bufNum;
         std::tie( tsc, bufNum ) = file->nextTsc( UINT64_MAX );
         if ( bufNum < 0 ) {
            // file is empty
            file = files_.erase( file );
         } else {
            QueueKey key( tsc, fileNum++ );
            QueueEntry entry( &*file, bufNum );
            fileQueue_.insert( std::make_pair( key, entry ) );
            ++file;
         }
      }
   }

   struct QueueKey {
      uint64_t tsc;
      uint32_t fileNum;

      QueueKey( uint64_t tsc, uint32_t fileNum ) :
         tsc( tsc ), fileNum( fileNum ) {}

      bool operator<( const QueueKey & rhs ) const {
         if ( tsc != rhs.tsc ) {
            return tsc < rhs.tsc;
         } else {
            return fileNum < rhs.fileNum;
         }
      }
   };

   struct QueueEntry {
      Tail * tail;
      int bufNum;

      QueueEntry( Tail * tail, int bufNum ) :
         tail( tail ), bufNum( bufNum ) {}
   };

   std::map< QueueKey, QueueEntry > fileQueue_; // ordered file queue
   int nFiles_; // number of files to tail
   int options_; // output options
   std::list< Tail > files_; // files to print
};

// controls tailing of one or more files and watching the corresponding directories
class TailControl {
public:
   TailControl( char * const * filenames, int n, int options ) {
      pfds_[ 0 ].fd = STDOUT_FILENO;
      pfds_[ 0 ].events = 0;
      pfds_[ 1 ].fd = inotify_init1( IN_NONBLOCK );
      if ( pfds_[ 1 ].fd < 0 ) {
         pabort( "inotify_init1()" );
      }
      pfds_[ 1 ].events = POLLIN;
      nFiles_ = n;
      options_ = options;
      if ( n > 1 ) {
         options_ |= Options::PRINT_QT_FILE_NAME;
      }
      std::map< std::string, Watch* > watchesByDir;
      for ( int i = 0; i < n; i++ ) {
         std::string dirname, filename = filenames[ i ], shortname;
         std::string::size_type sep = filename.rfind( '/' );
         if ( sep == std::string::npos ) {
            dirname = ".";
            shortname = filename;
         } else {
            dirname.assign( filename, 0, sep );
            shortname.assign( filename, sep + 1, filename.length() - sep );
         }
         if ( watchesByDir.count( dirname ) == 0 ) {
            Watch watch( pfds_[ 1 ].fd, dirname );
            int wd = watch.wd();
            auto w = watches_.insert( std::make_pair( wd, std::move( watch ) ) );
            watchesByDir.insert( std::make_pair( dirname, &w.first->second ) );
         }
         watchesByDir.find( dirname )->second->tails().insert(
            make_pair( std::move( shortname ),
                       Tail( std::move( filename ), true, options_ ) ) );
      }
   }

   ~TailControl() {
      if ( pfds_[ 1 ].fd >= 0 ) {
         ::close( pfds_[ 1 ].fd );
      }
   }

   void tail() {
      if ( nFiles_ == 1 ) {
         tailOne();
      } else {
         tailMany();
      }
   }

private:
   void tailOne() {
      Tail & tail = watches_.begin()->second.tails().begin()->second;
      for ( ;; ) {
         if ( tail.tail( rdtsc() ) ) {
            if ( TimestampFormatter::nextSecond_ ) {
               // check files every second at least
               TimestampFormatter::nextSecond_ = false;
               watch( false );
            }
         } else {
            // no next message available, time to do other work
            errno = 0;
            std::cout << std::flush;
            if ( !std::cout ) {
               pexit( "---------- error writing to stdout, aborting" );
            }
            if ( tail.status() == Tail::DELETE_PENDING ) {
               // file has been deleted and all messages have been emitted
               break;
            }
            watch( true );
         }
      }
   }

   void tailMany() {
      for ( ;; ) {
         bool allFilesDeleted = true;
         uint64_t curTsc = rdtsc();
         Tail * minTail = nullptr;
         uint64_t minTsc = UINT64_MAX;
         int minBufNum = -1;
         for ( auto & watch : watches_ ) {
            for ( auto & tail : watch.second.tails() ) {
               uint64_t tsc;
               int bufNum;
               std::tie( tsc, bufNum ) = tail.second.nextTsc( curTsc );
               if ( bufNum < 0 ) {
                  if ( tail.second.status() != Tail::DELETE_PENDING ) {
                     allFilesDeleted = false;
                  }
               } else if ( tsc < minTsc ) {
                  minTsc = tsc;
                  minTail = &tail.second;
                  minBufNum = bufNum;
               }
            }
         }
         if ( minTail != nullptr ) {
            if ( !minTail->tailBuffer( minTsc, curTsc, minBufNum ) ||
                 TimestampFormatter::nextSecond_ ) {
               // check files when either:
               // - tailBuffer has returned false due to corruption (add a delay
               //   by making a system call, after which the corruption has
               //   hopefully been resolved) or
               // - the next second has been reached (prevent starving)
               TimestampFormatter::nextSecond_ = false;
               watch( false );
            }
         } else {
            // no next message available, time to do other work
            std::cout << std::flush;
            if ( !std::cout ) {
               pexit( "---------- error writing to stdout, aborting" );
            }
            if ( allFilesDeleted ) {
               // all files have been deleted and all messages have been emitted
               return;
            }
            watch( true );
         }
      }
   }

   void watch( bool doPoll ) {
      bool doRead = !doPoll;
      if ( doPoll ) {
         // no trace message is currently available, wait 1ms and re-try
         // in order to avoid busy-looping the cpu
         int ec = poll( pfds_, 2, 1 );
         if ( ec < 0 ) {
            pabort( "poll" );
         } else if ( ec > 0 ) {
            if ( ( pfds_[ 0 ].revents & POLLERR ) != 0 ) {
               // stdout got closed -> give up right away
               exit( EXIT_FAILURE );
            }
            if ( ( pfds_[ 1 ].revents & POLLIN ) != 0 ) {
               doRead = true;
            }
         }
      }
      if ( doRead ) {
         char buf[ 4096 ];
         int len = read( pfds_[ 1 ].fd, buf, sizeof( buf ) );
         if ( len < 0 ) {
            if ( errno != EAGAIN ) {
               pabort( "read" );
            }
         } else {
            const inotify_event * event;
            for ( const char * ptr = buf; ptr < buf + len;
                  ptr += sizeof( inotify_event ) + event->len ) {
               event = reinterpret_cast< const inotify_event * >( ptr );
               auto watch = watches_.find( event->wd );
               assert( watch != watches_.end() );
               watch->second.process( event );
            }
         }
      }
   }

   pollfd pfds_[ 2 ]; // stdout (index 0) and inotify (index 1) file descriptors
   int nFiles_; // number of files to tail
   int options_; // output options
   std::map< int, Watch > watches_; // watch descriptor to watcher object
};

void usage( const char * msg = nullptr, int ec = EXIT_FAILURE ) {
   if ( msg != nullptr ) {
      std::cerr << "qttail: " << msg << std::endl;
   } else {
      std::cerr
         << "Usage: qttail [options] <file> ...\n\n"
         << "dump new log messages from quicktrace files\n\n"
         << "Options:\n"
         << "  -h, --help            show this help message and exit\n"
         << "  -c, --cat             print existing messages and exit (qtcat mode)\n"
         << "  -f, --files           print file and line number for trace "
            "statements\n"
         << "  -l, --levels <levels> list of levels to output\n"
         << "  --tsc                 print timestamp counter values\n"
         << "  -x                    print quicktrace file events\n"
         << "  -w, --wallClock       print only those messages which have "
            "wall-clock timestamp\n"
         << std::endl;
   }
   exit( ec );
}

int parseLevel( const std::string & arg ) {
   int ret = 0;
   auto parse = []( const std::string & s ) {
      int level;
      try {
         level = std::stoi( s );
      } catch ( const std::exception & ) {
         level = -1;
      }
      if ( level < 0 || level > 9 ) {
         usage( "invalid log level, must be 0..9" );
      }
      return level;
   };
   std::string::size_type pos = arg.find( '-' );
   if ( pos == std::string::npos ) {
      // single level
      ret = 1 << parse( arg );
   } else {
      // range of levels
      int startLevel = parse( arg.substr( 0, pos ) );
      int endLevel = parse( arg.substr( pos + 1 ) );
      if ( startLevel > endLevel ) {
         usage( "start log level must be less than end log level" );
      }
      while ( startLevel <= endLevel ) {
         ret |= 1 << startLevel;
         startLevel++;
      }
   }
   return ret;
}

int parseLevels( const std::string & arg ) {
   int levels = 0;
   std::string::size_type spos = 0, epos = arg.find( ',' );
   while ( spos != std::string::npos ) {
      std::string s;
      if ( epos == std::string::npos ) {
         s = arg.substr( spos );
         spos = epos;
      } else {
         s = arg.substr( spos, epos - spos );
         spos = epos + 1;
         epos = arg.find( spos, ',' );
      }
      levels |= parseLevel( s );
   }
   return levels;
}
} // namespace QuickTrace

int main( int argc, char * const * argv ) {
   using namespace QuickTrace;
   MessageFormatter::addPlugin();
   int options = Options::TAIL;

   static constexpr option const longOptions[] = {
      { "cat", no_argument, nullptr, 'c' },
      { "files", no_argument, nullptr, 'f' },
      { "help", no_argument, nullptr, 'h' },
      { "levels", required_argument, nullptr, 'l' },
      { "tsc", no_argument, nullptr, 0 },
      { "wallClock", no_argument, nullptr, 'w' },
      { nullptr, 0, nullptr, 0 }
   };
   static constexpr int tscOption = 4; // index into longOptions

   int opt, longOptIdx = 0;
   while ( ( opt = getopt_long(
                argc, argv, "cdfhl:xw", longOptions, &longOptIdx ) ) >= 0 ) {
      switch ( opt ) {
       case 0: // a long option
         switch ( longOptIdx ) {
          case tscOption: // --tsc
            static_assert( strcmp( longOptions[ tscOption ].name, "tsc" ) == 0,
                           "incorrect index for option --tsc" );
            options |= Options::PRINT_TSC;
            break;
         }
         break;
       case 'c':
         options &= ~Options::TAIL;
         break;
       case 'd':
         options |= Options::DEBUG;
         break;
       case 'f':
         options |= Options::PRINT_FILE_LINE;
         break;
       case 'h':
         usage( nullptr, EXIT_SUCCESS );
         break;
       case 'l':
         options |= parseLevels( optarg );
         break;
       case 'x':
         options |= Options::PRINT_QT_FILE_EVENTS;
         break;
       case 'w':
         options |= Options::PRINT_WALL_CLOCK_TIME;
         break;
       default:
         std::cerr << std::endl;
         usage();
      }
   }
   if ( optind >= argc ) {
      usage( "at least one <file> argument is required" );
   }
   if ( ( options & Options::LEVEL_ALL ) == 0 ) {
      // no levels specified, dump all
      options |= Options::LEVEL_ALL;
   } else {
      options |= Options::CHECK_LEVEL;
   }
   if ( ( options & Options::TAIL ) != 0 ) {
      TailControl tc( argv + optind, argc - optind, options );
      tc.tail();
   } else {
      CatControl cc( argv + optind, argc - optind, options );
      cc.cat();
   }
   return EXIT_SUCCESS;
}
