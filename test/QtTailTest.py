#!/usr/bin/env python3
# Copyright (c) 2020, Arista Networks, Inc.
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

"""Utilities for qttail tests"""

from __future__ import absolute_import, division, print_function

import atexit
import os
from pexpect import spawn as pexpectSpawn
import signal
import sys
import time
from QuickTrace import ( initialize, trace0, trace1, trace2, trace3, trace4, trace5,
                         trace6, trace7, trace8, trace9 )

defaultTimeout = 600 # seconds == 10 minutes

traceLevels = ( trace0, trace1, trace2, trace3, trace4, trace5, trace6, trace7,
                trace8, trace9 )

def createQtfile( qtfile, msg=None ):
   if os.path.exists( qtfile ):
      os.unlink( qtfile )
   pid = os.fork()
   if pid == 0:
      initialize( qtfile )
      if msg:
         for trace in traceLevels:
            signal.pause()
            trace( msg )
      # Use os._exit so that the child does not run atexit handlers. Otherwise it
      # might kill its sibling, which is can lead to test failures, see BUG587358
      os._exit( 0 ) # pylint: disable=protected-access
   return pid

def spawn( command, **kwargs ):
   if "timeout" not in kwargs:
      kwargs[ "timeout" ] = defaultTimeout
   spwn = pexpectSpawn( command, **kwargs )
   def _killPexpect( spwn ):
      spwn.terminate( force=True )
   atexit.register( _killPexpect, spwn )
   return spwn

def waitFor( condition, description, timeout=defaultTimeout ):
   success = condition()
   tryNo = 0
   while not success:
      if tryNo == 0:
         print( "waiting for", description, end="" )
         print( "...", end="" )
         tryNo = 1
      elif tryNo >= timeout:
         print( "timed out" )
      else:
         if tryNo % 10 == 0:
            print( ".", end="" )
         tryNo += 1
      sys.stdout.flush()
      time.sleep( 1 )
      success = condition()
   if success and tryNo > 0:
      print( "ok" )
   assert success, "timed out waiting for %s" % description

def waitForQtfile( qtfile, timeout=defaultTimeout ):
   waitFor( lambda: os.path.exists( qtfile ), "qt file to exist", timeout )
