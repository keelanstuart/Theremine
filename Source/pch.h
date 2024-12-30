// **************************************************************
// Theremine Source File
// An optical theremin for the Leap Motion controller series of devices
//
// Copyright © 2020-2025, Keelan Stuart

// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"

#include <math.h>
#include <float.h>
#include <corecrt_math_defines.h>
#include <limits.h>

#include <string>
#include <deque>
#include <map>
#include <vector>
#include <algorithm>
#include <ios>
#include <regex>

#include <windows.h>
#include <mmsystem.h>
#include <tchar.h>


// macros to convert to and from wide- and multi-byte- character
// strings from the other type... all on the stack.
#define LOCAL_WCS2MBCS(wcs, mbcs) {               \
  size_t origsize = _tcslen(wcs) + 1;             \
  size_t newsize = (origsize * 2) * sizeof(char); \
  mbcs = (char *)_alloca(newsize);                \
  size_t retval = 0;                              \
  wcstombs_s(&retval, mbcs, newsize, wcs, newsize); }

#define LOCAL_MBCS2WCS(mbcs, wcs) {               \
  size_t origsize = strlen(mbcs) + 1;             \
  size_t newsize = origsize * sizeof(wchar_t);    \
  wcs = (wchar_t *)_alloca(newsize);              \
  size_t retval = 0;                              \
  mbstowcs_s(&retval, wcs, origsize, mbcs, newsize); }

#if defined(_UNICODE) || defined(UNICODE)

#define LOCAL_TCS2MBCS(tcs, mbcs) LOCAL_WCS2MBCS(tcs, mbcs)

#define LOCAL_TCS2WCS(tcs, wcs) wcs = (wchar_t *)tcs;

#define LOCAL_MBCS2TCS(mbcs, tcs) LOCAL_MBCS2WCS(mbcs, tcs)

#define LOCAL_WCS2TCS(wcs, tcs) tcs = (TCHAR *)wcs;

#else

#define LOCAL_TCS2MBCS(tcs, mbcs) mbcs = (char *)tcs;

#define LOCAL_TCS2WCS(tcs, wcs) LOCAL_MBCS2WCS(tcs, wcs)

#define LOCAL_MBCS2TCS(mbcs, tcs) tcs = (TCHAR *)mbcs;

#define LOCAL_WCS2TCS(tcs, wcs) LOCAL_WCS2MBCS(wcs, tcs)

#endif


typedef std::basic_string<TCHAR> tstring;
typedef std::basic_ios<TCHAR, std::char_traits<TCHAR>> tios;
typedef std::basic_streambuf<TCHAR, std::char_traits<TCHAR>> tstreambuf;
typedef std::basic_istream<TCHAR, std::char_traits<TCHAR>> tistream;
typedef std::basic_ostream<TCHAR, std::char_traits<TCHAR>> tostream;
typedef std::basic_iostream<TCHAR, std::char_traits<TCHAR>> tiostream;
typedef std::basic_stringbuf<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>> tstringbuf;
typedef std::basic_istringstream<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>> tistringstream;
typedef std::basic_ostringstream<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>> tostringstream;
typedef std::basic_stringstream<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>> tstringstream;
typedef std::basic_filebuf<TCHAR, std::char_traits<TCHAR>> tfilebuf;
typedef std::basic_ifstream<TCHAR, std::char_traits<TCHAR>> tifstream;
typedef std::basic_ofstream<TCHAR, std::char_traits<TCHAR>> tofstream;
typedef std::basic_fstream<TCHAR, std::char_traits<TCHAR>> tfstream;
typedef std::basic_regex<TCHAR, std::regex_traits<TCHAR>> tregex;

#include <Pool.h>
#include <genio.h>
#include <PowerProps.h>

#include <LeapC.h>

#endif //PCH_H
