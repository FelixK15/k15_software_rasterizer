#define _CRT_SECURE_NO_WARNINGS

#define K15_SOFTWARE_RASTERIZER_IMPLEMENTATION
#include "../k15_software_rasterizer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Gdi32.lib")

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

void allocateDebugConsole()
{
	AllocConsole();
	AttachConsole(ATTACH_PARENT_PROCESS);
	freopen("CONOUT$", "w", stdout);
}

software_rasterizer_context_t* pContext = nullptr;
uint32_t* pBackBufferPixels = 0;
BITMAPINFO* pBackBufferBitmapInfo = 0;
HBITMAP backBufferBitmap = 0;

int virtualScreenWidth = 320;
int virtualScreenHeight = 240;

int screenWidth = virtualScreenWidth*3;
int screenHeight = virtualScreenHeight*3;

vector3f_t cameraPos = {0.0f, 0.0f, 1.5f};
vector3f_t cameraVelocity = {};

constexpr vector4f_t test_cube_vertices[] = {
	{-0.5f, -0.5f, -0.5f, 1.0f },
	{ 0.5f, -0.5f, -0.5f, 1.0f },
	{-0.5f,  0.5f, -0.5f, 1.0f },

	{ 0.5f, -0.5f, -0.5f, 1.0f },
	{ 0.5f,  0.5f, -0.5f, 1.0f },
	{-0.5f,  0.5f, -0.5f, 1.0f },

	{-0.5f, -0.5f,  0.5f, 1.0f },
	{ 0.5f, -0.5f,  0.5f, 1.0f },
	{-0.5f,  0.5f,  0.5f, 1.0f },

	{ 0.5f, -0.5f,  0.5f, 1.0f },
	{ 0.5f,  0.5f,  0.5f, 1.0f },
	{-0.5f,  0.5f,  0.5f, 1.0f },

	{-0.5f, -0.5f, -0.5f, 1.0f },
	{-0.5f,  0.5f, -0.5f, 1.0f },
	{-0.5f,  0.5f,  0.5f, 1.0f },

	{-0.5f,  0.5f,  0.5f, 1.0f },
	{-0.5f, -0.5f,  0.5f, 1.0f },
	{-0.5f, -0.5f, -0.5f, 1.0f },

	{ 0.5f, -0.5f, -0.5f, 1.0f },
	{ 0.5f,  0.5f, -0.5f, 1.0f },
	{ 0.5f,  0.5f,  0.5f, 1.0f },

	{ 0.5f,  0.5f,  0.5f, 1.0f },
	{ 0.5f, -0.5f,  0.5f, 1.0f },
	{ 0.5f, -0.5f, -0.5f, 1.0f },

	{ 0.5f, 0.5f,  0.5f, 1.0f },
	{-0.5f, 0.5f,  0.5f, 1.0f },
	{-0.5f, 0.5f, -0.5f, 1.0f },

	{-0.5f, 0.5f, -0.5f, 1.0f },
	{ 0.5f, 0.5f, -0.5f, 1.0f },
	{ 0.5f, 0.5f,  0.5f, 1.0f },

	{ 0.5f, -0.5f,  0.5f, 1.0f },
	{-0.5f, -0.5f,  0.5f, 1.0f },
	{-0.5f, -0.5f, -0.5f, 1.0f },

	{-0.5f, -0.5f, -0.5f, 1.0f },
	{ 0.5f, -0.5f, -0.5f, 1.0f },
	{ 0.5f, -0.5f,  0.5f, 1.0f },
};

constexpr vector4f_t test_triangle_vertices[] = {
	{  0.5f, -0.5f, 0.0f, 1.0f },
	{  0.5f,  0.5f, 0.0f, 1.0f },
	{ -0.5f, -0.5f, 0.0f, 1.0f }
};

constexpr vector2f_t test_triangle_uvs[] = {
	{  0.0f, 0.0f },
	{  0.0f, 1.0f },
	{  1.0f, 0.0f }
};

