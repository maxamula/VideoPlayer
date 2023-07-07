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

    //m_pPlayer->SetInput(L"C:\\Users\\maxamula\\Desktop\\ee.mp4"); // TODO remove
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

}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
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
    }
}

void MainWindow::Play()
{
    //m_pPlayer->ReadNext();
}

void MainWindow::Pause()
{

}

void MainWindow::Stop()
{

}

#pragma endregion
