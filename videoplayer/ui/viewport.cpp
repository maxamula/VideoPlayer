#include "viewport.h"

Viewport::Viewport(QWidget* parent)
	: QWidget(parent)
{
	media::CreateRenderTarget((HWND)this->winId(), 1000, 500, &m_surfaceId);
	media::OpenSource(m_surfaceId, L"C:\\ll.mp4");
	//media::SetRenderTarget((HWND)this->winId(), this->width(), this->height());
	//media::StartAsyncRenderer();
	//media::OpenSource(L"C:\\Users\\maxamula\\Desktop\\ee.mp4");
	//media::OpenSource(L"C:\\ll.mp4");
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
	//media::ResizeRenderSurface(event->size().width(), event->size().height());
}
