// Copyright (c) 2022 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#include <QuickTrace/MessageFormatter.h>
#include <QuickTrace/MessageFormatterCommon.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <vector>

unsigned
lengthDimension( const unsigned char * buf ) {
   unsigned len = QuickTrace::lengthFloat( buf );
   len += QuickTrace::lengthFloat( buf + len );
   len += QuickTrace::lengthFloat( buf + len );
   return len;
}

unsigned
formatDimension( const unsigned char * buf, std::ostream & os ) {
   os << "length: ";
   unsigned len = QuickTrace::formatFloat( buf, os );
   os << ", width: ";
   len += QuickTrace::formatFloat( buf + len, os );
   os << ", depth: ";
   len += QuickTrace::formatFloat( buf + len, os );
   return len;
}

class DimPlugin {
 public:
   DimPlugin() {
      QuickTrace::MessageFormatter::addNewFormattersById( "DIM", formatDimension );
      QuickTrace::MessageFormatter::addNewLengthById( "DIM", lengthDimension );
   }
};

static DimPlugin dimPlugin_;
