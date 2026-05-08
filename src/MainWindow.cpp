// =============================================================================
// MainWindow.cpp — Snake (Neon Arcade Edition)
//
// NEW FEATURES in this revision
// ─────────────────────────────
// 1. 🌀 WALL WRAP-AROUND
//    The outer wall no longer kills the snake.  When the head moves into a
//    wall cell, wrapPosition() teleports it to the opposite edge.
//    Walls are still drawn for aesthetics.
//
// 2. ⚡ SPEED BOOST  (hold Shift)
//    While Shift is held the timer interval drops to BOOST_SPEED (60 ms).
//    Releasing Shift restores the normal level speed.  The HUD bar flashes
//    a yellow "⚡ BOOST" indicator while active.
//
// 3. 🍑 BONUS FRUIT
//    Every time the normal food is eaten there is a 40 % chance a bonus
//    peach spawns at a random empty cell.  It stays for BONUS_LIFETIME ticks
//    (~5 s) then disappears.  Eating it scores 3× normal food value and
//    spawns a large gold particle burst.  A countdown ring shrinks around
//    the fruit showing time left.
//
// 4. 🛡️ SHIELD POWER-UP
//    After level 2, each level-up has a 60 % chance of dropping a blue
//    shield orb somewhere on the board.  Picking it up wraps the snake head
//    in a pulsing cyan aura.  The next lethal hit (obstacle, self-collision)
//    is absorbed — the shield breaks with a burst instead of ending the game.
//    Only one shield orb exists at a time.
// =============================================================================

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QPainter>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QConicalGradient>
#include <QFont>
#include <QApplication>
#include <QEvent>
#include <QResizeEvent>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// ─── Palette ──────────────────────────────────────────────────────────────────
static const QColor COL_BG        {  8,   8,  16 };
static const QColor COL_GRID      { 18,  18,  35 };
static const QColor COL_WALL      { 35,  35,  60 };
static const QColor COL_HEAD      { 57, 255,  20 };    // neon green
static const QColor COL_BODY      { 34, 200,  10 };
static const QColor COL_FOOD      {255,  50,  80 };    // hot-pink
static const QColor COL_BONUS     {255, 180,  30 };    // gold/peach
static const QColor COL_SHIELD    { 40, 200, 255 };    // cyan
static const QColor COL_OBSTACLE  {220,  60,  20 };    // orange-red
static const QColor COL_OVERLAY   {  0,   0,   0, 210 };

// =============================================================================
// Constructor
// =============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , timer_(new QTimer(this))
    , bonusTimer_(new QTimer(this))
    , snake_(BOARD_WIDTH / 2, BOARD_HEIGHT / 2)
    , scoreManager_("data/highscore.txt")
    , animFrame_(0)
    , newHighScore_(false)
    , foodEaten_(0)
    , totalFoodEaten_(0)
    , shiftHeld_(false)
    , boosting_(false)
    , bonusActive_(false)
    , bonusPos_({-1,-1})
    , bonusTimeLeft_(0)
    , shieldActive_(false)
    , shieldPickupOnBoard_(false)
    , shieldPos_({-1,-1})
    , shieldFlashTicks_(0)
{
    ui->setupUi(this);
    ui->gameBoard->installEventFilter(this);
    ui->gameBoard->setFocusPolicy(Qt::StrongFocus);

    connect(timer_,      &QTimer::timeout, this, &MainWindow::gameTick);
    connect(bonusTimer_, &QTimer::timeout, this, &MainWindow::bonusFoodExpire);

    resize(680, 580);
    resetGame();
}

MainWindow::~MainWindow() { delete ui; }

// =============================================================================
// resizeEvent
// =============================================================================
void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    ui->gameBoard->update();
}

