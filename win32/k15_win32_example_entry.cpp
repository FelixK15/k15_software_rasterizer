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
float* pDepthBufferPixels = 0;
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

struct loaded_model_t
{
	vertex_buffer_handle_t 	vertexBuffers[6];
	texture_handle_t 		textures[6];
	uint32_t 				vertexCounts[6];
	uint32_t 				subModelCount;
};

struct Win32FileMapping
{
	uint8_t* pFileBaseAddress		= nullptr;
	HANDLE	 pFileMappingHandle		= nullptr;
	HANDLE   pFileHandle			= nullptr;
	uint32_t fileSizeInBytes		= 0u;
};

void unmapFileMapping( Win32FileMapping* pFileMapping )
{
	if( pFileMapping->pFileBaseAddress != nullptr )
	{
		UnmapViewOfFile( pFileMapping->pFileBaseAddress );
		pFileMapping->pFileBaseAddress = nullptr;
	}

	if( pFileMapping->pFileMappingHandle != nullptr )
	{
		CloseHandle( pFileMapping->pFileMappingHandle );
		pFileMapping->pFileMappingHandle = nullptr;
	}

	if( pFileMapping->pFileHandle != nullptr )
	{
		CloseHandle( pFileMapping->pFileHandle );
		pFileMapping->pFileHandle = nullptr;
	}
}

bool mapFileForReading( Win32FileMapping* pOutFileMapping, const char* pFileName )
{
	const HANDLE pFileHandle = CreateFileA( pFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0u, nullptr );
	if( pFileHandle== INVALID_HANDLE_VALUE )
	{
		const DWORD lastError = GetLastError();
		printf("Could not open file handle to '%s'. CreateFileA() error = %lu\n", pFileName, lastError );
		return 0;
	}

	const HANDLE pFileMappingHandle = CreateFileMapping( pFileHandle, nullptr, PAGE_READONLY, 0u, 0u, nullptr );
	if( pFileMappingHandle == INVALID_HANDLE_VALUE )
	{
		const DWORD lastError = GetLastError();
		printf("Could not create file mapping handle for file '%s'. CreateFileMapping() error = %lu.\n", pFileName, lastError );

		CloseHandle( pFileHandle );
		return 0;
	}

	uint8_t* pFileBaseAddress = ( uint8_t* )MapViewOfFile( pFileMappingHandle, FILE_MAP_READ, 0u, 0u, 0u );
	if( pFileBaseAddress == nullptr )
	{
		const DWORD lastError = GetLastError();
		printf("Could not create map view of file '%s'. MapViewOfFile() error = %lu.\n", pFileName, lastError );

		CloseHandle( pFileHandle );
		CloseHandle( pFileMappingHandle );
		return 0;
	}

	pOutFileMapping->pFileHandle 		= pFileHandle;
	pOutFileMapping->pFileMappingHandle = pFileMappingHandle;
	pOutFileMapping->pFileBaseAddress	= pFileBaseAddress;
	pOutFileMapping->fileSizeInBytes 	= ( uint32_t )GetFileSize( pFileHandle, nullptr );

	return 1;
}

void readCountsFromWavefrontModelFile(const char* pModelFileContentStart, const char* pModelFileContentEnd, uint32_t* pOutVertexCount, uint32_t* pOutNormalCount, uint32_t* pOutTexCoordCount, uint32_t* pOutTriangleCount)
{
	uint32_t triangleCount = 0;
	uint32_t vertexCount = 0;
	uint32_t normalCount = 0;
	uint32_t texcoordCount = 0;
	bool waitUntilLineEnd = false;
	const char* pModelFileContentRunning = pModelFileContentStart;
	
	while( pModelFileContentRunning != pModelFileContentEnd )
	{
		if( waitUntilLineEnd && *pModelFileContentRunning == '\n' )
		{
			waitUntilLineEnd = false;
		}
		else
		{
			if( pModelFileContentRunning[0] == 'f' && pModelFileContentRunning[1] == ' ' )
			{
				++triangleCount;
				waitUntilLineEnd = true;
			}
			else if( pModelFileContentRunning[0] == 'v' && pModelFileContentRunning[1] == 't' )
			{
				++texcoordCount;
			}
			else if( pModelFileContentRunning[0] == 'v' && pModelFileContentRunning[1] == 'n' )
			{
				++normalCount;	
			}
			else if( pModelFileContentRunning[0] == 'v' && pModelFileContentRunning[1] == ' ')
			{
				++vertexCount;
			}
		}
		
		++pModelFileContentRunning;
	}

	*pOutVertexCount = vertexCount;
	*pOutNormalCount = normalCount;
	*pOutTexCoordCount = texcoordCount;
	*pOutTriangleCount = triangleCount;
}

