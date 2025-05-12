#!/usr/bin/env python3
# Copyright (c) 2023 Arista Networks, Inc.
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

import unittest
from os import getpid, remove
import subprocess

import QuickTrace

class QuickTracePythonApiTest( unittest.TestCase ):
   def writeTrace( self, trace ):
      QuickTrace.trace0( QuickTrace.Var( trace ) )

   def testReinitialize( self ):
      """ Test that QuickTrace can be initialized, used, closed, and then
      reinitialized with a new file """

      qtfile1 = '/tmp/qt_python_api_1-%s.qt' % getpid()
      qtfile2 = '/tmp/qt_python_api_2-%s.qt' % getpid()

      QuickTrace.initialize( qtfile1 )
      self.writeTrace( "First file trace" )

      # Include some binary content in the output to ensure we can at least
      # theoretically deal with surrogates in encoded UTF-8 data. This can
      # happen when converting incorrectly formatted C strings into unicode
      # strings, then back again, via the "surrogateescape" error handling.
      # "stringWithSorrogates" below will have "\xbdec" surrogate to encode
      # the invalid \ea byte, for example.
      binary = b"<\xea>"
      stringWithSurrogates = binary.decode( "utf-8", "surrogateescape" )
      stringWithoutSurrogates = "hello world"

      QuickTrace.trace0("test UTF-8 errors:",
            QuickTrace.Var( stringWithSurrogates ),
            QuickTrace.Var( stringWithoutSurrogates ) )

      QuickTrace.finalize()
      QuickTrace.close()

      QuickTrace.initialize( qtfile2 )
      self.writeTrace( "Second file trace" )
      QuickTrace.finalize()
      QuickTrace.close()

      # Check the contents of the first file
      output = subprocess.check_output( [ "/usr/bin/qttail", "-c", qtfile1 ],
                                        universal_newlines=True,
                                        errors="surrogateescape" )
      self.assertIn( "First file trace", output )
      self.assertIn( f"test UTF-8 errors: {stringWithSurrogates} "
                     f"{stringWithoutSurrogates}", output )
      self.assertNotIn( "Second file trace", output )

      # Check the contents of the second file
      output = subprocess.check_output( [ "/usr/bin/qttail", "-c", qtfile2 ],
                                        universal_newlines=True,
                                        errors="surrogateescape" )
      self.assertIn( "Second file trace", output )
      self.assertNotIn( "First file trace", output )

      remove( qtfile1 )
      remove( qtfile2 )

if __name__ == '__main__':
   unittest.main()
