#############################################################################
# Name:        GetGitHash.txt
# Purpose:     Retrieves information about a GIT repository
# Author:      Blake Madden
# Created:     2025-11-03
# Copyright:   (c) 2025 Blake Madden
# License:     Eclipse Public License 2.0
#############################################################################

set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "Suppress developer warnings")

# ==========================================================================================
#  Function: resolve_submodule_gitdir
#  ------------------------------------------------------------------------------------------
#  Resolves the actual Git directory used by a submodule.
#
#  In Git submodules, the `.git` entity is typically a **file** that points to the real Git
#  directory inside the parent repository’s `.git/modules/...` folder. This function detects
#  whether `.git` is a **directory** (standard repo) or a **file** (submodule), and resolves
#  the real Git directory path in either case.
#
#  Arguments:
#    SUBMODULE_ABS_PATH - The absolute path to the submodule working directory.
#    OUT_GITDIR         - The output variable name to store the resolved .git directory path.
#
#  Behavior:
#    - If `.git` is a directory (normal repo): returns it as-is.
#    - If `.git` is a file: parses it to resolve the actual Git directory location.
#    - If `.git` is missing or malformed: fails at configure time.
#
#  Example:
#    file(REAL_PATH "${SUBMODULE_PATH}" SUBMODULE_ABS_PATH BASE_DIRECTORY "${CMAKE_SOURCE_DIR}")
#    resolve_submodule_gitdir("${SUBMODULE_ABS_PATH}" GITDIR)
#    message(STATUS "Git dir: ${GITDIR}")
#
#  Notes:
#    - This function is useful for advanced inspection/debugging scenarios.
#    - For most Git metadata, prefer `git -C <path> <command>` instead of reading `.git` directly.
#
# ==========================================================================================
function(resolve_submodule_gitdir SUBMODULE_ABS_PATH OUT_GITDIR)
    set(GIT_FILE_PATH "${SUBMODULE_ABS_PATH}/.git")

    if(IS_DIRECTORY "${GIT_FILE_PATH}")
        # Normal repo, .git is a directory
        set(${OUT_GITDIR} "${GIT_FILE_PATH}" PARENT_SCOPE)
        return()
    endif()

    if(EXISTS "${GIT_FILE_PATH}")
        file(READ "${GIT_FILE_PATH}" GIT_FILE_CONTENTS)

        # Parse the line: "gitdir: ../.git/modules/xyz"
        string(REGEX MATCH "gitdir: (.+)" _ ${GIT_FILE_CONTENTS})
        set(REL_GITDIR "${CMAKE_MATCH_1}")

        if(REL_GITDIR STREQUAL "")
            message(FATAL_ERROR ".git file in ${SUBMODULE_ABS_PATH} is malformed.")
        endif()

        # Resolve the real path (relative to the submodule)
        file(REAL_PATH "${SUBMODULE_ABS_PATH}/${REL_GITDIR}" ABS_GITDIR)
        set(${OUT_GITDIR} "${ABS_GITDIR}" PARENT_SCOPE)
    else()
        message(FATAL_ERROR ".git not found in submodule: ${SUBMODULE_ABS_PATH}")
    endif()
endfunction()

# ==========================================================================================
#  Function: assert_git_repo
#  ------------------------------------------------------------------------------------------
#  Validates that the given path is a Git working tree (including submodules).
#
#  Arguments:
#    REPO_PATH - Relative or absolute path to the repo to check.
#
#  Fails if Git is not found or the path is not a valid working tree.
# ==========================================================================================
function(assert_git_repo REPO_PATH)
    message(DEBUG "Asserting Git repo at: ${REPO_PATH}")

    if(NOT Git_FOUND)
        message(FATAL_ERROR "Git is not available (Git_FOUND is false)")
    endif()

    if(NOT IS_ABSOLUTE "${REPO_PATH}")
        set(REPO_ABS "${CMAKE_SOURCE_DIR}/${REPO_PATH}")
    else()
        set(REPO_ABS "${REPO_PATH}")
    endif()

    if(NOT EXISTS "${REPO_ABS}")
        message(FATAL_ERROR "Path '${REPO_ABS}' does not exist.")
    endif()

    # Validate with `git rev-parse`
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --git-dir
        WORKING_DIRECTORY "${REPO_ABS}"
        OUTPUT_VARIABLE GIT_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_DIR_RESULT
        ERROR_VARIABLE GIT_DIR_ERROR)

    if(NOT GIT_DIR_RESULT EQUAL 0 OR GIT_DIR STREQUAL "")
        message(FATAL_ERROR
            "Path '${REPO_ABS}' is not a valid Git repository.\n"
            "Git error: ${GIT_DIR_ERROR}\n"
            "Tip: Run `git submodule update --init --recursive`")
    endif()
endfunction()

