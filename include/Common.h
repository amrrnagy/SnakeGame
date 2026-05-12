#pragma once

#include <utility>

const int BOARD_WIDTH  = 30;
const int BOARD_HEIGHT = 20;

enum class Direction { UP, DOWN, LEFT, RIGHT };

enum class Cell {
    EMPTY,
    WALL,
    SNAKE_HEAD,
    SNAKE_BODY,
    FOOD,
    OBSTACLE
};

enum class GameState { MENU, RUNNING, PAUSED, GAME_OVER, QUIT };

using Position = std::pair<int, int>;