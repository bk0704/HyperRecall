cmake_minimum_required(VERSION 3.21)

project(HyperRecall VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HYPERRECALL_ENABLE_DEVTOOLS "Enable developer tooling and diagnostics features" ON)
option(HYPERRECALL_USE_SYSTEM_RAYGUI "Use a system-provided raygui header instead of the bundled one" OFF)
set(HYPERRECALL_UI_BACKEND "RAYLIB" CACHE STRING "UI backend selection (RAYLIB or QT6)")
set(HYPERRECALL_RAYGUI_HEADER "${CMAKE_SOURCE_DIR}/external/raygui/raygui.h" CACHE FILEPATH "Path to raygui.h when bundling the single-header library")

# Validate UI backend option
set_property(CACHE HYPERRECALL_UI_BACKEND PROPERTY STRINGS "RAYLIB" "QT6")
if(NOT HYPERRECALL_UI_BACKEND STREQUAL "RAYLIB" AND NOT HYPERRECALL_UI_BACKEND STREQUAL "QT6")
    message(FATAL_ERROR "Invalid HYPERRECALL_UI_BACKEND: '${HYPERRECALL_UI_BACKEND}'. Must be 'RAYLIB' or 'QT6'.")
endif()

# Set convenience flag
if(HYPERRECALL_UI_BACKEND STREQUAL "QT6")
    set(HYPERRECALL_USE_QT6 ON)
else()
    set(HYPERRECALL_USE_QT6 OFF)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "Link-time optimization requested but not supported: ${ipo_output}")
    endif()
endif()

# Locate dependencies based on selected backend
if(HYPERRECALL_USE_QT6)
    message(STATUS "Building with Qt6 UI backend")
    find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)
else()
    message(STATUS "Building with Raylib UI backend")
    # Locate raylib 5.x
    set(_raylib_hints)
    if(DEFINED RAYLIB_ROOT)
        list(APPEND _raylib_hints "${RAYLIB_ROOT}")
    elseif(DEFINED ENV{RAYLIB_ROOT})
        list(APPEND _raylib_hints "$ENV{RAYLIB_ROOT}")
    endif()

    find_package(raylib 5 QUIET CONFIG)
    if(TARGET raylib::raylib)
        set(HYPERRECALL_RAYLIB_TARGET raylib::raylib)
    else()
        find_path(RAYLIB_INCLUDE_DIR raylib.h
            HINTS ${_raylib_hints}
            PATH_SUFFIXES include include/raylib)
        find_library(RAYLIB_LIBRARY NAMES raylib
            HINTS ${_raylib_hints}
            PATH_SUFFIXES lib lib64 lib/x64)
        if(NOT RAYLIB_INCLUDE_DIR OR NOT RAYLIB_LIBRARY)
            message(FATAL_ERROR "raylib 5.x not found. Install raylib or set RAYLIB_ROOT/CMAKE_PREFIX_PATH to the installation directory.")
        endif()
    endif()
endif()

# Locate SQLite3
find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    set(HYPERRECALL_SQLITE_INCLUDE_DIRS ${SQLite3_INCLUDE_DIRS})
    set(HYPERRECALL_SQLITE_LIBRARIES ${SQLite3_LIBRARIES})
