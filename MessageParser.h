// Copyright (c) 2018 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#ifndef QUICKTRACE_MESSAGEPARSER_H
#define QUICKTRACE_MESSAGEPARSER_H

#include <string>

struct Message {
 public:
   Message() : tsc_( 0 ), msgId_( 0 ), lineno_( 0 ) {}
   Message( uint64_t tsc, std::string filename, uint32_t lineno, std::string msg,
           std::string fmt, uint32_t msgId ) :
         tsc_( tsc ), msgId_( msgId ), lineno_( lineno ),
         filename_( std::move( filename ) ), msg_( std::move( msg ) ),
         fmt_( std::move( fmt ) ) {}
   uint64_t tsc() { return tsc_; }
   const std::string & filename() const { return filename_; }
   uint32_t lineno() { return lineno_; }
   const std::string & msg() const { return msg_; }
   const std::string & fmt() const { return fmt_; }
   uint32_t msgId() { return msgId_; }
 private:
   uint64_t tsc_;
   uint32_t msgId_;
   uint32_t lineno_;
   std::string filename_;
   std::string msg_;
   std::string fmt_;
};

class MessageParser {
public:
   MessageParser();

   void initialize( const void * fpp, int fd );
   bool more() const;
   Message parse();
   void recheck();

private:
   uint32_t index_;
   uint32_t version_;
   int fd_;
   const char * base_;
   const char * p_;
   const char * end_;
};

#endif
