#pragma once

#include <vector>
#include "Common.h"

class Food {
public:
    Food();
    void spawn(const std::vector<std::vector<Cell>>& grid);
    Position getPosition() const;

private:
    Position pos_;
};