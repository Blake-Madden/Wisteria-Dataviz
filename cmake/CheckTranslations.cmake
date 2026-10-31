#############################################################################
# Name:        CheckTranslations.cmake
# Purpose:     Reviews a project's gettext translation catalogs
# Author:      Blake Madden
# Created:     2025-10-26
# Copyright:   (c) 2025 Blake Madden
# License:     Eclipse Public License 2.0
#############################################################################

# Requires: CMake's FindGettext module (built-in)
# Will set GETTEXT_MSGFMT_EXECUTABLE if found.
find_package(Gettext QUIET)

# ==========================================================================================
#  Function: check_all_translations
#  ------------------------------------------------------------------------------------------
#  Scans the project's `locale/` directory for all `.po` files and performs a translation
#  check on each one using `check_translations()`.  Generates a Markdown-style summary and
#  also writes all translation stats to `${CMAKE_BINARY_DIR}/translation_stats.cmake` so
#  they can be included by downstream scripts or custom dashboard generators.
# ==========================================================================================
function(check_all_translations)
    set(_LOCALE_DIR "${CMAKE_SOURCE_DIR}/locale")
    file(GLOB _PO_FILES "${_LOCALE_DIR}/*.po")

    if(_PO_FILES STREQUAL "")
        message(WARNING "No .po files found in ${_LOCALE_DIR}")
        return()
    endif()

    set(_TRANSLATION_SUMMARY "")
    set(_TRANSLATION_EXPORT_FILE "${CMAKE_BINARY_DIR}/translation_stats.cmake")

    # Start fresh each configure
    file(WRITE "${_TRANSLATION_EXPORT_FILE}" "# Auto-generated translation stats\n")

    foreach(_PO_FILE IN LISTS _PO_FILES)
        get_filename_component(_LANG "${_PO_FILE}" NAME_WE)

        message(STATUS "Checking translation: ${_LANG}")
        check_translations(${_LANG})

        if(DEFINED ${_LANG}_TRANSLATION_STATS)
            set(_STAT "${${_LANG}_TRANSLATION_STATS}")
            list(APPEND _TRANSLATION_SUMMARY "${_LANG}: ${_STAT}")

            # Export each stat and percent to the .cmake file
            file(APPEND "${_TRANSLATION_EXPORT_FILE}"
                "set(${_LANG}_TRANSLATION_STATS [==[${_STAT}]==])\n")
        endif()

        if(DEFINED ${_LANG}_PERCENT_TRANSLATED)
            file(APPEND "${_TRANSLATION_EXPORT_FILE}"
                "set(${_LANG}_PERCENT_TRANSLATED ${${_LANG}_PERCENT_TRANSLATED})\n")
        endif()
    endforeach()

    message(STATUS "=== Translation Summary ===")
    foreach(_ENTRY IN LISTS _TRANSLATION_SUMMARY)
        message(STATUS "${_ENTRY}")
    endforeach()

    message(STATUS "Wrote translation stats to: ${_TRANSLATION_EXPORT_FILE}")
endfunction()

