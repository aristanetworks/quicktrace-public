// Copyright (c) 2021, Arista Networks, Inc.
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

#include <QuickTrace/QtFmtGeneric.h>

namespace QuickTrace {

void
msgDesc( QuickTrace::MsgDesc & qtmd,
         const char * className,
         const char * funcName,
         const char * fmtString ) noexcept {
   size_t pos = 0;

   if ( className ) {
      if ( *className != '\0' ) {
         qtmd << className << "::";
      }
      qtmd << funcName << "() ";
   }

   while ( fmtString[ pos ] != '\0' ) {
      size_t startPos = pos;
      // Find the opening '{'
      for ( ; fmtString[ startPos ] != '\0'; startPos++ ) {
         if ( fmtString[ startPos ] == '{' ) {
            if ( fmtString[ startPos + 1 ] != '{' ) {
               break;
            } else {
               // we have a "<before>{{<after> sequence, copy before and skip
               // over the second '{'
               startPos++;
               qtmd << std::string_view( fmtString + pos, startPos - pos );
               pos = startPos + 1;
            }
         } else if ( fmtString[ startPos ] == '}' ) {
            if ( fmtString[ startPos + 1 ] != '}' ) {
               // Unbalanced closing '}'
               qtmd << std::string_view( fmtString + pos );
               return;
            }
            // We have "<before>}}<after>", copy "<before>}" and skip over
            // the extra '}'
            startPos++;
            qtmd << std::string_view( fmtString + pos, startPos - pos );
            pos = startPos + 1;
         }
      }

      // Copy everything we saw leading to '{' or \0'
      qtmd << std::string_view( fmtString + pos, startPos - pos );
      if ( fmtString[ startPos ] == '\0' ) {
         // No opening '{' found, this is a string without format specifiers
         return;
      }

      // Now find our closing '}'
      bool isHex = false;
      size_t endPos = startPos + 1;
      for ( ; fmtString[ endPos ] != '\0'; endPos++ ) {
         if ( fmtString[ endPos ] == '{' ) {
            // Unexpected '{' after finding an opening '{'
            qtmd << std::string_view( fmtString + startPos );
            return;
         }
         if ( fmtString[ endPos ] == '}' ) {
            break; // ok
         } else if ( fmtString[ endPos ] == ':' && !isHex ) {
            // format specification, only allow for {:x} currently
            if ( fmtString[ endPos + 1 ] == 'x' ) {
               isHex = true;
               endPos++; // Skip over the ':x'
            } else {
               // Unexpected character in format specification
               qtmd << std::string_view( fmtString + startPos );
               return;
            }
         } else {
            // Unexpected character in format specification
            qtmd << std::string_view( fmtString + startPos );
            return;
         }
      }

      if ( fmtString[ endPos ] == '\0' ) {
         // Failed to find a matching '}'
         qtmd << std::string_view( fmtString + startPos );
         return;
      }

      if ( isHex ) {
         qtmd << QHEX;
      } else {
         qtmd << QVAR;
      }
      pos = endPos + 1;
   }
}

} // namespace QuickTrace
