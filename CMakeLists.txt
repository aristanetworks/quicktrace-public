# Copyright (c) 2023, Arista Networks, Inc.
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

cmake_minimum_required(VERSION 3.20.2)
project(QuickTrace)

include(GNUInstallDirs)

set(BUILD_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/bld/include)
if(NOT EXISTS ${BUILD_INCLUDE_DIR}/${CMAKE_PROJECT_NAME})
   file(MAKE_DIRECTORY ${BUILD_INCLUDE_DIR}/${CMAKE_PROJECT_NAME})
endif()

set(
   INCLUDE_DIRS
      ${PROJECT_SOURCE_DIR}
      ${PROJECT_SOURCE_DIR}/test
      ${BUILD_INCLUDE_DIR}
)
include_directories(${INCLUDE_DIRS})

find_package(PythonInterp 3.9 REQUIRED)
find_package(PythonLibs 3.9 REQUIRED)

#------------------------------------------------------------------------------------
# LIBRARIES
#------------------------------------------------------------------------------------

# NOTE: Prior to building QuickTrace, we copy all relevant headers to
# ./bld/include/QuickTrace so that all source files can easily find any included
# headers which may be in the form of #include <QuickTrace/...>

#------------------------------------------------------------------------------------
# QuickTrace

set(
   QUICKTRACE_HEADERS
      ${CMAKE_SOURCE_DIR}/Qtc.h
      ${CMAKE_SOURCE_DIR}/QtcDecl.h
      ${CMAKE_SOURCE_DIR}/QuickTrace.h
      ${CMAKE_SOURCE_DIR}/QuickTraceCommon.h
      ${CMAKE_SOURCE_DIR}/QuickTraceFormatString.h
      ${CMAKE_SOURCE_DIR}/QuickTraceOptFormatter.h
      ${CMAKE_SOURCE_DIR}/Registration.h
      ${CMAKE_SOURCE_DIR}/QtFmtGeneric.h
      ${CMAKE_SOURCE_DIR}/WallClockQt.h
      ${CMAKE_SOURCE_DIR}/QuickTraceRingBuf.h
      ${CMAKE_SOURCE_DIR}/QuickTraceFileHeader.h
      ${CMAKE_SOURCE_DIR}/QuickTraceFormatStringTraits.h
      ${CMAKE_SOURCE_DIR}/QuickTraceFormatStringUtils.h
)
set(
   QUICKTRACE_SOURCES 
      ${CMAKE_SOURCE_DIR}/QuickTrace.cpp
      ${CMAKE_SOURCE_DIR}/Qtc.cpp
      ${CMAKE_SOURCE_DIR}/Registration.cpp
      ${CMAKE_SOURCE_DIR}/QtFmtGeneric.cpp
)
file(
   COPY
      ${QUICKTRACE_HEADERS}
   DESTINATION
      ${BUILD_INCLUDE_DIR}/${CMAKE_PROJECT_NAME}
)

add_library(QuickTrace SHARED)
target_sources(
   QuickTrace
   PRIVATE
      ${QUICKTRACE_SOURCES}
      ${QUICKTRACE_HEADERS}
)
set_property(
   TARGET
      QuickTrace
   PROPERTY
      PUBLIC_HEADER ${QUICKTRACE_HEADERS}
)
target_link_libraries(
   QuickTrace
   PRIVATE
      rt
      dl
      pthread
)
install(
   TARGETS
      QuickTrace
   LIBRARY
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
   PUBLIC_HEADER
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_PROJECT_NAME}
)

#------------------------------------------------------------------------------------
# MessageFormatter

set(
   MESSAGE_FORMATTER_HEADERS
      ${CMAKE_SOURCE_DIR}/MessageFormatter.h
      ${CMAKE_SOURCE_DIR}/MessageFormatterCommon.h
)
set(
   MESSAGE_FORMATTER_SOURCES 
      ${CMAKE_SOURCE_DIR}/MessageFormatter.cpp
)
file(
   COPY
      ${MESSAGE_FORMATTER_HEADERS}
   DESTINATION
      ${BUILD_INCLUDE_DIR}/${CMAKE_PROJECT_NAME}
)

