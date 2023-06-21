#include "mainwindow.h"

MainWindow::MediaEvent::MediaEvent(IMFMediaEvent* pMediaEvent, MediaEventType type)
    : QEvent(EVENT_MEDIA), m_pMediaEvent(pMediaEvent), m_type(type)
{ }

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    HWND rt = (HWND)ui.frame->winId();
    m_pPlayer = player::MediaPlayer::CreateInstance(rt, (HWND)this->winId());
    m_pPlayer->Open(L"C:\\Users\\maxamula\\Desktop\\ll.3gp");
    m_pPlayer->Play();
}

MainWindow::~MainWindow()
{}

void MainWindow::PostMediaEvent(IMFMediaEvent* pMediaEvent, MediaEventType type)
{
    QApplication::postEvent(this, new MediaEvent(pMediaEvent, type));
}

void MainWindow::customEvent(QEvent* event)
{
    MainWindow::MediaEvent* pMediaEvent = dynamic_cast<MainWindow::MediaEvent*>(event);
    if(pMediaEvent)
        m_pPlayer->HandleEvent(pMediaEvent->GetMediaEvent());
}