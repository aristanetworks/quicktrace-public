#!/usr/bin/env python3
# Copyright (c) 2022, Arista Networks, Inc.
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

# This test runs the QtOffBitTest executable, pretty-prints via qttail and
# validates its output.

from __future__ import absolute_import, division, print_function
import subprocess

def check( line, match ):
   assert match in line, 'Line %s, does not contain %s ' % (
         line, match )

# Invoke QtOffBitTest to generate a qtrace file
QTFILE = '/tmp/QtOffBitTest.out'
subprocess.check_call( "./QtOffBitTest", env={ 'QTFILE': QTFILE } )

# invoke qttail to pretty-print qtrace file and capture stdout output
output = subprocess.check_output( [ "/usr/bin/qttail", "-c", QTFILE ],
                                  universal_newlines=True )
# split lines for scanning
outputlines = output.splitlines()  # pylint: disable=E1103

check( outputlines[ 0 ], 'float ft1 1.0' )
check( outputlines[ 1 ], 'float ft2 3.14' )
check( outputlines[ 2 ], 'float ft3 4.14' )
check( outputlines[ 3 ], 'float ft4 5.14' )
