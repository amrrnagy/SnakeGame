// =============================================================================
// Board.cpp — Implements the Board class (2-D grid + console renderer).
// =============================================================================

#include "Board.h"
#include <iostream>
#include <iomanip>

Board::Board()
    : grid_(BOARD_HEIGHT, std::vector<Cell>(BOARD_WIDTH, Cell::EMPTY))
{
    initWalls();
}
/*
void Board::initWalls() {
    for (int x = 0; x < BOARD_WIDTH; ++x) {
        grid_[0][x]               = Cell::WALL;
        grid_[BOARD_HEIGHT - 1][x] = Cell::WALL;
    }
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        grid_[y][0]               = Cell::WALL;
        grid_[y][BOARD_WIDTH - 1] = Cell::WALL;
    }
}*/

void Board::resetInner() {
    for (int y = 1; y < BOARD_HEIGHT - 1; ++y)
        for (int x = 1; x < BOARD_WIDTH - 1; ++x)
            if (grid_[y][x] != Cell::OBSTACLE)
                grid_[y][x] = Cell::EMPTY;
}

void Board::resetObstacles()
{
    for (int y = 1; y < BOARD_HEIGHT - 1; ++y)
    {
        for (int x = 1; x < BOARD_WIDTH - 1; ++x)
        {
            grid_[y][x] = Cell::EMPTY;
        }
    }
}

void Board::update(const Snake& snake, const Food& food) {
    resetInner();

    auto [fx, fy] = food.getPosition();
    if (fx > 0 && fy > 0) {
        grid_[fy][fx] = Cell::FOOD;
    }

    const auto& body = snake.getBody();
    for (int i = static_cast<int>(body.size()) - 1; i >= 0; --i) {
        auto [sx, sy] = body[i];
        if (sy >= 0 && sy < BOARD_HEIGHT && sx >= 0 && sx < BOARD_WIDTH) {
            grid_[sy][sx] = (i == 0) ? Cell::SNAKE_HEAD : Cell::SNAKE_BODY;
        }
    }
}

bool Board::isWall(int x, int y) const {
    if (y < 0 || y >= BOARD_HEIGHT || x < 0 || x >= BOARD_WIDTH) return true;
    return grid_[y][x] == Cell::WALL;
}

bool Board::isObstacle(int x, int y) const {
    if (y < 0 || y >= BOARD_HEIGHT || x < 0 || x >= BOARD_WIDTH) return false;
    return grid_[y][x] == Cell::OBSTACLE;
}

void Board::addObstacle(int x, int y) {
    if (y > 0 && y < BOARD_HEIGHT - 1 && x > 0 && x < BOARD_WIDTH - 1) {
        grid_[y][x] = Cell::OBSTACLE;
    }
}

const std::vector<std::vector<Cell>>& Board::getGrid() const {
    return grid_;
}
