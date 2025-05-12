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

#ifndef QUICKTRACE_QTC_H
#define QUICKTRACE_QTC_H

#include <QuickTrace/QtcDecl.h>
#include <QuickTrace/QuickTraceCommon.h>

static struct ignoreme_ {} ignoreme;
#define qt_put_fmt_ignoreme(hdl, dp, d)
#define qt_put_type_ignoreme(hdl, level, d)

#define qt_put_if_compat( hdl, _tp, _fn, _tpstr, _lvlorDesc )               \
   if (__builtin_types_compatible_p(typeof (_qt_tmp), _tp)) {               \
      qt_put_##_fn##_##_tpstr(hdl, _lvlorDesc, ((void *)&_qt_tmp));         \
   }

/*
 * The following is for each agent to extend new types and handle in their
 * own way.
 */
#define QT_PUT_EXTENSIONS( hdl, _tp, _fn, _lvlorDesc ) {}

/*
 * The qt_put_ macro function gets called for each of the variable argument
 * That is passed to QTRACEF/QPROFF (by the QT_FOREACH macro)
 * This checks for type and calls appropriate qt_put_TYPE function.
 * -O0 seems to optimize out the other types out.
 */
#define qt_put_( hdl, _lvl, _desc, x ) do {                             \
      __extension__ __auto_type _qt_tmp = ( x );                        \
      if ( QUICKTRACE_UNLIKELY( _desc != NULL ) ) qt_put_dispatch_( hdl, _desc, fmt ); \
      qt_put_dispatch_( hdl, _lvl, type );                              \
   } while ( 0 );

#define qt_put_dispatch_( hdl, _lvlorDesc, FN )                       \
   do {                                                                     \
      qt_put_if_compat( hdl, struct ignoreme_, FN, ignoreme,                \
                        _lvlorDesc ) else                                   \
      qt_put_if_compat( hdl, char, FN, char, _lvlorDesc ) else              \
      qt_put_if_compat( hdl, double, FN, double, _lvlorDesc ) else          \
      qt_put_if_compat( hdl, float, FN, float, _lvlorDesc ) else            \
      qt_put_if_compat( hdl, int8_t, FN, s8, _lvlorDesc ) else              \
      qt_put_if_compat( hdl, int16_t, FN, s16, _lvlorDesc ) else            \
      qt_put_if_compat( hdl, int32_t, FN, s32, _lvlorDesc ) else            \
      qt_put_if_compat( hdl, int64_t, FN, s64, _lvlorDesc ) else            \
      qt_put_if_compat( hdl, uint8_t, FN, u8, _lvlorDesc ) else             \
      qt_put_if_compat( hdl, uint16_t, FN, u16, _lvlorDesc ) else           \
      qt_put_if_compat( hdl, uint32_t, FN, u32, _lvlorDesc ) else           \
      qt_put_if_compat( hdl, uint64_t, FN, u64, _lvlorDesc ) else           \
      qt_put_if_compat( hdl, bool, FN, bool, _lvlorDesc ) else              \
      qt_put_if_compat( hdl, const char*, FN, charp, _lvlorDesc ) else      \
      qt_put_if_compat( hdl, char*, FN, charp, _lvlorDesc ) else            \
      qt_put_if_compat( hdl, unsigned int, FN, int, _lvlorDesc ) else       \
      qt_put_if_compat( hdl, int, FN, int, _lvlorDesc ) else                \
      qt_put_if_compat( hdl, long, FN, long, _lvlorDesc ) else              \
      qt_put_if_compat( hdl, unsigned long, FN, ulong, _lvlorDesc ) else    \
      qt_put_if_compat( hdl, void*, FN, ptr, _lvlorDesc ) else              \
      QT_PUT_EXTENSIONS( hdl, typeof( _qt_tmp ), FN, _lvlorDesc )           \
   } while(0);

