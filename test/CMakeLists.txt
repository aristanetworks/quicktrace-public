# Copyright (c) 2023 Arista Networks, Inc.  All rights reserved.
# Arista Networks, Inc. Confidential and Proprietary.

set(TEST_DIR ${PROJECT_SOURCE_DIR}/bld/test)

#------------------------------------------------------------------------------------
# TEST LIBRARIES
#------------------------------------------------------------------------------------

#------------------------------------------------------------------------------------
# QuickTraceFormatTest

add_library(QuickTraceFormatTest SHARED)
target_sources(
   QuickTraceFormatTest
   PRIVATE
      QuickTraceFormatTest.cpp
      QuickTraceFormatTest.h     
)
set_property(
   TARGET QuickTraceFormatTest
   PROPERTY
      PUBLIC_HEADER QuickTraceFormatTest.h
)
target_link_libraries(
   QuickTraceFormatTest
   PRIVATE
      rt
      dl
      pthread
      QuickTrace
)

#------------------------------------------------------------------------------------
# QuickTraceFormatBasicTestFunctions

add_library(QuickTraceFormatBasicTestFunctions SHARED)
target_sources(
   QuickTraceFormatBasicTestFunctions
   PRIVATE
      QuickTraceFormatBasicTestFunctions.cpp
      QuickTraceFormatBasicTestFunctions.h
)
set_property(
   TARGET QuickTraceFormatTest
   PROPERTY
      PUBLIC_HEADER QuickTraceFormatBasicTestFunctions.h
)
target_link_libraries(
   QuickTraceFormatBasicTestFunctions
   PRIVATE
      rt
      dl
      pthread
      QuickTrace
      QuickTraceFormatTest
)

#------------------------------------------------------------------------------------
# QtTailPluginTest

add_library(
   QtTailPluginTest
   SHARED
      QtTailPluginTest.cpp
)
target_link_libraries(
   QtTailPluginTest
   PRIVATE
      rt
      dl
      pthread
      MessageFormatter
)

#------------------------------------------------------------------------------------
# QuickTraceTestMessages

add_library(
   QuickTraceTestMessages
   SHARED
      QuickTraceTestMessages.cpp
)
target_link_libraries(
   QuickTraceTestMessages
   PRIVATE
      QuickTrace
)

#------------------------------------------------------------------------------------
# QtcTestMessages

add_library(
   QtcTestMessages
   SHARED
      QtcTestMessages.c
)
target_link_libraries(
   QtcTestMessages
   PRIVATE
      QuickTrace
)

#------------------------------------------------------------------------------------
# BINARIES INVOKED BY SCRIPT TESTS
#------------------------------------------------------------------------------------

#------------------------------------------------------------------------------------
# QtPluginType

add_executable(QtPluginType QtPluginType.cpp)
target_link_libraries(
   QtPluginType
   PRIVATE
      QuickTrace
)
set_target_properties(
   QtPluginType
   PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${TEST_DIR}
)

#------------------------------------------------------------------------------------
# QtMaxStrLen

add_executable(QtMaxStrLen QtMaxStrLen.cpp)
target_link_libraries(
   QtMaxStrLen
   PRIVATE
      QuickTrace
)
set_target_properties(
   QtMaxStrLen
   PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${TEST_DIR}
)

#------------------------------------------------------------------------------------
# QuickTraceMultipleThreadsTest

add_executable(QuickTraceMultipleThreadsTest QuickTraceMultipleThreadsTest.cpp)
target_link_libraries(
   QuickTraceMultipleThreadsTest
   PRIVATE
      QuickTrace
)
set_target_properties(
   QuickTraceMultipleThreadsTest
   PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${TEST_DIR}
)

#------------------------------------------------------------------------------------
# QuickTraceTest8

add_executable(QuickTraceTest8 QuickTraceTest8.cpp)
target_link_libraries(
   QuickTraceTest8
   PRIVATE
      QuickTrace
)
set_target_properties(
   QuickTraceTest8
   PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${TEST_DIR}
)

#------------------------------------------------------------------------------------
# QtOffBitTest

# We build QuickTrace separately in this test to enable the rdtsc mocker. It allows
# us to manipulate the rdtsc value to simulate a scenario where we get a rdtsc value
# so high that the `off` bit also gets set in lastTsc.
add_executable(
   QtOffBitTest
      QtOffBitTest.cpp
      ${QUICKTRACE_HEADERS}
      ${QUICKTRACE_SOURCES}
)
target_link_libraries(
   QtOffBitTest
   PRIVATE
      rt
      dl
      pthread
)
target_compile_definitions(
   QtOffBitTest
   PRIVATE
      QT_RDTSC_MOCK
)
set_target_properties(
   QtOffBitTest
   PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${TEST_DIR}
)

#------------------------------------------------------------------------------------
# QtTailCorruptionTest

add_executable(QtTailCorruptionTest QtTailCorruptionTest.cpp)
target_link_libraries(
   QtTailCorruptionTest
   PRIVATE
      QuickTraceFormatTest
)
set_target_properties(
   QtTailCorruptionTest
   PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${TEST_DIR}
)


#------------------------------------------------------------------------------------
# QtTailNewLogMessageOrderTest

