#!/usr/bin/env python3
# Copyright (c) 2020 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

"""Verfifies that qttail dumps messages from all ring buffers in the order they
were written, when tailing a single file.
"""

from __future__ import absolute_import, division, print_function

from QuickTrace import initialize, trace0, trace1, trace2, trace3, trace4, trace5, \
                       trace6, trace7, trace8, trace9
from QtTailTest import spawn, waitFor

# setup
qtfile = "/tmp/qttail_singlefile.qt"
initialize( qtfile )

# tail single file test
qttail = spawn( "qttail -x %s" % qtfile )
qttail.expect( "--- opened %s" % qtfile )
msg = "basic test message"
traceLevels = ( trace0, trace1, trace2, trace3, trace4, trace5, trace6, trace7,
                trace8, trace9 )
for trace in traceLevels:
   trace( msg )
for level in range( len( traceLevels ) ):
   qttail.expect( r'%d \+\d+ "%s"' % ( level, msg ) )
assert qttail.terminate()

# cat single file test
qttail = spawn( "qttail -c %s" % qtfile )
for level in range( len( traceLevels ) ):
   qttail.expect( r'%d \+\d+ "%s"' % ( level, msg ) )
waitFor( lambda: not qttail.isalive(), "qttail to exit" )
rc = qttail.wait()
assert rc == 0
