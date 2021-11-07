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

# Get source files
file(
    GLOB_RECURSE 
    SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.[hc]pp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.[hc]
)

# Create executable
add_executable(
    ${TEST_BINARY} 
    ${SOURCES}
)

# Set include directories
target_include_directories(
    ${TEST_BINARY} 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Setup code coverage
target_code_coverage(${TEST_BINARY} AUTO)

# Add tests folder to targets include directories
target_include_directories(
    ${TEST_BINARY}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add preprocessor definitions
target_compile_definitions( ${TEST_BINARY} PRIVATE __TESTING__)

# Add libraries to link
target_link_libraries(
    ${TEST_BINARY}
    ${LIB}
    gtest
    gmock
)
