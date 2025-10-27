#############################################################################
# Name:        CMakeLists.txt
# Purpose:     Build script for Wisteria Dataviz library and demo project
# Author:      Blake Madden
# Created:     2022-01-17
# Copyright:   (c) 2005-2025 Blake Madden
# License:     3-Clause BSD license
#############################################################################

# Declare the minimum required CMake version
cmake_minimum_required(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)
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

project(Wisteria VERSION 1.0.4.0 DESCRIPTION "Wisteria-Dataviz")

set(FULL_VERSION ${Wisteria_VERSION_MAJOR}.${Wisteria_VERSION_MINOR}.${Wisteria_VERSION_PATCH}.${Wisteria_VERSION_TWEAK})
# Build a displayable version string that only includes the necessary parts
set(FULL_DISPLAY_VERSION ${Wisteria_VERSION_MAJOR})
if(NOT ${Wisteria_VERSION_MINOR} EQUAL 0 OR NOT ${Wisteria_VERSION_PATCH} EQUAL 0 OR NOT ${Wisteria_VERSION_TWEAK} EQUAL 0)
    string(APPEND FULL_DISPLAY_VERSION ".${Wisteria_VERSION_MINOR}")
endif()
if(NOT ${Wisteria_VERSION_PATCH} EQUAL 0 OR NOT ${Wisteria_VERSION_TWEAK} EQUAL 0)
    string(APPEND FULL_DISPLAY_VERSION ".${Wisteria_VERSION_PATCH}")
endif()
if(NOT ${Wisteria_VERSION_TWEAK} EQUAL 0)
    string(APPEND FULL_DISPLAY_VERSION ".${Wisteria_VERSION_TWEAK}")
endif()

string(TIMESTAMP BUILD_YEAR "%Y")
set(WISTERIA_COPYRIGHT "Copyright (c) 2005-${BUILD_YEAR} Blake Madden")

# Build the documentation
#########################
find_package(Doxygen)
if(NOT DOXYGEN_FOUND)
    message(STATUS "Doxygen not found. API documentation cannot be generated.")
else()
    set(DOXYGEN_ALL_OPTION_MAYBE_UNAVAILABLE ALL)
endif()

add_custom_target(doxygen-docs
            ${DOXYGEN_ALL_OPTION_MAYBE_UNAVAILABLE}
            COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_SOURCE_DIR}/docs/doxygen/Doxyfile"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/docs/doxygen"
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)

find_program(QUARTO_EXECUTABLE NAMES quarto quarto.exe)

if(QUARTO_EXECUTABLE MATCHES "QUARTO_EXECUTABLE-NOTFOUND")
    message(STATUS "Quarto not found. JSON syntax documentation cannot be generated.")
else()
    set(QUARTO_ALL_OPTION_MAYBE_UNAVAILABLE ALL)
endif()

add_custom_target(quarto-docs
            ${QUARTO_ALL_OPTION_MAYBE_UNAVAILABLE}
            COMMAND ${QUARTO_EXECUTABLE} render "${CMAKE_SOURCE_DIR}/docs/syntax-manual"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/docs/syntax-manual"
            COMMENT "Generating JSON syntax documentation with Quarto"
            VERBATIM)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    # Enable hardened runtime:
	# https://developer.apple.com/documentation/security/hardened_runtime
    # Note that this will need to be set via codesign later because we are not
    # codesigning via CMake. This here is more for bookkeeping.
    set(CMAKE_XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES)
endif()

if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    # If no deployment target has been set default to the minimum supported
    # OS version (this has to be set before the first project() call)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "iOS Deployment Target")
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "macOS Deployment Target")
    endif()
endif()

# Add wxWidgets to the build
include(cmake/folders.cmake)

set(PROJECT_WX_DEBUG_LEVEL 1)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(PROJECT_WX_DEBUG_LEVEL 0)
else()
    set(PROJECT_WX_DEBUG_LEVEL 1)
endif()

