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
import subprocess
import unittest

# Validate that the messages produce by QTFMT and QTFMT_PROF are correctly
# decoded by qttail.
class TestQTFMTTest( unittest.TestCase ):
   execFile = './QtFmtTest'
   tracePrefix = 'QtFmtTest'

   # For qttail, verify the trace level and the trace message
   qttailExpected = []

   # For qtproc verify that the timing information is present, as well as the
   # unformatted trace message

   for prefix in [ '', 'testQtFmtFunc() ', 'ClassName::testQtFmtClass() ' ]:
      qttailExpected.append( ( 0, prefix + "QtFmtTest noparams extra" ) )

      for lvl in range( 0, 10 ):
         msg = prefix + "QtFmtTest lvl" + str( lvl ) + " %s %x"
         qttailExpected.append( ( lvl, msg % ( 42, 0xcafe ) ) )

      qttailExpected.append( ( 0, prefix + "QtFmtTest noparamsprof extra" ) )
      for lvl in range( 0, 10 ):
         msg = prefix + "QtFmtTest prof" + str( lvl ) + " %s %x"
         qttailExpected.append( ( lvl, msg % ( 42, 0xcafe ) ) )

   def runTestHelper( self ):
      qtFile = subprocess.check_output( self.execFile )
      self.assertTrue( os.access( qtFile, os.R_OK ) )
      return qtFile

   def testQttail( self ):
      self.maxDiff = None

      qtFile = self.runTestHelper()
      trace = subprocess.check_output( [ '/usr/bin/qttail', '-c', qtFile ],
                                       universal_newlines=True )

      obtained = []
      for l in trace.splitlines():
         # Skip data lines that don't have our test prefix
         if self.tracePrefix not in l:
            continue
         # Then parse the output of qttail, we are mainly interested in
         # validating the trace level and the trace message
         ( _, _, level, _, ccMessage ) = l.split( ' ', 4 )
         message = ccMessage.strip( '"' )
         obtained.append( ( int( level ), message ) )
      self.assertEqual( self.qttailExpected, obtained )

if __name__ == '__main__':
   unittest.main()
