#############################################################################
# Name:       CMakeLists.txt
# Purpose:    Unit test runner for Wisteria DV
# Author:      Blake Madden
# Created:     2022-10-31
# Copyright: (c) 2022 Blake Madden
# License:     3-Clause BSD license
#############################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED True)

PROJECT(WisteriaTestRunner)

ADD_COMPILE_OPTIONS("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
ADD_COMPILE_OPTIONS("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

MESSAGE(STATUS "Finding Catch2...")
# place Catch2 at the same folder level as this repo if it isn't installed
# (you will need to do this on Windows or macOS or if version 3 of Catch2 isn't installed)
GET_FILENAME_COMPONENT(_fullpath "${_dir}" REALPATH)
IF(EXISTS "${_fullpath}" AND EXISTS "${_fullpath}/../../Catch2")
    ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../Catch2 ${CMAKE_CURRENT_BINARY_DIR}/Catch2)
ELSE()
    # ...otherwise, see if it is installed
    FIND_PACKAGE(Catch2 3 REQUIRED)
ENDIF()

# Define the build target for the executable
MESSAGE(STATUS "Preparing the test runner...")
INCLUDE(${CMAKE_SOURCE_DIR}/../tools/testfiles.cmake)
SET(LIB_SRC_FILES
    ../src/import/cpp_extract_text.cpp
    ../src/import/doc_extract_text.cpp
    ../src/import/docx_extract_text.cpp
    ../src/import/html_extract_text.cpp
    ../src/import/odt_odp_extract_text.cpp
    ../src/import/markdown_extract_text.cpp
    ../src/import/postscript_extract_text.cpp
    ../src/import/xlsx_extract_text.cpp
    ../src/import/rtf_extract_text.cpp)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} ${TEST_SRC_FILES} ${LIB_SRC_FILES})

# Link required libraries to the executable
IF(WIN32)
    TARGET_COMPILE_DEFINITIONS(${CMAKE_PROJECT_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS __UNITTEST _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
    TARGET_COMPILE_OPTIONS(${CMAKE_PROJECT_NAME} PRIVATE /Zc:__cplusplus /fsanitize=address)
    TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} PRIVATE Catch2::Catch2WithMain)
ELSEIF(UNIX)
    TARGET_COMPILE_DEFINITIONS(${CMAKE_PROJECT_NAME} PUBLIC __UNITTEST)
    FIND_LIBRARY(TBB_LIB tbb)
    TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${TBB_LIB} Catch2::Catch2WithMain)
ENDIF()

# Copy datasets into build folder
MESSAGE(STATUS "Copying datasets...")
ADD_CUSTOM_COMMAND(TARGET ${CMAKE_PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/../datasets $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/datasets)

# load the test cases into the runner
INCLUDE(CTest)
INCLUDE(Catch)
CATCH_DISCOVER_TESTS(${CMAKE_PROJECT_NAME})
