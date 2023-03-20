#!/usr/bin/env python3
# Copyright (c) 2020 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

"""Verifies that qttail does not miss any message when a ring buffer rolls over.
This test creates 100 messages of size 17 in ring buffer 9 (which is 1k of size)
for a total of 1,700 bytes. This means the buffer must roll over once.
"""

from __future__ import absolute_import, division, print_function

from QuickTrace import initialize, trace9, Var as qv
from QtTailTest import spawn

# setup
qtfile = "/tmp/qttail_rollover.qt"
initialize( qtfile, "1,1,1,1,1,1,1,1,1,1" )

# rollover test
qttail = spawn( "qttail -x %s" % qtfile )
qttail.expect( "--- opened %s" % qtfile )
for i in range( 10 ):
   for j in range( 10 ):
      trace9( "test rollover", qv( i * 10 + j ) )
   for j in range( 10 ):
      qttail.expect( r'9 \+\d+ "test rollover %d"' % ( i * 10 + j ) )
