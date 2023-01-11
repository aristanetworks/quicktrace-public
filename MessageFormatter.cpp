// Copyright (c) 2020, Arista Networks, Inc.
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

#include <cassert>
#include <QuickTrace/MessageFormatter.h>
#include <QuickTrace/MessageFormatterCommon.h>
#include <arpa/inet.h>
#include <cstring>
#include <dlfcn.h>
#include <iomanip>
#include <iostream>
#include <dirent.h>
#include <errno.h>

namespace QuickTrace {
//
// -------- formatters for basic types provided by the QuickTrace package --------
//

template< typename T >
std::string
float_to_string( T t );

unsigned
lengthBool( const unsigned char * ) {
   return 1;
}

unsigned
formatBool( const unsigned char * buf, std::ostream & os ) {
   os << ( *buf ? "True" : "False" );
   return lengthBool( buf );
}

unsigned
lengthChar( const unsigned char * ) {
   return 1;
}

unsigned
formatChar( const unsigned char * buf, std::ostream & os ) {
   char str[ 8 ];
   switch ( *buf ) {
    case '\n':
      strcpy( str, "\\n" );
      break;
    case '\r':
      strcpy( str, "\\r" );
      break;
    case '\t':
      strcpy( str, "\\t" );
      break;
    case '\'':
      strcpy( str, "\\'" );
      break;
    case '\\':
      strcpy( str, "\\\\" );
      break;
    default:
      if ( isprint( *buf ) ) {
         sprintf( str, "%c", *buf );
      } else {
         sprintf( str, "\\x%02hhx", *buf );
      }
   }
   os << str;
   return lengthChar( buf );
}

template< typename T >
std::string
float_to_string( T t ) {
   // format a float or double similar to Python
   std::string str = std::to_string( t );
   if ( str.find( '.' ) != std::string::npos ) {
      // remove trailing zeros after the period except one
      // e.g. 123.400000 -> 123.4; 123.000000 -> 123.0
      std::string::size_type p = str.find_last_not_of( '0' );
      if ( p != std::string::npos ) {
         str.erase( str[ p ] == '.' ? p + 2 : p + 1 );
      }
   }
   return str;
}

unsigned
lengthWallClock( const unsigned char * ) {
   // Wall-clock timestamp from 'struct timeval' is saved as two uint64_t fields
   return 2 * sizeof( uint64_t );
}

// The wall-clock time returned by gettimeofday is stored in RingBuf.
// Return formatted timestamp string from the RingBuf data.
unsigned
formatWallClockTimeStamp( const unsigned char * buf, std::ostream & os ) {
   struct timeval tv;
   memcpy( &tv, buf, sizeof( tv ) );
   char tBuf_[ 24 ]; // formatted string representation of tv_sec
   char usecBuf_[ 8 ]; // formatted string representation of tv_usec
   strftime( tBuf_, sizeof( tBuf_ ), "%Y-%m-%d %H:%M:%S.", localtime( &tv.tv_sec ) );
   sprintf( usecBuf_, "%06ld", tv.tv_usec );
   os << tBuf_ << usecBuf_;
   return lengthWallClock( buf );
}

unsigned
formatWallClockRingBuf( const unsigned char * buf, std::ostream & os ) {
   // WallClock data is used only for printing timestamp preceding the actual
   // message. Ignore this field while formatting message from RingBuf.
   return lengthWallClock( buf );
}

unsigned
lengthDouble( const unsigned char * ) {
   return 8;
}

unsigned
formatDouble( const unsigned char * buf, std::ostream & os ) {
   double val;
   memcpy( &val, buf, sizeof( val ) );
   if ( ( os.flags() & std::ios_base::floatfield ) == std::ios_base::fixed ) {
      // fixed decimal format was selected as part of a C format string
      // just pass the value directly into the stream to honor width and precision
      os << val;
   } else {
      // a decimal is being traced outside of a C format. Format it more python-like
      os << float_to_string( val );
   }
   return lengthDouble( buf );
}

unsigned
lengthFloat( const unsigned char * ) {
   return 4;
}

unsigned
formatFloat( const unsigned char * buf, std::ostream & os ) {
   float val;
   memcpy( &val, buf, sizeof( val ) );
   if ( ( os.flags() & std::ios_base::floatfield ) == std::ios_base::fixed ) {
      // fixed decimal format was selected as part of a C format string
      // just pass the value directly into the stream to honor width and precision
      os << val;
   } else {
      // a float is being traced outside of a C format. Format it more python-like
      os << float_to_string( val );
   }
   return lengthFloat( buf );
}

unsigned
lengthString( const unsigned char * buf ) {
   return 1U + *buf;
}

unsigned
formatString( const unsigned char * buf, std::ostream & os ) {
   os.write( reinterpret_cast< const char * >( buf + 1 ), *buf );
   return lengthString( buf );
}

unsigned
lengthU8( const unsigned char * ) {
   return 1;
}

unsigned
formatU8( const unsigned char * buf, std::ostream & os ) {
   os << static_cast< uint16_t >( *buf );
   return lengthU8( buf );
}

unsigned
formatU8Hex( const unsigned char * buf, std::ostream & os ) {
   os << std::hex << static_cast< uint16_t >( *buf ) << std::dec;
   return lengthU8( buf );
}

unsigned
lengthU16( const unsigned char * ) {
   return 2;
}

unsigned
formatU16( const unsigned char * buf, std::ostream & os ) {
   uint16_t val;
   memcpy( &val, buf, sizeof( val ) );
   os << val;
   return lengthU16( buf );
}

unsigned
formatU16Hex( const unsigned char * buf, std::ostream & os ) {
   uint16_t val;
   memcpy( &val, buf, sizeof( val ) );
   os << std::hex << val << std::dec;
   return lengthU16( buf );
}

unsigned
lengthU32( const unsigned char * ) {
   return 4;
}

unsigned
formatU32( const unsigned char * buf, std::ostream & os ) {
   uint32_t val;
   memcpy( &val, buf, sizeof( val ) );
   if ( ( os.flags() & std::ios_base::showpos ) == std::ios_base::showpos ) {
      // streams never print a + for positive numbers but with C formats we want
      // that if %+d is used
      os << '+';
   }
   os << val;
   return lengthU32( buf );
}

unsigned
formatU32Hex( const unsigned char * buf, std::ostream & os ) {
   uint32_t val;
   memcpy( &val, buf, sizeof( val ) );
   os << std::hex << val << std::dec;
   return lengthU32( buf );
}

unsigned
lengthU64( const unsigned char * ) {
   return 8;
}

unsigned
formatU64( const unsigned char * buf, std::ostream & os ) {
   uint64_t val;
   memcpy( &val, buf, sizeof( val ) );
   os << val;
   return lengthU64( buf );
}

unsigned
formatU64Hex( const unsigned char * buf, std::ostream & os ) {
   uint64_t val;
   memcpy( &val, buf, sizeof( val ) );
   os << std::hex << val << std::dec;
   return lengthU64( buf );
}

unsigned
lengthOpt( const MessageFormatter::LengthType & length,
           const unsigned char * buf ) {
   uint8_t val;
   memcpy( &val, buf, sizeof( val ) );
   auto len = lengthU8( buf );
   if ( val != 0 ) {
      len += length( buf + len );
   }
   return len;
}

unsigned
formatOpt( const MessageFormatter::FormatterType & formatter,
           const unsigned char * buf,
           std::ostream & os ) {
   uint8_t val;
   memcpy( &val, buf, sizeof( val ) );
   auto len = lengthU8( buf );
   if ( val != 0 ) {
      len += formatter( buf + len, os );
   } else {
      os << "OptionalNone";
   }
   return len;
}

MessageFormatter::MessageFormatter(
      uint32_t msgId, std::string filename, uint32_t lineno, std::string msg,
      std::string fmt ) :
      Message( 0, std::move( filename ), lineno, std::move( msg ),
               std::move( fmt ), msgId ) {
   init();
}

void
MessageFormatter::formatWallClock( const unsigned char * buf,
                                   std::ostream & os ) const {
   formatWallClockTimeStamp( buf, os );
}

void
MessageFormatter::addPlugin() {
   std::string libDir;
   #if defined (__LP64__)
      libDir = "/usr/lib64/";
   #else
      libDir = "/usr/lib/";
   #endif
   std::string path = libDir + "QtTailPlugin/";
   if ( getenv( "QT_FORMATTER_DIR" ) ) {
      path = getenv( "QT_FORMATTER_DIR" );
   }
   if ( path.back() != '/' ) {
      path += "/";
   }
   DIR * dir = opendir( path.c_str() );
   if ( dir == NULL ) {
      // plugin dir may not exist and there are no plugins to be added.
      if ( errno!= ENOENT ) {
         std::cerr << "Failed to open " << path
            << " with error " << strerror( errno ) << std::endl;
      }
      return;
   }
   struct dirent * libName = readdir( dir );
   for ( ; libName != NULL; libName = readdir( dir ) ) {
      std::string fName = libName->d_name;
      if ( fName.compare( ".." ) == 0 || fName.compare( "." ) == 0 ) {
         continue;
      }
      std::string libPath = path + libName->d_name;
      void * handle = dlopen( libPath.c_str(), RTLD_NOW );
      if ( !handle ) {
         std::cerr << "qttail failed to load library " << libPath
                   << " with error : " << dlerror() << std::endl;
         continue;
      }
   }
   closedir( dir );
}

int
MessageFormatter::format( const unsigned char * buf, std::ostream & os ) const {
   int n = 0;
   for ( auto & formatter : formatters_ ) {
      if ( formatter.second != nullptr ) {
         // static text or C formatter specifications
         formatter.first(
               reinterpret_cast< const unsigned char * >( formatter.second ),
               os );
      } else {
         // parameter to decode from the ring buffer
         unsigned l = formatter.first( buf + n, os );
         if ( l == 0 ) {
            return -1; // failed to decode parameter
         }
         n += l;
      }
   }
   return n;
}

int
MessageFormatter::length( const unsigned char * buf ) const {
   int n = 0;
   for ( auto & length : lengths_ ) {
      unsigned l = length( buf + n );
      if ( l == 0 ) {
         return -1; // failed to decode parameters
      }
      n += l;
   }
   return n;
}

void
MessageFormatter::addNewFormattersById( std::string name,
                                        MessageFormatter::FormatterType type ) {
   formattersById_.emplace( name, type );
}

void
MessageFormatter::addNewLengthById( std::string name,
                                    MessageFormatter::LengthType type ) {
   lengthsById_.emplace( name, type );
}

unsigned
MessageFormatter::applyCFormatSpec( const unsigned char * buf, std::ostream & os ) {
   CFormatSpec formatSpec;
   formatSpec.value = reinterpret_cast< intptr_t >( buf );
   if ( formatSpec.showPos ) {
      os << std::showpos;
   }
   if ( formatSpec.zeroPad ) {
      os << std::setfill( '0' );
   }
   if ( formatSpec.hasWidth ) {
      os << std::fixed << std::setw( formatSpec.width );
   }
   if ( formatSpec.hasPrecision ) {
      os << std::fixed << std::setprecision( formatSpec.precision );
   }
   return 0;
}

unsigned
MessageFormatter::formatStatic( const unsigned char * buf, std::ostream & os ) {
   os << reinterpret_cast< const char * >( buf );
   return 0;
}

MessageFormatter::FormatterType
MessageFormatter::selectFormatter( const std::string & fmt, bool isHex ) {
   // First check if we have an exactly matching formatter
   if ( isHex && hexFormattersById_.count( fmt ) ) {
      return hexFormattersById_.at( fmt );
   } else if ( !isHex && formattersById_.count( fmt ) ) {
      return formattersById_.at( fmt );
   }

   // No matching formatter, it could be that this is the first time we run into a
   // parameterized formatter.
   for ( const auto & [ key, parameterizedFormatter ] :
         parameterizedFormattersById() ) {
      // We perform a reverse search, starting at index 0 so this checks for
      // the prefix
      if ( fmt.rfind( key, 0 ) == 0 ) {
         // We found a parameterized formatter! Now resolve the inner formatter
         auto innerFmt = fmt.substr( key.size() );
         FormatterType innerFormatter = nullptr;
         if ( isHex && hexFormattersById_.count( innerFmt ) ) {
            innerFormatter = hexFormattersById_.at( innerFmt );
         } else if ( formattersById_.count( innerFmt ) ) {
            // Fall back to a non-hex formatter even if isHex is specified
            innerFormatter = formattersById_.at( innerFmt );
         }

         if ( !innerFormatter ) {
            return nullptr;
         }

         auto formatter = [ parameterizedFormatter, innerFormatter ](
                             const unsigned char * buf,
                             std::ostream & os ) -> unsigned {
            return parameterizedFormatter( innerFormatter, buf, os );
         };
         // Record formatter in our map, so that following lookups can directly
         // resolve it
         if ( isHex ) {
            hexFormattersById_.emplace( fmt, formatter );
         } else {
            formattersById_.emplace( fmt, formatter );
         }
         return formatter;
      }
   }

   // It could be that we failed to find a hex matching formatter, fall back to a
   // non-hex formatter in that case
   if ( isHex && formattersById_.count( fmt ) ) {
      return formattersById_.at( fmt );
   } else {
      return nullptr;
   }
}

MessageFormatter::LengthType
MessageFormatter::selectLength( const std::string & fmt ) {
   for ( const auto & [ key, parameterizedLen ] : parameterizedLengthsById_ ) {
      // We perform a reverse search, starting at index 0 so this check for
      // the prefix
      if ( fmt.rfind( key, 0 ) == 0 ) {
         auto innerFmt = fmt.substr( key.size() );
         auto innerLen = lengthsById_.at( innerFmt );
         return
            [ parameterizedLen, innerLen ]( const unsigned char * buf ) -> unsigned {
               return parameterizedLen( innerLen, buf );
            };
      }
   }
   return lengthsById_.at( fmt );
}

void
MessageFormatter::init() {
   const char * fmtPtr = fmt().c_str();
   tokenizedMsg_ = msg();
   char * msgPtr = &tokenizedMsg_[ 0 ];
   auto addFormatter = [&]( char conversionSpecifier ) {
      const char * fmtEnd = strchr( fmtPtr, ',' );
      if ( fmtEnd == nullptr ) {
         fmtEnd = fmtPtr + strlen( fmtPtr );
      }
      std::string fmt( fmtPtr, fmtEnd - fmtPtr );
      FormatterType formatter = nullptr;
      switch ( conversionSpecifier ) {
       case 'p':
       case 'x': {
         // Pick a hex formatter, or fallback to a default one if none are found
         formatter = selectFormatter( fmt, true );
         break;
       }
       default: {
         formatter = selectFormatter( fmt, false );
         break;
       }
      }
      if ( !formatter ) {
         std::cerr << "unsupported format: " << fmt << std::endl;
         exit( EXIT_FAILURE );
      }
      formatters_.push_back( std::make_pair( formatter, nullptr ) );

      MessageFormatter::LengthType lenById = selectLength( fmt );
      lengths_.push_back( lenById );
      fmtPtr = fmtEnd;
      if ( *fmtEnd == ',' ) {
         fmtPtr++;
      }
   };

   auto getFormatParamSpec = []( const char * param ) ->
         std::pair< unsigned, CFormatSpec > {
      // return the length of a format parameter
      // the parameter can have length or other modifiers, e.g. %08p or %#A
      // zero if this is not a valid format parameter
      unsigned len = 0;
      CFormatSpec cFormatSpec;
      cFormatSpec.value = 0;
      if ( param != nullptr ) {
         if ( param[ ++len ] == '%' ) {
            // valid, % -> %%
            len++;
         } else {
            while ( strchr( "#0- +'I", param[ len ] ) != nullptr ) {
               // flags
               switch ( param[ len++ ] ) {
                case '+':
                  cFormatSpec.showPos = 1;
                  break;
                case '0':
                  cFormatSpec.zeroPad = 1;
                  break;
                default:
                  // ignore all other flags, they are not supported
                  break;
               }
            }
            if ( isdigit( param[ len ] ) ) {
               // width
               cFormatSpec.hasWidth = 1;
               unsigned pos = len;
               while ( isdigit( param[ len ] ) ) {
                  len++;
               }
               cFormatSpec.width = std::stoi( std::string( param + pos,
                                                           len - pos ) );
            }
            if ( param[ len ] == '.' ) {
               // precision
               cFormatSpec.hasPrecision = 1;
               unsigned pos = ++len;
               while ( isdigit( param[ len ] ) ) {
                  len++;
               }
               if ( len > pos ) {
                  cFormatSpec.precision = std::stoi( std::string( param + pos,
                                                                  len - pos ) );
               }
            }
            while ( strchr( "hlqLjzZt", param[ len ] ) != nullptr ) {
               // length modifier, ignored as it has no effect
               len++;
            }
            if ( isalpha( param[ len ] ) ) {
               // conversion specifier, consider any alphabetic character valid
               len++;
            } else {
               len = 0; // indicates invalid format spec
            }
         }
      }
      return std::make_pair( len, cFormatSpec );
   };

   // The messages with wall-clock timestamp are using RingBuf to store the
   // timestamp data and use file line-number value of 0 to differentiate such
   // messages. Add special formatter to ignore these timestamp fields in
   // RingBuf while printing the message.
   // Note that the wall-clock time returned by gettimeofday is stored in
   // RingBuf in two different fields (tv_sec & tv_usec) instead of storing
   // in single 'struct timeval' format. If we store the timestamp data in
   // 'struct timeval' format, we need a new formatString to decode that data.
   // Such change will be incompatible with the older versions of qtcat/qttail
   // file as it cannot parse this new formatString. By storing the timestamp in
   // two different fields, we can reuse the existing formatString 'q'.
   static const char * wcFormatStr = "q,q,";
   static unsigned wcFormatStrLen = strlen( wcFormatStr );
   if ( lineno() == 0 && strncmp( fmtPtr, wcFormatStr, wcFormatStrLen ) == 0 ) {
      fmtPtr += wcFormatStrLen;
      MessageFormatter::FormatterType formatter = selectFormatter( "w", false );
      MessageFormatter::LengthType lenById = selectLength( "w" );
      formatters_.emplace_back( formatter, nullptr );
      lengths_.push_back( lenById );
   }
   while ( *msgPtr != '\0' ) {
      char * param =  strchr( msgPtr, '%' );
      unsigned fpLen;
      CFormatSpec cFormatSpec;
      std::tie( fpLen, cFormatSpec ) = getFormatParamSpec( param );
      while ( param != nullptr && fpLen == 0 ) {
         param = strchr( param + 1, '%' );
         std::tie( fpLen, cFormatSpec ) = getFormatParamSpec( param );
      }
      if ( param == nullptr ) {
         // no more parameters, consume remaining message
         formatters_.push_back( std::make_pair( formatStatic, msgPtr ) );
         msgPtr += strlen( msgPtr );
      } else {
         char conversionSpecifier = param[ fpLen - 1 ];
         if ( conversionSpecifier == '%' ) {
            formatters_.push_back( std::make_pair( formatStatic, msgPtr ) );
            param[ fpLen - 1 ] = '\0';
         } else {
            if ( *fmtPtr == '\0' ) {
               // there are no more parameters, abort
               break;
            }
            if ( param > msgPtr ) {
               formatters_.push_back( std::make_pair( formatStatic, msgPtr ) );
               *param = '\0';
            }
            if ( conversionSpecifier == 'p' ) {
               // p format is hex with a 0x prefix
               formatters_.push_back( std::make_pair( formatStatic, "0x" ) );
            }
            if ( cFormatSpec.value != 0 ) {
               formatters_.push_back(
                     std::make_pair( applyCFormatSpec,
                                     reinterpret_cast< const char * >(
                                           cFormatSpec.value ) ) );
            }
            addFormatter( conversionSpecifier );
            if ( cFormatSpec.value != 0 ) {
               // undo the effects of applyCFormatSpec
               formatters_.push_back(
                     std::make_pair( resetCFormatSpec,
                                     reinterpret_cast< const char * >(
                                           cFormatSpec.value ) ) );
            }
         }
         msgPtr = param + fpLen;
      }
   }
   if ( *msgPtr != '\0' || *fmtPtr != '\0' ) {
      // a message might have a different number of parameters than placeholders
      // due to programming errors. in these cases, for compatibility with qtcat,
      // emit the log message and the parameters separately
      lengths_.clear();
      formatters_.clear();
      formatters_.push_back( std::make_pair( formatStatic, &msg()[ 0 ] ) );
      formatters_.push_back( std::make_pair( formatStatic, " % (" ) );
      for ( fmtPtr = fmt().c_str(); *fmtPtr != '\0'; ) {
         if ( fmtPtr != fmt().c_str() ) {
            formatters_.push_back( std::make_pair( formatStatic, "," ) );
         }
         addFormatter( 's' );
      }
      formatters_.push_back( std::make_pair( formatStatic, ") " ) );
   }
}

unsigned
MessageFormatter::resetCFormatSpec( const unsigned char *, std::ostream & os ) {
   os << std::noshowpos << std::setfill( ' ' ) << std::defaultfloat;
   return 0;
}

using ParameterizedFormatterMap = MessageFormatter::ParameterizedFormatterType;
const std::unordered_map< std::string, ParameterizedFormatterMap >
MessageFormatter::parameterizedFormattersById() {
   static std::unordered_map< std::string, ParameterizedFormatterMap > map;
   if ( map.empty() ) {
      // If the map is being initialized, ensure that the parameterized format
      // strings do not overlap with  the non-parameterized ones.
      map = { { "OPT", formatOpt } };
      for ( const auto & [ nonParamFmt, nonParamFormatter ] : formattersById_ ) {
         for ( const auto & [ fmt, formatter ] : map ) {
            assert( nonParamFmt.rfind( fmt, 0 ) == std::string::npos );
         }
      }
   }
   return map;
}

std::unordered_map< std::string, MessageFormatter::FormatterType >
   MessageFormatter::formattersById_{
      // see qtcat for supported formats
      { "u", formatU8 }, // QuickTrace
      { "s", formatU16 }, // QuickTrace
      { "i", formatU32 }, // QuickTrace
      { "q", formatU64 }, // QuickTrace
      { "c", formatChar }, // QuickTrace
      { "p", formatString }, // QuickTrace
      { "b", formatBool }, // QuickTrace
      { "f", formatFloat }, // QuickTrace
      { "d", formatDouble }, // QuickTrace
      { "w", formatWallClockRingBuf } // Used internally by qttail
   };

std::unordered_map< std::string, MessageFormatter::FormatterType >
   MessageFormatter::hexFormattersById_{ { "u", formatU8Hex },
                                         { "s", formatU16Hex },
                                         { "i", formatU32Hex },
                                         { "q", formatU64Hex } };

std::unordered_map< std::string, MessageFormatter::LengthType >
   MessageFormatter::lengthsById_{
      { "u", lengthU8 },
      { "s", lengthU16 },
      { "i", lengthU32 },
      { "q", lengthU64 },
      { "c", lengthChar },
      { "p", lengthString },
      { "b", lengthBool },
      { "f", lengthFloat },
      { "d", lengthDouble },
      { "w", lengthWallClock }
   };

const std::unordered_map< std::string, MessageFormatter::ParameterizedLengthType >
   MessageFormatter::parameterizedLengthsById_{ { "OPT", lengthOpt } };
} // namespace QuickTrace
