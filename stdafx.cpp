// stdafx.cpp : source file that includes just the standard includes
// NewtonBasin.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef UNICODE

CString s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = ::MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	CString r(buf);
	delete[] buf;
	return r;
}

std::string ws2s(CString ws)
{
	int len;
	int wslength = ws.GetLength();
	BOOL usedDefaultChar;
	// using -1 as length: the returned length includes null terminator.
	len = ::WideCharToMultiByte(CP_ACP, 0, ws, -1, 0, 0, 0, &usedDefaultChar);
	char *buf = new char[len];
	::WideCharToMultiByte(CP_ACP, 0, ws, -1, buf, len, 0, &usedDefaultChar);
	std::string r(buf);
	delete[] buf;
	return r;
}

#endif
