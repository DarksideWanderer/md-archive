set(test_root "${CMAKE_CURRENT_BINARY_DIR}/cli-config")
file(REMOVE_RECURSE "${test_root}")
file(MAKE_DIRECTORY "${test_root}")

execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" init
    WORKING_DIRECTORY "${test_root}"
    RESULT_VARIABLE init_result
    OUTPUT_VARIABLE init_output
    ERROR_VARIABLE init_error)
if(NOT init_result EQUAL 0)
    message(FATAL_ERROR "init failed\nstdout:\n${init_output}\nstderr:\n${init_error}")
endif()

file(READ "${test_root}/config.ini" generated_config)
if(NOT generated_config MATCHES "workspace = \\.($|[\r\n])")
    message(FATAL_ERROR "init must generate a portable relative workspace\n${generated_config}")
endif()

file(WRITE "${test_root}/config.ini"
    "[archive]\n"
    "workspace = Z:/md-archive-path-that-must-not-exist\n"
    "tags_dir = .tags\n"
    "archive_dir = .archive\n")
execute_process(
    COMMAND "${MD_ARCHIVE_BINARY}" rebuild
    WORKING_DIRECTORY "${test_root}"
    RESULT_VARIABLE invalid_result
    OUTPUT_VARIABLE invalid_output
    ERROR_VARIABLE invalid_error)
if(invalid_result EQUAL 0)
    message(FATAL_ERROR "invalid workspace unexpectedly succeeded")
endif()
if(NOT invalid_error MATCHES "config file loaded")
    message(FATAL_ERROR "error must say that config.ini was read\nstderr:\n${invalid_error}")
endif()
if(invalid_error MATCHES "md-archive init")
    message(FATAL_ERROR "invalid config must not be reported as a missing config\nstderr:\n${invalid_error}")
endif()
