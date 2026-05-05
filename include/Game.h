#pragma once
// =============================================================================
// Game.h — Declares the Game class (the main engine / coordinator).
//
// Game owns all subsystems and drives the game loop:
//   while (running) { handleInput() → update() → render() → sleep() }
//
// It is intentionally simple so every team member can trace the flow.
// =============================================================================

#include "Snake.h"
#include "Food.h"
#include "Board.h"
#include "ScoreManager.h"

enum class GameState { RUNNING, PAUSED, GAME_OVER, QUIT };

class Game {
public:
    Game();
    ~Game();

    // Enters the game loop; returns when the player quits.
    void run();

private:
    // ── Subsystems ────────────────────────────────────────────────────────────
    Snake        snake_;
    Food         food_;
    Board        board_;
    ScoreManager scoreManager_;

    // ── State ─────────────────────────────────────────────────────────────────
    GameState state_;
    int       score_;
    int       level_;
    int       speed_;    // milliseconds per frame (lower = faster)

    // ── Private helpers ───────────────────────────────────────────────────────
    void handleInput();
    void update();
    void checkCollisions();
    void spawnObstaclesForLevel(int lvl);
    void resetGame();
    void showGameOver();

    // Returns the frame delay in ms for the given level (min 50 ms).
    int speedForLevel(int lvl) const;

    // Cross-platform non-blocking key read; returns 0 if no key pressed.
    char getKey();

    // Platform-specific terminal setup / teardown (raw-mode on Linux).
    void initTerminal();
    void restoreTerminal();
};
