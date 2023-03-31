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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QuickTrace/QuickTrace.h>
#include <QuickTrace/QtFmtGeneric.h>

using namespace QuickTrace;

std::string
testMsgDesc( const char * className,
             const char * funcName,
             const std::string & srcString ) {
   MsgId msg( 0 );
   auto fileName = "TestFmtMsgdesc-%d";
   QuickTrace::initialize( fileName );
   MsgDesc qmd( QuickTrace::theTraceFile, &msg, "DummyFile", 0 );
   auto beforePtr = qmd.ptr();
   // This will serialize our format spec into a printf-style format string
   msgDesc( qmd, className, funcName, srcString.c_str() );
   // This injects a null byte
   qmd.finish();
   std::string resultString( beforePtr );
   QuickTrace::close();
   return resultString;
}

std::string
testFormatString( const std::string & srcString ) {
   return testMsgDesc( nullptr, "", srcString );
}

TEST( FmtMsgTest, SimpleSuccessFullTests ) {
   std::vector< std::pair< std::string, std::string > > testData{
      { "testString", "testString" },
      { "{}", "%s" },
      { "{:x}", "%x" },
      { "{{", "{" },
      { "}}", "}" },
      { "before{{after", "before{after" },
      { "before}}after", "before}after" },
      { "before{}after", "before%safter" },
      { "before{:x}after", "before%xafter" },
      { "{{}}", "{}" },
      { "{}a{}b{}c{}", "%sa%sb%sc%s" },
      { "{}{:x}{{}}", "%s%x{}" },
   };
   for ( const auto & [ srcString, expectedString ] : testData ) {
      EXPECT_EQ( testFormatString( srcString ), expectedString );
   }
}

// Verify that we correctly handle errors for invalid format strings: we'll try
// to format the string as far as we can, before noticing an error and copying
// the rest of the format string verbatim.
TEST( FmtMsgTest, ParseErrorTests ) {
   std::vector< std::pair< std::string, std::string > > testData{
      { "{", "{" },
      { "}", "}" },
      { "a{", "a{" },
      { "a{}}", "a%s}" },
      { "a}{}", "a}{}" },
      { "a{b", "a{b" },
      { "a{b}}", "a{b}}" },
      { "a{:}}", "a{:}}" },
      { "a{:e}}", "a{:e}}" },
      { "{:xx}", "{:xx}" },
      { "{:x:x}", "{:x:x}" },
   };
   for ( const auto & [ srcString, expectedString ] : testData ) {
      EXPECT_EQ( testFormatString( srcString ), expectedString );
   }
}

TEST( FmtMsgTest, TestClassAndFuncName ) {
   // This test focuses on exercising  the logic that includes the class name and
   // the function in the trace message
   EXPECT_EQ( testMsgDesc( "Class", "func", "trace" ), "Class::func() trace" );
   // _RAW version
   EXPECT_EQ( testMsgDesc( nullptr, "", "trace" ), "trace" );
   // _FUNC version
   EXPECT_EQ( testMsgDesc( "", "func", "trace" ), "func() trace" );
}

int
main( int argc, char ** argv ) {
   ::testing::InitGoogleTest( &argc, argv );
   return RUN_ALL_TESTS();
}
