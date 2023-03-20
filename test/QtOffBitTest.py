#!/usr/bin/env python3
# Copyright (c) 2022 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

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
