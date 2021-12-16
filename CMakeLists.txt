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

project(generic_lock VERSION 0.0.1 LANGUAGES CXX)

# Add package modules to cmake module path
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# --------------------------------------------------
# MAIN_PROJECT CHECK: determine if package is built 
# as a subproject (using add_subdirectory) or if it 
# is the main project
# --------------------------------------------------
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MAIN_PROJECT ON)
endif()

option(PROJECT_BUILD_TESTS "Build the unit tests when BUILD_TESTING is enabled." ${MAIN_PROJECT})
option(PROJECT_BUILD_INSTALL "Install CMake targets during install step." ${MAIN_PROJECT})


if(PROJECT_BUILD_TESTS)
    # Enable testing
    enable_testing()
endif()

# ------------------------------------------------
# Including CMake Modules
# -------------------------------------------------

include(c++-standards)
include(code-coverage)
include(sanitizers)
include(tools)
include(GitSubmodules)

# --------------------------------------------------

# --------------------------------------------------
# Adding sub-directories to build
# ---------------------------------------------------

add_subdirectory(${PROJECT_SOURCE_DIR}/third_party)
add_subdirectory(${PROJECT_SOURCE_DIR}/generic_lock)
add_subdirectory(${PROJECT_SOURCE_DIR}/docs)

# ----------------------------------------------------
