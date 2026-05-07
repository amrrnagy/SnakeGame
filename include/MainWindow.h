#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QEvent>

#include "Snake.h"
#include "Food.h"
#include "Board.h"
#include "ScoreManager.h"
#include "Game.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct Particle {
    float x, y, vx, vy, life;
    QColor color;
};

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
    Ui::MainWindow *ui;
    QTimer         *timer_;

    Snake        snake_;
    Food         food_;
    Board        board_;
    ScoreManager scoreManager_;

    GameState state_;
    int       score_;
    int       level_;
    int       speed_;

    std::vector<Particle> particles_;
    int  animFrame_;
    bool newHighScore_;
    int  foodEaten_;          // apples eaten this level (resets on level-up)
    int  totalFoodEaten_;     // all-time count (never resets)

    static constexpr int FOOD_PER_LEVEL = 5;

    void paintBoard(QPainter &painter, int boardW, int boardH);
    void drawCell(QPainter &p, float cx, float cy, float cw, float ch,
                  const QColor &color, bool rounded = true, float glowRadius = 0.0f);
    void drawGlow(QPainter &p, float cx, float cy, float radius,
                  const QColor &color, int alpha);
    void spawnParticles(float px, float py, const QColor &color, int count = 12);
    void updateParticles();

    void checkCollisions();
    void spawnObstaclesForLevel(int lvl);
    void resetGame();
    int  speedForLevel(int lvl) const;
    void updateHUD();
};

#endif // MAINWINDOW_H