#ifndef UTILS_H
#define UTILS_H

#pragma once
#include <array>

//用东南西北四个方向来表示上下左右；
enum class Direction {
    North= 0,
    East,
    South,
    West
};

namespace MazeUtils {
Direction getOppositeDirection(Direction dir);

std::array<Direction, 4> getShuffledDirections();
}


#endif // UTILS_H
