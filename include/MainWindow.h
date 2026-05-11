#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QEvent>
#include <QResizeEvent>

#include "Snake.h"
#include "Food.h"
#include "Board.h"
#include "ScoreManager.h"
#include "Game.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ─── Particle for food-eaten / shield-break burst effects ────────────────────
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
    void keyReleaseEvent(QKeyEvent *event) override;          // for Shift release
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void gameTick();
    void bonusFoodExpire();    // called when bonus food timer runs out

private:
    Ui::MainWindow *ui;
    QTimer *timer_;
    QTimer *bonusTimer_;       // counts down bonus fruit lifespan
private:
    bool inputRegisteredThisTick_ = false;

    // ── Core subsystems ───────────────────────────────────────────────────────
    Snake        snake_;
    Food         food_;
    Board        board_;
    ScoreManager scoreManager_;

    // ── Game state ────────────────────────────────────────────────────────────
    GameState state_;
    int  score_, level_, speed_;

    // ── Progress tracking ─────────────────────────────────────────────────────
    int  foodEaten_;          // resets each level
    int  totalFoodEaten_;
    static constexpr int FOOD_PER_LEVEL = 5;

    // ── Feature: Wall wrap-around ─────────────────────────────────────────────
    // No extra state needed — handled in wrapPosition()

    // ── Feature: Speed boost (hold Shift) ────────────────────────────────────
    bool shiftHeld_;           // true while Shift is pressed
    bool boosting_;            // true during an active boost tick
    static constexpr int BOOST_SPEED = 60;   // ms per tick while boosting

    // ── Feature: Bonus fruit ─────────────────────────────────────────────────
    bool        bonusActive_;      // is a bonus fruit on the board?
    Position    bonusPos_;         // where the bonus fruit is
    int         bonusTimeLeft_;    // ticks remaining (shown as countdown ring)
    static constexpr int BONUS_LIFETIME = 80;   // ticks (~5 s at 200 ms)
    static constexpr int BONUS_POINTS_MULT = 3; // worth 3× normal food

    // ── Feature: Shield power-up ─────────────────────────────────────────────
    bool     shieldActive_;    // snake currently has a shield
    bool     shieldPickupOnBoard_; // shield orb is placed on the grid
    Position shieldPos_;
    int      shieldFlashTicks_;    // brief flash when shield absorbs a hit

    // ── Visual ────────────────────────────────────────────────────────────────
    std::vector<Particle> particles_;
    int  animFrame_;
    bool newHighScore_;

    // ── Paint ─────────────────────────────────────────────────────────────────
    void paintBoard(QPainter &p, int boardW, int boardH);
    void drawCell(QPainter &p, float cx, float cy, float cw, float ch,
                  const QColor &col, bool rounded = true, float glowR = 0.f);
    void drawGlow(QPainter &p, float cx, float cy, float r,
                  const QColor &col, int alpha);
    void drawBonusFruit(QPainter &p, float cx, float cy, float cw, float ch);
    void drawShieldOrb(QPainter &p, float cx, float cy, float cw, float ch);
    void drawShieldAura(QPainter &p, float hx, float hy, float cw, float ch);

    // ── Particles ─────────────────────────────────────────────────────────────
    void spawnParticles(float px, float py, const QColor &col, int n = 12);
    void updateParticles();

    // ── Logic helpers ─────────────────────────────────────────────────────────
    void checkCollisions();
    void spawnObstaclesForLevel(int lvl);
    void maybeSpawnBonusFruit();
    void maybeSpawnShield();
    void wrapPosition();        // teleport head if it crossed a wall
    void resetGame();
    int  speedForLevel(int lvl) const;
    void updateHUD();
};

#endif // MAINWINDOW_H