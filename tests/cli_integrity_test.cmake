set(workspace "${CMAKE_CURRENT_BINARY_DIR}/cli-integrity")
file(REMOVE_RECURSE "${workspace}")
file(MAKE_DIRECTORY "${workspace}/notes")
file(WRITE "${workspace}/config.ini"
    "[archive]\nworkspace = .\ntags_dir = .tags\narchive_dir = .archive\n")
file(WRITE "${workspace}/notes/safe.md"
    "---\ntags: [Safety]\ntitle: Safe\n---\n# Original content\n")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/safe.md"
    RESULT_VARIABLE add_result)
if(NOT add_result EQUAL 0)
    message(FATAL_ERROR "integrity fixture add failed")
endif()

# Existing objects are verified and repaired from the source on a forced add.
file(GLOB_RECURSE objects "${workspace}/.archive/objects/*.md")
list(GET objects 0 object)
file(WRITE "${object}" "CORRUPTED")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/safe.md" --force
    RESULT_VARIABLE repair_result)
if(NOT repair_result EQUAL 0)
    message(FATAL_ERROR "corrupt object repair failed")
endif()
file(READ "${object}" repaired_content)
if(NOT repaired_content MATCHES "# Original content")
    message(FATAL_ERROR "forced add trusted a corrupt hash object")
endif()

file(RENAME "${object}" "${object}.bak")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" list Safety
    RESULT_VARIABLE object_recovery_result)
if(NOT object_recovery_result EQUAL 0 OR NOT EXISTS "${object}")
    message(FATAL_ERROR "interrupted object replacement backup was not recovered")
endif()

# Simulate interruption after the old index was moved aside. Startup must
# recover the backup before reading archive state.
file(RENAME "${workspace}/.archive/index.tsv" "${workspace}/.archive/index.tsv.bak")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" list Safety
    RESULT_VARIABLE recovery_result)
if(NOT recovery_result EQUAL 0 OR NOT EXISTS "${workspace}/.archive/index.tsv")
    message(FATAL_ERROR "index backup was not recovered")
endif()

# Invalid traversal rows must neither escape workspace nor be overwritten by a
# subsequent mutation attempt.
file(APPEND "${workspace}/.archive/index.tsv"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa \"../escape.md\"\n")
file(SHA256 "${workspace}/.archive/index.tsv" malformed_before)
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/safe.md" --force
    RESULT_VARIABLE malformed_result)
file(SHA256 "${workspace}/.archive/index.tsv" malformed_after)
if(NOT malformed_before STREQUAL malformed_after)
    message(FATAL_ERROR "malformed index was partially parsed and overwritten")
endif()
if(EXISTS "${workspace}/../escape.md")
    message(FATAL_ERROR "malformed index path escaped workspace")
endif()
