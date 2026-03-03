cmake_minimum_required(VERSION 3.25)

# Exclusion filter for the main Wisteria library
# (excludes test files, demo, UI components not part of core library, and submodule extras)
set(WISTERIA_LIB_EXCLUDE_FILTER "(testmainc|xml_format|rtf_extract|postscript_extract|cpp_extract|\
odt_odp_extract|doc_extract|docx_extract|ui/app[.]cpp|codeeditor[.]cpp|htmltablewin[.]cpp|\
listctrlex[.]cpp|listctrlexdataprovider[.]cpp|searchpanel[.]cpp|warningmanager[.]cpp|\
screenshot[.]cpp|idhelpers[.]cpp|mainframe[.]cpp|warningmessagesdlg[.]cpp|startpage[.]cpp|\
htmltablepanel[.]cpp|htmltablewinprintout[.]cpp|listctrlitemviewdlg[.]cpp|listdlg[.]cpp|\
listctrlsortdlg[.]cpp|artmetro[.]cpp|filelistdlg[.]cpp|functionbrowserdlg[.]cpp|getdirdlg[.]cpp|\
gridexportdlg[.]cpp|archivedlg[.]cpp|edittextdlg[.]cpp|excelpreviewdlg[.]cpp|sidebar[.]cpp|\
sidebarbook[.]cpp|downloadfile[.]cpp|formattedtextctrl[.]cpp|gtktextview-helper[.]cpp|\
demo[.]cpp|main[.]cpp|src/app/|utfcpp/tests/|utfcpp/samples/|utfcpp/extern)")

# Exclusion filter for GUI testing library (less restrictive, includes more UI components)
set(WISTERIA_GUI_LIB_EXCLUDE_FILTER "(testmainc|formattedtextctrl[.]cpp|gtktextview-helper[.]cpp|\
codeeditor[.]cpp|functionbrowserdlg[.]cpp|demo[.]cpp|main[.]cpp|src/app/|utfcpp/tests/|utfcpp/samples/|\
utfcpp/extern)")

