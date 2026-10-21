# Sanitizer setup + recommended runtime options.
# Toggle with: -DENABLE_SANITIZERS=ON
#
# Exports to parent scope:
#   SANITIZER_FLAGS
#   SANITIZER_EXTRA_FLAGS
#   SANITIZER_DEFS
#   ENABLED_SANITIZERS_SUMMARY  (pretty, bullet-point string for CMake output)
#   ENABLED_SANITIZERS_STR      (pretty, bullet-point string for code)
#   SANITIZER_ENV_VARS          (list of "NAME=VALUE" env assignments)
#   SANITIZER_ENV_HINTS_SUMMARY (pretty, multi-line help text for CMake output)
#   SANITIZER_ENV_HINTS_STR     (pretty, multi-line help text for code)

option(ENABLE_SANITIZERS "Enable sanitizers on supported compilers (applied to Debug builds via generator expressions)" OFF)

set(SANITIZER_FLAGS "")
set(SANITIZER_EXTRA_FLAGS "")
set(SANITIZER_LINK_FLAGS "")
set(SANITIZER_DEFS "")
set(SANITIZER_LIST "")
set(ENABLED_SANITIZERS_SUMMARY "")
set(ENABLED_SANITIZERS_STR "")

# We keep our default combo: ASan+UBSan on Clang/GCC; ASan on MSVC.
if(ENABLE_SANITIZERS)
    if(MSVC)
        # MSVC supports AddressSanitizer (/fsanitize=address).
        set(SANITIZER_FLAGS "/fsanitize=address")
        # These help when linked libs (e.g., wxWidgets) aren't ASan-instrumented
        # https://learn.microsoft.com/en-us/cpp/sanitizers/error-container-overflow?view=msvc-170
        list(APPEND SANITIZER_DEFS _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
        list(APPEND SANITIZER_LIST "ASAN (AddressSanitizer: detects memory errors such as use-after-free, buffer overflows)")

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang$" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(SANITIZER_FLAGS "-fsanitize=address,undefined;-fno-omit-frame-pointer")
        set(SANITIZER_LINK_FLAGS "-fsanitize=address,undefined")
        list(APPEND SANITIZER_LIST
            "ASAN (AddressSanitizer: detects memory errors such as use-after-free, buffer overflows)"
            "UBSAN (UndefinedBehaviorSanitizer: detects integer overflow, invalid casts, and other UB)")
        if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
            # AppleClang: extra ASan goodness
            set(SANITIZER_EXTRA_FLAGS "-fsanitize-address-use-after-scope")
            list(APPEND SANITIZER_LIST
                "EXTRA: -fsanitize-address-use-after-scope (detects use-after-scope of stack variables [AppleClang only])")
        endif()
        # NOTE: TSan conflicts with ASan. If you want TSan runs, swap flags to -fsanitize=thread in a different config.
    endif()

    # Pretty list (bulleted)
    string(REPLACE ";" "\n  • " ENABLED_SANITIZERS_SUMMARY "${SANITIZER_LIST}")
    set(ENABLED_SANITIZERS_SUMMARY "  • ${ENABLED_SANITIZERS_SUMMARY}")

    # ------------------------------
    # Recommended runtime env values
    # ------------------------------
    set(SANITIZER_ENV_VARS "")
    set(_env_hints "")
    set(_newline "\n")

    # ASAN / UBSAN recommendations (Clang/GCC)
    if(NOT MSVC)
        # ASAN_OPTIONS
        set(_asan_opts "halt_on_error=1:abort_on_error=1:symbolize=1")
        # LeakSanitizer is generally available on Linux; macOS support is historically limited.
        if(APPLE)
            # Avoid suggesting detect_leaks on Apple (not fully supported)
            list(APPEND _env_hints
                "ASAN_OPTIONS (suggested): ${_asan_opts}"
                "Note: LeakSanitizer is limited on Apple platforms ('detect_leaks' may not be supported).")
        else()
            set(_asan_opts "${_asan_opts}:detect_leaks=1")
            list(APPEND _env_hints "ASAN_OPTIONS (suggested): ${_asan_opts}")
            # LSAN_OPTIONS
            set(_lsan_opts "suppressions=<path/to/lsan.supp>:print_suppressions=0")
            list(APPEND _env_hints "LSAN_OPTIONS (optional): ${_lsan_opts}")
            list(APPEND SANITIZER_ENV_VARS "LSAN_OPTIONS=${_lsan_opts}")
        endif()
        list(APPEND SANITIZER_ENV_VARS "ASAN_OPTIONS=${_asan_opts}")

        # UBSAN_OPTIONS
        set(_ubsan_opts "print_stacktrace=1:report_error_type=1:halt_on_error=1")
        list(APPEND _env_hints "UBSAN_OPTIONS (suggested): ${_ubsan_opts}")
        list(APPEND SANITIZER_ENV_VARS "UBSAN_OPTIONS=${_ubsan_opts}")

        # TSan guidance (not enabled by default here)
        set(_tsan_opts "halt_on_error=1:report_signal_unsafe=1:history_size=7")
        list(APPEND _env_hints "TSAN_OPTIONS (if using -fsanitize=thread instead): ${_tsan_opts}")
        # we don't append TSAN_OPTIONS to SANITIZER_ENV_VARS because TSan isn't active in this config
    else()
        # MSVC: ASan runtime options differ; ASAN_OPTIONS/UBSAN_OPTIONS are not used by MSVC's ASan.
        list(APPEND _env_hints
            "MSVC AddressSanitizer notes:"
            "The MSVC ASan runtime does not use ASAN_OPTIONS/UBSAN_OPTIONS env vars."
            "Leak detection is not available with MSVC ASan."
            "Keep PDBs and /Zi for better stack traces. Consider /DEBUG:FULL for richer info.")
    endif()

    # Build human-readable help block
    set(SANITIZER_ENV_HINTS_SUMMARY "")
    if(_env_hints)
        # Pretty bullets
        string(REPLACE ";" "\n  • " SANITIZER_ENV_HINTS_SUMMARY "${_env_hints}")
        set(SANITIZER_ENV_HINTS_SUMMARY "  ${SANITIZER_ENV_HINTS_SUMMARY}")
    endif()

    # Verbose summary
    message(STATUS "================== Sanitizer Configuration ==================")
    message(STATUS "ENABLE_SANITIZERS           : ${ENABLE_SANITIZERS}")
    if(CMAKE_CONFIGURATION_TYPES)
        message(STATUS "Generator                   : Multi-config (${CMAKE_GENERATOR})")
        message(STATUS "Applied to                  : Debug configuration via generator expressions")
    else()
        message(STATUS "Generator                   : Single-config (${CMAKE_GENERATOR})")
        message(STATUS "CMAKE_BUILD_TYPE            : ${CMAKE_BUILD_TYPE}")
        message(STATUS "Applied to                  : Only if build type is Debug")
    endif()
    message(STATUS "Compiler                    : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "Platform                    : ${CMAKE_SYSTEM_NAME}")
    if(SANITIZER_LIST)
        message(STATUS "Enabled Sanitizers and Features:")
        message("${ENABLED_SANITIZERS_SUMMARY}")
    else()
        message(STATUS "Enabled Sanitizers and Features: (none)")
    endif()
    if(SANITIZER_FLAGS)
        message(STATUS "Base compiler/link flags     : ${SANITIZER_FLAGS}")
    endif()
    if(SANITIZER_EXTRA_FLAGS)
        message(STATUS "Extra sanitizer flags        : ${SANITIZER_EXTRA_FLAGS}")
    endif()
    if(SANITIZER_DEFS)
        message(STATUS "Added preprocessor defines   : ${SANITIZER_DEFS}")
    endif()

    # Show recommended env usage
    message(STATUS "---------------- Recommended Runtime Environment ------------")
    if(SANITIZER_ENV_HINTS_SUMMARY)
        message(STATUS "Suggested variables:")
        message("${SANITIZER_ENV_HINTS_SUMMARY}")
    else()
        message(STATUS "(No runtime environment variables recommended for this toolchain.)")
    endif()

    if(NOT MSVC)
        message(STATUS "Examples:")
        message(STATUS "  • POSIX shell:   ASAN_OPTIONS='...' UBSAN_OPTIONS='...' ctest -C Debug")
        message(STATUS "  • One-off run:   ASAN_OPTIONS='...' UBSAN_OPTIONS='...' ./bin/your_test_binary")
        message(STATUS "  • CTest property: set_tests_properties(<name> PROPERTIES ENVIRONMENT \"ASAN_OPTIONS=...;UBSAN_OPTIONS=...\")")
    else()
        message(STATUS "Notes for Windows/MSVC:")
        message(STATUS "  • MSVC ASan ignores ASAN_OPTIONS/UBSAN_OPTIONS.")
        message(STATUS "  • Just run tests normally (CTest, IDE, or exe). Keep debug info (/Zi) for better stacks.")
    endif()
    message(STATUS "============================================================")

else()
    message(STATUS "Sanitizers disabled (ENABLE_SANITIZERS=OFF)")
endif()

set(SANITIZER_FLAGS "${SANITIZER_FLAGS}")
set(SANITIZER_EXTRA_FLAGS "${SANITIZER_EXTRA_FLAGS}")
set(SANITIZER_LINK_FLAGS "${SANITIZER_LINK_FLAGS}")
set(SANITIZER_DEFS "${SANITIZER_DEFS}")
set(ENABLED_SANITIZERS_SUMMARY "${ENABLED_SANITIZERS_SUMMARY}")

string(REPLACE "\n" "\\n" ENABLED_SANITIZERS_STR "${ENABLED_SANITIZERS_SUMMARY}")
string(REPLACE "\"" "\\\"" ENABLED_SANITIZERS_STR "${ENABLED_SANITIZERS_STR}")
set(ENABLED_SANITIZERS_STR "${ENABLED_SANITIZERS_STR}")

set(SANITIZER_ENV_VARS "${SANITIZER_ENV_VARS}")
set(SANITIZER_ENV_HINTS_SUMMARY "${SANITIZER_ENV_HINTS_SUMMARY}")

string(REPLACE "\n" "\\n" SANITIZER_ENV_HINTS_STR "${SANITIZER_ENV_HINTS_SUMMARY}")
string(REPLACE "\"" "\\\"" SANITIZER_ENV_HINTS_STR "${SANITIZER_ENV_HINTS_STR}")
set(SANITIZER_ENV_HINTS_STR "${SANITIZER_ENV_HINTS_STR}")