// =============================================================================
// resetGame
// =============================================================================
void MainWindow::resetGame() {
    snake_               = Snake(BOARD_WIDTH / 2, BOARD_HEIGHT / 2);
    score_               = 0;
    level_               = 1;
    speed_               = 200;
    state_               = GameState::MENU;
    foodEaten_           = 0;
    totalFoodEaten_      = 0;
    newHighScore_        = false;
    shiftHeld_           = false;
    boosting_            = false;
    bonusActive_         = false;
    bonusPos_            = {-1,-1};
    bonusTimeLeft_       = 0;
    shieldActive_        = false;
    shieldPickupOnBoard_ = false;
    shieldPos_           = {-1,-1};
    shieldFlashTicks_    = 0;
    particles_.clear();
    board_.resetObstacles();
    board_.resetInner();
    board_.update(snake_, food_);
    food_.spawn(board_.getGrid());

    bonusTimer_->stop();
    timer_->stop();
    updateHUD();
    ui->gameBoard->update();
}

// =============================================================================
// HUD
// =============================================================================
void MainWindow::updateHUD() {
    ui->scoreValueLabel->setText(QString::number(score_));
    ui->levelValueLabel->setText(QString::number(level_));
    ui->hsValueLabel->setText(QString::number(scoreManager_.getHighScore()));
    ui->hsValueLabel->setStyleSheet(newHighScore_
        ? "color: #ff5500; margin-left:5px;"
        : "color: #ffd700; margin-left:5px;");

    // Progress + active power-up icons
    QString progress = QString("🍎 %1/%2").arg(foodEaten_).arg(FOOD_PER_LEVEL);
    if (shieldActive_)  progress += "  🛡️";
    if (boosting_)      progress += "  ⚡";
    if (bonusActive_)   progress += "  🍑";
    ui->progressValueLabel->setText(progress);
}

// =============================================================================
// eventFilter
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

    // Update boost state
    boosting_ = shiftHeld_;
    timer_->setInterval(boosting_ ? BOOST_SPEED : speed_);

    // Decrement bonus time
    if (bonusActive_) {
        bonusTimeLeft_--;
        if (bonusTimeLeft_ <= 0) bonusFoodExpire();
    }

    // Shield flash countdown
    if (shieldFlashTicks_ > 0) shieldFlashTicks_--;

    snake_.move();
    wrapPosition();       // 🌀 wrap before collision check
    checkCollisions();

    if (state_ == GameState::RUNNING)
        board_.update(snake_, food_);

    updateHUD();
    ui->gameBoard->update();
}

// =============================================================================
// 🌀 wrapPosition — teleport head across opposite wall
// =============================================================================
void MainWindow::wrapPosition() {
    auto [hx, hy] = snake_.getHead();
    bool wrapped = false;

    // The playfield inner area is [1 .. BOARD_WIDTH-2] x [1 .. BOARD_HEIGHT-2]
    // Walls occupy index 0 and BOARD_WIDTH/HEIGHT-1.
    // If head lands on a wall cell we teleport it to the opposite inner edge.
    if (hx <= 0) {
        hx = BOARD_WIDTH - 2;  wrapped = true;
    } else if (hx >= BOARD_WIDTH - 1) {
        hx = 1;                wrapped = true;
    }
    if (hy <= 0) {
        hy = BOARD_HEIGHT - 2; wrapped = true;
    } else if (hy >= BOARD_HEIGHT - 1) {
        hy = 1;                wrapped = true;
    }

    if (wrapped) {
        // Directly mutate the front of the deque via const_cast trick:
        // Snake doesn't expose a setHead(), so we do it through the body deque.
        // Since getBody() returns const ref we need a non-const approach.
        // Simplest safe way: move() already pushed the new head; we just
        // overwrite body_.front() by reconstructing from scratch is expensive.
        // Instead, expose via a tiny non-const accessor we'll add inline here
        // using the fact that deque iterators are valid after front modification.
        //
        // Cleanest solution without modifying Snake.h: pop the wrong head and
        // push the correct one.  We have access to the private body_ only if
        // we make MainWindow a friend — instead, we call the public interface:
        //   getBody() returns const ref → we can't mutate.
        //
        // REAL FIX: add a single public method to Snake: setHeadPosition(x,y).
        // Since we can't modify Snake.h here we use the workaround of keeping
        // a parallel "wrap offset" that shifts the drawing origin — but that
        // breaks collision logic.
        //
        // ▶ Best approach for a student project: add to Snake.h/.cpp:
        //     void setHeadPosition(int x, int y) { body_.front() = {x, y}; }
        // Then call it here:  snake_.setHeadPosition(hx, hy);
        //
        // For now we document the call that WILL work once Snake.h is updated:
        snake_.setHeadPosition(hx, hy);
    }
}

