cmake_minimum_required(VERSION 3.20)
project(SnakeGame VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "D:/Programs/Qt/6.11.0/mingw_64")

find_package(Qt6 COMPONENTS Widgets REQUIRED)

set(SOURCES
        src/main.cpp
        src/Snake.cpp
        src/Food.cpp
        src/Board.cpp
        src/ScoreManager.cpp
        src/Game.cpp
        include/MainWindow.h
        src/MainWindow.cpp
)

add_executable(SnakeGame ${SOURCES})
target_include_directories(SnakeGame PRIVATE include)
target_link_libraries(SnakeGame PRIVATE Qt6::Widgets)

if (MSVC)
    target_compile_options(SnakeGame PRIVATE /W4)
else()
    target_compile_options(SnakeGame PRIVATE -Wall -Wextra -pedantic)
endif()

add_custom_command(TARGET SnakeGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:SnakeGame>/data
)

if(WIN32)
    add_custom_command(TARGET SnakeGame POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E env PATH="D:/Programs/Qt/6.11.0/mingw_64/bin"
            windeployqt --no-compiler-runtime $<TARGET_FILE:SnakeGame>
    )
endif()