add_executable(QtTailNewLogMessageOrderTest QtTailNewLogMessageOrderTest.cpp)
target_link_libraries(
   QtTailNewLogMessageOrderTest
   PRIVATE
      QuickTraceFormatTest
      gtest
)
set_target_properties(
   QtTailNewLogMessageOrderTest
   PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${TEST_DIR}
)

#------------------------------------------------------------------------------------
# QtTailOverrunTest

add_executable(QtTailOverrunTest QtTailOverrunTest.cpp)
target_link_libraries(
   QtTailOverrunTest
   PRIVATE
      QuickTraceFormatTest
)
set_target_properties(
   QtTailOverrunTest
   PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${TEST_DIR}
)

#------------------------------------------------------------------------------------
# QtFmtTest

add_executable(QtFmtTest QtFmtTest.cpp)
target_link_libraries(
   QtFmtTest
   PRIVATE
      QuickTrace
)
set_target_properties(
   QtFmtTest
   PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${TEST_DIR}
)

#------------------------------------------------------------------------------------
# BINARY BREADTH TESTS
#------------------------------------------------------------------------------------

#------------------------------------------------------------------------------------
# QtRotateLogTest

add_executable(QtRotateLogTest QtRotateLogTest.cpp)
target_link_libraries(
   QtRotateLogTest
   PRIVATE
      QuickTrace
)
add_test(
   NAME QtRotateLogTest
   COMMAND QtRotateLogTest
)

#------------------------------------------------------------------------------------
# QtRegisterTest

add_executable(QtRegisterTest QtRegisterTest.cpp)
target_link_libraries(
   QtRegisterTest
   PRIVATE
      QuickTrace
)
add_test(NAME QtRegisterTest COMMAND QtRegisterTest)

#------------------------------------------------------------------------------------
# QtcTest6

add_executable(QtcTest6 QtcTest6.c)
target_link_libraries(
   QtcTest6
   PRIVATE
      QuickTrace
)
add_test(NAME QtcTest6 COMMAND QtcTest6)

#------------------------------------------------------------------------------------
# QtcUnitTest

add_executable(QtcUnitTest QtcUnitTest.c)
target_link_libraries(
   QtcUnitTest
   PRIVATE
      QuickTrace
)
add_test(NAME QtcUnitTest COMMAND QtcUnitTest)

#------------------------------------------------------------------------------------
# QuickTraceFormatBasicTest

add_executable(QuickTraceFormatBasicTest QuickTraceFormatBasicTest.cpp)
target_link_libraries(
   QuickTraceFormatBasicTest
   PRIVATE
      QuickTraceFormatTest
      QuickTraceFormatBasicTestFunctions
)
add_test(NAME QuickTraceFormatBasicTest COMMAND QuickTraceFormatBasicTest)

#------------------------------------------------------------------------------------
# QuickTraceFilePathTest

add_executable(QuickTraceFilePathTest QuickTraceFilePathTest.cpp)
target_link_libraries(
   QuickTraceFilePathTest
   PRIVATE
      QuickTrace
      gtest
)
add_test(NAME QuickTraceFilePathTest COMMAND QuickTraceFilePathTest)

#------------------------------------------------------------------------------------
# QuickTraceTest7

add_executable(QuickTraceTest7 QuickTraceTest7.cpp)
target_link_libraries(
   QuickTraceTest7
   PRIVATE
      QuickTrace
)
add_test(NAME QuickTraceTest7 COMMAND QuickTraceTest7)

#------------------------------------------------------------------------------------
# TestFmtMsgDesc

add_executable(TestFmtMsgDesc TestFmtMsgDesc.cpp)
target_link_libraries(
   TestFmtMsgDesc
   PRIVATE
      QuickTrace
      gtest
)
add_test(NAME TestFmtMsgDesc COMMAND TestFmtMsgDesc)

#------------------------------------------------------------------------------------
# PYTHON BREADTH TESTS
#------------------------------------------------------------------------------------

add_test(
   NAME QtArguments
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtArguments.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtDiskFull
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtDiskFull.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtVaryingArgs
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtVaryingArgs.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtWrongArgType
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtWrongArgType.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QuickTraceMultipleThreadsTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QuickTraceMultipleThreadsTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtctlTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtctlTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtcatSizeSpec
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtcatSizeSpec.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailCloseStdoutTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailCloseStdoutTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailDeleteMultiFileTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailDeleteMultiFileTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailDeleteSingleFileTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailDeleteSingleFileTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailForeverLogTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailForeverLogTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailMultiFileTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailMultiFileTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailNewLogMessageTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailNewLogMessageTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailReopenTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailReopenTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailRolloverTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailRolloverTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailSingleFileTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailSingleFileTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailCorruptionTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailCorruptionTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailNewLogMessageOrderTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailNewLogMessageOrderTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtTailOverrunTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtTailOverrunTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
add_test(
   NAME QtOffBitTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtOffBitTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)

add_test(
   NAME QtFmtTest
   COMMAND
      ${Python_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/QtFmtTest.py
   WORKING_DIRECTORY ${TEST_DIR}
)
