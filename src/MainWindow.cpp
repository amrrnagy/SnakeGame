#include "MainWindow.h"
#include <QPainter>
#include <QRadialGradient>
#include <QFont>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <cstdlib>

static const QColor COL_BG       { 10,  10,  18 };
static const QColor COL_GRID     { 20,  25,  40 };
static const QColor COL_WALL     { 40,  40,  60 };
static const QColor COL_SNAKE    { 0,  240, 255 };
static const QColor COL_HEAD     { 57, 255,  20 };
static const QColor COL_FOOD     { 255, 20, 100 };
static const QColor COL_OBSTACLE { 255, 80,  20 };
static const QColor COL_BONUS    { 255, 220, 40 };
static const QColor COL_SHIELD   { 80, 160, 255 }; // Cyan/Blue Shield

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), timer_(new QTimer(this)),
      snake_(BOARD_WIDTH / 2, BOARD_HEIGHT / 2), scoreManager_("data/highscore.txt")
{
    setupUI();
    connect(timer_, &QTimer::timeout, this, &MainWindow::gameTick);

    // 750 width / 500 board height = 1.5 Aspect Ratio! (Perfect squares for 30x20)
    // The extra 60px height is reserved for the HUD.
    resize(750, 560);
    resetGame();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #0a0a12; color: #00f3ff;");
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *hudLayout = new QHBoxLayout();

    scoreValueLabel_ = new QLabel("0");
    levelValueLabel_ = new QLabel("1");
    hsValueLabel_ = new QLabel("0");
    progressValueLabel_ = new QLabel("🍎 0/5");

    QFont hudFont("Courier New", 14, QFont::Bold);
    scoreValueLabel_->setFont(hudFont);
    levelValueLabel_->setFont(hudFont);
    hsValueLabel_->setFont(hudFont);
    progressValueLabel_->setFont(hudFont);
    progressValueLabel_->setStyleSheet("color: #ff1493;");

    hudLayout->addWidget(new QLabel("SCORE:"));
    hudLayout->addWidget(scoreValueLabel_);
    hudLayout->addWidget(new QLabel("LEVEL:"));
    hudLayout->addWidget(levelValueLabel_);
    hudLayout->addWidget(progressValueLabel_);
    hudLayout->addStretch();
    hudLayout->addWidget(new QLabel("BEST:"));
    hudLayout->addWidget(hsValueLabel_);

    mainLayout->addLayout(hudLayout);

    gameBoard_ = new QWidget();
    gameBoard_->installEventFilter(this);
    gameBoard_->setFocusPolicy(Qt::StrongFocus);
    mainLayout->addWidget(gameBoard_, 1);
}

int MainWindow::speedForLevel(int lvl) const {
    return std::max(60, 200 - (lvl - 1) * 15);
}

void MainWindow::spawnObstaclesForLevel(int lvl) {
    if (lvl < 2) return;
    int count = (lvl - 1) * 2;
    auto [hx, hy] = snake_.getHead();

    for (int i = 0; i < count; ++i) {
        int rx, ry;
        do {
            rx = 1 + rand() % (BOARD_WIDTH - 2);
            ry = 1 + rand() % (BOARD_HEIGHT - 2);
        } while ((std::abs(rx - hx) < 3 && std::abs(ry - hy) < 3) || board_.getGrid()[ry][rx] != Cell::EMPTY);

        board_.addObstacle(rx, ry);
    }
}

void MainWindow::spawnBonus() {
    std::vector<Position> cands;
    const auto &grid = board_.getGrid();
    for (int y = 1; y < BOARD_HEIGHT - 1; ++y) {
        for (int x = 1; x < BOARD_WIDTH - 1; ++x) {
            if (grid[y][x] == Cell::EMPTY && Position{x, y} != food_.getPosition() && Position{x, y} != shieldPos_) {
                cands.push_back({x, y});
            }
        }
    }
    if (!cands.empty()) {
        bonusPos_ = cands[rand() % cands.size()];
        bonusTimeLeft_ = 40;
    }
}

void MainWindow::spawnShield() {
    if (shieldActive_ || shieldPos_.first > 0) return;
    std::vector<Position> cands;
    const auto &grid = board_.getGrid();
    for (int y = 1; y < BOARD_HEIGHT - 1; ++y) {
        for (int x = 1; x < BOARD_WIDTH - 1; ++x) {
            if (grid[y][x] == Cell::EMPTY && Position{x, y} != food_.getPosition() && Position{x, y} != bonusPos_) {
                cands.push_back({x, y});
            }
        }
    }
    if (!cands.empty()) {
        shieldPos_ = cands[rand() % cands.size()];
    }
}

void MainWindow::resetGame() {
    snake_         = Snake(BOARD_WIDTH / 2, BOARD_HEIGHT / 2);
    score_         = 0;
    level_         = 1;
    foodEaten_     = 0;
    bonusPos_      = {-1, -1};
    bonusTimeLeft_ = 0;
    shieldPos_     = {-1, -1};
    shieldActive_  = false;
    speed_         = speedForLevel(level_);
    state_         = GameState::MENU;

    board_.resetObstacles();
    board_.resetInner();
    board_.update(snake_, food_);
    food_.spawn(board_.getGrid());

    timer_->stop();
    updateHUD();
    gameBoard_->update();
}

