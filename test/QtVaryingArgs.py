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

from os import getpid, remove
import subprocess

import QuickTrace

qtfile = '/tmp/varying-args-%s.qt' % getpid()

def traceUnwisely( args ):
   # pylint: disable=map-builtin-not-iterating
   QuickTrace.trace2( *map( QuickTrace.Var, args ) )

try:
   QuickTrace.initialize( qtfile )
   traceUnwisely( [ 'Hello', 'World' ] )
   QuickTrace.trace2( "This one is", QuickTrace.Var( "OK" ) )
   traceUnwisely( [ 'Hi', 'There', 'How', 'Do', 'You', 'Do?' ] )
   traceUnwisely( [ 'Three', 'Fencepost', 'Bloop' ] )
   traceUnwisely( [ 'One Fencepost Sloop' ] )
   QuickTrace.trace2( "This one is", QuickTrace.Var( "also OK" ) )
   traceUnwisely( [ 'Goodbye', 'World' ] )
finally:
   # invoke qttail to pretty-print qtrace file and capture stdout output
   output = subprocess.check_output( [ "/usr/bin/qttail", '-c', qtfile ],
                                     universal_newlines=True )

   # Output that we expect to see:
   # Since the first call to traceUnwisely() used 2 args, extra args are
   # ignored and shorter args are padded with None:
   expected = ( "Hello World",
                "This one is OK",
                "Hi There",
                "Three Fencepost",
                "One Fencepost Sloop None",
                "This one is also OK",
                "Goodbye World" )
   for s in expected:
      assert '"' + s + '"' in output, s + " not in:\n" + output

   remove( qtfile )

