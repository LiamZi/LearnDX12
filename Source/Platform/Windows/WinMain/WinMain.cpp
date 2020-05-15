#include <windows.h>
#include "Resource.h"
#include <NixApplication.hpp>
#include <Nix/io/archive.h>
#include <string>
#include <regex>

#define MAX_LOADSTRING 100

static int captionHeight;
static int systemMenuHeight;
static int frameThicknessX;
static int frameThicknessY;

HINSTANCE hInst;                                
// WCHAR szTitle[MAX_LOADSTRING];                  
// WCHAR szWindowClass[MAX_LOADSTRING];     
LPCTSTR szTitle = L"HelloDX12";
LPCTSTR szWindowClass = L"DX12 Window";

NixApplication* object = nullptr;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain
(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	// LoadStringW(hInstance, IDC_EMPLTYWINDOW, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow))
	{
        MessageBox(0, L"Window Initialization - Failed", L"Error", MB_OK);
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EMPLTYWINDOW));
	//
	return object->run();
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EMPLTYWINDOW));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_EMPLTYWINDOW);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	captionHeight = GetSystemMetrics(SM_CYCAPTION);
	systemMenuHeight = GetSystemMetrics(SM_CYMENU);
	frameThicknessX = GetSystemMetrics(SM_CXSIZEFRAME);
	frameThicknessY = GetSystemMetrics(SM_CYSIZEFRAME);

	HWND hWnd = CreateWindowW(szWindowClass, L"HelloDX12", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 800, 600, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd) {
		return FALSE;
	}

	object = getApplication();

	char pathBuff[256];
	GetModuleFileNameA(NULL, pathBuff, 256);
	std::string assetRoot = "./";
	const char expr[] = R"(^(.*)\\[_0-9a-zA-Z]*.exe$)";
	std::regex pathRegex(expr);
	std::smatch result;
	std::string content(pathBuff);
	bool succees = std::regex_match(content, result, pathRegex);
	if (succees) {
		assetRoot = result[1];
	}
	assetRoot.append("/../../");
	auto archieve = Nix::CreateStdArchieve( assetRoot );
    if(!object->initialize(hWnd, archieve)) {
		return FALSE;
	}

	HDC hdcScreen = GetDC(NULL);
	int iX = GetDeviceCaps(hdcScreen, HORZRES);    // pixel
	int iY = GetDeviceCaps(hdcScreen, VERTRES);    // pixel
	int iPhsX = GetDeviceCaps(hdcScreen, HORZSIZE);    // mm
	int iPhsY = GetDeviceCaps(hdcScreen, VERTSIZE);    // mm
	if (NULL != hdcScreen){
		DeleteDC(hdcScreen);
	}
#define INCH 0.03937
	float iTemp = (float)(iPhsX * iPhsX + iPhsY * iPhsY);
	float fInch = (float)(sqrt(iTemp) * INCH);
	iTemp = (float)(iX * iX + iY * iY);
	float fPixel = sqrt(iTemp);

	float iDPI = fPixel / fInch;    // dpi pixel/inch
	//
	SetWindowTextA(hWnd, object->title());
	//
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT cx = LOWORD(lParam);
	UINT cy = HIWORD(lParam);

	switch (message)
	{
		case WM_ACTIVATE:
		{
			if(LOWORD(wParam) == WA_INACTIVE)
			{
				object->setAppPaused(true);
				object->getTimer().stop();
			}
			else
			{
				object->setAppPaused(false);
				object->getTimer().start();
			}
			return 0;
		}
		break;
		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			switch (wmId)
			{
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			case WM_KEYDOWN:
			{
				object->onKeyEvent((unsigned char)wParam, NixApplication::eKeyDown);
				break;
			}
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		break;
		case WM_DESTROY:
			object->release();
			PostQuitMessage(0);
			break;
		case WM_SIZE:
		{
			// UINT cx = LOWORD(lParam);
			// UINT cy = HIWORD(lParam);
			if(!object) break;
			switch (wParam)
			{
			case SIZE_MINIMIZED:
			{
				object->setAppPaused(true);
				object->setMinimized(true);
				object->setMaximized(false);
				
			}
				break;
			case SIZE_MAXIMIZED:
			{
				object->setAppPaused(false);
				object->setMinimized(false);
				object->setMaximized(true);
				object->resize(cx, cy);
			}
				break;
			case SIZE_RESTORED:
			{
				if(object->isMinimized())
				{
					object->setAppPaused(false);
					object->setMinimized(false);
				}
				else if(object->isMaximized()) 
				{
					object->setAppPaused(false);
					object->setMaximized(false);
					
				}
				else if(object->isResizing()) 
				{

				}
				
				object->resize(cx, cy);
			}
				break;
				
			default:
				break;
			}
			
			break;
		}
		case WM_ENTERSIZEMOVE:
		{
			object->setAppPaused(true);
			object->setResizing(true);
			object->getTimer().stop();
		}
			break;
		case WM_EXITSIZEMOVE:
		{
			object->setAppPaused(false);
			object->setResizing(false);
			object->getTimer().start();
			object->resize(cx, cy);
		}
			break;
		case WM_KEYDOWN:
		{
			object->onKeyEvent((unsigned char)wParam, NixApplication::eKeyDown);
			break;
		}
		case WM_KEYUP:
		{
			switch (wParam)
			{
			case VK_ESCAPE:
			{
				//PostQuitMessage(0);
				break;
			}
			default:
				object->onKeyEvent((unsigned char)wParam, NixApplication::eKeyUp);
				break;
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			short x = LOWORD(lParam);
			short y = HIWORD(lParam);
			switch (wParam)
			{
			case MK_LBUTTON:
				object->onMouseEvent(NixApplication::LButtonMouse, NixApplication::MouseMove, x, y);
				break;
			case MK_RBUTTON:
				object->onMouseEvent(NixApplication::RButtonMouse, NixApplication::MouseMove, x, y);
				break;
			case MK_MBUTTON:
				object->onMouseEvent(NixApplication::MButtonMouse, NixApplication::MouseMove, x, y);
				break;
			}
			break;
		}
		case WM_RBUTTONDOWN:
		{
			short x = LOWORD(lParam);
			short y = HIWORD(lParam);
			object->onMouseEvent(NixApplication::RButtonMouse, NixApplication::MouseDown, x, y);
			break;
		}
		case WM_RBUTTONUP:
		{
			short x = LOWORD(lParam);
			short y = HIWORD(lParam);
			object->onMouseEvent(NixApplication::RButtonMouse, NixApplication::MouseUp, x, y);
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
