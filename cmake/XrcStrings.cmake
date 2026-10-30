# Don't touch the file unless it is out of date or missing.
# CMake generators may break if build files get updated but their content didn't actually change.
function(update_file_if_needed FILE_MANIFEST_PATH FILE_CONTENT)
    if(NOT EXISTS "${FILE_MANIFEST_PATH}")
        message(STATUS "${FILE_MANIFEST_PATH}: creating list.")
        file(WRITE "${FILE_MANIFEST_PATH}" "${FILE_CONTENT}")
    else()
        file(READ "${FILE_MANIFEST_PATH}" ORIGINAL_FILE_CONTENT)
        string(COMPARE NOTEQUAL "${FILE_CONTENT}" "${ORIGINAL_FILE_CONTENT}" FILES_DIFFERENT)
        if(FILES_DIFFERENT)
            message(STATUS "${FILE_MANIFEST_PATH}: updating list.")
            file(WRITE "${FILE_MANIFEST_PATH}" "${FILE_CONTENT}")
        endif()
    endif()
endfunction()

# Extract translatable strings from XRC files for gettext
function(generate_xrc_strings XRC_DIR OUTPUT_FILE)
    set(TRANSLATABLE_TAGS label help title tooltip longhelp)

    set(OUTPUT_CONTENT "// Auto-generated from XRC files for gettext extraction\n")
    string(APPEND OUTPUT_CONTENT "// DO NOT EDIT - regenerate by running CMake\n\n")
    string(APPEND OUTPUT_CONTENT "#include <wx/intl.h>\n\n")
    string(APPEND OUTPUT_CONTENT "void _xrc_menu_strings()\n")
    string(APPEND OUTPUT_CONTENT "    {\n")

    file(GLOB XRC_FILES "${XRC_DIR}/*.xrc")
    foreach(XRC_FILE ${XRC_FILES})
        file(READ "${XRC_FILE}" XRC_CONTENT)
        # escape semicolons to prevent CMake from treating them as list separators
        string(REPLACE ";" "@SEMICOLON@" XRC_CONTENT "${XRC_CONTENT}")
        get_filename_component(XRC_FILENAME "${XRC_FILE}" NAME)
        string(APPEND OUTPUT_CONTENT "    // ${XRC_FILENAME}\n")

        foreach(TAG ${TRANSLATABLE_TAGS})
            string(REGEX MATCHALL "<${TAG}>[^<]+</${TAG}>" MATCHES "${XRC_CONTENT}")
            foreach(MATCH ${MATCHES})
                string(REGEX REPLACE "<${TAG}>([^<]+)</${TAG}>" "\\1" TEXT "${MATCH}")
                # restore semicolons
                string(REPLACE "@SEMICOLON@" ";" TEXT "${TEXT}")
                # decode XML entities, the msgid must match the parsed label
                string(REPLACE "&lt;" "<" TEXT "${TEXT}")
                string(REPLACE "&gt;" ">" TEXT "${TEXT}")
                string(REPLACE "&quot;" "\"" TEXT "${TEXT}")
                string(REPLACE "&apos;" "'" TEXT "${TEXT}")
                string(REPLACE "&amp;" "&" TEXT "${TEXT}")
                string(STRIP "${TEXT}" TEXT)
                if(NOT TEXT STREQUAL "")
                    string(REPLACE "\\" "\\\\" TEXT "${TEXT}")
                    string(REPLACE "\"" "\\\"" TEXT "${TEXT}")
                    string(REPLACE "\n" "\\n" TEXT "${TEXT}")
                    string(APPEND OUTPUT_CONTENT "    _(\"${TEXT}\");\n")
                endif()
            endforeach()
        endforeach()
    endforeach()

    string(APPEND OUTPUT_CONTENT "    }\n")

    update_file_if_needed("${OUTPUT_FILE}" "${OUTPUT_CONTENT}")
endfunction()
