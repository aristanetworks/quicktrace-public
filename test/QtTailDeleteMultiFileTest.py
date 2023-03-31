#!/usr/bin/env python3
# Copyright (c) 2019, Arista Networks, Inc.
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

"""Verifies that qttail exits when all of the log files that it is tailing are
   deleted.
"""

from __future__ import absolute_import, division, print_function

import os
from QtTailTest import createQtfile, spawn, waitFor, waitForQtfile

# prepare
qtfile1 = "/tmp/qttail_delete_multifile_1.qt"
qtfile2 = "/tmp/qttail_delete_multifile_2.qt"
createQtfile( qtfile1 )
createQtfile( qtfile2 )
waitForQtfile( qtfile1 )
waitForQtfile( qtfile2 )

# delete multi file test
qttail = spawn( "qttail -x %s %s" % ( qtfile1, qtfile2 ) )
qttail.expect( "--- opened %s" % qtfile1 )
qttail.expect( "--- opened %s" % qtfile2 )
os.unlink( qtfile1 )
os.unlink( qtfile2 )
qttail.expect( "--- deleted %s" % qtfile1 )
qttail.expect( "--- deleted %s" % qtfile2 )
waitFor( lambda: not qttail.isalive(), "qttail to exit" )
