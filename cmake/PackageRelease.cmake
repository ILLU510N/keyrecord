# keyrecord_release_package 目标调用此脚本生成发布归档（跨平台）。

cmake_minimum_required(VERSION 3.20)

set(BUILD_CONFIG "$ENV{BUILD_CONFIG}")
if(BUILD_CONFIG AND NOT BUILD_CONFIG STREQUAL "Release")
    message(STATUS "Skipping release package for ${BUILD_CONFIG} build")
    return()
endif()

set(TARGET_DIR "$ENV{TARGET_DIR}")
set(OUTPUT_ARCHIVE "$ENV{OUTPUT_ARCHIVE}")
set(ARCHIVE_FORMAT "$ENV{ARCHIVE_FORMAT}")
set(SOURCE_DIR "$ENV{SOURCE_DIR}")
set(KEYRECORD_EXE_PATH "$ENV{KEYRECORD_EXE_PATH}")
set(SERVER_EXE_PATH "$ENV{SERVER_EXE_PATH}")
set(SQLITE_DLL_PATH "$ENV{SQLITE_DLL_PATH}")
set(CONFIG_EXAMPLE_PATH "$ENV{CONFIG_EXAMPLE_PATH}")

if(NOT TARGET_DIR OR NOT OUTPUT_ARCHIVE OR NOT SOURCE_DIR)
    message(FATAL_ERROR "Missing required environment variables: TARGET_DIR, OUTPUT_ARCHIVE, SOURCE_DIR")
endif()
if(NOT ARCHIVE_FORMAT)
    set(ARCHIVE_FORMAT "gztar")
endif()

file(REMOVE_RECURSE "${TARGET_DIR}")
file(MAKE_DIRECTORY "${TARGET_DIR}")

if(KEYRECORD_EXE_PATH AND EXISTS "${KEYRECORD_EXE_PATH}")
    file(COPY "${KEYRECORD_EXE_PATH}" DESTINATION "${TARGET_DIR}")
endif()

if(SERVER_EXE_PATH AND EXISTS "${SERVER_EXE_PATH}")
    file(COPY "${SERVER_EXE_PATH}" DESTINATION "${TARGET_DIR}")
endif()

if(SQLITE_DLL_PATH AND EXISTS "${SQLITE_DLL_PATH}")
    file(COPY "${SQLITE_DLL_PATH}" DESTINATION "${TARGET_DIR}")
endif()

# 运行目录里的动态库（Windows 的 *.dll）随包分发；其它平台通常无此文件。
file(GLOB runtimeDlls "${SOURCE_DIR}/*.dll")
if(runtimeDlls)
    file(COPY ${runtimeDlls} DESTINATION "${TARGET_DIR}")
endif()

if(CONFIG_EXAMPLE_PATH AND EXISTS "${CONFIG_EXAMPLE_PATH}")
    file(COPY "${CONFIG_EXAMPLE_PATH}" DESTINATION "${TARGET_DIR}")
endif()

# 用 cmake -E tar 生成归档，跨平台且不依赖 PowerShell。
file(REMOVE "${OUTPUT_ARCHIVE}")
if(ARCHIVE_FORMAT STREQUAL "zip")
    set(archiveArgs cf "${OUTPUT_ARCHIVE}" --format=zip -- .)
else()
    set(archiveArgs czf "${OUTPUT_ARCHIVE}" -- .)
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar ${archiveArgs}
    WORKING_DIRECTORY "${TARGET_DIR}"
    RESULT_VARIABLE result
)
if(result)
    message(FATAL_ERROR "Failed to create archive: ${result}")
endif()

# Clean up
file(REMOVE_RECURSE "${TARGET_DIR}")

message(STATUS "Created release package: ${OUTPUT_ARCHIVE}")
