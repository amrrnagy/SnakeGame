// =============================================================================
// MainWindow.cpp — Snake (Neon Arcade Edition)
//
// Changes in this revision:
//   1. HUD is now a single compact top bar (score | level | progress | best).
//   2. Progress label shows "🍎 X / 5" — apples eaten toward next level.
//      foodEaten_ resets to 0 every time a level-up occurs.
//   3. Bombs scale aggressively: count = level² (so level 3 = 9, 4 = 16,
//      5 = 25 …) giving a real challenge at higher levels.
// =============================================================================

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QPainter>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QFont>
#include <QApplication>
#include <QEvent>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// ─── Palette ──────────────────────────────────────────────────────────────────
static const QColor COL_BG       {  8,   8,  16 };
static const QColor COL_GRID     { 18,  18,  35 };
static const QColor COL_WALL     { 35,  35,  60 };
static const QColor COL_HEAD     { 57, 255,  20 };
static const QColor COL_BODY     { 34, 200,  10 };
static const QColor COL_FOOD     {255,  50,  80 };
static const QColor COL_OBSTACLE {220,  60,  20 };   // brighter orange-red
static const QColor COL_OVERLAY  {  0,   0,   0, 210 };

// =============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , timer_(new QTimer(this))
    , snake_(BOARD_WIDTH / 2, BOARD_HEIGHT / 2)
    , scoreManager_("data/highscore.txt")
    , animFrame_(0)
    , newHighScore_(false)
    , foodEaten_(0)
    , totalFoodEaten_(0)
{
    ui->setupUi(this);
    ui->gameBoard->installEventFilter(this);
    ui->gameBoard->setFocusPolicy(Qt::StrongFocus);

    connect(timer_, &QTimer::timeout, this, &MainWindow::gameTick);
    resetGame();
}

MainWindow::~MainWindow() { delete ui; }

// =============================================================================
// resetGame
// =============================================================================
void MainWindow::resetGame() {
    snake_          = Snake(BOARD_WIDTH / 2, BOARD_HEIGHT / 2);
    score_          = 0;
    level_          = 1;
    speed_          = 200;
    state_          = GameState::MENU;
    foodEaten_      = 0;
    totalFoodEaten_ = 0;
    newHighScore_   = false;
    particles_.clear();

    board_.resetInner();
    board_.update(snake_, food_);
    food_.spawn(board_.getGrid());

    updateHUD();
    timer_->stop();
    ui->gameBoard->update();
}

// =============================================================================
// HUD updater
// =============================================================================
void MainWindow::updateHUD() {
    ui->scoreValueLabel->setText(QString::number(score_));
    ui->levelValueLabel->setText(QString::number(level_));
    ui->hsValueLabel->setText(QString::number(scoreManager_.getHighScore()));
    ui->hsValueLabel->setStyleSheet(newHighScore_
        ? "color: #ff5500; margin-left: 6px;"
        : "color: #ffd700; margin-left: 6px;");

    // Apple progress toward next level
    ui->progressValueLabel->setText(
        QString("🍎 %1 / %2").arg(foodEaten_).arg(FOOD_PER_LEVEL));
}

// =============================================================================
// eventFilter — paints directly onto gameBoard widget
// =============================================================================
bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->gameBoard && event->type() == QEvent::Paint) {
        QPainter painter(ui->gameBoard);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        paintBoard(painter, ui->gameBoard->width(), ui->gameBoard->height());
        return true;
    }
    return QMainWindow::eventFilter(watched, event);
}

// =============================================================================
// gameTick
// =============================================================================
void MainWindow::gameTick() {
    if (state_ != GameState::RUNNING) return;

    animFrame_++;
    updateParticles();

    snake_.move();
    checkCollisions();

    if (state_ == GameState::RUNNING)
        board_.update(snake_, food_);

    ui->gameBoard->update();
}

