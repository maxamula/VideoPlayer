#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    m_pPlayer = player::MediaPlayer::CreateInstance((HWND)this->winId(), (HWND)this->winId());
    m_pPlayer->Open(L"C:\\Users\\maxamula\\Desktop\\ll.mp4");
}

MainWindow::~MainWindow()
{}