void MainWindow::updateHUD() {
    scoreValueLabel_->setText(QString::number(score_));
    levelValueLabel_->setText(QString::number(level_));
    hsValueLabel_->setText(QString::number(scoreManager_.getHighScore()));

    QString progText = QString("🍎 %1/%2").arg(foodEaten_).arg(FOOD_PER_LEVEL);
    if (bonusTimeLeft_ > 0) progText += " 🌟";
    if (shieldActive_)      progText += " 🛡️";
    progressValueLabel_->setText(progText);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == gameBoard_ && event->type() == QEvent::Paint) {
        QPainter painter(gameBoard_);
        painter.setRenderHint(QPainter::Antialiasing, true);
        paintBoard(painter, gameBoard_->width(), gameBoard_->height());
        return true;
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::wrapPosition() {
    auto [hx, hy] = snake_.getHead();
    bool wrapped = false;

    if (hx <= 0)                      { hx = BOARD_WIDTH - 2;  wrapped = true; }
    else if (hx >= BOARD_WIDTH - 1)   { hx = 1;                wrapped = true; }
    if (hy <= 0)                      { hy = BOARD_HEIGHT - 2; wrapped = true; }
    else if (hy >= BOARD_HEIGHT - 1)  { hy = 1;                wrapped = true; }

    if (wrapped) snake_.setHeadPosition(hx, hy);
}

void MainWindow::gameTick() {
    if (state_ != GameState::RUNNING) return;
    inputRegisteredThisTick_ = false;

    if (bonusTimeLeft_ > 0) {
        bonusTimeLeft_--;
        if (bonusTimeLeft_ <= 0) bonusPos_ = {-1, -1}; // Disappears cleanly
    }

    snake_.move();
    wrapPosition();
    checkCollisions();

    if (state_ == GameState::RUNNING) board_.update(snake_, food_);
    updateHUD();
    gameBoard_->update();
}

void MainWindow::checkCollisions() {
    auto [hx, hy] = snake_.getHead();

    // Shield completely protects you from Obstacles by destroying them.
    // However, hitting your own body is always fatal.
    if (board_.isObstacle(hx, hy)) {
        if (shieldActive_) {
            shieldActive_ = false;
            board_.removeObstacle(hx, hy);
        } else {
            state_ = GameState::GAME_OVER;
            timer_->stop();
            scoreManager_.update(score_);
            return;
        }
    } else if (snake_.checkSelfCollision()) {
        state_ = GameState::GAME_OVER;
        timer_->stop();
        scoreManager_.update(score_);
        return;
    }

    // Check Shield Pickup
    if (shieldPos_.first > 0 && snake_.getHead() == shieldPos_) {
        shieldActive_ = true;
        shieldPos_ = {-1, -1};
    }

    // Check Bonus
    if (bonusTimeLeft_ > 0 && snake_.getHead() == bonusPos_) {
        score_ += 50 * level_;
        scoreManager_.update(score_);
        bonusTimeLeft_ = 0;
        bonusPos_ = {-1, -1};
        snake_.grow();
    }

    // Check Regular Food
    if (snake_.getHead() == food_.getPosition()) {
        snake_.grow();
        score_ += 10 * level_;
        foodEaten_++;
        scoreManager_.update(score_);

        if (foodEaten_ >= FOOD_PER_LEVEL) {
            foodEaten_ = 0;
            level_++;
            speed_ = speedForLevel(level_);
            timer_->setInterval(speed_);
            spawnObstaclesForLevel(level_);
        }

        // Random spawns
        if (bonusTimeLeft_ <= 0 && rand() % 100 < 25) spawnBonus();
        if (!shieldActive_ && shieldPos_.first < 0 && rand() % 100 < 15) spawnShield();

        board_.update(snake_, food_);
        food_.spawn(board_.getGrid());
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Q || event->key() == Qt::Key_Escape) {
        QApplication::quit(); return;
    }
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_R) {
        if (state_ == GameState::MENU || state_ == GameState::GAME_OVER) {
            resetGame();
            state_ = GameState::RUNNING;
            timer_->start(speed_);
        }
        return;
    }

    if (state_ == GameState::RUNNING && !inputRegisteredThisTick_) {
        switch (event->key()) {
            case Qt::Key_W: case Qt::Key_Up:    snake_.setDirection(Direction::UP);    break;
            case Qt::Key_S: case Qt::Key_Down:  snake_.setDirection(Direction::DOWN);  break;
            case Qt::Key_A: case Qt::Key_Left:  snake_.setDirection(Direction::LEFT);  break;
            case Qt::Key_D: case Qt::Key_Right: snake_.setDirection(Direction::RIGHT); break;
            case Qt::Key_P:
                state_ = GameState::PAUSED; timer_->stop(); gameBoard_->update(); return;
            default: return;
        }
        inputRegisteredThisTick_ = true;
    } else if (state_ == GameState::PAUSED && event->key() == Qt::Key_P) {
        state_ = GameState::RUNNING; timer_->start(speed_);
    }
}

