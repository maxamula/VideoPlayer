#include "ui/mainwindow.h"
#include "common.h"
#include "media.h"
#include <QtWidgets/QApplication>
#include <QFileDialog>

MainWindow* g_pMainWindow = nullptr;

int main(int argc, char *argv[])
{
    //HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(media::Initialize()))
    {
        media::Shutdown();
        return -1;
    }
    QApplication a(argc, argv);
    MainWindow w;
    g_pMainWindow = &w;
    w.show();
    int returnCode = a.exec();
    media::Shutdown();
    return returnCode;
}