# ==========================================================================================
#  Function: check_translations
#  ------------------------------------------------------------------------------------------
#  Validates the translation (.po) file for a specific language by invoking `msgfmt` with
#  the `--statistics` flag and parsing its output.
#
#  Behavior:
#    • Detects and reports the number of translated, fuzzy, and untranslated messages.
#    • Displays any warnings or errors emitted by `msgfmt`.
#    • Computes the percentage of completed translations.
#    • Emits a CMake WARNING if untranslated or fuzzy entries are found.
#    • Exports the results to a CMake variable named <LANG>_TRANSLATION_STATS.
#
#  Typical usage:
#      check_translations(fr)
#      message(STATUS "French stats: ${fr_TRANSLATION_STATS}")
#
#  Notes:
#    • Runs during the CMake *configure* phase (not at build time).
#    • Automatically searches for `msgfmt` (including the Poedit installation path on Windows).
# ==========================================================================================
function(check_translations LANG)
    if(NOT GETTEXT_MSGFMT_EXECUTABLE)
        set(_programfiles "$ENV{ProgramFiles}")
        set(_poedit_msgfmt "${_programfiles}/Poedit/GettextTools/bin/msgfmt.exe")

        if(EXISTS "${_poedit_msgfmt}")
            set(GETTEXT_MSGFMT_EXECUTABLE "${_poedit_msgfmt}" CACHE FILEPATH "Path to msgfmt from Poedit" FORCE)
            set(GETTEXT_FOUND TRUE CACHE BOOL "Gettext found through Poedit" FORCE)
            message(STATUS "Found msgfmt.exe via Poedit: ${_poedit_msgfmt}")
        else()
            message(STATUS "gettext(msgfmt) not found. Skipping translation check for ${LANG}.")
            return()
        endif()
    endif()

    set(_MSGFMT "${GETTEXT_MSGFMT_EXECUTABLE}")
    set(_SRC_PO "${CMAKE_SOURCE_DIR}/locale/${LANG}.po")

    if(NOT EXISTS "${_SRC_PO}")
        message(WARNING "[${LANG}] Missing .po file: ${_SRC_PO}")
        return()
    endif()

    # Choose null output device based on platform
    if(WIN32)
        set(_NULL_DEVICE "NUL")
    else()
        set(_NULL_DEVICE "/dev/null")
    endif()

    # Run msgfmt to check and get statistics
    execute_process(
        COMMAND "${_MSGFMT}" --statistics -o ${_NULL_DEVICE} "${_SRC_PO}"
        RESULT_VARIABLE _MSGFMT_RESULT
        OUTPUT_VARIABLE _MSGFMT_STDOUT
        ERROR_VARIABLE  _MSGFMT_STDERR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE)

    if(_MSGFMT_RESULT EQUAL 0)
        message(STATUS "[${LANG}] msgfmt succeeded.")
    else()
        message(WARNING "[${LANG}] msgfmt failed with code ${_MSGFMT_RESULT}")
    endif()

    if(_MSGFMT_STDOUT)
        message(STATUS "[${LANG}] msgfmt stdout: ${_MSGFMT_STDOUT}")
    endif()

    if(_MSGFMT_STDERR)
        message(WARNING "[${LANG}] msgfmt stderr: ${_MSGFMT_STDERR}")
    endif()

    # Extract translation statistics
    set(_MSGFMT_OUTPUT "${_MSGFMT_STDOUT}\n${_MSGFMT_STDERR}")

    string(REGEX MATCH "([0-9]+) translated messages" _MATCHED "${_MSGFMT_OUTPUT}")
    set(_NUM_TRANSLATED "${CMAKE_MATCH_1}")

    string(REGEX MATCH "([0-9]+) fuzzy translations" _MATCHED "${_MSGFMT_OUTPUT}")
    set(_NUM_FUZZY "${CMAKE_MATCH_1}")

    string(REGEX MATCH "([0-9]+) untranslated messages" _MATCHED "${_MSGFMT_OUTPUT}")
    set(_NUM_UNTRANSLATED "${CMAKE_MATCH_1}")

    # ---- Default to 0 if missing ----
    foreach(_var _NUM_TRANSLATED _NUM_FUZZY _NUM_UNTRANSLATED)
        if(NOT ${_var})
            set(${_var} 0)
        endif()
    endforeach()

    # ---- Emit warnings ----
    if(_NUM_UNTRANSLATED GREATER 0)
        message(WARNING "[${LANG}] ${_NUM_UNTRANSLATED} untranslated messages found.")
    endif()

    if(_NUM_FUZZY GREATER 0)
        message(WARNING "[${LANG}] ${_NUM_FUZZY} fuzzy messages found (need review).")
    endif()

    # ---- Compute translated percentage ----
    math(EXPR _TOTAL_MESSAGES "${_NUM_TRANSLATED} + ${_NUM_FUZZY} + ${_NUM_UNTRANSLATED}")
    if(_TOTAL_MESSAGES GREATER 0)
        math(EXPR _PERCENT_TRANSLATED "(${_NUM_TRANSLATED} * 100) / ${_TOTAL_MESSAGES}")
    else()
        set(_PERCENT_TRANSLATED 100)
    endif()

    # ---- Export stats ----
    set(${LANG}_TRANSLATION_STATS
        "${_PERCENT_TRANSLATED}% translated (${_NUM_TRANSLATED} OK, ${_NUM_FUZZY} fuzzy, ${_NUM_UNTRANSLATED} missing)"
        CACHE INTERNAL "Translation stats for ${LANG}" FORCE)
    set(${LANG}_PERCENT_TRANSLATED "${_PERCENT_TRANSLATED}" CACHE INTERNAL "Translation percentage for ${LANG}" FORCE)

    message(STATUS "[${LANG}] Translation stats: ${${LANG}_TRANSLATION_STATS}")
endfunction()
