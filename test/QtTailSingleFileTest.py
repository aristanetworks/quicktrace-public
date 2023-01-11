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

"""Verfifies that qttail dumps messages from all ring buffers in the order they
were written, when tailing a single file.
"""

from __future__ import absolute_import, division, print_function

from QuickTrace import initialize, trace0, trace1, trace2, trace3, trace4, trace5, \
                       trace6, trace7, trace8, trace9
from QtTailTest import spawn, waitFor

# setup
qtfile = "/tmp/qttail_singlefile.qt"
initialize( qtfile )

# tail single file test
qttail = spawn( "qttail -x %s" % qtfile )
qttail.expect( "--- opened %s" % qtfile )
msg = "basic test message"
traceLevels = ( trace0, trace1, trace2, trace3, trace4, trace5, trace6, trace7,
                trace8, trace9 )
for trace in traceLevels:
   trace( msg )
for level in range( len( traceLevels ) ):
   qttail.expect( r'%d \+\d+ "%s"' % ( level, msg ) )
assert qttail.terminate()

# cat single file test
qttail = spawn( "qttail -c %s" % qtfile )
for level in range( len( traceLevels ) ):
   qttail.expect( r'%d \+\d+ "%s"' % ( level, msg ) )
waitFor( lambda: not qttail.isalive(), "qttail to exit" )
rc = qttail.wait()
assert rc == 0
