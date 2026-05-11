#pragma once
#include <utility>   // std::pair


const int BOARD_WIDTH  = 30;
const int BOARD_HEIGHT = 20;

enum class Direction { UP, DOWN, LEFT, RIGHT };

enum class Cell {
    EMPTY,
    WALL,
    SNAKE_HEAD,
    SNAKE_BODY,
    FOOD,
    OBSTACLE   // used by level 3+
};


// Position = (x, y) where x is column, y is row
using Position = std::pair<int, int>;
