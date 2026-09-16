# --- Look up Visual Studio's bundled Git via vswhere (VS 2017+)

# Build a list of candidate vswhere locations without touching ProgramFiles(x86)
set(_VSWHERE_HINTS)

if(DEFINED ENV{ProgramW6432})
  list(APPEND _VSWHERE_HINTS "$ENV{ProgramW6432}/Microsoft Visual Studio/Installer")
endif()

if(DEFINED ENV{ProgramFiles})
  list(APPEND _VSWHERE_HINTS "$ENV{ProgramFiles}/Microsoft Visual Studio/Installer")
endif()

# Also allow PATH/CMAKE_PROGRAM_PATH to help if vswhere is installed elsewhere
find_program(VSWHERE_EXECUTABLE NAMES vswhere
  HINTS ${_VSWHERE_HINTS}
  NO_CACHE
)

# Ask VSWhere for the Git bundled with Visual Studio
if(VSWHERE_EXECUTABLE AND NOT DEFINED GIT_EXECUTABLE)
  execute_process(
    COMMAND "${VSWHERE_EXECUTABLE}"
            -latest
            -products *
            -requires Microsoft.VisualStudio.Component.Git
            -find **\\Git\\cmd\\git.exe
    OUTPUT_VARIABLE VS_GIT_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(VS_GIT_PATH AND EXISTS "${VS_GIT_PATH}")
    set(GIT_EXECUTABLE "${VS_GIT_PATH}" CACHE FILEPATH "Git from Visual Studio")
    mark_as_advanced(GIT_EXECUTABLE)
  endif()
endif()

# Fall back to normal PATH search (will use GIT_EXECUTABLE if we set it above)
find_package(Git QUIET)
