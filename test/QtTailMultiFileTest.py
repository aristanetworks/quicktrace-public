#!/usr/bin/env python3
# Copyright (c) 2020 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

"""Verfifies that qttail dumps messages from all ring buffers in the order they
were written, when tailing multiple files.
"""

from __future__ import absolute_import, division, print_function

import atexit
import os
import signal
from QtTailTest import createQtfile, traceLevels, spawn, waitFor, waitForQtfile

pid1 = pid2 = None
def cleanup():
   if pid1:
      os.kill( pid1, signal.SIGTERM )
   if pid2:
      os.kill( pid2, signal.SIGTERM )

atexit.register( cleanup )

# setup
signal.signal( signal.SIGUSR1, lambda *args: None )
msg = "message in (first|second) file"
msg1 = "message in first file"
msg2 = "message in second file"
qtfile1 = "/tmp/qttail_multifile_1.qt"
qtfile2 = "/tmp/qttail_multifile_2.qt"
pid1 = createQtfile( qtfile1, msg1 )
pid2 = createQtfile( qtfile2, msg2 )
waitForQtfile( qtfile1 )
waitForQtfile( qtfile2 )

def verifyMsgs():
   output1 = str( match1.group( 0 ) )
   tsc1 = int( match1.group( 1 ), 16 )
   output2 = str( match2.group( 0 ) )
   tsc2 = int( match2.group( 1 ), 16 )
   haveMsg1 = msg1 in output1 or msg1 in output2
   haveMsg2 = msg2 in output1 or msg2 in output2
   assert haveMsg1
   assert haveMsg2
   assert tsc1 <= tsc2

# tail multi file test
qttail = spawn( "qttail -x --tsc %s %s" % ( qtfile1, qtfile2 ) )
qttail.expect( "--- opened %s" % qtfile1 )
qttail.expect( "--- opened %s" % qtfile2 )
for level, trace in enumerate( traceLevels ):
   qttail.kill( signal.SIGSTOP )
   os.kill( pid2, signal.SIGUSR1 )
   os.kill( pid1, signal.SIGUSR1 )
   qttail.kill( signal.SIGCONT )
   qttail.expect( r'%d (0x\w+) (\w+.qt) \+\d+ "%s"' % ( level, msg ) )
   match1 = qttail.match
   qttail.expect( r'%d (0x\w+) (\w+.qt) \+\d+ "%s"' % ( level, msg ) )
   match2 = qttail.match
   verifyMsgs()
qttail.terminate()
cleanup()
os.wait4( pid1, 0 )
pid1 = None
os.wait4( pid2, 0 )
pid2 = None

# cat multi file test
qttail = spawn( "qttail -c --tsc %s %s" % ( qtfile1, qtfile2 ) )
for level in range( len( traceLevels ) ):
   qttail.expect( r'%d (0x\w+) (\w+.qt) \+\d+ "%s"' % ( level, msg ) )
   match1 = qttail.match
   qttail.expect( r'%d (0x\w+) (\w+.qt) \+\d+ "%s"' % ( level, msg ) )
   match2 = qttail.match
   verifyMsgs()
waitFor( lambda: not qttail.isalive(), "qttail to exit" )
