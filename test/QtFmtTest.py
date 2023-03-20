#!/usr/bin/env python3
# Copyright (c) 2021 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

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
