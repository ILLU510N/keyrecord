cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

project(keyrecord)

set(CMAKE_CXX_STANDARD 20)
set(KEYRECORD_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(EXISTS "C:/vcpkg/installed/x64-windows")
    list(APPEND CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows")
endif()

find_package(unofficial-sqlite3 CONFIG REQUIRED)

find_path(KEYRECORD_BOOST_INCLUDE_DIR
    NAMES boost/beast.hpp
    HINTS "C:/vcpkg/installed/x64-windows/include"
    DOC "Directory that contains the Boost headers root (the folder that contains boost/beast.hpp)"
)

add_definitions(-DUNICODE -D_UNICODE)

file(GLOB_RECURSE KEYRECORD_STATIC_RESOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/visualize/public/*"
)
set(KEYRECORD_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(KEYRECORD_GENERATED_RESOURCES_CPP "${KEYRECORD_GENERATED_DIR}/generated_resources.cpp")
set(KEYRECORD_GENERATED_RESOURCES_H "${KEYRECORD_GENERATED_DIR}/generated_resources.h")
set(KEYRECORD_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/keyrecord.ico")
set(KEYRECORD_APP_RESOURCE_RC "${CMAKE_CURRENT_BINARY_DIR}/keyrecord.rc")

if(NOT EXISTS "${KEYRECORD_ICON_FILE}")
    message(FATAL_ERROR "Application icon not found: ${KEYRECORD_ICON_FILE}")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/keyrecord.rc.in"
    "${KEYRECORD_APP_RESOURCE_RC}"
    @ONLY
)
set_source_files_properties("${KEYRECORD_APP_RESOURCE_RC}"
    PROPERTIES OBJECT_DEPENDS "${KEYRECORD_ICON_FILE}"
)

function(copy_sqlite_dll target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:unofficial::sqlite3::sqlite3>
            $<TARGET_FILE_DIR:${target_name}>
    )
endfunction()

function(enable_msvc_utf8 target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /utf-8)
    endif()
endfunction()

set(KEYRECORD_DEBUG_TEST_TARGETS)

function(add_keyrecord_test target_name)
    set(options COPY_SQLITE_DLL)
    set(multiValueArgs SOURCES LINKS INCLUDES)
    cmake_parse_arguments(ARG "${options}" "" "${multiValueArgs}" ${ARGN})

    add_executable(${target_name} ${ARG_SOURCES})
    if(ARG_INCLUDES)
        target_include_directories(${target_name} PRIVATE ${ARG_INCLUDES})
    endif()
    if(ARG_LINKS)
        target_link_libraries(${target_name} PRIVATE ${ARG_LINKS})
    endif()
    if(ARG_COPY_SQLITE_DLL)
        copy_sqlite_dll(${target_name})
    endif()
    add_test(NAME ${target_name} COMMAND ${target_name})
    set(KEYRECORD_DEBUG_TEST_TARGETS
        ${KEYRECORD_DEBUG_TEST_TARGETS}
        ${target_name}
        PARENT_SCOPE
    )
endfunction()

add_library(keyrecord_capture STATIC src/key_names.cpp src/key_event_writer.cpp src/config_path.cpp)
target_include_directories(keyrecord_capture PUBLIC ${KEYRECORD_SRC_DIR})
target_link_libraries(keyrecord_capture PUBLIC unofficial::sqlite3::sqlite3)

add_custom_command(
    OUTPUT
        "${KEYRECORD_GENERATED_RESOURCES_CPP}"
        "${KEYRECORD_GENERATED_RESOURCES_H}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${KEYRECORD_GENERATED_DIR}"
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/visualize/public
        -DOUTPUT_CPP=${KEYRECORD_GENERATED_RESOURCES_CPP}
        -DOUTPUT_H=${KEYRECORD_GENERATED_RESOURCES_H}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateEmbeddedResources.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateEmbeddedResources.cmake"
        ${KEYRECORD_STATIC_RESOURCE_FILES}
    VERBATIM
)

add_library(keyrecord_visualization_core STATIC src/keyboard_layout.cpp src/key_classification.cpp src/api_queries.cpp src/readonly_database.cpp)
target_include_directories(keyrecord_visualization_core PUBLIC ${KEYRECORD_SRC_DIR})
target_link_libraries(keyrecord_visualization_core PUBLIC unofficial::sqlite3::sqlite3)

add_library(keyrecord_resources STATIC src/embedded_resources.cpp "${KEYRECORD_GENERATED_RESOURCES_CPP}")
target_include_directories(keyrecord_resources PUBLIC ${KEYRECORD_SRC_DIR} PRIVATE "${KEYRECORD_GENERATED_DIR}")

add_library(keyrecord_http STATIC src/http_router.cpp)
target_include_directories(keyrecord_http PUBLIC ${KEYRECORD_SRC_DIR})
target_link_libraries(keyrecord_http PUBLIC keyrecord_visualization_core keyrecord_resources)

add_library(keyrecord_visualization_service STATIC
    src/visualization_service.cpp
    src/server_bootstrap.cpp
    src/server_startup.cpp
    src/config_path.cpp
)
target_include_directories(keyrecord_visualization_service PUBLIC ${KEYRECORD_SRC_DIR})
target_link_libraries(keyrecord_visualization_service PUBLIC keyrecord_http)

set(KEYRECORD_HAS_SERVER_SUPPORT OFF)
if(KEYRECORD_BOOST_INCLUDE_DIR
    AND EXISTS "${KEYRECORD_BOOST_INCLUDE_DIR}/boost/asio.hpp"
    AND EXISTS "${KEYRECORD_BOOST_INCLUDE_DIR}/boost/beast.hpp")
    set(KEYRECORD_HAS_SERVER_SUPPORT ON)
endif()

if(KEYRECORD_HAS_SERVER_SUPPORT)
    add_executable(keyrecord_server
        src/server_main.cpp
        src/server.cpp
        src/server_response_adapter.cpp
    )
    target_include_directories(keyrecord_server PRIVATE
        ${KEYRECORD_SRC_DIR}
        ${KEYRECORD_BOOST_INCLUDE_DIR}
    )
    target_link_libraries(keyrecord_server PRIVATE keyrecord_visualization_service ws2_32)
    copy_sqlite_dll(keyrecord_server)
else()
    message(STATUS "Boost.Beast/Asio headers not found; keyrecord_server target is disabled. Set KEYRECORD_BOOST_INCLUDE_DIR to an existing Boost include root to enable it.")
endif()

add_executable(keyrecord WIN32
    src/main.cpp
    src/tray_app.cpp
    "${KEYRECORD_APP_RESOURCE_RC}"
)
target_link_libraries(keyrecord PRIVATE keyrecord_capture shell32)
copy_sqlite_dll(keyrecord)

set(KEYRECORD_RELEASE_PACKAGE_ENV
    BUILD_CONFIG=$<CONFIG>
    SOURCE_DIR=$<TARGET_FILE_DIR:keyrecord>
    TARGET_DIR=${CMAKE_BINARY_DIR}/release_package/$<CONFIG>
    OUTPUT_ZIP=${CMAKE_BINARY_DIR}/keyrecord-windows-x64.zip
    KEYRECORD_EXE_PATH=$<TARGET_FILE:keyrecord>
    SQLITE_DLL_PATH=$<TARGET_FILE:unofficial::sqlite3::sqlite3>
)
if(TARGET keyrecord_server)
    list(APPEND KEYRECORD_RELEASE_PACKAGE_ENV SERVER_EXE_PATH=$<TARGET_FILE:keyrecord_server>)
else()
    list(APPEND KEYRECORD_RELEASE_PACKAGE_ENV SERVER_EXE_PATH=)
endif()

# Release 构建后生成包含采集端、服务端和运行目录 dll 的 zip；兼容 Visual Studio 多配置生成器。
add_custom_target(keyrecord_release_package ALL
    COMMAND ${CMAKE_COMMAND} -E env
        ${KEYRECORD_RELEASE_PACKAGE_ENV}
        ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageRelease.cmake
    COMMENT "Creating release package keyrecord-windows-x64.zip"
    VERBATIM
)
add_dependencies(keyrecord_release_package keyrecord)
if(TARGET keyrecord_server)
    add_dependencies(keyrecord_release_package keyrecord_server)
endif()

enable_testing()

add_keyrecord_test(keyrecord_tests SOURCES src/tests/keyrecord_db_tests.cpp LINKS keyrecord_capture COPY_SQLITE_DLL)
add_keyrecord_test(config_path_tests SOURCES src/tests/config_path_tests.cpp src/config_path.cpp INCLUDES ${KEYRECORD_SRC_DIR})
add_keyrecord_test(embedded_resources_tests SOURCES src/tests/embedded_resources_tests.cpp LINKS keyrecord_resources)
add_keyrecord_test(api_queries_tests SOURCES src/tests/api_queries_tests.cpp LINKS keyrecord_visualization_core COPY_SQLITE_DLL)
add_keyrecord_test(keyboard_layout_tests SOURCES src/tests/keyboard_layout_tests.cpp LINKS keyrecord_visualization_core)
add_keyrecord_test(key_classification_tests SOURCES src/tests/key_classification_tests.cpp LINKS keyrecord_visualization_core)
add_keyrecord_test(http_router_tests SOURCES src/tests/http_router_tests.cpp LINKS keyrecord_http COPY_SQLITE_DLL)
add_keyrecord_test(visualization_service_tests SOURCES src/tests/visualization_service_tests.cpp LINKS keyrecord_visualization_service COPY_SQLITE_DLL)
add_keyrecord_test(server_bootstrap_tests SOURCES src/tests/server_bootstrap_tests.cpp LINKS keyrecord_visualization_service COPY_SQLITE_DLL)
add_keyrecord_test(server_startup_tests SOURCES src/tests/server_startup_tests.cpp LINKS keyrecord_visualization_service COPY_SQLITE_DLL)

if(KEYRECORD_HAS_SERVER_SUPPORT)
    add_keyrecord_test(server_response_adapter_tests SOURCES src/tests/server_response_adapter_tests.cpp src/server_response_adapter.cpp INCLUDES ${KEYRECORD_SRC_DIR} ${KEYRECORD_BOOST_INCLUDE_DIR})
endif()

add_keyrecord_test(readonly_database_tests SOURCES src/tests/readonly_database_tests.cpp LINKS keyrecord_visualization_core COPY_SQLITE_DLL)

set(KEYRECORD_UTF8_TARGETS
    keyrecord_capture
    keyrecord_visualization_core
    keyrecord_resources
    keyrecord_http
    keyrecord_visualization_service
    keyrecord
    ${KEYRECORD_DEBUG_TEST_TARGETS}
)
if(KEYRECORD_HAS_SERVER_SUPPORT)
    list(APPEND KEYRECORD_UTF8_TARGETS keyrecord_server)
endif()
foreach(target_name IN LISTS KEYRECORD_UTF8_TARGETS)
    enable_msvc_utf8(${target_name})
endforeach()

add_custom_target(keyrecord_debug_tests)
add_dependencies(keyrecord_debug_tests ${KEYRECORD_DEBUG_TEST_TARGETS})
