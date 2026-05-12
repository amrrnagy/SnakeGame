#include "Game.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
    static struct termios g_originalTermios;
#endif

void Game::initTerminal() {
#ifndef _WIN32
    tcgetattr(STDIN_FILENO, &g_originalTermios);
    struct termios raw = g_originalTermios;
    raw.c_lflag     &= ~(ECHO | ICANON);
    raw.c_cc[VMIN]   = 0;
    raw.c_cc[VTIME]  = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
#endif
}

void Game::restoreTerminal() {
#ifndef _WIN32
    tcsetattr(STDIN_FILENO, TCSANOW, &g_originalTermios);
#endif
}

char Game::getKey() {
#ifdef _WIN32
    if (_kbhit()) {
        char key = static_cast<char>(_getch());
        if (key == 0 || key == '\xe0') {
            char arrow = static_cast<char>(_getch());
            switch (arrow) {
                case 72: return 'w';
                case 80: return 's';
                case 75: return 'a';
                case 77: return 'd';
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

Game::Game()
    : snake_(BOARD_WIDTH / 2, BOARD_HEIGHT / 2),
      state_(GameState::RUNNING),
      score_(0), level_(1), speed_(200), foodEaten_(0)
{
    initTerminal();
    board_.update(snake_, food_);
    food_.spawn(board_.getGrid());
}

Game::~Game() {
    restoreTerminal();
}

int Game::speedForLevel(int lvl) const {
    return std::max(50, 200 - (lvl - 1) * 15);
}

void Game::spawnObstaclesForLevel(int lvl) {
    if (lvl < 2) return;
    int count = (lvl - 1) * 2;
    auto [hx, hy] = snake_.getHead();

    for (int i = 0; i < count; ++i) {
        int rx, ry;
        do {
            rx = 1 + rand() % (BOARD_WIDTH - 2);
            ry = 1 + rand() % (BOARD_HEIGHT - 2);
        } while ((std::abs(rx - hx) < 3 && std::abs(ry - hy) < 3) || board_.getGrid()[ry][rx] != Cell::EMPTY);
        board_.addObstacle(rx, ry);
    }
}

void Game::handleInput() {
    char key = getKey();
    if (key == 0) return;

    switch (key) {
        case 'w': case 'W': snake_.setDirection(Direction::UP);    break;
        case 's': case 'S': snake_.setDirection(Direction::DOWN);  break;
        case 'a': case 'A': snake_.setDirection(Direction::LEFT);  break;
        case 'd': case 'D': snake_.setDirection(Direction::RIGHT); break;
        case 'p': case 'P':
            state_ = (state_ == GameState::PAUSED) ? GameState::RUNNING : GameState::PAUSED;
            break;
        case 'q': case 'Q':
            state_ = GameState::QUIT;
            break;
    }
}

void Game::checkCollisions() {
    auto [hx, hy] = snake_.getHead();

    if (board_.isWall(hx, hy) || board_.isObstacle(hx, hy) || snake_.checkSelfCollision()) {
        state_ = GameState::GAME_OVER;
        return;
    }

    if (snake_.getHead() == food_.getPosition()) {
        snake_.grow();
        score_ += 10 * level_;
        foodEaten_++;
        scoreManager_.update(score_);

        if (foodEaten_ >= 5) {
            foodEaten_ = 0;
            level_++;
            speed_ = speedForLevel(level_);
            spawnObstaclesForLevel(level_);
        }

        board_.update(snake_, food_);
        food_.spawn(board_.getGrid());
    }
}

void Game::update() {
    snake_.move();
    checkCollisions();
    if (state_ == GameState::RUNNING) {
        board_.update(snake_, food_);
    }
}

void Game::showGameOver() {
    board_.render(score_, level_, scoreManager_.getHighScore());
    std::cout << "\n  GAME OVER\n  Final Score: " << score_ << "\n";
    std::cout << "  Press Q to quit\n";
    std::cout.flush();
}

void Game::run() {
    while (state_ != GameState::QUIT) {
        handleInput();

        if (state_ == GameState::RUNNING) {
            update();
            if (state_ == GameState::RUNNING) {
                board_.render(score_, level_, scoreManager_.getHighScore());
            }
        } else if (state_ == GameState::PAUSED) {
            board_.render(score_, level_, scoreManager_.getHighScore());
            std::cout << "\n  *** PAUSED ***\n";
            std::cout.flush();
        } else if (state_ == GameState::GAME_OVER) {
            showGameOver();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(speed_));
    }
}