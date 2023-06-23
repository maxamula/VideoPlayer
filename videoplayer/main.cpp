#include "ui/mainwindow.h"
#include "common.h"
#include <QtWidgets/QApplication>


#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dxva2.lib")
#pragma comment(lib, "evr.lib")
#pragma comment(lib, "Shlwapi.lib")

MainWindow* g_pMainWindow = nullptr;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Init COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    // Initialize MMF
    assert(MFStartup(MF_VERSION) == S_OK);

    MainWindow w;
    g_pMainWindow = &w;
    w.show();
    int returnCode = a.exec();
    // Application shutdown
    MFShutdown();
    return returnCode;
}
