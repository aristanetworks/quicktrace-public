// Copyright (c) 2022 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#include <cstdint>
#include <iostream>
#include <stdlib.h>
#include <QuickTrace/QuickTrace.h>
#include <QuickTrace/QuickTraceCommon.h>

uint64_t qt_rdtsc_mocker;

char const * outfile = getenv( "QTFILE" ) ?: "QtOffBitTest.out";

void
AddQt( float ft, int i ) {
   QTRACE1( "float ft" << QVAR << " " << QVAR, i << ft );
}

int
main( int argc, char const ** argv ) {
   qt_rdtsc_mocker = 0;
   QuickTrace::initialize( outfile );

   float f1 = 1.0;
   float f2 = 3.14;
   float f3 = 4.14;
   float f4 = 5.14;

   AddQt( f1, 1 );
   AddQt( f2, 2 );

   // We set 60th bit to 1. we'll shift right by 28 bits, so this resulted in the
   // 32nd bit being set in the lastTsc, leading to the message being switched off
   qt_rdtsc_mocker = 0x800000000000000; // 60th bit is 1

   AddQt( f3, 3 );
   AddQt( f4, 4 );
}
