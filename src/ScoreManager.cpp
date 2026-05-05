// =============================================================================
// ScoreManager.cpp — Implements persistent high score via file I/O.
// =============================================================================

#include "ScoreManager.h"
#include <fstream>
#include <iostream>

ScoreManager::ScoreManager(const std::string& filePath)
    : highScore_(0), filePath_(filePath)
{
    loadFromFile();
}

// ─────────────────────────────────────────────────────────────────────────────
// loadFromFile() — reads the single integer stored in the high-score file.
// If the file does not exist (first run), highScore_ stays 0.
// ─────────────────────────────────────────────────────────────────────────────
void ScoreManager::loadFromFile() {
    std::ifstream file(filePath_);
    if (file.is_open()) {
        file >> highScore_;
    }
    // file closes automatically via RAII (destructor)
}

// ─────────────────────────────────────────────────────────────────────────────
// update() — compares current score to stored record and persists if better.
// ─────────────────────────────────────────────────────────────────────────────
void ScoreManager::update(int currentScore) {
    if (currentScore > highScore_) {
        highScore_ = currentScore;
        saveToFile();
    }
}

void ScoreManager::saveToFile() const {
    std::ofstream file(filePath_);
    if (file.is_open()) {
        file << highScore_;
    } else {
        // Non-fatal: high score simply won't persist this session
        std::cerr << "[ScoreManager] Warning: could not write to " << filePath_ << "\n";
    }
}

int ScoreManager::getHighScore() const { return highScore_; }
