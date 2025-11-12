#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <Qpainter>
#include <QEvent>
#include <QDebug>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>
#include <QMap>
#include <QPair>
#include <QSettings>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentMaze(nullptr)
    , cellSize(30)
    , totalSteps(0)
    , elapsedSeconds(0)
    , currentDifficulty(Difficulty::Medium)
    , currentGameMode(GameMode::Infinite)
    , currentLevel(1)
    , maxLevel(10)
    , stackedWidget(nullptr)
    , coverPage(nullptr)
    , gamePage(nullptr)
    , moveSound(nullptr)
    , winSound(nullptr)
    , wallHitSound(nullptr)
    , soundEnabled(true)
    , backgroundLoaded(false)
    , playerHealth(3)
    , maxHealth(3)
    , healthDisplay(nullptr)
    , playerAnimation(nullptr)
    , isAnimating(false)
    , currentPlayerPos(0 , 0)
    , isLoggedIn(false)
    , currentUser("")
    , currentUserId(-1)
    , totalGamesPlayed(0)
    , totalPlayTime(0)
{
    ui->setupUi(this);

    initializeLevels();

    // 测试资源文件是否加载成功
    qDebug() << "=== 资源文件测试 ===";
    qDebug() << "player.png exists:" << QFile(":/images/player.png").exists();
    qDebug() << "move.wav exists:" << QFile(":/sounds/move.wav").exists();
    qDebug() << "win.wav exists:" << QFile(":/sounds/win.wav").exists();
    qDebug() << "wall_hit.wav exists:" << QFile(":/sounds/wall_hit.wav").exists();
    qDebug() << "background_music.wav exists:" << QFile(":/sounds/background_music.wav").exists();
    qDebug() << "background.png exists:" << QFile(":/images/background.png").exists();
    qDebug() << "heart.png exists:" << QFile(":/images/heart.png").exists();

    if(!initializeDatabase()){
        QMessageBox::critical(this , "数据库错误" , "无法连接到数据库，将以离线模式进行");
    }

    initializeHealthSystem();
    loadHeartImages();
    initializeSound();
    initializeBGM();
    loadBackgroundImage();
    updateHealthDisplay();
    setupHealthInStatusBars();
    initializeAnimationSystem();

    showLoginDialog();


    setWindowTitle("迷宫游戏");
    resize(800 , 600);

    //创建堆叠窗口；
    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);

    //创建封面页和游戏页；
    createCoverPage();
    createGamePage();

    //添加到堆叠窗口；
    stackedWidget -> addWidget(coverPage);
    stackedWidget -> addWidget(gamePage);

    //显示封面页；
    stackedWidget -> setCurrentWidget(coverPage);

    //设置焦点策略以接收键盘事件；
    setFocusPolicy(Qt::StrongFocus);


    initializeUI();//初始化UI控件；

    setupHealthInStatusBars();

    gameTimer.start();//游戏开始立即开始计数；

    //创建更新计时器（每秒更新一次显示）；
    updateTimer = new QTimer(this);
    connect(updateTimer , &QTimer::timeout , this , &MainWindow::updateTimeDisplay);
    //updateTimer -> start(1000);//每秒触发一次；


    //图片加载；
    if(!loadPlayerImage()){
        QMessageBox::warning(this , "Error!" , "Player image loaded failed . Using default circle");
    }

    //加载游戏进度；
    //loadGameProgress();
}

MainWindow::~MainWindow()
{
    delete currentMaze;
    delete ui;
}

//定义难度参数；
const QMap<MainWindow::Difficulty , QPair <int , int>> difficultyConfig = {
    {MainWindow::Difficulty::Easy , {8 , 8}},
    {MainWindow::Difficulty::Medium , {12 , 12}},
    {MainWindow::Difficulty::Hard , {16 , 16}}
};

void MainWindow::paintEvent(QPaintEvent *event) {
    QMainWindow::paintEvent(event);
    QPainter painter(this);

    // 绘制背景
    drawBackground(painter);

    // 只在游戏页面绘制迷宫内容
    if (!stackedWidget || stackedWidget->currentWidget() != gamePage) {
        return;
    }

    if (currentMaze) {
        // 绘制迷宫区域背景
        drawMazeBackground(painter);

        // 绘制迷宫墙壁和单元格
        drawMazeWalls(painter);

        // 绘制玩家
        drawPlayer(painter);

        // 更新状态
        updateStatus();
    }
}

void MainWindow::drawMazeWalls(QPainter& painter) {
    if(!currentMaze) return;

    // 计算迷宫绘制区域（居中显示）
    int mazeWidth = currentMaze->getWidth() * cellSize;
    int mazeHeight = currentMaze->getHeight() * cellSize;
    int offsetX = (width() - mazeWidth) / 2;
    int offsetY = (height() - mazeHeight) / 2 + 20; // 稍微下移，给工具栏留空间

    // 绘制迷宫边框（保留原有样式）
    painter.setPen(Qt::black);
    painter.setBrush(Qt::white);
    painter.drawRect(offsetX, offsetY, mazeWidth, mazeHeight);

    // 绘制每个单元格（完全保留您原来的逻辑）
    for(int x = 0; x < currentMaze->getWidth(); ++x) {
        for(int y = 0; y < currentMaze->getHeight(); ++y) {
            const Cell& cell = currentMaze->getCell(x, y);
            int cellX = offsetX + x * cellSize;
            int cellY = offsetY + y * cellSize;

            // 绘制单元格背景颜色（完全保留）
            if(x == 0 && y == 0) {
                painter.setBrush(Qt::green); // 起点为绿色
            } else if(x == currentMaze->getWidth() - 1 && y == currentMaze->getHeight() - 1) {
                painter.setBrush(Qt::red);   // 终点为红色
            } else {
                painter.setBrush(Qt::white); // 其他单元格为白色
            }

            // 绘制单元格矩形（完全保留）
            painter.setPen(Qt::lightGray);
            painter.drawRect(cellX, cellY, cellSize, cellSize);

            // 绘制墙壁（完全保留）
            painter.setPen(QPen(Qt::black, 2)); // 稍微加粗墙壁线条
            if (cell.hasWall(Direction::North)) {
                painter.drawLine(cellX, cellY, cellX + cellSize, cellY);
            }
            if (cell.hasWall(Direction::East)) {
                painter.drawLine(cellX + cellSize, cellY, cellX + cellSize, cellY + cellSize);
            }
            if (cell.hasWall(Direction::South)) {
                painter.drawLine(cellX, cellY + cellSize, cellX + cellSize, cellY + cellSize);
            }
            if (cell.hasWall(Direction::West)) {
                painter.drawLine(cellX, cellY, cellX, cellY + cellSize);
            }
        }
    }

    // 修改标题显示，包含关卡信息
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setBold(true);
    painter.setFont(font);

    QString title;
    if (currentGameMode == GameMode::Level) {
        title = QString("第%1关 - %2x%3 - 使用WASD移动").arg(currentLevel).arg(currentMaze->getWidth()).arg(currentMaze->getHeight());
    } else {
        title = QString("迷宫 %1x%2 - 使用WASD移动").arg(currentMaze->getWidth()).arg(currentMaze->getHeight());
    }

    painter.drawText(offsetX, offsetY - 15, title);

    font.setBold(false);
    painter.setFont(font);
}

