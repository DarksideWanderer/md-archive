set(workspace "${CMAKE_CURRENT_BINARY_DIR}/cli-hierarchical-search")
file(REMOVE_RECURSE "${workspace}")
file(MAKE_DIRECTORY "${workspace}/notes")
file(WRITE "${workspace}/config.ini"
    "[archive]\nworkspace = .\ntags_dir = .tags\narchive_dir = .archive\n")
file(WRITE "${workspace}/notes/guide.md"
    "---\ntags: [图论/树/基础, Graph/Tree/Base]\ntitle: 层级树指南\n---\n# 正文\nBellman-Ford appears only in content.\n")

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/notes/guide.md"
    RESULT_VARIABLE add_result OUTPUT_VARIABLE add_output ERROR_VARIABLE add_error)
if(NOT add_result EQUAL 0)
    message(FATAL_ERROR "hierarchical add failed:\n${add_output}\n${add_error}")
endif()
if(NOT EXISTS "${workspace}/.tags/图论/树/基础/层级树指南.md")
    message(FATAL_ERROR "hierarchical tag entry was not created as nested directories")
endif()

file(WRITE "${workspace}/notes/parent.md"
    "---\ntags: [Graph]\ntitle: Parent Only\n---\n# Exact parent document\n")
file(WRITE "${workspace}/notes/flow.md"
    "---\ntags: [Graph/Flow]\ntitle: Flow Child\n---\n# Sibling child document\n")
foreach(extra_doc IN ITEMS parent.md flow.md)
    execute_process(
        COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
                "${workspace}/notes/${extra_doc}"
        RESULT_VARIABLE extra_result OUTPUT_VARIABLE extra_output ERROR_VARIABLE extra_error)
    if(NOT extra_result EQUAL 0)
        message(FATAL_ERROR "could not add ${extra_doc}:\n${extra_output}\n${extra_error}")
    endif()
endforeach()

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" list
    RESULT_VARIABLE list_result OUTPUT_VARIABLE list_output ERROR_VARIABLE list_error)
if(NOT list_result EQUAL 0 OR NOT list_output MATCHES "Graph/Tree/Base")
    message(FATAL_ERROR "list did not use the Unix-style hierarchical tag:\n${list_output}\n${list_error}")
endif()
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" list "Graph/Tree/Base"
    RESULT_VARIABLE tag_result OUTPUT_VARIABLE tag_output ERROR_VARIABLE tag_error)
if(NOT tag_result EQUAL 0 OR NOT tag_output MATCHES "notes[/\\\\]guide.md")
    message(FATAL_ERROR "hierarchical tag lookup failed:\n${tag_output}\n${tag_error}")
endif()

# Parent queries include every descendant by default and deduplicate documents.
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" list Graph
    RESULT_VARIABLE parent_result OUTPUT_VARIABLE parent_output ERROR_VARIABLE parent_error)
if(NOT parent_result EQUAL 0 OR
   NOT parent_output MATCHES "notes[/\\\\]guide.md" OR
   NOT parent_output MATCHES "notes[/\\\\]parent.md" OR
   NOT parent_output MATCHES "notes[/\\\\]flow.md")
    message(FATAL_ERROR "parent tag did not aggregate descendants:\n${parent_output}\n${parent_error}")
endif()

# -exact may appear before or after the tag and excludes descendant-only matches.
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" list -exact Graph
    RESULT_VARIABLE exact_result OUTPUT_VARIABLE exact_output ERROR_VARIABLE exact_error)
if(NOT exact_result EQUAL 0 OR
   NOT exact_output MATCHES "notes[/\\\\]parent.md" OR
   exact_output MATCHES "notes[/\\\\](guide|flow).md")
    message(FATAL_ERROR "exact parent lookup was not exact:\n${exact_output}\n${exact_error}")
endif()
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" list Graph/Tree --exact
    RESULT_VARIABLE branch_exact_result OUTPUT_VARIABLE branch_exact_output
    ERROR_VARIABLE branch_exact_error)
if(NOT branch_exact_result EQUAL 0 OR branch_exact_output MATCHES "guide.md")
    message(FATAL_ERROR "exact intermediate lookup included descendants:\n${branch_exact_output}\n${branch_exact_error}")
endif()

if(WIN32)
    # Simulate MSYS2 converting Graph/Tree into an absolute path for a native executable.
    execute_process(
        COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini"
                list "${workspace}/Graph/Tree"
        WORKING_DIRECTORY "${workspace}"
        RESULT_VARIABLE converted_result OUTPUT_VARIABLE converted_output
        ERROR_VARIABLE converted_error)
    if(NOT converted_result EQUAL 0 OR NOT converted_output MATCHES "guide.md")
        message(FATAL_ERROR "MSYS2-converted tag lookup failed:\n${converted_output}\n${converted_error}")
    endif()
endif()

# `.tags` is disposable. A read command must still discover every indexed source
# and the constructor should repair the nested view without a manual rebuild.
file(REMOVE_RECURSE "${workspace}/.tags")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" docs
    RESULT_VARIABLE docs_result OUTPUT_VARIABLE docs_output ERROR_VARIABLE docs_error)
if(NOT docs_result EQUAL 0 OR NOT docs_output MATCHES "notes[/\\\\]guide.md")
    message(FATAL_ERROR "docs missed an indexed document after .tags removal:\n${docs_output}\n${docs_error}")
endif()
if(NOT EXISTS "${workspace}/.tags/图论/树/基础/层级树指南.md")
    message(FATAL_ERROR "docs did not automatically restore the hierarchical tag view")
endif()

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" search "guide"
    RESULT_VARIABLE name_result OUTPUT_VARIABLE name_output ERROR_VARIABLE name_error)
if(NOT name_result EQUAL 0 OR NOT name_output MATCHES "notes[/\\\\]guide.md" OR
   NOT name_output MATCHES "Graph/Tree/Base")
    message(FATAL_ERROR "name search output is incorrect:\n${name_output}\n${name_error}")
endif()
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" search "Bellman-Ford"
    RESULT_VARIABLE shallow_result OUTPUT_VARIABLE shallow_output ERROR_VARIABLE shallow_error)
if(NOT shallow_result EQUAL 0 OR shallow_output MATCHES "guide.md")
    message(FATAL_ERROR "default search unexpectedly searched content:\n${shallow_output}\n${shallow_error}")
endif()
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" search -all "Bellman-Ford"
    RESULT_VARIABLE full_result OUTPUT_VARIABLE full_output ERROR_VARIABLE full_error)
if(NOT full_result EQUAL 0 OR NOT full_output MATCHES "notes[/\\\\]guide.md" OR
   NOT full_output MATCHES "Bellman-Ford appears only in content")
    message(FATAL_ERROR "full-text search failed:\n${full_output}\n${full_error}")
endif()

foreach(unsafe_tag IN ITEMS "图论//树" "图论\\树" "图论/../树")
    file(WRITE "${workspace}/notes/unsafe.md"
        "---\ntags: [${unsafe_tag}]\ntitle: Unsafe\n---\n")
    execute_process(
        COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
                "${workspace}/notes/unsafe.md"
        OUTPUT_VARIABLE unsafe_output ERROR_VARIABLE unsafe_error)
    file(READ "${workspace}/.archive/index.tsv" archive_index)
    if(archive_index MATCHES "unsafe.md")
        message(FATAL_ERROR "unsafe hierarchical tag was archived: ${unsafe_tag}\n${unsafe_output}\n${unsafe_error}")
    endif()
endforeach()
