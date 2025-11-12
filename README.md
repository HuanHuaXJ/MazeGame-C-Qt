# Qt迷宫游戏

一个使用Qt和C++开发的迷宫游戏，支持DFS和Prim算法生成迷宫。

## 🎮 功能特性

- **迷宫生成算法**：深度优先搜索(DFS)和随机Prim算法
- **图形界面**：基于Qt的友好用户界面  
- **游戏功能**：角色移动、碰撞检测、胜利条件
- **资源支持**：音效和图像资源

## 🏗️ 项目结构

```
MazeGame/
├── main.cpp                 # 程序入口
├── MainWindow.h/cpp/ui     # 主窗口类
├── Maze.h/cpp              # 迷宫核心逻辑
├── Cell1.h/cpp             # 迷宫单元格
├── Utils.h/cpp             # 工具函数
├── Resources.qrc           # 资源文件
├── images/                 # 图片资源
├── sounds/                 # 音效资源
└── MazeGame.pro            # Qt项目文件
```

## 🚀 构建说明

### 环境要求
- Qt 5.12+ 或 Qt 6.0+
- C++11兼容编译器

### 构建步骤
1. 使用Qt Creator打开 `MazeGame.pro`
2. 选择构建套件
3. 构建并运行

## 🎯 使用方法

1. 运行程序
2. 选择生成算法（DFS或Prim）
3. 使用方向键或WASD移动角色
4. 从起点(左上)移动到终点(右下)

## 👥 贡献

欢迎提交Issue和Pull Request！
