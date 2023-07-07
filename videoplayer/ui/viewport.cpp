#include "viewport.h"

Viewport::Viewport(QWidget* parent)
	: QWidget(parent)
{
	media::CreateRenderTarget((HWND)this->winId(), 1000, 500, &m_surfaceId);
}

bool Viewport::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
	MSG* msg = reinterpret_cast<MSG*>(message);
	if(m_surfaceId != 0xffffffff)
		media::HandleWin32Msg(m_surfaceId, msg->hwnd, msg->message, msg->wParam, msg->lParam);
	return false;
}

void Viewport::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	media::ResizeRenderTarget(m_surfaceId, event->size().width(), event->size().height());
}
