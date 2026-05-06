#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QKeyEvent>
#include <QPaintEvent>

// Include your game logic headers
#include "Snake.h"
#include "Food.h"
#include "Board.h"
#include "ScoreManager.h"
#include "Game.h" // We include this just to use your GameState enum

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // Replaces Game::handleInput()
    void keyPressEvent(QKeyEvent *event) override;

    // Replaces Board::render() - draws the game to the UI
    void paintEvent(QPaintEvent *event) override;

private slots:
    // Replaces the inner logic of Game::run() - called by QTimer
    void gameTick();

private:
    Ui::MainWindow *ui;
    QTimer *timer_;

    // Core Game Subsystems
    Snake snake_;
    Food food_;
    Board board_;
    ScoreManager scoreManager_;

    // State Variables
    GameState state_;
    int score_;
    int level_;
    int speed_;

    // Private helpers adapted from your Game class
    void checkCollisions();
    void spawnObstaclesForLevel(int lvl);
    void resetGame();
    int speedForLevel(int lvl) const;
    void updateLabels();
};

#endif // MAINWINDOW_H