constexpr vector4f_t test_quad_vertices[] = {
	{  0.5f, -0.5f, 0.0f, 1.0f },
	{  0.5f,  0.5f, 0.0f, 1.0f },
	{ -0.5f,  0.5f, 0.0f, 1.0f },

	{ -0.5f,  0.5f, 0.0f, 1.0f },
	{ -0.5f, -0.5f, 0.0f, 1.0f },
	{  0.5f, -0.5f, 0.0f, 1.0f }
};

constexpr vector2f_t test_quad_uvs[] = {
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f },
	{ 0.0f, 1.0f },

	{ 0.0f, 1.0f },
	{ 0.0f, 0.0f },
	{ 1.0f, 0.0f }
};

uint8_t* createGDIColorBuffer(HDC deviceContext, BITMAPINFO** ppColorBufferBitmapInfo, HBITMAP* pColorBufferBitmap, int width, int height)
{		
	if (*ppColorBufferBitmapInfo != NULL)
	{
		free(*ppColorBufferBitmapInfo);
		*ppColorBufferBitmapInfo = NULL;
	}

	if (*pColorBufferBitmap != NULL)
	{
		DeleteObject(*pColorBufferBitmap);
		*pColorBufferBitmap = NULL;
	}

	*ppColorBufferBitmapInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO));
	(*ppColorBufferBitmapInfo)->bmiHeader.biSize = sizeof(BITMAPINFO);
	(*ppColorBufferBitmapInfo)->bmiHeader.biWidth = width;
	(*ppColorBufferBitmapInfo)->bmiHeader.biHeight = -(int)height;
	(*ppColorBufferBitmapInfo)->bmiHeader.biPlanes = 1;
	(*ppColorBufferBitmapInfo)->bmiHeader.biBitCount = 32;
	(*ppColorBufferBitmapInfo)->bmiHeader.biCompression = BI_RGB;
	//FK: XRGB

	uint8_t* pColorBufferPixels = nullptr;
	*pColorBufferBitmap = CreateDIBSection(deviceContext, *ppColorBufferBitmapInfo, DIB_RGB_COLORS, (void**)&pColorBufferPixels, NULL, 0);   
	if (*pColorBufferBitmap == NULL && pBackBufferPixels == NULL)
	{
		MessageBoxA(0, "Error during CreateDIBSection.", "Error!", 0);
	}

	return pColorBufferPixels;
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
			cameraVelocity.y = 0.001f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.y = 0.0f;
		}
	}

	if(wparam == VK_UP)
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.y = -0.001f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.y = 0.0f;
		}
	}
	
	if(wparam == VK_LEFT)
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.x = 0.0005f;
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
			cameraVelocity.x = -0.0005f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.x = 0.0f;
		}
	}

	if(wparam == VK_NEXT)
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.z = -0.001f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.z = 0.0f;
		}
	}

	if(wparam == VK_PRIOR)
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.z = 0.001f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.z = 0.0f;
		}
	}
}

