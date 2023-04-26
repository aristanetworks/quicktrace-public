The QuickTrace library is a C++ library that records user-inserted trace
messages into a circular ring-buffer.
It writes to a memory-mapped file and stores data compactly in binary format,
so it is extremely efficient when tracing is enabled. The trace file persists
if the program crashes and is visible in real-time from outside a live process.

# Table of Contents
* [Prerequisites](#Prerequisites "Goto Prerequisites")
* [Installation](#Installation "Goto Installation")
* [Example Usage](#example-usage)
* [The API](#the-api)
* [Tools for managing QuickTrace files](#tools-for-managing-quicktrace-files)

## Prerequisites
To successfully build and use QuickTrace, ensure that the following programs are installed:
- CMake >= 3.20.2
- Python >= 3.9
- C++ >= 17
- git

## Installation
Clone the QuickTrace repository into your local working directory
```
git clone https://github.com/aristanetworks/quicktrace.git
```
To begin building and installing QuickTrace, CMake needs to be initialized to generate the appropriate configuration files. There are three configuration profiles: default, test, and release. The next few sections outline how to build QuickTrace using the available profiles.
**Note** Run the `cmake` commands inside the `quicktrace` directory that was cloned.

### CMake Default Configuration
To build QuickTrace as is, with debugging symbols and no optimizations enabled, run the following commands:
```
cmake --preset default
cmake --build --preset default
```

### CMake Test Configuration
To build QuickTrace and run its tests, use the `test` configuration. This is the same as the previous `default` configuration except with additional tests being generated.
```
cmake --preset test
cmake --build --preset test
```
To run the testing suite, run the following command:
```
ctest --preset test
```

### CMake Release Configuration
To build QuickTrace with optimizations enabled and no debugging symbols, run the following commands:
```
cmake --preset release
cmake --build --preset release
```

## Example usage

Quicktrace provides several trace macros in order to format trace messages. The
modern version of those trace macros are the QTFMT macros, which we’ll use in
that tutorial. We can write a simple program which produces a quicktrace
message then exits:
```c++
// Make the QTFMT macros available
#include <QuickTrace/QtFmtGeneric.h>

int main() {
   // Initialize QuickTrace and specify the file in which the traces will go
   QuickTrace::initialize( "myQuicktraceFile.qt" );

   // A simple qtrace message
   QTFMT0_RAW("Hello world: {}", 42 );

   return 0;
}
```
We can then compile that small example as follows:

```
> g++ test.cpp -lQuickTrace
> ./a.out
```

Upon running the program, you will see that `.qt/myQuicktraceFile.qt` has been
created. We can then run qttail to decode our trace file as follows:
```
>qttail -c .qt/myQuicktraceFile.qt
2023-03-22 11:31:16.574509 0 +147577317756540364 "Hello world: 42"
```

## The API
QuickTrace provides the following APIs:
- QTRACE0…QTRACE9 for tracing events
- QPROF0 … QPROF9 for tracing events and also profiling the basic block
- QPROF for profiling without a trace in the log
- QPROF_S for recording self-profiling data
- QTFMT0…QTFMT9 and QTFMT_PROF0…QTFMT_PROF9 for tracing and profiling using an fmt-style syntax
- QuickTrace::initialize to enable tracing from C++
- QuickTrace.initialize to enable tracing from Python
These are described in more detail below.

### The C++ API
#### QTFMT0 (and QTFMT1 up to 9)

The QTFMT macros use an std::format inspired syntax to specify the trace message. These macros are defined in "QuickTrace/QtFmtGeneric.h."
As small example of their use would be as follows:

```c++
#include <QuickTrace/QtFmtGeneric.h>
// ....
QTFMT0_RAW( "Programming {} at offset 0x{:x}", value, offset );
```

The "{}" in the trace message is a format specifier that will be used when the
message is pretty printed and will format the parameters in the right location.
The result you should get is as follows:

```
Programming route 1.2.3.4 at offset 0x99.
```


By default, QTFMT macros try to include the class name and function name:
- QTFMT0: includes the class name and the function name.
- QTFMT0_FUNC: includes only the function name
- QTFMT0_RAW: doesn’t include anything

That is you can also do:
```c++
QTFMT0( "Programming {} at offset 0x{:x}", value, offset );
```

And obtain:
```
MyClass::programRoute() Programming route 1.2.3.4 at offset 0x99.
```

While the syntax tries to follow an std::format style, this is a more limited subset:
only the following format specifiers are currently supported:
- `{}` to indicate that the default formatter for the type should be used
- `{:x}` to indicate that a hex formatter should be used. This is limited to integral types.
- To escape '{' and '}' in the format message, you can write '{{' and '}}'

Only the above forms to specify parameters are supported. For instance, it's
not possible to specify the argument number in the format specifier, so writing
'{0}' or '{1}' will not work as expected.

To pick up the classname, QuickTrace macros refer to a `qtraceClassName` and expects
the user to define it to a meaningful expression. For instance, one could do the
following define:
```C++
#define qtraceClassName currentClassName()
```
And then ensure that every class that invokes QTFMT0 also define a
`currentClassName()` public method.

#### QTRACE0( fixed, dynamic )
*(and QTRACE1 … QTRACE9.)*
This is the original QuickTrace API which predates the QTFMT API. The macros are
defined in QuickTrace/QuickTrace.h, alongside the other macros that are presented in
that document. The 'fixed' and 'dynamic' arguments are sequences of quicktraceable
expressions, separated by the '<<' operator. So it's used like this:
```c++
#include <QuickTrace/Quicktrace.h>

QTRACE0( "Entering doProgramRoute " << cellId(), ipAddr << petraName() );
```
The strings 'Entering doProgramRoute 3' is stored in the fixed portion of the log, one time when the trace is first executed. 'ipAddr' and 'offset' are computed every time the trace is hit and are stored in the circular-buffer portion of the log. The above trace QTRACE statement generates a trace message like this:
```
"Entering doProgramRoute 3 % (1.2.3.4, Petra1)"
```
If you want the dynamic arguments to interleave with the static ones, you can use the special QVAR macro kind of like a "%s" in a printf format string to indicate where to put the dynamic arguments. Here's an example:
```c++
QTRACE0( "Programming route " << QVAR << " at offset " << QVAR << ".", ipAddr << offset );
```
When this trace message is printed by qttail, the trace string will look something like this:
```
Programming route 1.2.3.4 at offset 99.
```

QVAR is currently identical to just sticking "%s" into the fixed string. If you goof up and put the wrong number of QVARs into your string, nothing terribly bad happens, but qttail will simply append the dynamic arguments to the fixed trace string, as shown in the first example
In addition to inserting a record into the circular buffer, each time a trace message is hit, a per-message counter is incremented and a last-hit time is updated in the trace file. These counts are not overwritten or reset when the log wraps, but they can be cleared with the 'qtclear' tool.
QNULL is available to use if there is no dynamic data for the QTRACE statement, like this:
QTRACE0( "I was here but I have nothing else to day", QNULL );

It acts as an empty string (but sticks nothing in the trace buffer) if there is nothing to trace dynamically. (QNULL is required because the trace macro requires that there be some syntactic element after the comma.)
QHEX is available to output a value in hex form:
```c++
QTRACE0( "Programming " << QVAR << " at offset 0x" << QHEX, value << offset );
```

QTRACE can be extended to support new datatypes as well – see below.
#### QPROF0( fixed, dynamic )
*(and QPROF1 … QPROF9.)*

This macro counts the amount of time that elapses between the QPROF statement and the exit from the enclosing basic block. The fixed and dynamic parameters are entered into the log when the trace is executed, exactly like the QTRACE macros.
When the innermost enclosing basic block is exited, a per-message time counter is incremented in the QuickTrace file, recording the number of processor ticks that elapsed between the initial trace and the exit point. context switches are not specially handled, so if the process was context switched out during this interval, then that time is still accounted in the total and billed to the QPROF statement.
Note: The macro uses a stack allocated object with a fixed name in a RAII pattern, and so QPROF can only be used once per basic block.
Note: QPROF0 is not a single statement. You cannot do this:
```c++
if( blah )
   QPROF0(...);
```

– this will generate a compiler error with only a moderately helpful message.
#### QPROF( fixed )
QPROF( fixed ) is a lightweight version of QPROFN(…). It measures the time until the end of the basic block, just like QPROF0. It updates a per-message hit counts, last hit time, and total time like a QTRACE does.

The differences are:
- it does not insert anything into the circular buffer
- it does less work, and so is a little bit faster
- it does not support any dynamic arguments

#### QPROF_S( fixed )
*(as well as QPROF_F_S, QPROFN_S, and so on)*

All profiling macros have self-profiling versions, which have identical names but with _S appended to the end. They still measure the time until the end of the basic block, but also record self time. Self time is the total time until the end of the block, excluding time spent in nested functions.

**Important details:** Only nested functions that contain their own self-profiling macros will be excluded from the caller's self time.
The self-profiling macros consume ~30 additional CPU cycles per statement. (QPROF consumes ~60 CPU cycles, while QPROF_S consumes ~90).
Self-profiling macros maintain per-thread data structures, so they can be used in multithreaded environments as long as multiple threads are not writing to the same tracefile.

When should QPROF_S be used instead of QPROF?
More information is better than less information, so the only disadvantage to using QPROF_S is the additional overhead. If the performance overhead is not a concern, then QPROF_S should likely be used.
The main advantages of QPROF_S are as follows:
- Self-profiling records valid data for recursive function calls, while normal profiling will double-count the time and report messy results.
- Self-profiling makes it much easier to determine which specific functions are causing bottlenecks.
- Self-profiling makes it possible to see how a specific function is affected by a change.
- QPROF_S records a valid timestamp, while QPROF does not.

### Which C++ types can be traced?
QuickTrace allows you to trace the following data types:
- `int8_t`,`uint8_t` and their larger size counterpart up to `int64_t` and `uint64_t`
- `char`, `bool`
- `double`, `float`
- `std::optional`
- `char const *`, `std::string`, `std::string_view`: Only the first 24
characters at most are traced. To override this 24 character limit, pass in an
int for the new size to the maxStringLen argument of `Quicktrace::initialize`
(argument 5). For instance, to set the limit to 80 characters, you could use:
`QuickTrace::initialize( %d.qt, 0, NULL, 0, 80);`

### Python API

QuickTrace is enabled from python by importing the QuickTrace module, which has the following methods:
- `initialize`: enables QuickTracing in this process if it is not already enabled
- `trace0` - `trace9`: used to trace a message. The arguments are comma-separated.
- `Var`: used to wrap a traced argument to indicate that it is dynamic

Here's an example of how to use QuickTrace from python.
```py
import QuickTrace
QuickTrace.initialize("qt-out-%d.qt")
qv = QuickTrace.Var
qt0 = QuickTrace.trace0

def foo( ent ):
   qt0( "Called foo(", qv(ent.name), ",", qv(ent.ipAddr) ")" )
   ...
```

It is often convenient to alias Var and trace0 to local variables with short names, like 'qv' and 'qt0'.
The use of qv ensures that those arguments are re-evaluated every time the trace statement is hit, rather than just the first time.

### Turning QuickTrace on
The QuickTrace macros all write to one file, and this file is used by all tracing in the process.
 QuickTrace is enabled by setting the filename with the `QuickTrace::initialize` function, like this:
```c++
QuickTrace::initialize( "MyProcess-%d.qt" );
```

The `initialize()` function takes two arguments:
-  The first argument is the name of a file to use as output. The first occurrence of the string `%d` (if it exists) in the filename is replaced by the pid of the process. If the filename argument is null, then quicktracing is not enabled, enabling this pattern: `QuickTrace::initialize( getenv( "QTFILE" ));`
- The second, optional, argument is of the SizeSpec type which is a pointer to a ten-entry long array of ints, where the nth entry represents the size in kilobytes, of the trace-level N circular buffer. So the first entry specifies the size of the level 0 trace buffer, and the last specifies the size of the level 9 trace buffer. The size array argument can be omitted, in which case a default size (currently 10K per level) is used.

There is a python API to do the same thing as well:
```py
import QuickTrace
Quicktrace.initialize(...)
```

#### Creating Multiple QuickTrace Files
Multiple QuickTrace files can be created by the same process by using the `initialize_handle` API. It takes the same arguments as the initialize API above but returns a TraceHandle that can be used with the lower-level `QTFMT_H`, `QPROF_H` and `QTRACE_H` macros.
The `initialize_handle` API can be used independently or in addition to the initialize API.

#### Multithreaded Processes
Multithreaded processes that require tracing from more than one thread must initialize QuickTrace through its MT specific APIs.
```
QuickTrace::initializeMt(...);
QuickTrace::initializeHandleMt(...);
```

The MT initialization calls must only be invoked a single time and not per-thread.
Note that even if the application code only uses traces in a single thread, libraries used by the application that are invoked from other threads may still issue traces against the `defaultQuickTraceHandle`. If this is a possibility, the MT initialization calls must be used.

QuickTrace can only support a single thread within a process writing to a TraceFile. When a multithreaded process needs the ability to trace from multiple threads, a separate TraceFile must be used by each of the threads. Thread specific TraceFile creation is automatically handled by the QuickTrace library. The TraceFile corresponding to a thread is created the first time that thread issues a trace message against a TraceHandle.

The first argument of initializeMt() and initializeHandleMt() provides the suffix for the names of the created trace files. The name of a file is constructed by concatenating the thread name with the provided suffix. The thread name must have been previously set through a call to `pthread_setname_np()`.

For example, a process could set its main thread name using: `pthread_setname_np( pthread_self(), "MyProcess-main" );`

It then initializes QuickTrace using: `QuickTrace::initializeMt( "-%d.qt" )`

When the first trace is issued by the main thread against the `defaultQuickTraceHandle`, the QuickTrace library will create a file named `MyProcess-main-12345.qt`.



### Where do the QuickTrace files go?
QuickTrace::initialize looks at the `QUICKTRACEDIR` environment variable and uses this as the directory to store the requested QuickTrace file. It is added as a path prefix to the filename specified by the user, unless the user-specified filename starts with a `/` or `.`. If the environment variable is set, but the directory does not exist, then QuickTrace is not initialized. If the environment variable is not set, then '.qt' under the current working directory is used.
#### Up to 3 saved qt files
When creating a QuickTrace file with filename x.qt, we do the following:
if x.qt.1 exists, we will rename it (with link()) to x.qt.2, but not overwrite x.qt.2, if it already exists.
Any existing x.qt file is renamed to x.qt.1, overwriting whatever x.qt.1 was there before.
So, there will be at most three QuickTrace files saved:
MyProcess.qt MyProcess.qt.1 MyProcess.qt.2
This ensures that the two most recent QuickTrace files are there, and the first
crash is there. This means that if a process crashes once for some reason and
then restarts quickly 10 more times for some other reason, we won't lose the
original reason why it crashed. The cost of this is 3 tracefiles per process.

## Tools for managing QuickTrace files

### qttail: read QuickTrace files
`qttail` is a tool that prints out the quicktrace message stored in quicktrace file.
By default it operates in tail mode, meaning that it'll run forever and print out
any new trace messages that appear:
```
>qttail .qt/myQuicktraceFile.qt
2023-04-07 10:15:20.314466 0 +147804167922769620 "Hello world: 42"
2023-04-07 10:15:20.314469 0 +5550 "Urgent trace message"
2023-04-07 10:15:20.314471 0 +3498 "Another event: "this is a test string""
^C to exit
```

It is also possible to run it in cat mode, where it will print the current contents
of the file then exit:
```
qttail -c MyFile.qt
```
There are other several useful options that are covered in the `--help` output of
`qttail`.


### qtclear: clears counters on a qt file
To clear the profiling and hit counters on a QuickTrace file, you simply run `qtclear`, like this:
```
qtclear MyFile.qt
```

And this will reset the counters and the last hit time back to zero.

### qtctl: control which trace messages are on
By default every QuickTrace message is enabled when the process starts. Sometimes there is too much noise from particular traces that you are not interested in and you can use qtctl to turn on or off individual traces.
- `qtctl show foo.qt` will show you which trace messages are on and which are off.
- `qtctl on foo.qt` turns everything on, and `qtctl off foo.qt` turns everything off.
- `qtctl` accepts a regexp argument with -r like this: `qtctl on -r <regexp> foo.qt`, which turns on every trace that matches `<regexp>`, using PCRE (perl-compatible RE) format. The regexp is matched against the message template (the one with %s in it) and against the filename. If there is a regexp match in either case then the message matches. `qtctl` on prints out all messages that got enabled that were not already enabled, so you can see what you changed.
- `qtctl` on also accepts a '-m' argument to turn on an individual message by its message id: `qtctl on -m 7 foo.qt` turns on message 7.
- `qtctl` show can also be given a regexp and it will show only messages that match that regexp.


`qtctl on|off` also takes an optional regexp or msgid argument, like `qtctl` on. So a useful thing to do might be something like this:
```
qtctl off MyProcess.qt
qtctl on -r Pattern1 MyProcess.qt
qtctl on -r Pattern2 MyProcess.qt
```
which turns off all trace messages except those that have Pattern1 or Pattern2 in their message text.
