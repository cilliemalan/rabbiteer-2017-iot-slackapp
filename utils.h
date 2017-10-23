#pragma once

#include "pch.h"

#ifdef _UTF16_STRINGS
#define N(x) utf16_to_utf8(x)
#define W(x) utf8_to_utf16(x)
#else
#define N(x) (x)
#define W(x) (x)
#endif

pplx::task<void> async_do_while(std::function<pplx::task<bool>(void)> func);

template <class T, class _Rep, class _Period>
pplx::task<T> task_after(T w, std::chrono::duration<_Rep, _Period> delay);

std::string utf16_to_utf8(const std::wstring &w);
std::wstring utf8_to_utf16(const std::string &w);

