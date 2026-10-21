# Sanity-check: if ASan was requested, verify the toolchain can link it
if(ENABLE_SANITIZERS)
  include(CheckCXXSourceCompiles)

  # Stash current try-compile flags
  set(_SAVE_REQ_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(_SAVE_REQ_LINK  "${CMAKE_REQUIRED_LINK_OPTIONS}")

  # Add ASan compile+link flags for the probe
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
  list(APPEND CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")

  # This compiles AND links a tiny program (doesn't run it)
  check_cxx_source_compiles("
    #include <stdlib.h>
    int main() {
      char* p = (char*)malloc(1);
      p[1] = 0; // force an out-of-bounds write (just to touch code)
      return 0;
    }" ASAN_LINKS)

  # Restore flags
  set(CMAKE_REQUIRED_FLAGS "${_SAVE_REQ_FLAGS}")
  set(CMAKE_REQUIRED_LINK_OPTIONS "${_SAVE_REQ_LINK}")

  if(NOT ASAN_LINKS)
    if(APPLE)
      message(FATAL_ERROR
        "AddressSanitizer requested but a probe failed to link.\n"
        "Hints: Use Apple Clang/Xcode (xcode-select --install). Some custom LLVM builds miss compiler-rt.")
    else()
      message(FATAL_ERROR
        "AddressSanitizer requested but a probe failed to link (missing libasan / compiler-rt?).\n"
        "Hints:\n"
        "  • GCC toolchains: install the matching libasan package (e.g. sudo apt install libasan8 on Ubuntu 22.04).\n"
        "  • Clang toolchains: install compiler-rt runtimes (e.g. sudo apt install libclang-rt-<ver>-dev).")
    endif()
  endif()
endif()
