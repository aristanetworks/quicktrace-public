// Copyright (c) 2016, Arista Networks, Inc.
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

#include <stdlib.h>
#include <QuickTrace/QuickTrace.h>

char const * outfile = getenv( "QTFILE" ) ?: "QtMaxStrLen.out";

int main( int argc, char const ** argv ) {

   QuickTrace::initialize( outfile, 0, NULL, 0, 80 );

   QTRACE1( "long string", "This string is longer than 24 characters and less "
            "than 80." );
   QTRACE1( "Now see, even though this line is far longer than 80 characters, "
            "it is part of the static portion. Thus, we can expect it to not "
            "be truncated.", "Good" );
   QTRACE1( "too long", "Clocking in at over 80 characters, this line is way "
            "way too long, so even with our new quick trace, this should "
            "fail" );

   /* We place a limit of 80 on maxStringLen. If a higher number is passed in,
    * the limit is set to 80 instead. Thus, the following tests should behave
    * identically to the ones above. */
   QuickTrace::initialize( outfile, 0, NULL, 0, 500 );

   QTRACE1( "500 should be 80 test", "Clocking in at over 80 characters, this "
            "line is way way too long, so even with our new quick trace, this "
            "should fail" );

   // Test what happens when we wrap around the 8k buffer. We'll use QT level 2
   // so as not to erase our messages on QT level 1.
   for( int i = 0; i < 10000; ++i ) {
      QTRACE2( "Wrap-around", "This message is more than 80 bytes long. Since "
               "only the first 80 bytes will be stored, we should wrap around "
               "the 8kb buffer after 100 messages. We used 10000 to be extra "
               "sure and also test multiple wrap-arounds." );
   }

   // Now let's write a message and see if anything weird happens
   QTRACE2( "wrap-around test", "Did we have any trouble with wrap-around?" );

}
