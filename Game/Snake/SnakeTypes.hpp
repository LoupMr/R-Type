#ifndef SNAKE_TYPES_HPP
#define SNAKE_TYPES_HPP

#include <vector>

enum class Direction {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3
};

struct GridPosition {
    int x, y;
};

struct Snake {
    int netID;
    int score;
    Direction currentDirection;
    std::vector<GridPosition> body;
};

struct Food {
    int id;
    GridPosition pos;
};

#endif // SNAKE_TYPES_HPP
