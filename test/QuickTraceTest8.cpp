// Copyright (c) 2017 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

// Test trace buffers of different sizes

// Each trace level should hold more messages than the last.
// We can verify this with qtcat, by counting the number of times
// each trace appears in th output.

// Each trace is 21 bytes:
//   U64 tsc
//   U32 msgId
//   U64 custom parameter
//   U8 len/

// so each trace ring should store an aditional ~48 messages over the
// previous one

#include <QuickTrace/QuickTrace.h>
#include <stdlib.h>

using namespace QuickTrace;

static const char * outfile = getenv( "QTFILE" ) ?: "qtt8.out";
static const uint64_t NUM_LOOPS = 1000;

int
main( int argc, char * argv[] ) {
   QuickTrace::SizeSpec ss = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 };
   QuickTrace::initialize( outfile, &ss );

   for ( uint64_t i = 0; i < NUM_LOOPS; ++i ) {
      QTRACE0( "Level0 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE1( "Level1 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE2( "Level2 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE3( "Level3 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE4( "Level4 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE5( "Level5 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE6( "Level6 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE7( "Level7 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE8( "Level8 regex match this!" << QVAR, i * 10 + 1 );
      QTRACE9( "Level9 regex match this!" << QVAR, i * 10 + 1 );
   }

   return 0;
}