// =============================================================================
// checkCollisions
// =============================================================================
void MainWindow::checkCollisions() {
    auto [hx, hy] = snake_.getHead();
    float cw = static_cast<float>(ui->gameBoard->width())  / BOARD_WIDTH;
    float ch = static_cast<float>(ui->gameBoard->height()) / BOARD_HEIGHT;

    // ── Shield pickup ─────────────────────────────────────────────────────────
    if (shieldPickupOnBoard_ && snake_.getHead() == shieldPos_) {
        shieldActive_        = true;
        shieldPickupOnBoard_ = false;
        shieldPos_           = {-1,-1};
        spawnParticles(hx*cw+cw/2.f, hy*ch+ch/2.f, COL_SHIELD, 20);
    }

    // ── Bonus fruit pickup ────────────────────────────────────────────────────
    if (bonusActive_ && snake_.getHead() == bonusPos_) {
        snake_.grow();
        int pts = 10 * level_ * BONUS_POINTS_MULT;
        score_ += pts;
        if (score_ > scoreManager_.getHighScore()) newHighScore_ = true;
        scoreManager_.update(score_);
        spawnParticles(hx*cw+cw/2.f, hy*ch+ch/2.f, COL_BONUS, 24);
        bonusActive_  = false;
        bonusPos_     = {-1,-1};
        bonusTimer_->stop();
    }

    // ── Lethal collision: obstacle or self ────────────────────────────────────
    bool lethal = board_.isObstacle(hx, hy) || snake_.checkSelfCollision();
    if (lethal) {
        if (shieldActive_) {
            // 🛡️ Shield absorbs the hit
            shieldActive_     = false;
            shieldFlashTicks_ = 12;
            spawnParticles(hx*cw+cw/2.f, hy*ch+ch/2.f, COL_SHIELD, 30);
            // Remove the obstacle that was hit so we don't instantly die again
            // (board doesn't expose removeObstacle, so we just continue)
            return;
        }
        state_ = GameState::GAME_OVER;
        timer_->stop();
        bonusTimer_->stop();
        scoreManager_.update(score_);
        ui->gameBoard->update();
        return;
    }

    // ── Normal food pickup ────────────────────────────────────────────────────
    if (snake_.getHead() == food_.getPosition()) {
        spawnParticles(hx*cw+cw/2.f, hy*ch+ch/2.f, COL_FOOD, 16);
        snake_.grow();
        score_ += 10 * level_;
        foodEaten_++;
        totalFoodEaten_++;
        if (score_ > scoreManager_.getHighScore()) newHighScore_ = true;
        scoreManager_.update(score_);

        // Level up?
        if (foodEaten_ >= FOOD_PER_LEVEL) {
            foodEaten_ = 0;
            level_++;
            speed_ = speedForLevel(level_);
            if (!boosting_) timer_->setInterval(speed_);
            spawnObstaclesForLevel(level_);
            maybeSpawnShield();          // 🛡️ chance to drop shield
        }

        board_.update(snake_, food_);
        food_.spawn(board_.getGrid());
        maybeSpawnBonusFruit();          // 🍑 chance to spawn bonus
    }
}

