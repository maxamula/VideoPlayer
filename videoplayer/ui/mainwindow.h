#pragma once
#include <videosurface.h>
#include <QtWidgets/QMainWindow>
#include "ui_mainwindow.h"
#include "common.h"


const QEvent::Type EVENT_MEDIA = static_cast<QEvent::Type>(QEvent::User + 1);

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::mainwindowClass ui;
private slots:
    void Play();
    void Pause();
    void Stop();
};
