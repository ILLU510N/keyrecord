# keyrecord 目标 POST_BUILD 调用此脚本生成 Release zip。

cmake_minimum_required(VERSION 3.20)

set(BUILD_CONFIG "$ENV{BUILD_CONFIG}")
if(BUILD_CONFIG AND NOT BUILD_CONFIG STREQUAL "Release")
    message(STATUS "Skipping release package for ${BUILD_CONFIG} build")
    return()
endif()

set(TARGET_DIR "$ENV{TARGET_DIR}")
set(OUTPUT_ZIP "$ENV{OUTPUT_ZIP}")
set(KEYRECORD_EXE_PATH "$ENV{KEYRECORD_EXE_PATH}")
set(SERVER_EXE_PATH "$ENV{SERVER_EXE_PATH}")
set(SOURCE_DIR "$ENV{SOURCE_DIR}")
set(SQLITE_DLL_PATH "$ENV{SQLITE_DLL_PATH}")

if(NOT TARGET_DIR OR NOT OUTPUT_ZIP OR NOT KEYRECORD_EXE_PATH OR NOT SOURCE_DIR)
    message(FATAL_ERROR "Missing required environment variables: TARGET_DIR, OUTPUT_ZIP, KEYRECORD_EXE_PATH, SOURCE_DIR")
endif()

file(REMOVE_RECURSE "${TARGET_DIR}")
file(MAKE_DIRECTORY "${TARGET_DIR}")

file(COPY "${KEYRECORD_EXE_PATH}" DESTINATION "${TARGET_DIR}")

if(SERVER_EXE_PATH AND EXISTS "${SERVER_EXE_PATH}")
    file(COPY "${SERVER_EXE_PATH}" DESTINATION "${TARGET_DIR}")
endif()

if(SQLITE_DLL_PATH AND EXISTS "${SQLITE_DLL_PATH}")
    file(COPY "${SQLITE_DLL_PATH}" DESTINATION "${TARGET_DIR}")
endif()

file(GLOB runtimeDlls "${SOURCE_DIR}/*.dll")
if(runtimeDlls)
    file(COPY ${runtimeDlls} DESTINATION "${TARGET_DIR}")
endif()

execute_process(
    COMMAND powershell -NoProfile -Command
        "Compress-Archive -Path '${TARGET_DIR}/*' -DestinationPath '${OUTPUT_ZIP}' -Force"
    RESULT_VARIABLE result
)
if(result)
    message(FATAL_ERROR "Failed to create zip archive: ${result}")
endif()

# Clean up
file(REMOVE_RECURSE "${TARGET_DIR}")

message(STATUS "Created release package: ${OUTPUT_ZIP}")
