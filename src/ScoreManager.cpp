#include "ScoreManager.h"
#include <fstream>
#include <iostream>

ScoreManager::ScoreManager(const std::string& filePath)
    : highScore_(0), filePath_(filePath)
{
    loadFromFile();
}

void ScoreManager::loadFromFile() {
    std::ifstream file(filePath_);
    if (file.is_open()) {
        file >> highScore_;
    }
}

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
        std::cerr << "[ScoreManager] Warning: could not write to " << filePath_ << "\n";
    }
}

int ScoreManager::getHighScore() const { return highScore_; }