void fillVertexArrays(const char* pFileStart, const char* pFileEnd, vector4f_t* restrict_modifier pPositions, vector3f_t* restrict_modifier pNormals, vector2f_t* restrict_modifier pTexCoords, float scaleFactor)
{
	const char* pFileRunning = pFileStart;
	bool seekEndOfLine = false;
	uint32_t positionIndex = 0;
	uint32_t normalIndex = 0;
	uint32_t texCoordIndex = 0;

	while( pFileRunning != pFileEnd )
	{
		if( seekEndOfLine  )
		{
			if( *pFileRunning == '\n')
			{
				seekEndOfLine = false;
			}

			++pFileRunning;
		}
		if( pFileRunning[0] == 'v' && pFileRunning[1] == ' ' )
		{
			sscanf(pFileRunning + 3, "%f %f %f\n", &pPositions[positionIndex].x, &pPositions[positionIndex].y, &pPositions[positionIndex].z );
			pPositions[positionIndex] = _k15_vector4f_scale(pPositions[positionIndex], scaleFactor);
			pPositions[positionIndex].w = 1.0f;

			++positionIndex;

			seekEndOfLine = true;
			continue;
		}
		else if( pFileRunning[0] == 'v' && pFileRunning[1] == 'n' )
		{
			sscanf(pFileRunning + 3, "%f %f %f\n", &pNormals[normalIndex].x, &pNormals[normalIndex].y, &pNormals[normalIndex].z );
			++normalIndex;

			seekEndOfLine = true;
			continue;
		}
		else if( pFileRunning[0] == 'v' && pFileRunning[1] == 't' )
		{
			sscanf(pFileRunning + 3, "%f %f 0.0000\n", &pTexCoords[texCoordIndex].x, &pTexCoords[texCoordIndex].y );
			++texCoordIndex;

			seekEndOfLine = true;
			continue;
		}
		else
		{
			seekEndOfLine = true;
		}
	}
}

bool areStringsEqual( const char* restrict_modifier pStringA, const char* restrict_modifier pStringB )
{
	while( true )
	{
		if( *pStringA == 0 || *pStringB == 0)
		{
			return true;
		}

		if( *pStringA != *pStringB )
		{
			return false;
		}

		++pStringA;
		++pStringB;
	}

	return true;
}