void MainWindow::drawGlow(QPainter &p, float cx, float cy, float r, const QColor &col) {
    QRadialGradient g(cx, cy, r);
    QColor cStart = col; cStart.setAlpha(90);
    QColor cEnd = col;   cEnd.setAlpha(0);
    g.setColorAt(0, cStart);
    g.setColorAt(1, cEnd);
    p.setBrush(g);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(cx, cy), r, r);
}

void MainWindow::paintBoard(QPainter &p, int boardW, int boardH) {
    float cw = static_cast<float>(boardW) / BOARD_WIDTH;
    float ch = static_cast<float>(boardH) / BOARD_HEIGHT;
    const auto &grid = board_.getGrid();

    p.fillRect(0, 0, boardW, boardH, COL_BG);

    p.setPen(QPen(COL_GRID, 1));
    for (int x=0; x<=BOARD_WIDTH; ++x) p.drawLine(x*cw, 0, x*cw, boardH);
    for (int y=0; y<=BOARD_HEIGHT; ++y) p.drawLine(0, y*ch, boardW, y*ch);

    for (int y=0; y<BOARD_HEIGHT; ++y) {
        for (int x=0; x<BOARD_WIDTH; ++x) {
            float cx = x*cw + cw/2.0f;
            float cy = y*ch + ch/2.0f;
            QRectF rect(x*cw + 2, y*ch + 2, cw - 4, ch - 4);

            p.setPen(Qt::NoPen);
            switch (grid[y][x]) {
                case Cell::WALL:
                    p.setBrush(COL_WALL); p.drawRect(rect);
                    break;
                case Cell::SNAKE_HEAD:
                    // Draw a protective blue aura if shield is active
                    if (shieldActive_) drawGlow(p, cx, cy, cw * 2.0f, COL_SHIELD);
                    else drawGlow(p, cx, cy, cw * 1.5f, COL_HEAD);
                    p.setBrush(COL_HEAD); p.drawRoundedRect(rect, 4, 4);
                    break;
                case Cell::SNAKE_BODY:
                    drawGlow(p, cx, cy, cw * 1.2f, COL_SNAKE);
                    p.setBrush(COL_SNAKE); p.drawRoundedRect(rect, 4, 4);
                    break;
                case Cell::FOOD:
                    drawGlow(p, cx, cy, cw * 1.8f, COL_FOOD);
                    p.setBrush(COL_FOOD); p.drawEllipse(rect);
                    break;
                case Cell::OBSTACLE:
                    drawGlow(p, cx, cy, cw * 1.3f, COL_OBSTACLE);
                    p.setBrush(COL_OBSTACLE); p.drawRect(rect);
                    break;
                default: break;
            }
        }
    }

    // Draw Bonus Fruit (Blinks rapidly when under 10 ticks)
    if (bonusPos_.first > 0 && bonusTimeLeft_ > 0) {
        if (bonusTimeLeft_ > 10 || (bonusTimeLeft_ % 2 == 0)) {
            float cx = bonusPos_.first * cw + cw/2.0f;
            float cy = bonusPos_.second * ch + ch/2.0f;
            QRectF rect(bonusPos_.first*cw + 4, bonusPos_.second*ch + 4, cw - 8, ch - 8);
            drawGlow(p, cx, cy, cw * 2.2f, COL_BONUS);
            p.setBrush(COL_BONUS);
            p.drawEllipse(rect);
        }
    }

    // Draw Shield Pickup
    if (shieldPos_.first > 0 && !shieldActive_) {
        float cx = shieldPos_.first * cw + cw/2.0f;
        float cy = shieldPos_.second * ch + ch/2.0f;
        QRectF rect(shieldPos_.first*cw + 4, shieldPos_.second*ch + 4, cw - 8, ch - 8);
        drawGlow(p, cx, cy, cw * 1.8f, COL_SHIELD);
        p.setBrush(COL_SHIELD);
        p.drawEllipse(rect);
    }

    if (state_ != GameState::RUNNING) {
        p.fillRect(0, 0, boardW, boardH, QColor(0, 0, 0, 200));
        p.setPen(COL_SNAKE);
        p.setFont(QFont("Courier New", 26, QFont::Bold));

        if (state_ == GameState::MENU) {
            p.drawText(0, 0, boardW, boardH, Qt::AlignCenter, "NEON SNAKE\n\nPress SPACE to Start");
        } else if (state_ == GameState::PAUSED) {
            p.drawText(0, 0, boardW, boardH, Qt::AlignCenter, "PAUSED\n\nPress P to Resume");
        } else if (state_ == GameState::GAME_OVER) {
            p.setPen(COL_FOOD);
            p.drawText(0, 0, boardW, boardH, Qt::AlignCenter,
                QString("GAME OVER\nScore: %1\n\nPress SPACE to Restart").arg(score_));
        }
    }
}