// Copyright (c) 2021 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#ifndef QUICKTRACE_REGISTRATION_H
#define QUICKTRACE_REGISTRATION_H

#include <functional>

namespace QuickTrace {

class TraceHandle;

// The registration callback is a hook for third party libraries to register a
// callback that is invoked by QuickTrace::initialize when a handler is created
using RegistrationCallback = std::function< void( TraceHandle &, bool ) >;
extern RegistrationCallback registerHandle;

}

#endif // QUICKTRACE_REGISTRATION_H
