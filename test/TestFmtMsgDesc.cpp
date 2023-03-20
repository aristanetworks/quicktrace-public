// Copyright (c) 2021 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

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
