#include "Snake.h"

Snake::Snake(int startX, int startY)
    : dir_(Direction::RIGHT), growing_(false)
{
    body_.push_back({ startX,     startY });
    body_.push_back({ startX - 1, startY });
    body_.push_back({ startX - 2, startY });
}

void Snake::move() {
    Position head = body_.front();
    Position newHead = head;

    switch (dir_) {
        case Direction::UP:    newHead.second--; break;
        case Direction::DOWN:  newHead.second++; break;
        case Direction::LEFT:  newHead.first--;  break;
        case Direction::RIGHT: newHead.first++;  break;
    }

    body_.push_front(newHead);

    if (!growing_) {
        body_.pop_back();
    } else {
        growing_ = false;
    }
}

void Snake::grow() {
    growing_ = true;
}

void Snake::setDirection(Direction newDir) {
    bool blocked =
        (dir_ == Direction::UP    && newDir == Direction::DOWN)  ||
        (dir_ == Direction::DOWN  && newDir == Direction::UP)    ||
        (dir_ == Direction::LEFT  && newDir == Direction::RIGHT) ||
        (dir_ == Direction::RIGHT && newDir == Direction::LEFT);

    if (!blocked) {
        dir_ = newDir;
    }
}

Direction Snake::getDirection() const { return dir_; }

Position Snake::getHead() const { return body_.front(); }

const std::deque<Position>& Snake::getBody() const { return body_; }

int Snake::getLength() const { return static_cast<int>(body_.size()); }

bool Snake::checkSelfCollision() const {
    Position head = body_.front();
    for (std::size_t i = 1; i < body_.size(); ++i) {
        if (body_[i] == head) return true;
    }
    return false;
}

void Snake::setHeadPosition(int x, int y) {
    if (!body_.empty()) {
        body_.front() = { x, y };
    }
}