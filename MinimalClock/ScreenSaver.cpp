#define VC_EXTRALEAN
//#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES

#include <windows.h>
#include <scrnsave.h>
#include <gdiplus.h>
#include <time.h>
#include <math.h>
#include <string>

#include "resource.h"

#pragma comment(lib, "scrnsavw.lib")
#pragma comment (lib, "comctl32.lib")

using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")

HDC m_hDC = NULL;
HBITMAP m_hBmp = NULL;
HFONT m_hFontResource = NULL;
HFONT m_hFont = NULL;
HFONT m_hSmallFont = NULL;
SIZE m_screenSize;
SIZE m_textSize;
BOOL m_is24Hour;

TCHAR szAppName[] = L"MinimalClock";
TCHAR szIniFile[] = L"MinClock.ini";
const TCHAR m_szOptionName[] = L"Use24Hour";

GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;

#define DRAW_TIMER 0x1

void Init(HWND hWnd);
void UpdateFrame(HWND hWnd);

LRESULT WINAPI ScreenSaverProcW(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		{
			Init(hWnd);
			break;
		}
	case WM_DESTROY:
		{
			KillTimer(hWnd, DRAW_TIMER);
			ReleaseDC(hWnd, m_hDC);
			DeleteObject(m_hDC);
			DeleteObject(m_hBmp);
			DeleteObject(m_hFont);
			DeleteObject(m_hSmallFont);
			RemoveFontMemResourceEx(m_hFontResource);
			GdiplusShutdown(gdiplusToken);
			break;
		}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			BitBlt(hdc, 0, 0, m_screenSize.cx, m_screenSize.cy, m_hDC, 0, 0, SRCCOPY);
			EndPaint(hWnd, &ps);
			return 0;
			break;
		}
	case WM_TIMER:
		{
			if (wParam == DRAW_TIMER)
				UpdateFrame(hWnd);
			break;
		}
	}

	return DefScreenSaverProc(hWnd, message, wParam, lParam);
}

void LoadConfiguration() 
{
	LoadString(hMainInstance, idsAppName, szAppName, APPNAMEBUFFERLEN * sizeof(TCHAR));
	LoadString(hMainInstance, idsIniFile, szIniFile, MAXFILELEN * sizeof(TCHAR));
	m_is24Hour = GetPrivateProfileInt(szAppName, m_szOptionName, FALSE, szIniFile);
}

void Init(HWND hWnd)
{
	LoadConfiguration();

	m_screenSize.cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	m_screenSize.cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	HMODULE hResInstance = NULL;
	HRSRC hResource = FindResource(hResInstance, MAKEINTRESOURCE(IDR_FONT_GEOSANSLIGHT), L"BINARY");
	if (hResource) 
	{
		HGLOBAL mem = LoadResource(hResInstance, hResource);
		void *data = LockResource(mem);
		size_t len = SizeofResource(hResInstance, hResource);

		DWORD nFonts;
		m_hFontResource = (HFONT)AddFontMemResourceEx(data, len, NULL, &nFonts);
	}

	HDC hDc = GetDC(hWnd);

	m_hBmp = CreateCompatibleBitmap(hDc, m_screenSize.cx, m_screenSize.cy);

	m_hDC = CreateCompatibleDC(hDc);
	ReleaseDC(hWnd, hDc);

	SelectObject(m_hDC, m_hBmp);

	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	wcscpy_s(lf.lfFaceName, L"GeosansLight");
	lf.lfQuality = ANTIALIASED_QUALITY;
	lf.lfHeight = m_screenSize.cy / 5;

	m_hFont = CreateFontIndirect(&lf);

	SelectObject(m_hDC, m_hFont);

	std::wstring strPlaceHolder(L"00:00:00");
	GetTextExtentPoint32(m_hDC, strPlaceHolder.c_str(), strPlaceHolder.size(), &m_textSize);

	lf.lfHeight = m_screenSize.cy / 28;
	m_hSmallFont = CreateFontIndirect(&lf);

	SetTextColor(m_hDC, RGB(255, 255, 255));
	SetBkColor(m_hDC, RGB(0, 0, 0));

	SetTimer(hWnd, DRAW_TIMER, 1000, NULL);
	UpdateFrame(hWnd);
}

void UpdateFrame(HWND hWnd)
{
	INT diameter = m_screenSize.cy / 25;
	INT radius = diameter / 2;
	INT gap = diameter / 5;
	INT left = m_screenSize.cx - m_textSize.cx - m_textSize.cx / 8;
	INT top = (m_screenSize.cy + m_textSize.cy) / 2 - diameter * 2;
	INT centerX = left + radius;
	INT centerY = top + radius;
	INT lineWidth = radius / 4;

	{
		Graphics graphics(m_hDC);
		graphics.SetSmoothingMode(SmoothingModeAntiAlias);
		graphics.Clear(RGB(0, 0, 0));

		Pen pen(Color(255,255,255), lineWidth);
		graphics.DrawEllipse(&pen, 
			left,
			top,
			diameter,
			diameter);

		pen.SetWidth(lineWidth - 1);

		graphics.DrawLine(&pen, 
			centerX,
			centerY + lineWidth / 4,
			centerX,
			centerY - (radius - gap));

		const REAL angle = M_PI / 4;

		graphics.DrawLine(&pen, 
			(REAL)centerX,
			(REAL)centerY,
			centerX + cos(angle) * (radius - gap),
			centerY + sin(angle) * (radius - gap));
	}

	{
		const size_t bufferSize = 16;
		TCHAR buffer[bufferSize];

		tm timeinfo;
		time_t rawtime;
		time(&rawtime);

		localtime_s(&timeinfo, &rawtime);

		wcsftime(buffer, bufferSize, m_is24Hour ? L"%H:%M:%S" : L"%I:%M:%S", &timeinfo);
		std::wstring strval(buffer);

		SelectObject(m_hDC, m_hFont);
		TextOut(m_hDC, m_screenSize.cx - m_textSize.cx, (m_screenSize.cy - m_textSize.cy) / 2, strval.c_str(), strval.size());

		if (!m_is24Hour)
		{
			wcsftime(buffer, bufferSize, L"%p", &timeinfo);
			strval = buffer;

			SelectObject(m_hDC, m_hSmallFont);
			SIZE smallTextSize;
			GetTextExtentPoint32(m_hDC, strval.c_str(), strval.size(), &smallTextSize);
			TextOut(m_hDC, left - (smallTextSize.cx - diameter) / 2, (m_screenSize.cy - smallTextSize.cy) / 2, strval.c_str(), strval.size());
		}
	}

	RECT r;
	GetClientRect(hWnd, &r);
	InvalidateRect(hWnd, &r, false);

}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch(message) 
	{ 
	case WM_INITDIALOG:
		{
			LoadConfiguration();
			CheckDlgButton(hDlg, IDC_24HOUR_CHECK, m_is24Hour);
			return TRUE;
		}
	case WM_COMMAND:
		{
			switch(LOWORD(wParam)) 
			{ 
			case ID_OK:
				{
					m_is24Hour = IsDlgButtonChecked(hDlg, IDC_24HOUR_CHECK);
					TCHAR buffer[8];
					_itow(m_is24Hour, buffer, 10);
					WritePrivateProfileString(szAppName, m_szOptionName, buffer, szIniFile); 
				}
			case ID_CANCEL:
				{
					EndDialog(hDlg, LOWORD(wParam) == ID_OK); 
					return TRUE; 
				}
			}
		}
	}
	return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE /*hInst*/)
{
	return TRUE;
}
