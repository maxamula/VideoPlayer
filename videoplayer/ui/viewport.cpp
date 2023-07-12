#include "viewport.h"

Viewport::Viewport(QWidget* parent)
	: QWidget(parent)
{
	m_video = media::VideoSurface::Create((HWND)this->winId(), 1000, 500);
}

Viewport::~Viewport()
{
	m_video->Release();
}

bool Viewport::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
	MSG* msg = reinterpret_cast<MSG*>(message);
	m_video->HandleWin32Msg(msg->hwnd, msg->message, msg->wParam, msg->lParam);
	return false;
}

void Viewport::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	m_video->Resize(event->size().width(), event->size().height());
}
