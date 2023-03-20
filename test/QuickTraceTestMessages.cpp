// Copyright (c) 2020 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#include <QuickTrace/QuickTrace.h>

// Trace a message once. These expect the default handle to be intialized already.
void
traceCounter( int level, int counter ) {
   QTRACE_H( QuickTrace::theTraceFile, level, "counter: " << QVAR, counter );
}

void
traceString( int level, const char * string ) {
   QTRACE_H( QuickTrace::theTraceFile, level, "string: " << QVAR, string );
}

extern "C" {

// C wrappers for the above so that ctypes can use them

void
qtTraceCounter( int level, int counter ) {
   traceCounter( level, counter );
}

void
qtTraceString( int level, const char * string ) {
   traceString( level, string );
}

// Stubs for extern GoQuickTrace functions
QuickTrace::MsgId GoQuickTrace_create_msg_id(
   void * hdl, const char * file, int line, const char * fmt, const char * msg );

void GoQuickTrace_fullMsg( void * hdl,
                           uint64_t * tsc,
                           int id,
                           int level,
                           int numArgs,
                           uint32_t * argLens,
                           void * args );

// Create a MsgId using the GoQuickTrace API
int
qtCreateCounterMsgId( const char * msg ) {
   return GoQuickTrace_create_msg_id(
      QuickTrace::defaultQuickTraceHandle, __FILE__, __LINE__, "i", msg );
}

int
qtCreateStringMsgId( const char * msg ) {
   return GoQuickTrace_create_msg_id(
      QuickTrace::defaultQuickTraceHandle, __FILE__, __LINE__, "p", msg );
}

// Trace a message once. These expect the default handle to be
// initialized already
void
qtGoTraceCounter( int level, int msgId, int counter ) {
   uint64_t tsc;
   int numArgs = 1;
   uint32_t fullMsgU32Mask = 3 << 10;
   uint32_t argLens[ 1 ] = { fullMsgU32Mask };
   GoQuickTrace_fullMsg( QuickTrace::defaultQuickTraceHandle,
                         &tsc,
                         msgId,
                         level,
                         numArgs,
                         argLens,
                         &counter );
}

void
qtGoTraceString( int level, int msgId, const char * string ) {
   uint64_t tsc;
   int numArgs = 1;
   uint32_t argLens[ 1 ] = { static_cast< uint32_t >( strlen( string ) + 1 ) };
   GoQuickTrace_fullMsg( QuickTrace::defaultQuickTraceHandle,
                         &tsc,
                         msgId,
                         level,
                         numArgs,
                         argLens,
                         ( void * )string );
}
}
