cmake_minimum_required(VERSION 4.3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.4")
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
        "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
else()
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
        "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
endif()

if(NOT DEFINED CMAKE_CXX_STDLIB_MODULES_JSON)
    set(_md_archive_compiler_hint "${CMAKE_CXX_COMPILER}")
    if(NOT _md_archive_compiler_hint AND DEFINED ENV{CXX})
        set(_md_archive_compiler_hint "$ENV{CXX}")
    endif()
    if(NOT _md_archive_compiler_hint)
        find_program(_md_archive_compiler_hint NAMES clang++)
    endif()

    if(_md_archive_compiler_hint)
        if(IS_ABSOLUTE "${_md_archive_compiler_hint}")
            set(_md_archive_compiler_path "${_md_archive_compiler_hint}")
        else()
            find_program(_md_archive_compiler_path NAMES "${_md_archive_compiler_hint}")
        endif()

        if(_md_archive_compiler_path)
            get_filename_component(_md_archive_compiler_realpath "${_md_archive_compiler_path}" REALPATH)
            get_filename_component(_md_archive_compiler_bindir "${_md_archive_compiler_realpath}" DIRECTORY)
            get_filename_component(_md_archive_llvm_prefix "${_md_archive_compiler_bindir}" DIRECTORY)
            set(_md_archive_libcxx_modules_json
                "${_md_archive_llvm_prefix}/lib/c++/libc++.modules.json")
            if(EXISTS "${_md_archive_libcxx_modules_json}")
                set(CMAKE_CXX_STDLIB_MODULES_JSON
                    "${_md_archive_libcxx_modules_json}"
                    CACHE FILEPATH "Path to libc++ standard library module metadata")
            endif()
        endif()
    endif()
endif()

project(md-archive
    VERSION 1.0.1
    DESCRIPTION "Markdown archive and tag navigation CLI"
    LANGUAGES CXX)

option(MD_ARCHIVE_BUILD_API_DOCS "Build API documentation with Doxygen" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

if(NOT 23 IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
    message(FATAL_ERROR
        "This project uses `import std;`, but the active toolchain does not report C++23 import-std support. "
        "Use Clang 18.1.2+ with libc++/libstdc++, Ninja, and set CMAKE_CXX_STDLIB_MODULES_JSON if needed.\n"
        "Active compiler: ${CMAKE_CXX_COMPILER}\n"
        "Compiler id/version: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\n"
        "CMake reason: ${CMAKE_CXX_COMPILER_IMPORT_STD_ERROR_MESSAGE}\n"
        "On macOS, try: cmake --fresh -S . -B build -G Ninja "
        "-DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++")
endif()

add_library(md_archive_core)

target_sources(md_archive_core
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${PROJECT_SOURCE_DIR}/src
        FILES
            src/path_encoding.cppm
            src/content_hash.cppm
            src/archive_store.cppm
            src/config.cppm
            src/frontmatter.cppm
            src/tag_manager.cppm
            src/md_archive.cppm)

target_compile_features(md_archive_core PUBLIC cxx_std_23)
set_target_properties(md_archive_core PROPERTIES
    CXX_MODULE_STD ON
    CXX_SCAN_FOR_MODULES ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(md_archive_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-reserved-module-identifier)
endif()

add_executable(md-archive src/main.cpp)
target_link_libraries(md-archive PRIVATE md_archive_core)
if(WIN32)
    target_link_libraries(md-archive PRIVATE shell32)
endif()
target_compile_features(md-archive PRIVATE cxx_std_23)
target_compile_definitions(md-archive PRIVATE MD_ARCHIVE_VERSION="${PROJECT_VERSION}")
set_target_properties(md-archive PROPERTIES
    CXX_MODULE_STD ON
    CXX_SCAN_FOR_MODULES ON)

install(TARGETS md-archive RUNTIME DESTINATION bin)

configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/uninstall.cmake
    ${PROJECT_BINARY_DIR}/cmake_uninstall.cmake
    @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/cmake_uninstall.cmake
    COMMENT "Uninstalling files listed in install_manifest.txt"
    VERBATIM)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(MD_ARCHIVE_BUILD_API_DOCS)
    find_package(Doxygen REQUIRED)
    add_custom_target(api-docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
endif()
