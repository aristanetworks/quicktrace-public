#!/usr/bin/env python3
# Copyright (c) 2016 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

from __future__ import absolute_import, division, print_function
import subprocess

def check( isPresent, lines, match, addr ):
   for line in lines:
      if match in line:
         assert ((addr in line) == isPresent), 'For %s expected %s but found: %s' % (
            match, addr, line )
            
QTFILE = '/tmp/QtMaxStrLen.out'
subprocess.check_call( "./QtMaxStrLen", env={ 'QTFILE': QTFILE } )

# invoke qttail to pretty-print qtrace file and capture stdout output
output = subprocess.check_output( [ "/usr/bin/qttail", '-c', QTFILE ],
                                  universal_newlines=True )

# split lines for scanning
outputlines = output.splitlines() # pylint: disable=E1103

check( True, outputlines, "long string", "This string is longer than 24 " \
       "characters and less than 80." )
check( True, outputlines, "Now see, even though this line is far longer " \
       "than 80 characters, it is part of the static portion. Thus, we " \
       "can expect it to not be truncated.", "Good" )

# The check below fails because we check far more than 80 chars
check( False, outputlines, "too long", "Clocking in at over 80 characters, " \
       "this line is way way too long, so even with our new quick trace, " \
       "this should fail" )
# The check below succeeds because we only check the first 80 chars
check( True, outputlines, "too long", "Clocking in at over 80 characters, " \
       "this line is way way too long, so even with o" )
# The check below fails because we check the first 81 chars
check( False, outputlines, "too long", "Clocking in at over 80 characters, " \
       "this line is way way too long, so even with ou" )

# Lastly, we check to make sure that trying to set maxStringLen above 80 
# results in it being set to 80 instead.

# The check below succeeds because we only check the first 80 chars
check( True, outputlines, "500 should be 80 test", "Clocking in at over 80 " \
       "characters, this line is way way too long, so even with o" )
# The check below fails because we check the first 81 chars
check( False, outputlines, "500 should be 80 test", "Clocking in at over 80 " \
       "characters, this line is way way too long, so even with ou" )

check( True, outputlines, "wrap-around test", "Did we have any trouble with " \
       "wrap-around?" )
