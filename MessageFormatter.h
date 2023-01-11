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

#ifndef QUICKTRACE_MESSAGEFORMATTER_H
#define QUICKTRACE_MESSAGEFORMATTER_H

#include <unordered_map>
#include <vector>
#include <functional>
#include <QuickTrace/MessageParser.h>

namespace QuickTrace {

class MessageFormatter : public Message {
public:
   MessageFormatter() {}
   MessageFormatter( uint32_t msgId, std::string filename, uint32_t lineno,
                     std::string msg, std::string fmt );

   // this class is not safe to copy or assign due to formatters_ containing
   // copies of char * returned from the std::string tokenizedMsg_ opoerator[]
   MessageFormatter( const MessageFormatter & ) = delete;
   MessageFormatter & operator=( const MessageFormatter & ) = delete;

   int format( const unsigned char * buf, std::ostream & os ) const;
   void formatWallClock( const unsigned char * buf, std::ostream & os ) const;
   int length( const unsigned char * buf ) const;

   using FormatterType =
      std::function< unsigned( const unsigned char *, std::ostream & ) >;
   using LengthType = std::function< unsigned( const unsigned char * buf ) >;

   // adds new formatters to formattersById_ from plugin.
   static void addNewFormattersById( std::string name, FormatterType type );
   // adds new formatter lengths to lengthById_ from plugin.
   static void addNewLengthById( std::string name, LengthType type );
   // loads plugin to add new formatters from QT_FORMATTER_DIR env var if available
   // else from a prespecified plugin directory.
   static void addPlugin();

   typedef unsigned ( *ParameterizedFormatterType )( const FormatterType & formatter,
                                                     const unsigned char * buf,
                                                     std::ostream & os );
   typedef unsigned ( *ParameterizedLengthType )( const LengthType & len,
                                                  const unsigned char * buf );

 private:
   union CFormatSpec {
      intptr_t value;
      struct {
         int showPos:1;
         int zeroPad:1;
         int hasWidth:1;
         int hasPrecision:1;
         int width:8;
         int precision:8;
      };
   };
   static_assert( sizeof( CFormatSpec ) == sizeof( void * ) );

   static unsigned applyCFormatSpec( const unsigned char * buf, std::ostream & os );
   static unsigned formatStatic( const unsigned char * buf, std::ostream & os );

   FormatterType selectFormatter( const std::string & fmt, bool isHex );
   LengthType selectLength( const std::string & fmt );

   void init();

   static unsigned resetCFormatSpec( const unsigned char * buf, std::ostream & os );

   // The map will be updated once we resolve parameterized formatters
   static std::unordered_map< std::string, FormatterType > formattersById_;
   static std::unordered_map< std::string, FormatterType > hexFormattersById_;
   static std::unordered_map< std::string, LengthType > lengthsById_;

   static const std::unordered_map< std::string, ParameterizedFormatterType >
   parameterizedFormattersById();
   static const std::unordered_map< std::string, ParameterizedLengthType >
      parameterizedLengthsById_;

   std::vector< std::pair< FormatterType, const char * > > formatters_;
   std::vector< LengthType > lengths_;
   std::string tokenizedMsg_;
};
} // namespace QuickTrace
#endif
