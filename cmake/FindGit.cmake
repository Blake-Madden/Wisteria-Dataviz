# FindGit.cmake
#
# Wrapper around CMake's built-in FindGit module.
#   1. Normal search (GIT_EXECUTABLE, PATH, standard install locations).
#   2. Windows fallback: the Git bundled with Visual Studio (2017+).
#
# If this file lives on CMAKE_MODULE_PATH, the find_package(Git) calls below
# load CMake's own FindGit module rather than recursing into this file.

find_package(Git QUIET)

if(NOT Git_FOUND AND CMAKE_HOST_WIN32)
  set(_VS_GIT_PATH "")

  # Visual Studio installs Git under the IDE folder of each instance.
  set(_VS_GIT_REL "Common7/IDE/CommonExtensions/Microsoft/TeamFoundation/Team Explorer/Git/cmd/git.exe")

  # 1. Ask vswhere (always installed with VS 2017+ and Build Tools)
  set(_VSWHERE_HINTS)
  foreach(_pf "ProgramFiles(x86)" "ProgramFiles" "ProgramW6432")
    if(DEFINED ENV{${_pf}})
      file(TO_CMAKE_PATH "$ENV{${_pf}}" _pf_dir)
      list(APPEND _VSWHERE_HINTS "${_pf_dir}/Microsoft Visual Studio/Installer")
    endif()
  endforeach()

  find_program(_VSWHERE_EXECUTABLE NAMES vswhere HINTS ${_VSWHERE_HINTS})

  if(_VSWHERE_EXECUTABLE)
    execute_process(
      COMMAND "${_VSWHERE_EXECUTABLE}"
              -latest
              -products *
              -requires Microsoft.VisualStudio.Component.Git
              -find "**\\Git\\cmd\\git.exe"
      OUTPUT_VARIABLE _VS_GIT_OUT
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
    # vswhere may print several matches; keep the first.
    string(REGEX MATCH "^[^\r\n]+" _VS_GIT_FIRST "${_VS_GIT_OUT}")
    if(_VS_GIT_FIRST)
      file(TO_CMAKE_PATH "${_VS_GIT_FIRST}" _VS_GIT_PATH)
    endif()
  endif()
  unset(_VSWHERE_EXECUTABLE CACHE)

  # 2. Direct check of the active VS instance (no vswhere needed)
  if(NOT _VS_GIT_PATH OR NOT EXISTS "${_VS_GIT_PATH}")
    set(_VS_ROOTS)
    # set in a Developer Command Prompt
    if(DEFINED ENV{VSINSTALLDIR})
      list(APPEND _VS_ROOTS "$ENV{VSINSTALLDIR}")
    endif()
    # set by the Visual Studio generators
    if(CMAKE_GENERATOR_INSTANCE)
      list(APPEND _VS_ROOTS "${CMAKE_GENERATOR_INSTANCE}")
    endif()
    foreach(_root IN LISTS _VS_ROOTS)
      file(TO_CMAKE_PATH "${_root}" _root)
      if(EXISTS "${_root}/${_VS_GIT_REL}")
        set(_VS_GIT_PATH "${_root}/${_VS_GIT_REL}")
        break()
      endif()
    endforeach()
  endif()

  # 3. Hand the result to the standard module
  if(_VS_GIT_PATH AND EXISTS "${_VS_GIT_PATH}")
    message(STATUS "Git not found in PATH; using Visual Studio's Git: ${_VS_GIT_PATH}")
    # FORCE replaces the GIT_EXECUTABLE-NOTFOUND left by the first search.
    set(GIT_EXECUTABLE "${_VS_GIT_PATH}" CACHE FILEPATH "Git from Visual Studio" FORCE)
    mark_as_advanced(GIT_EXECUTABLE)
    find_package(Git QUIET) # re-run so GIT_VERSION_STRING / Git::Git are set
  endif()

  unset(_VS_GIT_PATH)
  unset(_VS_GIT_OUT)
  unset(_VS_GIT_FIRST)
  unset(_VS_GIT_REL)
  unset(_VS_ROOTS)
  unset(_VSWHERE_HINTS)
endif()
