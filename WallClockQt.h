// Copyright (c) 2022, Arista Networks, Inc.
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

#ifndef QUICKTRACE_WALLCLOCKQT_H
#define QUICKTRACE_WALLCLOCKQT_H

#include <QuickTrace/QuickTrace.h>

// This macro is identical to QTRACE_H, except it prepends wallClockTimestamp in
// RingBuf data.
// Note that the wall-clock timestamp is stored in in RingBuf in two separate
// fields. This is to make sure we can reuse the existing formatString value for
// these fields without redefining a new formatString and avoid incompatibility
// with the older versions of qtcat/qttail file.
#define WC_QTRACE_H( _qtf, _n, _x, _y )                                             \
   do {                                                                             \
      static QuickTrace::MsgId _msgId;                                              \
      if ( QUICKTRACE_LIKELY( !!( _qtf ) ) ) {                                      \
         struct timeval _tv = ( _qtf )->wallClockTimestamp();                       \
         WC_QTRACE_H_MSGID(                                                         \
            _qtf,                                                                   \
            _msgId,                                                                 \
            _n,                                                                     \
            _x,                                                                     \
            ( uint64_t )_tv.tv_sec << ( uint64_t )_tv.tv_usec << _y );              \
      }                                                                             \
   } while ( 0 )

// The goal of the _VAR macros below is to make sure that we only
// invoke qtvar() a single time for each variable. This seems to be
// needed as in some cases gcc advances __LINE__ in the middle of a
// macro.
#define WC_QTRACE_H_MSGID( _qtf, _msgId, _n, _x, _y )                               \
   WC_QTRACE_H_MSGID_VAR( _qtf, _msgId, qtvar( _rb ), _n, _x, _y )

#define WC_QTRACE_H_MSGID_VAR( _qtf, _msgId, _rb, _n, _x, _y )                      \
   WC_QTRACE_H_MSGID_INIT_FMT( _qtf, _msgId, _x, _y );                              \
   QuickTrace::RingBuf & _rb = ( _qtf )->log( _n );                                 \
   _rb.startMsg( _qtf, _msgId );                                                    \
   _rb << _y;                                                                       \
   _rb.endMsg()

// This macro is identical to QTRACE_H_MSGID_INIT_FMT, except it hard-codes the
// tracefile line number value to 0. This is a simple hack to differentiate the
// messages with wall-clock timestamp from the other regular messages without
// wall-clock timestamp.
// Note: The other alternative is to modify MsgDesc to identify this condition.
#define WC_QTRACE_H_MSGID_INIT_FMT( _qtf, _msgId, _x, _y )                                     \
   if ( QUICKTRACE_UNLIKELY( !!( _qtf ) && !( _qtf )->msgIdInitialized( _msgId ) ) ) {         \
      QuickTrace::MsgDesc _qtmd( _qtf, &_msgId, __FILE__, 0 );                                 \
      _qtmd.formatString() << _y;                                                              \
      _qtmd << _x;                                                                             \
      _qtmd.finish();                                                                          \
   }

#define WCQT0( _x, _y ) WC_QTRACE_H( QuickTrace::theTraceFile, 0, _x, _y )
#define WCQT1( _x, _y ) WC_QTRACE_H( QuickTrace::theTraceFile, 1, _x, _y )
#define WCQT2( _x, _y ) WC_QTRACE_H( QuickTrace::theTraceFile, 2, _x, _y )
#define WCQT3( _x, _y ) WC_QTRACE_H( QuickTrace::theTraceFile, 3, _x, _y )
#define WCQT4( _x, _y ) WC_QTRACE_H( QuickTrace::theTraceFile, 4, _x, _y )
#define WCQT5( _x, _y ) WC_QTRACE_H( QuickTrace::theTraceFile, 5, _x, _y )
#define WCQT6( _x, _y ) WC_QTRACE_H( QuickTrace::theTraceFile, 6, _x, _y )
#define WCQT7( _x, _y ) WC_QTRACE_H( QuickTrace::theTraceFile, 7, _x, _y )
#define WCQT8( _x, _y ) WC_QTRACE_H( QuickTrace::theTraceFile, 8, _x, _y )
#define WCQT9( _x, _y ) WC_QTRACE_H( QuickTrace::theTraceFile, 9, _x, _y )

#define WCQT0_F( _qtf, _x, _y ) WC_QTRACE_H( _qtf, 0, _x, _y )
#define WCQT1_F( _qtf, _x, _y ) WC_QTRACE_H( _qtf, 1, _x, _y )
#define WCQT2_F( _qtf, _x, _y ) WC_QTRACE_H( _qtf, 2, _x, _y )
#define WCQT3_F( _qtf, _x, _y ) WC_QTRACE_H( _qtf, 3, _x, _y )
#define WCQT4_F( _qtf, _x, _y ) WC_QTRACE_H( _qtf, 4, _x, _y )
#define WCQT5_F( _qtf, _x, _y ) WC_QTRACE_H( _qtf, 5, _x, _y )
#define WCQT6_F( _qtf, _x, _y ) WC_QTRACE_H( _qtf, 6, _x, _y )
#define WCQT7_F( _qtf, _x, _y ) WC_QTRACE_H( _qtf, 7, _x, _y )
#define WCQT8_F( _qtf, _x, _y ) WC_QTRACE_H( _qtf, 8, _x, _y )
#define WCQT9_F( _qtf, _x, _y ) WC_QTRACE_H( _qtf, 9, _x, _y )

#endif // QUICKTRACE_WALLCLOCKQT_H