bool assembleVerticesForMaterial(const char* pMaterialName, uint32_t* pOutVertexCountForThisMaterial, vertex_t* restrict_modifier pVertices, const char* pFileStart, const char* pFileEnd, uint32_t totalVertexCount, uint32_t positionCount, uint32_t normalCount, uint32_t texCoordCount, float scaleFactor)
{
	vector4f_t* pPositions = (vector4f_t*)malloc(positionCount * sizeof(vector4f_t));
	vector3f_t* pNormals   = (vector3f_t*)malloc(normalCount * sizeof(vector3f_t));
	vector2f_t* pTexCoords = (vector2f_t*)malloc(texCoordCount * sizeof(vector2f_t));

	if( pPositions == nullptr || pNormals == nullptr || pTexCoords == nullptr )
	{
		return false;
	}

	fillVertexArrays(pFileStart, pFileEnd, pPositions, pNormals, pTexCoords, scaleFactor);
	
	uint32_t vertexIndex = 0;
	const char* pFileRunning = pFileStart;
	bool seekLineEnd = false;
	bool matchingMaterial = false;
	while( pFileRunning != pFileEnd )
	{
		if( seekLineEnd )
		{
			if( *pFileRunning == '\n' )
			{
				seekLineEnd = false;
			}

			++pFileRunning;
		}
		else if( matchingMaterial && pFileRunning[0] == 'f' && pFileRunning[1] == ' ' )
		{
			uint32_t positionIndex[3], normalIndex[3], texCoordIndex[3];
			sscanf(pFileRunning + 2, "%u/%u/%u %u/%u/%u %u/%u/%u\n", 
				positionIndex + 0, texCoordIndex + 0, normalIndex + 0,
				positionIndex + 1, texCoordIndex + 1, normalIndex + 1,
				positionIndex + 2, texCoordIndex + 2, normalIndex + 2);

			seekLineEnd = true;

			pVertices[vertexIndex + 0].position = pPositions[positionIndex[0] - 1u];
			pVertices[vertexIndex + 1].position = pPositions[positionIndex[1] - 1u];
			pVertices[vertexIndex + 2].position = pPositions[positionIndex[2] - 1u];

			pVertices[vertexIndex + 0].normal = pNormals[normalIndex[0] - 1u];
			pVertices[vertexIndex + 1].normal = pNormals[normalIndex[1] - 1u];
			pVertices[vertexIndex + 2].normal = pNormals[normalIndex[2] - 1u];

			pVertices[vertexIndex + 0].texcoord = pTexCoords[texCoordIndex[0] - 1u];
			pVertices[vertexIndex + 1].texcoord = pTexCoords[texCoordIndex[1] - 1u];
			pVertices[vertexIndex + 2].texcoord = pTexCoords[texCoordIndex[2] - 1u];
			vertexIndex += 3u;
		}
		else if( areStringsEqual( pFileRunning, "usemtl" ) )
		{
			pFileRunning += 7u;
			matchingMaterial = areStringsEqual(pFileRunning, pMaterialName);
			seekLineEnd = true;
		}
		else
		{
			seekLineEnd = true;
		}
	}
	
	free(pPositions);
	free(pNormals);
	free(pTexCoords);

	*pOutVertexCountForThisMaterial = vertexIndex;

	return true;
}

struct wavefront_material_t
{
	char materialName[64];
	char materialTexturePath[64];
};

vertex_buffer_handle_t extractVerticesForMaterialFromWavefrontModel(software_rasterizer_context_t* restrict_modifier pContext, const char* restrict_modifier pModelFilePath, wavefront_material_t* restrict_modifier pMaterial, uint32_t* restrict_modifier pOutVertexCount, float scaleFactor)
{
	Win32FileMapping modelFileMapping;
	if(!mapFileForReading(&modelFileMapping, pModelFilePath))
	{
		return k15_invalid_vertex_buffer_handle;
	}

	const char* pFileStart 	= (const char*)modelFileMapping.pFileBaseAddress;
	const char* pFileEnd 	= (const char*)modelFileMapping.pFileBaseAddress + modelFileMapping.fileSizeInBytes;

	uint32_t vertexCount = 0;
	uint32_t normalCount = 0;
	uint32_t texCoordCount = 0;
	uint32_t triangleCount = 0;
	readCountsFromWavefrontModelFile(pFileStart, pFileEnd, &vertexCount, &normalCount, &texCoordCount, &triangleCount);

	const uint32_t totalVertexCount = triangleCount * 3;

	vertex_t* pVertices = (vertex_t*)malloc(totalVertexCount * sizeof(vertex_t));
	if( pVertices == nullptr )
	{
		unmapFileMapping(&modelFileMapping);
		return k15_invalid_vertex_buffer_handle;
	}

	uint32_t vertexCountForThisMaterial = 0;
	assembleVerticesForMaterial(pMaterial->materialName, &vertexCountForThisMaterial, pVertices, pFileStart, pFileEnd, totalVertexCount, vertexCount, normalCount, texCoordCount, scaleFactor);
	unmapFileMapping(&modelFileMapping);

	*pOutVertexCount = vertexCountForThisMaterial;

	return k15_create_vertex_buffer(pContext, pVertices, vertexCountForThisMaterial);
}

