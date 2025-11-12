#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Maze.h"
#include <QPaintEvent>
#include <QPixmap>
#include <QLCDNumber>
#include <QElapsedTimer>
#include <QTimer>
#include <QLabel>
#include <QComboBox>
#include <QToolBar>
#include <QAction>
#include <QStackedWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QSoundEffect>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPropertyAnimation>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QInputDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(QPointF playerPosition READ getPlayerPosition WRITE setPlayerPosition)

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //设置不同难度等级；
    enum class Difficulty{
        Easy = 0,
        Medium = 1,
        Hard = 2
    };

    enum class GameMode{
        Infinite = 0,//随机生成模式；
        Level = 1//关卡模式；
    };

    //用于动画的属性；


    QPointF getPlayerPosition();
    void startPlayerAnimation(const QPointF& start , const QPointF& end);


private:
    Ui::MainWindow *ui;
    Maze* currentMaze;
    int cellSize;

    //玩家图片；
    QPixmap playerPixmap;
    bool loadPlayerImage();

    //添加背景图片成员；
    QPixmap backgroundPixmap;
    bool backgroundLoaded;
    void loadBackgroundImage();

    void drawBackground(QPainter& paniter);//背景绘制；
    void drawMazeBackground(QPainter& painter);
    void drawMazeWalls(QPainter& painter);//迷宫绘制；
    void drawPlayer(QPainter& painter);//玩家绘制；
    void updateStatus();//状态更新；

    QLCDNumber *stepCounter;//计步器显示控件；
    int totalSteps = 0;//总步数计步器；

    void createStepCounter();//创建计步器控件；
    void updateStepCounter();//更新计步器显示；
    void initializeUI();//初始化UI界面；

    QLCDNumber *timeDisplay;//时间显示控件；
    QElapsedTimer gameTimer;//游戏计时器；
    QTimer *updateTimer;//更新显示的定时器
    void createTimeDisplay();
    int elapsedSeconds;//经过的秒数；

    //添加难度相关变量；
    Difficulty currentDifficulty;
    //QComboBox *difficultyComboBox;//难度下拉选择框；
    //QToolBar *mainToolBar;

    //难度配置函数；
    void setDifficulty(Difficulty difficulty);
    // void setupDifficultySelector();

    //封面界面相关；
    QStackedWidget *stackedWidget;
    QWidget *coverPage;
    QWidget *gamePage;
    void createCoverPage();
    void createGamePage();

    //音效成员变量；
    QMediaPlayer *moveSound;
    QMediaPlayer *winSound;
    QMediaPlayer *wallHitSound;
    bool soundEnabled;
    //音效相关函数；
    void initializeSound();
    void playMoveSound();
    void playWinSound();
    void playWallHitSound();

    //背景音乐相关；
    QMediaPlayer *bgmPlayer;
    QAudioOutput *bgmAudioOutput;
    bool musicEnabled;

    //背景音乐相关函数；
    void initializeBGM();
    void playBGM();
    void stopBGM();
    void toggleMusic(bool enabled);

    //关卡相关变量；
    GameMode currentGameMode;
    int currentLevel;
    int maxLevel;
    QMap<int , QPair<int , int> > levelConfig;//关卡配置 ： 关卡号 -> （宽度 ， 高度）；

    //添加关卡选择控件；
    //QComboBox *gameModeComboBox;
    //QComboBox *levelComboBox;
    //QPushButton *nextLevelButton;
    //QPushButton *prevLevelButton;

    //添加关卡相关函数；
    void initializeLevels();
    //void setupGameModeSelector();
    //void switchToLevelMode();
    //void switchToInfiniteMode();
    void loadLevel(int level);
    void goToNextLevel();
    void goToPrevLevel();
    //void updateLevelNavigation();
    // void updateLevelNavigationVisibility(bool visible = false);
    //void saveGameProgress();
    //void loadGameProgress();

    void showGameModeSelection();
    QDialog* modeSelectionDialog;
    void startInfiniteMode();
    void startLevelMode();
    void switchToGamePage();

    //血量相关；
    int playerHealth;
    int maxHealth;
    QPixmap heartPixmap;
    QLabel *healthDisplay;

    void initializeHealthSystem();
    bool loadHeartImages();
    void setupHealthInStatusBars();
    void updateHealthDisplay();
    void decreaseHealth(int);
    void increaseHealth(int);
    void gameOver();
    void resetHealth();

    //平滑移动相关；
    QPropertyAnimation* playerAnimation;
    QPointF currentPlayerPos;
    QPointF targetPlayerPos;
    bool isAnimating;

    void initializeAnimationSystem();
    void setPlayerPosition(const QPointF& position);
    void initializePlayerPosition();

    //数据库相关函数；
    bool initializeDatabase();
    bool createUser(const QString& username , const QString& password , const QString& email);
    bool validateUser(const QString& username , const QString& password);
    QString hashPassword(const QString& password);
    void showLoginDialog();
    void showRegisterDialog();
    void updateUserStats();  // 更新用户游戏统计
    void showUserProfile();
    void logout();
    void startGameWithStats();
    void endGameWithStats();

    //数据库相关变量；
    QSqlDatabase db;
    QString currentUser;
    int currentUserId;
    bool isLoggedIn;

    //用户统计；
    int totalGamesPlayed;
    int totalPlayTime;

    void resetGame();

//槽函数；
private slots:
    void updateTimeDisplay();
    void showSettings();//显示设置；
    void exitGame();//退出游戏；
    void backToCover();//返回封面；


protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

    void onLoginSuccess(const QString& username);
    void onUserProfileClicked();
    void onLogoutClicked();
};

#endif // MAINWINDOW_H
