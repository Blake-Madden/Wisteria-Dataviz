#############################################################################
# Name:        CMakeLists.txt
# Purpose:     Build script for Wisteria Dataviz library and demo project
# Author:      Blake Madden
# Created:     2022-01-17
# Copyright:   (c) 2005-2022 Blake Madden
# Licence:     3-Clause BSD licence
#############################################################################

# Declare the minimum required CMake version
# Note that CMake 3.24 is needed if compiling wxWidgets 3.2
CMAKE_MINIMUM_REQUIRED(VERSION 3.14)
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

# Build the documentation
########################
# https://vicrucann.github.io/tutorials/quick-cmake-doxygen/
FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
    # set input and output files
    SET(DOXYGEN_IN "${CMAKE_SOURCE_DIR}/docs/doxygen/Doxyfile")
    IF(EXISTS ${DOXYGEN_IN})
        MESSAGE("Doxygen build started")

        # note the option ALL which allows to build the docs together with the application
        ADD_CUSTOM_TARGET( doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_IN}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/docs/doxygen"
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )
    ELSE()
        MESSAGE("Doxygen project file missing (${DOXYGEN_IN}).")
    ENDIF()
ELSE(DOXYGEN_FOUND)
    MESSAGE("Doxygen not found. API documentation will not be generated.")
ENDIF()

# Setup wxWidgets
########################

# Linx: install the wxWidgets development files to your system
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
ADD_LIBRARY(WisteriaLib STATIC ${WISTERIA_SRC})
TARGET_INCLUDE_DIRECTORIES(WisteriaLib PUBLIC ${wxWidgets_INCLUDE_DIRS})