# ==========================================================================================
#  Function: get_git_submodule_hash
#  ------------------------------------------------------------------------------------------
#  Retrieves the current Git commit hash (SHA-1) of a submodule at a given path.
#
#  This function is useful for generating reproducible builds, embedding submodule hashes
#  into version headers, or generating a Software Bill of Materials (SBOM).
#
#  It enforces that the submodule has no uncommitted or untracked changes (i.e., is clean).
#  If the submodule is dirty, the function fails at configure time with a detailed error.
#
#  Arguments:
#    SUBMODULE_PATH   - Relative path (from top-level source dir) to the Git submodule.
#    OUT_VAR          - The name of the variable to set with the Git commit hash.
#
#  Requirements:
#    - Git must be available and its path stored in GIT_EXECUTABLE.
#    - Git discovery must have succeeded, i.e., Git_FOUND must be TRUE.
#    - The submodule must exist and contain a .git directory.
#
#  Behavior:
#    - Fails at configure time if:
#        - Git is not found.
#        - Submodule is missing or not a valid repo.
#        - Submodule has uncommitted or untracked changes (release build).
#        - Git hash cannot be retrieved.
#    - On success, sets the variable OUT_VAR in the parent scope.
#
#  Example:
#    get_git_submodule_hash("external/Catch2" CATCH2_HASH)
#    message(STATUS "Catch2 submodule hash: ${CATCH2_HASH}")
#
# ==========================================================================================
function(get_git_submodule_hash SUBMODULE_PATH OUT_VAR)
    if(NOT Git_FOUND)
        message(FATAL_ERROR "Git not found (Git_FOUND is false)")
    endif()

    assert_git_repo("${SUBMODULE_PATH}")

    file(REAL_PATH "${SUBMODULE_PATH}" SUBMODULE_ABS_PATH BASE_DIRECTORY "${CMAKE_SOURCE_DIR}")
    resolve_submodule_gitdir("${SUBMODULE_ABS_PATH}" GITDIR)

    message(STATUS "Resolved submodule path: ${SUBMODULE_ABS_PATH}")

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${SUBMODULE_ABS_PATH}" status --porcelain
        OUTPUT_VARIABLE STATUS_OUTPUT
        ERROR_VARIABLE STATUS_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE STATUS_RESULT)

    if(NOT STATUS_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Failed to check Git status for submodule\n"
            "  '${SUBMODULE_ABS_PATH}'\n\n"
            "Git error: ${STATUS_ERROR}")
    endif()

    if(NOT "${STATUS_OUTPUT}" STREQUAL "")
        if(ALLOW_DIRTY_SUBMODULES)
            message(WARNING "Submodule '${SUBMODULE_PATH}' has uncommitted or untracked changes:\n${STATUS_OUTPUT}")
        else()
            message(FATAL_ERROR "Submodule '${SUBMODULE_PATH}' has uncommitted or untracked changes:\n${STATUS_OUTPUT}")
        endif()
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${SUBMODULE_ABS_PATH}" rev-parse HEAD
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE HASH_RESULT
        ERROR_VARIABLE HASH_ERROR)

    if(NOT HASH_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Failed to retrieve Git hash for submodule '${SUBMODULE_ABS_PATH}'\n"
            "Git error: ${HASH_ERROR}")
    endif()

    string(TOUPPER "${GIT_HASH}" GIT_HASH_UPPER)
    set(${OUT_VAR} "${GIT_HASH_UPPER}" PARENT_SCOPE)
endfunction()

# ==========================================================================================
#  Function: get_git_submodule_origin
#  ------------------------------------------------------------------------------------------
#  Retrieves the Git remote origin URL of a submodule using `git config --get remote.origin.url`.
#
#  Arguments:
#    SUBMODULE_PATH   - Relative or absolute path to the submodule.
#    OUT_VAR          - Variable to set with the origin URL.
#
#  Fails if Git is not found or the origin cannot be determined.
# ==========================================================================================
function(get_git_submodule_origin SUBMODULE_PATH OUT_VAR)
    if(NOT Git_FOUND)
        message(FATAL_ERROR "Git not found (Git_FOUND is false)")
    endif()

    file(REAL_PATH "${SUBMODULE_PATH}" SUBMODULE_ABS_PATH BASE_DIRECTORY "${CMAKE_SOURCE_DIR}")
    resolve_submodule_gitdir("${SUBMODULE_ABS_PATH}" GITDIR)

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${SUBMODULE_ABS_PATH}" config --get remote.origin.url
        OUTPUT_VARIABLE ORIGIN
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE ORIGIN_RESULT
        ERROR_VARIABLE ORIGIN_ERROR)

    if(NOT ORIGIN_RESULT EQUAL 0 OR ORIGIN STREQUAL "")
        message(FATAL_ERROR
            "Failed to retrieve Git origin URL for submodule '${SUBMODULE_ABS_PATH}'\n"
            "Git error: ${ORIGIN_ERROR}")
    endif()

    set(${OUT_VAR} "${ORIGIN}" PARENT_SCOPE)
endfunction()

# ==========================================================================================
#  Function: get_git_submodule_description
#  ------------------------------------------------------------------------------------------
#  Retrieves the Git tag or description of a submodule using `git describe --tags --always`.
#
#  Arguments:
#    SUBMODULE_PATH   - Relative or absolute path to the submodule.
#    OUT_VAR          - Variable to set with the Git description.
#
#  Fails if Git is not found or no description is available.
# ==========================================================================================
function(get_git_submodule_description SUBMODULE_PATH OUT_VAR)
    if(NOT Git_FOUND)
        message(FATAL_ERROR "Git not found (Git_FOUND is false)")
    endif()

    file(REAL_PATH "${SUBMODULE_PATH}" SUBMODULE_ABS_PATH BASE_DIRECTORY "${CMAKE_SOURCE_DIR}")
    resolve_submodule_gitdir("${SUBMODULE_ABS_PATH}" GITDIR)

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${SUBMODULE_ABS_PATH}" describe --tags --always
        OUTPUT_VARIABLE GIT_DESC
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE DESC_RESULT
        ERROR_VARIABLE DESC_ERROR)

    if(NOT DESC_RESULT EQUAL 0 OR GIT_DESC STREQUAL "")
        message(FATAL_ERROR
            "Failed to retrieve Git description for submodule '${SUBMODULE_ABS_PATH}'\n"
            "Git error: ${DESC_ERROR}")
    endif()

    set(${OUT_VAR} "${GIT_DESC}" PARENT_SCOPE)
endfunction()