# Use the directory containing this file to find the project root
# (works correctly whether included from main CMakeLists.txt or test CMakeLists.txt)
get_filename_component(FILE_SRC_PATH "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

message(STATUS "Generating file lists for build system.")

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

# Generate main library source file list
block()
    file(GLOB_RECURSE LISTED_SRC_FILES LIST_DIRECTORIES false RELATIVE ${FILE_SRC_PATH}
         "${FILE_SRC_PATH}/src/*.cpp")
    # Include cJSON.c file
    file(GLOB_RECURSE C_SRC_FILES LIST_DIRECTORIES false RELATIVE ${FILE_SRC_PATH}
         "${FILE_SRC_PATH}/src/*cJSON[.]c")
    list(APPEND LISTED_SRC_FILES ${C_SRC_FILES})
    list(SORT LISTED_SRC_FILES CASE INSENSITIVE)
    # Filter out unwanted files
    list(FILTER LISTED_SRC_FILES EXCLUDE REGEX ${WISTERIA_LIB_EXCLUDE_FILTER})

    set(FILE_CONTENT "# Automatically generated from 'ListFiles.cmake'\n")
    string(APPEND FILE_CONTENT "# DO NOT MODIFY MANUALLY!\n\n")
    string(APPEND FILE_CONTENT "SET(WISTERIA_SRC")
    foreach(CURR_FILE IN LISTS LISTED_SRC_FILES)
        string(APPEND FILE_CONTENT "\n    ${CURR_FILE}")
    endforeach()
    string(APPEND FILE_CONTENT ")")

    set(FILE_MANIFEST_PATH "${FILE_SRC_PATH}/cmake/includes/libfiles.cmake")
    update_file_if_needed("${FILE_MANIFEST_PATH}" "${FILE_CONTENT}")
endblock()

# Generate GUI testing library source file list (includes more UI components)
block()
    file(GLOB_RECURSE LISTED_SRC_FILES LIST_DIRECTORIES false RELATIVE ${FILE_SRC_PATH}
         "${FILE_SRC_PATH}/src/*.cpp")
    # Include cJSON.c file
    file(GLOB_RECURSE C_SRC_FILES LIST_DIRECTORIES false RELATIVE ${FILE_SRC_PATH}
         "${FILE_SRC_PATH}/src/*cJSON[.]c")
    list(APPEND LISTED_SRC_FILES ${C_SRC_FILES})
    list(SORT LISTED_SRC_FILES CASE INSENSITIVE)
    # Filter out unwanted files (less restrictive for GUI testing)
    list(FILTER LISTED_SRC_FILES EXCLUDE REGEX ${WISTERIA_GUI_LIB_EXCLUDE_FILTER})

    # Prepend relative path for test directory usage
    set(PREFIXED_FILES "")
    foreach(CURR_FILE IN LISTS LISTED_SRC_FILES)
        list(APPEND PREFIXED_FILES "../../${CURR_FILE}")
    endforeach()

    set(FILE_CONTENT "# Automatically generated from 'ListFiles.cmake'\n")
    string(APPEND FILE_CONTENT "# This should be used for the unit test runner.\n")
    string(APPEND FILE_CONTENT "# DO NOT MODIFY MANUALLY!\n\n")
    string(APPEND FILE_CONTENT "SET(WISTERIA_SRC")
    foreach(CURR_FILE IN LISTS PREFIXED_FILES)
        string(APPEND FILE_CONTENT "\n    ${CURR_FILE}")
    endforeach()
    string(APPEND FILE_CONTENT ")")

    set(FILE_MANIFEST_PATH "${FILE_SRC_PATH}/cmake/includes/guilibfiles_testing.cmake")
    update_file_if_needed("${FILE_MANIFEST_PATH}" "${FILE_CONTENT}")
endblock()

# Generate non-GUI test file list
block()
    file(GLOB LISTED_TEST_FILES LIST_DIRECTORIES false RELATIVE "${FILE_SRC_PATH}/tests"
         "${FILE_SRC_PATH}/tests/*.cpp")
    list(SORT LISTED_TEST_FILES CASE INSENSITIVE)

    set(FILE_CONTENT "# Automatically generated from 'ListFiles.cmake'\n")
    string(APPEND FILE_CONTENT "# This should be used for the unit test runner.\n")
    string(APPEND FILE_CONTENT "# DO NOT MODIFY MANUALLY!\n\n")
    string(APPEND FILE_CONTENT "SET(TEST_SRC_FILES")
    foreach(CURR_FILE IN LISTS LISTED_TEST_FILES)
        string(APPEND FILE_CONTENT "\n    ${CURR_FILE}")
    endforeach()
    string(APPEND FILE_CONTENT ")")

    set(FILE_MANIFEST_PATH "${FILE_SRC_PATH}/cmake/includes/testfiles.cmake")
    update_file_if_needed("${FILE_MANIFEST_PATH}" "${FILE_CONTENT}")
endblock()

# Generate app source file list (all sources except test and demo files)
block()
    file(GLOB_RECURSE LISTED_SRC_FILES LIST_DIRECTORIES false RELATIVE ${FILE_SRC_PATH}
         "${FILE_SRC_PATH}/src/*.cpp")
    # Include cJSON.c file
    file(GLOB_RECURSE C_SRC_FILES LIST_DIRECTORIES false RELATIVE ${FILE_SRC_PATH}
         "${FILE_SRC_PATH}/src/*cJSON[.]c")
    list(APPEND LISTED_SRC_FILES ${C_SRC_FILES})
    list(SORT LISTED_SRC_FILES CASE INSENSITIVE)
    # Filter out unwanted files (only exclude test, demo, and submodule extras)
    list(FILTER LISTED_SRC_FILES EXCLUDE REGEX ${WISTERIA_GUI_LIB_EXCLUDE_FILTER})
    # Add back the app source files (excluded by the shared filter above)
    file(GLOB APP_SPECIFIC_FILES LIST_DIRECTORIES false RELATIVE ${FILE_SRC_PATH}
         "${FILE_SRC_PATH}/src/app/*.cpp")
    list(APPEND LISTED_SRC_FILES ${APP_SPECIFIC_FILES})
    list(SORT LISTED_SRC_FILES CASE INSENSITIVE)

    set(FILE_CONTENT "# Automatically generated from 'ListFiles.cmake'\n")
    string(APPEND FILE_CONTENT "# This should be used for the application build.\n")
    string(APPEND FILE_CONTENT "# DO NOT MODIFY MANUALLY!\n\n")
    string(APPEND FILE_CONTENT "SET(APP_WISTERIA_SRC")
    foreach(CURR_FILE IN LISTS LISTED_SRC_FILES)
        string(APPEND FILE_CONTENT "\n    ${CURR_FILE}")
    endforeach()
    string(APPEND FILE_CONTENT ")")

    set(FILE_MANIFEST_PATH "${FILE_SRC_PATH}/cmake/includes/appfiles.cmake")
    update_file_if_needed("${FILE_MANIFEST_PATH}" "${FILE_CONTENT}")
endblock()

# Generate GUI test file list
block()
    file(GLOB LISTED_TEST_FILES LIST_DIRECTORIES false RELATIVE "${FILE_SRC_PATH}/tests/gui-tests"
         "${FILE_SRC_PATH}/tests/gui-tests/*.cpp")
    list(SORT LISTED_TEST_FILES CASE INSENSITIVE)

    set(FILE_CONTENT "# Automatically generated from 'ListFiles.cmake'\n")
    string(APPEND FILE_CONTENT "# This should be used for the unit test runner.\n")
    string(APPEND FILE_CONTENT "# DO NOT MODIFY MANUALLY!\n\n")
    string(APPEND FILE_CONTENT "SET(TEST_SRC_FILES")
    foreach(CURR_FILE IN LISTS LISTED_TEST_FILES)
        string(APPEND FILE_CONTENT "\n    ${CURR_FILE}")
    endforeach()
    string(APPEND FILE_CONTENT ")")

    set(FILE_MANIFEST_PATH "${FILE_SRC_PATH}/cmake/includes/guitestfiles.cmake")
    update_file_if_needed("${FILE_MANIFEST_PATH}" "${FILE_CONTENT}")
endblock()

# Resource files to exclude from res.wad
set(RES_FILES_EXCLUDE_FILTER "(thisisengineering-raeng-64YrPKiguAE-unsplash|tobias_Blue_Twingo)")

# Generate flat list of resource files to include in res.wad
block()
    file(GLOB_RECURSE LISTED_RES_FILES LIST_DIRECTORIES false
         RELATIVE "${FILE_SRC_PATH}/res"
         "${FILE_SRC_PATH}/res/*.svg"
         "${FILE_SRC_PATH}/res/*.png"
         "${FILE_SRC_PATH}/res/*.jpg")
    list(FILTER LISTED_RES_FILES EXCLUDE REGEX ${RES_FILES_EXCLUDE_FILTER})
    list(SORT LISTED_RES_FILES CASE INSENSITIVE)

    set(FILE_CONTENT "")
    foreach(CURR_FILE IN LISTS LISTED_RES_FILES)
        string(APPEND FILE_CONTENT "${CURR_FILE}\n")
    endforeach()
    string(STRIP "${FILE_CONTENT}" FILE_CONTENT)

    set(FILE_MANIFEST_PATH "${FILE_SRC_PATH}/cmake/includes/resfiles.cmake")
    update_file_if_needed("${FILE_MANIFEST_PATH}" "${FILE_CONTENT}")
endblock()
