#!/usr/bin/env python3
# Copyright (c) 2019 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

"""Verifies that qttail exits when then only log file that it is tailing is deleted.
"""

from __future__ import absolute_import, division, print_function

import os
from QuickTrace import initialize
from QtTailTest import spawn, waitFor

qtfile = "/tmp/qttail_delete.qt"
initialize( qtfile, "1,1,1,1,1,1,1,1,1,1" )

# delete single file test
qttail = spawn( "qttail -x %s" % qtfile )
qttail.expect( "--- opened %s" % qtfile )
os.unlink( qtfile )
qttail.expect( "--- deleted %s" % qtfile )
waitFor( lambda: not qttail.isalive(), "qttail to exit" )
