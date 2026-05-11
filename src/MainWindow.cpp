// =============================================================================
// MainWindow.cpp — Snake (Neon Arcade Edition)
//
// NEW IN THIS REVISION
// ────────────────────
// 🎨 SNAKE SKIN SELECTOR
//    On the main menu press → / ← (or D/A) to browse 5 skins:
//      • Neon Green (classic)   • Plasma Purple   • Ice Blue
//      • Fire Red               • Gold Rush
//    Each skin changes the head color, body color, and glow.
//    A small animated preview snake is shown for each skin.
//    Press SPACE / ENTER to confirm and start.
//
// 🌡️ DIFFICULTY SELECTOR
//    On the main menu press ↑ / ↓ (or W/S) to cycle difficulty:
//      EASY   — slow start (250 ms), no bombs until level 5,
//                shield always drops, wall wrap always on.
//      NORMAL — 200 ms start, bombs from level 3 (level²),
//                shield drops with 60 % chance, wrap on.
//      HARD   — 150 ms start, bombs from level 2 (level² + level),
//                NO shield drops, NO wall wrap, walls kill instantly.
//    The difficulty badge is shown in the HUD during play.
// =============================================================================

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QPainter>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QFont>
#include <QApplication>
#include <QEvent>
#include <QResizeEvent>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// ─── Static palette (non-skin colours) ───────────────────────────────────────
static const QColor COL_BG       {  8,   8,  16 };
static const QColor COL_GRID     { 18,  18,  35 };
static const QColor COL_WALL     { 35,  35,  60 };
static const QColor COL_WALL_H   { 90,  20,  20 };   // Hard mode wall colour
static const QColor COL_FOOD     {255,  50,  80 };
static const QColor COL_BONUS    {255, 180,  30 };
static const QColor COL_SHIELD   { 40, 200, 255 };
static const QColor COL_OBSTACLE {220,  60,  20 };
static const QColor COL_OVERLAY  {  0,   0,   0, 210 };

