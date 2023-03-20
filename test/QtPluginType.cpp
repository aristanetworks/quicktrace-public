// Copyright (c) 2022 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

// Creates a new formatter type for qttail to dlopen and trace..

#include <iostream>
#include <unistd.h>
#include <QuickTrace/QuickTrace.h>
namespace {

struct Dimension {
   Dimension( float len, float wid, float dep )
         : length( len ), width( wid ), depth( dep ) {}
   float length;
   float width;
   float depth;
};

const char *
formatString( const Dimension & ) {
   return "DIM";
}

void
put( QuickTrace::RingBuf * rb, const Dimension & dim ) {
   rb->push( dim.length );
   rb->push( dim.width );
   rb->push( dim.depth );
}

} // namespace

int
main( int argc, char ** argv ) {
   char const * outfile = getenv( "QTFILE" ) ?: "qttail_plugin_test.qt";
   QuickTrace::initialize( outfile );
   Dimension dim = Dimension( 50.00, 30.50, 10.50 );
   QTRACE0( "New Dimension is " << QVAR, dim );
}
