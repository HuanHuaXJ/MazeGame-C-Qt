#ifndef MAZE_H
#define MAZE_H

#pragma once
#include <iostream>
#include <vector>
#include "Cell.h"
#include <random>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <cmath>

class Maze {
private:
    std::vector<std::vector<Cell>> grid;
    int width;
    int height;
    int startX, startY;
    int endX, endY;
    int playerX;
    int playerY;
    bool gameWon;


public:

    Maze(int w, int h);

    ~Maze();

    int getWidth() const;

    int getHeight() const;

    Cell& getCell(int x, int y);

    const Cell& getCell(int x, int y) const;

    void setSize(int x ,int y);

    void generateSimple();

    void generateDFS();

    void generatePrim();

    bool solveBFS();

    bool solveAStar();

    void reset();

    void clearSolution();

    void setStart(int x, int y);

    void setEnd(int x, int y);

    void printToConsole() const;

    bool movePlayer(Direction dir);

    bool isGameOver() const;

    void resetGame();

    void play_The_Game(Maze& maze);

    int getPlayerX()const;

    int getPlayerY()const;

    int getStartX()const;

    int getStartY()const;

    int getEndX()const;

    int getEndY()const;

};

class MazeGenerator {
private:
    std::random_device rd;
    std::mt19937 gen;

public:
    MazeGenerator() : gen(rd()) {
        // 构造函数初始化
    }
};

#endif // MAZE_H
