// Copyright (c) 2019, Arista Networks, Inc.
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

#include "QuickTraceFormatTest.h"
#include "QuickTraceFormatBasicTestFunctions.h"

TraceFuncReturn
traceBoolFalse() {
   bool val = false;
   TRACE( val );
   return { "False" };
}

TraceFuncReturn
traceBoolTrue() {
   bool val = true;
   TRACE( val );
   return { "True" };
}

TraceFuncReturn
traceChar() {
   TraceFuncReturn expected;
   for ( int i = 0; i <= 255; i++ ) {
      char ch = i;
      TRACE( ch );
      char str[ 8 ];
      switch ( ch ) {
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
         if ( isprint( ch ) ) {
            sprintf( str, "%c", ch );
         } else {
            sprintf( str, "\\x%02hhx", ch );
         }
      }
      expected.push_back( str );
   }
   return expected;
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

TraceFuncReturn
traceDouble() {
   // no decimal places
   double val1 = randomInt();
   TRACE( val1 );
   // 2 decimal places
   double val2 = 9876543.21;
   TRACE( val2 );
   // 6 decimal places
   double val3 = 0.123456;
   TRACE( val3 );
   // negative number with 3 decimal places
   double val4 = -43243543.124;
   TRACE( val4 );
   return { float_to_string( val1 ),
            float_to_string( val2 ),
            float_to_string( val3 ),
            float_to_string( val4 ) };
}

TraceFuncReturn
traceFloat() {
   // no decimal places
   float val1 = randomInt();
   TRACE( val1 );
   // 2 decimal places
   float val2 = 123456.75;
   TRACE( val2 );
   // negative number with 4 decimal places
   float val3 = -147321.4375;
   TRACE( val3 );
   return { float_to_string( val1 ),
            float_to_string( val2 ),
            float_to_string( val3 ) };
}

TraceFuncReturn
traceString() {
   std::string val = randomAsciiString();
   TRACE( val.c_str() );
   return { val };
}

TraceFuncReturn
traceU8() {
   uint8_t val = randomInt( 0, UINT8_MAX );
   TRACE( val );
   return { std::to_string( val ) };
}

TraceFuncReturn
traceU8Hex() {
   uint8_t val = randomInt( 0, UINT8_MAX );
   TRACE_HEX( val );
   std::ostringstream oss;
   oss << std::hex << static_cast< uint16_t >( val );
   return { oss.str() };
}

TraceFuncReturn
traceU16() {
   uint16_t val = randomInt( 0, UINT16_MAX );
   TRACE( val );
   return { std::to_string( val ) };
}

TraceFuncReturn
traceU16Hex() {
   uint16_t val = randomInt( 0, UINT16_MAX );
   TRACE_HEX( val );
   std::ostringstream oss;
   oss << std::hex << val;
   return { oss.str() };
}

TraceFuncReturn
traceU32() {
   uint32_t val = randomInt();
   TRACE( val );
   return { std::to_string( val ) };
}

TraceFuncReturn
traceU32Hex() {
   uint32_t val = randomInt();
   TRACE_HEX( val );
   std::ostringstream oss;
   oss << std::hex << val;
   return { oss.str() };
}

TraceFuncReturn
traceU64() {
   uint64_t val = randomInt();
   val *= 4000000011; // generate a large number that uses the upper 32 bits
   TRACE( val );
   return { std::to_string( val ) };
}

TraceFuncReturn
traceU64Hex() {
   uint64_t val = randomInt();
   val *= 4000000011; // generate a large number that uses the upper 32 bits
   TRACE_HEX( val );
   std::ostringstream oss;
   oss << std::hex << val;
   return { oss.str() };
}

TraceFuncReturn
traceCFormatA() {
   std::string val = randomAsciiString();
   QTRACE0( __FUNCTION__ << SEP1 << "%A" << SEP2, val.c_str() );
   return { val };
}

TraceFuncReturn
traceCFormatD() {
   // use unsigned integer data types for compatibility with qtcat
   uint32_t val32 = randomInt();
   QTRACE0( __FUNCTION__ << SEP1 << "%d" << SEP2, val32 );
   uint16_t val16 = val32;
   QTRACE0( __FUNCTION__ << SEP1 << "%hd" << SEP2, val16 );
   uint64_t val64 = val32;
   val64 *= 4000000011; // generate a large number that uses the upper 32 bits
   QTRACE0( __FUNCTION__ << SEP1 << "%ld" << SEP2, val64 );
   return { std::to_string( val32 ),
            std::to_string( val16 ),
            std::to_string( val64 ) };
}

TraceFuncReturn
traceCFormatIgnored() {
   // test several ignored format flags
   uint32_t val1 = randomInt();
   QTRACE0( __FUNCTION__ << SEP1 << "%#d" << SEP2, val1 );
   uint32_t val2 = randomInt();
   QTRACE0( __FUNCTION__ << SEP1 << "%-d" << SEP2, val2 );
   return { std::to_string( val1 ), std::to_string( val2 ) };
}

TraceFuncReturn
traceCFormatM() {
   std::string val = randomAsciiString();
   QTRACE0( __FUNCTION__ << SEP1 << "%M" << SEP2, val.c_str() );
   return { val };
}

TraceFuncReturn
traceCFormatP() {
   uint32_t val = randomInt();
   QTRACE0( __FUNCTION__ << SEP1 << "%p" << SEP2, val );
   std::ostringstream os;
   os << "0x" << std::hex << val;
   return { os.str() };
}

TraceFuncReturn
traceCFormatPrecision() {
   // a positive double with two decimal places
   double dblVal = randomInt( 1, 999 ) + randomInt( 1, 999 ) / 1000.0;
   QTRACE0( __FUNCTION__ << SEP1 << "%.2f" << SEP2, dblVal );
   char buf1[ 16 ];
   sprintf( buf1, "%.2f", dblVal );
   // a negative float with one decimal place
   float floatVal = -( randomInt( 1000, 9999 ) + randomInt( 1, 999 ) / 1000.0 );
   QTRACE0( __FUNCTION__ << SEP1 << "%3.1f" << SEP2, floatVal );
   char buf2[ 16 ];
   sprintf( buf2, "%3.1f", floatVal );
   return { buf1, buf2 };
}

TraceFuncReturn
traceCFormatPercent() {
   QTRACE0( __FUNCTION__ << SEP1 << "%%" << SEP2, QNULL );
   return { "%" };
}

TraceFuncReturn
traceCFormatSign() {
   // test several ignored format flags
   uint32_t val = randomInt();
   QTRACE0( __FUNCTION__ << SEP1 << "%+d" << SEP2, val );
   return { "+" + std::to_string( val ) };
}

TraceFuncReturn
traceCFormatU() {
   uint32_t val32 = randomInt();
   QTRACE0( __FUNCTION__ << SEP1 << "%u" << SEP2, val32 );
   uint16_t val16 = val32;
   QTRACE0( __FUNCTION__ << SEP1 << "%hu" << SEP2, val16 );
   uint64_t val64 = val32;
   QTRACE0( __FUNCTION__ << SEP1 << "%lu" << SEP2, val64 );
   return { std::to_string( val32 ),
            std::to_string( val16 ),
            std::to_string( val64 ) };
}

TraceFuncReturn
traceCFormatZeroFill() {
   uint32_t val32 = randomInt( 1, 999 );
   QTRACE0( __FUNCTION__ << SEP1 << "%08d" << SEP2, val32 );
   char buf1[ 16 ];
   sprintf( buf1, "%08d", val32 );
   // hexadecimal
   QTRACE0( __FUNCTION__ << SEP1 << "%06x" << SEP2, val32 );
   char buf2[ 16 ];
   sprintf( buf2, "%06x", val32 );
   // message with mixed zero-fill, space-fill and no fill
   QTRACE0( __FUNCTION__ << SEP1 << "%05d %6d %d" << SEP2,
            val32 << ( val32 + 1 ) << ( val32 + 2 ) );
   char buf3[ 32 ];
   sprintf( buf3, "%05d %6d %d", val32, val32 + 1, val32 + 2 );
   return { buf1, buf2, buf3 };
}

// for each new parameter type, implement and add one or more trace functions here
namespace QuickTrace {
TraceFunc traceFuncs[ 24 ] = {
   traceU8, // u
   traceU8Hex, // u
   traceU16, // s
   traceU16Hex, // s
   traceU32, // i
   traceU32Hex, // i
   traceU64, // q
   traceU64Hex, // q
   traceChar, // c
   traceString, // p
   traceBoolFalse, // b
   traceBoolTrue, // b
   traceFloat, // f
   traceDouble, // d
   traceCFormatA, // %A, %#A
   traceCFormatD, // %d, %ld, %hd
   traceCFormatIgnored, // %#, %- ...
   traceCFormatM, // %M
   traceCFormatP, // %p
   traceCFormatPrecision, // %.2f, ...
   traceCFormatPercent, // %%
   traceCFormatSign, // %+d
   traceCFormatU, // %u, %lu, %ld
   traceCFormatZeroFill, // %08d, %06p, ...
};
} // namespace QuickTrace
