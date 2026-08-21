set(GIT_WORKING_DIR "${CMAKE_SOURCE_DIR}")

# Get the current commit id without relying on tags.
execute_process(
    COMMAND "${GIT}" rev-parse --short=7 HEAD
    WORKING_DIRECTORY "${GIT_WORKING_DIR}"
    RESULT_VARIABLE COMMIT_RET
    OUTPUT_VARIABLE GIT_COMMIT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT "${COMMIT_RET}" STREQUAL "0")
    message(WARNING "Cannot determine current revision. Make sure that you are building either from a Git working tree or from a source archive.")

    # Preserve the old fallback behavior if COMMIT is supplied elsewhere,
    # for example by a source archive.
    if(DEFINED COMMIT AND NOT "${COMMIT}" STREQUAL "")
        set(VERSION "${COMMIT}")
    else()
        set(VERSION "unknown")
    endif()
else()
    set(VERSION "${GIT_COMMIT}")

    # Detect dirty working tree without using tags.
    # This tracks committed-vs-working-tree changes, similar to `git describe --dirty`.
    execute_process(
        COMMAND "${GIT}" update-index -q --refresh
        WORKING_DIRECTORY "${GIT_WORKING_DIR}"
        ERROR_QUIET
    )

    execute_process(
        COMMAND "${GIT}" diff-index --quiet HEAD --
        WORKING_DIRECTORY "${GIT_WORKING_DIR}"
        RESULT_VARIABLE DIRTY_RET
        ERROR_QUIET
    )

    if("${DIRTY_RET}" STREQUAL "1")
        set(VERSION "${VERSION}-dirty")
    elseif(NOT "${DIRTY_RET}" STREQUAL "0")
        message(WARNING "Cannot determine whether the Git working tree is dirty.")
    endif()
endif()

# Get the current branch name without relying on tags.
# symbolic-ref fails in detached HEAD instead of returning "HEAD".
execute_process(
    COMMAND "${GIT}" symbolic-ref --quiet --short HEAD
    WORKING_DIRECTORY "${GIT_WORKING_DIR}"
    RESULT_VARIABLE BRANCH_RET
    OUTPUT_VARIABLE BRANCH_NAME
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Add branch name suffix if not on master or release.
if("${BRANCH_RET}" STREQUAL "0" AND NOT "${BRANCH_NAME}" STREQUAL "")
    if(NOT "${BRANCH_NAME}" STREQUAL "master" AND NOT "${BRANCH_NAME}" STREQUAL "release")
        # Make branch names safe for version strings:
        # feature/foo -> feature-foo
        string(REGEX REPLACE "[^A-Za-z0-9._-]" "-" BRANCH_NAME_SAFE "${BRANCH_NAME}")
        set(VERSION "${VERSION}-${BRANCH_NAME_SAFE}")
    endif()
endif()

# Add testnet prefix if TESTNET is defined and true.
if(TESTNET)
    set(VERSION "testnet-${VERSION}")
endif()

configure_file("src/version.h.in" "${TO}")
