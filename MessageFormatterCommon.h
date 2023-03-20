// Copyright (c) 2022 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

namespace QuickTrace {

//
// -------- formatters for basic types provided by the QuickTrace package --------
//

unsigned
lengthBool( const unsigned char * );

unsigned
formatBool( const unsigned char * buf, std::ostream & os );

unsigned
lengthChar( const unsigned char * );

unsigned
formatChar( const unsigned char * buf, std::ostream & os );

unsigned
lengthDouble( const unsigned char * );

unsigned
formatDouble( const unsigned char * buf, std::ostream & os );

unsigned
lengthFloat( const unsigned char * );

unsigned
formatFloat( const unsigned char * buf, std::ostream & os );

unsigned
lengthString( const unsigned char * buf );

unsigned
formatString( const unsigned char * buf, std::ostream & os );

unsigned
lengthU8( const unsigned char * );

unsigned
formatU8( const unsigned char * buf, std::ostream & os );

unsigned
formatU8Hex( const unsigned char * buf, std::ostream & os );

unsigned
lengthU16( const unsigned char * );

unsigned
formatU16( const unsigned char * buf, std::ostream & os );

unsigned
formatU16Hex( const unsigned char * buf, std::ostream & os );

unsigned
lengthU32( const unsigned char * );

unsigned
formatU32( const unsigned char * buf, std::ostream & os );

unsigned
formatU32Hex( const unsigned char * buf, std::ostream & os );

unsigned
lengthU64( const unsigned char * );

unsigned
formatU64( const unsigned char * buf, std::ostream & os );

unsigned
formatU64Hex( const unsigned char * buf, std::ostream & os );

unsigned
lengthOpt( const MessageFormatter::LengthType & length,
           const unsigned char * buf );

unsigned
formatOpt( const MessageFormatter::FormatterType & formatter,
           const unsigned char * buf,
           std::ostream & os );

} // namespace QuickTrace
