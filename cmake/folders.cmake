# Looks for a folder in the project's folder,
# and if not found begins looking for it in parent folders.
# First argument is the folder to look for, and the second is the result folder.
function(find_directory DIR_TO_SEARCH_FOR FOUND_DIR)
    set(CURRENT_SEARCH_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    foreach(i RANGE 1 5)
        if(EXISTS "${CURRENT_SEARCH_DIR}/${DIR_TO_SEARCH_FOR}")
            set(${FOUND_DIR} "${CURRENT_SEARCH_DIR}/${DIR_TO_SEARCH_FOR}")
            return(PROPAGATE ${FOUND_DIR})
        endif()
        # look in the parent dir in the next loop
        cmake_path(GET CURRENT_SEARCH_DIR PARENT_PATH CURRENT_SEARCH_DIR)
    endforeach()

    # couldn't find it
    set(${FOUND_DIR} "")
    return(PROPAGATE ${FOUND_DIR})
endfunction()