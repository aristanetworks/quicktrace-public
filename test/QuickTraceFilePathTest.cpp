// Copyright (c) 2022, Arista Networks, Inc.
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
