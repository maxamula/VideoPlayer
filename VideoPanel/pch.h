#pragma once

#define WINVER 0x0A00

#include <sdkddkver.h>
#include <collection.h>
#include <ppltasks.h>
#include <wrl/client.h>
#include <xaudio2.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfobjects.h>
#include <mfplay.h>
#include <mfreadwrite.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "dxva2.lib")
#pragma comment(lib, "evr.lib")
#pragma comment(lib, "Shlwapi.lib")


using namespace Microsoft::WRL;

#define RELEASE(com) { if(com) { com->Release(); com = nullptr; } }
#define DISABLE_MOVE_COPY(class_name) class_name(const class_name&) = delete; class_name& operator=(const class_name&) = delete; class_name(class_name&&) = delete; class_name& operator=(class_name&&) = delete;
#define DISABLE_COPY(class_name) class_name(const class_name&) = delete; class_name& operator=(const class_name&) = delete;
#ifdef _DEBUG
#define SET_NAME(x, name) x->SetName(name)
#define INDEXED_NAME_BUFFER(size) wchar_t namebuf[size]
#define SET_NAME_INDEXED(x, name, index) swprintf_s(namebuf, L"%s (%d)", name, index); x->SetName(namebuf)
#else
#define SET_NAME(x, name) ((void)0)
#define INDEXED_NAME_BUFFER(size) ((void)0)
#define SET_NAME_INDEXED(x, name, index) ((void)0)
#endif
#define MAX_RESOURSE_NAME 60

#define BACKBUFFER_COUNT 2
#define COPY_FRAMES_COUNT 2

#define uint32_invalid 0xffffffff
#define uint64_invalid 0xffffffffffffffff

namespace VideoPanel
{
	public enum class PlayerState
	{
		Invalid,
		Idle,
		Playing,
		Paused,
	};

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch DirectX API errors.
			throw Platform::Exception::CreateException(hr);
		}
	}
}