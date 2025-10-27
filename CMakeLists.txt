cmake_minimum_required(VERSION 3.21)

project(HyperRecall VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HYPERRECALL_ENABLE_DEVTOOLS "Enable developer tooling and diagnostics features" ON)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "Link-time optimization requested but not supported: ${ipo_output}")
    endif()
endif()

# Locate Qt6 dependencies
message(STATUS "Building with Qt6 UI backend")
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)

# Enable Qt's automoc for processing Q_OBJECT macros
set(CMAKE_AUTOMOC ON)

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

# Core C sources
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
    src/types.h
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

# Qt6 GUI sources
set(HYPERRECALL_BACKEND_SOURCES
    src/main_qt.cpp
    src/qt/qt_platform.cpp
    src/qt/qt_ui.cpp
    src/qt/main_window.cpp
    src/qt/study_screen.cpp
    src/qt/analytics_screen.cpp
    src/qt/library_screen.cpp)
set(HYPERRECALL_BACKEND_HEADERS
    src/platform.h
    src/ui.h
    src/qt/qt_platform.h
    src/qt/qt_ui.h
    src/qt/main_window.h
    src/qt/study_screen.h
    src/qt/analytics_screen.h
    src/qt/library_screen.h)

set(HYPERRECALL_SOURCES ${HYPERRECALL_CORE_SOURCES} ${HYPERRECALL_BACKEND_SOURCES})
set(HYPERRECALL_HEADERS ${HYPERRECALL_CORE_HEADERS} ${HYPERRECALL_BACKEND_HEADERS})

add_executable(hyperrecall ${HYPERRECALL_SOURCES} ${HYPERRECALL_HEADERS})
set_target_properties(hyperrecall PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Qt6 linking and compile definitions
target_link_libraries(hyperrecall PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core)
target_compile_definitions(hyperrecall PRIVATE HYPERRECALL_UI_QT6=1)

target_include_directories(hyperrecall PRIVATE
    src
    ${HYPERRECALL_SQLITE_INCLUDE_DIRS})

target_link_libraries(hyperrecall PRIVATE ${HYPERRECALL_SQLITE_LIBRARIES})

if(WIN32)
    target_compile_definitions(hyperrecall PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_compile_definitions(hyperrecall PRIVATE
    $<$<BOOL:${HYPERRECALL_ENABLE_DEVTOOLS}>:HYPERRECALL_ENABLE_DEVTOOLS=1>
    $<$<NOT:$<BOOL:${HYPERRECALL_ENABLE_DEVTOOLS}>>:HYPERRECALL_ENABLE_DEVTOOLS=0>)

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
