// Copyright (c) 2021 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.
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