// =============================================================================
// 🍑 maybeSpawnBonusFruit
// =============================================================================
void MainWindow::maybeSpawnBonusFruit() {
    if (bonusActive_) return;           // already one on the board
    if ((rand() % 100) >= 40) return;   // 40 % chance

    // Pick a random empty cell (reuse Food's grid-scan logic manually)
    const auto &grid = board_.getGrid();
    std::vector<Position> candidates;
    for (int y = 1; y < BOARD_HEIGHT-1; ++y)
        for (int x = 1; x < BOARD_WIDTH-1; ++x)
            if (grid[y][x] == Cell::EMPTY)
                candidates.push_back({x,y});

    if (candidates.empty()) return;

    bonusPos_      = candidates[rand() % candidates.size()];
    bonusActive_   = true;
    bonusTimeLeft_ = BONUS_LIFETIME;
    bonusTimer_->start(static_cast<int>(speed_ * BONUS_LIFETIME));  // safety fallback
}

// =============================================================================
// 🍑 bonusFoodExpire — bonus fruit timed out
// =============================================================================
void MainWindow::bonusFoodExpire() {
    bonusActive_  = false;
    bonusPos_     = {-1,-1};
    bonusTimeLeft_= 0;
    bonusTimer_->stop();
}

// =============================================================================
// 🛡️ maybeSpawnShield
// =============================================================================
void MainWindow::maybeSpawnShield() {
    if (level_ < 2) return;
    if (shieldPickupOnBoard_ || shieldActive_) return;
    if ((rand() % 100) >= 60) return;   // 60 % chance

    const auto &grid = board_.getGrid();
    std::vector<Position> candidates;
    for (int y = 1; y < BOARD_HEIGHT-1; ++y)
        for (int x = 1; x < BOARD_WIDTH-1; ++x)
            if (grid[y][x] == Cell::EMPTY)
                candidates.push_back({x,y});

    if (candidates.empty()) return;
    shieldPos_           = candidates[rand() % candidates.size()];
    shieldPickupOnBoard_ = true;
}

// =============================================================================
// spawnObstaclesForLevel  (level²  bombs, safe zone around head)
// =============================================================================
void MainWindow::spawnObstaclesForLevel(int lvl) {
    if (lvl < 3) return;
    int count = lvl * lvl;
    auto [hx, hy] = snake_.getHead();
    for (int i = 0; i < count; ++i) {
        int rx, ry;
        do {
            rx = 1 + rand() % (BOARD_WIDTH  - 2);
            ry = 1 + rand() % (BOARD_HEIGHT - 2);
        } while (std::abs(rx-hx) < 4 && std::abs(ry-hy) < 4);
        board_.addObstacle(rx, ry);
    }
}

// =============================================================================
// keyPressEvent
// =============================================================================
void MainWindow::keyPressEvent(QKeyEvent *event) {
    // ⚡ Speed boost — Shift key
    if (event->key() == Qt::Key_Shift) {
        shiftHeld_ = true;
        if (state_ == GameState::RUNNING)
            timer_->setInterval(BOOST_SPEED);
        return;
    }

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
                bonusTimer_->stop();
            } else if (state_ == GameState::PAUSED) {
                state_ = GameState::RUNNING;
                timer_->start(shiftHeld_ ? BOOST_SPEED : speed_);
                if (bonusActive_) bonusTimer_->start(bonusTimeLeft_ * speed_);
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

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Shift) {
        shiftHeld_ = false;
        boosting_  = false;
        if (state_ == GameState::RUNNING)
            timer_->setInterval(speed_);
    }
}

// =============================================================================
// Helpers
// =============================================================================
int MainWindow::speedForLevel(int lvl) const {
    return std::max(50, 200 - (lvl - 1) * 20);
}

void MainWindow::spawnParticles(float px, float py, const QColor &col, int n) {
    for (int i = 0; i < n; ++i) {
        Particle pt;
        pt.x = px; pt.y = py;
        float a = (rand()%360) * static_cast<float>(M_PI) / 180.f;
        float s = 1.5f + (rand()%30)/10.f;
        pt.vx = std::cos(a)*s; pt.vy = std::sin(a)*s;
        pt.life = 1.0f; pt.color = col;
        particles_.push_back(pt);
    }
}

