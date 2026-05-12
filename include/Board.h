#pragma once

#include <vector>
#include "Common.h"
#include "Snake.h"
#include "Food.h"

class Board {
public:
    Board();
    void update(const Snake& snake, const Food& food);
    void render(int score, int level, int highScore) const;
    bool isWall(int x, int y) const;
    bool isObstacle(int x, int y) const;
    void addObstacle(int x, int y);
    const std::vector<std::vector<Cell>>& getGrid() const;
    void resetInner();
    void resetObstacles();

private:
    std::vector<std::vector<Cell>> grid_;
    void initWalls();
};