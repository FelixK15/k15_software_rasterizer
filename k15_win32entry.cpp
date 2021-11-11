#define _CRT_SECURE_NO_WARNINGS

#include "k15_base.hpp"

#define K15_SOFTWARE_RASTERIZER_IMPLEMENTATION
#include "k15_software_rasterizer.hpp"

#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Gdi32.lib")

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

using namespace k15;

void allocateDebugConsole()
{
	AllocConsole();
	AttachConsole(ATTACH_PARENT_PROCESS);
	freopen("CONOUT$", "w", stdout);
}

software_rasterizer_context* pContext = nullptr;
uint32* pBackBufferPixels = 0;

BITMAPINFO* pBackBufferBitmapInfo = 0;
HBITMAP backBufferBitmap = 0;

int screenWidth = 800;
int screenHeight = 600;

void createBackBuffer(HWND hwnd, int width, int height)
{		
	if (pBackBufferBitmapInfo != NULL)
	{
		free(pBackBufferBitmapInfo);
		pBackBufferBitmapInfo = NULL;
		pBackBufferPixels = NULL;
	}

	if (backBufferBitmap != NULL)
	{
		DeleteObject(backBufferBitmap);
		backBufferBitmap = NULL;
	}

	pBackBufferBitmapInfo = ( BITMAPINFO* )malloc( sizeof(BITMAPINFO) );
	pBackBufferBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFO);
	pBackBufferBitmapInfo->bmiHeader.biWidth = width;
	pBackBufferBitmapInfo->bmiHeader.biHeight = -(int)height;
	pBackBufferBitmapInfo->bmiHeader.biPlanes = 1;
	pBackBufferBitmapInfo->bmiHeader.biBitCount = 32;
	pBackBufferBitmapInfo->bmiHeader.biCompression = BI_RGB;
	//FK: XRGB

	HDC deviceContext = GetDC(hwnd);
	backBufferBitmap = CreateDIBSection( deviceContext, pBackBufferBitmapInfo, DIB_RGB_COLORS, (void**)&pBackBufferPixels, NULL, 0 );   
	if (backBufferBitmap == NULL && pBackBufferPixels == NULL)
	{
		MessageBoxA(0, "Error during CreateDIBSection.", "Error!", 0);
	}

	screenWidth = width;
	screenHeight = height;

	if( pContext != nullptr )
	{
		set_back_buffer( pContext, pBackBufferPixels, screenWidth, screenHeight );
	}
}

void K15_WindowCreated(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{

}

void K15_WindowClosed(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{

}

void K15_KeyInput(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{

}

void K15_MouseButtonInput(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{

}

void K15_MouseMove(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{

}

void K15_MouseWheel(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{

}

void K15_WindowResized(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	WORD newWidth = (WORD)(lparam);
	WORD newHeight = (WORD)(lparam >> 16);

	createBackBuffer(hwnd, newWidth, newHeight);
}

LRESULT CALLBACK K15_WNDPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	bool8 messageHandled = false;

	switch (message)
	{
	case WM_CREATE:
		K15_WindowCreated(hwnd, message, wparam, lparam);
		break;

	case WM_CLOSE:
		K15_WindowClosed(hwnd, message, wparam, lparam);
		PostQuitMessage(0);
		messageHandled = true;
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		K15_KeyInput(hwnd, message, wparam, lparam);
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
		K15_MouseButtonInput(hwnd, message, wparam, lparam);
		break;

	case WM_MOUSEMOVE:
		K15_MouseMove(hwnd, message, wparam, lparam);
		break;

	case WM_MOUSEWHEEL:
		K15_MouseWheel(hwnd, message, wparam, lparam);
		break;

	case WM_SIZE:
		K15_WindowResized(hwnd, message, wparam, lparam);
		break;
	}

	if (messageHandled == false)
	{
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	return 0;
}

HWND setupWindow(HINSTANCE instance, int width, int height)
{
	WNDCLASS wndClass = {0};
	wndClass.style = CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wndClass.hInstance = instance;
	wndClass.lpszClassName = "K15_Win32Template";
	wndClass.lpfnWndProc = K15_WNDPROC;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wndClass);

	HWND hwnd = CreateWindowA("K15_Win32Template", "Win32 Template",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		width, height, 0, 0, instance, 0);

	if (hwnd == INVALID_HANDLE_VALUE)
	{
		MessageBox(0, "Error creating Window.\n", "Error!", 0);
	}
	else
	{
		ShowWindow(hwnd, SW_SHOW);

		RECT clientRect;
		GetClientRect(hwnd, &clientRect);

		const int cWidth = clientRect.right - clientRect.left;
		const int cHeight = clientRect.bottom - clientRect.top;
		
		createBackBuffer(hwnd, cWidth, cHeight);
	}
	return hwnd;
}

uint32 getTimeInMilliseconds(LARGE_INTEGER PerformanceFrequency)
{
	LARGE_INTEGER appTime = {0};
	QueryPerformanceFrequency(&appTime);

	appTime.QuadPart *= 1000; //to milliseconds

	return (uint32)(appTime.QuadPart / PerformanceFrequency.QuadPart);
}

bool setup()
{
	software_rasterizer_context_init_parameters parameters;
	parameters.backBufferWidth 	= screenWidth;
	parameters.backBufferHeight = screenHeight;
	parameters.pBackBuffer 		= pBackBufferPixels;
	
	const k15::result<void> initResult = create_software_rasterizer_context( &pContext, parameters );
	if( initResult.hasError() )
	{
		printFormattedString( "Could not create software rasterizer context. Error=%s\n", initResult.getError() );
		return false;
	}

	return true;
}

void drawBackBuffer(HWND hwnd)
{
	HDC deviceContext = GetDC( hwnd );
	StretchDIBits( deviceContext, 0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, 
		pBackBufferPixels, pBackBufferBitmapInfo, DIB_RGB_COLORS, SRCCOPY );  

	fillMemory< uint32 >( pBackBufferPixels, 0u, screenWidth * screenHeight );
}

void doFrame(uint32 DeltaTimeInMS)
{
	begin_geometry( pContext, topology::triangle );
	vertex_position( pContext, 1.0f, 1.0f, 1.0f );
	vertex_position( pContext, 1.0f, 1.0f, 1.0f );
	vertex_position( pContext, 1.0f, 1.0f, 1.0f );
	end_geometry( pContext );

	draw_geometry( pContext );
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
	LARGE_INTEGER performanceFrequency;
	QueryPerformanceFrequency(&performanceFrequency);

	allocateDebugConsole();

	HWND hwnd = setupWindow(hInstance, 800, 600);

	if (hwnd == INVALID_HANDLE_VALUE)
	{
		return -1;
	}

	setup();

	uint32 timeFrameStarted = 0;
	uint32 timeFrameEnded = 0;
	uint32 deltaMs = 0;

	bool8 loopRunning = true;
	MSG msg = {0};

	while (loopRunning)
	{
		timeFrameStarted = getTimeInMilliseconds(performanceFrequency);

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
		{
			if (msg.message == WM_QUIT)
			{
				loopRunning = false;
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!loopRunning)
		{
			break;
		}

		doFrame(deltaMs);
		drawBackBuffer(hwnd);

		timeFrameEnded = getTimeInMilliseconds(performanceFrequency);
		deltaMs = timeFrameEnded - timeFrameStarted;
	}

	DestroyWindow(hwnd);

	return 0;
}