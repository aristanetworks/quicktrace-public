#!/usr/bin/env python3
# Copyright (c) 2021 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

from __future__ import absolute_import, division, print_function

import os
import pytest
import signal
import sys
from QuickTrace import initialize
from QtTailTest import spawn, waitFor

qtfile = None

def setup_module():
   global qtfile
   qtfile = "/tmp/qttail_closestdout.qt"
   initialize( qtfile )

@pytest.mark.parametrize( "ignoreSighup", [ False, True ],
                          ids=[ "Default", "IgnoreHUP" ] )
def testCloseStdout( ignoreSighup ):
   """Verify that qttail exits when its stdout (which could be the write end of a
   pipe) is closed.

   The test runs in two variants, one where SIGHUP is ignored and one where it isn't.
   When SIGHUP is ignored, qttail exits with status 1. When SIGHUP is not ignored
   qttail exits due to receiving the HUP signal."""

   # start qttail and wait for it to open the trace file
   qttail = spawn( "qttail -x %s" % qtfile, ignore_sighup=ignoreSighup )
   qttail.expect( "--- opened %s" % qtfile )

   # now close the pipe to qttail and expect it to exit
   os.close( qttail.child_fd )
   waitFor( lambda: not qttail.isalive(), "qttail to exit" )
   rc = qttail.wait()
   if ignoreSighup:
      assert rc == 1
      assert qttail.signalstatus is None
   else:
      assert rc is None
      assert qttail.signalstatus == signal.SIGHUP

if __name__ == "__main__":
   sys.exit( pytest.main( sys.argv ) )
