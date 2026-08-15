set(workspace "${CMAKE_CURRENT_BINARY_DIR}/cli-utf8-工作区")
file(REMOVE_RECURSE "${workspace}")
file(MAKE_DIRECTORY "${workspace}/笔记")
file(WRITE "${workspace}/config.ini"
    "[archive]\nworkspace = .\ntags_dir = .tags\narchive_dir = .archive\n")
file(WRITE "${workspace}/笔记/中文 空格.md"
    "---\ntags: [算法 标签]\ntitle: 中文 标题\n---\n# UTF-8 正文\n")

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/笔记/中文 空格.md"
    RESULT_VARIABLE add_result ERROR_VARIABLE add_error)
if(NOT add_result EQUAL 0)
    message(FATAL_ERROR "UTF-8 add failed: ${add_error}")
endif()
if(NOT EXISTS "${workspace}/.tags/算法 标签/中文 标题.md")
    message(FATAL_ERROR "UTF-8 tag/title path was not created")
endif()
file(READ "${workspace}/.archive/index.tsv" utf8_index)
if(NOT utf8_index MATCHES "笔记/中文 空格.md")
    message(FATAL_ERROR "UTF-8 source path was not preserved in index.tsv")
endif()

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" list "算法 标签"
    RESULT_VARIABLE list_result OUTPUT_VARIABLE list_output ERROR_VARIABLE list_error)
if(NOT list_result EQUAL 0 OR list_output MATCHES "[.]archive[/\\\\]objects")
    message(FATAL_ERROR "UTF-8 list output is incorrect:\n${list_output}\n${list_error}")
endif()

# A tag is a single path component on every platform; traversal must be rejected.
file(WRITE "${workspace}/笔记/逃逸.md"
    "---\ntags: [../escape]\ntitle: Escape\n---\n")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" --config "${workspace}/config.ini" add
            "${workspace}/笔记/逃逸.md")
if(EXISTS "${workspace}/escape")
    message(FATAL_ERROR "unsafe tag escaped the .tags directory")
endif()
