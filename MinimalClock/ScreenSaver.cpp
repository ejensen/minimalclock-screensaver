#define VC_EXTRALEAN
#define _USE_MATH_DEFINES

#include <windows.h>
#include <commctrl.h>
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

static HDC g_hDC = NULL;
static HBITMAP g_hBmp = NULL;
static HFONT g_hFontResource = NULL;
static HFONT g_hLargeFont = NULL;
static HFONT g_hSmallFont = NULL;
static SIZE g_screenSize;
static SIZE g_largeTextSize;
static BOOL g_is24Hour;
static BOOL g_AnimatedClockHands;

static GdiplusStartupInput g_gdiplusStartupInput;
static ULONG_PTR g_gdiplusToken;

TCHAR szAppName[APPNAMEBUFFERLEN];// = L"MinimalClock";
TCHAR szIniFile[MAXFILELEN];// = L"MinClock.ini";
const TCHAR g_szFormatOptionName[] = L"Use24Hour";
const TCHAR g_szClockHandOptionName[] = L"AnimatedClockHands";

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
			ReleaseDC(hWnd, g_hDC);
			DeleteObject(g_hDC);
			DeleteObject(g_hBmp);
			DeleteObject(g_hLargeFont);
			DeleteObject(g_hSmallFont);
			RemoveFontMemResourceEx(g_hFontResource);
			GdiplusShutdown(g_gdiplusToken);
			break;
		}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			BitBlt(hdc, 0, 0, g_screenSize.cx, g_screenSize.cy, g_hDC, 0, 0, SRCCOPY);
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
	g_is24Hour = GetPrivateProfileInt(szAppName, g_szFormatOptionName, FALSE, szIniFile);
	g_AnimatedClockHands = GetPrivateProfileInt(szAppName, g_szClockHandOptionName, FALSE, szIniFile);
}

void Init(HWND hWnd)
{
	LoadConfiguration();

	GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, NULL);

	RECT r;
	GetClientRect(hWnd, &r);

	g_screenSize.cy = r.bottom - r.top;
	g_screenSize.cx = r.right - r.left;

	HDC hDc = GetDC(hWnd);
	g_hBmp = CreateCompatibleBitmap(hDc, g_screenSize.cx, g_screenSize.cy);
	g_hDC = CreateCompatibleDC(hDc);
	ReleaseDC(hWnd, hDc);
	SelectObject(g_hDC, g_hBmp);

	SetTextColor(g_hDC, RGB(255, 255, 255));
	SetBkColor(g_hDC, RGB(0, 0, 0));

	{	// Load Geo-sans Light font from application resources.
		HMODULE hResInstance = NULL;
		HRSRC hResource = FindResource(hResInstance, MAKEINTRESOURCE(IDR_FONT_GEOSANSLIGHT), L"BINARY");
		if (hResource) 
		{
			HGLOBAL mem = LoadResource(hResInstance, hResource);
			if (mem)
			{
				void *data = LockResource(mem);
				size_t len = SizeofResource(hResInstance, hResource);

				DWORD nFonts;
				g_hFontResource = (HFONT)AddFontMemResourceEx(data, len, NULL, &nFonts);
			}
		}

		LOGFONT lf;
		memset(&lf, 0, sizeof(lf));
		wcscpy_s(lf.lfFaceName, L"GeosansLight");
		lf.lfQuality = ANTIALIASED_QUALITY;
		lf.lfHeight = g_screenSize.cy / 5;

		g_hLargeFont = CreateFontIndirect(&lf);

		SelectObject(g_hDC, g_hLargeFont);

		std::wstring strPlaceHolder(L"00:00:00");
		GetTextExtentPoint32(g_hDC, strPlaceHolder.c_str(), strPlaceHolder.size(), &g_largeTextSize);

		lf.lfHeight = g_screenSize.cy / 28;
		g_hSmallFont = CreateFontIndirect(&lf);
	}

	SetTimer(hWnd, DRAW_TIMER, 1000, NULL);
	UpdateFrame(hWnd);
}