// =============================================================================
// checkCollisions
// =============================================================================
void MainWindow::checkCollisions() {
    auto [hx, hy] = snake_.getHead();

    if (board_.isWall(hx, hy) || board_.isObstacle(hx, hy) || snake_.checkSelfCollision()) {
        state_ = GameState::GAME_OVER;
        timer_->stop();
        scoreManager_.update(score_);
        ui->gameBoard->update();
        return;
    }

    if (snake_.getHead() == food_.getPosition()) {
        // Particle burst
        float cw = static_cast<float>(ui->gameBoard->width())  / BOARD_WIDTH;
        float ch = static_cast<float>(ui->gameBoard->height()) / BOARD_HEIGHT;
        spawnParticles(hx * cw + cw / 2.f, hy * ch + ch / 2.f, COL_FOOD, 16);

        snake_.grow();
        score_         += 10 * level_;
        foodEaten_++;
        totalFoodEaten_++;

        if (score_ > scoreManager_.getHighScore()) newHighScore_ = true;
        scoreManager_.update(score_);

        // ── Level up every FOOD_PER_LEVEL apples ─────────────────────────────
        if (foodEaten_ >= FOOD_PER_LEVEL) {
            foodEaten_ = 0;      // reset progress for next level
            level_++;
            speed_ = speedForLevel(level_);
            timer_->setInterval(speed_);
            spawnObstaclesForLevel(level_);
        }

        board_.update(snake_, food_);
        food_.spawn(board_.getGrid());
        updateHUD();
    }
}

// =============================================================================
// spawnObstaclesForLevel
//
// Bomb count formula: level²
//   Level 1 → 0   (no bombs)
//   Level 2 → 0   (no bombs yet — grace period)
//   Level 3 → 9
//   Level 4 → 16
//   Level 5 → 25
//   Level 6 → 36
//   ...and so on — gets genuinely brutal fast.
// =============================================================================
void MainWindow::spawnObstaclesForLevel(int lvl) {
    if (lvl < 3) return;

    int count = lvl * lvl;   // quadratic scaling

    for (int i = 0; i < count; ++i) {
        // Avoid placing right on the snake's current head area
        int rx, ry;
        auto [hx, hy] = snake_.getHead();
        do {
            rx = 1 + rand() % (BOARD_WIDTH  - 2);
            ry = 1 + rand() % (BOARD_HEIGHT - 2);
        } while (std::abs(rx - hx) < 4 && std::abs(ry - hy) < 4);

        board_.addObstacle(rx, ry);
    }
}

// =============================================================================
// keyPressEvent
// =============================================================================
void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (state_ == GameState::MENU) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
            state_ = GameState::RUNNING;
            timer_->start(speed_);
            ui->gameBoard->update();
        }
        return;
    }

    switch (event->key()) {
        case Qt::Key_W: case Qt::Key_Up:    snake_.setDirection(Direction::UP);    break;
        case Qt::Key_S: case Qt::Key_Down:  snake_.setDirection(Direction::DOWN);  break;
        case Qt::Key_A: case Qt::Key_Left:  snake_.setDirection(Direction::LEFT);  break;
        case Qt::Key_D: case Qt::Key_Right: snake_.setDirection(Direction::RIGHT); break;

        case Qt::Key_P:
            if (state_ == GameState::RUNNING) {
                state_ = GameState::PAUSED;
                timer_->stop();
            } else if (state_ == GameState::PAUSED) {
                state_ = GameState::RUNNING;
                timer_->start(speed_);
            }
            ui->gameBoard->update();
            break;

        case Qt::Key_R:
            if (state_ == GameState::GAME_OVER || state_ == GameState::PAUSED)
                resetGame();
            break;

        case Qt::Key_Q:
        case Qt::Key_Escape:
            QApplication::quit();
            break;
    }
}

// =============================================================================
// Helpers
// =============================================================================
int MainWindow::speedForLevel(int lvl) const {
    return std::max(50, 200 - (lvl - 1) * 20);
}

