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

"""Verifies that qttail does not miss any message when a ring buffer rolls over,
in conjunction with forever log.
This test creates 100 messages of size 17 in ring buffer 9 (which is 1k of size)
for a total of 1,700 bytes. 1k gives a usable size of 768 bytes. This means the
buffer rolls over twice.
"""

from __future__ import absolute_import, division, print_function

from QuickTrace import initialize, trace9, Var as qv
from QtTailTest import spawn

# setup
qtfile = "/tmp/qttail_forever.qt"
initialize( qtfile, "1,1,1,1,1,1,1,1,1,1", foreverLogPath=qtfile )

# 10 billion to enforce use of 64 bit
valueBase = 10000000000

# rollover test with forever log
qttail = spawn( "qttail -x %s" % qtfile )
qttail.expect( "--- opened %s" % qtfile )
value = valueBase
for i in range( 10 ):
   for j in range( 10 ):
      trace9( "test forever rollover", qv( value + j ) )
   for j in range( 10 ):
      qttail.expect( r'9 \+\d+ "test forever rollover %d"' % ( value + j ) )
   value += 10
assert qttail.terminate()

# numMsgs holds the number of messages per file. each message logs a 64 bit integer
# message size = 8 + 13 = 21
# buffer size = 1024
# start of trailer = 1024 - 256 = 768
# 37 messages * 21 bytes = 777
numMsgs = 37

# verify that all the files (two rolled over files plus the current one) 
# can be decoded properly, and contain the expected messages
for fileName, firstId, lastId in ( ( qtfile + ".0", 0, numMsgs ),
                                   ( qtfile + ".1", numMsgs, numMsgs * 2 ),
                                   ( qtfile, numMsgs * 2, 100 ) ):
   qttail = spawn( "qttail -cx %s" % fileName )
   qttail.expect( "--- opened %s" % fileName )
   for i in range( firstId, lastId ):
      qttail.expect( r'9 \+\d+ "test forever rollover %d"' % ( valueBase + i ) )
   assert qttail.terminate()
