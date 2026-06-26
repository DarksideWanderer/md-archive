if(NOT DEFINED MD_ARCHIVE_BINARY)
    message(FATAL_ERROR "MD_ARCHIVE_BINARY is required")
endif()

set(workspace "${CMAKE_CURRENT_BINARY_DIR}/cli-workspace")
file(REMOVE_RECURSE "${workspace}")
file(MAKE_DIRECTORY "${workspace}" "${workspace}/notes")

file(WRITE "${workspace}/config.ini"
"[archive]\n"
"workspace = ${workspace}\n"
"tags_dir = .tags\n"
"archive_dir = .archive\n")

file(WRITE "${workspace}/notes/dijkstra.md"
"---\n"
"tags: [算法, 图论]\n"
"title: Dijkstra 最短路径\n"
"---\n"
"# Dijkstra\n")

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add "${workspace}/notes/dijkstra.md"
    RESULT_VARIABLE add_result
    OUTPUT_VARIABLE add_output
    ERROR_VARIABLE add_error)
if(NOT add_result EQUAL 0)
    message(FATAL_ERROR "add failed\nstdout:\n${add_output}\nstderr:\n${add_error}")
endif()

if(NOT EXISTS "${workspace}/.archive/notes/dijkstra.md")
    message(FATAL_ERROR "archive copy was not created")
endif()

if(NOT EXISTS "${workspace}/.tags/算法/Dijkstra 最短路径.md")
    message(FATAL_ERROR "tag symlink or stub file was not created")
endif()

if(NOT EXISTS "${workspace}/.tags/算法.md")
    message(FATAL_ERROR "tag index was not created")
endif()

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add "${workspace}/notes/dijkstra.md"
    RESULT_VARIABLE duplicate_result
    OUTPUT_VARIABLE duplicate_output
    ERROR_VARIABLE duplicate_error)
if(NOT duplicate_result EQUAL 0)
    message(FATAL_ERROR "duplicate add failed\nstdout:\n${duplicate_output}\nstderr:\n${duplicate_error}")
endif()

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add "${workspace}/notes/dijkstra.md" --force
    RESULT_VARIABLE force_result
    OUTPUT_VARIABLE force_output
    ERROR_VARIABLE force_error)
if(NOT force_result EQUAL 0)
    message(FATAL_ERROR "force add failed\nstdout:\n${force_output}\nstderr:\n${force_error}")
endif()

file(WRITE "${workspace}/notes/other.md"
"---\n"
"tags: [算法]\n"
"title: Dijkstra 最短路径\n"
"---\n"
"# Other\n")

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add "${workspace}/notes/other.md"
    RESULT_VARIABLE title_conflict_result
    OUTPUT_VARIABLE title_conflict_output
    ERROR_VARIABLE title_conflict_error)
if(NOT title_conflict_result EQUAL 0)
    message(FATAL_ERROR "title conflict add failed\nstdout:\n${title_conflict_output}\nstderr:\n${title_conflict_error}")
endif()

if(IS_SYMLINK "${workspace}/.tags/算法/Dijkstra 最短路径.md")
    file(READ_SYMLINK "${workspace}/.tags/算法/Dijkstra 最短路径.md" conflict_target)
    if(NOT conflict_target MATCHES "dijkstra.md")
        message(FATAL_ERROR "title conflict without force should keep existing link, got ${conflict_target}")
    endif()
endif()

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add "${workspace}/notes/other.md" --force
    RESULT_VARIABLE title_force_result
    OUTPUT_VARIABLE title_force_output
    ERROR_VARIABLE title_force_error)
if(NOT title_force_result EQUAL 0)
    message(FATAL_ERROR "title conflict force failed\nstdout:\n${title_force_output}\nstderr:\n${title_force_error}")
endif()

if(IS_SYMLINK "${workspace}/.tags/算法/Dijkstra 最短路径.md")
    file(READ_SYMLINK "${workspace}/.tags/算法/Dijkstra 最短路径.md" force_target)
    if(NOT force_target MATCHES "other.md")
        message(FATAL_ERROR "title conflict with force should replace link, got ${force_target}")
    endif()
endif()

file(WRITE "${workspace}/.archive/generated.md"
"---\n"
"tags: [ShouldSkip]\n"
"title: Generated\n"
"---\n")

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" scan
    RESULT_VARIABLE scan_result
    OUTPUT_VARIABLE scan_output
    ERROR_VARIABLE scan_error)
if(NOT scan_result EQUAL 0)
    message(FATAL_ERROR "scan failed\nstdout:\n${scan_output}\nstderr:\n${scan_error}")
endif()

if(EXISTS "${workspace}/.tags/ShouldSkip.md")
    message(FATAL_ERROR "scan should skip .archive")
endif()
