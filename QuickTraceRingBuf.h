// Copyright (c) 2014 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

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

