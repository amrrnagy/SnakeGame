#pragma once

#include <string>

class ScoreManager {
public:
    explicit ScoreManager(const std::string& filePath = "data/highscore.txt");
    void update(int currentScore);
    int getHighScore() const;

private:
    int         highScore_;
    std::string filePath_;
    void loadFromFile();
    void saveToFile() const;
};