#!/usr/bin/env python3
# Copyright (c) 2016 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

# QuickTraceTest8 tests QuickTrace Files with different sized levels
# Each level is 1kb larger than the previous one starting with 8kb
# The same size (21 bytes) trace is logged to each level many times to fill them up
# Counting the number of trace messages in each buffer will tell us whether they were
# sized correctly according to the SizeSpec.

from __future__ import absolute_import, division, print_function
import os
import subprocess
import re

os.environ[ 'QTFILE' ] = 'qtt8.qt'
QTDIR = os.environ.get( 'QUICKTRACEDIR', '.qt' )
qtfile = '%s/%s' % ( QTDIR, os.environ[ 'QTFILE' ] )
subprocess.check_call( [ './QuickTraceTest8' ] )
qtout = subprocess.check_output( [ '/usr/bin/qttail',
                                   '-c', qtfile ],
                                 universal_newlines=True ).split( '\n' )

pattern = re.compile( r'Level(\d) regex match this!' )
traceCounters = [ 0 ] * 10
for trace in qtout:
   match = pattern.search( trace )
   if match:
      traceCounters[ int( match.group( 1 ) ) ] += 1

for i in range( 9 ):
   assert traceCounters[ i ] < traceCounters[ i + 1 ], \
   "Number of traces is not ascending"
   assert traceCounters[ i + 1 ] - traceCounters[ i ] in [ 48, 49 ], \
   "Numbers of traces not ascending by expected amount"
for i in range( 10 ):
   expect = ( ( 8 + i ) * 1024 - 256 - 8 ) // 21
   assert abs( traceCounters[ i ] - expect ) < 2, \
      "Incorrect number of messages %d expected around %d" \
      % ( traceCounters[ i ], expect )
