# Calculates a deterministic SHA-256 hash of all files in the HTML folder
# files are sorted by relative path to ensure reproducibility across platforms
#
# Input variables:
#   HTML_DIR  - absolute path to the html directory
#   TO        - output header file path
#
# Output: generates a header with #define HTML_CONTENT_HASH "..."

if(NOT EXISTS "${HTML_DIR}")
  message(FATAL_ERROR "HTML_DIR does not exist: ${HTML_DIR}")
endif()

file(GLOB_RECURSE _all_files RELATIVE "${HTML_DIR}" "${HTML_DIR}/*")
list(FILTER _all_files EXCLUDE REGEX "\\.(map|scss|ts|tsx)$")
list(SORT _all_files)

# compute hash each files relative path + content together
set(_combined "")
foreach(f IN LISTS _all_files)
  file(SHA256 "${HTML_DIR}/${f}" _fhash)
  string(APPEND _combined "${f}:${_fhash}\n")
endforeach()

string(SHA256 HTML_HASH "${_combined}")

message(STATUS "[html_content_hash] HTML folder hash: ${HTML_HASH}")
message(STATUS "[html_content_hash] Writing to: ${TO}")

file(WRITE "${TO}" "#pragma once\n\n#define HTML_CONTENT_HASH \"${HTML_HASH}\"\n")