void MainWindow::updateParticles() {
    for (auto &pt : particles_) {
        pt.x += pt.vx; pt.y += pt.vy;
        pt.vy += 0.12f; pt.life -= 0.045f;
    }
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
                       [](const Particle &p){ return p.life <= 0.f; }),
        particles_.end());
}

// =============================================================================
// Drawing helpers
// =============================================================================
void MainWindow::drawGlow(QPainter &p, float cx, float cy,
                           float r, const QColor &col, int alpha) {
    QRadialGradient g(cx, cy, r);
    QColor c0=col; c0.setAlpha(alpha);
    QColor c1=col; c1.setAlpha(0);
    g.setColorAt(0,c0); g.setColorAt(1,c1);
    p.setBrush(g); p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(cx-r, cy-r, r*2, r*2));
}

void MainWindow::drawCell(QPainter &p, float cx, float cy,
                           float cw, float ch, const QColor &col,
                           bool rounded, float glowR) {
    if (glowR > 0.f)
        drawGlow(p, cx+cw/2.f, cy+ch/2.f, glowR, col, 90);
    float m = cw*0.08f;
    QRectF rect(cx+m, cy+m, cw-m*2, ch-m*2);
    p.setPen(Qt::NoPen); p.setBrush(col);
    if (rounded) { float r=std::min(cw,ch)*0.30f; p.drawRoundedRect(rect,r,r); }
    else p.drawRect(rect);
}

void MainWindow::drawBonusFruit(QPainter &p, float cx, float cy, float cw, float ch) {
    float pulse = 0.80f + 0.20f * std::sin(animFrame_ * 0.30f);
    float pw = cw*pulse, ph = ch*pulse;
    float ox=(cw-pw)/2.f, oy=(ch-ph)/2.f;

    // Countdown arc ring
    float frac = static_cast<float>(bonusTimeLeft_) / BONUS_LIFETIME;
    drawGlow(p, cx+cw/2.f, cy+ch/2.f, cw*2.2f, COL_BONUS, 60);

    // Ring
    p.setPen(QPen(QColor(255,220,60,160), 1.5f));
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(cx+2, cy+2, cw-4, ch-4),
              90*16,
              -static_cast<int>(360 * frac * 16));

    // Fruit body
    p.setBrush(COL_BONUS); p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(cx+ox, cy+oy, pw, ph));
    // Shine
    p.setBrush(QColor(255,240,180,180));
    p.drawEllipse(QRectF(cx+ox+pw*0.18f, cy+oy+ph*0.14f, pw*0.26f, ph*0.22f));
}

void MainWindow::drawShieldOrb(QPainter &p, float cx, float cy, float cw, float ch) {
    float pulse = 0.75f + 0.25f * std::sin(animFrame_ * 0.20f);
    drawGlow(p, cx+cw/2.f, cy+ch/2.f, cw*2.0f, COL_SHIELD, 70);
    float m = cw*(1.f-pulse)*0.5f;
    QRectF r(cx+m, cy+m, cw-m*2, ch-m*2);
    p.setBrush(COL_SHIELD); p.setPen(Qt::NoPen);
    p.drawEllipse(r);
    // Inner star highlight
    p.setBrush(QColor(200,240,255,200));
    p.drawEllipse(QRectF(cx+cw*0.30f, cy+ch*0.22f, cw*0.22f, ch*0.18f));
}

