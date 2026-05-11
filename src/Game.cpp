

#include "Game.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>   // rand()


#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
static struct termios g_originalTermios;
#endif

// ─────────────────────────────────────────────────────────────────────────────
// initTerminal() / restoreTerminal()
// On Linux we switch the terminal to "raw" mode so keystrokes are delivered
// immediately without waiting for Enter, and without local echo.
// On Windows, _kbhit() / _getch() already do this for us.
// ─────────────────────────────────────────────────────────────────────────────
void Game::initTerminal() {
#ifndef _WIN32
    tcgetattr(STDIN_FILENO, &g_originalTermios);
    struct termios raw = g_originalTermios;
    raw.c_lflag     &= ~(ECHO | ICANON);  // no echo, no line buffering
    raw.c_cc[VMIN]   = 0;                 // return immediately if no key
    raw.c_cc[VTIME]  = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
#endif
}

void Game::restoreTerminal() {
#ifndef _WIN32
    tcsetattr(STDIN_FILENO, TCSANOW, &g_originalTermios);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// getKey() — non-blocking; returns 0 if no key is waiting.
// On Windows, arrow keys send two bytes: 0 or 224, then the scan code.
// We consume both bytes and map the scan code to our Direction chars.
// ─────────────────────────────────────────────────────────────────────────────
char Game::getKey() {
#ifdef _WIN32
    if (_kbhit()) {
        char key = static_cast<char>(_getch());
        if (key == 0 || key == '\xe0') {   // arrow-key prefix
            char arrow = static_cast<char>(_getch());
            switch (arrow) {
                case 72: return 'w';  // Up
                case 80: return 's';  // Down
                case 75: return 'a';  // Left
                case 77: return 'd';  // Right
            }
        }
        return key;
    }
#else
    char key = 0;
    if (read(STDIN_FILENO, &key, 1) == 1) return key;
#endif
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
Game::Game()
    : snake_(BOARD_WIDTH / 2, BOARD_HEIGHT / 2),
      state_(GameState::RUNNING),
      score_(0), level_(1), speed_(200)
{
    initTerminal();
    // Initial board snapshot so food can find empty cells
    board_.update(snake_, food_);
    food_.spawn(board_.getGrid());
}

Game::~Game() {
    restoreTerminal();
}

// ─────────────────────────────────────────────────────────────────────────────
// speedForLevel() — frame delay in milliseconds.
// Level 1 = 200 ms/frame ≈ 5 fps.  Each level reduces by 20 ms.  Floor = 50 ms.
// ─────────────────────────────────────────────────────────────────────────────
int Game::speedForLevel(int lvl) const {
    return std::max(50, 200 - (lvl - 1) * 20);
}

// ─────────────────────────────────────────────────────────────────────────────
// handleInput() — reads one key per frame and mutates state accordingly.
// ─────────────────────────────────────────────────────────────────────────────
void Game::handleInput() {
    char key = getKey();
    if (key == 0) return;

    switch (key) {
        case 'w': case 'W': snake_.setDirection(Direction::UP);    break;
        case 's': case 'S': snake_.setDirection(Direction::DOWN);  break;
        case 'a': case 'A': snake_.setDirection(Direction::LEFT);  break;
        case 'd': case 'D': snake_.setDirection(Direction::RIGHT); break;

        case 'p': case 'P':
            state_ = (state_ == GameState::PAUSED)
                     ? GameState::RUNNING
                     : GameState::PAUSED;
            break;

        case 'q': case 'Q':
            state_ = GameState::QUIT;
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// checkCollisions() — called after snake_.move() each tick.
// ─────────────────────────────────────────────────────────────────────────────
void Game::checkCollisions() {
    auto [hx, hy] = snake_.getHead();

    // 1. Wall collision — O(1)
    if (board_.isWall(hx, hy)) {
        state_ = GameState::GAME_OVER;
        return;
    }

    // 2. Obstacle collision — O(1)
    if (board_.isObstacle(hx, hy)) {
        state_ = GameState::GAME_OVER;
        return;
    }

    // 3. Self collision — O(n)
    if (snake_.checkSelfCollision()) {
        state_ = GameState::GAME_OVER;
        return;
    }

    // 4. Food collision — O(1)
    if (snake_.getHead() == food_.getPosition()) {
        snake_.grow();
        score_ += 10 * level_;
        scoreManager_.update(score_);

        // Level up every 5 foods eaten (50 * level_ points threshold)
        int newLevel = (score_ / (50 * level_)) + 1;
        if (newLevel > level_) {
            level_ = newLevel;
            speed_ = speedForLevel(level_);
            spawnObstaclesForLevel(level_);
        }

        // Update grid so food cell is now SNAKE_HEAD, then spawn new food
        board_.update(snake_, food_);
        food_.spawn(board_.getGrid());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// spawnObstaclesForLevel() — adds a few '+' tiles at level 3 and beyond.
// ─────────────────────────────────────────────────────────────────────────────
void Game::spawnObstaclesForLevel(int lvl) {
    if (lvl < 3) return;
    int count = (lvl - 2) * 2;   // 2 extra obstacles per level above 2
    for (int i = 0; i < count; ++i) {
        int rx = 1 + rand() % (BOARD_WIDTH  - 2);
        int ry = 1 + rand() % (BOARD_HEIGHT - 2);
        board_.addObstacle(rx, ry);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// update() — one game tick.
// ─────────────────────────────────────────────────────────────────────────────
void Game::update() {
    snake_.move();
    checkCollisions();
    board_.update(snake_, food_);
}

// ─────────────────────────────────────────────────────────────────────────────
// resetGame() — restores initial state for a new game.
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// showGameOver() — blocks until R (restart) or Q (quit).
// ─────────────────────────────────────────────────────────────────────────────
void Game::showGameOver() {
    board_.render(score_, level_, scoreManager_.getHighScore());
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════╗\n";
    std::cout << "  ║         GAME  OVER           ║\n";
    std::cout << "  ║  Final Score: " << std::to_string(score_);
    // pad to align box
    int pad = 14 - static_cast<int>(std::to_string(score_).size());
    for (int i = 0; i < pad; ++i) std::cout << ' ';
    std::cout << "║\n";
    std::cout << "  ║  Press R to restart          ║\n";
    std::cout << "  ║  Press Q to quit             ║\n";
    std::cout << "  ╚══════════════════════════════╝\n";
    std::cout.flush();
}

// ─────────────────────────────────────────────────────────────────────────────
// run() — the main game loop.
// Typical frame: read input → advance state → render → sleep(speed_ ms).
// ─────────────────────────────────────────────────────────────────────────────
void Game::run() {
    while (state_ != GameState::QUIT) {
        handleInput();

        if (state_ == GameState::RUNNING) {
            update();
            board_.render(score_, level_, scoreManager_.getHighScore());

        } else if (state_ == GameState::PAUSED) {
            // Re-render with PAUSED banner
            board_.render(score_, level_, scoreManager_.getHighScore());
            std::cout << "\n  *** PAUSED — Press P to resume or Q to quit ***\n";
            std::cout.flush();

        } else if (state_ == GameState::GAME_OVER) {
            showGameOver();
            continue;   // skip sleep so restart is instant
        }

        // Frame rate control — sleep for 'speed_' milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(speed_));
    }

    std::cout << "\nGame saved.  High Score: " << scoreManager_.getHighScore() << "\n";
    std::cout << "Thanks for playing!\n";
}
