#include "Cell.h"

Cell::Cell()
    : walls{ true , true , true , true },
    visited(false),
    inPath(false),
    isStart(false),
    isEnd(false)
{

}

//检查某一方向是否有墙；
bool Cell::hasWall(Direction dir) const {
    return walls[static_cast<int> (dir)];
}

//去除某一方向的墙；
void Cell::removeWall(Direction dir) {
    walls[static_cast<int> (dir)] = false;
}
