#!/usr/bin/env python3
# Copyright (c) 2013, Arista Networks, Inc.
# All rights reserved.

# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:

# 	* Redistributions of source code must retain the above copyright notice,
#  	  this list of conditions and the following disclaimer.
# 	* Redistributions in binary form must reproduce the above copyright notice,
# 	  this list of conditions and the following disclaimer in the documentation
# 	  and/or other materials provided with the distribution.
# 	* Neither the name of Arista Networks nor the names of its contributors may
# 	  be used to endorse or promote products derived from this software without
# 	  specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL ARISTA NETWORKS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

# This test that passing wrong type of parameter to an existing message
# definition will trigger TypeError. Failing to do this would cause
# mysterious errors down the road (BUG51574).
#
# Note mixing numbers and None is not considered an error, since it's
# common to have them mixed.

from __future__ import absolute_import, division, print_function
import QuickTrace, re

qv = QuickTrace.Var

ret = QuickTrace.initialize( )
assert ret, "QuickTrace failed to initialize"

def _qt( level, args ):
   QuickTrace.traceWithFrameOffset( level, 2, *args )

def qt0( *args ):
   _qt( 0, args )

def testQ1( x=None ):
   qt0( "x=", qv( x ) )

def testQ2( x=None ):
   qt0( "x=", qv( x ) )

def testQ3( x=None ):
   # Add a good argument after the bad one to make sure
   # earlier error isn't overwritten.
   qt0( qv( "good" ), qv( x ), qv( 100 ) )

def testQ4( x=None ):
   qt0( "x=", qv( x ) )

def test( func, value, wrongValue ):
   func( value )
   # mixing None is OK
   func( None )
   try:
      func( wrongValue )
      assert False
   except TypeError:
      pass
   # This should work
   re.match( 'x', 'x' )

test( testQ1, 1.23, "foo" )
test( testQ2, 10, "foo" )
test( testQ3, 123, "foo" )

# test if we start with None, things should still work
testQ4( None )
testQ4( "foo" )
testQ4( 1.2 )
testQ4( True )
