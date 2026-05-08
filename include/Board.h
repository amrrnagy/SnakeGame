#pragma once
// =============================================================================
// Board.h — Declares the Board class.
//
// DATA STRUCTURE CHOICE: 2-D vector<vector<Cell>>
// ─────────────────────────────────────────────────────────────────────────────
//  The Board stores the game state as a grid.
//  grid_[row][col] = Cell enum value.
//  - O(1) read/write for any cell — direct index access.
//  - O(W*H) to clear and redraw each frame (W=30, H=20 → 600 ops, negligible).
//  - Simplifies collision detection: just check grid_[y][x] == Cell::WALL.
//  - Alternative (flat 1-D array) offers the same complexity with more
//    index arithmetic; 2-D vector is clearer for students.
// =============================================================================

#include <vector>
#include <string>
#include "Common.h"
#include "Snake.h"
#include "Food.h"

class Board {
public:
    Board();

    // Rebuilds the grid from the current Snake and Food state.
    // Called every game tick before render(). O(W*H + snake_length).
    void update(const Snake& snake, const Food& food);

    // Prints the board to stdout with HUD (score, level, high score).
    void render(int score, int level, int highScore) const;

    // O(1) wall check — used by Game for collision detection.
    bool isWall(int x, int y) const;
    bool isObstacle(int x, int y) const;

    // Grants read access to the grid for Food::spawn().
    const std::vector<std::vector<Cell>>& getGrid() const;

    // Adds a static obstacle tile (used by levels 3+).
    void addObstacle(int x, int y);

    // Resets all inner (non-wall) cells and removes obstacles.
    void resetInner();

    void resetObstacles();
private:
    std::vector<std::vector<Cell>> grid_;  // grid_[y][x]
    void initWalls();
};
