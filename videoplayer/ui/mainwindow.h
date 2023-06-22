#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mainwindow.h"
#include "media/mediaplayer.h"


const QEvent::Type EVENT_MEDIA = static_cast<QEvent::Type>(QEvent::User + 1);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    class MediaEvent : public QEvent
    {
    public:
        MediaEvent(IMFMediaEvent* pMediaEvent, MediaEventType type);

        inline IMFMediaEvent* GetMediaEvent() { return m_pMediaEvent; }
        inline MediaEventType GetMediaEventType() { return m_type; }
    private:
        IMFMediaEvent* m_pMediaEvent = nullptr;
        MediaEventType m_type = MEUnknown;
    };
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void PostMediaEvent(IMFMediaEvent* pMediaEvent, MediaEventType type);

private:
    Ui::mainwindowClass ui;
    player::MediaPlayer* m_pPlayer;
private slots:
    void OpenFile();
    void Resume();
    void Pause();
protected:
    void customEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
};