# Set definitions, warnings, and optimizations (will propagate to the demo project also)
IF(MSVC)
    TARGET_COMPILE_DEFINITIONS(WisteriaLib PUBLIC __WXMSW__ _CRT_SECURE_NO_WARNINGS
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
    TARGET_COMPILE_OPTIONS(WisteriaLib PUBLIC /Zc:__cplusplus /MP /W3 /WX /wd6211
                           $<$<CONFIG:Debug>:/Od> $<$<CONFIG:Release>:/O2>)
ELSEIF(MINGW OR MSYS)
    TARGET_COMPILE_DEFINITIONS(WisteriaLib PUBLIC __WXMSW__
            $<$<CONFIG:Debug>:__WXDEBUG__ wxDEBUG_LEVEL=2> $<$<CONFIG:Release>:wxDEBUG_LEVEL=0>)
    TARGET_COMPILE_OPTIONS(WisteriaLib PUBLIC -Wall -Wextra -Wpedantic -Wshadow
                           $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Release>:-O2>)
# TODO: Placeholder code for macOS (needs testing)
#ELSEIF(APPLE)
#    TARGET_COMPILE_DEFINITIONS(WisteriaLib PUBLIC __WXOSX__
#        $<$<CONFIG:Debug>:__WXDEBUG__ wxDEBUG_LEVEL=2> $<$<CONFIG:Release>:wxDEBUG_LEVEL=0>)
#    TARGET_COMPILE_OPTIONS(WisteriaLib PUBLIC -Wall -Wextra -Wpedantic
#                           $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Release>:-O2>)
ELSEIF(UNIX)
    TARGET_COMPILE_DEFINITIONS(WisteriaLib PUBLIC __WXGTK__
        $<$<CONFIG:Debug>:__WXDEBUG__ wxDEBUG_LEVEL=2> $<$<CONFIG:Release>:wxDEBUG_LEVEL=0>)
    TARGET_COMPILE_OPTIONS(WisteriaLib PUBLIC -Wall -Wextra -Wpedantic -Wshadow
                           $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Release>:-O2>)
ENDIF()

# Build the library demo
########################
SET(DEMO_SRC_FILES ${CMAKE_SOURCE_DIR}/demo/demo.cpp)

IF(WIN32)
    # Include an RC file and HiDPI manifest for Windows (provided by wxWidgets)
    SET(wxUSE_DPI_AWARE_MANIFEST 1)
    LIST(APPEND DEMO_SRC_FILES ${wxWidgets_ROOT_DIR}/include/wx/msw/wx.rc)
    LIST(APPEND DEMO_SRC_FILES ${wxWidgets_ROOT_DIR}/include/wx/msw/wx_dpi_aware.manifest)
ELSEIF(APPLE)
    # Add an icon for the apple .app file
    LIST(APPEND DEMO_SRC_FILES ${CMAKE_SOURCE_DIR}/demo/wxmac.icns)
ENDIF()

# Define the build target for the executable and copy it into the "demo" subfolder
MESSAGE(STATUS "Building the demonstration program...")
SET(DEMO_APP WisteriaDemo)
ADD_EXECUTABLE(${DEMO_APP} WIN32 MACOSX_BUNDLE ${DEMO_SRC_FILES})
SET_TARGET_PROPERTIES(${DEMO_APP} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demo")

# Link required libraries to the executable
IF(WIN32)
    TARGET_LINK_LIBRARIES(${DEMO_APP} WisteriaLib ${wxWidgets_LIBRARIES})
ELSEIF(UNIX)
    FIND_LIBRARY(TBB_LIB tbb)
    TARGET_LINK_LIBRARIES(${DEMO_APP} WisteriaLib ${wxWidgets_LIBRARIES} ${TBB_LIB})
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
    SET_TARGET_PROPERTIES(${DEMO_APP} PROPERTIES
        RESOURCE "demo/wxmac.icns"
        MACOSX_BUNDLE_ICON_FILE wxmac.icns
        MACOSX_BUNDLE_COPYRIGHT "Copyright Blake Madden"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.wisteria.demo")
ENDIF()

# Build the app (not currently available)
########################
IF(EXISTS "${CMAKE_SOURCE_DIR}/app" AND EXISTS "${CMAKE_SOURCE_DIR}/app/wisteriaapp.cpp")
    SET(APP_SRC_FILES ${CMAKE_SOURCE_DIR}/app/wisteriaapp.cpp
        ${CMAKE_SOURCE_DIR}/app/wisteriaview.cpp
        ${CMAKE_SOURCE_DIR}/app/wisteriadoc.cpp)

    IF(WIN32)
        # Include an RC file and HiDPI manifest for Windows (provided by wxWidgets)
        SET(wxUSE_DPI_AWARE_MANIFEST 1)
        LIST(APPEND APP_SRC_FILES ${wxWidgets_ROOT_DIR}/include/wx/msw/wx.rc)
        LIST(APPEND APP_SRC_FILES ${wxWidgets_ROOT_DIR}/include/wx/msw/wx_dpi_aware.manifest)
    ELSEIF(APPLE)
        # Add an icon for the apple .app file
        LIST(APPEND APP_SRC_FILES ${CMAKE_SOURCE_DIR}/app/wxmac.icns)
    ENDIF()

    # Define the build target for the executable and copy it into the "app" subfolder
    MESSAGE(STATUS "Building the program...")
    SET(WISTERIA_APP WisteriaDV)
    ADD_EXECUTABLE(${WISTERIA_APP} WIN32 MACOSX_BUNDLE ${APP_SRC_FILES})
    SET_TARGET_PROPERTIES(${WISTERIA_APP} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/app")

    # Link required libraries to the executable
    IF(WIN32)
        TARGET_LINK_LIBRARIES(${WISTERIA_APP} WisteriaLib ${wxWidgets_LIBRARIES})
    ELSEIF(UNIX)
        FIND_LIBRARY(TBB_LIB tbb)
        TARGET_LINK_LIBRARIES(${WISTERIA_APP} WisteriaLib ${wxWidgets_LIBRARIES} ${TBB_LIB})
    ENDIF()

    # Copy resources and datasets into build folder
    MESSAGE(STATUS "Copying application resources...")
    ADD_CUSTOM_COMMAND(TARGET ${WISTERIA_APP}
                       POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/app/res $<TARGET_FILE_DIR:${WISTERIA_APP}>/res)
    ADD_CUSTOM_COMMAND(TARGET ${WISTERIA_APP}
                       POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/datasets $<TARGET_FILE_DIR:${WISTERIA_APP}>/datasets)

    IF(APPLE)
        SET_TARGET_PROPERTIES(${WISTERIA_APP} PROPERTIES
            RESOURCE "app/wxmac.icns"
            MACOSX_BUNDLE_ICON_FILE wxmac.icns
            MACOSX_BUNDLE_COPYRIGHT "Copyright Blake Madden"
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.wisteria.app")
    ENDIF()

ENDIF()
