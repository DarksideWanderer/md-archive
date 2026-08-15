set(workspace "${CMAKE_CURRENT_BINARY_DIR}/cli-deleted-source-rebuild")
file(REMOVE_RECURSE "${workspace}")
file(MAKE_DIRECTORY "${workspace}/notes")
file(WRITE "${workspace}/config.ini"
    "[archive]\nworkspace = .\ntags_dir = .tags\narchive_dir = .archive\n")
file(WRITE "${workspace}/notes/deleted-source.md"
    "---\ntags: [Recovery]\ntitle: Recover From Archive\n---\n# Durable content\n")
configure_file("${workspace}/notes/deleted-source.md"
               "${workspace}/notes/deleted-source-copy.md" COPYONLY)

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/deleted-source.md"
    RESULT_VARIABLE add_result ERROR_VARIABLE add_error)
if(NOT add_result EQUAL 0)
    message(FATAL_ERROR "fixture add failed: ${add_error}")
endif()
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/deleted-source-copy.md"
    RESULT_VARIABLE copy_add_result ERROR_VARIABLE copy_add_error)
if(NOT copy_add_result EQUAL 0)
    message(FATAL_ERROR "same-hash fixture add failed: ${copy_add_error}")
endif()
file(READ "${workspace}/.archive/index.tsv" source_index)
if(NOT source_index MATCHES "notes/deleted-source.md" OR
   NOT source_index MATCHES "notes/deleted-source-copy.md")
    message(FATAL_ERROR "index did not retain every source path sharing one hash")
endif()
string(REGEX MATCH "([0-9a-f]+) \"notes/deleted-source-copy[.]md\""
       copy_index_row "${source_index}")
set(shared_hash "${CMAKE_MATCH_1}")
if(shared_hash STREQUAL "")
    message(FATAL_ERROR "could not read the platform-specific shared hash from index.tsv")
endif()

# Updating only one of two same-hash paths must split the mappings without
# damaging the other path's object or tag entry.
file(WRITE "${workspace}/notes/deleted-source.md"
    "---\ntags: [RecoveryUpdated]\ntitle: Updated Archive\n---\n# Updated content\n")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/deleted-source.md" --force
    RESULT_VARIABLE update_result ERROR_VARIABLE update_error)
if(NOT update_result EQUAL 0)
    message(FATAL_ERROR "same-hash alias update failed: ${update_error}")
endif()
file(READ "${workspace}/.archive/index.tsv" split_index)
if(NOT split_index MATCHES
       "${shared_hash} \"notes/deleted-source-copy.md\"")
    message(FATAL_ERROR "updating A changed or removed B's old-hash mapping")
endif()
if(split_index MATCHES
   "${shared_hash} \"notes/deleted-source.md\"")
    message(FATAL_ERROR "updated A incorrectly remained on the shared old hash")
endif()
file(GLOB_RECURSE split_objects "${workspace}/.archive/objects/*.md")
list(LENGTH split_objects split_object_count)
if(NOT split_object_count EQUAL 2)
    message(FATAL_ERROR "expected old and updated objects, got ${split_object_count}")
endif()
if(NOT EXISTS "${workspace}/.tags/Recovery/Recover From Archive.md")
    message(FATAL_ERROR "updating A removed B's shared old-hash tag entry")
endif()
if(NOT EXISTS "${workspace}/.tags/RecoveryUpdated/Updated Archive.md")
    message(FATAL_ERROR "updated A did not receive its new tag entry")
endif()

# Different content with the same tag/title has one visible entry. Removing the
# currently visible A must delete only A's mapping and immediately reveal B.
file(WRITE "${workspace}/notes/deleted-source.md"
    "---\ntags: [Recovery]\ntitle: Recover From Archive\n---\n# A replacement\n")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/deleted-source.md" --force
    RESULT_VARIABLE collision_result ERROR_VARIABLE collision_error)
if(NOT collision_result EQUAL 0)
    message(FATAL_ERROR "same-title collision setup failed: ${collision_error}")
endif()
file(READ "${workspace}/.tags/Recovery/Recover From Archive.md" collision_content)
if(NOT collision_content MATCHES "# A replacement")
    message(FATAL_ERROR "--force did not make A the visible same-title entry")
endif()
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" remove
            "${workspace}/notes/deleted-source.md"
    RESULT_VARIABLE remove_a_result ERROR_VARIABLE remove_a_error)
if(NOT remove_a_result EQUAL 0)
    message(FATAL_ERROR "removing visible A failed: ${remove_a_error}")
endif()
file(READ "${workspace}/.archive/index.tsv" after_remove_index)
if(after_remove_index MATCHES "notes/deleted-source.md" OR
   NOT after_remove_index MATCHES "notes/deleted-source-copy.md")
    message(FATAL_ERROR "remove did not delete exactly A's source mapping")
endif()
file(READ "${workspace}/.tags/Recovery/Recover From Archive.md" revealed_b_content)
if(NOT revealed_b_content MATCHES "# Durable content")
    message(FATAL_ERROR "removing A did not immediately reveal remaining B")
endif()

# Deleting a source is different from the explicit `remove` command. The
# durable object and index must be enough to recover both kinds of tag index.
file(REMOVE "${workspace}/notes/deleted-source.md")
file(REMOVE "${workspace}/notes/deleted-source-copy.md")
file(REMOVE_RECURSE "${workspace}/.tags")
file(MAKE_DIRECTORY "${workspace}/.tags")
file(WRITE "${workspace}/.tags/Recovery.md" "# obsolete overview\n")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" rebuild
    RESULT_VARIABLE rebuild_result OUTPUT_VARIABLE rebuild_output ERROR_VARIABLE rebuild_error)
if(NOT rebuild_result EQUAL 0)
    message(FATAL_ERROR "rebuild failed: ${rebuild_error}")
endif()
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" list Recovery
    RESULT_VARIABLE list_result OUTPUT_VARIABLE list_output ERROR_VARIABLE list_error)
if(NOT list_result EQUAL 0)
    message(FATAL_ERROR "list failed: ${list_error}")
endif()
string(FIND "${list_output}" "${shared_hash}.md ->" leaked_hash_path)
if(NOT list_output MATCHES "Recover From Archive" OR NOT leaked_hash_path EQUAL -1)
    message(FATAL_ERROR "list displayed the object hash instead of the title:\n${list_output}")
endif()
if(NOT list_output MATCHES "notes[/\\\\]deleted-source-copy.md")
    message(FATAL_ERROR "list did not display the first indexed source path:\n${list_output}")
endif()

set(entry "${workspace}/.tags/Recovery/Recover From Archive.md")
if(NOT EXISTS "${entry}")
    message(FATAL_ERROR "deleted-source tag entry was not recovered")
endif()
file(READ "${entry}" entry_content)
if(NOT entry_content MATCHES "# Durable content")
    message(FATAL_ERROR "recovered entry does not resolve to the durable object")
endif()

if(EXISTS "${workspace}/.tags/Recovery.md")
    message(FATAL_ERROR "rebuild must not generate a root tag overview page")
endif()
file(READ "${workspace}/.tags/.gitignore" tags_gitignore)
string(FIND "${tags_gitignore}" "*" ignore_all_rule)
if(ignore_all_rule EQUAL -1)
    message(FATAL_ERROR "all generated .tags content must remain ignored")
endif()
