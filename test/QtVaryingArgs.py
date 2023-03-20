#!/usr/bin/env python3
# Copyright (c) 2021 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

from __future__ import absolute_import, division, print_function

from os import getpid, remove
import subprocess

import QuickTrace

qtfile = '/tmp/varying-args-%s.qt' % getpid()

def traceUnwisely( args ):
   # pylint: disable=map-builtin-not-iterating
   QuickTrace.trace2( *map( QuickTrace.Var, args ) )

try:
   QuickTrace.initialize( qtfile )
   traceUnwisely( [ 'Hello', 'World' ] )
   QuickTrace.trace2( "This one is", QuickTrace.Var( "OK" ) )
   traceUnwisely( [ 'Hi', 'There', 'How', 'Do', 'You', 'Do?' ] )
   traceUnwisely( [ 'Three', 'Fencepost', 'Bloop' ] )
   traceUnwisely( [ 'One Fencepost Sloop' ] )
   QuickTrace.trace2( "This one is", QuickTrace.Var( "also OK" ) )
   traceUnwisely( [ 'Goodbye', 'World' ] )
finally:
   # invoke qttail to pretty-print qtrace file and capture stdout output
   output = subprocess.check_output( [ "/usr/bin/qttail", '-c', qtfile ],
                                     universal_newlines=True )

   # Output that we expect to see:
   # Since the first call to traceUnwisely() used 2 args, extra args are
   # ignored and shorter args are padded with None:
   expected = ( "Hello World",
                "This one is OK",
                "Hi There",
                "Three Fencepost",
                "One Fencepost Sloop None",
                "This one is also OK",
                "Goodbye World" )
   for s in expected:
      assert '"' + s + '"' in output, s + " not in:\n" + output

   remove( qtfile )

