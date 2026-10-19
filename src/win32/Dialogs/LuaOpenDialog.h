#if !defined(VBA_WIN32_LUAOPENDIALOG_H_INCLUDED)
#define VBA_WIN32_LUAOPENDIALOG_H_INCLUDED

#pragma once
// -*- C++ -*-
// LuaOpenDialog.h : header file
//

#include "../stdafx.h"
#include "../resource.h"
#include <cstdint>

extern HWND LuaConsoleHWnd;

void PrintToWindowConsole(std::intptr_t hDlgAsInt, const char* str);
void WinLuaOnStart(std::intptr_t hDlgAsInt);
void WinLuaOnStop(std::intptr_t hDlgAsInt);
INT_PTR CALLBACK DlgLuaScriptDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

#endif // !defined(VBA_WIN32_LUAOPENDIALOG_H_INCLUDED)
