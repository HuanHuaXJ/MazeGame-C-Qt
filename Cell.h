#ifndef CELL_H
#define CELL_H

#pragma once
#include "Utils.h"

class Cell {
private:
    bool walls[4];//布尔数组表示四个方向是否存在墙；有墙为1，无墙为0；默认有墙；


public:
    bool visited;//是否访问过；
    bool inPath;//是否在最终的解决方案路径上；
    bool isStart;//是否为起点；
    bool isEnd;//是否为终点；
    Cell();

    bool hasWall(Direction dir) const;

    void removeWall(Direction dir);

    void setWall(Direction dir, bool state);  // 新增方法



};
#endif // CELL_H
