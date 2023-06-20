#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mainwindow.h"
#include "media/mediaplayer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::mainwindowClass ui;
    player::MediaPlayer* m_pPlayer;
};
