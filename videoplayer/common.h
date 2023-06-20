#pragma once

#include <windows.h>
#include <assert.h>
#include <exception>
#include <stdexcept>
#include <string>

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfobjects.h>
#include <mfplay.h>

#define DISABLE_COPY(className) \
    className(const className&) = delete; \
    className& operator=(const className&) = delete;

#define DISABLE_MOVE_COPY(className) \
    className(className&&) = delete; \
    className& operator=(className&&) = delete;

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }
#endif