// Difficulty label colours
static const QColor COL_EASY   { 60, 220,  80 };
static const QColor COL_NORMAL {220, 180,  30 };
static const QColor COL_HARD   {230,  50,  50 };

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
    , selectedSkin_(0)
    , menuSkinScroll_(0)
    , difficulty_(Difficulty::NORMAL)
    , selectedDiff_(1)
    , menuPage_(MenuPage::MAIN)
{
    // ── Define all skins ──────────────────────────────────────────────────────
    skins_ = {
        { "Neon Green",    { 57, 255,  20}, { 34, 200,  10}, { 57, 255,  20} },
        { "Plasma Purple", {200,  50, 255}, {150,  30, 200}, {200,  50, 255} },
        { "Ice Blue",      { 50, 200, 255}, { 20, 150, 210}, { 50, 200, 255} },
        { "Fire Red",      {255,  80,  20}, {210,  50,  10}, {255, 120,  20} },
        { "Gold Rush",     {255, 210,  30}, {210, 160,  10}, {255, 220,  80} },
    };

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
// Difficulty helpers
// =============================================================================
int MainWindow::baseSpeed() const {
    switch (difficulty_) {
        case Difficulty::EASY:   return 250;
        case Difficulty::NORMAL: return 200;
        case Difficulty::HARD:   return 150;
    }
    return 200;
}

int MainWindow::bombCount(int lvl) const {
    switch (difficulty_) {
        case Difficulty::EASY:
            if (lvl < 5) return 0;
            return (lvl - 4) * 3;            // gentle ramp from level 5
        case Difficulty::NORMAL:
            if (lvl < 3) return 0;
            return lvl * lvl;                // quadratic
        case Difficulty::HARD:
            if (lvl < 2) return 0;
            return lvl * lvl + lvl;          // quadratic + linear, brutal
    }
    return 0;
}

bool MainWindow::shieldAllowed() const {
    return difficulty_ != Difficulty::HARD;
}

bool MainWindow::wrapAllowed() const {
    return difficulty_ != Difficulty::HARD;
}

int MainWindow::speedForLevel(int lvl) const {
    int base = baseSpeed();
    return std::max(50, base - (lvl - 1) * 18);
}

void MainWindow::applyDifficulty() {
    speed_ = baseSpeed();
}

// =============================================================================
// resetGame
// =============================================================================
void MainWindow::resetGame() {
    snake_               = Snake(BOARD_WIDTH / 2, BOARD_HEIGHT / 2);
    score_               = 0;
    level_               = 1;
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
    menuPage_            = MenuPage::MAIN;

    applyDifficulty();

    board_.resetInner();
    board_.update(snake_, food_);
    food_.spawn(board_.getGrid());

    state_ = GameState::MENU;
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

    QString progress = QString("🍎 %1/%2").arg(foodEaten_).arg(FOOD_PER_LEVEL);
    if (shieldActive_)  progress += "  🛡️";
    if (boosting_)      progress += "  ⚡";
    if (bonusActive_)   progress += "  🍑";

    // Difficulty badge
    switch (difficulty_) {
        case Difficulty::EASY:   progress += "  🟢 EASY";   break;
        case Difficulty::NORMAL: progress += "  🟡 NORMAL"; break;
        case Difficulty::HARD:   progress += "  🔴 HARD";   break;
    }

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
    inputRegisteredThisTick_ = false;

    boosting_ = shiftHeld_;
    timer_->setInterval(boosting_ ? BOOST_SPEED : speed_);

    if (bonusActive_) {
        bonusTimeLeft_--;
        if (bonusTimeLeft_ <= 0) bonusFoodExpire();
    }
    if (shieldFlashTicks_ > 0) shieldFlashTicks_--;

    snake_.move();

    if (wrapAllowed()) wrapPosition();

    checkCollisions();

    if (state_ == GameState::RUNNING)
        board_.update(snake_, food_);

    updateHUD();
    ui->gameBoard->update();
}

// =============================================================================
// wrapPosition
// =============================================================================
void MainWindow::wrapPosition() {
    auto [hx, hy] = snake_.getHead();
    bool wrapped = false;
    if      (hx <= 0)              { hx = BOARD_WIDTH  - 2; wrapped = true; }
    else if (hx >= BOARD_WIDTH-1)  { hx = 1;                wrapped = true; }
    if      (hy <= 0)              { hy = BOARD_HEIGHT - 2; wrapped = true; }
    else if (hy >= BOARD_HEIGHT-1) { hy = 1;                wrapped = true; }
    if (wrapped) snake_.setHeadPosition(hx, hy);
}

// =============================================================================
// checkCollisions
// =============================================================================
void MainWindow::checkCollisions() {
    auto [hx, hy] = snake_.getHead();
    float cw = static_cast<float>(ui->gameBoard->width())  / BOARD_WIDTH;
    float ch = static_cast<float>(ui->gameBoard->height()) / BOARD_HEIGHT;

    // Shield pickup
    if (shieldPickupOnBoard_ && snake_.getHead() == shieldPos_) {
        shieldActive_        = true;
        shieldPickupOnBoard_ = false;
        shieldPos_           = {-1,-1};
        spawnParticles(hx*cw+cw/2.f, hy*ch+ch/2.f, COL_SHIELD, 20);
    }

    // Bonus fruit pickup
    if (bonusActive_ && snake_.getHead() == bonusPos_) {
        snake_.grow();
        score_ += 10 * level_ * BONUS_POINTS_MULT;
        if (score_ > scoreManager_.getHighScore()) newHighScore_ = true;
        scoreManager_.update(score_);
        spawnParticles(hx*cw+cw/2.f, hy*ch+ch/2.f, COL_BONUS, 24);
        bonusActive_ = false;
        bonusPos_    = {-1,-1};
        bonusTimer_->stop();
    }

    // Wall collision (only Hard — wrap handles it on Easy/Normal)
    bool wallHit = !wrapAllowed() &&
                   (hx <= 0 || hx >= BOARD_WIDTH-1 ||
                    hy <= 0 || hy >= BOARD_HEIGHT-1);

    bool lethal = board_.isObstacle(hx, hy) || snake_.checkSelfCollision() || wallHit;

    if (lethal) {
        if (shieldActive_) {
            shieldActive_     = false;
            shieldFlashTicks_ = 12;
            spawnParticles(hx*cw+cw/2.f, hy*ch+ch/2.f, COL_SHIELD, 30);
            if (wallHit) {
                if (hx <= 0)              hx = 1;
                else if (hx >= BOARD_WIDTH-1)  hx = BOARD_WIDTH-2;
                if (hy <= 0)              hy = 1;
                else if (hy >= BOARD_HEIGHT-1) hy = BOARD_HEIGHT-2;
                snake_.setHeadPosition(hx, hy);
            }
            return;
        }
        state_ = GameState::GAME_OVER;
        inputRegisteredThisTick_ = false;   // unblock keyboard immediately
        timer_->stop();
        bonusTimer_->stop();
        scoreManager_.update(score_);
        ui->gameBoard->update();
        return;
    }

    // Normal food
    if (snake_.getHead() == food_.getPosition()) {
        spawnParticles(hx*cw+cw/2.f, hy*ch+ch/2.f, COL_FOOD, 16);
        snake_.grow();
        score_ += 10 * level_;
        foodEaten_++;
        totalFoodEaten_++;
        if (score_ > scoreManager_.getHighScore()) newHighScore_ = true;
        scoreManager_.update(score_);

        if (foodEaten_ >= FOOD_PER_LEVEL) {
            foodEaten_ = 0;
            level_++;
            speed_ = speedForLevel(level_);
            if (!boosting_) timer_->setInterval(speed_);
            spawnObstaclesForLevel(level_);
            if (shieldAllowed()) maybeSpawnShield();
        }

        board_.update(snake_, food_);
        food_.spawn(board_.getGrid());
        maybeSpawnBonusFruit();
    }
}

// =============================================================================
// Spawners
// =============================================================================
void MainWindow::spawnObstaclesForLevel(int lvl) {
    int count = bombCount(lvl);
    if (count <= 0) return;
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

void MainWindow::maybeSpawnBonusFruit() {
    if (bonusActive_) return;
    if ((rand() % 100) >= 40) return;
    const auto &grid = board_.getGrid();
    std::vector<Position> cands;
    for (int y=1; y<BOARD_HEIGHT-1; ++y)
        for (int x=1; x<BOARD_WIDTH-1; ++x)
            if (grid[y][x] == Cell::EMPTY) cands.push_back({x,y});
    if (cands.empty()) return;
    bonusPos_      = cands[rand() % cands.size()];
    bonusActive_   = true;
    bonusTimeLeft_ = BONUS_LIFETIME;
    bonusTimer_->start(speed_ * BONUS_LIFETIME);
}

void MainWindow::bonusFoodExpire() {
    bonusActive_   = false;
    bonusPos_      = {-1,-1};
    bonusTimeLeft_ = 0;
    bonusTimer_->stop();
    ui->gameBoard->update();
}

void MainWindow::maybeSpawnShield() {
    if (!shieldAllowed()) return;
    if (level_ < 2) return;
    if (shieldPickupOnBoard_ || shieldActive_) return;
    if ((rand() % 100) >= 60) return;
    const auto &grid = board_.getGrid();
    std::vector<Position> cands;
    for (int y=1; y<BOARD_HEIGHT-1; ++y)
        for (int x=1; x<BOARD_WIDTH-1; ++x)
            if (grid[y][x] == Cell::EMPTY) cands.push_back({x,y});
    if (cands.empty()) return;
    shieldPos_           = cands[rand() % cands.size()];
    shieldPickupOnBoard_ = true;
}

// =============================================================================
// keyPressEvent
// =============================================================================
void MainWindow::keyPressEvent(QKeyEvent *event) {
    // Shift = boost
    if (event->key() == Qt::Key_Shift) {
        shiftHeld_ = true;
        if (state_ == GameState::RUNNING)
            timer_->setInterval(BOOST_SPEED);
        return;
    }

    // ── MENU navigation ───────────────────────────────────────────────────────
    if (state_ == GameState::MENU) {
        if (menuPage_ == MenuPage::MAIN) {
            switch (event->key()) {
                // Left/Right → cycle skins
                case Qt::Key_A: case Qt::Key_Left:
                    selectedSkin_ = (selectedSkin_ - 1 + (int)skins_.size()) % (int)skins_.size();
                    break;
                case Qt::Key_D: case Qt::Key_Right:
                    selectedSkin_ = (selectedSkin_ + 1) % (int)skins_.size();
                    break;
                // Up/Down → cycle difficulty
                case Qt::Key_W: case Qt::Key_Up:
                    selectedDiff_ = (selectedDiff_ - 1 + 3) % 3;
                    difficulty_   = static_cast<Difficulty>(selectedDiff_);
                    break;
                case Qt::Key_S: case Qt::Key_Down:
                    selectedDiff_ = (selectedDiff_ + 1) % 3;
                    difficulty_   = static_cast<Difficulty>(selectedDiff_);
                    break;
                // Space/Enter → start
                case Qt::Key_Space: case Qt::Key_Return:
                    applyDifficulty();
                    speed_ = baseSpeed();
                    state_ = GameState::RUNNING;
                    timer_->start(speed_);
                    break;
                case Qt::Key_Q: case Qt::Key_Escape:
                    QApplication::quit();
                    break;
            }
        }
        ui->gameBoard->update();
        return;
    }

    // ── R and Q work in ANY non-menu state (not guarded by tick lock) ────────
    if (event->key() == Qt::Key_R) {
        if (state_ == GameState::GAME_OVER || state_ == GameState::PAUSED)
            resetGame();
        return;
    }
    if (event->key() == Qt::Key_Q || event->key() == Qt::Key_Escape) {
        QApplication::quit();
        return;
    }

    // ── In-game controls (direction + pause — one per tick) ───────────────────
    if (!inputRegisteredThisTick_) {
        switch (event->key()) {
            case Qt::Key_W: case Qt::Key_Up:
                snake_.setDirection(Direction::UP);    inputRegisteredThisTick_ = true; break;
            case Qt::Key_S: case Qt::Key_Down:
                snake_.setDirection(Direction::DOWN);  inputRegisteredThisTick_ = true; break;
            case Qt::Key_A: case Qt::Key_Left:
                snake_.setDirection(Direction::LEFT);  inputRegisteredThisTick_ = true; break;
            case Qt::Key_D: case Qt::Key_Right:
                snake_.setDirection(Direction::RIGHT); inputRegisteredThisTick_ = true; break;

            case Qt::Key_P:
                if (state_ == GameState::RUNNING) {
                    state_ = GameState::PAUSED;
                    timer_->stop(); bonusTimer_->stop();
                } else if (state_ == GameState::PAUSED) {
                    state_ = GameState::RUNNING;
                    timer_->start(shiftHeld_ ? BOOST_SPEED : speed_);
                    if (bonusActive_) bonusTimer_->start(bonusTimeLeft_ * speed_);
                }
                ui->gameBoard->update();
                break;

            default: break;
        }
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
    if (glowR > 0.f) drawGlow(p, cx+cw/2.f, cy+ch/2.f, glowR, col, 90);
    float m = cw*0.08f;
    QRectF rect(cx+m, cy+m, cw-m*2, ch-m*2);
    p.setPen(Qt::NoPen); p.setBrush(col);
    if (rounded) { float r=std::min(cw,ch)*0.30f; p.drawRoundedRect(rect,r,r); }
    else p.drawRect(rect);
}

void MainWindow::drawBonusFruit(QPainter &p, float cx, float cy, float cw, float ch) {
    float pulse = 0.80f + 0.20f * std::sin(animFrame_ * 0.30f);
    float pw=cw*pulse, ph=ch*pulse;
    float ox=(cw-pw)/2.f, oy=(ch-ph)/2.f;
    float frac = static_cast<float>(bonusTimeLeft_) / BONUS_LIFETIME;
    drawGlow(p, cx+cw/2.f, cy+ch/2.f, cw*2.2f, COL_BONUS, 60);
    p.setPen(QPen(QColor(255,220,60,160), 1.5f)); p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(cx+2,cy+2,cw-4,ch-4), 90*16, -static_cast<int>(360*frac*16));
    p.setBrush(COL_BONUS); p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(cx+ox, cy+oy, pw, ph));
    p.setBrush(QColor(255,240,180,180));
    p.drawEllipse(QRectF(cx+ox+pw*0.18f, cy+oy+ph*0.14f, pw*0.26f, ph*0.22f));
}

void MainWindow::drawShieldOrb(QPainter &p, float cx, float cy, float cw, float ch) {
    float pulse = 0.75f + 0.25f * std::sin(animFrame_ * 0.20f);
    drawGlow(p, cx+cw/2.f, cy+ch/2.f, cw*2.0f, COL_SHIELD, 70);
    float m = cw*(1.f-pulse)*0.5f;
    p.setBrush(COL_SHIELD); p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(cx+m, cy+m, cw-m*2, ch-m*2));
    p.setBrush(QColor(200,240,255,200));
    p.drawEllipse(QRectF(cx+cw*0.30f, cy+ch*0.22f, cw*0.22f, ch*0.18f));
}

