// Copyright (c) 2014, Arista Networks, Inc.
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

#ifdef QUICKTRACE_HEADER_INCLUDED_MARKER
#error Incorrect QuickTrace header ordering. \
       Please refer to README, Section ##: QuickTrace Header Ordering
#endif // QUICKTRACE_HEADER_INCLUDED_MARKER

#ifndef QUICKTRACE_RINGBUF_H
#define QUICKTRACE_RINGBUF_H

#include <QuickTrace/QuickTraceCommon.h>

namespace QuickTrace {

class TraceFile;

typedef int MsgId;

struct MsgCounter {
   uint32_t count;
   uint32_t lastTsc;                 // high bit of lastTsc disables traces
   uint64_t tscCount;
   uint64_t tscSelfCount;
};

struct QNull {};                // placeholder for no argument

class RingBuf {
 public:
   RingBuf() noexcept;
   void bufIs( void * buf, int bufSize ) noexcept;
   void qtFileIs( TraceFile * ) noexcept;
   void msgCounterIs( MsgCounter * m ) noexcept {
      msgCounter_ = m;
   }
   inline void maybeWrap( TraceFile * ) noexcept;
   void doWrap() noexcept;
   template < class T >
   RingBuf & operator<<( T t ) noexcept {
      if ( likely( enabled() ) ) {
         put( this, t );        // 'put' is overloaded
      }
      return *this;
   }
   MsgCounter * msgCounter( MsgId id ) noexcept {
      return &(msgCounter_[ id % numMsgCounters_ ]);
   }
   uint64_t startMsg( TraceFile *th, MsgId id ) noexcept;
   void endMsg() noexcept;
   template< class T >
   void push( T x ) noexcept {
      memcpy( ptr_, &x, sizeof( x ) );
      ptr_ += sizeof( T );
   }
   void * ptr() noexcept { return ptr_; }
   void ptrInc( int n ) noexcept { ptr_ += n; }
   void ptrIs( void * p ) noexcept { ptr_ = ( char * )p; }
   void numMsgCountersIs( int n ) noexcept { numMsgCounters_ = n; }
   bool enabled() const noexcept { return msgStart_; }

 private:
   static int const TrailerSize = 256;
   friend class TraceFile;
   uint32_t numMsgCounters_;
   char * ptr_;
   char * msgStart_;
   char * bufEnd_;
   char * buf_;
   MsgCounter * msgCounter_;
   TraceFile * qtFile_;
};

inline void put( RingBuf * log, char x ) noexcept { log->push( x ); }
inline void put( RingBuf * log, int8_t x ) noexcept { log->push( x ); }
inline void put( RingBuf * log, uint8_t x ) noexcept { log->push( x ); }
inline void put( RingBuf * log, uint16_t x ) noexcept { log->push( x ); }
inline void put( RingBuf * log, int16_t x ) noexcept { log->push( x ); }
inline void put( RingBuf * log, uint32_t x ) noexcept { log->push( x ); }
inline void put( RingBuf * log, int32_t x ) noexcept { log->push( x ); }
inline void put( RingBuf * log, uint64_t x ) noexcept { log->push( x ); }
#ifdef __LP64__
inline void put( RingBuf * log, long long unsigned int x ) noexcept {
   log->push( x );
}
#endif
inline void put( RingBuf * log, long long int x ) noexcept { log->push( x ); }
inline void put( RingBuf * log, long x ) noexcept { log->push( x ); }
#ifndef __LP64__
inline void put( RingBuf * log, unsigned long x ) noexcept {
   log->push( ( uint32_t )x );
}
#endif
inline void put( RingBuf * log, bool x ) noexcept { log->push( ( char )x ); }
inline void put( RingBuf * log, float x ) noexcept { log->push( x ); }
inline void put( RingBuf * log, double x ) noexcept { log->push( x ); }
inline void put( RingBuf * log, QNull x ) noexcept {}
void put( RingBuf * log, char const * x ) noexcept;
inline void put( RingBuf * log, void * x ) noexcept { log->push( ( uintptr_t )x ); }

} // namespace QuickTrace 

#endif // QUICKTRACE_RINGBUF_H