void MainWindow::drawPlayer(QPainter& painter) {
    if(!currentMaze) return;

    // 使用相同的坐标计算
    int mazeWidth = currentMaze->getWidth() * cellSize;
    int mazeHeight = currentMaze->getHeight() * cellSize;
    int offsetX = (width() - mazeWidth) / 2;
    int offsetY = (height() - mazeHeight) / 2 + 20;

    // 获取玩家位置（使用动画位置或实际位置）
    QPointF drawPos;
    if (isAnimating) {
        // 使用动画中的位置
        drawPos = currentPlayerPos;
    } else {
        // 使用实际位置
        int playerX = currentMaze->getPlayerX();
        int playerY = currentMaze->getPlayerY();
        drawPos = QPointF(
            offsetX + playerX * cellSize,
            offsetY + playerY * cellSize
            );
        // 确保currentPlayerPos同步
        if (currentPlayerPos != drawPos) {
            currentPlayerPos = drawPos;
        }
    }

    // 计算玩家在画布上的位置
    int drawX = drawPos.x();
    int drawY = drawPos.y();

    if(!playerPixmap.isNull()) {
        // 使用图片绘制玩家
        int xOffset = (cellSize - playerPixmap.width()) / 2;
        int yOffset = (cellSize - playerPixmap.height()) / 2;
        painter.drawPixmap(drawX + xOffset, drawY + yOffset, playerPixmap);
    } else {
        // 使用圆形绘制玩家
        painter.setPen(Qt::blue);
        painter.setBrush(Qt::blue);
        painter.drawEllipse(drawX + 5, drawY + 5, cellSize - 10, cellSize - 10);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    qDebug() << "key pressed:" << event->key();

    // 首先检查迷宫是否存在
    if (!currentMaze) {
        qDebug() << "迷宫不存在";
        return;
    }

    // 处理R键重置
    if (event->key() == Qt::Key_R) {
        resetGame();
        return;
    }

    if(playerHealth <= 0){
        QMessageBox::warning(this , "游戏结束" , "血量耗尽 \n\n按R键重新开始  ");
        return;
    }

    if(isAnimating){
        qDebug()<<"正在移动中，忽略按键";
        return;
    }

    // 保存移动前的玩家迷宫坐标
    int oldPlayerX = currentMaze->getPlayerX();
    int oldPlayerY = currentMaze->getPlayerY();

    qDebug() << "移动前玩家坐标:(" << oldPlayerX << "," << oldPlayerY << ")";
    qDebug() << "移动前玩家屏幕位置:" << currentPlayerPos;

    // 正常的游戏按键处理
    Direction dir;
    bool moved = false;

    switch(event->key()){
    case Qt::Key_W:
        dir = Direction::North;
        moved = currentMaze->movePlayer(dir);
        qDebug() << "moving upward" << (moved ? "Success" : "Failure");
        break;

    case Qt::Key_S:
        dir = Direction::South;
        moved = currentMaze->movePlayer(dir);
        qDebug() << "moving downward" << (moved ? "Success" : "Failure");
        break;

    case Qt::Key_D:
        dir = Direction::East;
        moved = currentMaze->movePlayer(dir);
        qDebug() << "moving rightward" << (moved ? "Success" : "Failure");
        break;

    case Qt::Key_A:
        dir = Direction::West;
        moved = currentMaze->movePlayer(dir);
        qDebug() << "moving leftward" << (moved ? "Success" : "Failure");
        break;

    default:
        return;
    }

    if (moved) {
        // 获取移动后的玩家迷宫坐标
        int newPlayerX = currentMaze->getPlayerX();
        int newPlayerY = currentMaze->getPlayerY();

        qDebug() << "移动后玩家坐标:(" << newPlayerX << "," << newPlayerY << ")";

        // 计算屏幕坐标
        int mazeWidth = currentMaze->getWidth() * cellSize;
        int mazeHeight = currentMaze->getHeight() * cellSize;
        int offsetX = (width() - mazeWidth) / 2;
        int offsetY = (height() - mazeHeight) / 2 + 20;

        // 起始位置是当前位置
        QPointF startPos = currentPlayerPos;

        // 目标位置是新的屏幕位置
        QPointF endPos = QPointF(
            offsetX + newPlayerX * cellSize,
            offsetY + newPlayerY * cellSize
            );

        if (startPos == endPos) {
            qDebug() << "警告: 起始位置和目标位置相同，跳过动画";
            // 即使位置相同，也要更新currentPlayerPos
            currentPlayerPos = endPos;
            update();
        } else {
            // 启动动画
            startPlayerAnimation(startPos, endPos);
        }

        playMoveSound();
        totalSteps++;
        updateStepCounter();

    }else {
        // 移动失败（撞墙）
        playWallHitSound();
        decreaseHealth(1);
        QMessageBox::warning(this, "Warning!", "Wall existed! Cannot move.");
    }
}


    void MainWindow::updateStatus(){
        if(!currentMaze){
            return;
        }

        /*QString status = QString()
                             .arg(currentMaze->getPlayerX())    // 玩家X坐标
                             .arg(currentMaze->getPlayerY())    // 玩家Y坐标
                             .arg(currentMaze->getEndX())       // 终点X坐标
                             .arg(currentMaze->getEndY())       // 终点Y坐标
                             .arg(totalSteps) // 移动次数
                             .arg(playerHealth);//玩家血量

        if(currentMaze->isGameOver()){
            status += "Winner！";
        }

        statusBar()->showMessage(status);*/

}

    bool MainWindow::loadPlayerImage()
    {

        if (playerPixmap.load(":/images/player.png")) {
            playerPixmap = playerPixmap.scaled(
                cellSize - 2,
                cellSize - 2,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation);
            return true;
        }
        return false;
    }

    void MainWindow::initializeUI(){
        // 检查是否已经创建过，避免重复创建
        /*if (stepCounter != nullptr || timeDisplay != nullptr) {
            return;  // 已经初始化过，直接返回
        }*/
        createStepCounter();//创建计步器；
        createTimeDisplay();//创建时间显示器；

        // 按顺序从左到右添加控件
        ui->statusbar->addPermanentWidget(new QLabel("步数:"));
        ui->statusbar->addPermanentWidget(stepCounter);

        // 添加分隔符
        ui->statusbar->addPermanentWidget(new QLabel("  |  "));

        ui->statusbar->addPermanentWidget(new QLabel("时间:"));
        ui->statusbar->addPermanentWidget(timeDisplay);
    }

    void MainWindow::createStepCounter(){
        stepCounter = new QLCDNumber(this);
        stepCounter->setDigitCount(4);
        stepCounter->setSegmentStyle(QLCDNumber::Filled);
        stepCounter->setStyleSheet("background: black; color: green;");
        stepCounter -> display(0);//初始显示0；
    }

    void MainWindow::updateStepCounter(){
        /*if(stepCounter){
            stepCounter -> display(totalSteps);
        }*/
        stepCounter -> display(totalSteps);
        updateStatus();
    }

    void MainWindow::createTimeDisplay(){
        /*如果已存在且父对象正确，直接返回
        if (timeDisplay && timeDisplay->parent() == this) {
            qDebug() << "timeDisplay already exists and has correct parent";
            return;
        }

        // 如果存在但父对象不对，先安全删除
        if (timeDisplay) {
            qDebug() << "Deleting old timeDisplay with incorrect parent";
            // 先断开所有连接
            timeDisplay->disconnect();
            // 安全删除
            timeDisplay->deleteLater();
            timeDisplay = nullptr;
        }*/
        // 创建新的 timeDisplay
        timeDisplay = new QLCDNumber(this);  // 确保父对象是 MainWindow
        timeDisplay->setDigitCount(5);
        timeDisplay->setSegmentStyle(QLCDNumber::Filled);
        timeDisplay->setStyleSheet("background: black; color: green;");
        timeDisplay->display("00:00");

        qDebug() << "Time display created at address:" << timeDisplay;
    }

    void MainWindow::updateTimeDisplay(){
         qDebug()<<"定时器正常";
        /* 严格检查：只有在游戏页面且 timeDisplay 有效时才更新
        if (!stackedWidget || stackedWidget->currentWidget() != gamePage) {
            qDebug() << "不在游戏页面，跳过时间更新";
            return;
        }

        if (!timeDisplay) {
            qDebug() << "timeDisplay 为 null，跳过更新";
            return;
        }

        // 检查 timeDisplay 是否有效
        if (timeDisplay->parent() != gamePage) {
            qDebug() << "timeDisplay 父对象不正确，可能已失效";
            return;
        }*/

        elapsedSeconds = gameTimer.elapsed() / 1000;
        int minutes = elapsedSeconds / 60;
        int seconds = elapsedSeconds % 60;

        QString timeString = QString("%1:%2")
                                 .arg(minutes, 2, 10, QLatin1Char('0'))
                                 .arg(seconds, 2, 10, QLatin1Char('0'));

        timeDisplay->display(timeString);
        /*try {
            timeDisplay->display(timeString);
        } catch (...) {
            qDebug() << "更新时间显示时发生异常";
        }*/
    }
    // 独立的重置函数
    void MainWindow::resetGame()
    {

        //停止所有动画；
        if(playerAnimation -> state() == QPropertyAnimation::Running){
            playerAnimation -> stop();
        }
        isAnimating = false;

        if(currentGameMode == GameMode::Level){
            //关卡模式：重新加载当前关卡；
            loadLevel(currentLevel);
        }else{
            //无限模式：重置迷宫；
            if(currentMaze) {
            delete currentMaze;

            //根据当前难度重新创建迷宫；
            auto size = difficultyConfig[currentDifficulty];
            currentMaze = new Maze(size.first , size.second);
            currentMaze ->generateDFS();

            resetHealth();//重置血量；

        }

        totalSteps = 0;//步数清零；
        elapsedSeconds = 0;//时间清零；
        gameTimer.restart();

        updateStepCounter();//更新计步器；
        updateTimeDisplay();//更新计时器；

        //只在游戏页面更新显示;
        if(stackedWidget && stackedWidget -> currentWidget() == gamePage){
        update();//重绘界面；
        }
        qDebug() << "游戏已重置，步数和时间归零";

         QMessageBox::information(this, "游戏重置", "新迷宫已生成！步数和时间归零！");
        }
    }

    //难度设置函数；
    void MainWindow::setDifficulty(Difficulty difficulty){
        qDebug() << "设置难度:" << static_cast<int>(difficulty);
        currentDifficulty = difficulty;

        //获取对应尺寸；
        auto size = difficultyConfig[difficulty];
        int mazeSize = size.first;//宽度和高度相等；

        //重新创建迷宫；
        if(currentMaze){
            delete currentMaze;
        }
        currentMaze = new Maze(mazeSize , mazeSize);
        currentMaze -> generateDFS();
        //调整单元格大小以适应不同难度；
        if(mazeSize <= 8){
            cellSize = 40;
        }else if(mazeSize <= 12){
            cellSize = 30;
        }else{
            cellSize = 25;
        }

        //重置游戏状态；
        totalSteps = 0;
        elapsedSeconds = 0;
        gameTimer.restart();

        //更新游戏显示；
       /* updateStepCounter();
        updateTimeDisplay();
        update();*/

    }

    /*void MainWindow::setupDifficultySelector(){

        // 创建工具栏
        mainToolBar = new QToolBar(this);
        mainToolBar->setObjectName("mainToolBar");
        addToolBar(mainToolBar);

        //创建难度选择下拉框；
        difficultyComboBox = new QComboBox(this);
        difficultyComboBox -> addItem("简单" , static_cast<int>(Difficulty::Easy));
        difficultyComboBox -> addItem("中等" , static_cast<int>(Difficulty::Medium));
        difficultyComboBox -> addItem("困难" , static_cast<int>(Difficulty::Hard));

        // 根据当前难度设置下拉框选择
        switch(currentDifficulty) {
        case Difficulty::Easy:
            difficultyComboBox->setCurrentIndex(0);
            break;
        case Difficulty::Medium:
            difficultyComboBox->setCurrentIndex(1);  // 选择中等
            break;
        case Difficulty::Hard:
            difficultyComboBox->setCurrentIndex(2);
            break;
        }

        //连接信号槽；
        connect(difficultyComboBox , QOverload<int>::of(&QComboBox::currentIndexChanged) , this , [this](int index){
            Difficulty diff = static_cast<Difficulty> (difficultyComboBox -> itemData(index).toInt());
            setDifficulty(diff);
        });

        // 添加到工具栏
        mainToolBar->addWidget(new QLabel("难度 : "));
        mainToolBar->addWidget(difficultyComboBox);

    }*/

    void MainWindow::createCoverPage(){
        coverPage = new QWidget(this);
        QVBoxLayout *layout = new QVBoxLayout(coverPage);
        layout -> setAlignment(Qt::AlignCenter);
        layout -> setSpacing(30);
        layout -> setContentsMargins(50 , 50 , 50 , 50);

        // 设置背景
        coverPage->setStyleSheet(
            "QWidget {"
            "   background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "               stop: 0 #2c3e50, stop: 0.5 #3498db, stop: 1 #1a2530);"
            "}"
            );

        // 半透明容器
        QWidget *container = new QWidget(coverPage);
        container->setStyleSheet(
            "QWidget {"
            "   background: rgba(0, 0, 0, 0.7);"
            "   border-radius: 15px;"
            "   padding: 30px;"
            "}"
            );
        QVBoxLayout *containerLayout = new QVBoxLayout(container);
        containerLayout->setSpacing(25);
        containerLayout->setAlignment(Qt::AlignCenter);

        QLabel *titleLabel = new QLabel("迷宫游戏" , container);
        titleLabel -> setAlignment(Qt::AlignCenter);
        QFont titleFont;
        titleFont.setPointSize(36);
        titleFont.setBold(true);
        titleFont.setFamily("Arial");
        titleLabel -> setFont(titleFont);
        titleLabel -> setStyleSheet("color : white ; margin bottom : 20px");

        // 开始游戏按钮
        QPushButton *startButton = new QPushButton("开始游戏" , container);
        startButton -> setMinimumSize(200, 60);
        startButton -> setFont(QFont("Arial" , 16 , QFont::Bold));
        startButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #27ae60;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 10px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #2ecc71;"
            "   transform: scale(1.05);"
            "}"
            "QPushButton:pressed {"
            "   background-color: #229954;"
            "}"
            );

        // 移除设置按钮，只保留退出按钮
        QPushButton *exitButton = new QPushButton("退出游戏", container);
        exitButton->setMinimumSize(200, 60);
        exitButton->setFont(QFont("Arial", 16, QFont::Bold));
        exitButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #e74c3c;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 10px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #ec7063;"
            "   transform: scale(1.05);"
            "}"
            "QPushButton:pressed {"
            "   background-color: #cb4335;"
            "}"
            );

        // 添加到容器布局
        containerLayout -> addWidget(titleLabel);
        containerLayout -> addWidget(startButton);
        containerLayout -> addWidget(exitButton);

        // 添加到主布局
        layout -> addWidget(container);

        // 连接信号槽 - 开始游戏按钮连接到新的模式选择函数
        connect(startButton , &QPushButton::clicked , this , &MainWindow::showGameModeSelection);
        connect(exitButton , &QPushButton::clicked , this , &MainWindow::exitGame);

        qDebug()<<"封面创建成功";
    }

    void MainWindow::createGamePage(){
        gamePage = new QWidget(this);
        QVBoxLayout *layout = new QVBoxLayout(gamePage);
        layout -> setContentsMargins(0 ,0 ,0 ,0);
        layout -> setSpacing(0);

        // 移除顶部工具栏，直接在游戏页面添加返回按钮
        QPushButton *backButton = new QPushButton("返回主菜单", gamePage);
        backButton->setFixedSize(120, 40);
        backButton->move(20, 20); // 放置在左上角
        backButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #7d3c98;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 5px;"
            "   padding: 8px 15px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background-color: #9b59b6;"
            "}"
            "QPushButton:pressed {"
            "   background-color: #6c3483;"
            "}"
            );

        // 连接返回按钮
        connect(backButton , &QPushButton::clicked , this , &MainWindow::backToCover);

        qDebug()<<"游戏界面创建成功";
    }

    /*void MainWindow::startGame(){
        qDebug()<<"开始游戏按钮点击";
        // 添加安全检查
        if (!stackedWidget) {
            qDebug() << "错误: stackedWidget 为 null";
            return;
        }

        if (!gamePage) {
            qDebug() << "错误: gamePage 为 null";
            return;
        }

        qDebug() << "切换到游戏页面";
        stackedWidget->setCurrentWidget(gamePage);

        //切换到游戏界面；
        stackedWidget ->setCurrentWidget(gamePage);
        qDebug()<<"已切换到游戏界面";

        //开始播放背景音乐;
        playBGM();

        //如果迷宫尚未创建，则创建迷宫；
        if(!currentMaze){
            qDebug() << "创建新迷宫，难度:" << static_cast<int>(currentDifficulty);
            setDifficulty(currentDifficulty);
        }
        qDebug()<<"迷宫已创建";
        // 启动定时器
        if (updateTimer && !updateTimer->isActive()) {
            updateTimer->start(1000);
            qDebug() << "游戏开始，定时器启动";
        }


        //获取焦点以便接受键盘事件；
        setFocus();

        //触发重绘；
        update();

    }*/

    void MainWindow::showSettings(){

        QString difficultyStr;
        switch(currentDifficulty) {
        case Difficulty::Easy: difficultyStr = "简单"; break;
        case Difficulty::Medium: difficultyStr = "中等"; break;
        case Difficulty::Hard: difficultyStr = "困难"; break;
        }

        QMessageBox::information(this, "游戏设置",
                                 QString("游戏设置功能开发中...\n\n当前难度: %1").arg(difficultyStr));

    }

    void MainWindow::exitGame(){
        QApplication::quit();
    }

    void MainWindow::backToCover(){

        if (!stackedWidget) {
            qDebug() << "stackedWidget 为 null，直接返回";
            return;
        }

        stackedWidget->setCurrentWidget(coverPage);

        // 确保封面页也有血量显示（如果需要）
        setupHealthInStatusBars();
        updateHealthDisplay();

        //停止背景音乐;
        stopBGM();
        if (updateTimer) {
            qDebug() << "3.1 updateTimer 地址:" << updateTimer;
            if (updateTimer->isActive()) {
                updateTimer->stop();
                qDebug() << "3.2 定时器已停止";
            } else {
                qDebug() << "3.2 定时器未运行";
            }
        } else {
            qDebug() << "3.1 updateTimer 为 null";
        }

        totalSteps = 0;
        elapsedSeconds = 0;
        gameTimer.restart();  // 可能的崩溃点！

        // 暂时注释掉后续操作，先测试基本功能
        totalSteps = 0;
         if (stepCounter) {
            qDebug() << "调用 updateStepCounter()";
             //updateStepCounter();
         }
    }

    void MainWindow::showEvent(QShowEvent *event) {
        QMainWindow::showEvent(event);
        qDebug() << "窗口显示事件";

        // 只有在游戏页面时才启动定时器
        if (stackedWidget && stackedWidget->currentWidget() == gamePage) {
        if (updateTimer && !updateTimer->isActive()) {
            updateTimer->start(1000);
            qDebug() << "定时器启动";
        }
        }
    }

    void MainWindow::initializeSound()
    {
        qDebug() << "=== 使用QMediaPlayer初始化音效 ===";

        // 创建QMediaPlayer对象
        moveSound = new QMediaPlayer(this);
        winSound = new QMediaPlayer(this);
        wallHitSound = new QMediaPlayer(this);

    // Qt6需要设置AudioOutput
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QAudioOutput *moveOutput = new QAudioOutput(this);
        QAudioOutput *winOutput = new QAudioOutput(this);
        QAudioOutput *wallOutput = new QAudioOutput(this);

        moveSound->setAudioOutput(moveOutput);
        winSound->setAudioOutput(winOutput);
        wallHitSound->setAudioOutput(wallOutput);

        moveOutput->setVolume(0.3);
        winOutput->setVolume(0.8);
        wallOutput->setVolume(0.5);
#else
        // Qt5设置音量
        moveSound->setVolume(30);   // 0-100
        winSound->setVolume(80);
        wallHitSound->setVolume(50);
#endif

        // 加载WAV文件
        moveSound->setSource(QUrl("qrc:/sounds/move.wav"));
        winSound->setSource(QUrl("qrc:/sounds/win.wav"));
        wallHitSound->setSource(QUrl("qrc:/sounds/wall_hit.wav"));

        // 连接错误信号
        connect(moveSound, &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error error) {
            qDebug() << "移动音效错误:" << error;
        });

        qDebug() << "QMediaPlayer音效初始化完成";
    }

    void MainWindow::playMoveSound()
    {
        if (soundEnabled && moveSound) {
            moveSound->setPosition(0); // 重置到开头
            moveSound->play();
        }
    }

    void MainWindow::playWinSound()
    {
        if (soundEnabled && winSound) {
            winSound->setPosition(0);
            winSound->play();
        }
    }

    void MainWindow::playWallHitSound()
    {
        if (soundEnabled && wallHitSound) {
            wallHitSound->setPosition(0);
            wallHitSound->play();
        }
    }

    void MainWindow::initializeBGM(){
        qDebug()<<"==初始化背景音乐==";

        bgmPlayer = new QMediaPlayer(this);

        bgmAudioOutput = new QAudioOutput(this);
        bgmPlayer -> setAudioOutput(bgmAudioOutput);
        bgmAudioOutput -> setVolume(0.3);//背景音乐音量较小；

        //加载背景音乐；
        bgmPlayer -> setSource(QUrl("qrc:/sounds/background_music.wav"));
        bgmPlayer -> setLoops(QMediaPlayer::Infinite);//循环播放；

        musicEnabled = true;
        qDebug()<<"背景音乐初始化完成";
    }

    void MainWindow::playBGM(){
        if(musicEnabled && bgmPlayer){
            bgmPlayer -> play();
            qDebug()<<"开始播放背景音乐";
        }
    }

    void MainWindow::stopBGM(){
        if(bgmPlayer){
            bgmPlayer -> stop();
            qDebug()<<"停止背景音乐";
        }
    }

    void MainWindow::toggleMusic(bool enabled){
        musicEnabled = enabled;
        if(musicEnabled){
            playBGM();
        }else{
            stopBGM();
        }
    }

    void MainWindow::loadBackgroundImage() {
        // 尝试从资源文件加载
        if(backgroundPixmap.load(":/images/background.png")) {
            backgroundLoaded = true;
            qDebug() << "背景图片加载成功，尺寸:" << backgroundPixmap.size();
        } else {
            // 尝试从文件系统加载（开发时方便调试）
            if(backgroundPixmap.load("images/background.png")) {
                backgroundLoaded = true;
                qDebug() << "从文件系统加载背景图片成功";
            } else {
                qDebug() << "背景图片加载失败，将使用默认背景";
                backgroundLoaded = false;
            }
        }

        // 如果图片太大，进行缩放
        if(backgroundLoaded && backgroundPixmap.width() > 2000) {
            backgroundPixmap = backgroundPixmap.scaled(
                width(), height(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        }
    }

    void MainWindow::drawBackground(QPainter& painter) {
        if(backgroundLoaded && !backgroundPixmap.isNull()) {
            // 绘制背景图片
            painter.drawPixmap(rect(), backgroundPixmap);

            // 添加半透明黑色遮罩，让前景内容更清晰
            painter.fillRect(rect(), QColor(0, 0, 0, 50));
        } else {
            // 备用：使用原有的渐变背景
            QLinearGradient gradient(0, 0, width(), height());
            gradient.setColorAt(0, QColor(240, 240, 240));
            gradient.setColorAt(1, QColor(200, 200, 200));
            painter.fillRect(rect(), gradient);
        }
    }

    void MainWindow::drawMazeBackground(QPainter& painter) {
        if(!currentMaze) return;

        // 计算迷宫绘制区域（居中）
        int mazeWidth = currentMaze->getWidth() * cellSize;
        int mazeHeight = currentMaze->getHeight() * cellSize;
        int offsetX = (width() - mazeWidth) / 2;
        int offsetY = (height() - mazeHeight) / 2 + 20;

        // 绘制半透明白色底衬，让迷宫在背景上更清晰
        painter.fillRect(offsetX - 10, offsetY - 30,
                         mazeWidth + 20, mazeHeight + 40,
                         QColor(255, 255, 255, 180));
    }

    void MainWindow::initializeLevels()
    {
        qDebug() << "=== 初始化关卡配置 ===";

        // 清空现有配置
        levelConfig.clear();

        // 重新配置每个关卡的迷宫尺寸
        levelConfig = {
            {1, {5, 5}},    // 第1关：5x5
            {2, {6, 6}},    // 第2关：6x6
            {3, {7, 7}},    // 第3关：7x7
            {4, {8, 8}},    // 第4关：8x8
            {5, {9, 9}},    // 第5关：9x9
            {6, {10, 10}},  // 第6关：10x10
            {7, {12, 12}},  // 第7关：12x12
            {8, {14, 14}},  // 第8关：14x14
            {9, {16, 16}},  // 第9关：16x16
            {10, {18, 18}}  // 第10关：18x18
        };

        // 验证配置
        qDebug() << "关卡配置数量:" << levelConfig.size();
        for (int i = 1; i <= maxLevel; i++) {
            if (levelConfig.contains(i)) {
                auto size = levelConfig[i];
                qDebug() << "第" << i << "关:" << size.first << "x" << size.second;
            } else {
                qDebug() << "第" << i << "关: 配置缺失!";
            }
        }

        qDebug() << "=== 关卡配置初始化完成 ===";
    }
/*
    void MainWindow::setupGameModeSelector(){
        if(!mainToolBar){
            mainToolBar = new QToolBar(this);
            mainToolBar -> setObjectName("MainTooBar");
            addToolBar(mainToolBar);
        }

        //游戏模式选择框；
        mainToolBar -> addWidget(new QLabel("模式 ： "));
        gameModeComboBox = new QComboBox(this);
        gameModeComboBox -> addItem("无限模式" , static_cast<int> (GameMode::Infinite));
        gameModeComboBox -> addItem("关卡模式" , static_cast<int> (GameMode::Level));
        gameModeComboBox -> setCurrentIndex(static_cast<int> (currentGameMode));

        connect(gameModeComboBox , QOverload<int>::of(&QComboBox::currentIndexChanged) , this , [this](int index) {
            GameMode mode = static_cast<GameMode> (gameModeComboBox -> itemData(index).toInt());
            if(mode != currentGameMode){
                currentGameMode = mode;
                if(mode == GameMode::Level){
                    switchToLevelMode();
                }else{
                    switchToInfiniteMode();
                }
            }
        });

        mainToolBar -> addWidget(gameModeComboBox);

        //关卡选择控件（初始隐藏）；
        mainToolBar -> addWidget(new QLabel("关卡 ： "));
        levelComboBox = new QComboBox(this);
        for(int i = 1 ; i <= maxLevel ; ++i){
            levelComboBox -> addItem(QString("第%1关").arg(i) , i);
        }
        levelComboBox -> setCurrentIndex(currentLevel - 1);

        connect(levelComboBox , QOverload<int>::of(&QComboBox::currentIndexChanged) , this , [this](int index){
            int level = levelComboBox -> itemData(index).toInt();
            if(level != currentLevel && level >= 1 && level <= maxLevel){
                loadLevel(level);
            }
        });

        //关卡导航按钮；
        prevLevelButton = new QPushButton("上一关" , this);
        nextLevelButton = new QPushButton("下一关" , this);

        prevLevelButton -> setEnabled(false);
        nextLevelButton -> setEnabled(false);

        connect(prevLevelButton , &QPushButton::clicked , this , &MainWindow::goToPrevLevel);
        connect(nextLevelButton , &QPushButton::clicked , this , &MainWindow::goToNextLevel);

        mainToolBar -> addWidget(prevLevelButton);
        mainToolBar -> addWidget(levelComboBox);
        mainToolBar -> addWidget(nextLevelButton);

        //初始隐藏关卡相关控件；
        updateLevelNavigationVisibility(currentGameMode == GameMode::Level);
        updateLevelNavigation();
    }*/

    /*
    //切换到关卡模式；
    void MainWindow::switchToLevelMode(){
        qDebug()<<"切换到关卡模式";
        currentGameMode = GameMode::Level;
        updateLevelNavigationVisibility(true);
        qDebug()<<"updateLevelNavigationVisibility()函数正常";
        loadLevel(currentLevel);
        qDebug()<<"成功切换到关卡模式";

        updateStatus();
    }*/

    /*
    //切换到无限模式；
    void MainWindow::switchToInfiniteMode(){
        qDebug()<<"切换到无限模式；";
        currentGameMode = GameMode::Infinite;
        updateLevelNavigationVisibility(false);
        setDifficulty(currentDifficulty);

        updateStatus();
    }*/

/*
    //更新关卡导航控件的可见性；
    void MainWindow::updateLevelNavigationVisibility(bool visible) {
        qDebug() << "updateLevelNavigationVisibility:" << visible;

        // 控制关卡相关控件的可见性
        if (levelComboBox) {
            levelComboBox->setVisible(visible);
            qDebug() << "levelComboBox 可见性设置为:" << visible;
        }
        if (prevLevelButton) {
            prevLevelButton->setVisible(visible);
            qDebug() << "prevLevelButton 可见性设置为:" << visible;
        }
        if (nextLevelButton) {
            nextLevelButton->setVisible(visible);
            qDebug() << "nextLevelButton 可见性设置为:" << visible;
        }

        // 控制难度相关控件的可见性（与关卡模式相反）
        bool difficultyVisible = !visible;
        if (difficultyComboBox) {
            difficultyComboBox->setVisible(difficultyVisible);
            qDebug() << "difficultyComboBox 可见性设置为:" << difficultyVisible;
        }

        // 找到并控制难度标签的可见性
        if (mainToolBar) {
            // 查找所有QLabel
            QList<QLabel*> labels = mainToolBar->findChildren<QLabel*>();
            for (QLabel* label : labels) {
                if (label && label->text().contains("难度")) {
                    label->setVisible(difficultyVisible);
                    qDebug() << "难度标签可见性设置为:" << difficultyVisible;
                    break;
                }
            }

            // 查找模式标签并确保它始终可见
            for (QLabel* label : labels) {
                if (label && label->text().contains("模式")) {
                    label->setVisible(true);  // 模式标签始终可见
                    qDebug() << "模式标签保持可见";
                    break;
                }
            }
        }

        // 强制更新工具栏布局
        if (mainToolBar) {
            mainToolBar->update();
            mainToolBar->adjustSize();
        }
    }
*/

    void MainWindow::loadLevel(int level){
        qDebug() << "loadLevel()函数正常开始";

        if(level < 1 || level > maxLevel){
            QMessageBox::warning(this , "错误" , "关卡号无效");
            return;
        }

        currentLevel = level;

        //停止动画；
        if(playerAnimation -> state() == QPropertyAnimation::Running){
            playerAnimation -> stop();
        }
        isAnimating = false;

        // 移除关卡选择框更新代码（因为工具栏已移除）
        // if(levelComboBox){
        //     levelComboBox -> setCurrentIndex(level - 1);
        // }

        // 获取关卡配置
        auto size = levelConfig[level];
        int width = size.first;
        int height = size.second;
        qDebug() << "关卡配置尺寸:" << width << "x" << height;

        // 根据关卡大小调整单元格尺寸
        if (width <= 6) {
            cellSize = 50;
        } else if (width <= 10) {
            cellSize = 40;
        } else if (width <= 14) {
            cellSize = 30;
        } else {
            cellSize = 25;
        }

        // 创建迷宫
        if(currentMaze){
            delete currentMaze;
        }

        currentMaze = new Maze(width , height);
        currentMaze->generateDFS();

        //初始化玩家位置；
        initializePlayerPosition();

        // 重置游戏状态
        totalSteps = 0;
        elapsedSeconds = 0;
        gameTimer.restart();
        resetHealth();//重置血量；

        // 更新UI（移除关卡导航更新）
        updateStepCounter();
        updateTimeDisplay();
        // updateLevelNavigation(); // 这行移除
        // updateLevelNavigationVisibility(true); // 这行移除

        qDebug() << "第" << level << "关加载完成";
    }

    //上一关；
    void MainWindow::goToPrevLevel(){
        if(currentLevel > 1){
            loadLevel(currentLevel - 1);
        }
    }


    //下一关；
    void MainWindow::goToNextLevel(){
        if(currentLevel < maxLevel){
            loadLevel(currentLevel + 1);
        }
    }

    /*
    //更新关卡导航按钮状态；
    void MainWindow::updateLevelNavigation(){
        if(prevLevelButton){
            prevLevelButton -> setEnabled(currentLevel > 1);
        }
        if(nextLevelButton){
            nextLevelButton -> setEnabled(currentLevel < maxLevel);
        }
    }

    // 保存游戏进度
    void MainWindow::saveGameProgress()
    {
        QSettings settings("MyCompany", "MazeGame");
        settings.setValue("currentLevel", currentLevel);
        settings.setValue("unlockedLevel", qMax(settings.value("unlockedLevel", 1).toInt(), currentLevel + 1));
        qDebug() << "游戏进度已保存，当前关卡：" << currentLevel;
    }*/

    /*
    // 加载游戏进度
    void MainWindow::loadGameProgress()
    {
        QSettings settings("MyCompany", "MazeGame");
        int savedLevel = settings.value("currentLevel", 1).toInt();
        int unlockedLevel = settings.value("unlockedLevel", 1).toInt();

        currentLevel = qMin(savedLevel, unlockedLevel);
        maxLevel = qMax(maxLevel, unlockedLevel); // 确保显示所有已解锁关卡

        qDebug() << "加载游戏进度，当前关卡：" << currentLevel << "，已解锁关卡：" << unlockedLevel;
    }*/

    void MainWindow::showGameModeSelection()
    {
        qDebug() << "显示游戏模式选择对话框";

        // 创建模式选择对话框
        modeSelectionDialog = new QDialog(this);
        modeSelectionDialog->setWindowTitle("选择游戏模式");
        modeSelectionDialog->setFixedSize(500, 450);
        modeSelectionDialog->setStyleSheet(
            "QDialog {"
            "   background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "               stop: 0 #2c3e50, stop: 1 #34495e);"
            "   border: 2px solid #1a252f;"
            "   border-radius: 15px;"
            "}"
            "QLabel {"
            "   color: white;"
            "   background: transparent;"
            "}"
            "QPushButton {"
            "   border: none;"
            "   border-radius: 8px;"
            "   padding: 12px;"
            "   font-size: 14px;"
            "   font-weight: bold;"
            "   min-width: 180px;"
            "}"
            "QPushButton:hover {"
            "   transform: scale(1.02);"
            "}"
            "QPushButton:pressed {"
            "   transform: scale(0.98);"
            "}"
            "QComboBox {"
            "   background-color: white;"
            "   color: #2c3e50;"
            "   border: 2px solid #bdc3c7;"
            "   border-radius: 5px;"
            "   padding: 8px;"
            "   min-width: 120px;"
            "   font-size: 14px;"
            "}"
            "QComboBox::drop-down {"
            "   border: none;"
            "}"
            );

        QVBoxLayout *mainLayout = new QVBoxLayout(modeSelectionDialog);
        mainLayout->setAlignment(Qt::AlignCenter);
        mainLayout->setSpacing(20);
        mainLayout->setContentsMargins(40, 30, 40, 30);

        // === 标题区域 ===
        QLabel *titleLabel = new QLabel("请选择游戏模式", modeSelectionDialog);
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont titleFont;
        titleFont.setPointSize(26);
        titleFont.setBold(true);
        titleFont.setFamily("Microsoft YaHei");
        titleLabel->setFont(titleFont);
        titleLabel->setStyleSheet("color: #ecf0f1; margin-bottom: 10px;");

        // === 描述区域 ===
        QLabel *descriptionLabel = new QLabel("", modeSelectionDialog);
        descriptionLabel->setAlignment(Qt::AlignLeft);
        descriptionLabel->setWordWrap(true);
        descriptionLabel->setStyleSheet(
            "color: #bdc3c7;"
            "font-size: 14px;"
            "font-weight: normal;"
            "background: rgba(0, 0, 0, 0.3);"
            "border-radius: 8px;"
            "padding: 15px;"
            "margin: 10px 0px;"
            "line-height: 1.5;"
            );

        // === 难度选择区域 ===
        QWidget *difficultyWidget = new QWidget(modeSelectionDialog);
        difficultyWidget->setVisible(false);
        QHBoxLayout *difficultyLayout = new QHBoxLayout(difficultyWidget);
        difficultyLayout->setContentsMargins(0, 10, 0, 10);
        difficultyLayout->setSpacing(15);

        QLabel *difficultyLabel = new QLabel("选择难度：", difficultyWidget);
        difficultyLabel->setStyleSheet("color: #ecf0f1; font-size: 16px; font-weight: bold;");

        QComboBox *difficultyCombo = new QComboBox(difficultyWidget);
        difficultyCombo->addItem("简单", static_cast<int>(Difficulty::Easy));
        difficultyCombo->addItem("中等", static_cast<int>(Difficulty::Medium));
        difficultyCombo->addItem("困难", static_cast<int>(Difficulty::Hard));
        difficultyCombo->setCurrentIndex(1);

        difficultyLayout->addWidget(difficultyLabel);
        difficultyLayout->addWidget(difficultyCombo);
        difficultyLayout->addStretch();

        // === 模式选择按钮区域 ===
        QWidget *modeButtonWidget = new QWidget(modeSelectionDialog);
        QHBoxLayout *modeButtonLayout = new QHBoxLayout(modeButtonWidget);
        modeButtonLayout->setSpacing(20);
        modeButtonLayout->setContentsMargins(0, 0, 0, 0);

        // 无限模式按钮
        QPushButton *infiniteModeButton = new QPushButton("🎮 无限模式", modeButtonWidget);
        infiniteModeButton->setFixedSize(180, 60);
        infiniteModeButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #27ae60;"
            "   color: white;"
            "   font-size: 16px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #2ecc71;"
            "}"
            );

        // 关卡模式按钮
        QPushButton *levelModeButton = new QPushButton("🏆 关卡模式", modeButtonWidget);
        levelModeButton->setFixedSize(180, 60);
        levelModeButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #3498db;"
            "   color: white;"
            "   font-size: 16px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #5dade2;"
            "}"
            );

        modeButtonLayout->addWidget(infiniteModeButton);
        modeButtonLayout->addWidget(levelModeButton);

        // === 操作按钮区域 ===
        QWidget *actionButtonWidget = new QWidget(modeSelectionDialog);
        QHBoxLayout *actionButtonLayout = new QHBoxLayout(actionButtonWidget);
        actionButtonLayout->setSpacing(15);
        actionButtonLayout->setContentsMargins(0, 10, 0, 0);

        // 开始游戏按钮
        QPushButton *startButton = new QPushButton("🚀 开始游戏", actionButtonWidget);
        startButton->setFixedSize(150, 50);
        startButton->setEnabled(false);
        startButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #9b59b6;"
            "   color: white;"
            "   font-size: 16px;"
            "}"
            "QPushButton:hover:enabled {"
            "   background-color: #8e44ad;"
            "}"
            "QPushButton:disabled {"
            "   background-color: #7f8c8d;"
            "   color: #bdc3c7;"
            "}"
            );

        // 取消按钮
        QPushButton *cancelButton = new QPushButton("❌ 取消", actionButtonWidget);
        cancelButton->setFixedSize(100, 50);
        cancelButton->setStyleSheet(
            "QPushButton {"
            "   background-color: #e74c3c;"
            "   color: white;"
            "   font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #ec7063;"
            "}"
            );

        actionButtonLayout->addStretch();
        actionButtonLayout->addWidget(startButton);
        actionButtonLayout->addWidget(cancelButton);
        actionButtonLayout->addStretch();

        // === 添加到主布局 ===
        mainLayout->addWidget(titleLabel);
        mainLayout->addWidget(descriptionLabel);
        mainLayout->addWidget(difficultyWidget);
        mainLayout->addWidget(modeButtonWidget);
        mainLayout->addWidget(actionButtonWidget);
        mainLayout->addStretch();

        // === 变量定义 ===
        GameMode selectedMode = GameMode::Infinite;
        bool modeSelected = false;

        // === 信号连接 ===

        // 无限模式选择
        connect(infiniteModeButton, &QPushButton::clicked, this, [&]() {
            selectedMode = GameMode::Infinite;
            modeSelected = true;
            startButton->setEnabled(true);
            difficultyWidget->setVisible(true);

            descriptionLabel->setText(
                "• 随机生成不同难度的迷宫\n"
                "• 自由选择简单、中等、困难难度\n"
                "• 适合想要自由探索的玩家"
                );

            // 按钮高亮效果
            infiniteModeButton->setStyleSheet(
                "QPushButton {"
                "   background-color: #2ecc71;"
                "   color: white;"
                "   border: 3px solid #27ae60;"
                "   font-size: 16px;"
                "}"
                );
            levelModeButton->setStyleSheet(
                "QPushButton {"
                "   background-color: #3498db;"
                "   color: white;"
                "   font-size: 16px;"
                "}"
                );
        });

        // 关卡模式选择
        connect(levelModeButton, &QPushButton::clicked, this, [&]() {
            selectedMode = GameMode::Level;
            modeSelected = true;
            startButton->setEnabled(true);
            difficultyWidget->setVisible(false);

            descriptionLabel->setText(
                "• 从易到难的预设关卡\n"
                "• 逐步挑战更高难度\n"
                "• 适合喜欢目标导向的玩家"
                );

            // 按钮高亮效果
            levelModeButton->setStyleSheet(
                "QPushButton {"
                "   background-color: #5dade2;"
                "   color: white;"
                "   border: 3px solid #3498db;"
                "   font-size: 16px;"
                "}"
                );
            infiniteModeButton->setStyleSheet(
                "QPushButton {"
                "   background-color: #27ae60;"
                "   color: white;"
                "   font-size: 16px;"
                "}"
                );
        });

        // 开始游戏
        connect(startButton, &QPushButton::clicked, this, [&]() {
            if (!modeSelected) return;

            currentGameMode = selectedMode;

            if (currentGameMode == GameMode::Infinite) {
                currentDifficulty = static_cast<Difficulty>(difficultyCombo->currentData().toInt());
                qDebug() << "选择无限模式，难度:" << static_cast<int>(currentDifficulty);
            } else {
                qDebug() << "选择关卡模式";
            }

            modeSelectionDialog->accept();

            if (currentGameMode == GameMode::Infinite) {
                startInfiniteMode();
            } else {
                startLevelMode();
            }
        });

        // 取消
        connect(cancelButton, &QPushButton::clicked, modeSelectionDialog, &QDialog::reject);

        // 对话框关闭清理
        connect(modeSelectionDialog, &QDialog::finished, this, [this]() {
            modeSelectionDialog->deleteLater();
            modeSelectionDialog = nullptr;
        });

        // 初始描述
        descriptionLabel->setText("请点击下方按钮选择游戏模式");

        // 显示对话框
        modeSelectionDialog->exec();
    }
    void MainWindow::startInfiniteMode()
    {
        qDebug() << "开始无限模式，难度:" << static_cast<int>(currentDifficulty);

        // 创建迷宫
        if(currentMaze){
            delete currentMaze;
        }

        auto size = difficultyConfig[currentDifficulty];
        currentMaze = new Maze(size.first, size.second);
        currentMaze->generateDFS();

        // 根据难度调整单元格大小
        int mazeSize = size.first;
        if(mazeSize <= 8){
            cellSize = 40;
        }else if(mazeSize <= 12){
            cellSize = 30;
        }else{
            cellSize = 25;
        }

        // 切换到游戏页面
        switchToGamePage();

        qDebug() << "无限模式启动完成，迷宫尺寸:" << size.first << "x" << size.second;
    }

    void MainWindow::startLevelMode()
    {
        qDebug() << "开始关卡模式";

        // 加载当前关卡
        loadLevel(currentLevel);

        // 切换到游戏页面
        switchToGamePage();
    }
    void MainWindow::switchToGamePage()
    {
        qDebug() << "切换到游戏页面";

        if (!stackedWidget || !gamePage) {
            qDebug() << "错误: stackedWidget 或 gamePage 为 null";
            return;
        }

        stackedWidget->setCurrentWidget(gamePage);

        // 确保状态栏血量显示正确
        setupHealthInStatusBars();
        updateHealthDisplay();

        // 开始播放背景音乐
        playBGM();

        // 启动定时器
        if (updateTimer && !updateTimer->isActive()) {
            updateTimer->start(1000);
            qDebug() << "游戏开始，定时器启动";
        }

        // 获取焦点以便接受键盘事件
        setFocus();

        // 触发重绘
        update();
    }

    //初始化血量系统；
    void MainWindow::initializeHealthSystem(){
        playerHealth = maxHealth;

        healthDisplay = new QLabel(this);
        healthDisplay -> setStyleSheet("background : transparent ; color : white ; font-weight : bold");
        healthDisplay->setVisible(true);  // 确保可见
        updateHealthDisplay();
    }

    //加载血量图片；
    bool MainWindow::loadHeartImages()
    {
        qDebug() << "=== 加载心形图片 ===";

        // 检查资源文件是否存在
        QString resourcePath = ":/images/heart.png";
        qDebug() << "资源路径:" << resourcePath;
        qDebug() << "资源文件存在:" << QFile::exists(resourcePath);

        // 尝试加载图片
        if (heartPixmap.load(resourcePath)) {
            qDebug() << "心形图片加载成功，原始尺寸:" << heartPixmap.size();

            // 调整图片大小为16x16
            int heartSize = 16;
            heartPixmap = heartPixmap.scaled(heartSize, heartSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            qDebug() << "调整后尺寸:" << heartPixmap.size();
            qDebug() << "调整后isNull:" << heartPixmap.isNull();
            return true;
        } else {
            qDebug() << "心形图片加载失败";

            // 尝试其他可能的路径
            QStringList possiblePaths = {
                ":/images/heart.png",
                ":/images/heart_full.png",
                ":/heart.png",
                "images/heart.png"
            };

            for (const QString& path : possiblePaths) {
                qDebug() << "尝试路径:" << path << "存在:" << QFile::exists(path);
                if (heartPixmap.load(path)) {
                    qDebug() << "从备用路径加载成功:" << path;
                    int heartSize = 16;
                    heartPixmap = heartPixmap.scaled(heartSize, heartSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    return true;
                }
            }

            qDebug() << "所有路径尝试失败，将使用文字显示";
            return false;
        }
    }
    //在状态栏显示血量；
    void MainWindow::setupHealthInStatusBars(){
        if(healthDisplay){
            //在状态栏最左侧添加血量提示；
            ui -> statusbar -> insertWidget(0 , healthDisplay);

            //添加分隔符；
            QLabel* separator = new QLabel("|" , this);
            separator -> setStyleSheet("color : gray ; margin : 0 8px;");
            ui -> statusbar -> insertWidget(1 , separator);
            qDebug()<<"成功显示血量";
        }
    }

    //更新血量显示 - 扣血时红心直接消失；
    void MainWindow::updateHealthDisplay(){
        qDebug() << "=== updateHealthDisplay() 开始 ===";
        qDebug() << "healthDisplay:" << healthDisplay;
        qDebug() << "playerHealth:" << playerHealth;
        qDebug() << "heartPixmap.isNull():" << heartPixmap.isNull();

        if(!healthDisplay){
            qDebug() << "错误: healthDisplay 为 null";
            return;
        }

        if(!heartPixmap.isNull()){
            QString html;
            for(int i = 0; i < playerHealth ; ++i){
                html += "<img src=':/images/heart.png' width='16' height='16' style='margin: 0 1px;'>";
            }

            healthDisplay->setText(html);
            qDebug() << "HTML内容:" << html;
            qDebug() << "血量更新成功";
        } else {
            qDebug() << "心形图片未加载，使用文字显示";
            // 使用文字显示
            QString healthText;
            for(int i = 0; i < playerHealth ; ++i){
                healthText += "❤";
            }
            healthDisplay->setText(healthText);
        }

        healthDisplay->adjustSize();
        qDebug() << "=== updateHealthDisplay() 结束 ===";
    }
    //游戏结束处理；
    void MainWindow::gameOver(){
        qDebug()<<"血量耗尽 ， 游戏结束";

        if(soundEnabled && wallHitSound){
            wallHitSound -> setPosition(0);
            wallHitSound -> play();
        }

        QMessageBox::information(this , "游戏结束" , "血量耗尽 ， \n\n按R键重新开始");
    }

    //重置血量；
    void MainWindow::resetHealth(){
        playerHealth = maxHealth;
        updateHealthDisplay();
        qDebug()<<"血量已重置；";
    }

    //扣血函数；
    void MainWindow::decreaseHealth(int amount){
        playerHealth = qMax(0 , playerHealth - amount);
        updateHealthDisplay();

        // 播放扣血音效（可以使用wall_hit音效）
        if (soundEnabled && wallHitSound) {
            wallHitSound->setPosition(0);
            wallHitSound->play();
        }

        //检查游戏结束；
        if(playerHealth <= 0){
            gameOver();
        }
    }

    //加血函数；
    void MainWindow::increaseHealth(int amount){
        playerHealth = qMin(maxHealth , playerHealth + amount);
        updateHealthDisplay();
    }

    // 初始化动画系统
    void MainWindow::initializeAnimationSystem(){

        // 创建属性动画
        playerAnimation = new QPropertyAnimation(this, "playerPosition");
        playerAnimation->setDuration(300);  // 改为300毫秒，更合理
        playerAnimation->setEasingCurve(QEasingCurve::InOutQuad);

        // 连接动画信号
        connect(playerAnimation, &QPropertyAnimation::valueChanged, this, [this](){
            update();  // 动画过程中不断重绘
        });

        connect(playerAnimation, &QPropertyAnimation::finished, this, [this]() {
            isAnimating = false;

            // 动画结束后检查是否到达终点
            if (currentMaze && currentMaze->isGameOver()) {
                playWinSound();
                if(currentGameMode == GameMode::Level){
                    // 关卡模式：通关处理
                    QString message = QString("恭喜通过第%1关！").arg(currentLevel);

                    if (currentLevel < maxLevel) {
                        message += "\n是否要挑战下一关？";
                        QMessageBox::StandardButton reply = QMessageBox::question(
                            this, "通关!", message,
                            QMessageBox::Yes | QMessageBox::No);

                        if (reply == QMessageBox::Yes) {
                            goToNextLevel();
                        } else{
                            exitGame();
                        }
                    } else {
                        message += "\n你已经通关所有关卡！";
                        QMessageBox::information(this, "全部通关!", message);
                    }
                } else {
                    // 无限模式：通关时恢复血量作为奖励
                    increaseHealth(1);
                    QMessageBox::information(this, "恭喜!",
                                             QString("游戏胜利！获得1点血量奖励。\n当前血量：%1\n\n按R键重新开始！")
                                                 .arg(playerHealth));
                }
            }

            update();
        });
    }

    //玩家位置属性的setter（用于动画）；
    void MainWindow::setPlayerPosition(const QPointF& position){
        currentPlayerPos = position;
    }

    //玩家位置属性的getter；
    QPointF MainWindow::getPlayerPosition(){
        return currentPlayerPos;
    }

    // 启动玩家动画
    void MainWindow::startPlayerAnimation(const QPointF& start, const QPointF& end){

        if(!playerAnimation){
            qDebug() << "错误: playerAnimation 为 null";
            return;
        }

        isAnimating = true;

        // 停止当前动画（如果有）
        if(playerAnimation->state() == QPropertyAnimation::Running){
            qDebug() << "停止当前运行的动画";
            playerAnimation->stop();
        }

        // 设置动画参数
        playerAnimation->setStartValue(start);
        playerAnimation->setEndValue(end);

        // 启动动画
        playerAnimation->start();

    }

    // 初始化玩家位置
    void MainWindow::initializePlayerPosition()
    {
        if (!currentMaze) return;

        int mazeWidth = currentMaze->getWidth() * cellSize;
        int mazeHeight = currentMaze->getHeight() * cellSize;
        int offsetX = (width() - mazeWidth) / 2;
        int offsetY = (height() - mazeHeight) / 2 + 20;

        int playerX = currentMaze->getPlayerX();
        int playerY = currentMaze->getPlayerY();

        currentPlayerPos = QPointF(
            offsetX + playerX * cellSize,
            offsetY + playerY * cellSize
            );

    }

    //数据库初始化；
    bool MainWindow::initializeDatabase(){
        //使用MySQL数据库；
        db = QSqlDatabase::addDatabase("QMYSQL");

        //数据库连接配置；
        db.setHostName("localhost");
        db.setPort(3306);
        db.setDatabaseName("maze_game");
        db.setUserName("root");
        db.setPassword("LExuwenle060912@");

        if(!db.open()){
            qDebug()<<"数据库连接失败"<<db.lastError().text();
            return false;
    }

        qDebug()<<"MySQL数据库连接成功";
        return true;
}

    //密码哈希；
    QString MainWindow::hashPassword(const QString& password){
        QByteArray passwordData = password.toUtf8();
        QByteArray hashData = QCryptographicHash::hash(passwordData , QCryptographicHash::Sha256);
        return QString(hashData.toHex());
    }

    //用户注册；
    bool MainWindow::createUser(const QString& username , const QString& password , const QString& email){
        if(username.isEmpty() || password.isEmpty()){
            QMessageBox::warning(this , "输入错误" , "用户名和密码不能为空");
            return false;
        }

        if(username.length() < 3 || username.length() > 20){
            QMessageBox::warning(this , "输入错误" , "用户名长度应为3-20个字符");
            return false;
        }

        if(password.length() < 6){
            QMessageBox::warning(this , "输入错误" , "密码长度至少为6个字符");
            return false;
        }

        //检查用户名是否已存在；
        QSqlQuery checkQuery(db);
        checkQuery.prepare("SELECT id FROM users WHERE username = ?");
        checkQuery.addBindValue(username);

        if(!checkQuery.exec()){
            QMessageBox::critical(this , "数据库错误" , "查询用户失败");
            return false;
        }

        if(checkQuery.next()){
            QMessageBox::warning(this , "注册失败" , "用户名已存在");
            return false;
        }

        //创建新用户；
        QString passwordHash = hashPassword(password);
        QSqlQuery insertQuery(db);
        insertQuery.prepare("INSERT INTO users (username , password_hash , email) VALUES(? , ? , ?)");
        insertQuery.addBindValue(username);
        insertQuery.addBindValue(passwordHash);
        insertQuery.addBindValue(email);

        if(!insertQuery.exec()){
            QMessageBox::critical(this , "注册失败" , "创建用户失败");
            return false;
        }

        qDebug()<<"用户注册成功";
        return true;
    }

    //用户登录验证
    bool MainWindow::validateUser(const QString& username, const QString& password)
    {
        qDebug() << "=== 开始用户验证 ===";
        qDebug() << "用户名:" << username;

        // 使用更简单的查询方式
        QString queryStr = "SELECT id, password_hash FROM users WHERE username = '" + username + "'";
        QSqlQuery query(db);

        qDebug() << "执行查询:" << queryStr;

        if (!query.exec(queryStr)) {
            qDebug() << "查询失败:" << query.lastError().text();
            qDebug() << "错误详情:" << query.lastError().databaseText();

            // 尝试使用SQLite语法
            qDebug() << "=== 尝试备用查询方法 ===";
            queryStr = "SELECT * FROM users WHERE username = ?";
            query.prepare(queryStr);
            query.addBindValue(username);

            if (!query.exec()) {
                qDebug() << "备用查询也失败:" << query.lastError().text();
                QMessageBox::critical(this, "数据库错误", "查询用户失败！\n请检查数据库连接。");
                return false;
            }
        }

        if (!query.next()) {
            QMessageBox::warning(this, "登录失败", "用户名不存在！");
            return false;
        }

        // 使用字段索引而不是字段名（更兼容）
        QString storedHash = query.value(1).toString(); // password_hash在第二个字段
        int userId = query.value(0).toInt(); // id在第一个字段

        qDebug() << "用户ID:" << userId;
        qDebug() << "存储的哈希:" << storedHash;

        QString inputHash = hashPassword(password);
        qDebug() << "输入的哈希:" << inputHash;

        if (storedHash != inputHash) {
            QMessageBox::warning(this, "登录失败", "密码错误！");
            return false;
        }

        // 登录成功
        currentUser = username;
        currentUserId = userId;
        isLoggedIn = true;

        // 更新最后登录时间
        QSqlQuery updateQuery(db);
        updateQuery.exec("UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE id = " + QString::number(userId));

        qDebug() << "用户登录成功:" << username;

        return true;
    }

    //更新用户游戏统计；
    void MainWindow::updateUserStats(){
        if (!isLoggedIn) return;

        QSqlQuery query(db);
        query.prepare("UPDATE users SET total_games_played = ?, total_play_time = ? WHERE id = ?");
        query.addBindValue(totalGamesPlayed);
        query.addBindValue(totalPlayTime);
        query.addBindValue(currentUserId);

        if (!query.exec()) {
            qDebug() << "更新用户统计失败:" << query.lastError().text();
        }
    }

    //在游戏开始时调用统计更新；
    void MainWindow::startGameWithStats(){
        totalGamesPlayed++;
        updateUserStats();
    }

    //在游戏结束时更新游戏时间；
    void MainWindow::endGameWithStats(){
        totalPlayTime += elapsedSeconds;
        updateUserStats();
    }

    void MainWindow::showLoginDialog()
    {
        QDialog loginDialog(this);
        loginDialog.setWindowTitle("用户登录 - 迷宫游戏");
        loginDialog.setFixedSize(550, 600);  // 进一步增大对话框尺寸
        loginDialog.setStyleSheet(
            "QDialog {"
            "   background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "               stop: 0 #2c3e50, stop: 1 #34495e);"
            "   border-radius: 15px;"
            "}"
            "QLabel { "
            "   color: white; "
            "   font-weight: bold; "
            "   font-size: 18px;"  // 增大标签字体
            "   margin-bottom: 8px;"
            "}"
            "QLineEdit {"
            "   padding: 10px 8px;"  // 进一步增加内边距
            "   border: 2px solid #bdc3c7;"
            "   border-radius: 10px;"  // 增大圆角
            "   font-size: 16px;"
            "   background: white;"
            "   min-height: 16px;"  // 增加最小高度
            "   min-width: 300px;"  // 设置最小宽度防止挤压
            "}"
            "QLineEdit:focus {"
            "   border-color: #3498db;"
            "   background: #f8f9fa;"
            "   border-width: 3px;"  // 聚焦时增加边框宽度
            "}"
            "QPushButton {"
            "   background-color: #3498db;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 10px;"  // 增大圆角
            "   padding: 15px 25px;"  // 进一步增大按钮内边距
            "   font-weight: bold;"
            "   font-size: 16px;"  // 增大字体
            "   min-width: 140px;"  // 增大最小宽度
            "   min-height: 45px;"  // 增加最小高度
            "}"
            "QPushButton:hover { "
            "   background-color: #5dade2; "
            "   transform: scale(1.02);"
            "}"
            "QPushButton:pressed { "
            "   background-color: #2980b9; "
            "}"
            "QPushButton#registerBtn {"
            "   background-color: #27ae60;"
            "}"
            "QPushButton#registerBtn:hover {"
            "   background-color: #2ecc71;"
            "}"
            "QPushButton#testBtn {"
            "   background-color: #f39c12;"
            "}"
            "QPushButton#testBtn:hover {"
            "   background-color: #f1c40f;"
            "}"
            );

        QVBoxLayout *mainLayout = new QVBoxLayout(&loginDialog);
        mainLayout->setSpacing(30);  // 进一步增加间距
        mainLayout->setContentsMargins(50, 40, 50, 40);  // 进一步增加边距

        // 标题区域
        QLabel *titleLabel = new QLabel("迷宫游戏", &loginDialog);
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont titleFont;
        titleFont.setPointSize(28);  // 增大标题字体
        titleFont.setBold(true);
        titleFont.setFamily("Microsoft YaHei");
        titleLabel->setFont(titleFont);
        titleLabel->setStyleSheet("color: #ecf0f1; margin-bottom: 15px;");

        QLabel *subTitleLabel = new QLabel("请先登录以开始游戏", &loginDialog);
        subTitleLabel->setAlignment(Qt::AlignCenter);
        subTitleLabel->setStyleSheet("color: #bdc3c7; font-size: 18px; margin-bottom: 40px;");

        // 输入区域 - 使用网格布局更好地控制间距
        QWidget *inputWidget = new QWidget(&loginDialog);
        QGridLayout *inputLayout = new QGridLayout(inputWidget);
        inputLayout->setVerticalSpacing(20);  // 增加垂直间距
        inputLayout->setHorizontalSpacing(15);  // 增加水平间距
        inputLayout->setContentsMargins(0, 0, 0, 0);

        // 用户名输入
        QLabel *userLabel = new QLabel("用户名:", inputWidget);
        QLineEdit *userEdit = new QLineEdit(inputWidget);
        userEdit->setPlaceholderText("请输入用户名（3-20个字符）");
        userEdit->setMinimumHeight(45);  // 增加最小高度

        // 密码输入
        QLabel *passLabel = new QLabel("密码:", inputWidget);
        QLineEdit *passEdit = new QLineEdit(inputWidget);
        passEdit->setPlaceholderText("请输入密码（至少6个字符）");
        passEdit->setEchoMode(QLineEdit::Password);
        passEdit->setMinimumHeight(45);  // 增加最小高度

        // 使用网格布局，标签和输入框各占一行
        inputLayout->addWidget(userLabel, 0, 0);
        inputLayout->addWidget(userEdit, 0, 1);
        inputLayout->addWidget(passLabel, 1, 0);
        inputLayout->addWidget(passEdit, 1, 1);

        // 设置列宽比例，让输入框有更多空间
        inputLayout->setColumnStretch(0, 1);  // 标签列
        inputLayout->setColumnStretch(1, 3);  // 输入框列（3倍宽度）

        // 按钮区域
        QWidget *buttonWidget = new QWidget(&loginDialog);
        QVBoxLayout *buttonLayout = new QVBoxLayout(buttonWidget);
        buttonLayout->setSpacing(15);  // 增加按钮间距
        buttonLayout->setContentsMargins(0, 0, 0, 0);

        QHBoxLayout *mainButtonLayout = new QHBoxLayout();
        mainButtonLayout->setSpacing(20);  // 增加按钮间水平间距
        QPushButton *loginBtn = new QPushButton("登录", buttonWidget);
        QPushButton *registerBtn = new QPushButton("注册账号", buttonWidget);
        registerBtn->setObjectName("registerBtn");

        mainButtonLayout->addWidget(loginBtn);
        mainButtonLayout->addWidget(registerBtn);

        // 测试按钮（可选）
        QHBoxLayout *testButtonLayout = new QHBoxLayout();
        testButtonLayout->setSpacing(0);
        QPushButton *testBtn = new QPushButton("使用测试账号", buttonWidget);
        testBtn->setObjectName("testBtn");
        testButtonLayout->addWidget(testBtn);
        testButtonLayout->setAlignment(Qt::AlignCenter);

        buttonLayout->addLayout(mainButtonLayout);
        buttonLayout->addSpacing(10);  // 在按钮组之间添加额外间距
        buttonLayout->addLayout(testButtonLayout);

        // 添加到主布局
        mainLayout->addWidget(titleLabel);
        mainLayout->addWidget(subTitleLabel);
        mainLayout->addSpacing(20);  // 添加额外间距
        mainLayout->addWidget(inputWidget);
        mainLayout->addStretch(2);  // 增加弹性空间
        mainLayout->addWidget(buttonWidget);

        // 连接信号
        connect(loginBtn, &QPushButton::clicked, &loginDialog, [&]() {
            QString username = userEdit->text().trimmed();
            QString password = passEdit->text();

            if (validateUser(username, password)) {
                loginDialog.accept();
            }
        });

        connect(registerBtn, &QPushButton::clicked, &loginDialog, [&]() {
            loginDialog.reject();
            showRegisterDialog();
        });

        connect(testBtn, &QPushButton::clicked, &loginDialog, [&]() {
            userEdit->setText("test");
            passEdit->setText("123456");
            loginBtn->click();  // 自动点击登录按钮
        });

        // 按Enter键登录
        connect(passEdit, &QLineEdit::returnPressed, loginBtn, &QPushButton::click);

        // 显示对话框
        if (loginDialog.exec() != QDialog::Accepted) {
            showLoginDialog();  // 重新显示登录对话框
        }
    }

    void MainWindow::showRegisterDialog()
    {
        QDialog registerDialog(this);
        registerDialog.setWindowTitle("用户注册 - 迷宫游戏");
        registerDialog.setFixedSize(550, 600);  // 增大对话框尺寸
        registerDialog.setStyleSheet(
            "QDialog {"
            "   background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
            "               stop: 0 #2c3e50, stop: 1 #34495e);"
            "   border-radius: 15px;"
            "}"
            "QLabel { "
            "   color: white; "
            "   font-weight: bold; "
            "   font-size: 14px;"  // 增大字体
            "   margin-bottom: 5px;"
            "}"
            "QLineEdit {"
            "   padding: 10px 12px;"  // 增加内边距
            "   border: 2px solid #bdc3c7;"
            "   border-radius: 8px;"
            "   font-size: 14px;"  // 增大字体
            "   background: white;"
            "   min-height: 25px;"  // 设置最小高度
            "}"
            "QLineEdit:focus {"
            "   border-color: #3498db;"
            "   background: #f8f9fa;"
            "}"
            "QPushButton {"
            "   background-color: #27ae60;"
            "   color: white;"
            "   border: none;"
            "   border-radius: 8px;"
            "   padding: 10px 18px;"  // 增大按钮内边距
            "   font-weight: bold;"
            "   font-size: 14px;"  // 增大字体
            "   min-width: 100px;"  // 增大最小宽度
            "   min-height: 20px;"
            "}"
            "QPushButton:hover { "
            "   background-color: #2ecc71; "
            "   transform: scale(1.02);"
            "}"
            "QPushButton:pressed { "
            "   background-color: #229954; "
            "}"
            "QPushButton#backBtn {"
            "   background-color: #95a5a6;"
            "}"
            "QPushButton#backBtn:hover {"
            "   background-color: #bdc3c7;"
            "}"
            );

        QVBoxLayout *mainLayout = new QVBoxLayout(&registerDialog);
        mainLayout->setSpacing(15);  // 增加间距
        mainLayout->setContentsMargins(40, 30, 40, 30);  // 增加边距

        // 标题
        QLabel *titleLabel = new QLabel("用户注册", &registerDialog);
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont titleFont;
        titleFont.setPointSize(24);  // 增大标题字体
        titleFont.setBold(true);
        titleFont.setFamily("Microsoft YaHei");
        titleLabel->setFont(titleFont);
        titleLabel->setStyleSheet("color: #ecf0f1; margin-bottom: 20px;");

        // 输入区域
        QWidget *inputWidget = new QWidget(&registerDialog);
        QVBoxLayout *inputLayout = new QVBoxLayout(inputWidget);
        inputLayout->setSpacing(12);  // 增加输入框间距
        inputLayout->setContentsMargins(0, 0, 0, 0);

        // 用户名输入
        QLabel *userLabel = new QLabel("用户名:", inputWidget);
        QLineEdit *userEdit = new QLineEdit(inputWidget);
        userEdit->setPlaceholderText("3-20个字符");
        userEdit->setMinimumHeight(45);

        // 密码输入
        QLabel *passLabel = new QLabel("密码:", inputWidget);
        QLineEdit *passEdit = new QLineEdit(inputWidget);
        passEdit->setPlaceholderText("至少6个字符");
        passEdit->setEchoMode(QLineEdit::Password);
        passEdit->setMinimumHeight(45);

        // 确认密码
        QLabel *confirmLabel = new QLabel("确认密码:", inputWidget);
        QLineEdit *confirmEdit = new QLineEdit(inputWidget);
        confirmEdit->setPlaceholderText("再次输入密码");
        confirmEdit->setEchoMode(QLineEdit::Password);
        confirmEdit->setMinimumHeight(45);

        // 邮箱输入（可选）
        QLabel *emailLabel = new QLabel("邮箱(可选):", inputWidget);
        QLineEdit *emailEdit = new QLineEdit(inputWidget);
        emailEdit->setPlaceholderText("请输入邮箱地址");
        emailEdit->setMinimumHeight(45);

        inputLayout->addWidget(userLabel);
        inputLayout->addWidget(userEdit);
        inputLayout->addWidget(passLabel);
        inputLayout->addWidget(passEdit);
        inputLayout->addWidget(confirmLabel);
        inputLayout->addWidget(confirmEdit);
        inputLayout->addWidget(emailLabel);
        inputLayout->addWidget(emailEdit);

        // 按钮区域
        QWidget *buttonWidget = new QWidget(&registerDialog);
        QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
        buttonLayout->setSpacing(20);  // 增加按钮间距
        buttonLayout->setContentsMargins(0, 20, 0, 0);

        QPushButton *registerBtn = new QPushButton("注册", buttonWidget);
        QPushButton *backBtn = new QPushButton("返回登录", buttonWidget);
        backBtn->setObjectName("backBtn");

        buttonLayout->addWidget(registerBtn);
        buttonLayout->addWidget(backBtn);

        // 添加到主布局
        mainLayout->addWidget(titleLabel);
        mainLayout->addWidget(inputWidget);
        mainLayout->addStretch(1);
        mainLayout->addWidget(buttonWidget);

        // 连接信号
        connect(registerBtn, &QPushButton::clicked, &registerDialog, [&]() {
            QString username = userEdit->text().trimmed();
            QString password = passEdit->text();
            QString confirmPassword = confirmEdit->text();
            QString email = emailEdit->text().trimmed();

            // 输入验证
            if (username.isEmpty() || password.isEmpty()) {
                QMessageBox::warning(&registerDialog, "输入错误", "用户名和密码不能为空！");
                return;
            }

            if (username.length() < 3 || username.length() > 20) {
                QMessageBox::warning(&registerDialog, "输入错误", "用户名长度应为3-20个字符！");
                return;
            }

            if (password.length() < 6) {
                QMessageBox::warning(&registerDialog, "输入错误", "密码长度至少为6个字符！");
                return;
            }

            if (password != confirmPassword) {
                QMessageBox::warning(&registerDialog, "输入错误", "两次输入的密码不一致！");
                return;
            }

            if (createUser(username, password, email)) {
                QMessageBox::information(&registerDialog, "注册成功",
                                         "注册成功！\n\n用户名: " + username + "\n已自动登录。");
                registerDialog.accept();
            }
        });

        connect(backBtn, &QPushButton::clicked, &registerDialog, [&]() {
            registerDialog.reject();
            showLoginDialog();
        });

        // 按Enter键注册
        connect(confirmEdit, &QLineEdit::returnPressed, registerBtn, &QPushButton::click);

        registerDialog.exec();
    }
