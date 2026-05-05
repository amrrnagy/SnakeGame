#pragma once
// =============================================================================
// Snake.h — Declares the Snake class.
//
// DATA STRUCTURE CHOICE: std::deque<Position>
// ─────────────────────────────────────────────────────────────────────────────
//  A snake moves by adding a new head and removing the tail every tick.
//  deque supports:
//    push_front()  → O(1)  add new head
//    pop_back()    → O(1)  remove old tail
//  A vector would cost O(n) for push_front (shifting all elements).
//  A singly-linked list gives O(1) prepend but no O(1) tail removal without
//  a doubly-linked list. std::deque gives us both ends in O(1) — ideal here.
// =============================================================================

#include <deque>
#include "C:/Users/Amr/CLionProjects/SnakeGame/include/Common.h"

class Snake {
public:
    Snake(int startX, int startY);

    // Advances the snake one step; skips tail removal if grow() was called.
    void move();

    // Call when food is eaten — next move() will not remove the tail.
    void grow();

    // Ignores 180-degree reversals.
    void setDirection(Direction newDir);
    Direction getDirection() const;

    Position getHead() const;
    const std::deque<Position>& getBody() const;
    int getLength() const;

    // O(n) self-collision: checks if head matches any body segment.
    bool checkSelfCollision() const;

private:
    std::deque<Position> body_;   // body_[0] = head; body_.back() = tail
    Direction            dir_;
    bool                 growing_;
};