#define _QT_NUM_ARGS( X24, X23, X22, X21, X20, X19, X18, X17, X16, X15,     \
                      X14, X13, X12, X11, X10, X9, X8, X7, X6, X5, X4,      \
                      X3, X2, X1, N, ...)   N

#define QT_NUM_ARGS(...) _QT_NUM_ARGS(__VA_ARGS__, 24, 23, 22, 21, 20,      \
                                       19, 18, 17, 16, 15, 14, 13, 12,      \
                                       11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

/*
 * The following FOREACH macro technique was lifted off of the following 
 * website:
 * 
 * http://stackoverflow.com/questions/1872220/is-it-possible-to-iterate-over
 * -arguments-in-variadic-macros
 *
 * This macro allows us to iterate over the arguments passed to a variadic
 * macro (like the QTRACEF/QPROFF below).
 */
#define QT_EXPAND(X)                            X
#define QT_FIRSTARG(X, ...)                     (X)
#define QT_RESTARGS(X, ...)                     __VA_ARGS__
#define QT_FOREACH(hdl, M, _lvlorDesc, FN, ...)                             \
                QT_FOREACH_(hdl, QT_NUM_ARGS(__VA_ARGS__),                  \
                            M, _lvlorDesc, FN, __VA_ARGS__)
#define QT_FOREACH_(hdl, N, M, _lvlorDesc, FN, ...)                         \
                QT_FOREACH__(hdl, N, M, _lvlorDesc, FN, __VA_ARGS__)
#define QT_FOREACH__(hdl, N, M, _lvlorDesc, FN, ...)                        \
                QT_FOREACH_##N(hdl, M, _lvlorDesc, FN, __VA_ARGS__)

#define QT_FOREACH_1(hdl, M, _lvlorDesc, FN, ...)                           \
                M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))
