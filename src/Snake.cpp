// =============================================================================
// Snake.cpp — Implements the Snake class.
// =============================================================================

#include "Snake.h"
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor — creates a 3-tile snake centred on (startX, startY).
// Initial direction: RIGHT.
// Body layout: head at (startX, startY), tail extends to the left.
// ─────────────────────────────────────────────────────────────────────────────
Snake::Snake(int startX, int startY)
    : dir_(Direction::RIGHT), growing_(false)
{
    // push_front so body_[0] is always the head
    body_.push_front({ startX,     startY });
    body_.push_front({ startX + 1, startY });
    body_.push_front({ startX + 2, startY });
    // After 3 push_fronts:  body_[0]=(startX+2), body_[1]=(startX+1), body_[2]=(startX)
    // Re-seed so head is genuinely at startX facing right:
    body_.clear();
    body_.push_back({ startX,     startY });   // head
    body_.push_back({ startX - 1, startY });   // body
    body_.push_back({ startX - 2, startY });   // tail
}

// ─────────────────────────────────────────────────────────────────────────────
// move() — one game tick of movement.
//
// Algorithm:
//   1. Compute the new head position based on current direction.
//   2. push_front the new head  → O(1)
//   3. If NOT growing: pop_back the old tail  → O(1)
//      If growing: leave the tail, clear the growing_ flag.
//
// Net effect without growth: snake slides forward (length unchanged).
// Net effect with growth:    snake gains one tile (length + 1).
// ─────────────────────────────────────────────────────────────────────────────
void Snake::move() {
    Position head = body_.front();
    Position newHead = head;

    switch (dir_) {
        case Direction::UP:    newHead.second--; break;
        case Direction::DOWN:  newHead.second++; break;
        case Direction::LEFT:  newHead.first--;  break;
        case Direction::RIGHT: newHead.first++;  break;
    }

    body_.push_front(newHead);   // O(1) deque push_front

    if (!growing_) {
        body_.pop_back();        // O(1) deque pop_back — removes old tail
    } else {
        growing_ = false;        // consume the grow flag
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// grow() — sets a flag; the NEXT move() call will skip pop_back.
// ─────────────────────────────────────────────────────────────────────────────
void Snake::grow() {
    growing_ = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// setDirection() — rejects 180-degree reversals to prevent instant self-hit.
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
// checkSelfCollision() — O(n) linear scan.
// Compares the head against every body segment after the head.
// ─────────────────────────────────────────────────────────────────────────────
bool Snake::checkSelfCollision() const {
    Position head = body_.front();
    for (std::size_t i = 1; i < body_.size(); ++i) {
        if (body_[i] == head) return true;
    }
    return false;
}
void Snake::setHeadPosition(int x, int y) {
    body_.front() = { x, y };
}
