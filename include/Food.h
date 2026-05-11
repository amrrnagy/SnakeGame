#pragma once
#include <vector>
#include "Common.h"

class Food {
public:
    Food();

    // Scans the provided grid, collects all EMPTY cells, picks one at random.
    void spawn(const std::vector<std::vector<Cell>>& grid);

    Position getPosition() const;

private:
    Position pos_;  // (-1,-1) means not yet spawned
};
