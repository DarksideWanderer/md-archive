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

file(GLOB_RECURSE initial_objects "${workspace}/.archive/objects/*.md")
list(LENGTH initial_objects initial_object_count)
if(NOT initial_object_count EQUAL 1)
    message(FATAL_ERROR "expected one hash-addressed archive object, got ${initial_object_count}")
endif()
list(GET initial_objects 0 dijkstra_object)
if(NOT dijkstra_object MATCHES "/[0-9a-f][0-9a-f]/[0-9a-f]+\\.md$")
    message(FATAL_ERROR "archive object does not use hash layout: ${dijkstra_object}")
endif()

# `.tags` is intentionally not transported. A clone containing sources plus
# the portable `.archive` state must recreate native links on the first command.
file(REMOVE_RECURSE "${workspace}/.tags")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" docs
    RESULT_VARIABLE clone_restore_result
    OUTPUT_VARIABLE clone_restore_output
    ERROR_VARIABLE clone_restore_error)
if(NOT clone_restore_result EQUAL 0)
    message(FATAL_ERROR "clone-time link restoration failed\nstdout:\n${clone_restore_output}\nstderr:\n${clone_restore_error}")
endif()
if(NOT EXISTS "${workspace}/.tags/算法/Dijkstra 最短路径.md")
    message(FATAL_ERROR "first command after clone did not restore tag links")
endif()

if(NOT EXISTS "${workspace}/.tags/算法/Dijkstra 最短路径.md")
    message(FATAL_ERROR "tag symlink or stub file was not created")
endif()

# Simulate a symlink created on macOS and checked out by Git on Windows with
# core.symlinks=false: Git materializes it as a one-line regular file.
file(REMOVE "${workspace}/.tags/算法/Dijkstra 最短路径.md")
file(WRITE "${workspace}/.tags/算法/Dijkstra 最短路径.md"
    "../../notes/dijkstra.md\n")

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" list
    RESULT_VARIABLE rebuild_result
    OUTPUT_VARIABLE rebuild_output
    ERROR_VARIABLE rebuild_error)
if(NOT rebuild_result EQUAL 0)
    message(FATAL_ERROR "automatic rebuild failed\nstdout:\n${rebuild_output}\nstderr:\n${rebuild_error}")
endif()

file(READ "${workspace}/.tags/算法/Dijkstra 最短路径.md" rebuilt_content)
if(NOT rebuilt_content MATCHES "# Dijkstra")
    message(FATAL_ERROR "macOS symlink stub was not converted into a usable Windows link")
endif()

# Simulate the reverse direction. Git stores a Windows hard link as an
# independent regular file; the first command must recognize its content and
# recreate a native link on the destination platform.
file(REMOVE "${workspace}/.tags/算法/Dijkstra 最短路径.md")
configure_file(
    "${workspace}/notes/dijkstra.md"
    "${workspace}/.tags/算法/Dijkstra 最短路径.md"
    COPYONLY)
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" docs
    RESULT_VARIABLE reverse_result
    OUTPUT_VARIABLE reverse_output
    ERROR_VARIABLE reverse_error)
if(NOT reverse_result EQUAL 0)
    message(FATAL_ERROR "reverse automatic rebuild failed\nstdout:\n${reverse_output}\nstderr:\n${reverse_error}")
endif()
file(READ "${workspace}/.tags/算法/Dijkstra 最短路径.md" reverse_content)
if(NOT reverse_content MATCHES "# Dijkstra")
    message(FATAL_ERROR "Windows hard-link checkout was not converted into a usable link")
endif()

# An identical copy gets another source-path index entry but no second backup.
configure_file(
    "${workspace}/notes/dijkstra.md"
    "${workspace}/notes/dijkstra-copy.md"
    COPYONLY)
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/dijkstra-copy.md"
    RESULT_VARIABLE copy_result
    OUTPUT_VARIABLE copy_output
    ERROR_VARIABLE copy_error)
if(NOT copy_result EQUAL 0)
    message(FATAL_ERROR "deduplicated copy add failed\nstdout:\n${copy_output}\nstderr:\n${copy_error}")
endif()
file(GLOB_RECURSE copy_objects "${workspace}/.archive/objects/*.md")
list(LENGTH copy_objects copy_object_count)
if(NOT copy_object_count EQUAL 1)
    message(FATAL_ERROR "identical files created redundant backups: ${copy_object_count}")
endif()
file(READ "${workspace}/.archive/index.tsv" copy_index)
if(NOT copy_index MATCHES "notes/dijkstra.md" OR
   NOT copy_index MATCHES "notes/dijkstra-copy.md")
    message(FATAL_ERROR "hash index does not record all identical source paths")
