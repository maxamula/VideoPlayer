#pragma once

namespace VideoPanel
{
	class d3d
	{
	public:
		static d3d& Instance()
		{
			static d3d instance;
			return instance;
		}
		// Rendering
		ComPtr<ID3D11Device> d3dDevice;
		ComPtr<ID3D11DeviceContext> d3dContext;

		ComPtr<ID2D1Factory> d2dFactory;
		ComPtr<ID2D1Device> d2dDevice;
		ComPtr<ID2D1DeviceContext> d2dContext;

		ComPtr<IDXGIOutput> dxgiOutput;


		 // Audio
		ComPtr<IXAudio2> audio;
		IXAudio2MasteringVoice* masteringVoice;

	private:
		d3d();
		~d3d();
	};
}