#include "Food.h"
#include <cstdlib>

Food::Food() : pos_({ -1, -1 }) {}

void Food::spawn(const std::vector<std::vector<Cell>>& grid) {
    std::vector<Position> candidates;
    candidates.reserve(BOARD_WIDTH * BOARD_HEIGHT);

    for (int y = 1; y < BOARD_HEIGHT - 1; ++y) {
        for (int x = 1; x < BOARD_WIDTH - 1; ++x) {
            if (grid[y][x] == Cell::EMPTY) {
                candidates.push_back({ x, y });
            }
        }
    }

    if (!candidates.empty()) {
        int idx = rand() % static_cast<int>(candidates.size());
        pos_ = candidates[idx];
    }
}

Position Food::getPosition() const { return pos_; }