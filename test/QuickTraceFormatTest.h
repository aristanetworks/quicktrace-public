// Copyright (c) 2019 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

// Test library for qttail format and stress tests.
//
// The format test verifies that both qtcat and qttail can properly decode
// each and every parameter type. First, it creates a qt file and for each parameter
// type logs a corresponding message, while simultaneously tailing the file with
// qttail. For each message logged it verifies that qttail printed out the expected
// parameter representation.
// After that, it calls qtcat to dump the entire file, and again for each message
// verifies that the parameter if in the expected representation.
//
// The stress test (in package QuickTraceTest) makes use of the functions that
// start and stop qttail and read its output.

#ifndef TEST_QUICKTRACEFORMATTEST_H
#define TEST_QUICKTRACEFORMATTEST_H

#include <list>
#include <string>
#include <QuickTrace/QuickTrace.h>

#define SEP1 "|>"
#define SEP2 "<|"
#define TRACE( P ) QTRACE0( __FUNCTION__ << SEP1 << QVAR << SEP2, P )
#define TRACE_HEX( P ) QTRACE0( __FUNCTION__ << SEP1 << QHEX << SEP2, P )

typedef std::list< std::string > TraceFuncReturn;
typedef TraceFuncReturn ( *TraceFunc )();

void verifyLine( const std::string & stage,
                 const std::string & actual,
                 const std::string & expected );
std::list< std::string > testQtTail( const char * qtFileName,
                                     TraceFunc * traceFuncs,
                                     unsigned numTraceFuncs );

std::string initializeQuickTrace( const char * fileName, unsigned bufferSize,
                                  bool multiThreading = false );
void killProcess();
void pabort( const char * msg );
void pexit( const char * msg );
std::string readQtLine( int fd, bool failOnEof = true, unsigned timeoutMs = 600000 );
std::string randomAsciiString( unsigned minLength = 16, unsigned maxLength = 0 );
unsigned randomInt( unsigned minVal = 0, unsigned maxVal = UINT32_MAX );
int runProcess( const char * file, const char ** argv );
int runQtTail( const char * qtFileName, bool debug = false );
void signalProcess( int signum );
TraceFuncReturn traceStatic();

#endif
