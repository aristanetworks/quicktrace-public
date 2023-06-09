#!/usr/bin/env python3
# Copyright (c) 2021, Arista Networks, Inc.
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
