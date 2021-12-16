# Function to update a git submodule.
#
# Example: 
#    initialize_submodule(third_party/googletest)
#    add_subdirectory(third_party/googletest)
#
function(initialize_submodule DIRECTORY)
  find_package(Git QUIET REQUIRED)
  if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    message(STATUS "Updating ${DIRECTORY} submodule ...")
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init ${DIRECTORY}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_EXIT_CODE)
    if(NOT GIT_EXIT_CODE EQUAL "0")
      message(FATAL_ERROR "${GIT_EXECUTABLE} submodule update --init dependencies/${DIRECTORY} failed with exit code ${GIT_EXIT_CODE}, please checkout submodules")
    endif()
  endif()
endfunction(initialize_submodule)