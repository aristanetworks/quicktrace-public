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
