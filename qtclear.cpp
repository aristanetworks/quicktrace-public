// Copyright (c) 2010, 2011 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#include <iostream>
#include <QuickTrace/QuickTrace.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// This program clears all of the QuickTrace message counters in one
// or more quicktrace output files.  It simply opens the file, mmaps
// enough of it to include all of the counters, and then clears them.

using namespace QuickTrace;
int main( int argc, char const ** argv ) {
   if( argc < 2 ) {
      std::cerr << "usage: qtclear <file> ..." << std::endl;
      exit(1);
   }

   for( int i = 1 ; i < argc; ++i ) {
      char const * filename = argv[i];
      int fd = open( filename, O_RDWR, 0777 );
      if( fd < 0 ) {
         std::cerr << "open " << filename << ": "
                   << errno << " " << strerror( errno ) << std::endl;
         return 1;
      }
      struct stat buf;
      if ( fstat( fd, &buf ) < 0 ) {
         close( fd );
         std::cerr << "fstat " << filename << ": "
                   << errno << " " << strerror( errno ) << std::endl;
         return 1;
      }
      int size = 16384;
     retry:
      if ( size > buf.st_size ) {
         close( fd );
         std::cerr << filename << ": likely not a qt file "
                   << std::endl;
         return 1;
      }
      void * m = mmap( 0, size, PROT_WRITE, MAP_SHARED, fd, 0 );
      if( m == MAP_FAILED ) {
         std::cerr << "mmap " << filename << ": "
                   << errno << " " << strerror( errno ) << std::endl;
         close( fd );
         return 1;
      }
      TraceFileHeader * tf = (TraceFileHeader*) m;
      int fhs = tf->fileHeaderSize;
      if( fhs > size ) {
         munmap( m, size );
         size = fhs;
         goto retry;
      }
      memset( (char*)m + (tf->firstMsgOffset), 0,
              tf->fileHeaderSize - tf->firstMsgOffset );
      // Call msync to update the file modification time.
      msync( m, size, MS_ASYNC );
      munmap( m, size );
      close( fd );
   }
}
