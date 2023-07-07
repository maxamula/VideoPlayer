#pragma once
#include <qwidget.h>
#include <qevent.h>
#include <media.h>

// Singleton
class Viewport : public QWidget
{
	Q_OBJECT
public:
	explicit Viewport(QWidget* parent = nullptr);

private:
	uint32_t m_surfaceId = 0xffffffff;
protected:
	bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
	void resizeEvent(QResizeEvent* event) override;
};
