#include "MainWindow.h"
#include <QPainter>
#include <QFont>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <cstdlib>

static const QColor COL_BG       { 15,  15,  20 };
static const QColor COL_GRID     { 30,  30,  40 };
static const QColor COL_WALL     { 80,  80, 100 };
static const QColor COL_SNAKE    { 50, 220,  80 };
static const QColor COL_HEAD     { 30, 180,  50 };
static const QColor COL_FOOD     { 255, 60,  60 };
static const QColor COL_OBSTACLE { 210, 100, 30 }; // Orange block

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), timer_(new QTimer(this)),
      snake_(BOARD_WIDTH / 2, BOARD_HEIGHT / 2), scoreManager_("data/highscore.txt")
{
    setupUI();
    connect(timer_, &QTimer::timeout, this, &MainWindow::gameTick);
    resize(680, 580);
    resetGame();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #0d0d22; color: white;");
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *hudLayout = new QHBoxLayout();

    scoreValueLabel_ = new QLabel("0");
    levelValueLabel_ = new QLabel("1");
    hsValueLabel_ = new QLabel("0");
    progressValueLabel_ = new QLabel("🍎 0/5");

    QFont hudFont("Arial", 12, QFont::Bold);
    scoreValueLabel_->setFont(hudFont);
    levelValueLabel_->setFont(hudFont);
    hsValueLabel_->setFont(hudFont);
    progressValueLabel_->setFont(hudFont);

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
    return std::max(50, 200 - (lvl - 1) * 15);
}

void MainWindow::spawnObstaclesForLevel(int lvl) {
    if (lvl < 2) return;
    int count = (lvl - 1) * 2; // Spawn 2 new obstacles every level
    auto [hx, hy] = snake_.getHead();

    for (int i = 0; i < count; ++i) {
        int rx, ry;
        do {
            rx = 1 + rand() % (BOARD_WIDTH - 2);
            ry = 1 + rand() % (BOARD_HEIGHT - 2);
        // Avoid spawning directly on the snake head or existing non-empty spaces
        } while ((std::abs(rx - hx) < 3 && std::abs(ry - hy) < 3) || board_.getGrid()[ry][rx] != Cell::EMPTY);

        board_.addObstacle(rx, ry);
    }
}

void MainWindow::resetGame() {
    snake_     = Snake(BOARD_WIDTH / 2, BOARD_HEIGHT / 2);
    score_     = 0;
    level_     = 1;
    foodEaten_ = 0;
    speed_     = speedForLevel(level_);
    state_     = GameState::MENU;

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
    progressValueLabel_->setText(QString("🍎 %1/%2").arg(foodEaten_).arg(FOOD_PER_LEVEL));
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

void MainWindow::gameTick() {
    if (state_ != GameState::RUNNING) return;
    inputRegisteredThisTick_ = false;
    snake_.move();
    checkCollisions();
    if (state_ == GameState::RUNNING) board_.update(snake_, food_);
    updateHUD();
    gameBoard_->update();
}

void MainWindow::checkCollisions() {
    auto [hx, hy] = snake_.getHead();

    if (hx <= 0 || hx >= BOARD_WIDTH-1 || hy <= 0 || hy >= BOARD_HEIGHT-1 ||
        board_.isObstacle(hx, hy) || snake_.checkSelfCollision()) {
        state_ = GameState::GAME_OVER;
        timer_->stop();
        scoreManager_.update(score_);
        return;
    }

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
            QRectF rect(x*cw + 1, y*ch + 1, cw - 2, ch - 2);
            p.setPen(Qt::NoPen);
            switch (grid[y][x]) {
                case Cell::WALL:       p.setBrush(COL_WALL);     p.drawRect(rect); break;
                case Cell::SNAKE_HEAD: p.setBrush(COL_HEAD);     p.drawRect(rect); break;
                case Cell::SNAKE_BODY: p.setBrush(COL_SNAKE);    p.drawRect(rect); break;
                case Cell::FOOD:       p.setBrush(COL_FOOD);     p.drawEllipse(rect); break;
                case Cell::OBSTACLE:   p.setBrush(COL_OBSTACLE); p.drawRect(rect); break;
                default: break;
            }
        }
    }

    if (state_ != GameState::RUNNING) {
        p.fillRect(0, 0, boardW, boardH, QColor(0, 0, 0, 180));
        p.setPen(Qt::white);
        p.setFont(QFont("Courier New", 24, QFont::Bold));

        if (state_ == GameState::MENU) {
            p.drawText(0, 0, boardW, boardH, Qt::AlignCenter, "CLASSIC SNAKE\n\nPress SPACE to Start");
        } else if (state_ == GameState::PAUSED) {
            p.drawText(0, 0, boardW, boardH, Qt::AlignCenter, "PAUSED\n\nPress P to Resume");
        } else if (state_ == GameState::GAME_OVER) {
            p.drawText(0, 0, boardW, boardH, Qt::AlignCenter,
                QString("GAME OVER\nScore: %1\n\nPress SPACE to Restart").arg(score_));
        }
    }
}