#pragma once
// =============================================================================
// ScoreManager.h — Declares the ScoreManager class.
//
// Handles current score tracking and persistent high score via a plain text
// file ("data/highscore.txt"). Demonstrates file I/O with std::fstream.
// =============================================================================

#include <string>

class ScoreManager {
public:
    explicit ScoreManager(const std::string& filePath = "data/highscore.txt");

    // If currentScore beats the stored high score, updates and saves the file.
    void update(int currentScore);

    int getHighScore() const;
    void saveToFile() const;

private:
    int         highScore_;
    std::string filePath_;

    void loadFromFile();
};
