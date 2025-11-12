#include "MainWindow.h"
#include <QApplication>
#include <QStringConverter>
#include <QApplication>
#include<QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 测试资源文件是否存在
    if (QFile::exists(":/images/player.png")) {
        qDebug() << "Resource file exists!";
    } else {
        qDebug() <<  "Resource file NOT found!";
    }
    MainWindow w;
    w.show();
    return a.exec();
}
