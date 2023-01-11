#!/usr/bin/env python3
# Copyright (c) 2017, Arista Networks, Inc.
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
