#include "Utils.h"
#include <algorithm>
#include <random>
#include <chrono>

Direction MazeUtils::getOppositeDirection(Direction dir) {
    switch (dir) {
    case Direction::North:
        return Direction::South;
        break;
    case Direction::South:
        return Direction::North;
        break;
    case Direction::East:
        return Direction::West;
        break;
    case Direction::West:
        return Direction::East;
        break;
    default:
        return Direction::North;

    }
}

std::array<Direction, 4> MazeUtils::getShuffledDirections() {
    //创建包含四个方向的数组；
    std::array<Direction, 4> directions = {
        Direction::North,
        Direction::East,
        Direction::South,
        Direction::South
    };

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::shuffle(directions.begin(), directions.end(), gen);

    return directions;
}
