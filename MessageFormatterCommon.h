// Copyright (c) 2022 Arista Networks, Inc.
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
