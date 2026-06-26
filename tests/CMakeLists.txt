add_executable(frontmatter_tests frontmatter_tests.cpp)

target_link_libraries(frontmatter_tests PRIVATE md_archive_core)
target_compile_features(frontmatter_tests PRIVATE cxx_std_23)
set_target_properties(frontmatter_tests PROPERTIES
    CXX_MODULE_STD ON
    CXX_SCAN_FOR_MODULES ON)

add_test(NAME frontmatter_tests COMMAND frontmatter_tests)

add_test(
    NAME cli_archive_test
    COMMAND ${CMAKE_COMMAND}
        -DMD_ARCHIVE_BINARY=$<TARGET_FILE:md-archive>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_archive_test.cmake)
