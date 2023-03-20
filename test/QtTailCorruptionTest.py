#!/usr/bin/env python3
# Copyright (c) 2021 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

from __future__ import absolute_import, division, print_function

import os
import sys
import subprocess

# FIXME BUG611690 - wrapper needed to support multiple Python versions
if __name__ == '__main__':
   os.environ[ 'PYTHON' ] = sys.executable
   subprocess.check_call( [ "./QtTailCorruptionTest" ] )
