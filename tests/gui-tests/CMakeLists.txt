#############################################################################
# Name:        CMakeLists.txt
# Purpose:     Unit test runner for Wisteria DV
# Author:      Blake Madden
# Created:     2022-10-31
# Copyright:   (c) 2022 Blake Madden
# License:     3-Clause BSD license
#############################################################################

cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_CONFIGURATION_TYPES)
    get_property(HAVE_MULTI_CONFIG_GENERATOR GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    # Set default configuration types for multi-config generators
    if(HAVE_MULTI_CONFIG_GENERATOR)
        set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
    endif()
endif()

# https://blog.kitware.com/cmake-and-the-default-build-type/
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(default_build_type "Debug")
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

project(WisteriaGuiTestRunner)

# Setup wxWidgets
##############################################################

# Find wxWidgets (wxWidgetsConfig.cmake, specifically) and select its components
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../wxWidgets")
    if(WIN32)
        set(wxWidgets_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../wxWidgets/out/install/x64-${CMAKE_BUILD_TYPE}/lib/cmake/wxWidgets")
    elseif(UNIX)
        set(wxWidgets_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../wxWidgets/wxlib/lib/cmake/wxWidgets")
    endif()
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../wxWidgets")
    if(WIN32)
        set(wxWidgets_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../wxWidgets/out/install/x64-${CMAKE_BUILD_TYPE}/lib/cmake/wxWidgets")
    elseif(UNIX)
        set(wxWidgets_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../wxWidgets/wxlib/lib/cmake/wxWidgets")
    endif()
endif()

message(STATUS "Adding wxWidgets CMake cofig folder: ${wxWidgets_DIR}")
if(NOT EXISTS "${wxWidgets_DIR}/wxWidgetsConfig.cmake" AND NOT EXISTS "${wxWidgets_DIR}/wxwidgets-config.cmake")
    message(WARNING "Cannot find wxWidgets CMake configuration. (Looked in ${wxWidgets_DIR}.)")
endif()
set(wxWidgets_USE_STATIC 1)
find_package(wxWidgets 3.3 COMPONENTS core base xrc propgrid xml aui ribbon qa stc net richtext REQUIRED CONFIG)

# Include the wxWidgets use file to initialize various settings
message(STATUS "Adding wxWidgets libraries: ${wxWidgets_LIBRARIES}")
message(STATUS "Adding wxWidgets: ${wxPLATFORM_LIB_DIR}")

if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    # If no deployment target has been set default to the minimum supported
    # OS version (this has to be set before the first project() call)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        SET(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "iOS Deployment Target")
    else()
        SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.10 CACHE STRING "macOS Deployment Target")
    endif()
endif()

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# Build Wisteria GUI test runner
########################
set(TEST_APP WisteriaGuiTestRunner)
# (run "tools/Build CMake Files List.R" to update this)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tools/guilibfiles_testing.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../tools/guitestfiles.cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_executable(${TEST_APP} WIN32 MACOSX_BUNDLE ${WISTERIA_SRC} ${TEST_SRC_FILES})

message(STATUS "Finding Catch2...")
# place Catch2 at the same folder level as this repo if it isn't installed
# (you will need to do this on Windows or macOS or if version 3 of Catch2 isn't installed)
get_filename_component(_fullpath "${_dir}" REALPATH)
if(EXISTS "${_fullpath}" AND EXISTS "${_fullpath}/../../../Catch2")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../Catch2 ${CMAKE_CURRENT_BINARY_DIR}/Catch2)
else()
    # ...otherwise, see if it is installed
    find_package(Catch2 3 REQUIRED)
endif()

# Set definitions, warnings, and optimizations (will propagate to the demo project also)
if(MSVC)
    target_compile_definitions(${TEST_APP} PUBLIC __WXMSW__ _CRT_SECURE_NO_WARNINGS __UNITTEST _UNICODE
        $<$<CONFIG:Debug>:__WXDEBUG__> $<$<CONFIG:Release>:NDEBUG>)
    # /Zc:__cplusplus tells MSVC to set the C++ version what we are
    # actually compiling as. The default behavior in MSVC is to say that the
    # C++ version is 98 always (for compatibility reasons).

    # /MP enables multi-core build support, speeding up compilation time.

    # /W3 cranks up the warning level, /WX treats all warnings as error.
    # Note that simply using /W4 causes a warning that it is overriding
    # /W3, and then that turns into an error because of /W4. So /W3 is the
    # highest you can go with /WX.

    # /wd6211 turns off C6211 warning: leaking memory due to an exception.
    # wxWidgets uses heap-based objects for most everything, and MVSC complains
    # about not wrapping all of this logic in try blocks.
    target_compile_options(${TEST_APP} PUBLIC /Zc:__cplusplus /MP /W3 /WX /wd6211
                           $<$<CONFIG:Debug>:/Od> $<$<CONFIG:Release>:/O2>)
# Placeholder code for macOS (needs testing)
#ELSEIF(APPLE)
#    target_compile_definitions(${TEST_APP} PUBLIC __WXOSX__ __UNITTEST
#        $<$<CONFIG:Debug>:__WXDEBUG__ wxDEBUG_LEVEL=2> $<$<CONFIG:Release>:wxDEBUG_LEVEL=0>)
#    target_compile_options(${TEST_APP} PUBLIC -Wall -Wextra -Wpedantic
#                           $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Release>:-O2>)
elseif(UNIX)
    target_compile_definitions(${TEST_APP} PUBLIC __WXGTK__ __UNITTEST
        $<$<CONFIG:Debug>:__WXDEBUG__ wxDEBUG_LEVEL=2> $<$<CONFIG:Release>:wxDEBUG_LEVEL=0>)
    target_compile_options(${TEST_APP} PUBLIC -Wall -Wextra -Wpedantic
                           $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Release>:-O2>)
endif()

# Link required libraries to the executable
if(WIN32)
    target_link_libraries(${TEST_APP} PRIVATE Catch2::Catch2 ${wxWidgets_LIBRARIES})
elseif(UNIX)
    find_library(TBB_LIB tbb)
    target_link_libraries(${TEST_APP} ${wxWidgets_LIBRARIES} ${TBB_LIB} Catch2::Catch2)
endif()

# Add OpenMP for mutlithreading
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP available; will be included.")
    target_link_libraries(${TEST_APP} PUBLIC OpenMP::OpenMP_CXX)
endif()

# Copy datasets and test files into build folder
message(STATUS "Copying datasets...")
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_SOURCE_DIR}/../../datasets $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/datasets)

message(STATUS "Copying test files...")
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_SOURCE_DIR}/test_files $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/test_files)

# load the test cases into the runner
include(CTest)
include(Catch)
catch_discover_tests(${TEST_APP})
