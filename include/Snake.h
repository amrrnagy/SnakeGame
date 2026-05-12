#pragma once

#include <deque>
#include "Common.h"

class Snake {
public:
    Snake(int startX, int startY);
    void move();
    void grow();
    void setDirection(Direction newDir);
    Direction getDirection() const;
    Position getHead() const;
    const std::deque<Position>& getBody() const;
    int getLength() const;
    bool checkSelfCollision() const;

private:
    std::deque<Position> body_;
    Direction            dir_;
    bool                 growing_;
};