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

"""Verifies that qttail does not miss any message when a ring buffer rolls over.
This test creates 100 messages of size 17 in ring buffer 9 (which is 1k of size)
for a total of 1,700 bytes. This means the buffer must roll over once.
"""

from __future__ import absolute_import, division, print_function

from QuickTrace import initialize, trace9, Var as qv
from QtTailTest import spawn

# setup
qtfile = "/tmp/qttail_rollover.qt"
initialize( qtfile, "1,1,1,1,1,1,1,1,1,1" )

# rollover test
qttail = spawn( "qttail -x %s" % qtfile )
qttail.expect( "--- opened %s" % qtfile )
for i in range( 10 ):
   for j in range( 10 ):
      trace9( "test rollover", qv( i * 10 + j ) )
   for j in range( 10 ):
      qttail.expect( r'9 \+\d+ "test rollover %d"' % ( i * 10 + j ) )