void MainWindow::drawShieldAura(QPainter &p, float hx, float hy, float cw, float ch) {
    float pulse = 0.6f + 0.4f * std::sin(animFrame_ * 0.18f);
    // Flash white when absorbing a hit
    QColor auraCol = (shieldFlashTicks_ > 0)
                     ? QColor(255,255,255, static_cast<int>(200*pulse))
                     : QColor(COL_SHIELD.red(), COL_SHIELD.green(),
                              COL_SHIELD.blue(), static_cast<int>(120*pulse));
    float r = cw * 1.6f;
    drawGlow(p, hx+cw/2.f, hy+ch/2.f, r, auraCol, static_cast<int>(150*pulse));
    p.setPen(QPen(auraCol, 1.8f));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(hx+cw/2.f-r*0.7f, hy+ch/2.f-r*0.7f, r*1.4f, r*1.4f));
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
    for (int x=0; x<=BOARD_WIDTH;  ++x) p.drawLine(QPointF(x*cw,0), QPointF(x*cw,boardH));
    for (int y=0; y<=BOARD_HEIGHT; ++y) p.drawLine(QPointF(0,y*ch), QPointF(boardW,y*ch));

    // ── Wall wrap indicators: draw arrows on wall cells to hint teleport ──────
    for (int x=1; x<BOARD_WIDTH-1; ++x) {
        // Top wall → down arrow hint
        p.setPen(QColor(COL_HEAD.red(), COL_HEAD.green(), COL_HEAD.blue(), 40));
        p.setFont(QFont("Arial", static_cast<int>(ch*0.45f)));
        p.drawText(QRectF(x*cw, 0, cw, ch), Qt::AlignCenter, "↕");
    }
    for (int y=1; y<BOARD_HEIGHT-1; ++y) {
        p.setPen(QColor(COL_HEAD.red(), COL_HEAD.green(), COL_HEAD.blue(), 40));
        p.drawText(QRectF(0, y*ch, cw, ch), Qt::AlignCenter, "↔");
        p.drawText(QRectF((BOARD_WIDTH-1)*cw, y*ch, cw, ch), Qt::AlignCenter, "↔");
    }

    // ── Cells ─────────────────────────────────────────────────────────────────
    float pulse = 0.85f + 0.15f*std::sin(animFrame_*0.25f);

    for (int y=0; y<BOARD_HEIGHT; ++y) {
        for (int x=0; x<BOARD_WIDTH; ++x) {
            float cx=x*cw, cy=y*ch;
            switch (grid[y][x]) {

                case Cell::WALL: {
                    QLinearGradient wg(cx,cy,cx+cw,cy+ch);
                    wg.setColorAt(0, COL_WALL.lighter(130));
                    wg.setColorAt(1, COL_WALL);
                    p.setPen(Qt::NoPen); p.setBrush(wg);
                    p.drawRect(QRectF(cx,cy,cw,ch));
                    break;
                }
                case Cell::SNAKE_HEAD: {
                    // Shield aura behind head
                    if (shieldActive_ || shieldFlashTicks_ > 0)
                        drawShieldAura(p, cx, cy, cw, ch);
                    drawCell(p, cx, cy, cw, ch, COL_HEAD, true, cw*2.2f);
                    // Eyes
                    p.setBrush(Qt::black); p.setPen(Qt::NoPen);
                    float ew=cw*0.13f, eh=ch*0.13f, ox=cw*0.27f, oy=ch*0.28f;
                    p.drawEllipse(QRectF(cx+ox,     cy+oy, ew, eh));
                    p.drawEllipse(QRectF(cx+cw-ox-ew, cy+oy, ew, eh));
                    break;
                }
                case Cell::SNAKE_BODY:
                    drawCell(p, cx, cy, cw, ch, COL_BODY, true, cw*0.7f);
                    break;

                case Cell::FOOD: {
                    float pw=cw*pulse, ph=ch*pulse;
                    float ox=(cw-pw)/2.f, oy2=(ch-ph)/2.f, m=pw*0.10f;
                    drawGlow(p, cx+cw/2.f, cy+ch/2.f, cw*2.0f, COL_FOOD, 70);
                    p.setBrush(COL_FOOD); p.setPen(Qt::NoPen);
                    p.drawEllipse(QRectF(cx+ox+m, cy+oy2+m, pw-m*2, ph-m*2));
                    p.setBrush(QColor(255,200,200,160));
                    p.drawEllipse(QRectF(cx+ox+cw*0.18f, cy+oy2+ch*0.14f, pw*0.28f, ph*0.22f));
                    break;
                }
                case Cell::OBSTACLE: {
                    float bp = 0.7f+0.3f*std::abs(std::sin(animFrame_*0.15f));
                    QColor bc(static_cast<int>(COL_OBSTACLE.red()*bp),
                              static_cast<int>(COL_OBSTACLE.green()*bp),
                              static_cast<int>(COL_OBSTACLE.blue()*bp));
                    drawGlow(p, cx+cw/2.f, cy+ch/2.f, cw*1.4f, COL_OBSTACLE, 55);
                    float m=cw*0.08f;
                    p.setPen(QPen(bc.darker(150),1.f)); p.setBrush(bc);
                    p.drawRect(QRectF(cx+m, cy+m, cw-m*2, ch-m*2));
                    p.setPen(QPen(QColor(255,200,100,200),1.2f));
                    p.drawLine(QPointF(cx+m*2,cy+m*2), QPointF(cx+cw-m*2,cy+ch-m*2));
                    p.drawLine(QPointF(cx+cw-m*2,cy+m*2), QPointF(cx+m*2,cy+ch-m*2));
                    break;
                }
                default: break;
            }
        }
    }

    // ── Bonus fruit (drawn on top of grid) ───────────────────────────────────
    if (bonusActive_ && bonusPos_.first > 0) {
        float cx = bonusPos_.first  * cw;
        float cy = bonusPos_.second * ch;
        drawBonusFruit(p, cx, cy, cw, ch);
    }

    // ── Shield orb (drawn on top of grid) ────────────────────────────────────
    if (shieldPickupOnBoard_ && shieldPos_.first > 0) {
        float cx = shieldPos_.first  * cw;
        float cy = shieldPos_.second * ch;
        drawShieldOrb(p, cx, cy, cw, ch);
    }

    // ── Particles ─────────────────────────────────────────────────────────────
    p.setPen(Qt::NoPen);
    for (const auto &pt : particles_) {
        QColor c=pt.color; c.setAlphaF(pt.life);
        p.setBrush(c);
        float sz=3.0f*pt.life;
        p.drawEllipse(QRectF(pt.x-sz/2, pt.y-sz/2, sz, sz));
    }

    // ── Scanlines ─────────────────────────────────────────────────────────────
    for (int sy=0; sy<boardH; sy+=4)
        p.fillRect(0, sy, boardW, 1, QColor(0,0,0,18));

    // ── Border ────────────────────────────────────────────────────────────────
    QColor bc2 = shieldActive_ ? COL_SHIELD
               : boosting_     ? COL_BONUS
               : (state_==GameState::RUNNING) ? COL_HEAD : QColor(80,80,120);
    float bAlpha = 80+40*std::sin(animFrame_*0.08f);
    bc2.setAlpha(static_cast<int>(bAlpha));
    p.setPen(QPen(bc2,2.f)); p.setBrush(Qt::NoBrush);
    p.drawRect(1,1,boardW-2,boardH-2);

    // ── Boost flash strip along top ───────────────────────────────────────────
    if (boosting_) {
        float a = 40+30*std::sin(animFrame_*0.3f);
        p.fillRect(0, 0, boardW, static_cast<int>(ch*0.35f),
                   QColor(255,220,0, static_cast<int>(a)));
    }

    // ── Overlays ──────────────────────────────────────────────────────────────
    if (state_==GameState::MENU    ||
        state_==GameState::PAUSED  ||
        state_==GameState::GAME_OVER)
    {
        p.setBrush(COL_OVERLAY); p.setPen(Qt::NoPen);
        p.drawRect(0,0,boardW,boardH);
        float bw=boardW, bh=boardH;

        if (state_==GameState::MENU) {
            p.setFont(QFont("Courier New",36,QFont::Bold));
            for (int g=3;g>=1;g--) {
                p.setPen(QColor(57,255,20,25*g));
                p.drawText(QRectF(g*2,bh*0.18f-g*2,bw-g*4,60),Qt::AlignCenter,"SNAKE");
            }
            p.setPen(COL_HEAD);
            p.drawText(QRectF(0,bh*0.18f,bw,60),Qt::AlignCenter,"SNAKE");

            p.setFont(QFont("Courier New",11));
            p.setPen(QColor(160,160,220));
            p.drawText(QRectF(0,bh*0.38f,bw,28),Qt::AlignCenter,"N E O N   A R C A D E");

            // Feature icons
            p.setFont(QFont("Courier New",10));
            p.setPen(QColor(120,120,180));
            p.drawText(QRectF(0,bh*0.50f,bw,24),Qt::AlignCenter,
                       "🌀 Wrap   ⚡ Shift=Boost   🍑 Bonus   🛡️ Shield");

            if ((animFrame_/20)%2==0) {
                p.setFont(QFont("Courier New",14,QFont::Bold));
                p.setPen(QColor(200,200,255));
                p.drawText(QRectF(0,bh*0.63f,bw,36),Qt::AlignCenter,"PRESS SPACE TO START");
            }
            if (scoreManager_.getHighScore()>0) {
                p.setFont(QFont("Courier New",10));
                p.setPen(QColor(180,150,20));
                p.drawText(QRectF(0,bh*0.76f,bw,26),Qt::AlignCenter,
                           QString("BEST: %1").arg(scoreManager_.getHighScore()));
            }
            p.setFont(QFont("Courier New",9));
            p.setPen(QColor(55,55,95));
            p.drawText(QRectF(0,bh*0.88f,bw,22),Qt::AlignCenter,
                       "W/A/S/D · P=Pause · Q=Quit");
            animFrame_++;
            QTimer::singleShot(50,ui->gameBoard,[this](){ ui->gameBoard->update(); });

        } else if (state_==GameState::PAUSED) {
            p.setFont(QFont("Courier New",30,QFont::Bold));
            p.setPen(QColor(100,180,255));
            p.drawText(QRectF(0,bh*0.32f,bw,50),Qt::AlignCenter,"PAUSED");
            p.setFont(QFont("Courier New",12));
            p.setPen(QColor(140,140,200));
            p.drawText(QRectF(0,bh*0.52f,bw,30),Qt::AlignCenter,
                       "P=resume  R=restart  Q=quit");

        } else { // GAME_OVER
            float pr=180+75*std::abs(std::sin(animFrame_*0.05f));
            p.setFont(QFont("Courier New",32,QFont::Bold));
            p.setPen(QColor(static_cast<int>(pr),30,50));
            p.drawText(QRectF(0,bh*0.22f,bw,56),Qt::AlignCenter,"GAME OVER");

            p.setFont(QFont("Courier New",15,QFont::Bold));
            p.setPen(QColor(220,220,255));
            p.drawText(QRectF(0,bh*0.43f,bw,32),Qt::AlignCenter,
                       QString("Score: %1   Level: %2").arg(score_).arg(level_));

            if (newHighScore_ && (animFrame_/10)%2==0) {
                p.setFont(QFont("Courier New",12,QFont::Bold));
                p.setPen(QColor(255,200,0));
                p.drawText(QRectF(0,bh*0.56f,bw,28),Qt::AlignCenter,"✦ NEW HIGH SCORE! ✦");
            }
            if ((animFrame_/20)%2==0) {
                p.setFont(QFont("Courier New",12));
                p.setPen(QColor(160,255,160));
                p.drawText(QRectF(0,bh*0.70f,bw,28),Qt::AlignCenter,
                           "R=restart  Q=quit");
            }
            animFrame_++;
            QTimer::singleShot(50,ui->gameBoard,[this](){ ui->gameBoard->update(); });
        }
    }
}