#!/usr/bin/env python3
# Copyright (c) 2011 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

# This test mounts a tmpfs that is too small to hold our default
# quicktrace file.  It then tries to initialize quicktrace on the full
# filesystem and makes sure that the process does not exit, that the
# quicktrace calls are silently ignored, and that no .qt file gets
# created.

# Run the current process as root, using sudo.
from __future__ import absolute_import, division, print_function
import os
import subprocess

if os.getuid():
   import sys
   os.execv( '/usr/bin/sudo', ['sudo'] + sys.argv )

# Now create a mount namespace, and mount a tmpfs on /tmp, so that we
# don't leave any files or mounts behind after the test finishes
import netns
netns.unshare( netns.CLONE_FS  | netns.CLONE_NEWNS )
subprocess.check_output(['sudo', '-E', 'mount','-t','tmpfs','none','/tmp',
                         '-osize=10000'])
os.chdir( '/tmp' )

# --------------------------------
# Here is the meat of the test.  Initialize quicktrace, which should
# fail, then run a bunch of quicktraces, which should do nothing
# --------------------------------
import QuickTrace
mntdir = 'smalldisk'
os.mkdir( mntdir )
subprocess.check_output(['sudo', '-E', 'mount','-t','tmpfs','none',mntdir,
                         '-osize=1000'])
os.chdir( mntdir )
qtfile = os.environ.get( 'QTFILE', 'simple.qt' )
QuickTrace.initialize( qtfile )
assert not os.path.exists( qtfile )

qt0 = QuickTrace.trace0
qt1 = QuickTrace.trace1
qt2 = QuickTrace.trace2
qt3 = QuickTrace.trace3
qt4 = QuickTrace.trace4
qt5 = QuickTrace.trace5
qt6 = QuickTrace.trace6
qt7 = QuickTrace.trace7
qt8 = QuickTrace.trace8
qt9 = QuickTrace.trace9
for qt in (qt0,qt1,qt2,qt3,qt4,qt5,qt6,qt7,qt8,qt9):
   for i in range(100):
      qt2( 'hi', 'there' )

assert not os.path.exists( qtfile )
