#pragma once

#include "Common.h"
#include "Snake.h"
#include "Food.h"
#include "Board.h"
#include "ScoreManager.h"

class Game {
public:
    Game();
    ~Game();
    void run();

private:
    Snake        snake_;
    Food         food_;
    Board        board_;
    ScoreManager scoreManager_;

    GameState state_;
    int       score_;
    int       level_;
    int       speed_;
    int       foodEaten_;

    Position  bonusPos_;
    int       bonusTimeLeft_;

    Position  shieldPos_;
    bool      shieldActive_;

    void handleInput();
    void wrapPosition();
    void update();
    void checkCollisions();
    void spawnObstaclesForLevel(int lvl);
    void spawnBonus();
    void spawnShield();
    void showGameOver();
    int  speedForLevel(int lvl) const;
    char getKey();
    void initTerminal();
    void restoreTerminal();
};