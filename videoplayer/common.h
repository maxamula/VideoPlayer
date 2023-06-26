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

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfobjects.h>
#include <mfplay.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "dxva2.lib")
#pragma comment(lib, "evr.lib")
#pragma comment(lib, "Shlwapi.lib")

//directx 12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12video.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

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

#ifdef _DEBUG
#define SET_NAME(x, name) x->SetName(name)
#define INDEXED_NAME_BUFFER(size) wchar_t namebuf[size]
#define SET_NAME_INDEXED(x, name, index) swprintf_s(namebuf, L"%s (%d)", name, index); x->SetName(namebuf)
#else
#define SET_NAME(x, name) ((void)0)
#define INDEXED_NAME_BUFFER(size) ((void)0)
#define SET_NAME_INDEXED(x, name, index) ((void)0)
#endif

// Rendering options
#define BACKBUFFER_COUNT 3