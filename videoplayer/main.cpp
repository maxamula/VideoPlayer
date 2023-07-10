#include "ui/mainwindow.h"
#include "common.h"
#include <QtWidgets/QApplication>
#include <QFileDialog>

MainWindow* g_pMainWindow = nullptr;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    g_pMainWindow = &w;
    w.show();
    int returnCode = a.exec();
    return returnCode;
}
