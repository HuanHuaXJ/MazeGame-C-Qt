#include "Maze.h"
#include <iostream>
#include <vector>
#include <stack>
#include <array>
#include <cctype>
#include <QDebug>
#include "Cell.h"
#include "MainWindow.h"

Maze::Maze(int w, int h)
    : width(w),
    height(h),
    startX(0),
    startY(0),
    endX(width - 1),
    endY(height - 1),
    gameWon(false),
    playerX(0),
    playerY(0)
{
    grid.resize(height);          // 设置行数
    for (int y = 0; y < height; y++) {
        grid[y].resize(width);    // 设置每行的列数
        // 每个Cell会自动调用构造函数
    }
}

Maze::~Maze() {

}

int Maze::getWidth() const {
    return width;
}

int Maze::getHeight() const {
    return height;
}

//返回单元格的引用，y是行，x是列；
Cell& Maze::getCell(int x, int y) {
    if (x < 0) x = 0;
    if (x >= width) x = width - 1;
    if (y < 0) y = 0;
    if (y >= height) y = height - 1;
    return grid[y][x];
}

const Cell& Maze::getCell(int x, int y) const {
    if (x < 0) x = 0;
    if (x >= width) x = width - 1;
    if (y < 0) y = 0;
    if (y >= height) y = height - 1;
    return grid[y][x];
}

void Maze::printToConsole() const {
    using std::cout;
    using std::endl;

    // 1. 打印顶部的边界
    for (int x = 0; x < width; ++x) {
        cout << "##";
    }
    cout << "#" << endl;

    // 2. 遍历每一行
    for (int y = 0; y < height; ++y) {
        // 先打印左侧边界和西墙
        cout << "#";
        for (int x = 0; x < width; ++x) {
            const Cell& cell = getCell(x, y);

            if (x == playerX && y == playerY) {
                cout << "P";//玩家位置；
            }
            // 打印单元格内容 - 添加起点终点判断！
            else if (x == endX && y == endY) {
                cout << "E";  // 终点
            }
            else if (cell.inPath) {
                cout << "*";
            }
            else if (cell.visited) {
                cout << " ";
            }
            else {
                cout << " ";
            }

            // 打印东墙
            if (cell.hasWall(Direction::East)) {
                cout << "#";
            }
            else {
                cout << " ";
            }
        }
        cout << endl;

        // 打印南墙和底部边界
        cout << "#";
        for (int x = 0; x < width; ++x) {
            const Cell& cell = getCell(x, y);

            if (cell.hasWall(Direction::South)) {
                cout << "##";
            }
            else {
                cout << " #";
            }
        }
        cout << endl;
    }
}

void Maze::setSize(int x , int y) {
    height = y;
    width = x;
}

void Maze::generateSimple() {
    qDebug() << "Maze::generateSimple() 开始";

    reset();

    // 方法2: 创建之字形路径
    for (int y = 0; y < height; y++) {
        // 每一行都向右打通
        for (int x = 0; x < width - 1; x++) {
            getCell(x , y).removeWall(Direction::East);
        }

        // 在每一行的末尾向下打通（除了最后一行）
        if (y < height - 1) {
            getCell(width - 1 , y).removeWall(Direction::South);

            // 然后向左打通（创建之字形）
            for (int x = width - 1; x > 0; x--) {
               getCell(x , y + 1).removeWall(Direction::West);
            }

            // 在下一行的开头向下打通（继续之字形）
            if (y < height - 2) {
                getCell(0 , y + 1).removeWall(Direction::South);
            }
        }
    }

    setStart(0, 0);
    setEnd(width - 1, height - 1);

    qDebug() << "Maze::generateSimple() 完成";
}

