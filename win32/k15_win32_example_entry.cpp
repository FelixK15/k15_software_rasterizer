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

struct light_t
{
	vector4f_t position;
	vector4f_t color;
	float radius;
};

struct shader_uniform_data_t
{
	light_t lights[16];
	uint8_t lightCount;

	vector4f_t viewPos;
	vector4f_t viewDir;
	vector4f_t ambientColor;
	texture_handle_t texture;
	texture_handle_t normalMap;
	matrix4x4f_t viewProjMatrix;
	matrix4x4f_t modelMatrix;
};

software_rasterizer_context_t* pContext = nullptr;
uint32_t* pBackBufferPixels = 0;
float* pDepthBufferPixels = 0;
BITMAPINFO* pBackBufferBitmapInfo = 0;
HBITMAP backBufferBitmap = 0;

int virtualScreenWidth = 1920;
int virtualScreenHeight = 1080;
int screenScaleFactor = 1;
int screenWidth = virtualScreenWidth*screenScaleFactor;
int screenHeight = virtualScreenHeight*screenScaleFactor;

bool appHasFocus = true;
bool drawDepthBuffer = false;
bool drawWireframe = false;

vector4f_t cameraPos = {0.0f, 0.0f, 2.5f, 1.0f};
vector4f_t cameraVelocity = {};
vector2f_t cameraAngles = {};

matrix4x4f_t projectionMatrix;
shader_uniform_data_t shaderData;

vertex_shader_handle_t vertexShaderHandle;
pixel_shader_handle_t pixelShaderHandle;
uniform_buffer_handle_t uniformBufferHandle;

constexpr vertex_t test_triangle_vertices[] = {
	{ k15_create_vector4f(  0.5f, -0.5f, 0.0f, 1.0f ), k15_create_vector4f( 0.0f, 0.0f, 1.0f, 0.0f), k15_create_vector4f( 1.0f, 0.0f, 1.0f, 0.0f), k15_create_vector2f( 1.0f, 0.0f ) },
	{ k15_create_vector4f(  0.5f,  0.5f, 0.0f, 1.0f ), k15_create_vector4f( 0.0f, 0.0f, 1.0f, 0.0f), k15_create_vector4f( 1.0f, 0.0f, 1.0f, 0.0f), k15_create_vector2f( 1.0f, 1.0f ) },
	{ k15_create_vector4f( -0.5f, -0.5f, 0.0f, 1.0f ), k15_create_vector4f( 0.0f, 0.0f, 1.0f, 0.0f), k15_create_vector4f( 1.0f, 0.0f, 1.0f, 0.0f), k15_create_vector2f( 0.0f, 1.0f ) }
};

