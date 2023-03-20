// Copyright (c) 2010, 2011 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#include <iostream>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <pcrecpp.h>
#include <getopt.h>
#include <unistd.h>
#include <QuickTrace/MessageParser.h>
#include <QuickTrace/QuickTrace.h>

// qtctl modifies a QuickTrace trace file to enable or disable
// messages "on the fly".  It can also be used to show which messages
// are enabled or disabled.

using namespace QuickTrace;

class MessageIterator {
 public:
   MessageIterator( void * fpp, int fd ) {
      parser_.initialize( fpp, fd );
   }
   std::unique_ptr< Message > next() {
      if( !parser_.more() ) return std::unique_ptr< Message >();
      return std::unique_ptr< Message >( new Message( parser_.parse() ) );
   }
 private:
   MessageParser parser_;
};

void ferror( char const * what, char const * filename ) {
   std::cerr << what << " " << filename << ": "
             << errno << " " << strerror( errno ) << std::endl;
   exit( 1 );
}

static int numMsgCounters;

MsgCounter* msgCounters( void * fp ) {
   TraceFileHeader * tfh = (TraceFileHeader*) fp;
   numMsgCounters = ( tfh->fileHeaderSize - sizeof( TraceFileHeader ) ) /
                         sizeof( MsgCounter );
   return (MsgCounter*) ((char*)fp + tfh->firstMsgOffset);
}

void usage() {
      std::cerr 
         << "usage: qtctl show|on|off [-r <regexp>] [-m msgid] <file> ..." 
         << "qtctl controls or displays which QuickTrace statements are enabled\n"
         << "in a process.  The regexp and msgid arguments control limit which\n"
         << "statements are shown or changed.  The regexp is matched against the\n"
         << "filename and the message text.  It is in PCRE syntax." << std::endl;
      exit(1);
}

int main( int argc, char * const * argv ) {
   if( argc < 2 ) {
      usage();
   }
   
   char const * pattern = 0;

   int opt;
   opterr = 0;
   int msgIndex = -1;
   while( 1 ) {
      opt = getopt(argc, argv, "r:m:");
      switch( opt ) {
       case 'h':
         usage();
         break;
       case 'r':
         pattern = optarg;
         break;
       case 'm':
         msgIndex = atoi(optarg);
         break;
       case '?':
         std::cerr << "Unknown option: -" << (char)optopt  << std::endl;
         usage();
         break;
       case -1:
         goto done;
      }
   }
  done:

   if( optind+1 >= argc ) { usage(); }
   bool on = !strcmp( "on", argv[optind] );
   bool show = !strcmp( "show", argv[optind] );
   static pcrecpp::RE * regexp = pattern ? new pcrecpp::RE( pattern ) : 0;
   for( int i = optind+1 ; i < argc; ++i ) {
      char const * filename = argv[i];
      int fd = open( filename, O_RDWR|O_CREAT, 0777 );
      if( fd < 0 ) { ferror( "open", filename ); }
      off_t size = lseek( fd, 0, SEEK_END );
      void * m = mmap( 0, size, PROT_WRITE, MAP_SHARED, fd, 0 );
      if( m == MAP_FAILED ) { ferror( "mmap", filename ); }
      MessageIterator mi( m, fd );
      MsgCounter * mc  = msgCounters( m );
      while( auto msg = mi.next() ) {
         bool match = !pattern || regexp->PartialMatch( msg->msg() )
            || regexp->PartialMatch( msg->filename() );
         match = match && (msgIndex == -1 || msgIndex == (int)msg->msgId() );
         if( !match ) continue;
         uint32_t counterIndex = msg->msgId() % numMsgCounters;
         bool wasOff = mc[ counterIndex ].lastTsc & 0x80000000;
         bool isOff;
         if( !show ) {
            if( on ) {
               mc[ counterIndex ].lastTsc &= 0x7fffffff;
            } else {
               mc[ counterIndex ].lastTsc |= 0x80000000;
            }
            isOff = !on;
         } else {
            isOff = wasOff;
         }
         if( show || isOff != wasOff ){
            std::cout << msg->msgId() << " " << ( isOff ? "off " : "on  " )
                      << msg->filename() << ":" << msg->lineno()
                      << " " << msg->msg()
                      << " " << msg->fmt() << std::endl;
         }

      }
      close( fd );
   }
}

