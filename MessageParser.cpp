// Copyright (c) 2018 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

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
