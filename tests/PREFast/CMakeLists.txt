#############################################################################
# Name:        CMakeListst.txt
# Purpose:     PREFast analysis project for Wisteria
# Author:      Blake Madden
# Created:     2023-06-04
# Copyright:   (c) 2023 Blake Madden
# Licence:     3-Clause BSD licence
#############################################################################

PROJECT(WDVDemo)

CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED True)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} wdvprefast.cpp
    src/import/cpp_extract_text.cpp
    src/import/doc_extract_text.cpp
    src/import/docx_extract_text.cpp
    src/import/html_extract_text.cpp
    src/import/odt_odp_extract_text.cpp
    src/import/postscript_extract_text.cpp
    src/import/xlsx_extract_text.cpp
    src/import/rtf_extract_text.cpp)

IF(MSVC)
    TARGET_COMPILE_DEFINITIONS(${CMAKE_PROJECT_NAME} PUBLIC _CRT_SECURE_NO_WARNINGS)
    TARGET_COMPILE_OPTIONS(${CMAKE_PROJECT_NAME} PUBLIC /Zc:__cplusplus /MP /W3 /WX)
ENDIF()