void MainWindow::spawnParticles(float px, float py, const QColor &color, int count) {
    for (int i = 0; i < count; ++i) {
        Particle pt;
        pt.x    = px;  pt.y    = py;
        float a = (rand() % 360) * static_cast<float>(M_PI) / 180.f;
        float s = 1.5f + (rand() % 30) / 10.f;
        pt.vx   = std::cos(a) * s;
        pt.vy   = std::sin(a) * s;
        pt.life = 1.0f;
        pt.color= color;
        particles_.push_back(pt);
    }
}

void MainWindow::updateParticles() {
    for (auto &pt : particles_) {
        pt.x += pt.vx;  pt.y += pt.vy;
        pt.vy += 0.12f;
        pt.life -= 0.045f;
    }
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
                       [](const Particle &p){ return p.life <= 0.f; }),
        particles_.end());
}

void MainWindow::drawGlow(QPainter &p, float cx, float cy,
                           float r, const QColor &color, int alpha) {
    QRadialGradient g(cx, cy, r);
    QColor c0 = color; c0.setAlpha(alpha);
    QColor c1 = color; c1.setAlpha(0);
    g.setColorAt(0, c0);  g.setColorAt(1, c1);
    p.setBrush(g);  p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(cx - r, cy - r, r * 2, r * 2));
}

void MainWindow::drawCell(QPainter &p, float cx, float cy,
                           float cw, float ch, const QColor &color,
                           bool rounded, float glowRadius) {
    if (glowRadius > 0.f)
        drawGlow(p, cx + cw/2.f, cy + ch/2.f, glowRadius, color, 90);
    float m = cw * 0.08f;
    QRectF rect(cx + m, cy + m, cw - m*2, ch - m*2);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    if (rounded) {
        float r = std::min(cw, ch) * 0.30f;
        p.drawRoundedRect(rect, r, r);
    } else {
        p.drawRect(rect);
    }
}

