// Copyright (c) 2021 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#ifndef QUICKTRACE_QTFMT_H
#define QUICKTRACE_QTFMT_H

#include <QuickTrace/QuickTrace.h>

namespace QuickTrace {

// This function decodes a std-style format specification, turning {} and {:x}
// to corresponding calls to QVAR and QHEX, and copying anything else.
// To escape '{' and '}', use {{ and }}.
// If className is a nullptr, we'll omit tracing the className and the funcName.
// If className is an empty, we'll only trace the function name.
void msgDesc( QuickTrace::MsgDesc & qtmd,
              const char * className,
              const char * funcName,
              const char * fmtString ) noexcept;

template< typename... Ts >
constexpr void
msgIdInit( QuickTrace::MsgDesc & qtmd,
           const char * className,
           const char * funcName,
           const char * fmtString,
           Ts &&... args ) noexcept {
   ( ( qtmd.formatString() << std::forward< Ts >( args ) ), ... );
   msgDesc( qtmd, className, funcName, fmtString );
   qtmd.finish();
}

template< typename... Ts >
constexpr uint64_t
fmtMsg( QuickTrace::RingBuf & rb,
        QuickTrace::TraceFile * tf,
        QuickTrace::MsgId id,
        Ts &&... args ) noexcept {
   uint64_t tsc = rb.startMsg( tf, id );
   ( ( rb << std::forward< Ts >( args ) ), ... );
   rb.endMsg();
   return tsc;
}

} // namespace QuickTrace