set(wxUSE_WEBVIEW_EDGE ON CACHE BOOL "Use Edge backend for WebView (MSW)" FORCE)
set(wxBUILD_SHARED OFF CACHE BOOL "Build wxWidgets statically" FORCE)
set(wxBUILD_DEBUG_LEVEL ${PROJECT_WX_DEBUG_LEVEL} CACHE STRING "wxWidgets runtime debug level" FORCE)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(wxBUILD_OPTIMISE ON CACHE BOOL "Optimize wxWidgets for Release" FORCE)
    set(wxBUILD_STRIPPED_RELEASE ON CACHE BOOL "Strip symbols from wxWidgets Release builds" FORCE)
endif()
find_directory("wxWidgets" wxWidgets_BASEDIR)
if(NOT EXISTS "${wxWidgets_BASEDIR}")
    message(FATAL_ERROR "wxWidgets base directory not found: ${wxWidgets_BASEDIR}")
endif()
add_subdirectory(${wxWidgets_BASEDIR} ${CMAKE_CURRENT_SOURCE_DIR}/wxWidgets_lib)
set(wxWidgets_LIBRARIES wx::core wx::base wx::xrc wx::propgrid wx::xml wx::aui wx::ribbon wx::qa wx::stc wx::net wx::richtext)
message(STATUS "Adding wxWidgets libraries: ${wxWidgets_LIBRARIES}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
               "${CMAKE_CURRENT_SOURCE_DIR}/src/base/version.h")

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# Build Wisteria library
########################
# (run "tools/build_cmake_files_list.R" to update this)
include(${CMAKE_SOURCE_DIR}/tools/libfiles.cmake)
set(WISTIER_LIB wisteria)
add_library(${WISTIER_LIB} STATIC ${WISTERIA_SRC})
if(WIN32 OR APPLE)
    target_link_libraries(${WISTIER_LIB} PUBLIC ${wxWidgets_LIBRARIES})
elseif(UNIX)
    # Add OpenMP for mutlithreading
    find_package(OpenMP)
    find_library(TBB_LIB tbb)
    target_link_libraries(${WISTIER_LIB} PUBLIC OpenMP::OpenMP_CXX ${TBB_LIB} ${wxWidgets_LIBRARIES})
endif()

# Define wxDEBUG_LEVEL consistently for all platforms
target_compile_definitions(${WISTIER_LIB} PUBLIC 
    wxDEBUG_LEVEL=${PROJECT_WX_DEBUG_LEVEL})

