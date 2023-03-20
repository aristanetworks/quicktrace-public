// Copyright (c) 2022 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#include <gtest/gtest.h>

#include <QuickTrace/QuickTrace.h>

const std::string QtDir = "/tmp";

using namespace QuickTrace;

TEST( FilePathTest, NotQtDirTests ) {
   // When multithreading is disabled and QuickTrace file name starts with
   // / ./ or ../ then we skip retrieving QUICKTRACEDIR

   MultiThreading m{ MultiThreading::disabled };
   std::string fnf{ "" };

   fnf = "./currentDirQt";
   auto qtd = QuickTrace::TraceHandle::getQtDir( m, fnf );
   EXPECT_EQ( qtd.value(), "" );
   fnf = "/rootDirQt";
   qtd = QuickTrace::TraceHandle::getQtDir( m, fnf );
   EXPECT_EQ( qtd.value(), "" );
   fnf = "../parentDirQt";
   qtd = QuickTrace::TraceHandle::getQtDir( m, fnf );
   EXPECT_EQ( qtd.value(), "" );
}

TEST( FilePathTest, QtDirTests ) {
   // When multithreading is disabled and QuickTrace file name does not start with
   // / ./ or ../ then the value of QUICKTRACEDIR is retreieved
   MultiThreading m{ MultiThreading::disabled };
   std::string fnf{ "" };

   fnf = ".onlySuffixQt";
   auto qtd = QuickTrace::TraceHandle::getQtDir( m, fnf );
   EXPECT_EQ( qtd.value(), QtDir );
   fnf = "noExtensionQt";
   qtd = QuickTrace::TraceHandle::getQtDir( m, fnf );
   EXPECT_EQ( qtd.value(), QtDir );
   fnf = "regular.qt";
   qtd = QuickTrace::TraceHandle::getQtDir( m, fnf );
   EXPECT_EQ( qtd.value(), QtDir );

   // When multithreading is enabled the QUICKTRACEDIR value is always retrieved
   m = MultiThreading::enabled;

   fnf = "./currentDirQt";
   qtd = QuickTrace::TraceHandle::getQtDir( m, fnf );
   EXPECT_EQ( qtd.value(), QtDir );
   fnf = "/rootDirQt";
   qtd = QuickTrace::TraceHandle::getQtDir( m, fnf );
   EXPECT_EQ( qtd.value(), QtDir );
   fnf = "../parentDirQt";
   qtd = QuickTrace::TraceHandle::getQtDir( m, fnf );
   EXPECT_EQ( qtd.value(), QtDir );
}

TEST( InvalidPathTest, InvalidPathTest ) {
   // QuickTrace file name can never be just .qt in non-multithreading case
   EXPECT_DEATH( QuickTrace::initialize( ".qt" ), "Filename cannot be .qt" );
}

int
main( int argc, char ** argv ) {
   ::testing::InitGoogleTest( &argc, argv );
   setenv( "QUICKTRACEDIR", QtDir.c_str(), 1 );
   return RUN_ALL_TESTS();
}
