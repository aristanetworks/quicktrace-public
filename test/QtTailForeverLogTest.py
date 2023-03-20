#!/usr/bin/env python3
# Copyright (c) 2020 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

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
