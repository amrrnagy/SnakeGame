#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QEvent>
#include <QResizeEvent>
#include <QColor>
#include <QString>

#include "Snake.h"
#include "Food.h"
#include "Board.h"
#include "ScoreManager.h"
#include "Game.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ─── Particle ────────────────────────────────────────────────────────────────
struct Particle {
    float x, y, vx, vy, life;
    QColor color;
};

// ─── Snake Skin ───────────────────────────────────────────────────────────────
struct SnakeSkin {
    QString name;
    QColor  headColor;
    QColor  bodyColor;
    QColor  glowColor;
};

// ─── Difficulty ───────────────────────────────────────────────────────────────
enum class Difficulty { EASY, NORMAL, HARD };

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void gameTick();
    void bonusFoodExpire();

private:
    Ui::MainWindow *ui;
    QTimer *timer_;
    QTimer *bonusTimer_;

    bool inputRegisteredThisTick_ = false;

    // ── Core subsystems ───────────────────────────────────────────────────────
    Snake        snake_;
    Food         food_;
    Board        board_;
    ScoreManager scoreManager_;

    // ── Game state ────────────────────────────────────────────────────────────
    GameState  state_;
    int        score_, level_, speed_;

    // ── Progress ──────────────────────────────────────────────────────────────
    int  foodEaten_;
    int  totalFoodEaten_;
    static constexpr int FOOD_PER_LEVEL = 5;

    // ── Speed boost ───────────────────────────────────────────────────────────
    bool shiftHeld_;
    bool boosting_;
    static constexpr int BOOST_SPEED = 60;

    // ── Bonus fruit ───────────────────────────────────────────────────────────
    bool     bonusActive_;
    Position bonusPos_;
    int      bonusTimeLeft_;
    static constexpr int BONUS_LIFETIME     = 80;
    static constexpr int BONUS_POINTS_MULT  = 3;

    // ── Shield ────────────────────────────────────────────────────────────────
    bool     shieldActive_;
    bool     shieldPickupOnBoard_;
    Position shieldPos_;
    int      shieldFlashTicks_;

    // ── 🎨 Skin selector ──────────────────────────────────────────────────────
    // All available skins defined in cpp
    std::vector<SnakeSkin> skins_;
    int  selectedSkin_;        // index into skins_
    // Menu navigation
    int  menuSkinScroll_;      // index of skin being previewed in menu

    // ── 🌡️ Difficulty ─────────────────────────────────────────────────────────
    Difficulty difficulty_;
    int        selectedDiff_;  // 0=Easy 1=Normal 2=Hard (menu cursor)

    // Difficulty tuning helpers
    int  baseSpeed()       const;   // starting ms/tick
    int  bombCount(int lvl)const;   // obstacles per level-up
    bool shieldAllowed()   const;   // Hard disables shield drops
    bool wrapAllowed()     const;   // Hard disables wall wrap

    // ── Visual ────────────────────────────────────────────────────────────────
    std::vector<Particle> particles_;
    int  animFrame_;
    bool newHighScore_;

    // ── Menu state ────────────────────────────────────────────────────────────
    // Menu has two sub-pages: skin select and diff select
    enum class MenuPage { MAIN, SKIN, DIFF };
    MenuPage menuPage_;

    // ── Paint ─────────────────────────────────────────────────────────────────
    void paintBoard(QPainter &p, int boardW, int boardH);
    void paintMenuMain  (QPainter &p, float bw, float bh);
    void paintMenuSkin  (QPainter &p, float bw, float bh);
    void paintMenuDiff  (QPainter &p, float bw, float bh);

    void drawCell(QPainter &p, float cx, float cy, float cw, float ch,
                  const QColor &col, bool rounded = true, float glowR = 0.f);
    void drawGlow(QPainter &p, float cx, float cy, float r,
                  const QColor &col, int alpha);
    void drawBonusFruit (QPainter &p, float cx, float cy, float cw, float ch);
    void drawShieldOrb  (QPainter &p, float cx, float cy, float cw, float ch);
    void drawShieldAura (QPainter &p, float hx, float hy, float cw, float ch);
    void drawSnakePreview(QPainter &p, const SnakeSkin &skin,
                          float cx, float cy, float size);

    // ── Particles ─────────────────────────────────────────────────────────────
    void spawnParticles(float px, float py, const QColor &col, int n = 12);
    void updateParticles();

    // ── Logic ─────────────────────────────────────────────────────────────────
    void checkCollisions();
    void spawnObstaclesForLevel(int lvl);
    void maybeSpawnBonusFruit();
    void maybeSpawnShield();
    void wrapPosition();
    void resetGame();
    int  speedForLevel(int lvl) const;
    void updateHUD();
    void applyDifficulty();   // called at game start to set speed_ etc.
};

#endif // MAINWINDOW_H