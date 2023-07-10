#include "mainwindow.h"
#include <qevent.h>
#include <QFileDialog>
#include <iostream>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    // signals/slots
    connect(ui.btnPlay, &QPushButton::pressed, this, &MainWindow::Play);
    connect(ui.btnPause, &QPushButton::pressed, this, &MainWindow::Pause);
    connect(ui.btnStop, &QPushButton::pressed, this, &MainWindow::Stop);
}

MainWindow::~MainWindow()
{}

#pragma region SLOTS

void MainWindow::Play()
{
    if (ui.viewport->Video()->GetState() == media::PLAYER_STATE_IDLE)
    {
        Beep(1000, 300);
        QFileDialog dialog;
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setNameFilter("Video files (*.mp4 *.avi *.3gp)");
        if (dialog.exec())
        {
            wchar_t szwFileName[MAX_PATH];
            ZeroMemory(szwFileName, MAX_PATH * sizeof(wchar_t));
            dialog.selectedFiles().first().toWCharArray(szwFileName);
            ui.viewport->Video()->OpenSource(szwFileName);
        }
        return;
    }
    else
        ui.viewport->Video()->Play();
}

void MainWindow::Pause()
{
    ui.viewport->Video()->Pause();
}

void MainWindow::Stop()
{

}

#pragma endregion
