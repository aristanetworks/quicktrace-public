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

void
traceLongString( int level, const char * string ) {
   QTRACE_H_LONGSTRING( QuickTrace::theTraceFile, level, string );
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

void
qtTraceLongString( int level, const char * string ) {
   traceLongString( level, string );
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
