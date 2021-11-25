# Copyright 2021 Ketan Goyal
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.1)

# Require C++17
cxx_17()

# Cpp checks for code quality
cppcheck(
    --enable=warning,performance,portability,missingInclude
  --template="[{severity}][{id}] {message} {callstack} \(On {file}:{line}\)"
  --suppress=missingIncludeSystem --quiet --verbose --force
)

# Add project version information
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/version.hpp.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/version.hpp"
)

# Set project lib name
set(LIB ${PROJECT_NAME}_lib)
# Set test binary name
set(TEST_BINARY ${PROJECT_NAME}_test)

# Get source files
file(
    GLOB_RECURSE 
    SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.[hc]pp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.[hc]
)

# ---------------------------------------------
# Library
# ---------------------------------------------

# If source files found then make static lib else create header-only interface lib
if(${SOURCES})
    # Create static lib for running test suits
    add_library(
        ${LIB} 
        STATIC 
        ${SOURCES}
    )
    # Add include directories
    target_include_directories(
        ${LIB} 
        PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(
        ${LIB} 
        PUBLIC
        # NOTE: Add any required dependencies here
    )
    # Add libraries to link for lib
    if (${CMAKE_BUILD_TYPE} MATCHES Debug)
        target_compile_definitions(${LIB} PUBLIC DEBUG)
    else()
        target_compile_definitions(${LIB} PUBLIC NDEBUG)
    endif()
    # Setup code coverage
    target_code_coverage(${LIB})
else()
    # Create interface lib for running test suits
    add_library(${LIB} INTERFACE)
    # Add include directories
    target_include_directories(${LIB} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    # Set compiler flags
    if (${CMAKE_BUILD_TYPE} MATCHES Debug)
        target_compile_definitions(${LIB} INTERFACE DEBUG)
    else()
        target_compile_definitions(${LIB} INTERFACE NDEBUG)
    endif()
    # Setup code coverage
    target_code_coverage(${LIB} INTERFACE)
endif()

# --------------------------------------------


# --------------------------------------------
# Unit testing
# --------------------------------------------

# Tests
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)

# --------------------------------------------
