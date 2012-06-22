// ConsoleMax.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>


// Change the console window's internal properties so that it doesn't show a horizontal scroll bar. The window's physical size is not changed.
// If returns FALSE please call GetLastError to get error code.
BOOL FitConsoleWindowWidth(HANDLE hOutput)
{
	CONSOLE_SCREEN_BUFFER_INFOEX info;
	info.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	if (!GetConsoleScreenBufferInfoEx(hOutput, &info)){
		return FALSE;
	}

	info.dwSize.X = info.srWindow.Right - info.srWindow.Left;
	if (!SetConsoleScreenBufferInfoEx(hOutput, &info)){
		return FALSE;
	}

	return TRUE;
}

// Returns non-client total height consisting caption bar, frame, border, and an optional horizontal scrollbar in the bottom of console window.
int GetNonClientSizeY(BOOL hscroll)
{
	int caption = GetSystemMetrics(SM_CYCAPTION);
	int cyframe = GetSystemMetrics(SM_CYFRAME);
	int cyborder = GetSystemMetrics(SM_CYBORDER);
	return (hscroll ? GetSystemMetrics(SM_CYHSCROLL) : 0) + caption + 2 * (cyframe + cyborder);
}

// Returns non-client total width consisting frame, border, and an optional vertical scrollbar in the right side of console window.
int GetNonClientSizeX(BOOL vscroll)
{
	int cxframe = GetSystemMetrics(SM_CXFRAME);
	int cxborder = GetSystemMetrics(SM_CXBORDER);
	return (vscroll ? GetSystemMetrics(SM_CXVSCROLL) : 0) + 2 * (cxframe + cxborder);
}

// Returns maximum size of a console window based on specified monitor, it will also consider into non-client size and scroll bar visibilities.
// If it fails, returned COORD will be zero. Call GetLastError to get error code.
COORD GetLargestConsoleWindowSize2(HANDLE hOutput, HMONITOR hMonitor)
{
	COORD result = {0};

	HWND hwndConsole = GetConsoleWindow();
	if (hwndConsole != NULL) {
		MONITORINFO mi;
		mi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(hMonitor, &mi)){
			int height = mi.rcWork.bottom - mi.rcWork.top + 1 - GetNonClientSizeY(FALSE);
			CONSOLE_FONT_INFO fi;
			if (GetCurrentConsoleFont(hOutput, FALSE, &fi)){
				int rows = height / fi.dwFontSize.Y;
				CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
				if (GetConsoleScreenBufferInfo(hOutput, &consoleScreenBufferInfo)){
					BOOL vscroll = consoleScreenBufferInfo.dwSize.Y > rows;
					int width = mi.rcWork.right - mi.rcWork.left + 1 - GetNonClientSizeX(vscroll);
					result.X = width / fi.dwFontSize.X;
					result.Y = rows;
				}
			}
		}
	}
	return result;
}

// Sets console window size. Note it doesn't change the physical window size. You still need to move the window and resize it using MoveWindow.
// Call GetLastError if it returns FALSE.
BOOL SetConsoleWindowSize(HANDLE hOutput, COORD size)
{
	CONSOLE_SCREEN_BUFFER_INFOEX info;
	info.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	if (!GetConsoleScreenBufferInfoEx(hOutput, &info)){
		return FALSE;
	}

	info.dwSize.X = size.X;
	info.dwSize.Y = size.Y;
	if (!SetConsoleScreenBufferInfoEx(hOutput, &info)){
		return FALSE;
	}

	return TRUE;
}

// Move a console window that has been resized its character-based window size based on largest size on a monitor.
BOOL MoveConsoleWindow(HWND hwndConsole, HMONITOR hMonitor)
{
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfo(hMonitor, &mi)){
		int width = mi.rcWork.right - mi.rcWork.left + 1; // this is the maximum width of the work area of the monitor
		// however, if I just pass it to MoveWindow below, then the console window will still show a horizontal scrollbar
		// as the console window cannot be resized bigger than its character-based sizes, it is fine to just pass a bigger width to make the horizontal scrollbar disappear
		if (MoveWindow(hwndConsole, mi.rcWork.left, mi.rcWork.top, width * 2, mi.rcWork.bottom - mi.rcWork.top + 1, TRUE)){
			return TRUE;
		}
	}
	return FALSE;
}

// The final function that wraps other calls to easily resize and move current console window to specified monitor.
// hMonitor can be NULL, which means to use the monitor that the console window currently is on.
BOOL MaximizeCurrentConsoleWindowOnMonitor(HMONITOR hMonitor)
{
	if (hMonitor == NULL) {
		HWND hwndConsole = GetConsoleWindow();
		if (hwndConsole == NULL){
			return FALSE;
		}
		hMonitor = MonitorFromWindow(hwndConsole, MONITOR_DEFAULTTONEAREST);
		if (hMonitor == NULL) {
			return FALSE;
		}
	}

	HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOutput != NULL) {
		HWND hwndConsole = GetConsoleWindow();
		if (hwndConsole != NULL) {
			COORD max = GetLargestConsoleWindowSize2(hOutput, hMonitor);
			if (max.X != 0 && max.Y != 0) {
				if (SetConsoleWindowSize(hOutput, max)){
					if (MoveConsoleWindow(hwndConsole, hMonitor)){
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

struct MyMonitors {
	HMONITOR monitors[16];	// maximum count of monitors, this should be enough?
	int count;
};

BOOL CALLBACK MyMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MyMonitors* monitors = (MyMonitors*)dwData;
	monitors->monitors[monitors->count++] = hMonitor;
	return TRUE;
}

BOOL MaximizeCurrentConsoleWindowOnMonitorByIndex(int monitorIndex)
{
	MyMonitors monitors = {0};
	if (EnumDisplayMonitors(NULL, NULL, MyMonitorEnumProc, (LPARAM) &monitors)){
		if (monitorIndex > monitors.count - 1) {
			monitorIndex = 0;
		}
		return MaximizeCurrentConsoleWindowOnMonitor(monitors.monitors[monitorIndex]);
	}
	return FALSE;
}

int _tmain(int argc, const char* argv[])
{
	BOOL ret;
	if (argc == 2) {
		int monitorIndex = atoi(argv[1]);
		ret = MaximizeCurrentConsoleWindowOnMonitorByIndex(monitorIndex);
	} else {
		ret = MaximizeCurrentConsoleWindowOnMonitor(NULL);
	}
	if (!ret) {
		return GetLastError();
	}
	return 0;
}

