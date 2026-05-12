#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QEvent>

#include "Common.h"
#include "Snake.h"
#include "Food.h"
#include "Board.h"
#include "ScoreManager.h"

class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void gameTick();

private:
    QWidget *gameBoard_;
    QLabel  *scoreValueLabel_;
    QLabel  *levelValueLabel_;
    QLabel  *hsValueLabel_;
    QLabel  *progressValueLabel_;

    QTimer *timer_;
    bool inputRegisteredThisTick_ = false;

    Snake        snake_;
    Food         food_;
    Board        board_;
    ScoreManager scoreManager_;

    GameState  state_;
    int        score_, level_, speed_;
    int        foodEaten_;
    static constexpr int FOOD_PER_LEVEL = 5;

    void setupUI();
    void checkCollisions();
    void spawnObstaclesForLevel(int lvl);
    void resetGame();
    void updateHUD();
    int  speedForLevel(int lvl) const;
    void paintBoard(QPainter &p, int boardW, int boardH);
};

#endif // MAINWINDOW_H