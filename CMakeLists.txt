#############################################################################
# Name:        CMakeListst.txt
# Purpose:     Build script for Wisteria Dataviz library and demo project
# Author:      Blake Madden
# Created:     2022-01-17
# Copyright:   (c) 2005-2022 Blake Madden
# Licence:     3-Clause BSD licence
#############################################################################

# Declare the minimum required CMake version
CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED True)

IF(NOT CMAKE_CONFIGURATION_TYPES)
    GET_PROPERTY(HAVE_MULTI_CONFIG_GENERATOR GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    # Set default configuration types for multi-config generators
    IF(HAVE_MULTI_CONFIG_GENERATOR)
        SET(CMAKE_CONFIGURATION_TYPES "Debug;Release")
    ENDIF()
ENDIF()

# https://blog.kitware.com/cmake-and-the-default-build-type/
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    SET(default_build_type "Debug")
    MESSAGE(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    SET(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
ENDIF()

PROJECT(Wisteria)

# Setup wxWidgets
########################

# UNIX systems: set wxWidgets_CONFIG_EXECUTABLE to the path of wx-config
# Windows: set wxWidgets_ROOT_DIR to the wxWidgets root path

# Find wxWidgets and select its components
FIND_PACKAGE(wxWidgets REQUIRED core base adv html ribbon xml xrc qa propgrid OPTIONAL_COMPONENTS net)
# Include the wxWidgets use file to initialize various settings
INCLUDE(${wxWidgets_USE_FILE})
MESSAGE(STATUS "Adding wxWidgets libraries: ${wxWidgets_LIBRARIES}")
MESSAGE(STATUS "Adding wxWidgets configuration file: ${wxWidgets_CONFIGURATION}/wx/setup.h")

IF(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    # If no deployment target has been set default to the minimum supported
    # OS version (this has to be set before the first project() call)
    IF(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        SET(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "iOS Deployment Target")
    ELSE()
        SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.10 CACHE STRING "macOS Deployment Target")
    ENDIF()
ENDIF()

# Build Wisteria library
########################
# (run "tools/Build CMake Files List.R" to update this)
INCLUDE(${CMAKE_SOURCE_DIR}/tools/files.cmake)
ADD_LIBRARY(Wisteria STATIC ${WISTERIA_SRC})
TARGET_INCLUDE_DIRECTORIES(Wisteria PUBLIC ${wxWidgets_INCLUDE_DIRS})

# Set definitions, warnings, and optimizations (will propagate to the demo project also)
IF(MSVC)
    TARGET_COMPILE_DEFINITIONS(Wisteria PUBLIC __WXMSW__ _CRT_SECURE_NO_WARNINGS
        $<$<CONFIG:Debug>:__WXDEBUG__ wxDEBUG_LEVEL=2> $<$<CONFIG:Release>:wxDEBUG_LEVEL=0>)
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
    TARGET_COMPILE_OPTIONS(Wisteria PUBLIC /Zc:__cplusplus /MP /W3 /WX /wd6211
                           $<$<CONFIG:Debug>:/Od> $<$<CONFIG:Release>:/O2>)
ELSE()
    TARGET_COMPILE_DEFINITIONS(Wisteria PUBLIC __WXMSW__
        $<$<CONFIG:Debug>:__WXDEBUG__ wxDEBUG_LEVEL=2> $<$<CONFIG:Release>:wxDEBUG_LEVEL=0>)
    TARGET_COMPILE_OPTIONS(Wisteria PUBLIC -Wall -Wextra -Wpedantic
                           $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Release>:-O2>)
ENDIF()

# Build the library demo
########################
SET(DEMO_SRC_FILES ${CMAKE_SOURCE_DIR}/demo/demo.cpp)

IF(WIN32)
    # Include an RC file for windows
    SET(wxUSE_DPI_AWARE_MANIFEST 1)
    LIST(APPEND DEMO_SRC_FILES ${wxWidgets_ROOT_DIR}/include/wx/msw/wx.rc)
    LIST(APPEND DEMO_SRC_FILES ${wxWidgets_ROOT_DIR}/include/wx/msw/wx_dpi_aware.manifest)
    CONFIGURE_FILE(${wxWidgets_ROOT_DIR}/include/wx/msw/wx_dpi_aware.manifest
                   ${CMAKE_SOURCE_DIR}/demo/wx_dpi_aware.manifest
                   COPYONLY)
ELSEIF(APPLE)
    # Add an icon for the apple .app file
    LIST(APPEND DEMO_SRC_FILES ${CMAKE_SOURCE_DIR}/demo/wxmac.icns)
ENDIF()

# Define the build target for the executable
MESSAGE(STATUS "Building the demonstration program...")
SET(DEMO_APP WisteriaDemo)
ADD_EXECUTABLE(${DEMO_APP} WIN32 MACOSX_BUNDLE ${DEMO_SRC_FILES})
# Link required libraries to the executable
TARGET_LINK_LIBRARIES(${DEMO_APP} Wisteria ${wxWidgets_LIBRARIES})

# Embed the manifest file
IF(WIN32)
    SET(CMAKE_SHARED_LINKER_FLAGS /MANIFEST:NO)
    ADD_CUSTOM_COMMAND(
        TARGET ${DEMO_APP}
        POST_BUILD
        # Note about mt.exe, #1 is for EXEs and #2 is for DLLs
        COMMAND "mt.exe" -manifest \"${CMAKE_SOURCE_DIR}/demo/wx_dpi_aware.manifest\"
                         -outputresource:\"$<TARGET_FILE:${DEMO_APP}>\"\;\#1
        COMMENT "Adding manifest...")
    MESSAGE(STATUS "Adding manifest...")
ENDIF()

# Copy resources and datasets into build folder
MESSAGE(STATUS "Copying demo resources...")
ADD_CUSTOM_COMMAND(TARGET ${DEMO_APP}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/demo/res $<TARGET_FILE_DIR:${DEMO_APP}>/res)
ADD_CUSTOM_COMMAND(TARGET ${DEMO_APP}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/datasets $<TARGET_FILE_DIR:${DEMO_APP}>/datasets)

IF(APPLE)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
        RESOURCE "demo/wxmac.icns"
        MACOSX_BUNDLE_ICON_FILE wxmac.icns
        MACOSX_BUNDLE_COPYRIGHT "Copyright Blake Madden"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.wisteria.demo")
ENDIF()
