
// stdafx.cpp : source file that includes just the standard includes
// ServerNode.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"


char g_szBuffer[0x20000];
HWND g_hwndMain = nullptr;

logfunc	DbgLog = [](const char* format_, ...)
{
	va_list args;
	va_start(args, format_);
	int nBuf;
	nBuf = _vsnprintf_s(g_szBuffer, 0x20000, format_, args);
	va_end(args);

	::OutputDebugStringA(g_szBuffer);
	::SendMessage(g_hwndMain, RCNT_LOG_MSG, (WPARAM)g_szBuffer, 0);
};