void MainWindow::drawShieldAura(QPainter &p, float hx, float hy, float cw, float ch) {
    float pulse = 0.6f + 0.4f * std::sin(animFrame_ * 0.18f);
    QColor auraCol = (shieldFlashTicks_ > 0)
        ? QColor(255,255,255, static_cast<int>(200*pulse))
        : QColor(COL_SHIELD.red(), COL_SHIELD.green(), COL_SHIELD.blue(),
                 static_cast<int>(120*pulse));
    float r = cw * 1.6f;
    drawGlow(p, hx+cw/2.f, hy+ch/2.f, r, auraCol, static_cast<int>(150*pulse));
    p.setPen(QPen(auraCol, 1.8f)); p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(hx+cw/2.f-r*0.7f, hy+ch/2.f-r*0.7f, r*1.4f, r*1.4f));
}

// Small animated snake preview used on the skin-select menu
void MainWindow::drawSnakePreview(QPainter &p, const SnakeSkin &skin,
                                   float cx, float cy, float size) {
    // Draw 4-segment preview snake curled in an S shape
    const std::vector<std::pair<float,float>> segs = {
        {cx + size*0.5f, cy + size*0.1f},
        {cx + size*0.5f, cy + size*0.35f},
        {cx + size*0.3f, cy + size*0.35f},
        {cx + size*0.3f, cy + size*0.60f},
    };
    float r = size * 0.13f;
    for (int i = (int)segs.size()-1; i >= 0; --i) {
        QColor col = (i == 0) ? skin.headColor : skin.bodyColor;
        drawGlow(p, segs[i].first, segs[i].second, r*2.2f, skin.glowColor,
                 (i==0) ? 90 : 50);
        p.setBrush(col); p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(segs[i].first-r, segs[i].second-r, r*2, r*2));
    }
    // Eyes on head
    p.setBrush(Qt::black); p.setPen(Qt::NoPen);
    float er = r * 0.28f;
    p.drawEllipse(QRectF(segs[0].first - r*0.4f - er, segs[0].second - er, er*2, er*2));
    p.drawEllipse(QRectF(segs[0].first + r*0.4f - er, segs[0].second - er, er*2, er*2));
}

