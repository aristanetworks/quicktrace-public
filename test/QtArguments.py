#!/usr/bin/env python3
# Copyright (c) 2012 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

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
