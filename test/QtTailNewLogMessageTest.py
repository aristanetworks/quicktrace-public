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

"""Verfifies that qttail can handle a new message id that appears later than
previously logged messages.
"""

from __future__ import absolute_import, division, print_function

from QuickTrace import initialize, trace0
from QtTailTest import spawn, waitFor

# setup
qtfile = "/tmp/qttail_newlogmessage.qt"
initialize( qtfile )

# start qttail and log the first message
qttail = spawn( "qttail -x %s" % qtfile )
qttail.expect( "--- opened %s" % qtfile )
msg1 = "first message"
trace0( msg1 )
qttail.expect( r'0 \+\d+ "%s"' % msg1 )

# log the second message (which will have a new message id)
msg2 = "second message"
trace0( msg2 )
qttail.expect( r'0 \+\d+ "%s"' % msg2 )
qttail.terminate( force=True )

# Sometimes signals sent through terminate() to exit qttail may get processed later.
# In that case terminate() will exit without verifying that qttail has succesfully
# exited. So, we add a waitFor to verify that qttail has exited.
waitFor( lambda: not qttail.isalive(), "qttail to exit" )
