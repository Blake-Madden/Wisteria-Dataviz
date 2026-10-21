# =============================================================================
#  PrintCompilerFlags.cmake
#
#  Purpose:
#    Extracts and prints a human-readable summary of the *actual* compiler
#    flags applied to a target.
#
#  Features:
#    • Identifies and classifies real compiler flags (warnings, optimizations,
#      security flags, MSVC extensions, etc.)
#    • Prints a clean, annotated list of the meaningful flags used to build
#      the target.
#    • Produces an exportable multi-line string (COMPILE_FLAGS_SUMMARY_STR) for
#      embedding in the application (e.g., About dialogs or diagnostic output).
#
#  Notes:
#    • Only *concrete* flags are summarized.
#    • Safe to call after all target_compile_options() and related settings.
#
# Exports to parent scope:
#   COMPILE_FLAGS_SUMMARY     (pretty, multi-line text to print in build script)
#   COMPILE_FLAGS_SUMMARY_STR (pretty, multi-line text to export to code)
#
# =============================================================================

set(COMPILE_FLAGS_SUMMARY "")
set(COMPILE_FLAGS_SUMMARY_STR "")

# ======================================================
# Extracts the main flag from a compile option
# ======================================================
function(extract_flag token out_var)
    # Skip empty or punctuation tokens
    if("${token}" STREQUAL "" OR "${token}" STREQUAL ">" OR "${token}" STREQUAL "<")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    # Normal flags (not generator expressions)
    if(NOT "${token}" MATCHES "^\\$<")
        set(${out_var} "${token}" PARENT_SCOPE)
        return()
    endif()

    # If GE but NOT involving CXX_COMPILER_ID, skip
    if(NOT "${token}" MATCHES "CXX_COMPILER_ID")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    # GE containing CXX_COMPILER_ID, extract payload
    string(REGEX REPLACE "^.*:([^>]+)>$" "\\1" payload "${token}")

    # Split payload and take first flag
    separate_arguments(tokens NATIVE_COMMAND "${payload}")
    list(GET tokens 0 first)

    set(${out_var} "${first}" PARENT_SCOPE)
endfunction()

# ======================================================
# Pretty-prints meaningful flag descriptions
# ======================================================
function(print_compile_flags_summary TARGET_NAME)
    get_target_property(_flags ${TARGET_NAME} COMPILE_OPTIONS)

    if(NOT _flags)
        message(WARNING "No compile options found for target ${TARGET_NAME}")
        set(COMPILE_FLAGS_SUMMARY "" PARENT_SCOPE)
        return()
    endif()

    message(STATUS "=== Compile Flag Summary ===")

    set(_summary_list "")

    set(_skip_config_ge FALSE)

    foreach(flag IN LISTS _flags)
        # Detect CONFIG-generator-expression START
        if("${flag}" MATCHES "^\\$<[^>]*CONFIG:")
            set(_skip_config_ge TRUE)
            continue()
        endif()

        # If we are skipping a CONFIG GE, skip until ">"
        if(_skip_config_ge)
            if("${flag}" STREQUAL ">")
                set(_skip_config_ge FALSE)
            endif()
            continue()  # skip -Og, -g3, :, >, etc.
        endif()

        # Handle normal flags OR CXX compiler-ID GEs
        extract_flag("${flag}" clean_flag)

        if("${clean_flag}" STREQUAL "")
            continue()
        endif()

        set(line "")

        # -------- Flag classification --------
        if(clean_flag STREQUAL "-Wall" OR clean_flag STREQUAL "/W3")
            set(line "• ${clean_flag}: Enable common warnings")

        elseif(clean_flag STREQUAL "-Wextra" OR clean_flag STREQUAL "/W4")
            set(line "• ${clean_flag}: Enable extra warnings")

        elseif(clean_flag STREQUAL "-Wpedantic")
            set(line "• ${clean_flag}: Enforce strict ISO compliance")

        elseif(clean_flag STREQUAL "-Wshadow")
            set(line "• ${clean_flag}: Warn about variable shadowing")

        elseif(clean_flag STREQUAL "-Werror" OR clean_flag STREQUAL "/WX")
            set(line "• ${clean_flag}: Treat warnings as errors")

        elseif(clean_flag STREQUAL "-fstack-protector-strong")
            set(line "• ${clean_flag}: Enable strong stack protection")

        elseif(clean_flag MATCHES "-D_FORTIFY_SOURCE=2")
            set(line "• ${clean_flag}: Enable fortified libc functions")

        elseif(clean_flag STREQUAL "/sdl")
            set(line "• ${clean_flag}: Enable SDL security checks") # MSVC

        elseif(clean_flag STREQUAL "/permissive-")
            set(line "• ${clean_flag}: Enforce strict ISO C++") # MSVC

        # intentionally ignored flags
        elseif(clean_flag STREQUAL "/MP" OR clean_flag STREQUAL "/Zc:__cplusplus" OR
               clean_flag STREQUAL "/utf-8")
            continue()

        elseif(clean_flag STREQUAL "/wd6211")
            message(STATUS "• ${clean_flag}: Suppress MSVC code analysis warning C6211 (false-positive memory leak detection)")

        elseif(clean_flag STREQUAL "-Og" OR clean_flag STREQUAL "/Od")
            set(line "• ${clean_flag}: Optimize for debugging")

        elseif(clean_flag STREQUAL "-O2" OR clean_flag STREQUAL "/O2")
            set(line "• ${clean_flag}: Optimize for speed")

        elseif(clean_flag STREQUAL "-g3")
            set(line "• ${clean_flag}: Full debug info")

        else()
            set(line "• ${clean_flag}: [Unrecognized or custom flag]")
        endif()

        if(line)
            message(STATUS "${line}")
            list(APPEND _summary_list "${line}")
        endif()

    endforeach()

    # Convert list to pretty multi-line string
    string(REPLACE ";" "\n" COMPILE_FLAGS_SUMMARY "${_summary_list}")
    # Make a human-readable list for code (e.g., About box) also
    string(REPLACE "\n" "\\n" COMPILE_FLAGS_SUMMARY_STR "${COMPILE_FLAGS_SUMMARY}")
    string(REPLACE "\"" "\\\"" COMPILE_FLAGS_SUMMARY_STR "${COMPILE_FLAGS_SUMMARY_STR}")

    # Export to parent scope
    set(COMPILE_FLAGS_SUMMARY "${COMPILE_FLAGS_SUMMARY}" PARENT_SCOPE)
    set(COMPILE_FLAGS_SUMMARY_STR "${COMPILE_FLAGS_SUMMARY_STR}" PARENT_SCOPE)
endfunction()
