#pragma once
// =============================================================================
// Food.h — Declares the Food class.
//
// Food holds a single Position and knows how to re-spawn itself on a Board
// grid by choosing a random EMPTY cell.
//
// DATA STRUCTURE: std::vector<Position> used internally for candidate cells.
// Building the candidate list is O(W*H); random selection is O(1).
// =============================================================================

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
