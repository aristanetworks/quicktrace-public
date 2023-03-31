// Copyright (c) 2018, Arista Networks, Inc.
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

#include <unistd.h>
#include <iostream>
#include <QuickTrace/MessageParser.h>
#include <QuickTrace/QuickTrace.h>

namespace {

uint64_t readU64( const char *& p ) {
   char * end;
   uint64_t x = strtouq( p, &end, 0 );
   p = end;
   return x;
}

uint32_t readU32( const char *& p ) {
   char * end;
   uint32_t x = strtoul( p, &end, 0 );
   p = end;
   return x;
}

std::string readPString( const char *& p ) {
   uint32_t len;
   memcpy( &len, p, sizeof( len ) );
   p += sizeof( len );
   std::string ret( p, len - 1 );
   p += len;
   return ret;
}

// return a malloc'ed copy of what p points at up and until a whitespace
std::string readStringUntilWhitespace( const char *& p ) {
   const char * pp = p;
   while ( !isspace( *pp ) ) { pp++; }
   std::string ret( p, pp - p );
   p = pp;
   return ret;
}

void skipWhiteSpace( const char *& p ) {
   while ( isspace( *p ) ) { p++; }
}

} // namespace

MessageParser::MessageParser() :
      index_( 0 ), version_( 0 ), fd_( -1 ), base_( nullptr ), p_( nullptr ),
      end_( nullptr ) {
   // no-op
}

void MessageParser::initialize( const void * fpp, int fd ) {
   auto tfh = static_cast< const QuickTrace::TraceFileHeader * >( fpp );
   version_ = tfh->version;
   base_ = static_cast< const char * >( fpp );
   p_ = base_ + tfh->fileSize + tfh->fileTrailerSize;
   fd_ = fd;
   index_ = 0;
   recheck();
}

bool MessageParser::more() const {
   return p_ < end_;
}

Message MessageParser::parse() {
   if ( p_ < end_ && *p_ != '\0' ) {
      ++index_;
      uint64_t tsc = readU64( p_ );
      skipWhiteSpace( p_ );
      std::string filename = readStringUntilWhitespace( p_ );
      skipWhiteSpace( p_ );
      uint32_t lineNo = readU32( p_ );
      uint32_t msgId;
      if( version_ >= 3 ) {
         skipWhiteSpace( p_ );
         msgId = readU32( p_ );
      } else {
         msgId = index_;
      }
      p_++;
      std::string msg = readPString( p_ );
      std::string fmt = readPString( p_ );
      return Message( tsc, std::move( filename ), lineNo, std::move( msg ),
                      std::move( fmt ), msgId );
   } else {
      return Message();
   }
}

void MessageParser::recheck() {
   off_t size = lseek( fd_, 0, SEEK_END );
   end_ = static_cast< const char * >( base_ ) + size;
}