add_library(MessageFormatter SHARED)
target_sources(
   MessageFormatter
   PRIVATE
      ${MESSAGE_FORMATTER_SOURCES}
      ${MESSAGE_FORMATTER_HEADERS}
)
set_property(
   TARGET
      MessageFormatter
   PROPERTY
      PUBLIC_HEADER ${MESSAGE_FORMATTER_HEADERS}
)
target_link_libraries(
   MessageFormatter
   PRIVATE
      rt
      dl
      pthread
)
install(
   TARGETS
      MessageFormatter
   LIBRARY
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
   PUBLIC_HEADER
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_PROJECT_NAME}
)

#------------------------------------------------------------------------------------
# PYTHON EXTENSION MODULES
#------------------------------------------------------------------------------------

add_library(QuickTracePy SHARED)
target_sources(
   QuickTracePy
   PRIVATE
      ${CMAKE_SOURCE_DIR}/_QuickTrace.cpp
)
target_include_directories(
   QuickTracePy
   PRIVATE
      ${PYTHON_INCLUDE_DIRS}
)
target_link_libraries(
   QuickTracePy
   PRIVATE
      ${PYTHON_LIBRARIES}
      pthread
      QuickTrace
)

#------------------------------------------------------------------------------------
# BINARIES
#------------------------------------------------------------------------------------

set(
   MESSAGE_PARSER_SOURCES
      ${CMAKE_SOURCE_DIR}/MessageParser.cpp
)
set(
   MESSAGE_PARSER_HEADERS
      ${CMAKE_SOURCE_DIR}/MessageParser.h
)
file(
   COPY
      ${MESSAGE_PARSER_HEADERS}
   DESTINATION
      ${BUILD_INCLUDE_DIR}/${CMAKE_PROJECT_NAME}
)

#------------------------------------------------------------------------------------
# qtclear

add_executable(
   qtclear
      qtclear.cpp
)
install(
   TARGETS
      qtclear
   RUNTIME
      DESTINATION ${CMAKE_INSTALL_BINDIR}
)

#------------------------------------------------------------------------------------
# qtctl

add_executable(
   qtctl
      qtctl.cpp
      ${MESSAGE_PARSER_SOURCES}
      ${MESSAGE_PARSER_HEADERS}
)
target_link_libraries(
   qtctl
   PRIVATE
      pcrecpp  
)
install(
   TARGETS
      qtctl
   RUNTIME
      DESTINATION ${CMAKE_INSTALL_BINDIR}
)

#------------------------------------------------------------------------------------
# qttail

add_executable(
   qttail
      qttail.cpp
      ${MESSAGE_PARSER_SOURCES}
      ${MESSAGE_PARSER_HEADERS}
)
set_target_properties(
   qttail
   PROPERTIES
      PUBLIC_HEADER ${MESSAGE_PARSER_HEADERS}
      COMPILE_FLAGS "-fomit-frame-pointer"
)
target_link_libraries(
   qttail
   PRIVATE
      dl
      MessageFormatter  
)
install(
   TARGETS
      qttail
   RUNTIME
      DESTINATION ${CMAKE_INSTALL_BINDIR}
   PUBLIC_HEADER
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_PROJECT_NAME}
)

#------------------------------------------------------------------------------------
# SCRIPTS
#------------------------------------------------------------------------------------

#------------------------------------------------------------------------------------
# qtpkt

install(
   PROGRAMS
      qtpkt
   DESTINATION
      ${CMAKE_INSTALL_BINDIR}
)

if(DEFINED BUILD_TEST)
   enable_testing()
   add_subdirectory(test)
endif()
