#!/usr/bin/env python3
# Copyright (c) 2017 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

from __future__ import absolute_import, division, print_function
import subprocess, unittest, os

class QuickTraceMultipleThreadsTest( unittest.TestCase ):
   def setUp( self ):
      # pylint: disable=W0201
      self.existingFiles = os.environ.get( "QTMT_EXISTING_FILES" )
      if not self.existingFiles:
         # Invoke QuickTraceMultipleThreadsTest to generate qtrace files
         subprocess.check_call( [ '/usr/bin/timeout', '300s',
                                  './QuickTraceMultipleThreadsTest' ],
                                env={ 'QUICKTRACEDIR': '/tmp' } )

   def getFilename( self, threadIndex, traceName ):
      return '/tmp/QtMtTest-%d-%s' % ( threadIndex, traceName )

   def qttail( self, threadIndex, traceName ):
      filename = self.getFilename( threadIndex, traceName )
      output = subprocess.check_output( [ '/usr/bin/qttail', '-c', filename ],
                                          universal_newlines=True )
      return output.splitlines()  # pylint: disable=E1103

   def checkFile( self, threadIndex, traceName ):
      '''Verify that the QuickTrace file produced by a specific thread is
         parsable by qttail and contains the expected entries.'''
      outputLines = self.qttail( threadIndex, traceName )
      lineIter = iter( outputLines )
      for msgId in range( 500 ):
         for level in range( 10 ):
            for i in range( 4 ):
               line = next( lineIter ).split()
               self.assertEqual( line[ 2 ], str( level ) )
               expectedMsg = '%s-%d-%d-%d' % ( traceName.capitalize()[ 0 ],
                                               threadIndex, msgId, i )
               self.assertEqual( line[ -1 ].strip( '"' ), expectedMsg )

   def test( self ):
      for threadIndex_ in range( 10 ):
         for traceName_ in [ 'default', '1', '2' ]:
            filename_ = self.getFilename( threadIndex_, traceName_ )
            self.checkFile( threadIndex_, traceName_ )
            if not self.existingFiles:
               os.remove( filename_ )

if __name__ == '__main__':
   unittest.main()
