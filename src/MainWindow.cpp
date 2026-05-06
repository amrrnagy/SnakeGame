//
// Created by Ahmed Ayman on 5/6/2026.
//#include "MainWindow.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QPainter>
#include <QMessageBox>

// Initialize the snake in the center, just like Game.cpp
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , timer_(new QTimer(this))
    , snake_(BOARD_WIDTH / 2, BOARD_HEIGHT / 2) //[cite: 3, 15]
    , scoreManager_("data/highscore.txt")       //[cite: 11, 17]
{
    ui->setupUi(this);

    // Connect the timer to our gameTick function
    connect(timer_, &QTimer::timeout, this, &MainWindow::gameTick);

    // Initialize the game
    resetGame();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::resetGame() {
    snake_ = Snake(BOARD_WIDTH / 2, BOARD_HEIGHT / 2);
    score_ = 0;
    level_ = 1;
    speed_ = 200;

    // START IN THE MENU STATE INSTEAD OF RUNNING
    state_ = GameState::MENU;

    board_.resetInner();
    board_.update(snake_, food_);
    food_.spawn(board_.getGrid());

    updateLabels();

    // Stop the timer so the game is completely frozen on the menu
    timer_->stop();
}

int MainWindow::speedForLevel(int lvl) const {
    return std::max(50, 200 - (lvl - 1) * 20); //[cite: 3, 15]
}

void MainWindow::updateLabels() {
    QString labelText = QString("Score: %1  |  Level: %2  |  High Score: %3")
                        .arg(score_)
                        .arg(level_)
                        .arg(scoreManager_.getHighScore());
    ui->scoreLabel->setText(labelText);
}

void MainWindow::gameTick() {
    if (state_ == GameState::RUNNING) {
        snake_.move();
        checkCollisions();

        if (state_ == GameState::RUNNING) {
            board_.update(snake_, food_);
        }

        // Trigger a repaint of the window
        update();
    }
}

void MainWindow::checkCollisions() {
    auto [hx, hy] = snake_.getHead();

    // 1. Wall or Obstacle collision
    if (board_.isWall(hx, hy) || board_.isObstacle(hx, hy) || snake_.checkSelfCollision()) {
        state_ = GameState::GAME_OVER;
        timer_->stop();
        scoreManager_.update(score_); // Save high score
        update(); // Force one last draw to show Game Over text
        return;
    }

    // 2. Food collision
    if (snake_.getHead() == food_.getPosition()) {
        snake_.grow();
        score_ += 10 * level_;
        scoreManager_.update(score_);

        // Level up logic[cite: 3, 15]
        int newLevel = (score_ / (50 * level_)) + 1;
        if (newLevel > level_) {
            level_ = newLevel;
            speed_ = speedForLevel(level_);
            timer_->setInterval(speed_); // Update Qt Timer speed
            spawnObstaclesForLevel(level_);
        }

        board_.update(snake_, food_);
        food_.spawn(board_.getGrid());
        updateLabels();
    }
}

void MainWindow::spawnObstaclesForLevel(int lvl) {
    if (lvl < 3) return;
    int count = (lvl - 2) * 2; //[cite: 3, 15]
    for (int i = 0; i < count; ++i) {
        int rx = 1 + rand() % (BOARD_WIDTH - 2);
        int ry = 1 + rand() % (BOARD_HEIGHT - 2);
        board_.addObstacle(rx, ry);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    // If we are on the main menu, press Space or Enter to start
    if (state_ == GameState::MENU) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
            state_ = GameState::RUNNING;
            timer_->start(speed_); // Start the game loop!
            update();
        }
        return; // Ignore all other keys on the main menu
    }
    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_Up:    snake_.setDirection(Direction::UP); break;
        case Qt::Key_S:
        case Qt::Key_Down:  snake_.setDirection(Direction::DOWN); break;
        case Qt::Key_A:
        case Qt::Key_Left:  snake_.setDirection(Direction::LEFT); break;
        case Qt::Key_D:
        case Qt::Key_Right: snake_.setDirection(Direction::RIGHT); break;

        case Qt::Key_P:
            if (state_ == GameState::RUNNING) {
                state_ = GameState::PAUSED;
                timer_->stop();
            } else if (state_ == GameState::PAUSED) {
                state_ = GameState::RUNNING;
                timer_->start(speed_);
            }
            update(); // Redraw to show/hide pause text
            break;

        case Qt::Key_R:
            if (state_ == GameState::GAME_OVER) {
                resetGame();
            }
            break;

        case Qt::Key_Q:
            QApplication::quit();
            break;
    }
}

void MainWindow::paintEvent(QPaintEvent *event) {
    QMainWindow::paintEvent(event);
    QPainter painter(this);

    // Get the area where the game board should be drawn
    QRect boardRect = ui->gameBoard->geometry();
    painter.translate(boardRect.topLeft()); // Shift drawing origin to the game board frame

    // Calculate dynamic cell sizes based on the window size
    float cellWidth = static_cast<float>(boardRect.width()) / BOARD_WIDTH;
    float cellHeight = static_cast<float>(boardRect.height()) / BOARD_HEIGHT;

    const auto& grid = board_.getGrid();

    // Draw the grid
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            QRectF cell(x * cellWidth, y * cellHeight, cellWidth, cellHeight);

            switch (grid[y][x]) {
                case Cell::WALL:
                    painter.fillRect(cell, Qt::darkGray);
                    break;
                case Cell::SNAKE_HEAD:
                    painter.fillRect(cell, Qt::darkGreen);
                    break;
                case Cell::SNAKE_BODY:
                    painter.fillRect(cell, Qt::green);
                    break;
                case Cell::FOOD:
                    painter.fillRect(cell, Qt::red);
                    break;
                case Cell::OBSTACLE:
                    painter.fillRect(cell, QColor(139, 69, 19)); // SaddleBrown
                    break;
                default:
                    // Draw a faint grid line for EMPTY cells
                    painter.setPen(QColor(30, 30, 30));
                    painter.drawRect(cell);
                    break;
            }
        }
    }

    // Draw Overlays (Paused / Game Over)
    // Draw Overlays (Menu / Paused / Game Over)
    if (state_ == GameState::MENU || state_ == GameState::PAUSED || state_ == GameState::GAME_OVER) {
        painter.setBrush(QColor(0, 0, 0, 200)); // Darker transparent black
        painter.drawRect(0, 0, boardRect.width(), boardRect.height());

        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 24, QFont::Bold));

        QString message;
        if (state_ == GameState::MENU) {
            painter.setPen(Qt::green); // Make the title green!
            message = "SNAKE GAME\n\nPress SPACE to Start";
        } else if (state_ == GameState::PAUSED) {
            message = "PAUSED\nPress P to Resume";
        } else {
            message = "GAME OVER\nPress R to Restart";
        }

        painter.drawText(QRectF(0, 0, boardRect.width(), boardRect.height()),
                         Qt::AlignCenter, message);
    }
}