uint32_t getPositionOfNextNewLine(const char* pLine)
{
	const char* pLineStart = pLine;
	while( *pLine != '\n' && *pLine != '\r' )
	{
		++pLine;
	}

	return (uint32_t)(pLine - pLineStart);
}

bool extractMaterialsFromWavefrontMaterial(const char* restrict_modifier pMaterialFilePath, wavefront_material_t* restrict_modifier pOutMaterials, uint32_t* restrict_modifier pOutMaterialCount)
{
	Win32FileMapping materialFileMapping;
	if(!mapFileForReading(&materialFileMapping, pMaterialFilePath))
	{
		return false;
	}

	const char* pFileStart = (const char*)materialFileMapping.pFileBaseAddress;
	const char* pFileEnd = pFileStart + materialFileMapping.fileSizeInBytes;
	const char* pFileRunning = pFileStart;

	bool seekToEndOfLine = false;
	bool insideMaterial = false;
	uint32_t materialIndex = 0;
	while( pFileRunning != pFileEnd )
	{
		if( seekToEndOfLine )
		{
			if( *pFileRunning == '\n' )
			{
				seekToEndOfLine = false;
			}

			++pFileRunning;
		}
		else if( insideMaterial && areStringsEqual(pFileRunning, "\tmap_Ka") )
		{
			pFileRunning += 8;

			memcpy(pOutMaterials[materialIndex].materialTexturePath, pFileRunning, getPositionOfNextNewLine(pFileRunning));
			++materialIndex;
			insideMaterial = false;
			seekToEndOfLine = true;
		}
		else if( areStringsEqual(pFileRunning, "newmtl") )
		{
			pFileRunning += 7;

			memcpy(pOutMaterials[materialIndex].materialName, pFileRunning, getPositionOfNextNewLine(pFileRunning));
			insideMaterial = true;
			seekToEndOfLine = true;
		}
		else
		{
			seekToEndOfLine = true;
		}
	}

	unmapFileMapping(&materialFileMapping);
	*pOutMaterialCount = materialIndex;
	return true;
}

bool loadModel(software_rasterizer_context_t* pContext, loaded_model_t* pOutModel, const char* pModelFileName, const char* pMaterialFileName, float scaleFactor)
{
	loaded_model_t model = {};

	char modelPath[512], materialPath[512];
	sprintf(modelPath, "test_models/%s", pModelFileName);
	sprintf(materialPath, "test_models/%s", pMaterialFileName);
	
	wavefront_material_t materials[6] = {};
	uint32_t materialCount = 0;
	extractMaterialsFromWavefrontMaterial(materialPath, materials, &materialCount);

	char texturePath[512];
	for( uint32_t materialIndex = 0; materialIndex < materialCount; ++materialIndex )
	{
		model.vertexBuffers[materialIndex] = extractVerticesForMaterialFromWavefrontModel(pContext, modelPath, materials + materialIndex, &model.vertexCounts[materialIndex], scaleFactor);

		sprintf(texturePath, "test_models/%s", materials[materialIndex].materialTexturePath);

		int textureWidth = 0;
		int textureHeight = 0;
		int textureComponents = 0;
		const uint8_t* pImageData = stbi_load(texturePath, &textureWidth, &textureHeight, &textureComponents, 3);
		RuntimeAssert(pImageData != nullptr);

		model.textures[materialIndex] = k15_create_texture(pContext, materials[materialIndex].materialName, textureWidth, textureHeight, textureWidth, textureComponents, pImageData);
		++model.subModelCount;
	}
	
	*pOutModel = model;
	return true;
}

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
	if(message == WM_LBUTTONDOWN)
	{
		uint16_t localMouseX = (lparam & 0xFFFF) / 3;
		uint16_t localMouseY = ((lparam & 0xFFFF0000) >> 16) / 3;
		printf("x:%u, y:%u\n", localMouseX, localMouseY);
	}
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