endif()
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" remove
            "${workspace}/notes/dijkstra-copy.md"
    RESULT_VARIABLE copy_remove_result)
if(NOT copy_remove_result EQUAL 0)
    message(FATAL_ERROR "removing a duplicate source mapping failed")
endif()
file(GLOB_RECURSE retained_objects "${workspace}/.archive/objects/*.md")
list(LENGTH retained_objects retained_object_count)
if(NOT retained_object_count EQUAL 1 OR
   NOT EXISTS "${workspace}/.tags/算法/Dijkstra 最短路径.md")
    message(FATAL_ERROR "removing one alias damaged the shared object or canonical tag link")
endif()

# A moved file keeps the same object, drops its missing old path, and retargets
# its tag link to the new source path.
file(WRITE "${workspace}/notes/movable.md"
"---\n"
"tags: [移动测试]\n"
"title: 可移动文档\n"
"---\n"
"# Move me\n")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/movable.md"
    RESULT_VARIABLE movable_add_result)
if(NOT movable_add_result EQUAL 0)
    message(FATAL_ERROR "movable file add failed")
endif()
file(RENAME "${workspace}/notes/movable.md" "${workspace}/notes/moved.md")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/moved.md"
    RESULT_VARIABLE moved_add_result
    OUTPUT_VARIABLE moved_add_output
    ERROR_VARIABLE moved_add_error)
if(NOT moved_add_result EQUAL 0)
    message(FATAL_ERROR "moved file add failed\nstdout:\n${moved_add_output}\nstderr:\n${moved_add_error}")
endif()
file(READ "${workspace}/.archive/index.tsv" moved_index)
if(moved_index MATCHES "notes/movable.md" OR NOT moved_index MATCHES "notes/moved.md")
    message(FATAL_ERROR "move was not reconciled in hash index")
endif()
file(READ "${workspace}/.tags/移动测试/可移动文档.md" moved_link_content)
if(NOT moved_link_content MATCHES "# Move me")
    message(FATAL_ERROR "moved file tag does not resolve to source content")
endif()

# --force changes the source mapping to a new hash and prunes the unreferenced
# old object without conflicting with title replacement behavior.
file(WRITE "${workspace}/notes/moved.md"
"---\n"
"tags: [移动测试]\n"
"title: 可移动文档\n"
"---\n"
"# Changed with force\n")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/moved.md" --force
    RESULT_VARIABLE changed_force_result
    OUTPUT_VARIABLE changed_force_output
    ERROR_VARIABLE changed_force_error)
if(NOT changed_force_result EQUAL 0)
    message(FATAL_ERROR "hash update with --force failed\nstdout:\n${changed_force_output}\nstderr:\n${changed_force_error}")
endif()
file(READ "${workspace}/.tags/移动测试/可移动文档.md" changed_link_content)
if(NOT changed_link_content MATCHES "# Changed with force")
    message(FATAL_ERROR "forced content update did not retarget tag to source")
endif()
file(GLOB_RECURSE changed_objects "${workspace}/.archive/objects/*.md")
list(LENGTH changed_objects changed_object_count)
if(NOT changed_object_count EQUAL 2)
    message(FATAL_ERROR "forced update left an unreferenced archive object: ${changed_object_count}")
endif()

# A pre-1.0 path-mirrored archive is migrated on the next invocation.
file(MAKE_DIRECTORY "${workspace}/legacy" "${workspace}/.archive/legacy")
file(WRITE "${workspace}/legacy/old.md"
"---\n"
"tags: [旧版]\n"
"title: 旧版归档\n"
"---\n"
"# Legacy\n")
configure_file(
    "${workspace}/legacy/old.md"
    "${workspace}/.archive/legacy/old.md"
    COPYONLY)
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" list
    RESULT_VARIABLE migration_result
    OUTPUT_VARIABLE migration_output
    ERROR_VARIABLE migration_error)
if(NOT migration_result EQUAL 0)
    message(FATAL_ERROR "legacy migration failed\nstdout:\n${migration_output}\nstderr:\n${migration_error}")
endif()
if(EXISTS "${workspace}/.archive/legacy/old.md")
    message(FATAL_ERROR "legacy path-mirrored archive was not migrated")
endif()
file(READ "${workspace}/.archive/index.tsv" migrated_index)
if(NOT migrated_index MATCHES "legacy/old.md")
    message(FATAL_ERROR "legacy source path was not added to hash index")
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
