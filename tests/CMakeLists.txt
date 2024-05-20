#############################################################################
# Name:       CMakeLists.txt
# Purpose:    Unit test runner for Wisteria DV
# Author:      Blake Madden
# Created:     2022-10-31
# Copyright: (c) 2022 Blake Madden
# License:     3-Clause BSD license
#############################################################################

cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Options:
# - USE_CLANG_TIDY to run clang-tidy

find_program(CLANG_TIDY_COMMAND
      NAMES
         clang-tidy
      NO_CACHE)
if(CLANG_TIDY_COMMAND AND USE_CLANG_TIDY)
    if(MSVC)
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_COMMAND} --extra-arg=/EHsc)
    else()
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_COMMAND})
    endif()
endif()

project(WisteriaTestRunner)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

message(STATUS "Finding Catch2...")
# place Catch2 at the same folder level as this repo if it isn't installed
# (you will need to do this on Windows or macOS or if version 3 of Catch2 isn't installed)
get_filename_component(_fullpath "${_dir}" REALPATH)
if(EXISTS "${_fullpath}" AND EXISTS "${_fullpath}/../../Catch2")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../Catch2 ${CMAKE_CURRENT_BINARY_DIR}/Catch2)
else()
    # ...otherwise, see if it is installed
    find_package(Catch2 3 REQUIRED)
endif()

# Define the build target for the executable
message(STATUS "Preparing the test runner...")
include(${CMAKE_SOURCE_DIR}/../tools/testfiles.cmake)
set(LIB_SRC_FILES
    ../src/import/cpp_extract_text.cpp
    ../src/import/doc_extract_text.cpp
    ../src/import/docx_extract_text.cpp
    ../src/import/html_extract_text.cpp
    ../src/import/odt_odp_extract_text.cpp
    ../src/import/markdown_extract_text.cpp
    ../src/import/postscript_extract_text.cpp
    ../src/import/xlsx_extract_text.cpp
    ../src/import/rtf_extract_text.cpp)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
add_executable(${CMAKE_PROJECT_NAME} ${TEST_SRC_FILES} ${LIB_SRC_FILES})

# Link required libraries to the executable
if(WIN32)
    TARGET_COMPILE_DEFINITIONS(${CMAKE_PROJECT_NAME} PUBLIC __UNITTEST _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
    TARGET_COMPILE_OPTIONS(${CMAKE_PROJECT_NAME} PRIVATE /Zc:__cplusplus /fsanitize=address)
    TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} PRIVATE Catch2::Catch2WithMain)
elseif(UNIX)
    TARGET_COMPILE_DEFINITIONS(${CMAKE_PROJECT_NAME} PUBLIC __UNITTEST)
    FIND_LIBRARY(TBB_LIB tbb)
    TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${TBB_LIB} Catch2::Catch2WithMain)
endif()

# Copy datasets into build folder
message(STATUS "Copying datasets...")
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/../datasets $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/datasets)

# load the test cases into the runner
include(CTest)
include(Catch)
catch_discover_tests(${CMAKE_PROJECT_NAME})
