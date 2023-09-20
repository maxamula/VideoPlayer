#pragma once
#include "graphics.h"
#include <chrono>

#define MAX_FRAMES 1000

namespace VideoPanel
{
	namespace GFX
	{
		class RenderSurface
		{
		public:
			RenderSurface() = default;
			DISABLE_MOVE_COPY(RenderSurface);
			RenderSurface(unsigned short width, unsigned short height);
			~RenderSurface() { assert(m_pSwap == NULL); }

			inline void Present() { m_pSwap->Present(0, 0); m_backBufferIndex = m_pSwap->GetCurrentBackBufferIndex(); }
			inline D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle(uint32_t index) { if (index > BACKBUFFER_COUNT) throw std::runtime_error("Invalid backbuffer index!"); return m_renderTargets[index].allocation.CPU; }
			inline RENDER_TARGET CurrentRenderTarget() { return m_renderTargets[m_backBufferIndex]; }

			void Release();
			void Resize(uint16 width, uint16 height);
			inline uint16 GetWidth() const { return m_width; }
			inline uint16 GetHeight() const { return m_height; }
			inline IDXGISwapChain4* GetSwapChain() { return m_pSwap; };
			inline D3D12_VIEWPORT GetViewport() { return m_viewport; };
			inline D3D12_RECT GetScissors() { return m_scissors; };
		protected:
			void _CreateRendertargetViews();

			IDXGISwapChain4* m_pSwap = nullptr;
			D3D12_VIEWPORT m_viewport{};
			D3D12_RECT m_scissors{};
			RENDER_TARGET m_renderTargets[BACKBUFFER_COUNT]{};
			uint8 m_backBufferIndex = 0;

			uint16 m_width = 0;
			uint16 m_height = 0;
		};
	}
}

