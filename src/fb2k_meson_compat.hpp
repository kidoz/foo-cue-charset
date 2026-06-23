#pragma once

// Force-included (/FI) into component translation units that pull in the foobar2000 SDK.
// Ensures the Windows headers the SDK relies on are available with the project-wide macros
// (UNICODE / NOMINMAX / WIN32_LEAN_AND_MEAN) already in effect, in the include order the
// SDK's own Visual Studio project expects.

#include <WinSock2.h>
#include <Windows.h>
#include <mmsystem.h>
#include <objbase.h>
#include <objidl.h>
