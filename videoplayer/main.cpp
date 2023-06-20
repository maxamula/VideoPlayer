#include "ui/mainwindow.h"
#include "common.h"
#include <QtWidgets/QApplication>


#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Init COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    // Initialize MMF
    assert(MFStartup(MF_VERSION) == S_OK);

    MainWindow w;
    w.show();
    int returnCode = a.exec();
    // Application shutdown
    MFShutdown();
    return returnCode;
}
