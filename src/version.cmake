execute_process(COMMAND "${GIT}" describe --dirty --match "v${VERSION}" RESULT_VARIABLE RET OUTPUT_VARIABLE DESCRIPTION OUTPUT_STRIP_TRAILING_WHITESPACE)

# Get the current branch name
execute_process(COMMAND "${GIT}" rev-parse --abbrev-ref HEAD RESULT_VARIABLE BRANCH_RET OUTPUT_VARIABLE BRANCH_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)

if(RET) 
    message(WARNING "Cannot determine current revision. Make sure that you are building either from a Git working tree or from a source archive.")
    set(VERSION "${COMMIT}")
else()
    string(REGEX MATCH "[^0-9a-f]([0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f])[0-9a-f]*(-dirty)?" STUB_VAR "${DESCRIPTION}")
    set(VERSION "${CMAKE_MATCH_1}${CMAKE_MATCH_2}")
endif()

# Add branch name suffix if not on master or release
if(NOT BRANCH_RET)
    if(NOT BRANCH_NAME STREQUAL "master" AND NOT BRANCH_NAME STREQUAL "release")
        set(VERSION "${VERSION}-${BRANCH_NAME}")
    endif()
endif()

# Add testnet prefix if TESTNET is defined and true
if(TESTNET)
    set(VERSION "testnet-${VERSION}")
endif()

configure_file("src/version.h.in" "${TO}")