// =============================================================================
// Particles
// =============================================================================
void MainWindow::spawnParticles(float px, float py, const QColor &col, int n) {
    for (int i=0; i<n; ++i) {
        Particle pt;
        pt.x=px; pt.y=py;
        float a=(rand()%360)*static_cast<float>(M_PI)/180.f;
        float s=1.5f+(rand()%30)/10.f;
        pt.vx=std::cos(a)*s; pt.vy=std::sin(a)*s;
        pt.life=1.0f; pt.color=col;
        particles_.push_back(pt);
    }
}

void MainWindow::updateParticles() {
    for (auto &pt : particles_) {
        pt.x+=pt.vx; pt.y+=pt.vy; pt.vy+=0.12f; pt.life-=0.045f;
    }
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
                       [](const Particle &p){ return p.life<=0.f; }),
        particles_.end());
}

// =============================================================================
// paintBoard — master paint dispatcher
// =============================================================================
void MainWindow::paintBoard(QPainter &p, int boardW, int boardH) {
    float cw = static_cast<float>(boardW) / BOARD_WIDTH;
    float ch = static_cast<float>(boardH) / BOARD_HEIGHT;
    const auto &grid = board_.getGrid();
    const SnakeSkin &skin = skins_[selectedSkin_];

    // ── Background ────────────────────────────────────────────────────────────
    p.fillRect(0, 0, boardW, boardH, COL_BG);

    // ── Grid lines ────────────────────────────────────────────────────────────
    p.setPen(QPen(COL_GRID, 0.5f));
    for (int x=0;x<=BOARD_WIDTH; ++x) p.drawLine(QPointF(x*cw,0),     QPointF(x*cw,boardH));
    for (int y=0;y<=BOARD_HEIGHT;++y) p.drawLine(QPointF(0,y*ch),     QPointF(boardW,y*ch));

    // ── Wrap arrows on walls ──────────────────────────────────────────────────
    if (wrapAllowed() && state_ == GameState::RUNNING) {
        p.setPen(QColor(skin.headColor.red(), skin.headColor.green(),
                        skin.headColor.blue(), 35));
        p.setFont(QFont("Arial", static_cast<int>(ch*0.45f)));
        for (int x=1;x<BOARD_WIDTH-1; ++x) {
            p.drawText(QRectF(x*cw, 0,    cw, ch), Qt::AlignCenter, "↕");
            p.drawText(QRectF(x*cw, (BOARD_HEIGHT-1)*ch, cw, ch), Qt::AlignCenter, "↕");
        }
        for (int y=1;y<BOARD_HEIGHT-1;++y) {
            p.drawText(QRectF(0,    y*ch, cw, ch), Qt::AlignCenter, "↔");
            p.drawText(QRectF((BOARD_WIDTH-1)*cw, y*ch, cw, ch), Qt::AlignCenter, "↔");
        }
    }

    // ── Cells ─────────────────────────────────────────────────────────────────
    float pulse = 0.85f + 0.15f*std::sin(animFrame_*0.25f);
    QColor wallCol  = (difficulty_ == Difficulty::HARD) ? COL_WALL_H : COL_WALL;
    QColor wallColL = wallCol.lighter(130);

    for (int y=0;y<BOARD_HEIGHT;++y) {
        for (int x=0;x<BOARD_WIDTH;++x) {
            float cx=x*cw, cy=y*ch;
            switch (grid[y][x]) {
                case Cell::WALL: {
                    QLinearGradient wg(cx,cy,cx+cw,cy+ch);
                    wg.setColorAt(0, wallColL); wg.setColorAt(1, wallCol);
                    p.setPen(Qt::NoPen); p.setBrush(wg);
                    p.drawRect(QRectF(cx,cy,cw,ch));
                    break;
                }
                case Cell::SNAKE_HEAD: {
                    if (shieldActive_ || shieldFlashTicks_ > 0)
                        drawShieldAura(p, cx, cy, cw, ch);
                    drawCell(p, cx, cy, cw, ch, skin.headColor, true, cw*2.2f);
                    p.setBrush(Qt::black); p.setPen(Qt::NoPen);
                    float ew=cw*0.13f, eh=ch*0.13f, ox=cw*0.27f, oy=ch*0.28f;
                    p.drawEllipse(QRectF(cx+ox,      cy+oy, ew, eh));
                    p.drawEllipse(QRectF(cx+cw-ox-ew,cy+oy, ew, eh));
                    break;
                }
                case Cell::SNAKE_BODY:
                    drawCell(p, cx, cy, cw, ch, skin.bodyColor, true, cw*0.7f);
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
                    float bp=0.7f+0.3f*std::abs(std::sin(animFrame_*0.15f));
                    QColor bc(static_cast<int>(COL_OBSTACLE.red()*bp),
                              static_cast<int>(COL_OBSTACLE.green()*bp),
                              static_cast<int>(COL_OBSTACLE.blue()*bp));
                    drawGlow(p, cx+cw/2.f, cy+ch/2.f, cw*1.4f, COL_OBSTACLE, 55);
                    float m=cw*0.08f;
                    p.setPen(QPen(bc.darker(150),1.f)); p.setBrush(bc);
                    p.drawRect(QRectF(cx+m,cy+m,cw-m*2,ch-m*2));
                    p.setPen(QPen(QColor(255,200,100,200),1.2f));
                    p.drawLine(QPointF(cx+m*2,cy+m*2), QPointF(cx+cw-m*2,cy+ch-m*2));
                    p.drawLine(QPointF(cx+cw-m*2,cy+m*2), QPointF(cx+m*2,cy+ch-m*2));
                    break;
                }
                default: break;
            }
        }
    }

    // ── Bonus / Shield overlays ───────────────────────────────────────────────
    if (bonusActive_ && bonusPos_.first > 0)
        drawBonusFruit(p, bonusPos_.first*cw, bonusPos_.second*ch, cw, ch);
    if (shieldPickupOnBoard_ && shieldPos_.first > 0)
        drawShieldOrb(p, shieldPos_.first*cw, shieldPos_.second*ch, cw, ch);

    // ── Particles ─────────────────────────────────────────────────────────────
    p.setPen(Qt::NoPen);
    for (const auto &pt : particles_) {
        QColor c=pt.color; c.setAlphaF(pt.life);
        p.setBrush(c);
        float sz=3.f*pt.life;
        p.drawEllipse(QRectF(pt.x-sz/2, pt.y-sz/2, sz, sz));
    }

    // ── Scanlines ─────────────────────────────────────────────────────────────
    for (int sy=0; sy<boardH; sy+=4)
        p.fillRect(0, sy, boardW, 1, QColor(0,0,0,18));

    // ── Border (colour reflects skin + state) ─────────────────────────────────
    QColor bc2 = shieldActive_ ? COL_SHIELD
               : boosting_     ? COL_BONUS
               : (state_==GameState::RUNNING) ? skin.glowColor : QColor(80,80,120);
    bc2.setAlpha(static_cast<int>(80+40*std::sin(animFrame_*0.08f)));
    p.setPen(QPen(bc2,2.f)); p.setBrush(Qt::NoBrush);
    p.drawRect(1,1,boardW-2,boardH-2);

    // Boost strip
    if (boosting_) {
        float a=40+30*std::sin(animFrame_*0.3f);
        p.fillRect(0,0,boardW,static_cast<int>(ch*0.35f), QColor(255,220,0,static_cast<int>(a)));
    }

    // ── Overlay screens ───────────────────────────────────────────────────────
    if (state_==GameState::MENU || state_==GameState::PAUSED || state_==GameState::GAME_OVER) {
        p.setBrush(COL_OVERLAY); p.setPen(Qt::NoPen);
        p.drawRect(0,0,boardW,boardH);

        if      (state_==GameState::MENU)      paintMenuMain(p, boardW, boardH);
        else if (state_==GameState::PAUSED)    {
            p.setFont(QFont("Courier New",30,QFont::Bold));
            p.setPen(QColor(100,180,255));
            p.drawText(QRectF(0,boardH*0.32f,boardW,50),Qt::AlignCenter,"PAUSED");
            p.setFont(QFont("Courier New",12));
            p.setPen(QColor(140,140,200));
            p.drawText(QRectF(0,boardH*0.52f,boardW,30),Qt::AlignCenter,
                       "P=resume  R=restart  Q=quit");
        } else {
            // GAME OVER
            float pr=180+75*std::abs(std::sin(animFrame_*0.05f));
            p.setFont(QFont("Courier New",32,QFont::Bold));
            p.setPen(QColor(static_cast<int>(pr),30,50));
            p.drawText(QRectF(0,boardH*0.20f,boardW,56),Qt::AlignCenter,"GAME OVER");

            // Skin name badge
            p.setFont(QFont("Courier New",11));
            p.setPen(skin.headColor);
            p.drawText(QRectF(0,boardH*0.34f,boardW,26),Qt::AlignCenter,
                       QString("Skin: %1").arg(skin.name));

            p.setFont(QFont("Courier New",15,QFont::Bold));
            p.setPen(QColor(220,220,255));
            p.drawText(QRectF(0,boardH*0.44f,boardW,32),Qt::AlignCenter,
                       QString("Score: %1   Level: %2").arg(score_).arg(level_));

            if (newHighScore_ && (animFrame_/10)%2==0) {
                p.setFont(QFont("Courier New",12,QFont::Bold));
                p.setPen(QColor(255,200,0));
                p.drawText(QRectF(0,boardH*0.57f,boardW,28),Qt::AlignCenter,"✦ NEW HIGH SCORE! ✦");
            }
            if ((animFrame_/20)%2==0) {
                p.setFont(QFont("Courier New",12));
                p.setPen(QColor(160,255,160));
                p.drawText(QRectF(0,boardH*0.70f,boardW,28),Qt::AlignCenter,"R=restart  Q=quit");
            }
            animFrame_++;
            QTimer::singleShot(50,ui->gameBoard,[this](){ ui->gameBoard->update(); });
        }
    }
}