#define QT_FOREACH_2(hdl, M, _lvlorDesc, FN, ...)                           \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_1(hdl, M, _lvlorDesc, FN,                        \
                             QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_3(hdl, M, _lvlorDesc, FN, ...)                           \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_2(hdl, M, _lvlorDesc, FN,                        \
                             QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_4(hdl, M, _lvlorDesc, FN, ...)                           \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_3(hdl, M, _lvlorDesc, FN,                        \
                             QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_5(hdl, M, _lvlorDesc, FN, ...)                           \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_4(hdl, M, _lvlorDesc, FN,                        \
                             QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_6(hdl, M, _lvlorDesc, FN, ...)                           \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_5(hdl, M, _lvlorDesc, FN,                        \
                             QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_7(hdl, M, _lvlorDesc, FN, ...)                           \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_6(hdl, M, _lvlorDesc, FN,                        \
                             QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_8(hdl, M, _lvlorDesc, FN, ...)                           \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_7(hdl, M, _lvlorDesc, FN,                        \
                             QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_9(hdl, M, _lvlorDesc, FN, ...)                           \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_8(hdl, M, _lvlorDesc, FN,                        \
                             QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_10(hdl, M, _lvlorDesc, FN, ...)                          \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_9(hdl, M, _lvlorDesc, FN,                        \
                             QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_11(hdl, M, _lvlorDesc, FN, ...)                          \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_10(hdl, M, _lvlorDesc, FN,                       \
                              QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_12(hdl, M, _lvlorDesc, FN, ...)                          \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_11(hdl, M, _lvlorDesc, FN,                       \
                              QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_13(hdl, M, _lvlorDesc, FN, ...)                          \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_12(hdl, M, _lvlorDesc, FN,                       \
                              QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_14(hdl, M, _lvlorDesc, FN, ...)                          \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_13(hdl, M, _lvlorDesc, FN,                       \
                              QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_15(hdl, M, _lvlorDesc, FN, ...)                          \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_14(hdl, M, _lvlorDesc, FN,                       \
                              QT_RESTARGS(__VA_ARGS__))   
#define QT_FOREACH_16(hdl, M, _lvlorDesc, FN, ...)                          \
                QT_EXPAND(M(hdl, _lvlorDesc, FN, QT_FIRSTARG(__VA_ARGS__))) \
                QT_FOREACH_15(hdl, M, _lvlorDesc, FN,                       \
                              QT_RESTARGS(__VA_ARGS__))   


/*
 * Using the trace for multiple files will not work for the following example:
 * 
 * hdl[0] = qt_initialize_handle(...);
 * hdl[1] = qt_initialize_handle(...);
 *
 * for (x = 0; .... ) {
 *     QTRACEF_H(hdl[x], "Hello World"); 
 * }
 *
 * The static variable _qt_msgid is per block, but it should be per file and
 * block.  For the first file handle, the message will generate a message id
 * because _qt_msgid is 0.  But the subsequent file handle will not create a
 * new message descriptor, because the static variable is already non-zero.  The
 * problem will be seen when qtcat tries to decode the tracefile -- msgid will
 * not be known or an incorrect msgid maybe used.
 */
#define QTRACEF_H( hdl, _level, str, ... )                                  \
{                                                                           \
   static int _qt_msgid = 0;                                                \
   uint64_t _qt_tsc = 0;                                                    \
   if ( QUICKTRACE_LIKELY( !!( qt_isInitialized(hdl) ) ) ) {                \
       void *_qt_desc = NULL;                                               \
       if ( QUICKTRACE_UNLIKELY( !_qt_msgid ) ) {                           \
           _qt_desc = alloca( qt_msgDescSize() );                           \
           qt_msgDescInit( hdl, _qt_desc, &_qt_msgid, __FILE__, __LINE__ ); \
           if ( _qt_desc ) {                                                \
             qt_startMsg( hdl, &_qt_tsc, _qt_msgid, _level );               \
             QT_FOREACH( hdl, qt_put_, _level, _qt_desc, __VA_ARGS__ );     \
             qt_addMsg( _qt_desc, str );                                    \
             qt_finish( _qt_desc );                                         \
             qt_endMsg( hdl, _level );                                      \
           }                                                                \
       } else {                                                             \
          qt_startMsg( hdl, &_qt_tsc, _qt_msgid, _level );                  \
          QT_FOREACH( hdl, qt_put_, _level, _qt_desc, __VA_ARGS__ ); \
          qt_endMsg( hdl, _level );                                         \
       }                                                                    \
   }                                                                        \
}                                                                           \

#define qtvar__(a,x) _qt_##a##x
#define qtvar_(a,x) qtvar__(a,x)
#define qtvar( a ) qtvar_(a,__LINE__)


#define QPROFF_H( hdl, _level, str, ... )                                   \
    qtprof_t qtvar(_qprof_tmp_var) __attribute__((cleanup(qtproff_eob)));   \
    static int qtvar(_qt_msgid) = 0;                                        \
    uint64_t qtvar(_qt_tsc) = 0;                                            \
    qtvar(_qprof_tmp_var).th = hdl;                                         \
    if ( QUICKTRACE_LIKELY( !!( qt_isInitialized(hdl) ) ) ) {               \
        void *_qt_desc = 0;                                                 \
        if ( QUICKTRACE_UNLIKELY( !qtvar(_qt_msgid) ) ) {                   \
            _qt_desc = alloca(qt_msgDescSize());                            \
            qt_msgDescInit( hdl, _qt_desc, &qtvar(_qt_msgid),               \
                            __FILE__, __LINE__ );                           \
            if ( _qt_desc ) {                                               \
              qt_startMsg( hdl, &qtvar(_qt_tsc), qtvar(_qt_msgid), _level );\
              QT_FOREACH( hdl, qt_put_, _level, _qt_desc, __VA_ARGS__ );    \
              qt_endMsg( hdl, _level );                                     \
              qt_addMsg( _qt_desc, str );                                   \
              qt_finish( _qt_desc );                                        \
            }                                                               \
        } else {                                                            \
           qt_startMsg( hdl, &qtvar(_qt_tsc), qtvar(_qt_msgid), _level);    \
           QT_FOREACH( hdl, qt_put_, _level, _qt_desc, __VA_ARGS__ );       \
           qt_endMsg( hdl, _level );                                        \
        }                                                                   \
        qtvar(_qprof_tmp_var).mid = qtvar(_qt_msgid);                       \
        qtvar(_qprof_tmp_var).tsc = qtvar(_qt_tsc);                         \
    }

#define QPROF_H( hdl, str )                                                 \
    qtprof_t qtvar(_qprof_tmp_var) __attribute__((cleanup(qtprof_eob)));    \
    static int qtvar(_qt_msgid) = 0;                                        \
    qtvar(_qprof_tmp_var).th = hdl;                                         \
    if ( QUICKTRACE_LIKELY( !!( qt_isInitialized(hdl) ) ) ) {               \
        if ( QUICKTRACE_UNLIKELY( !qtvar(_qt_msgid) ) ) {                   \
            void *_qt_desc = alloca(qt_msgDescSize());                      \
            qt_msgDescInit( hdl, _qt_desc, &qtvar(_qt_msgid),               \
                            __FILE__, __LINE__ );                           \
            if ( _qt_desc ) {                                               \
              qt_addMsg( _qt_desc, str );                                   \
              qt_finish( _qt_desc );                                        \
            }                                                               \
        }                                                                   \
        qtvar(_qprof_tmp_var).mid = qtvar(_qt_msgid);                       \
        qtvar(_qprof_tmp_var).tsc = rdtsc();                                \
    }                                                                       \

/*
 * The ignoreme hack is to be able to take just a string argument in QTRACEFx()
 */
#define QTRACEF0( str, ... ) QTRACEF_H( default_hdl, 0, str, ignoreme,##__VA_ARGS__ )
#define QTRACEF1( str, ... ) QTRACEF_H( default_hdl, 1, str, ignoreme,##__VA_ARGS__ )
#define QTRACEF2( str, ... ) QTRACEF_H( default_hdl, 2, str, ignoreme,##__VA_ARGS__ )
#define QTRACEF3( str, ... ) QTRACEF_H( default_hdl, 3, str, ignoreme,##__VA_ARGS__ )
#define QTRACEF4( str, ... ) QTRACEF_H( default_hdl, 4, str, ignoreme,##__VA_ARGS__ )
#define QTRACEF5( str, ... ) QTRACEF_H( default_hdl, 5, str, ignoreme,##__VA_ARGS__ )
#define QTRACEF6( str, ... ) QTRACEF_H( default_hdl, 6, str, ignoreme,##__VA_ARGS__ )
#define QTRACEF7( str, ... ) QTRACEF_H( default_hdl, 7, str, ignoreme,##__VA_ARGS__ )
#define QTRACEF8( str, ... ) QTRACEF_H( default_hdl, 8, str, ignoreme,##__VA_ARGS__ )
#define QTRACEF9( str, ... ) QTRACEF_H( default_hdl, 9, str, ignoreme,##__VA_ARGS__ )

#define QPROFF0( str, ... ) QPROFF_H( default_hdl, 0, str, ignoreme,##__VA_ARGS__ )
#define QPROFF1( str, ... ) QPROFF_H( default_hdl, 1, str, ignoreme,##__VA_ARGS__ )
#define QPROFF2( str, ... ) QPROFF_H( default_hdl, 2, str, ignoreme,##__VA_ARGS__ )
#define QPROFF3( str, ... ) QPROFF_H( default_hdl, 3, str, ignoreme,##__VA_ARGS__ )
#define QPROFF4( str, ... ) QPROFF_H( default_hdl, 4, str, ignoreme,##__VA_ARGS__ )
#define QPROFF5( str, ... ) QPROFF_H( default_hdl, 5, str, ignoreme,##__VA_ARGS__ )
#define QPROFF6( str, ... ) QPROFF_H( default_hdl, 6, str, ignoreme,##__VA_ARGS__ )
#define QPROFF7( str, ... ) QPROFF_H( default_hdl, 7, str, ignoreme,##__VA_ARGS__ )
#define QPROFF8( str, ... ) QPROFF_H( default_hdl, 8, str, ignoreme,##__VA_ARGS__ )
#define QPROFF9( str, ... ) QPROFF_H( default_hdl, 9, str, ignoreme,##__VA_ARGS__ )

#define QPROF( str ) QPROF_H( default_hdl, str )

/*
 * Macros require a handle, which can be created via qt_initialize_handle()
 */
#define QTRACEF0_F( hdl, str, ... ) QTRACEF_H( hdl, 0, str, ignoreme, \
                                             ##__VA_ARGS__ )
#define QTRACEF1_F( hdl, str, ... ) QTRACEF_H( hdl, 1, str, ignoreme, \
                                             ##__VA_ARGS__ )
#define QTRACEF2_F( hdl, str, ... ) QTRACEF_H( hdl, 2, str, ignoreme, \
                                             ##__VA_ARGS__ )
#define QTRACEF3_F( hdl, str, ... ) QTRACEF_H( hdl, 3, str, ignoreme, \
                                             ##__VA_ARGS__ )
#define QTRACEF4_F( hdl, str, ... ) QTRACEF_H( hdl, 4, str, ignoreme, \
                                             ##__VA_ARGS__ )
#define QTRACEF5_F( hdl, str, ... ) QTRACEF_H( hdl, 5, str, ignoreme, \
                                             ##__VA_ARGS__ )
#define QTRACEF6_F( hdl, str, ... ) QTRACEF_H( hdl, 6, str, ignoreme, \
                                             ##__VA_ARGS__ )
#define QTRACEF7_F( hdl, str, ... ) QTRACEF_H( hdl, 7, str, ignoreme, \
                                             ##__VA_ARGS__ )
#define QTRACEF8_F( hdl, str, ... ) QTRACEF_H( hdl, 8, str, ignoreme, \
                                             ##__VA_ARGS__ )
#define QTRACEF9_F( hdl, str, ... ) QTRACEF_H( hdl, 9, str, ignoreme, \
                                             ##__VA_ARGS__ )

#define QPROFF0_F( hdl, str, ... ) QPROFF_H( hdl, 0, str, ignoreme,##__VA_ARGS__ )
#define QPROFF1_F( hdl, str, ... ) QPROFF_H( hdl, 1, str, ignoreme,##__VA_ARGS__ )
#define QPROFF2_F( hdl, str, ... ) QPROFF_H( hdl, 2, str, ignoreme,##__VA_ARGS__ )
#define QPROFF3_F( hdl, str, ... ) QPROFF_H( hdl, 3, str, ignoreme,##__VA_ARGS__ )
#define QPROFF4_F( hdl, str, ... ) QPROFF_H( hdl, 4, str, ignoreme,##__VA_ARGS__ )
#define QPROFF5_F( hdl, str, ... ) QPROFF_H( hdl, 5, str, ignoreme,##__VA_ARGS__ )
#define QPROFF6_F( hdl, str, ... ) QPROFF_H( hdl, 6, str, ignoreme,##__VA_ARGS__ )
#define QPROFF7_F( hdl, str, ... ) QPROFF_H( hdl, 7, str, ignoreme,##__VA_ARGS__ )
#define QPROFF8_F( hdl, str, ... ) QPROFF_H( hdl, 8, str, ignoreme,##__VA_ARGS__ )
#define QPROFF9_F( hdl, str, ... ) QPROFF_H( hdl, 9, str, ignoreme,##__VA_ARGS__ )

#define QPROF_F( hdl, str ) QPROF_H( hdl, str )

#endif  // QUICKTRACE_QTC_H
