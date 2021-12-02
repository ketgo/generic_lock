cmake_minimum_required(VERSION 3.1)

# Documentation
include(doxygen)

# Build documentation
build_docs(
    TARGET_NAME doxygen-${PROJECT_NAME}
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen
    PROCESS_DOXYFILE
)