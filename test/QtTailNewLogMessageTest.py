#!/usr/bin/env python3
# Copyright (c) 2020 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

"""Verfifies that qttail can handle a new message id that appears later than
previously logged messages.
"""

from __future__ import absolute_import, division, print_function

from QuickTrace import initialize, trace0
from QtTailTest import spawn, waitFor

# setup
qtfile = "/tmp/qttail_newlogmessage.qt"
initialize( qtfile )

# start qttail and log the first message
qttail = spawn( "qttail -x %s" % qtfile )
qttail.expect( "--- opened %s" % qtfile )
msg1 = "first message"
trace0( msg1 )
qttail.expect( r'0 \+\d+ "%s"' % msg1 )

# log the second message (which will have a new message id)
msg2 = "second message"
trace0( msg2 )
qttail.expect( r'0 \+\d+ "%s"' % msg2 )
qttail.terminate( force=True )

# Sometimes signals sent through terminate() to exit qttail may get processed later.
# In that case terminate() will exit without verifying that qttail has succesfully
# exited. So, we add a waitFor to verify that qttail has exited.
waitFor( lambda: not qttail.isalive(), "qttail to exit" )
