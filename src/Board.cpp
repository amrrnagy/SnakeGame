// =============================================================================
// Board.cpp — Implements the Board class (2-D grid + console renderer).
// =============================================================================

#include "Board.h"
#include <iostream>
#include <iomanip>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor — initialises a BOARD_HEIGHT × BOARD_WIDTH grid of EMPTY cells,
// then stamps the border walls.
// ─────────────────────────────────────────────────────────────────────────────
Board::Board()
    : grid_(BOARD_HEIGHT, std::vector<Cell>(BOARD_WIDTH, Cell::EMPTY))
{
    initWalls();
}

void Board::initWalls() {
    // Top and bottom rows
    for (int x = 0; x < BOARD_WIDTH; ++x) {
        grid_[0][x]               = Cell::WALL;
        grid_[BOARD_HEIGHT - 1][x] = Cell::WALL;
    }
    // Left and right columns
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        grid_[y][0]               = Cell::WALL;
        grid_[y][BOARD_WIDTH - 1] = Cell::WALL;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// resetInner() — clears all non-wall, non-obstacle inner cells back to EMPTY.
// Called at the start of each update() so we draw a fresh frame.
// O(W * H).
// ─────────────────────────────────────────────────────────────────────────────
void Board::resetInner() {
    for (int y = 1; y < BOARD_HEIGHT - 1; ++y)
        for (int x = 1; x < BOARD_WIDTH - 1; ++x)
            if (grid_[y][x] != Cell::OBSTACLE)
                grid_[y][x] = Cell::EMPTY;
}

// ─────────────────────────────────────────────────────────────────────────────
// update() — rebuilds the grid from current game-object positions.
//
// Order matters: snake is drawn last so its head/body overwrites the food
// cell if head and food overlap (used by food spawn logic).
// ─────────────────────────────────────────────────────────────────────────────
void Board::update(const Snake& snake, const Food& food) {
    resetInner();

    // Place food
    auto [fx, fy] = food.getPosition();
    if (fx > 0 && fy > 0) {
        grid_[fy][fx] = Cell::FOOD;
    }

    // Place snake body first, then head (head overwrites if on same cell)
    const auto& body = snake.getBody();
    for (int i = static_cast<int>(body.size()) - 1; i >= 0; --i) {
        auto [sx, sy] = body[i];
        if (sy >= 0 && sy < BOARD_HEIGHT && sx >= 0 && sx < BOARD_WIDTH) {
            grid_[sy][sx] = (i == 0) ? Cell::SNAKE_HEAD : Cell::SNAKE_BODY;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// render() — outputs the board to the console with a simple HUD.
//
// Characters used:
//   '#'  wall       '+'  obstacle
//   'O'  head       'o'  body
//   '*'  food       ' '  empty
// ─────────────────────────────────────────────────────────────────────────────
void Board::render(int score, int level, int highScore) const {
    // Clear screen — simple approach suitable for academic projects
#ifdef _WIN32
    system("cls");
#else
    // ANSI escape: move cursor to (0,0) and clear screen
    std::cout << "\033[H\033[2J";
#endif

    // HUD
    std::cout << " Score: " << std::setw(5) << score
              << "   Level: " << level
              << "   High Score: " << highScore
              << "\n";

    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            switch (grid_[y][x]) {
                case Cell::WALL:       std::cout << '#'; break;
                case Cell::SNAKE_HEAD: std::cout << 'O'; break;
                case Cell::SNAKE_BODY: std::cout << 'o'; break;
                case Cell::FOOD:       std::cout << '*'; break;
                case Cell::OBSTACLE:   std::cout << '+'; break;
                default:               std::cout << ' '; break;
            }
        }
        std::cout << '\n';
    }

    std::cout << " Controls: W/A/S/D  |  P = Pause  |  Q = Quit\n";
    std::cout.flush();
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