// =============================================================================
// paintMenuMain — full menu with skin + difficulty selectors
// =============================================================================
void MainWindow::paintMenuMain(QPainter &p, float bw, float bh) {
    const SnakeSkin &curSkin = skins_[selectedSkin_];

    // ── Title ─────────────────────────────────────────────────────────────────
    p.setFont(QFont("Courier New", 34, QFont::Bold));
    for (int g=3;g>=1;g--) {
        p.setPen(QColor(curSkin.headColor.red(), curSkin.headColor.green(),
                        curSkin.headColor.blue(), 25*g));
        p.drawText(QRectF(g*2, bh*0.05f-g*2, bw-g*4, 55), Qt::AlignCenter, "SNAKE");
    }
    p.setPen(curSkin.headColor);
    p.drawText(QRectF(0, bh*0.05f, bw, 55), Qt::AlignCenter, "SNAKE");

    p.setFont(QFont("Courier New", 10));
    p.setPen(QColor(140,140,200));
    p.drawText(QRectF(0, bh*0.20f, bw, 22), Qt::AlignCenter, "N E O N   A R C A D E");

    // ─────────────────────────────────────────────────────────────────────────
    // ── 🎨 SKIN SELECTOR ─────────────────────────────────────────────────────
    // ─────────────────────────────────────────────────────────────────────────
    p.setFont(QFont("Courier New", 9, QFont::Bold));
    p.setPen(QColor(100,100,160));
    p.drawText(QRectF(0, bh*0.27f, bw, 20), Qt::AlignCenter, "◄  SELECT SKIN  ►   ( ← / → )");

    // Draw 5 skin slots across the middle
    float slotW = bw / 6.f;
    float slotY = bh * 0.33f;
    float slotH = bh * 0.20f;

    for (int i=0; i<(int)skins_.size(); ++i) {
        float sx = (bw/2.f) + (i - selectedSkin_) * slotW * 1.1f - slotW/2.f;
        float sy = slotY;
        bool  isSel = (i == selectedSkin_);

        // Slot background
        float alpha = isSel ? 80 : 30;
        p.setBrush(QColor(skins_[i].headColor.red(),
                          skins_[i].headColor.green(),
                          skins_[i].headColor.blue(),
                          static_cast<int>(alpha)));
        p.setPen(isSel ? QPen(skins_[i].headColor, 2.f) : QPen(Qt::NoPen));
        float rr = slotW*0.12f;
        p.drawRoundedRect(QRectF(sx, sy, slotW, slotH), rr, rr);

        // Animated snake preview inside slot
        drawSnakePreview(p, skins_[i], sx, sy, slotW);

        // Skin name below slot (only selected)
        if (isSel) {
            p.setFont(QFont("Courier New", 9, QFont::Bold));
            p.setPen(skins_[i].headColor);
            p.drawText(QRectF(sx - slotW*0.3f, sy + slotH + 4, slotW*1.6f, 20),
                       Qt::AlignCenter, skins_[i].name);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // ── 🌡️ DIFFICULTY SELECTOR ───────────────────────────────────────────────
    // ─────────────────────────────────────────────────────────────────────────
    float diffY = bh * 0.60f;

    p.setFont(QFont("Courier New", 9, QFont::Bold));
    p.setPen(QColor(100,100,160));
    p.drawText(QRectF(0, diffY, bw, 20), Qt::AlignCenter, "▲  SELECT DIFFICULTY  ▼   ( ↑ / ↓ )");

    // Three difficulty badges
    const char* diffLabels[3] = { "EASY", "NORMAL", "HARD" };
    const char* diffDesc[3]   = {
        "Slow · No bombs til L5 · Shield on · Wrap on",
        "Normal · Bombs L3+ · Shield 60% · Wrap on",
        "Fast · Bombs from L2 · No shield · No wrap"
    };
    const QColor diffCols[3] = { COL_EASY, COL_NORMAL, COL_HARD };

    float bxW = bw / 4.f;
    float bxY = diffY + 24;
    float bxH = bh * 0.09f;

    for (int i=0; i<3; ++i) {
        float bx = bw/2.f + (i-1)*bxW*1.15f - bxW/2.f;
        bool  isSel = (i == selectedDiff_);

        p.setBrush(QColor(diffCols[i].red(), diffCols[i].green(), diffCols[i].blue(),
                          isSel ? 70 : 20));
        p.setPen(isSel ? QPen(diffCols[i], 2.f) : QPen(diffCols[i].darker(150), 1.f));
        p.drawRoundedRect(QRectF(bx, bxY, bxW, bxH), 6, 6);

        p.setFont(QFont("Courier New", isSel ? 12 : 10, QFont::Bold));
        p.setPen(diffCols[i]);
        p.drawText(QRectF(bx, bxY + bxH*0.15f, bxW, bxH*0.55f),
                   Qt::AlignCenter, diffLabels[i]);
    }

    // Description of selected difficulty
    p.setFont(QFont("Courier New", 8));
    p.setPen(QColor(130,130,180));
    p.drawText(QRectF(0, bxY + bxH + 6, bw, 20),
               Qt::AlignCenter, diffDesc[selectedDiff_]);

    // ── PRESS SPACE ───────────────────────────────────────────────────────────
    if ((animFrame_/20)%2==0) {
        p.setFont(QFont("Courier New", 13, QFont::Bold));
        p.setPen(QColor(200,200,255));
        p.drawText(QRectF(0, bh*0.84f, bw, 30), Qt::AlignCenter, "PRESS SPACE TO START");
    }

    // Best score
    if (scoreManager_.getHighScore() > 0) {
        p.setFont(QFont("Courier New", 9));
        p.setPen(QColor(160,130,10));
        p.drawText(QRectF(0, bh*0.91f, bw, 22), Qt::AlignCenter,
                   QString("BEST: %1").arg(scoreManager_.getHighScore()));
    }

    p.setFont(QFont("Courier New", 8));
    p.setPen(QColor(50,50,90));
    p.drawText(QRectF(0, bh*0.96f, bw, 18), Qt::AlignCenter,
               "W/A/S/D · P=Pause · Q=Quit  |  ←/→ Skin   ↑/↓ Diff");

    animFrame_++;
    QTimer::singleShot(50, ui->gameBoard, [this](){ ui->gameBoard->update(); });
}