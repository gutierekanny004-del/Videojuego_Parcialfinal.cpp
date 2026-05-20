cmake_minimum_required(VERSION 3.15)
project(DungeonCrawler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(dungeon ${SOURCES})
target_include_directories(dungeon PRIVATE include)

# ── ncurses / PDCurses (cross-platform) ──────────────────────────────────────
if(WIN32)
    # On Windows: expect PDCurses installed or bundled
    find_library(CURSES_LIB NAMES pdcurses curses REQUIRED)
    find_path(CURSES_INCLUDE curses.h REQUIRED)
    target_include_directories(dungeon PRIVATE ${CURSES_INCLUDE})
    target_link_libraries(dungeon PRIVATE ${CURSES_LIB})
else()
    find_package(Curses REQUIRED)
    target_include_directories(dungeon PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(dungeon PRIVATE ${CURSES_LIBRARIES})
    if(APPLE)
        # macOS ships ncurses but needs explicit link
        target_compile_definitions(dungeon PRIVATE _XOPEN_SOURCE_EXTENDED)
    endif()
endif()

# ── Build type defaults ───────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler   : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