// =============================================================================
// paintBoard
// =============================================================================
void MainWindow::paintBoard(QPainter &p, int boardW, int boardH) {
    float cw = static_cast<float>(boardW) / BOARD_WIDTH;
    float ch = static_cast<float>(boardH) / BOARD_HEIGHT;
    const auto &grid = board_.getGrid();

    // ── Background ────────────────────────────────────────────────────────────
    p.fillRect(0, 0, boardW, boardH, COL_BG);

    // ── Grid lines ────────────────────────────────────────────────────────────
    p.setPen(QPen(COL_GRID, 0.5f));
    for (int x = 0; x <= BOARD_WIDTH;  ++x)
        p.drawLine(QPointF(x*cw, 0), QPointF(x*cw, boardH));
    for (int y = 0; y <= BOARD_HEIGHT; ++y)
        p.drawLine(QPointF(0, y*ch), QPointF(boardW, y*ch));

    // ── Cells ─────────────────────────────────────────────────────────────────
    float pulse = 0.85f + 0.15f * std::sin(animFrame_ * 0.25f);

    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            float cx = x * cw, cy = y * ch;

            switch (grid[y][x]) {

                case Cell::WALL: {
                    QLinearGradient wg(cx, cy, cx+cw, cy+ch);
                    wg.setColorAt(0, COL_WALL.lighter(130));
                    wg.setColorAt(1, COL_WALL);
                    p.setPen(Qt::NoPen); p.setBrush(wg);
                    p.drawRect(QRectF(cx, cy, cw, ch));
                    break;
                }

                case Cell::SNAKE_HEAD: {
                    drawCell(p, cx, cy, cw, ch, COL_HEAD, true, cw * 2.2f);
                    p.setBrush(Qt::black); p.setPen(Qt::NoPen);
                    float ew = cw*0.13f, eh = ch*0.13f;
                    float ox = cw*0.27f, oy = ch*0.28f;
                    p.drawEllipse(QRectF(cx+ox, cy+oy, ew, eh));
                    p.drawEllipse(QRectF(cx+cw-ox-ew, cy+oy, ew, eh));
                    break;
                }

                case Cell::SNAKE_BODY:
                    drawCell(p, cx, cy, cw, ch, COL_BODY, true, cw * 0.7f);
                    break;

                case Cell::FOOD: {
                    float pw = cw*pulse, ph = ch*pulse;
                    float ox = (cw-pw)/2.f, oy2 = (ch-ph)/2.f;
                    float m  = pw * 0.10f;
                    drawGlow(p, cx+cw/2.f, cy+ch/2.f, cw*2.0f, COL_FOOD, 70);
                    p.setBrush(COL_FOOD); p.setPen(Qt::NoPen);
                    p.drawEllipse(QRectF(cx+ox+m, cy+oy2+m, pw-m*2, ph-m*2));
                    p.setBrush(QColor(255,200,200,160));
                    p.drawEllipse(QRectF(cx+ox+cw*0.18f, cy+oy2+ch*0.14f, pw*0.28f, ph*0.22f));
                    break;
                }

                case Cell::OBSTACLE: {
                    // Bright warning orange with inner X and pulsing glow
                    float bombPulse = 0.7f + 0.3f * std::abs(std::sin(animFrame_ * 0.15f));
                    QColor bCol(
                        static_cast<int>(COL_OBSTACLE.red()   * bombPulse),
                        static_cast<int>(COL_OBSTACLE.green() * bombPulse),
                        static_cast<int>(COL_OBSTACLE.blue()  * bombPulse)
                    );
                    drawGlow(p, cx+cw/2.f, cy+ch/2.f, cw*1.4f, COL_OBSTACLE, 55);
                    float m = cw * 0.08f;
                    QRectF or_(cx+m, cy+m, cw-m*2, ch-m*2);
                    p.setPen(QPen(bCol.darker(150), 1.0f));
                    p.setBrush(bCol);
                    p.drawRect(or_);
                    // X cross
                    p.setPen(QPen(QColor(255,200,100,200), 1.2f));
                    p.drawLine(QPointF(cx+m*2, cy+m*2), QPointF(cx+cw-m*2, cy+ch-m*2));
                    p.drawLine(QPointF(cx+cw-m*2, cy+m*2), QPointF(cx+m*2, cy+ch-m*2));
                    break;
                }

                default: break;
            }
        }
    }

    // ── Particles ─────────────────────────────────────────────────────────────
    p.setPen(Qt::NoPen);
    for (const auto &pt : particles_) {
        QColor c = pt.color; c.setAlphaF(pt.life);
        p.setBrush(c);
        float sz = 3.0f * pt.life;
        p.drawEllipse(QRectF(pt.x-sz/2, pt.y-sz/2, sz, sz));
    }

    // ── Scanlines ─────────────────────────────────────────────────────────────
    for (int sy = 0; sy < boardH; sy += 4)
        p.fillRect(0, sy, boardW, 1, QColor(0,0,0,18));

    // ── Border ────────────────────────────────────────────────────────────────
    float bAlpha = (state_ == GameState::RUNNING)
                   ? 80 + 40*std::sin(animFrame_*0.08f) : 40.f;
    QColor bc = (state_ == GameState::RUNNING) ? COL_HEAD : QColor(80,80,120);
    bc.setAlpha(static_cast<int>(bAlpha));
    p.setPen(QPen(bc, 2.0f)); p.setBrush(Qt::NoBrush);
    p.drawRect(1, 1, boardW-2, boardH-2);

    // ── Overlay screens ───────────────────────────────────────────────────────
    if (state_ == GameState::MENU    ||
        state_ == GameState::PAUSED  ||
        state_ == GameState::GAME_OVER)
    {
        p.setBrush(COL_OVERLAY); p.setPen(Qt::NoPen);
        p.drawRect(0, 0, boardW, boardH);
        float bw = boardW, bh = boardH;

        if (state_ == GameState::MENU) {
            p.setFont(QFont("Courier New", 36, QFont::Bold));
            for (int g = 3; g >= 1; g--) {
                p.setPen(QColor(57,255,20, 25*g));
                p.drawText(QRectF(g*2, bh*0.20f-g*2, bw-g*4, 60), Qt::AlignCenter, "SNAKE");
            }
            p.setPen(COL_HEAD);
            p.drawText(QRectF(0, bh*0.20f, bw, 60), Qt::AlignCenter, "SNAKE");

            p.setFont(QFont("Courier New", 11));
            p.setPen(QColor(160,160,220));
            p.drawText(QRectF(0, bh*0.40f, bw, 30), Qt::AlignCenter, "N E O N   A R C A D E");

            if ((animFrame_ / 20) % 2 == 0) {
                p.setFont(QFont("Courier New", 14, QFont::Bold));
                p.setPen(QColor(200,200,255));
                p.drawText(QRectF(0, bh*0.57f, bw, 36), Qt::AlignCenter, "PRESS SPACE TO START");
            }
            if (scoreManager_.getHighScore() > 0) {
                p.setFont(QFont("Courier New", 10));
                p.setPen(QColor(180,150,20));
                p.drawText(QRectF(0, bh*0.72f, bw, 28), Qt::AlignCenter,
                           QString("BEST: %1").arg(scoreManager_.getHighScore()));
            }
            p.setFont(QFont("Courier New", 9));
            p.setPen(QColor(60,60,100));
            p.drawText(QRectF(0, bh*0.86f, bw, 24), Qt::AlignCenter,
                       "W/A/S/D  ·  P = Pause  ·  Q = Quit");

            animFrame_++;
            QTimer::singleShot(50, ui->gameBoard, [this](){ ui->gameBoard->update(); });

        } else if (state_ == GameState::PAUSED) {
            p.setFont(QFont("Courier New", 30, QFont::Bold));
            p.setPen(QColor(100,180,255));
            p.drawText(QRectF(0, bh*0.32f, bw, 50), Qt::AlignCenter, "PAUSED");
            p.setFont(QFont("Courier New", 12));
            p.setPen(QColor(140,140,200));
            p.drawText(QRectF(0, bh*0.52f, bw, 30), Qt::AlignCenter,
                       "P = resume   R = restart   Q = quit");

        } else { // GAME_OVER
            float pRed = 180 + 75*std::abs(std::sin(animFrame_*0.05f));
            p.setFont(QFont("Courier New", 32, QFont::Bold));
            p.setPen(QColor(static_cast<int>(pRed), 30, 50));
            p.drawText(QRectF(0, bh*0.22f, bw, 56), Qt::AlignCenter, "GAME OVER");

            p.setFont(QFont("Courier New", 15, QFont::Bold));
            p.setPen(QColor(220,220,255));
            p.drawText(QRectF(0, bh*0.43f, bw, 32), Qt::AlignCenter,
                       QString("Score: %1   Level: %2").arg(score_).arg(level_));

            if (newHighScore_ && (animFrame_ / 10) % 2 == 0) {
                p.setFont(QFont("Courier New", 12, QFont::Bold));
                p.setPen(QColor(255,200,0));
                p.drawText(QRectF(0, bh*0.56f, bw, 28), Qt::AlignCenter, "✦ NEW HIGH SCORE! ✦");
            }
            if ((animFrame_ / 20) % 2 == 0) {
                p.setFont(QFont("Courier New", 12));
                p.setPen(QColor(160,255,160));
                p.drawText(QRectF(0, bh*0.70f, bw, 28), Qt::AlignCenter,
                           "R = restart   Q = quit");
            }

            animFrame_++;
            QTimer::singleShot(50, ui->gameBoard, [this](){ ui->gameBoard->update(); });
        }
    }
}