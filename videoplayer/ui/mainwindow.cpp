#include "mainwindow.h"
#include <qevent.h>
#include <QFileDialog>
#include <iostream>

MainWindow::MediaEvent::MediaEvent(IMFMediaEvent* pMediaEvent, MediaEventType type)
    : QEvent(EVENT_MEDIA), m_pMediaEvent(pMediaEvent), m_type(type)
{ }

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    // signals/slots
    connect(ui.btnPlay, &QPushButton::pressed, this, &MainWindow::Play);
    connect(ui.btnPause, &QPushButton::pressed, this, &MainWindow::Pause);
    connect(ui.btnStop, &QPushButton::pressed, this, &MainWindow::Stop);

    m_pPlayer = player::MediaPlayer::CreateInstance((HWND)ui.frame->winId());
}

MainWindow::~MainWindow()
{
    m_pPlayer->Release();
}

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

void MainWindow::paintEvent(QPaintEvent* event)
{
    QMainWindow::paintEvent(event);
    static int i = 0;
    std::cout << "here" << i << "\n";
    i++;
    //m_pPlayer->OnPaint();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    m_pPlayer->Resize(event->size().width(), event->size().height());
}

#pragma region SLOTS

void MainWindow::OpenFile()
{
    Beep(1000, 1000);
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter("Video files (*.mp4 *.avi *.3gp)");
    if (dialog.exec()) 
    {
        wchar_t szwFileName[MAX_PATH];
        ZeroMemory(szwFileName, MAX_PATH * sizeof(wchar_t));
        dialog.selectedFiles().first().toWCharArray(szwFileName);
        m_pPlayer->Open(szwFileName);
    }
}

void MainWindow::Play()
{
    player::MEDIA_PLAYER_STATE currentState = m_pPlayer->GetState();
    if (currentState == player::MEDIA_PLAYER_STATE_CLOSED || currentState == player::MEDIA_PLAYER_STATE_STOPPED)
        this->OpenFile();
    else if (currentState == player::MEDIA_PLAYER_STATE_PAUSED)
        m_pPlayer->Play();
}

void MainWindow::Pause()
{
    m_pPlayer->Pause();
}

void MainWindow::Stop()
{
    m_pPlayer->Stop();
}

#pragma endregion
