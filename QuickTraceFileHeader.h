// Copyright (c) 2025, Arista Networks, Inc.
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

#ifndef QUICKTRACE_QUICKTRACEFILEHEADER_H
#define QUICKTRACE_QUICKTRACEFILEHEADER_H

#include <stdint.h>

namespace QuickTrace {

struct SizeSpec {
   // Sizes, in kilobytes of the individual trace buffers
   // So for example, if you construct one  like this:
   //   SizeSpec s = {1000,100,1, 1,1,1,1,1,1,1000};
   // you'll allocate a megabyte for 0 and 9, 100K for 1, and 1K for
   // everything else
   static constexpr int SIZE = 10;
   uint32_t sz[ SIZE ];

   bool operator==( const SizeSpec &other ) const{
      for ( int i = 0; i < SIZE; i++ ){
         if( sz[i] != other.sz[i] ){
            return false;
         }
      }
      return true;
   }
};

struct TraceFileHeader {
   uint32_t version;
   uint32_t fileSize;
   uint32_t fileHeaderSize;
   uint32_t fileTrailerSize;
   uint32_t firstMsgOffset;
   uint32_t logCount;
   uint64_t tsc0;
   double monotime0;
   uint64_t tsc1;
   double monotime1;
   double utc1;
   SizeSpec logSizes;
};

} // namespace QuickTrace

#endif // QUICKTRACE_QUICKTRACEFILEHEADER_H
