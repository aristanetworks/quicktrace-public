// Copyright (c) 2020 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#include <Qtc.h>

// Trace a message once. These expect the default handle to be intialized already.

void
qtcTraceCounter( int level, int counter ) {
   QTRACEF_H( default_hdl, level, "counter: %d", counter );
}

void
qtcTraceString( int level, const char * string ) {
   QTRACEF_H( default_hdl, level, "string: %s", ignoreme, string );
}
