#define _CRT_SECURE_NO_WARNINGS

#include "k15_base.hpp"

#define K15_SOFTWARE_RASTERIZER_IMPLEMENTATION
#include "k15_software_rasterizer.hpp"

#define NOMINMAX
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
uint32_t* pBackBufferPixels = 0;

BITMAPINFO* pBackBufferBitmapInfo = 0;
HBITMAP backBufferBitmap = 0;

int virtualScreenWidth = 320;
int virtualScreenHeight = 240;

int screenWidth = virtualScreenWidth*3;
int screenHeight = virtualScreenHeight*3;

vector3 cameraPos = {};
vector3 cameraVelocity = {};

constexpr vector3 test_cube_vertices[] = {
	{-0.5f, -0.5f, 1.0f},
	{ 0.5f, -0.5f, 1.0f},
	{-0.5f,  0.5f, 1.0f},

	{ 0.5f, -0.5f, 1.0f},
	{ 0.5f,  0.5f, 1.0f},
	{-0.5f,  0.5f, 1.0f},

	{-0.5f, -0.5f, 2.0f},
	{ 0.5f, -0.5f, 2.0f},
	{-0.5f,  0.5f, 2.0f},

	{ 0.5f, -0.5f, 2.0f},
	{ 0.5f,  0.5f, 2.0f},
	{-0.5f,  0.5f, 2.0f},

	{-0.5f, -0.5f, 1.0f},
	{-0.5f,  0.5f, 1.0f},
	{-0.5f,  0.5f, 2.0f},

	{-0.5f,  0.5f, 2.0f},
	{-0.5f, -0.5f, 2.0f},
	{-0.5f, -0.5f, 1.0f},

	{ 0.5f, -0.5f, 1.0f},
	{ 0.5f,  0.5f, 1.0f},
	{ 0.5f,  0.5f, 2.0f},

	{ 0.5f,  0.5f, 2.0f},
	{ 0.5f, -0.5f, 2.0f},
	{ 0.5f, -0.5f, 1.0f},

	{ 0.5f, 0.5f, 2.0f},
	{-0.5f, 0.5f, 2.0f},
	{-0.5f, 0.5f, 1.0f},

	{-0.5f, 0.5f, 1.0f},
	{ 0.5f, 0.5f, 1.0f},
	{ 0.5f, 0.5f, 2.0f},

	{ 0.5f, -0.5f, 2.0f},
	{-0.5f, -0.5f, 2.0f},
	{-0.5f, -0.5f, 1.0f},

	{-0.5f, -0.5f, 1.0f},
	{ 0.5f, -0.5f, 1.0f},
	{ 0.5f, -0.5f, 2.0f},
};

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
}

void K15_WindowCreated(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{

}

void K15_WindowClosed(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{

}

void K15_KeyInput(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	const bool isKeyDown 	= (lparam & (1<<31)) == 0;
	const bool firstKeyDown = (lparam & (1<<30)) == 0;
	const bool isKeyUp 		= !isKeyDown;

	if(wparam == VK_DOWN)
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.z -= 0.01f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.z = 0.0f;
		}
	}

	if(wparam == VK_UP)
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.z += 0.01f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.z = 0.0f;
		}
	}
	
	if(wparam == VK_LEFT)
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.x += 0.01f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.x = 0.0f;
		}
	}

	if(wparam == VK_RIGHT)
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.x -= 0.01f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.x = 0.0f;
		}
	}
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

	screenWidth = newWidth;
	screenHeight = newHeight;

	//createBackBuffer(hwnd, newWidth, newHeight);
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
	RECT windowRect = {};
	windowRect.right = width;
	windowRect.bottom = height;
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	WNDCLASS wndClass = {0};
	wndClass.style = CS_HREDRAW | CS_OWNDC | CS_VREDRAW;
	wndClass.hInstance = instance;
	wndClass.lpszClassName = "K15_Win32Template";
	wndClass.lpfnWndProc = K15_WNDPROC;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wndClass);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	HWND hwnd = CreateWindowA("K15_Win32Template", "Win32 Template",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight, 0, 0, instance, 0);

	if (hwnd == INVALID_HANDLE_VALUE)
	{
		MessageBox(0, "Error creating Window.\n", "Error!", 0);
	}
	else
	{
		ShowWindow(hwnd, SW_SHOW);
		createBackBuffer(hwnd, virtualScreenWidth, virtualScreenHeight);

		screenWidth = windowWidth;
		screenHeight = windowHeight;

		if( pContext != nullptr )
		{
			uint32_t stride = virtualScreenWidth;
			set_back_buffer( pContext, pBackBufferPixels, virtualScreenWidth, virtualScreenHeight, stride );
		}
	}
	return hwnd;
}

uint32_t getTimeInMilliseconds(LARGE_INTEGER PerformanceFrequency)
{
	LARGE_INTEGER appTime = {0};
	QueryPerformanceFrequency(&appTime);

	appTime.QuadPart *= 1000; //to milliseconds

	return (uint32_t)(appTime.QuadPart / PerformanceFrequency.QuadPart);
}

bool setup()
{
	software_rasterizer_context_init_parameters parameters;
	parameters.backBufferWidth 	= virtualScreenWidth;
	parameters.backBufferHeight = virtualScreenHeight;
	parameters.backBufferStride = virtualScreenWidth;
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
	StretchDIBits( deviceContext, 0, 0, screenWidth, screenHeight, 0, 0, virtualScreenWidth, virtualScreenHeight, 
		pBackBufferPixels, pBackBufferBitmapInfo, DIB_RGB_COLORS, SRCCOPY );  

	fillMemory< uint32_t >( pBackBufferPixels, 0u, virtualScreenWidth * virtualScreenHeight );
}

void doFrame(uint32_t DeltaTimeInMS)
{
#if 1
	begin_geometry( pContext, topology::triangle );

	constexpr uint32_t vertexCount = sizeof(test_cube_vertices)/sizeof(vector3);
	for(uint32_t i = 0; i < vertexCount; ++i)
	{
		vertex_position( pContext, test_cube_vertices[i].x, test_cube_vertices[i].y, test_cube_vertices[i].z );
	}

	end_geometry( pContext );
#else
	begin_geometry( pContext, topology::triangle );
	vertex_position( pContext, -0.5f, 0.0f, 1.0f );
	vertex_position( pContext, 0.0f, 0.5f, 1.0f );
	vertex_position( pContext, 0.5f, 0.0f, 1.0f );
	end_geometry( pContext );
#endif
	
	cameraPos.x += cameraVelocity.x;
	cameraPos.y += cameraVelocity.y;
	cameraPos.z += cameraVelocity.z;

	set_camera_pos( pContext, cameraPos );
	draw_geometry( pContext );
}

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nShowCmd)
{
	LARGE_INTEGER performanceFrequency;
	QueryPerformanceFrequency(&performanceFrequency);

	allocateDebugConsole();

	HWND hwnd = setupWindow(hInstance, screenWidth, screenHeight);

	if (hwnd == INVALID_HANDLE_VALUE)
	{
		return -1;
	}

	setup();

	uint32_t timeFrameStarted = 0;
	uint32_t timeFrameEnded = 0;
	uint32_t deltaMs = 0;

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