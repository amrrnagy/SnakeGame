// =============================================================================
// Food.cpp — Implements the Food class.
// =============================================================================

#include "Food.h"
#include <cstdlib>   // rand()

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
Food::Food() : pos_({ -1, -1 }) {}

// ─────────────────────────────────────────────────────────────────────────────
// spawn() — picks a random EMPTY cell from the current board grid.
//
// Algorithm:
//   1. Iterate through inner cells (skip walls: y in [1, H-2], x in [1, W-2]).
//   2. Collect all EMPTY cells into a vector<Position> — O(W*H).
//   3. Pick one at random using rand() % size — O(1).
//
// Using a candidate list (instead of looping until we find an empty spot)
// avoids potential infinite loops when the board is nearly full.
// ─────────────────────────────────────────────────────────────────────────────
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
    // If no EMPTY cells exist (board full), food simply isn't placed.
}

Position Food::getPosition() const { return pos_; }
