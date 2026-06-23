#pragma once

// Force-included (/FI) when compiling the vendored foobar2000 SDK and pfc sources so
// that the Windows headers they rely on are present with the project-wide macros
// (UNICODE / NOMINMAX / WIN32_LEAN_AND_MEAN) already defined. Mirrors the include
// order the SDK's own Visual Studio project uses.

#include <WinSock2.h>
#include <Windows.h>
#include <mmsystem.h>
#include <objbase.h>
#include <objidl.h>
