cmake_minimum_required(VERSION 3.20)
project(SnakeGame VERSION 1.0 LANGUAGES CXX)

# ── C++17 required (structured bindings, std::filesystem, etc.) ──────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Source files ─────────────────────────────────────────────────────────────
set(SOURCES
        src/main.cpp
        src/Snake.cpp
        src/Food.cpp
        src/Board.cpp
        src/ScoreManager.cpp
        src/Game.cpp
)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(SnakeGame ${SOURCES})

# ── Include path (so #include "Snake.h" works without a path prefix) ─────────
target_include_directories(SnakeGame PRIVATE include)

# ── Compiler warnings ────────────────────────────────────────────────────────
if (MSVC)
    target_compile_options(SnakeGame PRIVATE /W4)
else()
    target_compile_options(SnakeGame PRIVATE -Wall -Wextra -pedantic)
endif()

# ── Copy data/ folder next to the binary after every build ───────────────────
# This ensures highscore.txt is always reachable as "data/highscore.txt"
# relative to the working directory CLion uses when it runs the program.
add_custom_command(TARGET SnakeGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:SnakeGame>/data
        COMMENT "Copying data/ folder to build output..."
)