else()
    set(_sqlite_hints)
    if(DEFINED SQLITE3_ROOT)
        list(APPEND _sqlite_hints "${SQLITE3_ROOT}")
    elseif(DEFINED ENV{SQLITE3_ROOT})
        list(APPEND _sqlite_hints "$ENV{SQLITE3_ROOT}")
    endif()
    find_path(SQLITE3_INCLUDE_DIR sqlite3.h
        HINTS ${_sqlite_hints}
        PATH_SUFFIXES include include/sqlite3)
    find_library(SQLITE3_LIBRARY NAMES sqlite3
        HINTS ${_sqlite_hints}
        PATH_SUFFIXES lib lib64 lib/x64)
    if(NOT SQLITE3_INCLUDE_DIR OR NOT SQLITE3_LIBRARY)
        message(FATAL_ERROR "SQLite3 development files not found. Install sqlite3 or set SQLITE3_ROOT/CMAKE_PREFIX_PATH accordingly.")
    endif()
    set(HYPERRECALL_SQLITE_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
    set(HYPERRECALL_SQLITE_LIBRARIES ${SQLITE3_LIBRARY})
endif()

# Handle raygui header integration (only needed for RAYLIB backend)
if(NOT HYPERRECALL_USE_QT6)
    if(HYPERRECALL_USE_SYSTEM_RAYGUI)
        set(_raygui_hints)
        if(DEFINED RAYGUI_ROOT)
            list(APPEND _raygui_hints "${RAYGUI_ROOT}")
        elseif(DEFINED ENV{RAYGUI_ROOT})
            list(APPEND _raygui_hints "$ENV{RAYGUI_ROOT}")
        endif()
        find_path(RAYGUI_INCLUDE_DIR raygui.h
            HINTS ${_raygui_hints}
            PATH_SUFFIXES include include/raygui)
        if(NOT RAYGUI_INCLUDE_DIR)
            message(FATAL_ERROR "System raygui requested but raygui.h was not found. Install raygui or set RAYGUI_ROOT/CMAKE_PREFIX_PATH to its location.")
        endif()
    else()
        if(NOT EXISTS "${HYPERRECALL_RAYGUI_HEADER}")
            message(FATAL_ERROR "Bundled raygui header not found at ${HYPERRECALL_RAYGUI_HEADER}. Provide the header or enable HYPERRECALL_USE_SYSTEM_RAYGUI.")
        endif()
        get_filename_component(RAYGUI_INCLUDE_DIR "${HYPERRECALL_RAYGUI_HEADER}" DIRECTORY)
    endif()
endif()

# Core C sources (common to both backends)
set(HYPERRECALL_CORE_SOURCES
    src/app.c
    src/theme.c
    src/db.c
    src/model.c
    src/srs.c
    src/sessions.c
    src/import_export.c
    src/media.c
    src/render.c
    src/cfg.c
    src/analytics.c
    src/json.c)

set(HYPERRECALL_CORE_HEADERS
    src/app.h
    src/theme.h
    src/db.h
    src/model.h
    src/srs.h
    src/sessions.h
    src/import_export.h
    src/media.h
    src/render.h
    src/cfg.h
    src/analytics.h
    src/json.h)

# Backend-specific sources
if(HYPERRECALL_USE_QT6)
    set(HYPERRECALL_BACKEND_SOURCES
        src/main_qt.cpp
        src/qt/qt_platform.cpp
        src/qt/qt_ui.cpp
        src/qt/main_window.cpp)
    set(HYPERRECALL_BACKEND_HEADERS
        src/platform.h
        src/ui.h
        src/qt/qt_platform.h
        src/qt/qt_ui.h
        src/qt/main_window.h)
else()
    set(HYPERRECALL_BACKEND_SOURCES
        src/main.c
        src/platform.c
        src/ui.c)
    set(HYPERRECALL_BACKEND_HEADERS
        src/platform.h
        src/ui.h)
endif()

set(HYPERRECALL_SOURCES ${HYPERRECALL_CORE_SOURCES} ${HYPERRECALL_BACKEND_SOURCES})
set(HYPERRECALL_HEADERS ${HYPERRECALL_CORE_HEADERS} ${HYPERRECALL_BACKEND_HEADERS})

add_executable(hyperrecall ${HYPERRECALL_SOURCES} ${HYPERRECALL_HEADERS})
set_target_properties(hyperrecall PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Backend-specific linking and includes
if(HYPERRECALL_USE_QT6)
    target_link_libraries(hyperrecall PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core)
    target_compile_definitions(hyperrecall PRIVATE HYPERRECALL_UI_QT6=1)
else()
    if(TARGET raylib::raylib)
        target_link_libraries(hyperrecall PRIVATE raylib::raylib)
        get_target_property(_raylib_includes raylib::raylib INTERFACE_INCLUDE_DIRECTORIES)
        if(_raylib_includes)
            target_include_directories(hyperrecall PRIVATE ${_raylib_includes})
        endif()
    else()
        target_include_directories(hyperrecall PRIVATE ${RAYLIB_INCLUDE_DIR})
        target_link_libraries(hyperrecall PRIVATE ${RAYLIB_LIBRARY})
    endif()
    target_include_directories(hyperrecall PRIVATE ${RAYGUI_INCLUDE_DIR})
endif()

target_include_directories(hyperrecall PRIVATE
    src
    ${HYPERRECALL_SQLITE_INCLUDE_DIRS})

target_link_libraries(hyperrecall PRIVATE ${HYPERRECALL_SQLITE_LIBRARIES})

if(WIN32 AND NOT HYPERRECALL_USE_QT6 AND NOT TARGET raylib::raylib)
    target_link_libraries(hyperrecall PRIVATE opengl32 gdi32 winmm)
endif()

if(UNIX AND NOT APPLE AND NOT HYPERRECALL_USE_QT6 AND NOT TARGET raylib::raylib)
    find_package(Threads REQUIRED)
    target_link_libraries(hyperrecall PRIVATE Threads::Threads m)
    if(CMAKE_DL_LIBS)
        target_link_libraries(hyperrecall PRIVATE ${CMAKE_DL_LIBS})
    endif()
    find_library(RT_LIBRARY rt)
    if(RT_LIBRARY)
        target_link_libraries(hyperrecall PRIVATE ${RT_LIBRARY})
    endif()
endif()

if(WIN32)
    target_compile_definitions(hyperrecall PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_compile_definitions(hyperrecall PRIVATE
    $<$<BOOL:${HYPERRECALL_ENABLE_DEVTOOLS}>:HYPERRECALL_ENABLE_DEVTOOLS=1>
    $<$<NOT:$<BOOL:${HYPERRECALL_ENABLE_DEVTOOLS}>>:HYPERRECALL_ENABLE_DEVTOOLS=0>
    $<$<BOOL:${HYPERRECALL_USE_SYSTEM_RAYGUI}>:HYPERRECALL_USE_SYSTEM_RAYGUI=1>)

if(MSVC)
    target_compile_options(hyperrecall PRIVATE /W4 /WX)
else()
    target_compile_options(hyperrecall PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

set(HYPERRECALL_ASSETS_DIR ${CMAKE_SOURCE_DIR}/assets)
add_custom_command(TARGET hyperrecall POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${HYPERRECALL_ASSETS_DIR}
            $<TARGET_FILE_DIR:hyperrecall>/assets
    COMMENT "Copying assets directory"
)

add_custom_target(run
    COMMAND $<TARGET_FILE:hyperrecall>
    WORKING_DIRECTORY $<TARGET_FILE_DIR:hyperrecall>
    DEPENDS hyperrecall
    COMMENT "Launching HyperRecall"
)
