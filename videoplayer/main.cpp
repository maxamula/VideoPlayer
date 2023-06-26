#include "ui/mainwindow.h"
#include "common.h"
#include "media/d3dmedia.h"
#include <QtWidgets/QApplication>

MainWindow* g_pMainWindow = nullptr;

int main(int argc, char *argv[])
{
    try
    {
        QApplication a(argc, argv);
        MainWindow w;
        g_pMainWindow = &w;
        w.show();
        int returnCode = a.exec();
        return returnCode;
    }
    catch (const std::exception& ex)
    {
        MessageBoxA(NULL, ex.what(), "Runtime error", MB_ICONERROR | MB_OK);
        media::Shutdown();
        return -1;
    }
}