#define QTFMT_H_MSGID_INIT_FMT( _qtf, _msgId, className, funcName, fmtString, ... ) \
   if ( unlikely( !!( _qtf ) && !( _qtf )->msgIdInitialized( _msgId ) ) ) {         \
      QuickTrace::MsgDesc _qtmd( _qtf, &_msgId, __FILE__, __LINE__ );               \
      QuickTrace::msgIdInit(                                                        \
         _qtmd, className, funcName, fmtString, ##__VA_ARGS__ );                    \
   }

#define QTFMT_MSGID_VAR(                                                            \
   _qtf, _msgId, _rb, _n, className, funcName, fmtString, ... )                     \
   QTFMT_H_MSGID_INIT_FMT(                                                          \
      _qtf, _msgId, className, funcName, fmtString, ##__VA_ARGS__ );                \
   QuickTrace::RingBuf & _rb = ( _qtf )->log( _n );                                 \
   QuickTrace::fmtMsg( _rb, _qtf, _msgId, ##__VA_ARGS__ );

#define QTFMT_H_MSGID( _qtf, _msgId, _n, className, funcName, fmtString, ... )      \
   QTFMT_MSGID_VAR( _qtf,                                                           \
                    _msgId,                                                         \
                    qtvar( _rb ),                                                   \
                    _n,                                                             \
                    className,                                                      \
                    funcName,                                                       \
                    fmtString,                                                      \
                    ##__VA_ARGS__ )

#define QTFMT_INTERNAL_H( _qtf, _n, className, fmtString, ... )                     \
   do {                                                                             \
      static QuickTrace::MsgId _msgId;                                              \
      if ( likely( !!( _qtf ) ) ) {                                                 \
         QTFMT_H_MSGID(                                                             \
            _qtf, _msgId, _n, className, __func__, fmtString, ##__VA_ARGS__ );      \
      }                                                                             \
   } while ( 0 )

// qtraceClassName is a macro that will be defined by non-generic users, and will be
// used to inject an appropriate class name.
#define QTFMT_H( _qtf, _n, fmtString, ... )                                         \
   QTFMT_INTERNAL_H( _qtf, _n, qtraceClassName, fmtString, ##__VA_ARGS__ )

#define QTFMT0( fmtString, ... )                                                    \
   QTFMT_H( QuickTrace::theTraceFile, 0, fmtString, ##__VA_ARGS__ )
#define QTFMT1( fmtString, ... )                                                    \
   QTFMT_H( QuickTrace::theTraceFile, 1, fmtString, ##__VA_ARGS__ )
#define QTFMT2( fmtString, ... )                                                    \
   QTFMT_H( QuickTrace::theTraceFile, 2, fmtString, ##__VA_ARGS__ )
#define QTFMT3( fmtString, ... )                                                    \
   QTFMT_H( QuickTrace::theTraceFile, 3, fmtString, ##__VA_ARGS__ )
#define QTFMT4( fmtString, ... )                                                    \
   QTFMT_H( QuickTrace::theTraceFile, 4, fmtString, ##__VA_ARGS__ )
#define QTFMT5( fmtString, ... )                                                    \
   QTFMT_H( QuickTrace::theTraceFile, 5, fmtString, ##__VA_ARGS__ )
#define QTFMT6( fmtString, ... )                                                    \
   QTFMT_H( QuickTrace::theTraceFile, 6, fmtString, ##__VA_ARGS__ )
#define QTFMT7( fmtString, ... )                                                    \
   QTFMT_H( QuickTrace::theTraceFile, 7, fmtString, ##__VA_ARGS__ )
#define QTFMT8( fmtString, ... )                                                    \
   QTFMT_H( QuickTrace::theTraceFile, 8, fmtString, ##__VA_ARGS__ )
#define QTFMT9( fmtString, ... )                                                    \
   QTFMT_H( QuickTrace::theTraceFile, 9, fmtString, ##__VA_ARGS__ )

#define QTFMT0_F( fmtString, ... )                                                  \
   QTFMT_H( ( hdl )->getFile(), 0, fmtString, ##__VA_ARGS__ )
#define QTFMT1_F( fmtString, ... )                                                  \
   QTFMT_H( ( hdl )->getFile(), 1, fmtString, ##__VA_ARGS__ )
#define QTFMT2_F( fmtString, ... )                                                  \
   QTFMT_H( ( hdl )->getFile(), 2, fmtString, ##__VA_ARGS__ )
#define QTFMT3_F( fmtString, ... )                                                  \
   QTFMT_H( ( hdl )->getFile(), 3, fmtString, ##__VA_ARGS__ )
#define QTFMT4_F( fmtString, ... )                                                  \
   QTFMT_H( ( hdl )->getFile(), 4, fmtString, ##__VA_ARGS__ )
#define QTFMT5_F( fmtString, ... )                                                  \
   QTFMT_H( ( hdl )->getFile(), 5, fmtString, ##__VA_ARGS__ )
#define QTFMT6_F( fmtString, ... )                                                  \
   QTFMT_H( ( hdl )->getFile(), 6, fmtString, ##__VA_ARGS__ )
#define QTFMT7_F( fmtString, ... )                                                  \
   QTFMT_H( ( hdl )->getFile(), 7, fmtString, ##__VA_ARGS__ )
#define QTFMT8_F( fmtString, ... )                                                  \
   QTFMT_H( ( hdl )->getFile(), 8, fmtString, ##__VA_ARGS__ )
#define QTFMT9_F( fmtString, ... )                                                  \
   QTFMT_H( ( hdl )->getFile(), 9, fmtString, ##__VA_ARGS__ )

#define QTFMT_FUNC_H( _qtf, _n, fmtString, ... )                                    \
   QTFMT_INTERNAL_H( _qtf, _n, "", fmtString, ##__VA_ARGS__ )

#define QTFMT0_FUNC( fmtString, ... )                                               \
   QTFMT_FUNC_H( QuickTrace::theTraceFile, 0, fmtString, ##__VA_ARGS__ )
#define QTFMT1_FUNC( fmtString, ... )                                               \
   QTFMT_FUNC_H( QuickTrace::theTraceFile, 1, fmtString, ##__VA_ARGS__ )
#define QTFMT2_FUNC( fmtString, ... )                                               \
   QTFMT_FUNC_H( QuickTrace::theTraceFile, 2, fmtString, ##__VA_ARGS__ )
#define QTFMT3_FUNC( fmtString, ... )                                               \
   QTFMT_FUNC_H( QuickTrace::theTraceFile, 3, fmtString, ##__VA_ARGS__ )
#define QTFMT4_FUNC( fmtString, ... )                                               \
   QTFMT_FUNC_H( QuickTrace::theTraceFile, 4, fmtString, ##__VA_ARGS__ )
#define QTFMT5_FUNC( fmtString, ... )                                               \
   QTFMT_FUNC_H( QuickTrace::theTraceFile, 5, fmtString, ##__VA_ARGS__ )
#define QTFMT6_FUNC( fmtString, ... )                                               \
   QTFMT_FUNC_H( QuickTrace::theTraceFile, 6, fmtString, ##__VA_ARGS__ )
#define QTFMT7_FUNC( fmtString, ... )                                               \
   QTFMT_FUNC_H( QuickTrace::theTraceFile, 7, fmtString, ##__VA_ARGS__ )
#define QTFMT8_FUNC( fmtString, ... )                                               \
   QTFMT_FUNC_H( QuickTrace::theTraceFile, 8, fmtString, ##__VA_ARGS__ )
#define QTFMT9_FUNC( fmtString, ... )                                               \
   QTFMT_FUNC_H( QuickTrace::theTraceFile, 9, fmtString, ##__VA_ARGS__ )

#define QTFMT_RAW_H( _qtf, _n, fmtString, ... )                                     \
   QTFMT_INTERNAL_H( _qtf, _n, nullptr, fmtString, ##__VA_ARGS__ )

#define QTFMT0_RAW( fmtString, ... )                                                \
   QTFMT_RAW_H( QuickTrace::theTraceFile, 0, fmtString, ##__VA_ARGS__ )
#define QTFMT1_RAW( fmtString, ... )                                                \
   QTFMT_RAW_H( QuickTrace::theTraceFile, 1, fmtString, ##__VA_ARGS__ )
#define QTFMT2_RAW( fmtString, ... )                                                \
   QTFMT_RAW_H( QuickTrace::theTraceFile, 2, fmtString, ##__VA_ARGS__ )
#define QTFMT3_RAW( fmtString, ... )                                                \
   QTFMT_RAW_H( QuickTrace::theTraceFile, 3, fmtString, ##__VA_ARGS__ )
#define QTFMT4_RAW( fmtString, ... )                                                \
   QTFMT_RAW_H( QuickTrace::theTraceFile, 4, fmtString, ##__VA_ARGS__ )
#define QTFMT5_RAW( fmtString, ... )                                                \
   QTFMT_RAW_H( QuickTrace::theTraceFile, 5, fmtString, ##__VA_ARGS__ )
#define QTFMT6_RAW( fmtString, ... )                                                \
   QTFMT_RAW_H( QuickTrace::theTraceFile, 6, fmtString, ##__VA_ARGS__ )
#define QTFMT7_RAW( fmtString, ... )                                                \
   QTFMT_RAW_H( QuickTrace::theTraceFile, 7, fmtString, ##__VA_ARGS__ )
#define QTFMT8_RAW( fmtString, ... )                                                \
   QTFMT_RAW_H( QuickTrace::theTraceFile, 8, fmtString, ##__VA_ARGS__ )
#define QTFMT9_RAW( fmtString, ... )                                                \
   QTFMT_RAW_H( QuickTrace::theTraceFile, 9, fmtString, ##__VA_ARGS__ )

#define QTFMT_ASSERT( _cond, fmtString, ... )                                       \
   if ( unlikely( !( _cond ) ) ) {                                                  \
      QTFMT0( fmtString, ##__VA_ARGS__ )                                            \
      assert( _cond );                                                              \
   }

#define QTFMT_PROF_INTERNAL_H( _qtf, _n, className, fmtString, ... )                \
   QTFMT_PROF_H_VAR(                                                                \
      _qtf, qtvar( msgid ), _n, className, __func__, fmtString, ##__VA_ARGS__ )

#define QTFMT_PROF_H( _qtf, _n, fmtString, ... )                                    \
   QTFMT_PROF_INTERNAL_H( _qtf, _n, qtraceClassName, fmtString, ##__VA_ARGS__ )

#define QTFMT_PROF_H_VAR( _qtf, _msgId, _n, className, funcName, fmtString, ... )   \
   static QuickTrace::MsgId _msgId;                                                 \
   QTFMT_PROF_H_MSGID_VAR( _qtf,                                                    \
                           _msgId,                                                  \
                           qtvar( tsc ),                                            \
                           _n,                                                      \
                           className,                                               \
                           funcName,                                                \
                           fmtString,                                               \
                           ##__VA_ARGS__ )

#define QTFMT_PROF_H_MSGID_VAR(                                                     \
   _qtf, _msgId, _tsc, _n, className, funcName, fmtString, ... )                    \
   uint64_t _tsc;                                                                   \
   if ( likely( !!( _qtf ) ) ) {                                                    \
      QTFMT_H_MSGID_INIT_FMT(                                                       \
         _qtf, _msgId, className, funcName, fmtString, ##__VA_ARGS__ );             \
      QuickTrace::RingBuf & _rb = ( _qtf )->log( _n );                              \
      qtvar( tsc ) = QuickTrace::fmtMsg( _rb, _qtf, _msgId, ##__VA_ARGS__ );        \
   } else {                                                                         \
      _tsc = 0;                                                                     \
   }                                                                                \
   QuickTrace::BlockTimerMsg qtvar( bt )( ( _qtf ), _msgId, _tsc )

#define QTFMT_PROF0( fmtString, ... )                                               \
   QTFMT_PROF_H( QuickTrace::theTraceFile, 0, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF1( fmtString, ... )                                               \
   QTFMT_PROF_H( QuickTrace::theTraceFile, 1, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF2( fmtString, ... )                                               \
   QTFMT_PROF_H( QuickTrace::theTraceFile, 2, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF3( fmtString, ... )                                               \
   QTFMT_PROF_H( QuickTrace::theTraceFile, 3, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF4( fmtString, ... )                                               \
   QTFMT_PROF_H( QuickTrace::theTraceFile, 4, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF5( fmtString, ... )                                               \
   QTFMT_PROF_H( QuickTrace::theTraceFile, 5, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF6( fmtString, ... )                                               \
   QTFMT_PROF_H( QuickTrace::theTraceFile, 6, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF7( fmtString, ... )                                               \
   QTFMT_PROF_H( QuickTrace::theTraceFile, 7, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF8( fmtString, ... )                                               \
   QTFMT_PROF_H( QuickTrace::theTraceFile, 8, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF9( fmtString, ... )                                               \
   QTFMT_PROF_H( QuickTrace::theTraceFile, 9, fmtString, ##__VA_ARGS__ )

#define QTFMT_PROF_FUNC_H( _qtf, _n, fmtString, ... )                               \
   QTFMT_PROF_INTERNAL_H( _qtf, _n, "", fmtString, ##__VA_ARGS__ )

#define QTFMT_PROF0_FUNC( fmtString, ... )                                          \
   QTFMT_PROF_FUNC_H( QuickTrace::theTraceFile, 0, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF1_FUNC( fmtString, ... )                                          \
   QTFMT_PROF_FUNC_H( QuickTrace::theTraceFile, 1, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF2_FUNC( fmtString, ... )                                          \
   QTFMT_PROF_FUNC_H( QuickTrace::theTraceFile, 2, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF3_FUNC( fmtString, ... )                                          \
   QTFMT_PROF_FUNC_H( QuickTrace::theTraceFile, 3, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF4_FUNC( fmtString, ... )                                          \
   QTFMT_PROF_FUNC_H( QuickTrace::theTraceFile, 4, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF5_FUNC( fmtString, ... )                                          \
   QTFMT_PROF_FUNC_H( QuickTrace::theTraceFile, 5, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF6_FUNC( fmtString, ... )                                          \
   QTFMT_PROF_FUNC_H( QuickTrace::theTraceFile, 6, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF7_FUNC( fmtString, ... )                                          \
   QTFMT_PROF_FUNC_H( QuickTrace::theTraceFile, 7, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF8_FUNC( fmtString, ... )                                          \
   QTFMT_PROF_FUNC_H( QuickTrace::theTraceFile, 8, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF9_FUNC( fmtString, ... )                                          \
   QTFMT_PROF_FUNC_H( QuickTrace::theTraceFile, 9, fmtString, ##__VA_ARGS__ )

#define QTFMT_PROF_RAW_H( _qtf, _n, fmtString, ... )                                \
   QTFMT_PROF_INTERNAL_H( _qtf, _n, nullptr, fmtString, ##__VA_ARGS__ )

#define QTFMT_PROF0_RAW( fmtString, ... )                                           \
   QTFMT_PROF_RAW_H( QuickTrace::theTraceFile, 0, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF1_RAW( fmtString, ... )                                           \
   QTFMT_PROF_RAW_H( QuickTrace::theTraceFile, 1, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF2_RAW( fmtString, ... )                                           \
   QTFMT_PROF_RAW_H( QuickTrace::theTraceFile, 2, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF3_RAW( fmtString, ... )                                           \
   QTFMT_PROF_RAW_H( QuickTrace::theTraceFile, 3, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF4_RAW( fmtString, ... )                                           \
   QTFMT_PROF_RAW_H( QuickTrace::theTraceFile, 4, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF5_RAW( fmtString, ... )                                           \
   QTFMT_PROF_RAW_H( QuickTrace::theTraceFile, 5, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF6_RAW( fmtString, ... )                                           \
   QTFMT_PROF_RAW_H( QuickTrace::theTraceFile, 6, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF7_RAW( fmtString, ... )                                           \
   QTFMT_PROF_RAW_H( QuickTrace::theTraceFile, 7, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF8_RAW( fmtString, ... )                                           \
   QTFMT_PROF_RAW_H( QuickTrace::theTraceFile, 8, fmtString, ##__VA_ARGS__ )
#define QTFMT_PROF9_RAW( fmtString, ... )                                           \
   QTFMT_PROF_RAW_H( QuickTrace::theTraceFile, 9, fmtString, ##__VA_ARGS__ )

// TODO: QPROF_F
// TODO: QPROF_S

#endif // QUICKTRACE_QTFMT_H