# Set definitions, warnings, and optimizations (will propagate to the demo project also)
if(MSVC)
    target_compile_definitions(${WISTIER_LIB} PUBLIC __WXMSW__ _CRT_SECURE_NO_WARNINGS _UNICODE
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
    target_compile_options(${WISTIER_LIB} PUBLIC /Zc:__cplusplus /MP /W3 /WX /wd6211 /openmp /permissive- /sdl
                           $<$<CONFIG:Debug>:/Od> $<$<CONFIG:Release>:/O2>)
elseif(MINGW OR MSYS)
    target_compile_definitions(${WISTIER_LIB} PUBLIC __WXMSW__ _CRT_SECURE_NO_WARNINGS _UNICODE
            $<$<CONFIG:Debug>:__WXDEBUG__> $<$<CONFIG:Release>:NDEBUG>)
    target_compile_options(${WISTIER_LIB} PUBLIC -Wall -Wextra -Wpedantic -Wshadow -Werror
                           $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fstack-protector-strong>
                           $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-D_FORTIFY_SOURCE=2>
                           $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Release>:-O2>)
elseif(APPLE)
    target_compile_definitions(${WISTIER_LIB} PUBLIC __WXOSX__
        $<$<CONFIG:Debug>:__WXDEBUG__> $<$<CONFIG:Release>:NDEBUG>)
    # TODO: add -Werror eventually.
    # Don't use -Werror for now because of warning from wxWidgets.
    target_compile_options(${WISTIER_LIB} PUBLIC -Wall -Wextra -Wpedantic -Wshadow
                           # Security hardening (only for GCC and Clang)
                           $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fstack-protector-strong>
                           $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-D_FORTIFY_SOURCE=2>
                           $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Release>:-O2>)
elseif(UNIX)
    target_compile_definitions(${WISTIER_LIB} PUBLIC __WXGTK__
        $<$<CONFIG:Debug>:__WXDEBUG__> $<$<CONFIG:Release>:NDEBUG>)
    target_compile_options(${WISTIER_LIB} PUBLIC -Wall -Wextra -Wpedantic -Wshadow -Werror
                           $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fstack-protector-strong>
                           $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-D_FORTIFY_SOURCE=2>
                           $<$<CONFIG:Debug>:-Og> $<$<CONFIG:Release>:-O2>)
    target_link_options(${WISTIER_LIB} PUBLIC
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wl,-z,relro,-z,now>)
endif()

if(DEFINED wxBUILD_DEBUG_LEVEL AND NOT "${wxBUILD_DEBUG_LEVEL}" STREQUAL "${PROJECT_WX_DEBUG_LEVEL}")
    message(FATAL_ERROR "Mismatch: wxBUILD_DEBUG_LEVEL=${wxBUILD_DEBUG_LEVEL} but your code uses wxDEBUG_LEVEL=${PROJECT_WX_DEBUG_LEVEL}")
endif()

# Build the library demo
########################
set(DEMO_SRC_FILES ${CMAKE_SOURCE_DIR}/demo/demo.cpp)

if(WIN32)
    # Include an RC file and HiDPI manifest for Windows (provided by wxWidgets)
    set(wxUSE_DPI_AWARE_MANIFEST 1)
    list(APPEND DEMO_SRC_FILES "${wxWidgets_BASEDIR}/include/wx/msw/wx.rc")
    list(APPEND DEMO_SRC_FILES "${wxWidgets_BASEDIR}/include/wx/msw/wx_dpi_aware.manifest")
elseif(APPLE)
    # Add an icon for the apple .app file
    list(APPEND DEMO_SRC_FILES ${CMAKE_SOURCE_DIR}/wisteria-dv.icns)
endif()

# Define the build target for the executable and copy it into the "demo" subfolder
message(STATUS "Configuring demonstration program...")
set(DEMO_APP demo)
add_executable(${DEMO_APP} WIN32 MACOSX_BUNDLE ${DEMO_SRC_FILES})
set_target_properties(${DEMO_APP} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demo")

# Link required libraries to the executable
target_link_libraries(${DEMO_APP} ${WISTIER_LIB} ${wxWidgets_LIBRARIES})

# Copy resources and datasets into build folder
message(STATUS "Copying demo resources...")
add_custom_command(TARGET ${DEMO_APP}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/demo/res $<TARGET_FILE_DIR:${DEMO_APP}>/res)
add_custom_command(TARGET ${DEMO_APP}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/datasets $<TARGET_FILE_DIR:${DEMO_APP}>/datasets)

if(APPLE)
    set_target_properties(${DEMO_APP} PROPERTIES
        RESOURCE wisteria-dv.icns
        MACOSX_BUNDLE_ICON_FILE wisteria-dv.icns
        MACOSX_BUNDLE_COPYRIGHT "Copyright Blake Madden"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.wisteria.demo")
endif()

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
        LIST(APPEND APP_SRC_FILES ${CMAKE_SOURCE_DIR}/wisteria-dv.icns)
    ENDIF()

    # Define the build target for the executable and copy it into the "app" subfolder
    MESSAGE(STATUS "Configuring the program...")
    SET(WISTERIA_APP WisteriaDV)
    ADD_EXECUTABLE(${WISTERIA_APP} WIN32 MACOSX_BUNDLE ${APP_SRC_FILES})
    SET_TARGET_PROPERTIES(${WISTERIA_APP} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/app")

    # Link required libraries to the executable
    IF(WIN32)
        TARGET_LINK_LIBRARIES(${WISTERIA_APP} ${WISTIER_LIB} ${wxWidgets_LIBRARIES})
    ELSEIF(UNIX)
        FIND_LIBRARY(TBB_LIB tbb)
        TARGET_LINK_LIBRARIES(${WISTERIA_APP} ${WISTIER_LIB} ${wxWidgets_LIBRARIES} ${TBB_LIB})
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
            RESOURCE wisteria-dv.icns
            MACOSX_BUNDLE_ICON_FILE wisteria-dv.icns
            MACOSX_BUNDLE_COPYRIGHT "Copyright Blake Madden"
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.wisteria.app")
    ENDIF()

ENDIF()
