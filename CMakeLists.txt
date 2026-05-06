cmake_minimum_required(VERSION 3.20)
project(SnakeGame VERSION 1.0 LANGUAGES CXX)

# ── C++17 required (structured bindings, std::filesystem, etc.) ──────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Instructs CMake to automatically handle Qt's Meta-Object Compiler and UI compiler
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Tell CMake EXACTLY where to look for Qt (replace with your actual path)
set(CMAKE_PREFIX_PATH "C:/Qt/6.11.0/mingw_64")

# Find the Qt Framework
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# ── Source files ─────────────────────────────────────────────────────────────
set(SOURCES
        src/main.cpp
        src/Snake.cpp
        src/Food.cpp
        src/Board.cpp
        src/ScoreManager.cpp
        src/Game.cpp
        include/MainWindow.h
        src/MainWindow.cpp
        src/MainWindow.ui    # Added the UI file here
)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(SnakeGame ${SOURCES})

# ── Include path (so #include "Snake.h" works without a path prefix) ─────────
target_include_directories(SnakeGame PRIVATE include)

# ── Link Qt Libraries ────────────────────────────────────────────────────────
# This links the required Qt widgets to your game executable
target_link_libraries(SnakeGame PRIVATE Qt6::Widgets)

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