void K15_MouseButtonInput(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{

}

void K15_MouseMove(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	mouseX = (lparam & 0xFFFF) / 3;
	mouseY = ((lparam & 0xFFFF0000) >> 16) / 3;
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
}

LRESULT CALLBACK K15_WNDPROC(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	bool messageHandled = false;

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

		screenWidth = windowWidth;
		screenHeight = windowHeight;

		HDC deviceContext = GetDC(hwnd);
		pBackBufferPixels = (uint32_t*)createGDIColorBuffer(deviceContext, &pBackBufferBitmapInfo, &backBufferBitmap, virtualScreenWidth, virtualScreenHeight);

		if(pContext != nullptr)
		{
			uint32_t stride = virtualScreenWidth;
			k15_change_color_buffers(pContext, (void**)&pBackBufferPixels, 1u, virtualScreenWidth, virtualScreenHeight, stride);
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

int imageWidth = 0;
int imageHeight = 0;
int imageComponentCount = 0;
uint8_t* pImage = NULL;

vertex_buffer_handle_t vertexBuffer = k15_invalid_vertex_buffer_handle;
texture_handle_t texture = k15_invalid_texture_handle;
uint32_t vertexCount = 0;

bool setup()
{
	software_rasterizer_context_init_parameters_t parameters;
	parameters.backBufferWidth 	= virtualScreenWidth;
	parameters.backBufferHeight = virtualScreenHeight;
	parameters.backBufferStride = virtualScreenWidth;
	parameters.pColorBuffers[0]	= pBackBufferPixels;
	parameters.colorBufferCount = 1;
	
	pImage = stbi_load("texture.png", &imageWidth, &imageHeight, &imageComponentCount, 3u);
	if(pImage == nullptr)
	{
		printf("Can't load \"texture.png\".\n");
		return false;
	}

	if(!k15_create_software_rasterizer_context(&pContext, &parameters))
	{
		printf("Couldn't create software rendering context.\n");
		return false;
	}

#if 0
	const vector4f_t* pGeometry = test_cube_vertices;
	vertexCount = sizeof(test_cube_vertices)/sizeof(vector4f_t);
#elif 0
	const vector4f_t* pGeometry = test_triangle_vertices;
	const vector2f_t* pUVs = test_triangle_uvs;
	vertexCount = sizeof(test_triangle_vertices)/sizeof(vector4f_t);
#else
	const vector4f_t* pGeometry = test_quad_vertices;
	const vector2f_t* pUVs = test_quad_uvs;
	vertexCount = sizeof(test_quad_vertices)/sizeof(vector4f_t);
#endif

	vertex_t* pVertices = (vertex_t*)malloc(sizeof(vertex_t) * vertexCount);
	for( uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex )
	{
		pVertices[vertexIndex].position = pGeometry[vertexIndex];
		pVertices[vertexIndex].normal 	= k15_create_vector3f(1.0f, 0.0f, 0.0f);
		pVertices[vertexIndex].texcoord = pUVs[vertexIndex];
	}

	vertexBuffer = k15_create_vertex_buffer( pContext, pVertices, vertexCount );
	if(!k15_is_valid_vertex_buffer(vertexBuffer))
	{
		return false;
	}

	texture = k15_create_texture( pContext, imageWidth, imageHeight, imageWidth, imageComponentCount, pImage );
	if(!k15_is_valid_texture(texture))
	{
		return false;
	}

	return true;
}

void drawBackBuffer(HDC deviceContext)
{
	StretchDIBits(deviceContext, 0, 0, screenWidth, screenHeight, 0, 0, virtualScreenWidth, virtualScreenHeight, 
		pBackBufferPixels, pBackBufferBitmapInfo, DIB_RGB_COLORS, SRCCOPY);  

	memset(pBackBufferPixels, 0u, virtualScreenWidth * virtualScreenHeight * sizeof(uint32_t));
}

void doFrame(uint32_t DeltaTimeInMS)
{
	static float angle = 0.5f;
	angle += 0.001f;

	cameraPos = _k15_vector3f_add(cameraVelocity, cameraPos);

	matrix4x4f_t rotation = {cosf(angle), 0.0f, sinf(angle), 0.0f,
							 0.0f, 1.0f, 0.0f, 0.0f,
							 -sinf(angle), 0.0f, cosf(angle), 0.0f,
							 0.0f, 0.0f, 0.0f, 1.0f};

	matrix4x4f_t viewMatrix = {1.0f, 0.0f, 0.0f, -cameraPos.x,
							   0.0f, 1.0f, 0.0f, -cameraPos.y,
							   0.0f, 0.0f, 1.0f, -cameraPos.z,
							   0.0f, 0.0f, 0.0f, 1.0f};

	k15_bind_vertex_buffer(pContext, vertexBuffer);
	k15_bind_texture(pContext, texture, 0);
	k15_bind_view_matrix(pContext, &viewMatrix);
	k15_draw(pContext, vertexCount, 0u);

	k15_draw_frame(pContext);
	k15_swap_color_buffers(pContext);
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

	bool loopRunning = true;
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

		HDC deviceContext = GetDC(hwnd);
		drawBackBuffer(deviceContext);

		timeFrameEnded = getTimeInMilliseconds(performanceFrequency);
		deltaMs = timeFrameEnded - timeFrameStarted;
	}

	DestroyWindow(hwnd);

	return 0;
}