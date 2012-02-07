#define VC_EXTRALEAN
//#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES

#include <windows.h>
#include <scrnsave.h>
#include <gdiplus.h>
#include <time.h>
#include <math.h>
#include <string>
#include <vector>

#include "resource.h"

#pragma comment(lib, "scrnsavw.lib")
#pragma comment (lib, "comctl32.lib")

using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")

HDC m_hDC = NULL;
HBITMAP m_hBmp = NULL;
HFONT m_hFontResource = NULL;
HFONT m_hFont = NULL;
int m_backBufferCX, m_backBufferCY;
SIZE m_textSize;
bool m_is24Hour;


GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;

#define DRAW_TIMER 0x1

void Init(HWND hWnd);
void UpdateFrame(HWND hWnd);

LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
			RemoveFontMemResourceEx(m_hFontResource);
			GdiplusShutdown(gdiplusToken);
			PostQuitMessage(0);
			break;
		}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			BitBlt(hdc, 0, 0, m_backBufferCX, m_backBufferCY, m_hDC, 0, 0, SRCCOPY);
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

void Init(HWND hWnd)
{
	TCHAR szAppName[80];
	TCHAR szOptionName[] = L"Use 24 Hour";
	LoadString(hMainInstance, idsAppName, szAppName, 80 * sizeof(TCHAR));
	LoadString(hMainInstance, idsIniFile, szIniFile, MAXFILELEN * sizeof(TCHAR));
	m_is24Hour = GetPrivateProfileInt(szAppName, szOptionName, FALSE, szIniFile);

	m_backBufferCX = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	m_backBufferCY = GetSystemMetrics(SM_CYVIRTUALSCREEN);

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

	m_hBmp = CreateCompatibleBitmap(hDc, m_backBufferCX, m_backBufferCY);

	m_hDC = CreateCompatibleDC(hDc);
	ReleaseDC(hWnd, hDc);

	SelectObject(m_hDC, m_hBmp);

	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight = m_backBufferCY / 6;
	lf.lfQuality = ANTIALIASED_QUALITY;
	wcscpy_s(lf.lfFaceName, L"GeosansLight");

	m_hFont = CreateFontIndirect(&lf);

	SelectObject(m_hDC, m_hFont);

	SetTextColor(m_hDC, RGB(255, 255, 255));
	SetBkColor(m_hDC, RGB(0, 0, 0));

	SetTimer(hWnd, DRAW_TIMER, 1000, NULL);
	UpdateFrame(hWnd);
}

void UpdateFrame(HWND hWnd)
{
	tm timeinfo;
	time_t rawtime;
	time(&rawtime);

	localtime_s(&timeinfo, &rawtime);
	std::vector<TCHAR> buffer(16);

	wcsftime(buffer.data(), buffer.size(), m_is24Hour ? L"%H:%M:%S" : L"%I:%M:%S", &timeinfo);
	std::wstring strval(buffer.data());

	std::wstring strPlaceHolder(L"00:00:00");
	GetTextExtentPoint32(m_hDC, strPlaceHolder.c_str(), strPlaceHolder.size(), &m_textSize);

	Graphics graphics(m_hDC);
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);
	graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
	graphics.SetCompositingQuality(CompositingQualityHighQuality);

	graphics.Clear(RGB(0, 0, 0));

	TextOut(m_hDC, m_backBufferCX - m_textSize.cx, (m_backBufferCY - m_textSize.cy) / 2, strval.c_str(), strval.size());
	
	REAL diameter = m_textSize.cy / 6;
	REAL gap = diameter / 6;
	REAL radius = diameter / 2;
	REAL left = m_backBufferCX - m_textSize.cx - m_textSize.cy / 3;
	REAL top = m_backBufferCY / 2 + radius;
	REAL centerX = left + radius;
	REAL centerY = top + radius;
	INT lineWidth = 3;
	REAL angle = M_PI * 1 / 4;

	Pen pen(Color(255,255,255), lineWidth + 2);
	graphics.DrawEllipse(&pen, 
		left,
		top,
		diameter,
		diameter);

	pen.SetWidth(lineWidth);
	//pen.SetStartCap(LineCapRound);

	graphics.DrawLine(&pen, 
		centerX,
		centerY + lineWidth / 4,
		centerX,
		centerY - (radius - gap));

	graphics.DrawLine(&pen, 
		centerX,
		centerY,
		centerX + cos(angle) * (radius - gap),
		centerY + sin(angle) * (radius - gap));

	if (!m_is24Hour)
	{
		wcsftime(buffer.data(), buffer.size(), L"%p", &timeinfo);
		strval = buffer.data();

		Font font(L"GeosansLight", m_textSize.cy / 8);
		SolidBrush brush(Color(255, 255, 255));
		graphics.DrawString(strval.c_str(), strval.size(), &font, PointF(left, (m_backBufferCY - m_textSize.cy / 8) / 2), &brush);
	}

	RECT r;
	GetClientRect(hWnd, &r);
	InvalidateRect(hWnd, &r, false);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND /*hDlg*/, UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	//Put your configuration interface code here
	return(FALSE);
}

BOOL WINAPI RegisterDialogClasses(HANDLE /*hInst*/)
{
	return(TRUE);
}
