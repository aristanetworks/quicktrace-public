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

from __future__ import absolute_import, division, print_function
import sys
import sysconfig
import libQuickTracePy as _QuickTrace

def initialize( filename="qt-%d.out", sizes="", foreverLogPath="", \
                foreverLogIndex=0, maxStringLen=24, numMsgCounters=512,
                multiThreadingEnabled=False ):
   """Initialize QuickTrace.  Output will go to 'filename'.  Sizes is
   a space and/or comma-separated array of ten integers, representing
   the sizes of the individual trace buffers for levels 0-9.  Sizes
   are in units of kilobytes.  Here's a sample usage:

   QuickTrace.initialize( "/var/log/agents/qt-%d.out", "16,16,16,1,1,1,1,1,16,16" )

   that might be appropriate for a program that does a lot of tracing
   at levels 0,1,2 and 8 and 9, but not much other tracing.  Returns
   True if QuickTrace was successfully initialized (or if it was
   already initialized to the same file).
   """
   import ctypes
   qt = ctypes.cdll.LoadLibrary("libQuickTrace.so")

   if foreverLogPath:
      foreverLogPath = foreverLogPath.encode()
   qt.QuickTrace_initialize_with_logging.restype = ctypes.c_void_p
   return bool( qt.QuickTrace_initialize_with_logging( filename.encode(),
                                     sizes.encode(), foreverLogPath,
                                     foreverLogIndex, maxStringLen,
                                     multiThreadingEnabled, numMsgCounters ) )

def finalize():
   _QuickTrace.close()

# Var is a little helper class that is used by tracef to wrap anything that you
# want to be *dynamically* traced (that is, have its value computed
# every time the trace statement is hit as opposed to just the first
# time.
class Var(object):
   __slots__ = ('me')
   def __init__( self, me ):
      self.me = me
   def __str__( self ):
      return str( self.me )

# The map from python type to format string tells qtcat how to decode
# our dynamic data.  It is a comma-separated list of 'format strings'.
# The list of format strings must be consistent with the set of types
# that are processed by _QuickTrace.trace, and also with the
# formatString declarations in QuickTrace.h
_typeToFmt = { str:'p',
               bool:'b',
               float:'d'
            }
if sys.version_info[ 0 ] == 2: # Python 2
   # Python 2 'long' type literal - using the 'long' keyword would cause a
   # syntax error in Python 3.
   _longType = type( sys.maxsize + 1 )
   _typeToFmt[ _longType ] = 'q'
   _typeToFmt[ int ] = 'i' if sysconfig.get_config_var( 'SIZEOF_LONG' ) == 4 else 'q'
else: # Python 3
   _typeToFmt[ int ] = 'q'

def _fmtString( a ):
   return _typeToFmt.get( type(a), 'p' )

# Helper function to create a new message.  This is called the first
# time a message is visited.
def _createMsg( filename, lineno, args ):
   msgStr = " ".join(["%s" if type(i) is Var else str(i) for i in args])
   formatStr = ",".join([_fmtString(a.me) for a in args if type(a) is Var])
   msgId = _QuickTrace.messageIs( filename, lineno, msgStr, formatStr )
   return msgId

# Cache this for a teensy boost of speed
gf = sys._getframe

# This is a map from a tuple (code,line) where code is the id of the
# calling code object, and line is the line within that caller.  This
# is used to look up the unique messageId for the calling trace
# statement
_msgId = {}

# The main trace function
def tracef( level, frameOffset, args ):
   # need to go up at least two stack frames to skip wrapper
   fb = gf( 2 + frameOffset )
   key = (id(fb.f_code), fb.f_lineno)
   # hash the enclosing code and lineno to find the msgId
   msgId = _msgId.get(key)
   if not msgId:
      msgId = _createMsg( fb.f_code.co_filename, fb.f_lineno, args )
      if not msgId: # looks like QuickTrace was not initialized
         return
      _msgId[key] = msgId
   vargs = [i.me for i in args if type(i) is Var]
   _QuickTrace.trace(msgId, level, vargs)

def traceWithFrameOffset( level, frameOffset, *args ):
   tracef( level, frameOffset, args )

def trace( *args ): tracef( 0, 0, args )

def trace0( *args ): tracef( 0, 0, args )
def trace1( *args ): tracef( 1, 0, args )
def trace2( *args ): tracef( 2, 0, args )
def trace3( *args ): tracef( 3, 0, args )
def trace4( *args ): tracef( 4, 0, args )
def trace5( *args ): tracef( 5, 0, args )
def trace6( *args ): tracef( 6, 0, args )
def trace7( *args ): tracef( 7, 0, args )
def trace8( *args ): tracef( 8, 0, args )
def trace9( *args ): tracef( 9, 0, args )

def close():
   global _msgId
   _msgId = {}

qtrace_string_size = _QuickTrace.getMaxStringTraceLen()

# A function for tracing long strings by splitting it across
# multiple messages. Do not use when performance is crucial
def traceLongString( trace, *arg ):
   strArg = str( arg )
   length = qtrace_string_size
   chunkList = ( strArg[ 0 + i : length + i ]
                 for i in range( 0, len( strArg ), length ) )
   trace ( Var( next( chunkList ) ) )
   for chunk in chunkList:
      trace( '...', Var( chunk ) )