void UpdateFrame(HWND hWnd)
{
	tm timeinfo;
	time_t rawtime;
	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);

	REAL clock_diameter = g_screenSize.cy / 25.0f;
	REAL clock_left = g_screenSize.cx - g_largeTextSize.cx - g_largeTextSize.cx / 8.0f;
	
	{ 	// Draw analog clock
		REAL clock_radius = clock_diameter / 2.0f;
		REAL clock_top = (g_screenSize.cy + g_largeTextSize.cy) / 2.0f - clock_diameter * 2.0f;

		REAL clock_centerX = clock_left + clock_radius;
		REAL clock_centerY = clock_top  + clock_radius;

		REAL clock_handWidth = clock_radius / 4.0f;
		REAL clock_handGap = clock_handWidth * 1.75;

		REAL clock_minuteAngle = -M_PI_2;
		REAL clock_hourAngle = M_PI_4;

		if (g_AnimatedClockHands) 
		{
			clock_minuteAngle = timeinfo.tm_min  / 60.0f * M_PI * 2 - M_PI_2;
			clock_hourAngle   = timeinfo.tm_hour / 12.0f * M_PI * 2 - M_PI_2 + clock_minuteAngle / 12;
		}

		Graphics graphics(g_hDC);
		graphics.SetSmoothingMode(SmoothingModeAntiAlias);
		graphics.Clear(RGB(0, 0, 0));

		Pen pen(Color(255,255,255), clock_handWidth + 1);
		graphics.DrawEllipse(&pen, 
			clock_left,
			clock_top,
			clock_diameter,
			clock_diameter);

		pen.SetWidth(clock_handWidth - 0.5f);
		pen.SetStartCap(LineCapRound);

		graphics.DrawLine(&pen, 
			clock_centerX,
			clock_centerY,
			clock_centerX + cos(clock_hourAngle) * (clock_radius - clock_handGap),
			clock_centerY + sin(clock_hourAngle) * (clock_radius - clock_handGap));

		graphics.DrawLine(&pen, 
			clock_centerX,
			clock_centerY,
			clock_centerX + cos(clock_minuteAngle) * (clock_radius - clock_handGap),
			clock_centerY + sin(clock_minuteAngle) * (clock_radius - clock_handGap));
	}

	{	// Draw time text
		const size_t nBufferSize = 16;
		TCHAR szBuffer[nBufferSize];

		wcsftime(szBuffer, nBufferSize, g_is24Hour ? L"%H:%M:%S" : L"%I:%M:%S", &timeinfo);
		std::wstring strval(szBuffer);

		SelectObject(g_hDC, g_hLargeFont);
		TextOut(g_hDC, g_screenSize.cx - g_largeTextSize.cx, (g_screenSize.cy - g_largeTextSize.cy) / 2, strval.c_str(), strval.size());

		if (!g_is24Hour)
		{	// Draw AM/PM
			wcsftime(szBuffer, nBufferSize, L"%p", &timeinfo);
			strval = szBuffer;

			SelectObject(g_hDC, g_hSmallFont);
			SIZE smallTextSize;
			GetTextExtentPoint32(g_hDC, strval.c_str(), strval.size(), &smallTextSize);
			TextOut(g_hDC, clock_left - (smallTextSize.cx - clock_diameter) / 2, (g_screenSize.cy - smallTextSize.cy) / 2, strval.c_str(), strval.size());
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
			CheckDlgButton(hDlg, IDC_24HOUR_CHECK, g_is24Hour);
			return TRUE;
		}
	case WM_COMMAND:
		{
			switch(LOWORD(wParam)) 
			{ 
			case ID_OK:
				{
					TCHAR szBuffer[8];
					g_is24Hour = IsDlgButtonChecked(hDlg, IDC_24HOUR_CHECK);
					_itow(g_is24Hour, szBuffer, 10);
					WritePrivateProfileString(szAppName, g_szFormatOptionName, szBuffer, szIniFile); 
				}
			case ID_CANCEL:
				{
					EndDialog(hDlg, LOWORD(wParam) == ID_OK); 
					return TRUE; 
				}
			}
			break;
		}
	case WM_CLOSE:
		{
			EndDialog(hDlg, FALSE); 
			return TRUE; 
		}
	}
	return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE /*hInst*/)
{
	InitCommonControls();
	return TRUE;
}
