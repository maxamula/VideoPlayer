#pragma once
#include "common.h"

namespace media
{
	enum PLAYER_STATE : uint8_t
	{
		PLAYER_STATE_INVALID,
		PLAYER_STATE_IDLE,
		PLAYER_STATE_PLAYING,
		PLAYER_STATE_PAUSED,
		PLAYER_STATE_RESIZING
	};

	HRESULT Initialize();
	HRESULT CreateRenderTarget(HWND hWnd, uint16_t width, uint16_t height, uint32_t* pSurfaceId);
	HRESULT ResizeRenderTarget(uint32_t rendererId, uint16_t width, uint16_t height);
	HRESULT OpenSource(uint32_t rendererId, const wchar_t* szPath);
	void DestroyRenderTarget(uint32_t rendererId);
	LRESULT HandleWin32Msg(uint32_t rendererId, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void Shutdown();
}

