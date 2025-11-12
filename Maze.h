#ifndef MAZE_H
#define MAZE_H

#pragma once
#include <iostream>
#include <vector>
#include "Cell.h"

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

#endif // MAZE_H
