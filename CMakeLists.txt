cmake_minimum_required(VERSION 3.20)

# vcpkg 工具链：优先显式传入的 CMAKE_TOOLCHAIN_FILE，其次 VCPKG_ROOT 环境变量，
# 最后才回退到默认 Windows 安装路径；避免在 Linux/macOS 上误用 Windows 路径。
if(NOT CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    elseif(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    endif()
endif()

project(keyrecord)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(KEYRECORD_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(EXISTS "C:/vcpkg/installed/x64-windows")
    list(APPEND CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows")
endif()

# 目标平台三元组命名，用于发布包命名（keyrecord-<os>-<arch>）。
if(WIN32)
    set(KEYRECORD_OS_NAME "windows")
elseif(APPLE)
    set(KEYRECORD_OS_NAME "darwin")
else()
    set(KEYRECORD_OS_NAME "linux")
endif()
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" KEYRECORD_ARCH_RAW)
if(KEYRECORD_ARCH_RAW MATCHES "arm64|aarch64")
    set(KEYRECORD_ARCH_NAME "arm64")
elseif(KEYRECORD_ARCH_RAW MATCHES "x86_64|amd64|x64")
    set(KEYRECORD_ARCH_NAME "x64")
elseif(KEYRECORD_ARCH_RAW STREQUAL "")
    set(KEYRECORD_ARCH_NAME "x64")
else()
    set(KEYRECORD_ARCH_NAME "${KEYRECORD_ARCH_RAW}")
endif()
set(KEYRECORD_PACKAGE_BASENAME "keyrecord-${KEYRECORD_OS_NAME}-${KEYRECORD_ARCH_NAME}")

# SQLite：优先 vcpkg 的 unofficial-sqlite3；缺失时回退到 CMake 内置 FindSQLite3，
# 便于在 Linux/macOS 上直接使用系统包管理器（apt/brew）安装的 sqlite3。
find_package(unofficial-sqlite3 CONFIG QUIET)
if(TARGET unofficial::sqlite3::sqlite3)
    set(KEYRECORD_SQLITE_TARGET unofficial::sqlite3::sqlite3)
    set(KEYRECORD_SQLITE_FROM_VCPKG ON)
else()
    find_package(SQLite3 REQUIRED)
    set(KEYRECORD_SQLITE_TARGET SQLite::SQLite3)
    set(KEYRECORD_SQLITE_FROM_VCPKG OFF)
endif()

find_package(Threads REQUIRED)

find_path(KEYRECORD_BOOST_INCLUDE_DIR
    NAMES boost/beast.hpp
    HINTS
        "C:/vcpkg/installed/x64-windows/include"
        "/opt/homebrew/include"
        "/usr/local/include"
        "/usr/include"
    DOC "Directory that contains the Boost headers root (the folder that contains boost/beast.hpp)"
)

if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

file(GLOB_RECURSE KEYRECORD_STATIC_RESOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/visualize/public/*"
)
set(KEYRECORD_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(KEYRECORD_GENERATED_RESOURCES_CPP "${KEYRECORD_GENERATED_DIR}/generated_resources.cpp")
set(KEYRECORD_GENERATED_RESOURCES_H "${KEYRECORD_GENERATED_DIR}/generated_resources.h")

# 应用图标与 Windows 资源脚本仅供 Windows 采集端使用。
set(KEYRECORD_APP_RESOURCE_RC "")
if(WIN32)
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
endif()

# 仅在使用 vcpkg 的 Windows 构建里需要把 sqlite3.dll 拷到可执行目录；
# 其它平台使用系统共享库或静态库，无需拷贝。
function(copy_sqlite_dll target_name)
    if(WIN32 AND KEYRECORD_SQLITE_FROM_VCPKG)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${KEYRECORD_SQLITE_TARGET}>
                $<TARGET_FILE_DIR:${target_name}>
        )
    endif()
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

add_library(keyrecord_capture STATIC src/key_names.cpp src/key_event_writer.cpp src/config_path.cpp src/app_config.cpp src/platform/platform_util.cpp)
target_include_directories(keyrecord_capture PUBLIC ${KEYRECORD_SRC_DIR})
target_link_libraries(keyrecord_capture PUBLIC ${KEYRECORD_SQLITE_TARGET} Threads::Threads)

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
target_link_libraries(keyrecord_visualization_core PUBLIC ${KEYRECORD_SQLITE_TARGET})

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
    src/app_config.cpp
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
    target_link_libraries(keyrecord_server PRIVATE keyrecord_visualization_service Threads::Threads)
    if(WIN32)
        # Windows 下 Boost.Asio 需要显式链接 Winsock。
        target_link_libraries(keyrecord_server PRIVATE ws2_32)
    endif()
    copy_sqlite_dll(keyrecord_server)
else()
    message(STATUS "Boost.Beast/Asio headers not found; keyrecord_server target is disabled. Set KEYRECORD_BOOST_INCLUDE_DIR to an existing Boost include root to enable it.")
endif()

# 采集端：当前仅 Windows 提供键盘 Hook + 托盘后端；Linux/macOS 采集后端在后续阶段加入。
if(WIN32)
    add_executable(keyrecord WIN32
        src/main.cpp
        src/tray_app.cpp
        "${KEYRECORD_APP_RESOURCE_RC}"
    )
    target_link_libraries(keyrecord PRIVATE keyrecord_capture shell32)
    copy_sqlite_dll(keyrecord)
endif()

# 发布包锚定目标：优先采集端，其次服务端。据此推导运行目录与依赖关系。
set(KEYRECORD_PACKAGE_ANCHOR "")
if(TARGET keyrecord)
    set(KEYRECORD_PACKAGE_ANCHOR keyrecord)
elseif(TARGET keyrecord_server)
    set(KEYRECORD_PACKAGE_ANCHOR keyrecord_server)
endif()

if(KEYRECORD_PACKAGE_ANCHOR)
    if(WIN32)
        set(KEYRECORD_PACKAGE_FILE "${CMAKE_BINARY_DIR}/${KEYRECORD_PACKAGE_BASENAME}.zip")
        set(KEYRECORD_ARCHIVE_FORMAT "zip")
    else()
        set(KEYRECORD_PACKAGE_FILE "${CMAKE_BINARY_DIR}/${KEYRECORD_PACKAGE_BASENAME}.tar.gz")
        set(KEYRECORD_ARCHIVE_FORMAT "gztar")
    endif()

    set(KEYRECORD_RELEASE_PACKAGE_ENV
        BUILD_CONFIG=$<CONFIG>
        SOURCE_DIR=$<TARGET_FILE_DIR:${KEYRECORD_PACKAGE_ANCHOR}>
        TARGET_DIR=${CMAKE_BINARY_DIR}/release_package/$<CONFIG>
        OUTPUT_ARCHIVE=${KEYRECORD_PACKAGE_FILE}
        ARCHIVE_FORMAT=${KEYRECORD_ARCHIVE_FORMAT}
        CONFIG_EXAMPLE_PATH=${CMAKE_CURRENT_SOURCE_DIR}/resources/config.example.ini
    )
    if(TARGET keyrecord)
        list(APPEND KEYRECORD_RELEASE_PACKAGE_ENV KEYRECORD_EXE_PATH=$<TARGET_FILE:keyrecord>)
    else()
        list(APPEND KEYRECORD_RELEASE_PACKAGE_ENV KEYRECORD_EXE_PATH=)
    endif()
    if(TARGET keyrecord_server)
        list(APPEND KEYRECORD_RELEASE_PACKAGE_ENV SERVER_EXE_PATH=$<TARGET_FILE:keyrecord_server>)
    else()
        list(APPEND KEYRECORD_RELEASE_PACKAGE_ENV SERVER_EXE_PATH=)
    endif()
    if(KEYRECORD_SQLITE_FROM_VCPKG)
        list(APPEND KEYRECORD_RELEASE_PACKAGE_ENV SQLITE_DLL_PATH=$<TARGET_FILE:${KEYRECORD_SQLITE_TARGET}>)
    else()
        list(APPEND KEYRECORD_RELEASE_PACKAGE_ENV SQLITE_DLL_PATH=)
    endif()

    # Release 构建后生成包含采集端/服务端及运行目录依赖的归档；兼容多配置生成器。
    add_custom_target(keyrecord_release_package ALL
        COMMAND ${CMAKE_COMMAND} -E env
            ${KEYRECORD_RELEASE_PACKAGE_ENV}
            ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageRelease.cmake
        COMMENT "Creating release package ${KEYRECORD_PACKAGE_BASENAME}"
        VERBATIM
    )
    add_dependencies(keyrecord_release_package ${KEYRECORD_PACKAGE_ANCHOR})
    if(TARGET keyrecord AND TARGET keyrecord_server)
        add_dependencies(keyrecord_release_package keyrecord_server)
    endif()
endif()

enable_testing()

add_keyrecord_test(keyrecord_tests SOURCES src/tests/keyrecord_db_tests.cpp LINKS keyrecord_capture COPY_SQLITE_DLL)
add_keyrecord_test(config_path_tests SOURCES src/tests/config_path_tests.cpp src/config_path.cpp INCLUDES ${KEYRECORD_SRC_DIR})
add_keyrecord_test(app_config_tests SOURCES src/tests/app_config_tests.cpp src/app_config.cpp src/config_path.cpp INCLUDES ${KEYRECORD_SRC_DIR})
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
add_keyrecord_test(virtual_keys_tests SOURCES src/tests/virtual_keys_tests.cpp INCLUDES ${KEYRECORD_SRC_DIR})
add_keyrecord_test(linux_keymap_tests SOURCES src/tests/linux_keymap_tests.cpp INCLUDES ${KEYRECORD_SRC_DIR})
add_keyrecord_test(macos_keymap_tests SOURCES src/tests/macos_keymap_tests.cpp INCLUDES ${KEYRECORD_SRC_DIR})

set(KEYRECORD_UTF8_TARGETS
    keyrecord_capture
    keyrecord_visualization_core
    keyrecord_resources
    keyrecord_http
    keyrecord_visualization_service
    ${KEYRECORD_DEBUG_TEST_TARGETS}
)
if(TARGET keyrecord)
    list(APPEND KEYRECORD_UTF8_TARGETS keyrecord)
endif()
if(KEYRECORD_HAS_SERVER_SUPPORT)
    list(APPEND KEYRECORD_UTF8_TARGETS keyrecord_server)
endif()
foreach(target_name IN LISTS KEYRECORD_UTF8_TARGETS)
    enable_msvc_utf8(${target_name})
endforeach()

add_custom_target(keyrecord_debug_tests)
add_dependencies(keyrecord_debug_tests ${KEYRECORD_DEBUG_TEST_TARGETS})