void Maze::generateDFS() {
    reset();

    std::stack<std::pair<int, int >> st;
    int startX = 0;
    int startY = 0;
    getCell(startX, startY).visited = true;
    st.push({ startX , startY });

    int step = 0;

    while (!st.empty()) {
        int x = st.top().first;
        int y = st.top().second;

        //先获得一个打乱方向的方向数组；
        std::array<Direction , 4> directions = MazeUtils::getShuffledDirections();
        bool foundVisited = false;

        //遍历四个方向，直到找到一个未访问过的邻居单元格；
        for (int i = 0; i < 4; ++i) {
            Direction dir = directions[i];
            int neighbor_x = x;
            int neighbor_y = y;

            switch (dir) {
            case Direction::North: neighbor_y = y - 1; break;
            case Direction::East:  neighbor_x = x + 1; break;
            case Direction::South: neighbor_y = y + 1; break;
            case Direction::West:  neighbor_x = x - 1; break;
            }

            qDebug() << "Step" << step << ": Checking from (" << x << "," << y
                     << ") direction" << static_cast<int>(dir)
                     << "to (" << neighbor_x << "," << neighbor_y << ")";

            if (neighbor_x >= 0 && neighbor_x < width && neighbor_y >= 0 && neighbor_y < height && !getCell(neighbor_x, neighbor_y).visited) {
                qDebug() << "  Removing wall between (" << x << "," << y
                         << ") and (" << neighbor_x << "," << neighbor_y << ")";

                getCell(x, y).removeWall(dir);
                getCell(neighbor_x, neighbor_y).removeWall(MazeUtils::getOppositeDirection(dir));

                getCell(neighbor_x, neighbor_y).visited = true;
                st.push({ neighbor_x , neighbor_y });

                foundVisited = true;
                step++;
                break;
            }
        }

        //如果四个方向的单元格都被访问过，开始回溯，弹出栈顶元素，回到上一个单元格再次开始循环；
        if (!foundVisited) {
            qDebug() << "Backtracking from (" << x << "," << y << ")";
            st.pop();
        }
    }

    // 打印最终的墙状态
    qDebug() << "Final wall states:";
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const Cell& cell = getCell(x, y);
            qDebug() << "Cell (" << x << "," << y << "):"
                     << "North:" << cell.hasWall(Direction::North)
                     << "East:" << cell.hasWall(Direction::East)
                     << "South:" << cell.hasWall(Direction::South)
                     << "West:" << cell.hasWall(Direction::West);
        }
    }

    setStart(0, 0);
    setEnd(width - 1, height - 1);
}

void Maze::generatePrim() {
    qDebug() << "Prim";
    reset();

    std::random_device rd;
    std::mt19937 gen(rd());

    // 存储墙的信息：当前单元格坐标和方向
    std::vector<std::tuple<int, int, Direction>> walls;

    // 从随机位置开始（使用奇数坐标）
    int startX = 1;
    int startY = 1;
    getCell(startX, startY).visited = true;

    // 将起点的所有墙加入列表
    if (startY > 1) walls.emplace_back(startX, startY, Direction::North);
    if (startX < width - 2) walls.emplace_back(startX, startY, Direction::East);
    if (startY < height - 2) walls.emplace_back(startX, startY, Direction::South);
    if (startX > 1) walls.emplace_back(startX, startY, Direction::West);

    while (!walls.empty()) {
        // 随机选择一堵墙
        std::uniform_int_distribution<> dis(0, walls.size() - 1);
        int index = dis(gen);
        auto [x, y, dir] = walls[index];
        walls.erase(walls.begin() + index);

        // 计算邻居单元格（步长为2）
        int nx = x, ny = y;
        switch (dir) {
        case Direction::North: ny = y - 2; break;
        case Direction::South: ny = y + 2; break;
        case Direction::East:  nx = x + 2; break;
        case Direction::West:  nx = x - 2; break;
        }

        // 确保邻居在边界内
        if (nx < 1 || nx >= width - 1 || ny < 1 || ny >= height - 1) {
            continue;
        }

        Cell& current = getCell(x, y);
        Cell& neighbor = getCell(nx, ny);

        // 关键：如果邻居未被访问，就打通这面墙
        if (!neighbor.visited) {
            // 打通当前单元格和目标单元格之间的墙
            current.removeWall(dir);
            neighbor.removeWall(MazeUtils::getOppositeDirection(dir));

            // 打通中间单元格的墙
            int midX = (x + nx) / 2;
            int midY = (y + ny) / 2;
            getCell(midX, midY).visited = true;
            getCell(midX, midY).removeWall(dir);
            getCell(midX, midY).removeWall(MazeUtils::getOppositeDirection(dir));

            // 标记邻居为已访问
            neighbor.visited = true;

            // 添加邻居的所有墙到列表
            if (ny > 1) walls.emplace_back(nx, ny, Direction::North);
            if (nx < width - 2) walls.emplace_back(nx, ny, Direction::East);
            if (ny < height - 2) walls.emplace_back(nx, ny, Direction::South);
            if (nx > 1) walls.emplace_back(nx, ny, Direction::West);
        }
    }

    // 设置入口和出口
    setStart(0, 1);
    setEnd(width - 1, height - 2);

    // 确保入口和出口的墙被打开
    getCell(0, 1).removeWall(Direction::East);
    getCell(1, 1).removeWall(Direction::West);

    getCell(width - 1, height - 2).removeWall(Direction::West);
    getCell(width - 2, height - 2).removeWall(Direction::East);

    // 设置玩家起始位置
    playerX = 0;
    playerY = 1;
}

