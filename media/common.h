#pragma once
#include <windows.h>
#include <assert.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <comdef.h>
#include <mutex>
#include <locale>
#include <codecvt>
#include <wrl/client.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfobjects.h>
#include <mfplay.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "dxva2.lib")
#pragma comment(lib, "evr.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "windowscodecs.lib")

//directx 11
#include <d3d11.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

//d2d
#include <d2d1_3.h>
#include <d2d1_3helper.h>
#pragma comment(lib, "d2d1.lib")

#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

using namespace Microsoft::WRL;

#define DISABLE_COPY(className) \
    className(const className&) = delete; \
    className& operator=(const className&) = delete;

#define DISABLE_MOVE_COPY(className) \
    className(className&&) = delete; \
    className& operator=(className&&) = delete;

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }
#endif

#define THROW_IF_FAILED(expression)                                         \
{                                                                           \
    HRESULT _hr = expression;                                               \
    if (FAILED(_hr)) {                                                      \
        _com_error err(_hr);                                                \
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;   \
        throw std::runtime_error(converter.to_bytes(err.ErrorMessage()));   \
    }                                                                       \
}                                                                           \

// Rendering options
#define NUM_BACKBUFFERS 2