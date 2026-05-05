#pragma once
// =============================================================================
// Common.h — Shared constants, types, and enums used across the entire project.
// Every other header includes this file so definitions stay in one place.
// =============================================================================

#include <utility>   // std::pair

// ── Board dimensions ─────────────────────────────────────────────────────────
const int BOARD_WIDTH  = 30;   // includes left/right wall columns
const int BOARD_HEIGHT = 20;   // includes top/bottom wall rows

// ── Direction enum ────────────────────────────────────────────────────────────
// Strongly-typed enum (enum class) avoids name collisions and integer confusion.
enum class Direction { UP, DOWN, LEFT, RIGHT };

// ── Cell states for the 2-D grid ─────────────────────────────────────────────
// The Board stores a 2-D vector<vector<Cell>> that records what occupies
// each tile. Collision detection reads directly from this grid — O(1) lookup.
enum class Cell {
    EMPTY,
    WALL,
    SNAKE_HEAD,
    SNAKE_BODY,
    FOOD,
    OBSTACLE   // used by level 3+
};

// ── Handy alias ───────────────────────────────────────────────────────────────
// Position = (x, y) where x is column, y is row (0-indexed, origin top-left).
using Position = std::pair<int, int>;