bool Maze::solveBFS() {
        // 清除之前的路径标记
        clearSolution();

        // 创建队列和访问标记
        std::queue<std::pair<int, int>> q;
        std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
        std::vector<std::vector<std::pair<int, int>>> parent(height, std::vector<std::pair<int, int>>(width, {-1, -1}));

        // 从起点开始
        q.push({startX, startY});
        visited[startY][startX] = true;

        // 定义四个方向
        std::array<Direction, 4> directions = {
            Direction::North, Direction::East, Direction::South, Direction::West
        };

        bool found = false;

        while (!q.empty() && !found) {
            auto [x, y] = q.front();
            q.pop();

            // 如果到达终点
            if (x == endX && y == endY) {
                found = true;
                break;
            }

            // 检查四个方向
            for (Direction dir : directions) {
                int nx = x, ny = y;
                switch (dir) {
                case Direction::North: ny = y - 1; break;
                case Direction::East:  nx = x + 1; break;
                case Direction::South: ny = y + 1; break;
                case Direction::West:  nx = x - 1; break;
                }

                // 检查边界和墙
                if (nx >= 0 && nx < width && ny >= 0 && ny < height &&
                    !visited[ny][nx] && !getCell(x, y).hasWall(dir)) {

                    visited[ny][nx] = true;
                    parent[ny][nx] = {x, y};
                    q.push({nx, ny});
                }
            }
        }

        // 如果找到路径，回溯并标记
        if (found) {
            // 从终点回溯到起点
            int x = endX, y = endY;
            while (x != startX || y != startY) {
                getCell(x, y).inPath = true;
                auto [px, py] = parent[y][x];
                x = px;
                y = py;
            }
            // 标记起点
            getCell(startX, startY).inPath = true;

        } else {
            qDebug() << "BFS could not find a solution!";
        }

    return found;

}

bool Maze::solveAStar() {
    // TODO: 后续实现
    return false;
}

void Maze::reset() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            grid[y][x].visited = false;
            grid[y][x].inPath = false;

            // 使用新的setWall方法重置所有墙
            grid[y][x].setWall(Direction::North, true);
            grid[y][x].setWall(Direction::East, true);
            grid[y][x].setWall(Direction::South, true);
            grid[y][x].setWall(Direction::West, true);
        }
    }

    playerX = startX;
    playerY = startY;
    gameWon = false;
}

void Maze::clearSolution() {
    // TODO: 后续实现
}

void Maze::setStart(int x, int y) {
    startX = x;
    startY = y;
}

void Maze::setEnd(int x, int y) {
    endX = x;
    endY = y;
}


bool Maze::movePlayer(Direction dir) {

    // 计算目标位置（只移动一格）
    int targetX = playerX;
    int targetY = playerY;

    switch (dir) {
    case Direction::North: targetY = playerY - 1; break;
    case Direction::East:  targetX = playerX + 1; break;
    case Direction::South: targetY = playerY + 1; break;
    case Direction::West:  targetX = playerX - 1; break;
    }

    // 检查边界
    if (targetX < 0 || targetX >= width || targetY < 0 || targetY >= height) {
        return false; // 不能移动
    }

    // 检查墙（关键：检查当前单元格的墙，不是目标单元格的）
    if (getCell(playerX, playerY).hasWall(dir)) {
        return false; // 有墙，不能移动
    }

    // 可以移动，更新位置
    playerX = targetX;
    playerY = targetY;

    // 检查是否获胜
    if (playerX == endX && playerY == endY) {
        gameWon = true;
    }

    return true;
}
bool Maze::isGameOver()const {
    if (gameWon) {
        return true;
    }
    return false;
}

void Maze::resetGame() {
    playerX = startX;
    playerY = startY;

    gameWon = false;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            grid[y][x].visited = false;  // 重置访问状态
            grid[y][x].inPath = false;   // 重置路径状态
        }
    }
}

void Maze::play_The_Game(Maze& maze) {
    maze.setSize(width, height);
    maze.generateDFS();
    maze.resetGame();

    char command;
    while (!maze.isGameOver()) {
        maze.printToConsole();
        std::cout << "Input w/a/s/d to move : ";
        std::cin >> command;

        Direction dir;
        bool validInput = true;

        switch (std::tolower(command)) {
        case 'w':
            dir = Direction::North;
            break;
        case 'd':
            dir = Direction::East;
            break;
        case 's':
            dir = Direction::South;
            break;
        case 'a':
            dir = Direction::West;
            break;
        case 'r':
            maze.resetGame();
            std::cout << "Game is reset!！" << std::endl;
            validInput = false;
            break;
        case 'q':
            std::cout << "Game exited" << std::endl;
            return;
        default:
            std::cout << "Invalid input！Pressing WASD to move" << std::endl;
            validInput = false;
        }

        if (validInput) {
            // 只调用一次 movePlayer！
            if (maze.movePlayer(dir)) {
                std::cout << "Success move!！" << std::endl;
            }
            else {
                std::cout << "Cannot move to this direction!" << std::endl;
            }
        }
    }
    maze.printToConsole();
    std::cout << "Winner!" << std::endl;
}

int Maze::getPlayerX()const{
    return playerX;
}

int Maze::getPlayerY()const{
    return playerY;
}

int Maze::getStartX()const{
    return startX;
}

int Maze::getStartY()const{
    return startY;
}

int Maze::getEndX()const{
    return endX;
}

int Maze::getEndY()const{
    return endY;
}

