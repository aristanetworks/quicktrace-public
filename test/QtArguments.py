#!/usr/bin/env python3
# Copyright (c) 2012, Arista Networks, Inc.
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

# Test quicktrace with a frame offset, verifying it allows to bypass wrappers. Also
# test all argument types

from __future__ import absolute_import, division, print_function
from os import getpid, remove
import sys

import QuickTrace

class DummyClass( object ):
   def __init__( self, me ):
      self.me = me

class DummyClassWithRepr( DummyClass ):
   def __str__( self ):
      return str( self.me )

dummy = DummyClass( "foo ")
dummyWithRepr = DummyClassWithRepr( "bar" )
oversizeStr = "maxargsizeis25padpadpadpadpad"
oversizeList = [ 'echo', 'sync', '0', 'wxyz' ]
qtfile = '/tmp/args-%s.qt' % getpid()

MAX_VALUES = 2

# Equivalent to 'long' in Python 2, but doesn't cause a syntax error in Python 3.
# Evaluates to 'int' in Python 3.
longType = type( sys.maxsize + 1 )

traceMsg = [ None, 100, longType( 100 ), "foo", True, 1.0, dummy, dummyWithRepr,
             oversizeStr, oversizeList, QuickTrace.Var( None ),
             QuickTrace.Var( 100 ), QuickTrace.Var( longType( 100 ) ),
             QuickTrace.Var( "foo" ), QuickTrace.Var( True ), QuickTrace.Var( 1.0 ),
             QuickTrace.Var( dummy ), QuickTrace.Var( dummyWithRepr ),
             QuickTrace.Var( oversizeStr ), QuickTrace.Var( oversizeList ) ]

def traceWrapper( msg, msgCount, frameOffset=False ):
   offset = int( frameOffset )
   QuickTrace.traceWithFrameOffset( 0, offset, *msg )
   lineno = sys._getframe( offset ).f_lineno
   if not frameOffset:
      lineno -= 1 # the trace call was actually the line before
   if msgCount:
      assert ( id( sys._getframe( offset ).f_code ), lineno ) in QuickTrace._msgId
   assert len( QuickTrace._msgId ) == msgCount

try:
   count = 0
   # this messages shouldn't go anywhere as QuickTrace was not initialized
   traceWrapper( traceMsg, count )
   QuickTrace.initialize( qtfile )
   # first message is always new
   count += 1
   traceWrapper( traceMsg, count )
   # this next message shouldn't be new as it looks like coming from the same code
   # (the wrapper)
   traceWrapper( traceMsg, count )
   # now we account for the offset, so the message logged here should look new
   count += 1
   traceWrapper( traceMsg, count, 1 )
   # now we do the same again, this next message should also look new
   count += 1
   traceWrapper( traceMsg, count, 1 )
finally:
   remove( qtfile )