loaded_model_t loadedModel = {};
bool setup()
{
	pDepthBufferPixels = (float*)malloc(virtualScreenWidth * virtualScreenHeight * sizeof(float));
	memset(pDepthBufferPixels, 0, virtualScreenWidth * virtualScreenHeight * sizeof(float));

	software_rasterizer_context_init_parameters_t parameters;
	parameters.backBufferWidth 	= virtualScreenWidth;
	parameters.backBufferHeight = virtualScreenHeight;
	parameters.backBufferStride = virtualScreenWidth;
	parameters.pColorBuffers[0]	= pBackBufferPixels;
	parameters.pDepthBuffers[0] = pDepthBufferPixels;
	parameters.colorBufferCount = 1;
	
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
#elif 0
	const vector4f_t* pGeometry = test_quad_vertices;
	const vector2f_t* pUVs = test_quad_uvs;
	vertexCount = sizeof(test_quad_vertices)/sizeof(vector4f_t);
#endif

#if 0
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
#endif

	return loadModel(pContext, &loadedModel, "crashbandicoot.obj", "crashbandicoot.mtl", 0.005f);
}

void drawBackBuffer(HDC deviceContext)
{
	StretchDIBits(deviceContext, 0, 0, screenWidth, screenHeight, 0, 0, virtualScreenWidth, virtualScreenHeight, 
		pBackBufferPixels, pBackBufferBitmapInfo, DIB_RGB_COLORS, SRCCOPY);  

	memset(pBackBufferPixels, 0u, virtualScreenWidth * virtualScreenHeight * sizeof(uint32_t));
}

void doFrame(float deltaTimeInMs)
{
	static float angle = 0.5f;
	angle += 0.001f;

	cameraPos = _k15_vector3f_add(cameraVelocity, cameraPos);

	matrix4x4f_t rotation = {cosf(angle), 	0.0f, 	sinf(angle), 	0.0f,
							 0.0f, 			-1.0f, 	0.0f, 			0.6f,
							 -sinf(angle), 	0.0f, 	cosf(angle), 	0.0f,
							 0.0f, 			0.0f, 	0.0f, 			1.0f};

	matrix4x4f_t viewMatrix = {1.0f, 0.0f, 0.0f, -cameraPos.x,
							   0.0f, 1.0f, 0.0f, -cameraPos.y,
							   0.0f, 0.0f, 1.0f, -cameraPos.z,
							   0.0f, 0.0f, 0.0f, 1.0f};

	k15_bind_view_matrix(pContext, &viewMatrix);
	k15_bind_model_matrix(pContext, &rotation);
	
	for( uint32_t subModelIndex = 0; subModelIndex < loadedModel.subModelCount; ++subModelIndex )
	{
		k15_bind_vertex_buffer(pContext, loadedModel.vertexBuffers[subModelIndex]);
		k15_bind_texture(pContext, loadedModel.textures[subModelIndex], 0u);
		k15_draw(pContext, loadedModel.vertexCounts[subModelIndex], 0u);
	}

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
	float deltaMs = 0;

	bool loopRunning = true;
	MSG msg = {0};

	char windowTitle[256] = {};

	LARGE_INTEGER start, end, frequency;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);

	while (loopRunning)
	{
		QueryPerformanceCounter(&start);
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

		QueryPerformanceCounter(&end);
		deltaMs = (float)(((end.QuadPart-start.QuadPart)*1000000)/frequency.QuadPart) / 1000.f;

		sprintf(windowTitle, "Software Renderer - %.3f ms", deltaMs);
		SetWindowText(hwnd, windowTitle);
	}

	DestroyWindow(hwnd);

	return 0;
}