#pragma once
#include <qwidget.h>
#include <qevent.h>
#include <videosurface.h>
#include <qtimer.h>

// Singleton
class Viewport : public QWidget
{
	Q_OBJECT
public:
	explicit Viewport(QWidget* parent = nullptr);
	~Viewport();
	inline media::VideoSurface* Video() const { return m_video; }
private:
	media::VideoSurface* m_video = nullptr;
	QTimer* timer;
protected:
	bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
	void resizeEvent(QResizeEvent* event) override;
};