constexpr vertex_t test_quad_vertices[] = {
	{ k15_create_vector4f(  0.5f, -0.5f, 0.0f, 1.0f ), k15_create_vector4f( 0.0f, 0.0f, 1.0f, 0.0f), k15_create_vector4f( 1.0f, 0.0f, 1.0f, 0.0f), k15_create_vector2f( 1.0f, 0.0f ) },
	{ k15_create_vector4f(  0.5f,  0.5f, 0.0f, 1.0f ), k15_create_vector4f( 0.0f, 0.0f, 1.0f, 0.0f), k15_create_vector4f( 1.0f, 0.0f, 1.0f, 0.0f), k15_create_vector2f( 1.0f, 1.0f ) },
	{ k15_create_vector4f( -0.5f,  0.5f, 0.0f, 1.0f ), k15_create_vector4f( 0.0f, 0.0f, 1.0f, 0.0f), k15_create_vector4f( 1.0f, 0.0f, 1.0f, 0.0f), k15_create_vector2f( 0.0f, 1.0f ) },

	{ k15_create_vector4f( -0.5f,  0.5f, 0.0f, 1.0f ), k15_create_vector4f( 0.0f, 0.0f, 1.0f, 0.0f), k15_create_vector4f( 1.0f, 0.0f, 1.0f, 0.0f), k15_create_vector2f( 0.0f, 1.0f ) },
	{ k15_create_vector4f( -0.5f, -0.5f, 0.0f, 1.0f ), k15_create_vector4f( 0.0f, 0.0f, 1.0f, 0.0f), k15_create_vector4f( 1.0f, 0.0f, 1.0f, 0.0f), k15_create_vector2f( 0.0f, 0.0f ) },
	{ k15_create_vector4f(  0.5f, -0.5f, 0.0f, 1.0f ), k15_create_vector4f( 0.0f, 0.0f, 1.0f, 0.0f), k15_create_vector4f( 1.0f, 0.0f, 1.0f, 0.0f), k15_create_vector2f( 1.0f, 0.0f ) }
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

bool fillVertexArrays(const char* pFileStart, const char* pFileEnd, dynamic_buffer_t<vector4f_t>* restrict_modifier pPositions, dynamic_buffer_t<vector4f_t>* restrict_modifier pNormals, dynamic_buffer_t<vector2f_t>* restrict_modifier pTexCoords, float scaleFactor)
{
	const char* pFileRunning = pFileStart;
	bool seekEndOfLine = false;

	while( pFileRunning != pFileEnd )
	{
		if( seekEndOfLine  )
		{
			if( *pFileRunning == '\n')
			{
				seekEndOfLine = false;
			}

			++pFileRunning;
			continue;
		}
		if( pFileRunning[0] == 'v' && pFileRunning[1] == ' ' )
		{
			vector4f_t position = {0.0f, 0.0f, 0.0f, 0.0f};
			sscanf(pFileRunning + 2, "%f %f %f\n", &position.x, &position.y, &position.z );
			
			position = k15_vector4f_scale(position, scaleFactor);
			position.w = 1.0f;
			if( _k15_dynamic_buffer_push_back(pPositions, position) == nullptr )
			{
				return false;
			}

			seekEndOfLine = true;
			continue;
		}
		else if( pFileRunning[0] == 'v' && pFileRunning[1] == 'n' )
		{
			vector4f_t normal = {0.0f, 0.0f, 0.0f, 0.0f};
			sscanf(pFileRunning + 2, "%f %f %f\n", &normal.x, &normal.y, &normal.z );

			if( _k15_dynamic_buffer_push_back(pNormals, normal) == nullptr )
			{
				return false;
			}

			seekEndOfLine = true;
			continue;
		}
		else if( pFileRunning[0] == 'v' && pFileRunning[1] == 't' )
		{
			vector2f_t texcoord = {0.0f, 0.0f};
			sscanf(pFileRunning + 2, "%f %f 0.0000\n", &texcoord.x, &texcoord.y );
			
			if( _k15_dynamic_buffer_push_back(pTexCoords, texcoord) == nullptr )
			{
				return false;
			}

			seekEndOfLine = true;
			continue;
		}
		else
		{
			seekEndOfLine = true;
		}
	}

	return true;
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

bool assembleVerticesForMaterial(const char* pMaterialName, uint32_t* pOutVertexCountForThisMaterial, dynamic_buffer_t<vertex_t>* restrict_modifier pVertices, const char* pFileStart, const char* pFileEnd, float scaleFactor)
{
	dynamic_buffer_t<vector4f_t> positions = {};
	dynamic_buffer_t<vector4f_t> normals = {};
	dynamic_buffer_t<vector2f_t> texcoords = {};

	if( !_k15_create_dynamic_buffer(&positions, 128u) ||
		!_k15_create_dynamic_buffer(&normals, 128u) ||
		!_k15_create_dynamic_buffer(&texcoords, 128u) )
	{
		return false;
	}

	fillVertexArrays(pFileStart, pFileEnd, &positions, &normals, &texcoords, scaleFactor);
	
	uint32_t vertexIndex = 0;
	const char* pFileRunning = pFileStart;
	bool seekLineEnd = false;
	bool matchingMaterial = pMaterialName == nullptr ? true : false;
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

			vertex_t vertices[3] = {};
			vertices[0].position = positions.pData[positionIndex[0] - 1u];
			vertices[1].position = positions.pData[positionIndex[1] - 1u];
			vertices[2].position = positions.pData[positionIndex[2] - 1u];

			vertices[0].normal = normals.pData[normalIndex[0] - 1u];
			vertices[1].normal = normals.pData[normalIndex[1] - 1u];
			vertices[2].normal = normals.pData[normalIndex[2] - 1u];

			vertices[0].texcoord = texcoords.pData[texCoordIndex[0] - 1u];
			vertices[1].texcoord = texcoords.pData[texCoordIndex[1] - 1u];
			vertices[2].texcoord = texcoords.pData[texCoordIndex[2] - 1u];

			if( _k15_dynamic_buffer_push_back(pVertices, vertices, 3u) == nullptr )
			{
				return false;
			}

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
	
	_k15_destroy_dynamic_buffer(&positions);
	_k15_destroy_dynamic_buffer(&normals);
	_k15_destroy_dynamic_buffer(&texcoords);

	*pOutVertexCountForThisMaterial = vertexIndex;

	return true;
}

struct wavefront_material_t
{
	char materialName[64];
	char materialTexturePath[64];
};

struct scn_material_t
{
	char baseColorMapPath[128];
	char normalMapPath[128];
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

	dynamic_buffer_t<vertex_t> vertices = {};
	if( !_k15_create_dynamic_buffer( &vertices, 128u ) )
	{
		unmapFileMapping(&modelFileMapping);
		return k15_invalid_vertex_buffer_handle;
	}
	
	uint32_t vertexCountForThisMaterial = 0;
	if(!assembleVerticesForMaterial(pMaterial == nullptr ? nullptr : pMaterial->materialName, &vertexCountForThisMaterial, &vertices, pFileStart, pFileEnd, scaleFactor))
	{
		unmapFileMapping(&modelFileMapping);
		return k15_invalid_vertex_buffer_handle;
	}

	unmapFileMapping(&modelFileMapping);
	*pOutVertexCount = vertexCountForThisMaterial;

	return k15_create_vertex_buffer(pContext, vertices.pData, vertices.count);
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

bool extractMaterialAndModelPathFromScnFile(const char* pScnPath, scn_material_t* pOutMaterial, char* pModelPath)
{
	Win32FileMapping scnFileMapping;
	if(!mapFileForReading(&scnFileMapping, pScnPath))
	{
		return false;
	}

	const char* pScnFileStart 		= (const char*)scnFileMapping.pFileBaseAddress;
	const char* pScnFileEnd 		= pScnFileStart + scnFileMapping.fileSizeInBytes;
	const char* pScnFileRunning 	= pScnFileStart;

	bool readMaterial = false;
	bool readModel = false;
	bool seekUntilEndOfLine = false;
	while( pScnFileRunning != pScnFileEnd )
	{
		if(seekUntilEndOfLine)
		{
			if(*pScnFileRunning == '\n')
			{
				seekUntilEndOfLine = false;
			}

			++pScnFileRunning;
			continue;
		}

		if(!readMaterial && !readModel)
		{
			if(areStringsEqual(pScnFileRunning, "materials"))
			{
				readMaterial = true;
			}
			else if(areStringsEqual(pScnFileRunning, "models"))
			{
				readModel = true;
			}
		}
		else if(readMaterial)
		{
			if(areStringsEqual(pScnFileRunning, "        alpha_cutoff"))
			{
				readMaterial = false;
			}
			else if(areStringsEqual(pScnFileRunning, "        basecolor_map"))
			{
				sscanf(pScnFileRunning, "        basecolor_map: %s", pOutMaterial->baseColorMapPath);
			}
			else if(areStringsEqual(pScnFileRunning, "        normal_map"))
			{
				sscanf(pScnFileRunning, "        normal_map: %s", pOutMaterial->normalMapPath);
			}
		}
		else if(readModel)
		{
			if(areStringsEqual(pScnFileRunning, "        transform"))
			{
				readModel = false;
			}
			else if(areStringsEqual(pScnFileRunning, "        mesh"))
			{
				sscanf(pScnFileRunning, "        mesh: %s", pModelPath);
			}
		}

		seekUntilEndOfLine = true;
	}

	unmapFileMapping(&scnFileMapping);
	return true;
}

bool loadScnModel(software_rasterizer_context_t* pContext, loaded_model_t* pOutModel, const char* pScnFileName, float scaleFactor)
{
	char scnPath[512];
	sprintf(scnPath, "test_models/%s", pScnFileName);

	char modelPath[512];
	scn_material_t material;
	if(!extractMaterialAndModelPathFromScnFile(scnPath, &material, modelPath))
	{
		return false;
	}

	char baseMapPath[512], normalMapPath[512];
	sprintf(baseMapPath, "test_models/%s", material.baseColorMapPath);
	sprintf(normalMapPath, "test_models/%s", material.normalMapPath);

	int textureWidth, textureHeight, textureComponents;
	const uint8_t* pBaseMapData = stbi_load(baseMapPath, &textureWidth, &textureHeight, &textureComponents, 0);
	if( pBaseMapData == nullptr )
	{
		return false;
	}
	pOutModel->textures[0] = k15_create_texture(pContext, "baseColorMap", textureWidth, textureHeight, textureWidth, textureComponents, pBaseMapData);

	const uint8_t* pNormalMapData = stbi_load(normalMapPath, &textureWidth, &textureHeight, &textureComponents, 0);
	if( pNormalMapData == nullptr )
	{
		return false;
	}
	pOutModel->textures[1] = k15_create_texture(pContext, "normalMap", textureWidth, textureHeight, textureWidth, textureComponents, pNormalMapData);

	char correctModelPath[512];
	sprintf(correctModelPath, "test_models/%s", modelPath);

	pOutModel->vertexBuffers[0] = extractVerticesForMaterialFromWavefrontModel(pContext, correctModelPath, nullptr, &pOutModel->vertexCounts[0], scaleFactor);
	pOutModel->subModelCount = 1u;
	return true;
}

bool loadObjModel(software_rasterizer_context_t* pContext, loaded_model_t* pOutModel, const char* pModelFileName, const char* pMaterialFileName, float scaleFactor)
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

bool loadTriangle(software_rasterizer_context_t* pContext, loaded_model_t* pOutModel)
{
	const char* pTriangleTexturePath = "test_models/back.png";
	int textureWidth = 0;
	int textureHeight = 0;
	int textureComponents = 0;
	const uint8_t* pImageData = stbi_load(pTriangleTexturePath, &textureWidth, &textureHeight, &textureComponents, 3);
	if( pImageData == nullptr )
	{
		return false;
	}

	pOutModel->subModelCount 	= 1u;
	pOutModel->vertexBuffers[0] = k15_create_vertex_buffer(pContext, test_triangle_vertices, 3u);
	pOutModel->textures[0] 		= k15_create_texture(pContext, "triangle_test", textureWidth, textureHeight, textureWidth, textureComponents, pImageData);
	pOutModel->vertexCounts[0] 	= 3u;

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

	if(wparam == 'S')
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.z = 1.0f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.z = 0.0f;
		}
	}

	if(wparam == 'W')
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.z = -1.0f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.z = 0.0f;
		}
	}
	
	if(wparam == 'D')
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.x = 1.0f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.x = 0.0f;
		}
	}

	if(wparam == 'A')
	{
		if(isKeyDown && firstKeyDown)
		{
			cameraVelocity.x = -1.0f;
		}
		else if(isKeyUp)
		{
			cameraVelocity.x = 0.0f;
		}
	}

	if(wparam == VK_F1)
	{
		printf("camera angles: {%.3f, %.3f} pos: {%.3f, %.3f, %.3f}\n", cameraAngles.x, cameraAngles.y, cameraPos.x, cameraPos.y, cameraPos.z);
	}
	
	if(wparam == VK_F2)
	{
		cameraAngles.x = -3.077f;
		cameraAngles.y = -0.111f;
		
		cameraPos.x = 0.025f;
		cameraPos.y = -0.559f;
		cameraPos.z = -0.332f;
	}

	if(wparam == VK_F3 && firstKeyDown)
	{
		drawDepthBuffer = !drawDepthBuffer;
	}

	if(wparam == VK_F4 && firstKeyDown)
	{
		drawWireframe = !drawWireframe;
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
	#if 0
	static int mouseX = 0;
	static int mouseY = 0;

	const int newMouseX = (lparam & 0xFFFF) / screenScaleFactor;
	const int newMouseY = ((lparam & 0xFFFF0000) >> 16) / screenScaleFactor;

	const int deltaX = newMouseX - mouseX;
	const int deltaY = newMouseY - mouseY;

	cameraAngles.x = (float)deltaX / 1000.f;
	cameraAngles.y = (float)deltaY / 1000.f;

	mouseX = newMouseX;
	mouseY = newMouseY;
	#endif
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

	case WM_ACTIVATE:
		if(wparam == WA_ACTIVE || wparam == WA_CLICKACTIVE)
		{
			appHasFocus = true;
		}
		else if(wparam == WA_INACTIVE)
		{
			appHasFocus = false;
		}
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

void vertexShader(vertex_shader_input_t* pInOutVertices, uint32_t vertexCount, const void* pUniformData)
{
	shader_uniform_data_t* pShaderData = (shader_uniform_data_t*)pUniformData;
	matrix4x4f_t positionMatrix     = _k15_mul_matrix4x4f(&pShaderData->viewProjMatrix, &pShaderData->modelMatrix);
	//matrix4x4f_t positionMatrix     = _k15_mul_matrix4x4f(&pShaderData->modelMatrix, &pShaderData->viewProjMatrix);
    //matrix4x4f_t invPositionMatrix  = _k15_inverse_matrix4x4f(&positionMatrix);
    matrix4x4f_t normalMatrix       = positionMatrix;

	const vector4f_t viewDir = pShaderData->viewDir;
	const vector4f_t specColor = k15_create_vector4f(1.0f, 1.0f, 1.0f, 1.0f);
    
	_k15_mul_multiple_vector4_matrix44(pInOutVertices->positions, &positionMatrix, vertexCount);
	_k15_mul_multiple_vector4_matrix44(pInOutVertices->normals, &normalMatrix, vertexCount);

#if 0
	for( uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex )
	{
		vector4f_t color = pShaderData->ambientColor;
		vector4f_t lightColor = k15_create_vector4f(0.0f, 0.0f, 0.0f, 1.0f);
		for( uint32_t lightIndex = 0; lightIndex < pShaderData->lightCount; ++lightIndex )
		{
			const float lightRadiusSquared = pShaderData->lights[lightIndex].radius *  pShaderData->lights[lightIndex].radius;
			const vector4f_t vertexToLight = k15_vector4f_sub(pShaderData->lights[lightIndex].position, pInOutVertices->positions[vertexIndex]);
			const vector4f_t vertexToLightNormalized = k15_vector4f_normalize(vertexToLight);
			const float lightAngle = k15_vector4f_dot(vertexToLightNormalized, pInOutVertices->normals[vertexIndex]);
			const float vertexToLightSquared = k15_vector4f_length_squared(vertexToLight);
			const vector4f_t halfDir = k15_vector4f_normalize(k15_vector4f_add(vertexToLightNormalized, viewDir));
			const float specAngle = get_max(0.0f, k15_vector4f_dot(halfDir, pInOutVertices->normals[vertexIndex]));
			const float specular = powf(specAngle, 16.0);

			if( vertexToLightSquared <= lightRadiusSquared )
			{
				const float distanceNormalized = 1.0f - (vertexToLightSquared / lightRadiusSquared);
				lightColor = k15_vector4f_add(lightColor, k15_vector4f_add(k15_vector4f_scale(specColor, specular), k15_vector4f_scale(pShaderData->lights[lightIndex].color, distanceNormalized * lightAngle)));
			}
		}

		pInOutVertices->colors[vertexIndex] = k15_vector4f_clamp01(k15_vector4f_add(lightColor, pShaderData->ambientColor));
	}
#endif
	
}

void pixelShader(const pixel_shader_input_t* pPixelShaderInput, pixel_shader_output_t* pPixelShaderOutput, uint32_t pixelCount, const void* pUniformData)
{
	shader_uniform_data_t* pShaderData = (shader_uniform_data_t*)pUniformData;
	texture_samples_t textureSamples = k15_sample_texture<sample_addressing_mode_t::clamp>(pShaderData->texture, pPixelShaderInput, pixelCount);
	
	const vector4f_t viewDir = pShaderData->viewDir;
	const vector4f_t specColor = k15_create_vector4f(1.0f, 1.0f, 1.0f, 1.0f);

#if 0
	for( uint32_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex )
	{
		vector4f_t color = k15_create_vector4f(pPixelShaderInput->pVertexData[pixelIndex].texcoord.x, pPixelShaderInput->pVertexData[pixelIndex].texcoord.y, 0.0f, 1.0f);
		pPixelShaderOutput->pColor[pixelIndex] = color;
	}
#else
	for( uint32_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex )
	{
		vector4f_t color = k15_create_vector4f(textureSamples.pColors[pixelIndex].x, textureSamples.pColors[pixelIndex].y, textureSamples.pColors[pixelIndex].z, textureSamples.pColors[pixelIndex].w);
		color = k15_vector4f_hadamard(color, pShaderData->ambientColor);

#if 0
		for( uint32_t lightIndex = 0; lightIndex < pShaderData->lightCount; ++lightIndex )
		{
			const vector4f_t lightPosition = _k15_mul_vector4_matrix44(&pShaderData->lights[lightIndex].position, &pShaderData->viewProjMatrix);
			const float lightRadiusSquared = pShaderData->lights[lightIndex].radius *  pShaderData->lights[lightIndex].radius;
			const vector4f_t pixelToLight = k15_vector4f_sub(lightPosition, pInOutPixels->vertexAttributes.positions[pixelIndex]);
			const vector4f_t pixelToLightNormalized = k15_vector4f_normalize(pixelToLight);
			const float lightAngle = k15_vector4f_dot(pixelToLightNormalized, pInOutPixels->vertexAttributes.positions[pixelIndex]);
			const float pixelToLightSquared = k15_vector4f_length_squared(pixelToLight);

			if( pixelToLightSquared > lightRadiusSquared )
			{
				continue;
			}

			const vector4f_t halfDir = k15_vector4f_normalize(k15_vector4f_add(pixelToLightNormalized, viewDir));
			const float specAngle = get_max(0.0f, k15_vector4f_dot(halfDir, pInOutPixels->vertexAttributes.normals[pixelIndex]));
			const float specular = powf(specAngle, 2.0);

			const float distanceNormalized = 1.0f - (pixelToLightSquared / lightRadiusSquared);
			color = k15_vector4f_clamp01(k15_vector4f_add(color, k15_vector4f_scale(pShaderData->lights[lightIndex].color, distanceNormalized * lightAngle)));
		}
#endif
		pPixelShaderOutput->pColor[pixelIndex] = color;
	}
#endif
}

bool setupThreadPool()
{
	return true;
}

loaded_model_t loadedModel = {};
bool setup()
{
	pDepthBufferPixels = (float*)_mm_malloc(virtualScreenWidth * virtualScreenHeight * sizeof(float), 16);
	memset(pDepthBufferPixels, 0, virtualScreenWidth * virtualScreenHeight * sizeof(float));

	if(!setupThreadPool())
	{
		return false;
	}

	software_rasterizer_context_init_parameters_t parameters = k15_create_default_software_rasterizer_context_parameters(virtualScreenWidth, virtualScreenHeight, (void**)&pBackBufferPixels, (void**)&pDepthBufferPixels, 1u);
	parameters.redShift 	= 16;
	parameters.greenShift 	= 8;
	parameters.blueShift 	= 0;

	if(!k15_create_software_rasterizer_context(&pContext, &parameters))
	{
		printf("Couldn't create software rendering context.\n");
		return false;
	}

	k15_create_projection_matrix(&projectionMatrix, virtualScreenWidth, virtualScreenHeight, 0.2f, 20.f, 90.f);

	vertexShaderHandle = k15_create_vertex_shader(pContext, vertexShader);
	pixelShaderHandle = k15_create_pixel_shader(pContext, pixelShader);
	uniformBufferHandle = k15_create_uniform_buffer(pContext, sizeof(shaderData));

	shaderData.lightCount = 1u;
	shaderData.lights[0].color = k15_create_vector4f(1.0f, 1.0f, 1.0f, 0.0f);
	shaderData.lights[0].position = k15_create_vector4f(0.0f, 0.0f, 1.0f, 1.0f);
	shaderData.lights[0].radius = 1.0f;

	shaderData.lights[1].color = k15_create_vector4f(1.0f, 0.0f, 1.0f, 0.0f);
	shaderData.lights[1].position = k15_create_vector4f(0.0f, 1.0f, 2.0f, 1.0f);
	shaderData.lights[1].radius = 1.0f;

	shaderData.ambientColor = k15_create_vector4f(1.0f, 1.0f, 1.0f, 1.0f);
	//shaderData.ambientColor = k15_create_vector4f(0.1f, 0.1f, 0.1f, 1.0f);

	return loadTriangle(pContext, &loadedModel);
	//return loadObjModel(pContext, &loadedModel, "crashbandicoot.obj", "crashbandicoot.mtl", 0.005f);
	//return loadScnModel(pContext, &loadedModel, "helmet.scn", 0.5f);
}

void drawBackBuffer(HDC deviceContext)
{
	StretchDIBits(deviceContext, 0, 0, screenWidth, screenHeight, 0, 0, virtualScreenWidth, virtualScreenHeight, 
		pBackBufferPixels, pBackBufferBitmapInfo, DIB_RGB_COLORS, SRCCOPY);  
}

void doFrame(float deltaTimeInMs)
{
	static float angle = 0.5f;
	static bool lightGoLeft = false;
	static bool lightGoUp = false;

	angle += 0.0002f * deltaTimeInMs;

	static float scale = 0.2f;

	matrix4x4f_t rotation = {cosf(angle), 	0.0f, 	sinf(angle), 	0.0f,
							 0.0f, 			1.0f, 	0.0f, 			0.0f,
							 -sinf(angle), 	0.0f, 	cosf(angle), 	0.0f,
							 0.0f, 			0.0f, 	0.0f, 			1.0f};

	shaderData.lights[0].position = _k15_mul_vector4_matrix44(&shaderData.lights[0].position, &rotation);

	matrix4x4f_t modelMatrix = {};
	k15_set_identity_matrix4x4f(&modelMatrix);

	modelMatrix.m11 = -1.0f;

	if( appHasFocus )
	{
		POINT mousePos = {};
		GetCursorPos(&mousePos);
		cameraAngles.x -= ( mousePos.x - (screenWidth / 2) ) / 1000.0f;
		cameraAngles.y -= ( mousePos.y - (screenHeight / 2) ) / 1000.0f;
		SetCursorPos(screenWidth / 2, screenHeight / 2);
	}

	const float cameraVelocityScale = 0.002f;

	matrix4x4f_t cameraOrientationX = {	cosf(cameraAngles.x), 	0.0f, 	sinf(cameraAngles.x), 	0.0f,
										0.0f, 					1.0f, 	0.0f, 			0.0f,
										-sinf(cameraAngles.x), 	0.0f, 	cosf(cameraAngles.x), 	0.0f,
										0.0f, 					0.0f, 	0.0f, 			1.0f};

	matrix4x4f_t cameraOrientationY = {	1.0f, 	0.0f, 					0.0f, 					0.0f,
										0.0f, 	-cosf(-cameraAngles.y), 	-sinf(-cameraAngles.y), 	0.0f,
										0.0f, 	sinf(-cameraAngles.y), 	cosf(-cameraAngles.y), 	0.0f,
										0.0f, 	0.0f, 					0.0f, 					1.0f};

	matrix4x4f_t cameraOrientation = _k15_mul_matrix4x4f(&cameraOrientationX, &cameraOrientationY);

	vector4f_t fwd = k15_create_vector4f(0.0f, 0.0f, 1.0f, 0.0f);
	vector4f_t fwd2 =_k15_mul_vector4_matrix44(&fwd, &cameraOrientation);
	const vector3f_t up = k15_create_vector3f(0.0f, 1.0f, 0.0f);
	const vector3f_t forward = k15_create_vector3f(fwd2.x, fwd2.y, fwd2.z);
	const vector3f_t xAxis = k15_vector3f_normalize(k15_vector3f_cross(up, forward));
	const vector3f_t zAxis = k15_vector3f_normalize(forward);
	const vector3f_t yAxis = k15_vector3f_cross(zAxis, xAxis);
	vector4f_t transformedCameraVelocity = _k15_mul_vector4_matrix44(&cameraVelocity, &cameraOrientation);
	transformedCameraVelocity = k15_vector4f_scale(transformedCameraVelocity, cameraVelocityScale);
	cameraPos = k15_vector4f_add(k15_vector4f_scale(transformedCameraVelocity, deltaTimeInMs), cameraPos);
	vector3f_t c = k15_create_vector3f(cameraPos.x, cameraPos.y, cameraPos.z);

	matrix4x4f_t viewMatrix = {xAxis.x, 	xAxis.y, 	xAxis.z, 	-k15_vector3f_dot(xAxis, c),
							   yAxis.x, 	yAxis.y, 	yAxis.z, 	-k15_vector3f_dot(yAxis, c),
							   zAxis.x, 	zAxis.y, 	zAxis.z, 	-k15_vector3f_dot(zAxis, c),
							   0.0f, 		0.0f, 		0.0f,		1.0f};
	
	if(lightGoLeft)
	{
		shaderData.lights[0].position.x -= 0.0005f * deltaTimeInMs;
		if( shaderData.lights[0].position.x < -1.0f )
		{
			lightGoLeft = false;
		}
	}
	else
	{
		shaderData.lights[0].position.x += 0.0005f * deltaTimeInMs;
		if( shaderData.lights[0].position.x > 1.0f )
		{
			lightGoLeft = true;
		}
	}

	if(lightGoUp)
	{
		shaderData.lights[1].position.y -= 0.0005f * deltaTimeInMs;
		if( shaderData.lights[1].position.y < -1.0f )
		{
			lightGoUp = false;
		}
	}
	else
	{
		shaderData.lights[1].position.y += 0.0005f * deltaTimeInMs;
		if( shaderData.lights[1].position.y > 1.0f )
		{
			lightGoUp = true;
		}
	}

	shaderData.modelMatrix = modelMatrix;
	shaderData.viewProjMatrix = _k15_mul_matrix4x4f(&projectionMatrix, &viewMatrix);
	shaderData.viewPos = cameraPos;

	k15_bind_vertex_shader(pContext, vertexShaderHandle);
	k15_bind_pixel_shader(pContext, pixelShaderHandle);
	k15_bind_uniform_buffer(pContext, uniformBufferHandle);

	pContext->settings.drawWireframe 	= drawWireframe;
	pContext->settings.drawDepthBuffer 	= drawDepthBuffer;

#if 1
	for( uint32_t subModelIndex = 0; subModelIndex < loadedModel.subModelCount; ++subModelIndex )
	{
		k15_bind_vertex_buffer(pContext, loadedModel.vertexBuffers[subModelIndex]);
		k15_bind_texture(pContext, loadedModel.textures[subModelIndex], 0u);
		shaderData.texture = loadedModel.textures[subModelIndex];
		shaderData.normalMap = loadedModel.textures[1];
		shaderData.viewDir = k15_create_vector4f(shaderData.viewProjMatrix.m20, shaderData.viewProjMatrix.m21, shaderData.viewProjMatrix.m22, 0.0f);
		k15_set_uniform_buffer_data(uniformBufferHandle, &shaderData, sizeof(shaderData), 0u);
		k15_draw(pContext, loadedModel.vertexCounts[subModelIndex], 0u);
	}
#else
	const uint32_t subModelIndex = 1u;
	k15_bind_vertex_buffer(pContext, loadedModel.vertexBuffers[subModelIndex]);
	k15_bind_texture(pContext, loadedModel.textures[subModelIndex], 0u);
	shaderData.texture = loadedModel.textures[subModelIndex];
	shaderData.normalMap = loadedModel.textures[1];
	shaderData.viewDir = k15_create_vector4f(shaderData.viewProjMatrix.m20, shaderData.viewProjMatrix.m21, shaderData.viewProjMatrix.m22, 0.0f);
	k15_set_uniform_buffer_data(uniformBufferHandle, &shaderData, sizeof(shaderData), 0u);
	k15_draw(pContext, loadedModel.vertexCounts[subModelIndex], 0u);
#endif

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

		sprintf(windowTitle, "Software Renderer - %.3f ms - camera angle {%.3f - %.3f}", deltaMs, cameraAngles.x, cameraAngles.y);
		SetWindowText(hwnd, windowTitle);
	}

	DestroyWindow(hwnd);

	return 0;
}