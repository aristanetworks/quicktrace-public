#!/usr/bin/env python3
# Copyright (c) 2022 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

# This test verifies that qttail can load plugins properly.
from __future__ import absolute_import, division, print_function
import subprocess
import os

def check( lines, match ):
   found = False
   for line in lines:
      if match in line:
         found = True
   assert found, ' Expected %s, was not found ' % ( match )

QTFILE = '/tmp/qttail_plugin_test.qt'
tDir = os.getenv( 'target_dir' )
tDir = tDir + "/QtTailPluginTestDir/"
subprocess.check_call( "./QtPluginType", env={ 'QTFILE': QTFILE } )
output = subprocess.check_output( [ "/usr/bin/qttail", '-c', QTFILE ],
      env={ 'QT_FORMATTER_DIR': tDir },
      universal_newlines=True )
# split lines for scanning
outputlines = output.splitlines()  # pylint: disable=E1103
check( outputlines, 'length: 50.0, width: 30.5, depth: 10.5' )
