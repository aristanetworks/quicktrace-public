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

from __future__ import absolute_import, division, print_function

import ctypes
import os
import subprocess
import unittest
import QuickTrace

qtfile = os.environ.get( 'QTFILE', 'qtctlTest.qt' )
qtdir = os.environ.get( 'QUICKTRACEDIR', '.qt' )
qtpath = os.path.join( qtdir, qtfile )

qtTestLib = ctypes.CDLL( 'libQuickTraceTestMessages.so' )
qtcTestLib = ctypes.CDLL( 'libQtcTestMessages.so' )

class TestQtctl( unittest.TestCase ):
   '''Test that disabling messags with qtctl doesn't corrupt the trace file.
   Do this by tracing some messages, disabling some messages, tracing the
   disabled messages, enabling the messages, and tracing some more. The
   trace file should not be corrupted, nor contain the traces that were
   executed at the time they were disabled.'''

   def executeTest( self, *expectedTraces ):
      # trace some stuff to get the messages set up in the file
      self.initialTraces()
      # disable some of the traces with qtctl
      self.disableSomeMessages()
      # trace messages, including disabled ones
      self.additionalTraces()
      # make sure qttail finds no problems
      subprocess.check_output( [ "/usr/bin/qttail", '-c', qtpath ] )
      # reenable the disabled traces
      self.reenableMessages()
      # trace more messages to make sure enablement works
      self.finalTraces()

      # check that nothing was traced that shouldn't have been
      output = subprocess.check_output( [ "/usr/bin/qttail", '-c', qtpath ],
                                        universal_newlines=True )

      i = 0
      for line in output.splitlines():
         if '---' in line:
            continue
         assert line.endswith( expectedTraces[ i ] + '"' )
         i += 1

   def initialTraces( self ):
      pass

   def disableSomeMessages( self ):
      pass

   def additionalTraces( self ):
      pass

   def reenableMessages( self ):
      pass

   def finalTraces( self ):
      pass

class TestCppAndPythonAndGoSources( TestQtctl ):
   '''This class tests Go, Python and C++ tracing together. This is done
   because we can, and it also allows us to get a bit of testing of
   qtctl's regex disable/enable of traces without creating a bunch of
   duplicate functions just to ensure they have unique msgIds.
   '''

   def __init__( self, *args ):
      self.goStringMsgId = None
      self.goCounterMsgId = None
      TestQtctl.__init__( self, *args )

   def setUp( self ):
      QuickTrace.initialize( qtfile )

   # to get the same trace message each time we trace this (as opposed to one
   # for each source line), use functions to do the tracing
   def traceString( self, s ):
      QuickTrace.trace2( "string:", QuickTrace.Var( s ) )

   def traceCounter( self, c ):
      QuickTrace.trace2( "counter:", QuickTrace.Var( c ) )

   def initialTraces( self ):
      qtTestLib.qtTraceString( 1, b"c++" )
      qtTestLib.qtTraceCounter( 1, 1 )
      self.traceString( "python" )
      self.traceCounter( 2 )

      self.goStringMsgId = qtTestLib.qtCreateStringMsgId( b"string: %s" )
      assert self.goStringMsgId != 0
      self.goCounterMsgId = qtTestLib.qtCreateCounterMsgId( b"counter: %s" )
      assert self.goCounterMsgId != 0
      qtTestLib.qtGoTraceString( 3, self.goStringMsgId, b"Go" )
      qtTestLib.qtGoTraceCounter( 3, self.goCounterMsgId, 3 )

   def disableSomeMessages( self ):
      subprocess.check_output( [ "qtctl", "off", "-r", "counter", qtpath ] )

   def additionalTraces( self ):
      qtTestLib.qtTraceCounter( 1, 11 )
      qtTestLib.qtTraceString( 1, b"c++ again" )
      qtTestLib.qtTraceCounter( 1, 12 )
      self.traceCounter( 21 )
      self.traceString( "python again" )
      self.traceCounter( 22 )
      qtTestLib.qtGoTraceCounter( 3, self.goCounterMsgId, 31 )
      qtTestLib.qtGoTraceString( 3, self.goStringMsgId, b"Go again" )
      qtTestLib.qtGoTraceCounter( 3, self.goCounterMsgId, 32 )

   def reenableMessages( self ):
      subprocess.check_output( [ "qtctl", "on", "-r", "counter", qtpath ] )

   def finalTraces( self ):
      qtTestLib.qtTraceCounter( 1, 13 )
      self.traceCounter( 23 )
      qtTestLib.qtGoTraceCounter( 3, self.goCounterMsgId, 33 )

   def testQtctl( self ):
      self.executeTest( 'string: c++', 'counter: 1',
                        'string: python', 'counter: 2',
                        'string: Go', 'counter: 3',
                        'string: c++ again', 'string: python again',
                        'string: Go again', 'counter: 13',
                        'counter: 23', 'counter: 33' )


class TestCSource( TestQtctl ):
   '''C uses a different default trace handle than C++, and Python can only use
   C++'s default trace handle. So C traces need to go to a different file than
   C++ and Python. It's also a lot easier to test qtctl's message ID
   disable/enable of traces when there is only one to look for.'''

   def setUp( self ):
      qtLib = ctypes.CDLL( 'libQuickTrace.so' )
      assert qtLib.qt_initialize_default_handle( qtfile.encode(),
                                                 b"1,1,0,0,0,0,0,0,0,0" )

   def initialTraces( self ):
      qtcTestLib.qtcTraceString( 3, b"c" )
      qtcTestLib.qtcTraceCounter( 3, 3 )

   def disableSomeMessages( self ):
      output = subprocess.check_output( [ "qtctl", "show", qtpath ],
                                        universal_newlines=True )
      for line in output.splitlines():
         if 'counter' in line:
            msgId = line.split()[ 0 ]
            break
      else:
         assert False, 'expected to find a message with "counter" in it'
      subprocess.check_output( [ "qtctl", "off", "-m", msgId, qtpath ] )

   def additionalTraces( self ):
      qtcTestLib.qtcTraceCounter( 3, 31 )
      qtcTestLib.qtcTraceString( 3, b"c again" )
      qtcTestLib.qtcTraceCounter( 3, 32 )

   def reenableMessages( self ):
      output = subprocess.check_output( [ "qtctl", "show", qtpath ],
                                        universal_newlines=True )
      for line in output.splitlines():
         if 'counter' in line:
            msgId = line.split()[ 0 ]
            subprocess.check_output( [ "qtctl", "off", "-m", msgId, qtpath ] )
            break

   def finalTraces( self ):
      qtcTestLib.qtcTraceCounter( 3, 33 )

   def testQtctl( self ):
      self.executeTest( 'string: c', 'counter: 3',
                        'string: c again', 'counter: 33' )

if __name__ == '__main__':
   unittest.main()
