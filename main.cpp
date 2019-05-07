#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
#if QT_VERSION >= 0x050600
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
