#!/usr/bin/env python3
# Copyright (c) 2011, Arista Networks, Inc.
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
