#!/usr/bin/env python3
# Copyright (c) 2019 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

"""Verifies that qttail exits when all of the log files that it is tailing are
   deleted.
"""

from __future__ import absolute_import, division, print_function

import os
from QtTailTest import createQtfile, spawn, waitFor, waitForQtfile

# prepare
qtfile1 = "/tmp/qttail_delete_multifile_1.qt"
qtfile2 = "/tmp/qttail_delete_multifile_2.qt"
createQtfile( qtfile1 )
createQtfile( qtfile2 )
waitForQtfile( qtfile1 )
waitForQtfile( qtfile2 )

# delete multi file test
qttail = spawn( "qttail -x %s %s" % ( qtfile1, qtfile2 ) )
qttail.expect( "--- opened %s" % qtfile1 )
qttail.expect( "--- opened %s" % qtfile2 )
os.unlink( qtfile1 )
os.unlink( qtfile2 )
qttail.expect( "--- deleted %s" % qtfile1 )
qttail.expect( "--- deleted %s" % qtfile2 )
waitFor( lambda: not qttail.isalive(), "qttail to exit" )
