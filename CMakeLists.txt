cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

project(keyrecord)

set(CMAKE_CXX_STANDARD 20)

if(EXISTS "C:/vcpkg/installed/x64-windows")
    list(APPEND CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows")
endif()

find_package(unofficial-sqlite3 CONFIG REQUIRED)

set(KEYRECORD_BOOST_HINT_DIRS "C:/vcpkg/installed/x64-windows/include")

find_path(KEYRECORD_BOOST_INCLUDE_DIR
    NAMES boost/beast.hpp
    HINTS ${KEYRECORD_BOOST_HINT_DIRS}
    DOC "Directory that contains the Boost headers root (the folder that contains boost/beast.hpp)"
)

add_definitions(-DUNICODE -D_UNICODE)

set(KEYRECORD_CAPTURE_SOURCES
    src/key_names.cpp
    src/key_event_writer.cpp
    src/config_path.cpp
)

set(KEYRECORD_VISUALIZATION_CORE_SOURCES
    src/keyboard_layout.cpp
    src/api_queries.cpp
    src/readonly_database.cpp
)

file(GLOB_RECURSE KEYRECORD_STATIC_RESOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/visualize/public/*"
)
set(KEYRECORD_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(KEYRECORD_GENERATED_RESOURCES_CPP "${KEYRECORD_GENERATED_DIR}/generated_resources.cpp")
set(KEYRECORD_GENERATED_RESOURCES_H "${KEYRECORD_GENERATED_DIR}/generated_resources.h")

function(copy_sqlite_dll target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:unofficial::sqlite3::sqlite3>
            $<TARGET_FILE_DIR:${target_name}>
    )
endfunction()

add_library(keyrecord_capture STATIC ${KEYRECORD_CAPTURE_SOURCES})
target_include_directories(keyrecord_capture PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(keyrecord_capture PUBLIC unofficial::sqlite3::sqlite3)
if(MSVC)
    target_compile_options(keyrecord_capture PRIVATE /utf-8)
endif()

add_library(keyrecord_visualization_core STATIC ${KEYRECORD_VISUALIZATION_CORE_SOURCES})
target_include_directories(keyrecord_visualization_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(keyrecord_visualization_core PUBLIC unofficial::sqlite3::sqlite3)
if(MSVC)
    target_compile_options(keyrecord_visualization_core PRIVATE /utf-8)
endif()

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

add_library(keyrecord_resources STATIC
    src/embedded_resources.cpp
    "${KEYRECORD_GENERATED_RESOURCES_CPP}"
)
target_include_directories(keyrecord_resources
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE "${KEYRECORD_GENERATED_DIR}"
)
if(MSVC)
    target_compile_options(keyrecord_resources PRIVATE /utf-8)
endif()

add_library(keyrecord_http STATIC
    src/http_router.cpp
)
target_include_directories(keyrecord_http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(keyrecord_http PUBLIC keyrecord_visualization_core keyrecord_resources)
if(MSVC)
    target_compile_options(keyrecord_http PRIVATE /utf-8)
endif()

add_library(keyrecord_visualization_service STATIC
    src/visualization_service.cpp
    src/server_bootstrap.cpp
    src/server_startup.cpp
    src/config_path.cpp
)
target_include_directories(keyrecord_visualization_service PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(keyrecord_visualization_service PUBLIC keyrecord_http)
if(MSVC)
    target_compile_options(keyrecord_visualization_service PRIVATE /utf-8)
endif()

if(KEYRECORD_BOOST_INCLUDE_DIR AND EXISTS "${KEYRECORD_BOOST_INCLUDE_DIR}/boost/asio.hpp")
    add_executable(keyrecord_server
        src/server_main.cpp
        src/server.cpp
        src/server_response_adapter.cpp
    )
    target_include_directories(keyrecord_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${KEYRECORD_BOOST_INCLUDE_DIR}
    )
    target_link_libraries(keyrecord_server PRIVATE keyrecord_visualization_service ws2_32)
    if(MSVC)
        target_compile_options(keyrecord_server PRIVATE /utf-8)
    endif()
    copy_sqlite_dll(keyrecord_server)
else()
    message(STATUS "Boost.Beast/Asio headers not found; keyrecord_server target is disabled. Set KEYRECORD_BOOST_INCLUDE_DIR to an existing Boost include root to enable it.")
endif()

add_executable(keyrecord WIN32
    src/main.cpp
    src/tray_app.cpp
)
target_link_libraries(keyrecord PRIVATE keyrecord_capture shell32)
if(MSVC)
    target_compile_options(keyrecord PRIVATE /utf-8)
endif()
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

add_executable(keyrecord_tests src/tests/keyrecord_db_tests.cpp)
target_link_libraries(keyrecord_tests PRIVATE keyrecord_capture)
if(MSVC)
    target_compile_options(keyrecord_tests PRIVATE /utf-8)
endif()
copy_sqlite_dll(keyrecord_tests)
add_test(NAME keyrecord_tests COMMAND keyrecord_tests)

add_executable(config_path_tests src/tests/config_path_tests.cpp src/config_path.cpp)
target_include_directories(config_path_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(MSVC)
    target_compile_options(config_path_tests PRIVATE /utf-8)
endif()
add_test(NAME config_path_tests COMMAND config_path_tests)

add_executable(embedded_resources_tests src/tests/embedded_resources_tests.cpp)
target_link_libraries(embedded_resources_tests PRIVATE keyrecord_resources)
if(MSVC)
    target_compile_options(embedded_resources_tests PRIVATE /utf-8)
endif()
add_test(NAME embedded_resources_tests COMMAND embedded_resources_tests)

add_executable(api_queries_tests src/tests/api_queries_tests.cpp)
target_link_libraries(api_queries_tests PRIVATE keyrecord_visualization_core)
if(MSVC)
    target_compile_options(api_queries_tests PRIVATE /utf-8)
endif()
copy_sqlite_dll(api_queries_tests)
add_test(NAME api_queries_tests COMMAND api_queries_tests)

add_executable(keyboard_layout_tests src/tests/keyboard_layout_tests.cpp)
target_link_libraries(keyboard_layout_tests PRIVATE keyrecord_visualization_core)
if(MSVC)
    target_compile_options(keyboard_layout_tests PRIVATE /utf-8)
endif()
add_test(NAME keyboard_layout_tests COMMAND keyboard_layout_tests)

add_executable(http_router_tests src/tests/http_router_tests.cpp)
target_link_libraries(http_router_tests PRIVATE keyrecord_http)
if(MSVC)
    target_compile_options(http_router_tests PRIVATE /utf-8)
endif()
copy_sqlite_dll(http_router_tests)
add_test(NAME http_router_tests COMMAND http_router_tests)

add_executable(visualization_service_tests src/tests/visualization_service_tests.cpp)
target_link_libraries(visualization_service_tests PRIVATE keyrecord_visualization_service)
if(MSVC)
    target_compile_options(visualization_service_tests PRIVATE /utf-8)
endif()
copy_sqlite_dll(visualization_service_tests)
add_test(NAME visualization_service_tests COMMAND visualization_service_tests)

add_executable(server_bootstrap_tests src/tests/server_bootstrap_tests.cpp)
target_link_libraries(server_bootstrap_tests PRIVATE keyrecord_visualization_service)
if(MSVC)
    target_compile_options(server_bootstrap_tests PRIVATE /utf-8)
endif()
copy_sqlite_dll(server_bootstrap_tests)
add_test(NAME server_bootstrap_tests COMMAND server_bootstrap_tests)

add_executable(server_startup_tests src/tests/server_startup_tests.cpp)
target_link_libraries(server_startup_tests PRIVATE keyrecord_visualization_service)
if(MSVC)
    target_compile_options(server_startup_tests PRIVATE /utf-8)
endif()
copy_sqlite_dll(server_startup_tests)
add_test(NAME server_startup_tests COMMAND server_startup_tests)

if(KEYRECORD_BOOST_INCLUDE_DIR AND EXISTS "${KEYRECORD_BOOST_INCLUDE_DIR}/boost/beast.hpp")
    add_executable(server_response_adapter_tests
        src/tests/server_response_adapter_tests.cpp
        src/server_response_adapter.cpp
    )
    target_include_directories(server_response_adapter_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${KEYRECORD_BOOST_INCLUDE_DIR}
    )
    if(MSVC)
        target_compile_options(server_response_adapter_tests PRIVATE /utf-8)
    endif()
    add_test(NAME server_response_adapter_tests COMMAND server_response_adapter_tests)
endif()

add_executable(readonly_database_tests src/tests/readonly_database_tests.cpp)
target_link_libraries(readonly_database_tests PRIVATE keyrecord_visualization_core)
if(MSVC)
    target_compile_options(readonly_database_tests PRIVATE /utf-8)
endif()
copy_sqlite_dll(readonly_database_tests)
add_test(NAME readonly_database_tests COMMAND readonly_database_tests)
