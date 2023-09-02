#ifndef K15_SOFTWARE_RASTERIZER_INCLUDE
#define K15_SOFTWARE_RASTERIZER_INCLUDE

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include <math.h>

#include "k15_font.hpp"

struct vector4f_t
{
    float x, y, z, w;
};

struct vector3f_t
{
    float x, y, z;
};

struct vector2i_t
{
    int32_t x, y;
};

struct bounding_box_t
{
    int32_t x1, x2, y1, y2;
};

union matrix4x4f_t
{
    struct
    {
        float m00, m01, m02, m03;
        float m10, m11, m12, m13;
        float m20, m21, m22, m23;
        float m30, m31, m32, m33;
    };

    float m[16];
};

enum topology_t
{
    none = 0,
    triangle
};

struct software_rasterizer_context_t;

struct software_rasterizer_context_init_parameters_t
{
    uint32_t    backBufferWidth;
    uint32_t    backBufferHeight;
    uint32_t    backBufferStride;
    void*       pColorBuffers[3];
    uint8_t     colorBufferCount;
};

bool    k15_create_software_rasterizer_context(software_rasterizer_context_t** pOutContextPtr, const software_rasterizer_context_init_parameters_t* pParameters);

void    k15_swap_color_buffers(software_rasterizer_context_t* pContext);
void    k15_draw_frame(software_rasterizer_context_t* pContext);

void    k15_change_color_buffers(software_rasterizer_context_t* pContext, void* pColorBuffers[3], uint8_t colorBufferCount, uint32_t widthInPixels, uint32_t heightInPixels, uint32_t strideInBytes);

bool    k15_begin_geometry(software_rasterizer_context_t* pContext, topology_t topology);
void    k15_end_geometry(software_rasterizer_context_t* pContext);

void    k15_vertex_position(software_rasterizer_context_t* pContext, float x, float y, float z);
void    k15_vertex_color(software_rasterizer_context_t* pContext, float r, float g, float b, float a);
void    k15_vertex_uv(software_rasterizer_context_t* pContext, float u, float v);

#ifdef K15_SOFTWARE_RASTERIZER_IMPLEMENTATION

#ifdef _MSC_BUILD
#define restrict_modifier       __restrict
#define RuntimeAssert(x)        if(!(x)){  __debugbreak(); }
#else
#warning No support for this compiler
#define restrict_modifier
#define RuntimeAssert(x)
#endif

#define RuntimeAssertMsg(x, msg) RuntimeAssert(x)
#define UnusedVariable(x) (void)(x)

#define get_min(a,b) (a)>(b)?(b):(a)
#define get_max(a,b) (a)>(b)?(a):(b)

constexpr uint32_t MaxColorBuffer = 3u;
constexpr uint32_t DefaultTriangleBufferCapacity = 1024u;
constexpr float pi = 3.141f;

constexpr matrix4x4f_t IdentityMatrix4x4[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

enum vertex_id_t : uint32_t
{
    position,
    color,

    count
};

struct geometry_t
{
    topology_t  topology;
    uint8_t   vertexCount;
    uint32_t  startIndices[ vertex_id_t::count ];
};

struct triangle4f_t
{
    vector4f_t points[3];
};

struct screenspace_triangle2i_t
{
    vector2i_t      points[3];
    bounding_box_t  boundingBox;
};

template<typename T>
struct base_static_buffer_t
{
protected:
    base_static_buffer_t() {};

public:
    uint32_t    capacity;
    uint32_t    count;
    T*          pStaticData;
};

template<typename T>
struct dynamic_buffer_t
{
    uint32_t    count;
    uint32_t    capacity;
    T*          pData;
};

template<typename T, uint32_t SIZE>
struct static_buffer_t : public base_static_buffer_t<T>
{
    static_buffer_t() 
    {
        pStaticData = data;
        capacity = 0;
        count = 0;
    }

    T data[SIZE];
};

struct software_rasterizer_settings_t
{
    uint8_t backFaceCullingEnabled : 1;
};

struct bitmap_font_t
{
    const uint8_t* pRGBPixels;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
};

struct software_rasterizer_context_t
{
    software_rasterizer_settings_t              settings;
    bitmap_font_t                               font;

    uint8_t                                     colorBufferCount;
    uint8_t                                     currentColorBufferIndex;
    uint8_t                                     currentTriangleVertexIndex;

    topology_t                                  currentTopology;

    uint32_t                                    backBufferWidth;
    uint32_t                                    backBufferHeight;
    uint32_t                                    backBufferStride;

    void*                                       pColorBuffer[MaxColorBuffer];
    
    dynamic_buffer_t<triangle4f_t>              triangles;
    dynamic_buffer_t<triangle4f_t>              visibleTriangles;
    dynamic_buffer_t<triangle4f_t>              clippedTriangles;
    dynamic_buffer_t<screenspace_triangle2i_t>  screenSpaceTriangles;

    matrix4x4f_t                                projectionMatrix;
    matrix4x4f_t                                viewMatrix;
};

//https://jsantell.com/3d-projection/
void _k15_create_projection_matrix(matrix4x4f_t* pOutMatrix, uint32_t width, uint32_t height, float near, float far, float fov)
{
    memset(pOutMatrix, 0, sizeof(matrix4x4f_t));

    const float widthF = (float)width;
    const float heightF = (float)height;
    const float fovRad = fov/180.f * pi;
    const float aspect = widthF / heightF;
    const float e = 1.0f/tanf(fovRad/2.0f);

    pOutMatrix->m00 = e/aspect;
    pOutMatrix->m11 = e;
    pOutMatrix->m22 = -((far + near)/(near - far));
    pOutMatrix->m23 = -((2.0f*far*near)/(near-far));
    pOutMatrix->m32 = -1.0f;
}

void _k15_create_orthographic_matrix(matrix4x4f_t* pOutMatrix, uint32_t width, uint32_t height, float near, float far)
{
    memset(pOutMatrix, 0, sizeof(matrix4x4f_t));

    pOutMatrix->m00 = 1.0f / (float)width;
    pOutMatrix->m11 = 1.0f / (float)height;
    pOutMatrix->m22 = -2.0f / (far-near);
    pOutMatrix->m23 = -((far+near)/(far-near));
    pOutMatrix->m33 = 1.0f;
}

void _k15_set_identity_matrix4x4f(matrix4x4f_t* pMatrix)
{
    memcpy(pMatrix, IdentityMatrix4x4, sizeof(IdentityMatrix4x4));
}

bool _k15_matrix4x4f_is_equal(const matrix4x4f_t* restrict_modifier pA, const matrix4x4f_t* restrict_modifier pB)
{
    return memcmp(pA, pB, sizeof(matrix4x4f_t)) == 0;
}

void _k15_test_image_for_color_buffer(const software_rasterizer_context_t* pContext)
{
    uint32_t* pBackBuffer = (uint32_t*)pContext->pColorBuffer[ pContext->currentColorBufferIndex ];
    for(uint32_t y = 0; y < pContext->backBufferHeight; ++y)
    {
        if((y%4)==0)
        {
            for(uint32_t x = 0; x < pContext->backBufferWidth; ++x)
            {
                const uint32_t index = x + y * pContext->backBufferStride;
                pBackBuffer[index]=0xFF0000FF;
            }
        }
        else if((y%3)==0)
        {
            for(uint32_t x = 0; x < pContext->backBufferWidth; ++x)
            {
                const uint32_t index = x + y * pContext->backBufferStride;
                pBackBuffer[index]=0xFFFF0000;
            }
        }
        else if((y%2)==0)
        {
            for(uint32_t x = 0; x < pContext->backBufferWidth; ++x)
            {
                const uint32_t index = x + y * pContext->backBufferStride;
                pBackBuffer[index]=0xFF00FF00;
            }
        }
        else
        {
            for(uint32_t x = 0; x < pContext->backBufferWidth; ++x)
            {
                const uint32_t index = x + y * pContext->backBufferStride;
                pBackBuffer[index]=0xFFFFFFFF;
            }
        }
    } 
}

vector4f_t _k15_mul_vector4_matrix44(const vector4f_t* pVector, const matrix4x4f_t* pMatrix)
{
    vector4f_t multipliedVector = {};
    multipliedVector.x = pVector->x * pMatrix->m00 + pVector->y * pMatrix->m01 + pVector->z * pMatrix->m02 + pVector->w * pMatrix->m03;
    multipliedVector.y = pVector->x * pMatrix->m10 + pVector->y * pMatrix->m11 + pVector->z * pMatrix->m12 + pVector->w * pMatrix->m13;
    multipliedVector.z = pVector->x * pMatrix->m20 + pVector->y * pMatrix->m21 + pVector->z * pMatrix->m22 + pVector->w * pMatrix->m23;
    multipliedVector.w = pVector->x * pMatrix->m30 + pVector->y * pMatrix->m31 + pVector->z * pMatrix->m32 + pVector->w * pMatrix->m33;
    return multipliedVector;
}

//http://www.edepot.com/linee.html
void _k15_draw_line(void* pColorBuffer, uint32_t colorBufferStride, const vector2i_t* pVectorA, const vector2i_t* pVectorB, uint32_t color)
{
    int x = pVectorA->x;
    int y = pVectorA->y;
    int x2 = pVectorB->x;
    int y2 = pVectorB->y;

    bool yLonger=false;
    int shortLen=y2-y;
    int longLen=x2-x;

    uint32_t* pBackBuffer = (uint32_t*)pColorBuffer;

    if (abs(shortLen)>abs(longLen)) {
        int swap=shortLen;
        shortLen=longLen;
        longLen=swap;				
        yLonger=true;
    }
    int decInc;
    if (longLen==0) decInc=0;
    else decInc = (shortLen << 16) / longLen;

    if (yLonger) {
        if (longLen>0) {
            longLen+=y;
            for (int j=0x8000+(x<<16);y<=longLen;++y) {
                const uint32_t localX = j >> 16;
                const uint32_t localY = y;
                pBackBuffer[localX + localY * colorBufferStride] = color;
                j+=decInc;
            }
            return;
        }
        longLen+=y;
        for (int j=0x8000+(x<<16);y>=longLen;--y) {
            const uint32_t localX = j >> 16;
            const uint32_t localY = y;
            pBackBuffer[localX + localY * colorBufferStride] = color;
            j-=decInc;
        }
        return;	
    }

    if (longLen>0) {
        longLen+=x;
        for (int j=0x8000+(y<<16);x<=longLen;++x) {
            const uint32_t localX = x;
            const uint32_t localY = j >> 16;
            pBackBuffer[localX + localY * colorBufferStride] = color;
            j+=decInc;
        }
        return;
    }
    longLen+=x;
    for (int j=0x8000+(y<<16);x>=longLen;--x) {
        const uint32_t localX = x;
        const uint32_t localY = j >> 16;
        pBackBuffer[localX + localY * colorBufferStride] = color;
        j-=decInc;
    }
}

void k15_draw_triangles(void* pColorBuffer, uint32_t colorBufferStride, const dynamic_buffer_t<screenspace_triangle2i_t>* pScreenSpaceTriangleBuffer)
{
    for(uint32_t triangleIndex = 0; triangleIndex < pScreenSpaceTriangleBuffer->count; ++triangleIndex)
    {
        const screenspace_triangle2i_t* pTriangle = pScreenSpaceTriangleBuffer->pData + triangleIndex;
        _k15_draw_line(pColorBuffer, colorBufferStride, &pTriangle->points[0], &pTriangle->points[1], 0xFFFFFFFF);
        _k15_draw_line(pColorBuffer, colorBufferStride, &pTriangle->points[1], &pTriangle->points[2], 0xFFFFFFFF);
        _k15_draw_line(pColorBuffer, colorBufferStride, &pTriangle->points[2], &pTriangle->points[0], 0xFFFFFFFF);
    }

    printf("Triangles: %d\n", pScreenSpaceTriangleBuffer->count);
}

//https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
inline bool _k15_is_pow2(uint32_t value)
{
    return (value & (value - 1)) == 0;
}

//https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline uint32_t _k15_get_next_pow2(uint32_t value)
{
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;

    return value;
}

template<typename T>
bool _k15_create_dynamic_buffer(dynamic_buffer_t<T>* pOutBuffer, uint32_t initialCapacity)
{
    pOutBuffer->pData = (T*)malloc(initialCapacity * sizeof(T));
    if(pOutBuffer->pData == nullptr)
    {
        return false;
    }

    pOutBuffer->capacity = initialCapacity;
    pOutBuffer->count = 0;

    return true;
}

template<typename T>
bool _k15_grow_dynamic_buffer(dynamic_buffer_t<T>* pBuffer, uint32_t newCapacity)
{
    if(pBuffer->capacity > newCapacity)
    {
        return true;
    }
    
    const uint32_t newPow2Size = _k15_is_pow2(newCapacity) ? newCapacity : _k15_get_next_pow2(newCapacity);
    T* pNewTriangleBuffer = (T*)malloc(newPow2Size * sizeof(T));
    if(pNewTriangleBuffer == nullptr)
    {
        return false;
    }

    memcpy(pNewTriangleBuffer, pBuffer->pData, pBuffer->capacity * sizeof(T));

    free(pBuffer->pData);
    pBuffer->pData = pNewTriangleBuffer;
    pBuffer->capacity = newPow2Size;

    return true;
}

template<typename T>
T* _k15_dynamic_buffer_push_back(dynamic_buffer_t<T>* pBuffer, uint32_t elementCount)
{
    const uint32_t newTriangleCount = pBuffer->count + elementCount;
    if(newTriangleCount >= pBuffer->capacity)
    {
        if(!_k15_grow_dynamic_buffer(pBuffer, newTriangleCount))
        {
            return nullptr;
        }
    }

    uint32_t oldCount = pBuffer->count;
    pBuffer->count += elementCount;
    return pBuffer->pData + oldCount;
}

template<typename T>
T* _k15_dynamic_buffer_push_back(dynamic_buffer_t<T>* pBuffer, T value)
{
    T* pValue = _k15_dynamic_buffer_push_back(pBuffer, 1u);
    if(pValue == nullptr)
    {
        return nullptr;
    }

    memcpy(pValue, &value, sizeof(T));
    return pValue;
}

template<typename T>
T* _k15_static_buffer_push_back(base_static_buffer_t<T>* pStaticBuffer, uint32_t elementCount)
{
    const uint32_t bufferCapacity = pStaticBuffer->capacity;
    if((pStaticBuffer->count + elementCount) == bufferCapacity)
    {
        return nullptr;
    }

    const uint32_t oldCount = pStaticBuffer->count;
    pStaticBuffer->count += elementCount;
    return pStaticBuffer->pStaticData + oldCount;
}

template<typename T>
T* _k15_static_buffer_push_back(base_static_buffer_t<T>* pStaticBuffer, T value)
{
    T* pValue = _k15_static_buffer_push_back(pStaticBuffer, 1u);
    if(pValue == nullptr)
    {
        return nullptr;
    }

    memcpy(pValue, &value, sizeof(T));
    return pValue;
}

template<typename T>
bool _k15_push_static_buffer_to_dynamic_buffer(base_static_buffer_t<T>* pStaticBuffer, dynamic_buffer_t<T>* pDynamicBuffer)
{
    T* pDataFromDynamicBuffer = _k15_dynamic_buffer_push_back<T>(pDynamicBuffer, pStaticBuffer->count);
    if(pDataFromDynamicBuffer == nullptr)
    {
        return false;
    }

    memcpy(pDataFromDynamicBuffer, pStaticBuffer->pStaticData, sizeof(T) * pStaticBuffer->count);
    return true;
}

vector4f_t k15_create_vector4f(float x, float y, float z, float w)
{
    return {x, y, z, w};
}

bool _k15_create_font(bitmap_font_t* pOutFont)
{
    const size_t bitmapSizeInBytes = imageSizeX * imageSizeY * imageChannelCount;
    pOutFont->pRGBPixels = pImageData;
    pOutFont->height = imageSizeX;
    pOutFont->width = imageSizeY;
    pOutFont->channels = imageChannelCount;
    return true;
}

bool k15_create_software_rasterizer_context(software_rasterizer_context_t** pOutContextPtr, const software_rasterizer_context_init_parameters_t* pParameters)
{
    software_rasterizer_context_t* pContext = (software_rasterizer_context_t*)malloc(sizeof(software_rasterizer_context_t));
    pContext->backBufferHeight  = pParameters->backBufferHeight;
    pContext->backBufferWidth   = pParameters->backBufferWidth;
    pContext->backBufferStride  = pParameters->backBufferStride;
    pContext->currentColorBufferIndex = 0;
    pContext->currentTopology = topology_t::none;
    pContext->currentTriangleVertexIndex = 0;

    if(!_k15_create_font(&pContext->font))
    {
        return false;
    }

    pContext->settings.backFaceCullingEnabled = 0;
    for(uint8_t colorBufferIndex = 0; colorBufferIndex < pParameters->colorBufferCount; ++colorBufferIndex)
    {
        pContext->pColorBuffer[colorBufferIndex] = pParameters->pColorBuffers[colorBufferIndex];
    }

    pContext->colorBufferCount = pParameters->colorBufferCount;

    if(!_k15_create_dynamic_buffer<triangle4f_t>(&pContext->triangles, DefaultTriangleBufferCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<triangle4f_t>(&pContext->visibleTriangles, DefaultTriangleBufferCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<triangle4f_t>(&pContext->clippedTriangles, DefaultTriangleBufferCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<screenspace_triangle2i_t>(&pContext->screenSpaceTriangles, DefaultTriangleBufferCapacity))
    {
        return false;
    }

    _k15_create_projection_matrix(&pContext->projectionMatrix, pParameters->backBufferWidth, pParameters->backBufferHeight, 1.0f, 10.f, 90.0f);
    _k15_set_identity_matrix4x4f(&pContext->viewMatrix);

    *pOutContextPtr = pContext;
    return true;
}

void k15_swap_color_buffers(software_rasterizer_context_t* pContext)
{
    if(pContext->colorBufferCount == 1u)
    {
        return;
    }

    const uint8_t newColorBufferIndex = ++pContext->currentColorBufferIndex % pContext->colorBufferCount;
    pContext->currentColorBufferIndex = newColorBufferIndex;
}

matrix4x4f_t _k15_mul_matrix4x4f(const matrix4x4f_t* restrict_modifier pMatA, const matrix4x4f_t* restrict_modifier pMatB)
{
    matrix4x4f_t mat = {};
    mat.m00 = pMatA->m00 * pMatB->m00 + pMatA->m01 * pMatB->m10 + pMatA->m02 * pMatB->m20 + pMatA->m03 * pMatB->m30;
    mat.m01 = pMatA->m00 * pMatB->m01 + pMatA->m01 * pMatB->m11 + pMatA->m02 * pMatB->m21 + pMatA->m03 * pMatB->m31;
    mat.m02 = pMatA->m00 * pMatB->m02 + pMatA->m01 * pMatB->m12 + pMatA->m02 * pMatB->m22 + pMatA->m03 * pMatB->m32;
    mat.m03 = pMatA->m00 * pMatB->m03 + pMatA->m01 * pMatB->m13 + pMatA->m02 * pMatB->m23 + pMatA->m03 * pMatB->m33;

    mat.m10 = pMatA->m10 * pMatB->m00 + pMatA->m11 * pMatB->m10 + pMatA->m12 * pMatB->m20 + pMatA->m13 * pMatB->m30;
    mat.m11 = pMatA->m10 * pMatB->m01 + pMatA->m11 * pMatB->m11 + pMatA->m12 * pMatB->m21 + pMatA->m13 * pMatB->m31;
    mat.m12 = pMatA->m10 * pMatB->m02 + pMatA->m11 * pMatB->m12 + pMatA->m12 * pMatB->m22 + pMatA->m13 * pMatB->m32;
    mat.m13 = pMatA->m10 * pMatB->m03 + pMatA->m11 * pMatB->m13 + pMatA->m12 * pMatB->m23 + pMatA->m13 * pMatB->m33;

    mat.m20 = pMatA->m20 * pMatB->m00 + pMatA->m21 * pMatB->m10 + pMatA->m22 * pMatB->m20 + pMatA->m23 * pMatB->m30;
    mat.m21 = pMatA->m20 * pMatB->m01 + pMatA->m21 * pMatB->m11 + pMatA->m22 * pMatB->m21 + pMatA->m23 * pMatB->m31;
    mat.m22 = pMatA->m20 * pMatB->m02 + pMatA->m21 * pMatB->m12 + pMatA->m22 * pMatB->m22 + pMatA->m23 * pMatB->m32;
    mat.m23 = pMatA->m20 * pMatB->m03 + pMatA->m21 * pMatB->m13 + pMatA->m22 * pMatB->m23 + pMatA->m23 * pMatB->m33;

    mat.m30 = pMatA->m30 * pMatB->m00 + pMatA->m31 * pMatB->m10 + pMatA->m32 * pMatB->m20 + pMatA->m33 * pMatB->m30;
    mat.m31 = pMatA->m30 * pMatB->m01 + pMatA->m31 * pMatB->m11 + pMatA->m32 * pMatB->m21 + pMatA->m33 * pMatB->m31;
    mat.m32 = pMatA->m30 * pMatB->m02 + pMatA->m31 * pMatB->m12 + pMatA->m32 * pMatB->m22 + pMatA->m33 * pMatB->m32;
    mat.m33 = pMatA->m30 * pMatB->m03 + pMatA->m31 * pMatB->m13 + pMatA->m32 * pMatB->m23 + pMatA->m33 * pMatB->m33;

    return mat;
}

void k15_transform_triangles(const matrix4x4f_t* restrict_modifier pProjectionMatrix, const matrix4x4f_t* restrict_modifier pViewMatrix, dynamic_buffer_t<triangle4f_t>* pTriangles)
{
    matrix4x4f_t viewProjMat = _k15_mul_matrix4x4f(pProjectionMatrix, pViewMatrix);
    
    for(uint32_t triangleIndex = 0; triangleIndex < pTriangles->count; ++triangleIndex)
    {
        triangle4f_t* pTriangle = pTriangles->pData + triangleIndex;
        pTriangle->points[0] = _k15_mul_vector4_matrix44(&pTriangle->points[0], &viewProjMat);
        pTriangle->points[1] = _k15_mul_vector4_matrix44(&pTriangle->points[1], &viewProjMat);
        pTriangle->points[2] = _k15_mul_vector4_matrix44(&pTriangle->points[2], &viewProjMat);
    }
}

inline vector4f_t k15_vector4f_normalize(vector4f_t vector)
{
    vector4f_t normalizedVector = {};
    float vectorLength = sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w);
    normalizedVector.x = vector.x / vectorLength;
    normalizedVector.y = vector.y / vectorLength;
    normalizedVector.z = vector.z / vectorLength;
    normalizedVector.w = vector.w / vectorLength;

    return normalizedVector;
}

inline vector4f_t _k15_vector4f_cross(vector4f_t a, vector4f_t b)
{
    vector4f_t crossVector = {};
    crossVector.x = a.y * b.z - a.z * b.y;
    crossVector.y = a.z * b.x - a.x * b.z;
    crossVector.z = a.x * b.y - a.y * b.x;
    crossVector.w = 0.0f; //TODO

    return crossVector;
}

vector4f_t _k15_vector4f_add(vector4f_t a, vector4f_t b)
{
    vector4f_t v = {};
    v.x = a.x + b.x;
    v.y = a.y + b.y;
    v.z = a.z + b.z;
    v.w = a.w + b.w;

    return v;
}

vector4f_t _k15_vector4f_sub(vector4f_t a, vector4f_t b)
{
    vector4f_t v = {};
    v.x = a.x - b.x;
    v.y = a.y - b.y;
    v.z = a.z - b.z;
    v.w = a.w - b.w;

    return v;
}

bool k15_cull_backfacing_triangles(software_rasterizer_context_t* pContext)
{
    dynamic_buffer_t<triangle4f_t>* pvisibleTriangles = &pContext->visibleTriangles;
    for(uint32_t triangleIndex = 0; triangleIndex < pContext->triangles.count; ++triangleIndex)
    {
        triangle4f_t* pTriangle = (triangle4f_t*)pContext->triangles.pData + triangleIndex;
        const vector4f_t ab = _k15_vector4f_sub(pTriangle->points[1], pTriangle->points[0]);
        const vector4f_t ac = _k15_vector4f_sub(pTriangle->points[2], pTriangle->points[0]);
        const vector4f_t sign = _k15_vector4f_cross(ab, ac);

        if(sign.z > 0.0f)
        {
            triangle4f_t* pNonCulledTriangle = _k15_dynamic_buffer_push_back(pvisibleTriangles, 1u);
            if(pNonCulledTriangle == nullptr)
            {
                return false;
            }
            memcpy(pNonCulledTriangle, pTriangle, sizeof(triangle4f_t));
        }
    }

    return true;
}

inline bool _k15_is_position_outside_frustum(const vector4f_t position)
{
    const float posW = position.w;
    const float negW = -position.w;
    return position.z < negW || position.z > posW || 
        position.y < negW || position.y > posW || 
        position.x < negW || position.x > posW;
}

template<bool APPLY_BACKFACE_CULLING>
void k15_cull_outside_frustum_triangles(dynamic_buffer_t<triangle4f_t>* restrict_modifier pTriangleBuffer, dynamic_buffer_t<triangle4f_t>* restrict_modifier pCulledTriangleBuffer)
{
    static_buffer_t<triangle4f_t, 256> visibleTriangles;

    triangle4f_t* pTriangles = pTriangleBuffer->pData;
    uint32_t numTrianglesCompletelyOutsideFrustum = 0;
    for(uint32_t triangleIndex = 0; triangleIndex < pTriangleBuffer->count; ++triangleIndex)
    {
        const triangle4f_t* pTriangle = pTriangles + triangleIndex;
        const bool triangleOutsideFrustum = _k15_is_position_outside_frustum(pTriangle->points[0]) && _k15_is_position_outside_frustum(pTriangle->points[1]) && _k15_is_position_outside_frustum(pTriangle->points[2]);

        if(!triangleOutsideFrustum)
        {
            if(APPLY_BACKFACE_CULLING)
            {
                const vector4f_t ab = _k15_vector4f_sub(pTriangle->points[1], pTriangle->points[0]);
                const vector4f_t ac = _k15_vector4f_sub(pTriangle->points[2], pTriangle->points[0]);
                const vector4f_t sign = _k15_vector4f_cross(ab, ac);

                const bool isTriangleBackfacing = sign.z < 0.0f;
                if(isTriangleBackfacing)
                {
                    continue;
                }
            }

            triangle4f_t* pVisibleTriangle = _k15_static_buffer_push_back(&visibleTriangles, 1u);
            if(pVisibleTriangle == nullptr)
            {
                if(!_k15_push_static_buffer_to_dynamic_buffer(&visibleTriangles, pCulledTriangleBuffer))
                {
                    //FK: Out of memory
                }

                visibleTriangles.count = 0;
                pVisibleTriangle = _k15_static_buffer_push_back(&visibleTriangles, 1u);
            }

            memcpy(pVisibleTriangle, pTriangle, sizeof(triangle4f_t));
        }
    }

    if(visibleTriangles.count > 0)
    {
        if(!_k15_push_static_buffer_to_dynamic_buffer(&visibleTriangles, pCulledTriangleBuffer))
        {
            //FK: Out of memory
        }
    }
}

void k15_cull_triangles(dynamic_buffer_t<triangle4f_t>* restrict_modifier pTriangleBuffer, dynamic_buffer_t<triangle4f_t>* restrict_modifier pVisibleTriangleBuffer, bool backFaceCullingEnabeld)
{
    if(backFaceCullingEnabeld)
    {
        k15_cull_outside_frustum_triangles<true>(pTriangleBuffer, pVisibleTriangleBuffer);
    }
    else
    {
        k15_cull_outside_frustum_triangles<false>(pTriangleBuffer, pVisibleTriangleBuffer);
    }
}

inline float _k15_signf(float value)
{
    return value > 0.0f ? 1.0f : -1.0f;
}

inline float _k15_vector4f_length_squared(vector4f_t vector)
{
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
}

void _k15_calculate_intersection_with_clip_planes(vector4f_t* pOutIntersectionPoint, const vector4f_t startPosition, const vector4f_t endPosition)
{
    const vector4f_t delta = _k15_vector4f_sub(endPosition, startPosition);
    const float posW = startPosition.w;
    const float negW = -posW;
    
    vector4f_t intersectionPoint = startPosition;
    const float absW = fabsf(intersectionPoint.w);
    float tx = 0.0f;
    float ty = 0.0f;
    float tz = 0.0f;

    if(fabsf(intersectionPoint.x) > posW)
    {
        const float sign = _k15_signf(intersectionPoint.x);
        tx = fabsf((intersectionPoint.x - (sign * absW)) / delta.x);

        intersectionPoint.x = intersectionPoint.x + tx * delta.x;
        intersectionPoint.y = startPosition.y + delta.y * (negW - startPosition.x) / delta.x;
    }

    if(fabsf(intersectionPoint.y) > posW)
    {
        const float sign = _k15_signf(intersectionPoint.y);
        ty = fabsf((intersectionPoint.y - (sign * absW)) / delta.y);
        intersectionPoint.y = intersectionPoint.y + ty * delta.y;
    }

    if(fabsf(intersectionPoint.z) > posW)
    {
        const float sign = _k15_signf(intersectionPoint.z);
        tz = fabsf((intersectionPoint.z - (sign * absW)) / delta.z);
        intersectionPoint.z = intersectionPoint.z + tz * delta.z;
    }

    vector4f_t intersectionDelta = _k15_vector4f_sub(intersectionPoint, startPosition);
    const float deltaLength = _k15_vector4f_length_squared(delta);
    const float intersectionDeltaLength = _k15_vector4f_length_squared(intersectionDelta);

    const float tw = intersectionDeltaLength / deltaLength;
    intersectionPoint.w = intersectionPoint.w + tw * delta.w;

    *pOutIntersectionPoint = intersectionPoint;
}

enum clip_flag_t : uint8_t
{
    Left    = 0b000001,
    Right   = 0b000010,
    Bottom  = 0b000100,
    Top     = 0b001000,
    Near    = 0b010000,
    Far     = 0b100000
};

void _k15_get_clipping_flags(const vector4f_t* ppPoints, uint8_t* ppClippingFlags)
{
    for( uint8_t pointIndex = 0; pointIndex < 2u; ++pointIndex )
    {
        const vector4f_t* pPoint = ppPoints + pointIndex;
        uint8_t* pClippingFlags = ppClippingFlags + pointIndex;

        const float posW = pPoint->w;
        const float negW = -posW;

        *pClippingFlags = 0u;

        if( pPoint->x < negW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Left;
        }
        else if( pPoint->x > posW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Right;
        }

        if( pPoint->y < negW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Top;
        }
        else if( pPoint->y > posW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Bottom;
        }

        if( pPoint->z < negW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Far;
        }
        else if( pPoint->z > posW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Near;
        }
    }
}

void k15_clip_triangles(dynamic_buffer_t<triangle4f_t>* restrict_modifier pVisibleTriangleBuffer, dynamic_buffer_t<triangle4f_t>* restrict_modifier pClippedTriangleBuffer)
{
    struct triangle_vertex4f_t
    {
        vector4f_t  position;
        uint32_t    triangleId;
        uint8_t     clipFlags;
    };

    static_buffer_t<triangle_vertex4f_t, 512> localClippedVertices;
    const triangle4f_t* pVisibleTriangles = (triangle4f_t*)pVisibleTriangleBuffer->pData;
    for(uint32_t triangleIndex = 0; triangleIndex < pVisibleTriangleBuffer->count; ++triangleIndex)
    {
        bool clipped = false;
        bool currentVertexClipped = false;
        const triangle4f_t* pTriangle = pVisibleTriangles + triangleIndex;
        vector4f_t intersectionPoint = {};

        for(uint32_t vertexIndex = 0; vertexIndex < 3u; ++vertexIndex)
        {
            const uint32_t nextVertexIndex = ( vertexIndex + 1u ) % 3u;

            vector4f_t edgeVertices[2] = {pTriangle->points[vertexIndex], pTriangle->points[nextVertexIndex]};
            uint8_t clippingFlags[2] = {0};

            _k15_get_clipping_flags(edgeVertices, clippingFlags);

        clipStart:
            if( ( clippingFlags[0] | clippingFlags[1] ) == 0 )
            {
                if( !currentVertexClipped )
                {
                    _k15_static_buffer_push_back(&localClippedVertices, {edgeVertices[0], triangleIndex});
                }

                currentVertexClipped = false;

                if( clipped )
                {
                    _k15_static_buffer_push_back(&localClippedVertices, {intersectionPoint, triangleIndex});
                    clipped = false;
                }

                _k15_get_clipping_flags(edgeVertices, clippingFlags);
                continue;
            }
            else
            {
                const uint8_t clipAnd = clippingFlags[0] & clippingFlags[1];
                if( clipAnd != 0 )
                {
                    continue;
                }

                const uint8_t vertexIndex = clippingFlags[0] == 0 ? 1 : 0;
                const vector4f_t delta = _k15_vector4f_sub(edgeVertices[!vertexIndex], edgeVertices[vertexIndex]);

                currentVertexClipped = clippingFlags[0] != 0u;
                intersectionPoint = edgeVertices[vertexIndex];
                if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Left )
                {
                    const float t = ( -edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].x ) / ( ( -edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].x ) - ( -edgeVertices[!vertexIndex].w - edgeVertices[!vertexIndex].x ) );
                    intersectionPoint.y = intersectionPoint.y + ( edgeVertices[!vertexIndex].y - intersectionPoint.y ) * t;
                    intersectionPoint.z = intersectionPoint.z + ( edgeVertices[!vertexIndex].z - intersectionPoint.z ) * t;
                    intersectionPoint.w = intersectionPoint.w + ( edgeVertices[!vertexIndex].w - intersectionPoint.w ) * t;

                    intersectionPoint.x = -intersectionPoint.w;
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Right )
                {
                    const float t = ( edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].x ) / ( ( edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].x ) - ( edgeVertices[!vertexIndex].w - edgeVertices[!vertexIndex].x ) );
                    intersectionPoint.y = intersectionPoint.y + delta.y * t;
                    intersectionPoint.z = intersectionPoint.z + delta.z * t;
                    intersectionPoint.w = intersectionPoint.w + delta.w * t;

                    intersectionPoint.x = intersectionPoint.w;
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Top )
                {
                    const float t = ( -edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].y ) / ( ( -edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].y ) - ( -edgeVertices[!vertexIndex].w - edgeVertices[!vertexIndex].y ) );
                    intersectionPoint.x = intersectionPoint.x + ( edgeVertices[!vertexIndex].x - intersectionPoint.x ) * t;
                    intersectionPoint.z = intersectionPoint.z + ( edgeVertices[!vertexIndex].z - intersectionPoint.z ) * t;
                    intersectionPoint.w = intersectionPoint.w + ( edgeVertices[!vertexIndex].w - intersectionPoint.w ) * t;

                    intersectionPoint.y = -intersectionPoint.w;
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Bottom )
                {
                    const float t = ( edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].y ) / ( ( edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].y ) - ( edgeVertices[!vertexIndex].w - edgeVertices[!vertexIndex].y ) );
                    intersectionPoint.x = intersectionPoint.x + delta.x * t;
                    intersectionPoint.z = intersectionPoint.z + delta.z * t;
                    intersectionPoint.w = intersectionPoint.w + delta.w * t;

                    intersectionPoint.y = intersectionPoint.w;
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Far )
                {
                    const float t = ( -edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].z ) / ( ( -edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].z ) - ( -edgeVertices[!vertexIndex].w - edgeVertices[!vertexIndex].z ) );
                    intersectionPoint.x = intersectionPoint.x + delta.x * t;
                    intersectionPoint.y = intersectionPoint.y + delta.y * t;
                    intersectionPoint.w = intersectionPoint.w + delta.w * t;

                    intersectionPoint.z = -intersectionPoint.w;
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Near )
                {
                    const float t = ( edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].z ) / ( ( edgeVertices[vertexIndex].w - edgeVertices[vertexIndex].z ) - ( edgeVertices[!vertexIndex].w - edgeVertices[!vertexIndex].z ) );
                    intersectionPoint.x = intersectionPoint.x + delta.x * t;
                    intersectionPoint.y = intersectionPoint.y + delta.y * t;
                    intersectionPoint.w = intersectionPoint.w + delta.w * t;

                    intersectionPoint.z = intersectionPoint.w;
                }

                edgeVertices[vertexIndex] = intersectionPoint;
                _k15_get_clipping_flags(edgeVertices, clippingFlags);
                clipped = true;
                goto clipStart;
            }
        }
    }

    triangle_vertex4f_t* pTriangleVertices = localClippedVertices.pStaticData;
    for(uint32_t clippedVertexIndex = 0; clippedVertexIndex < localClippedVertices.count;)
    {
        const uint32_t remainingVertexCount = localClippedVertices.count - clippedVertexIndex;
        RuntimeAssert(remainingVertexCount >= 3u);
        
        const uint32_t localTriangleIndex = pTriangleVertices[clippedVertexIndex].triangleId;
        if(remainingVertexCount >= 5u &&
           pTriangleVertices[clippedVertexIndex + 1].triangleId == localTriangleIndex &&
           pTriangleVertices[clippedVertexIndex + 2].triangleId == localTriangleIndex &&
           pTriangleVertices[clippedVertexIndex + 3].triangleId == localTriangleIndex &&
           pTriangleVertices[clippedVertexIndex + 4].triangleId == localTriangleIndex)
        {
            triangle4f_t* pTriangles = _k15_dynamic_buffer_push_back(pClippedTriangleBuffer, 3u);
            pTriangles[0] = {pTriangleVertices[clippedVertexIndex + 0].position, pTriangleVertices[clippedVertexIndex + 1].position, pTriangleVertices[clippedVertexIndex + 2].position};
            pTriangles[1] = {pTriangleVertices[clippedVertexIndex + 2].position, pTriangleVertices[clippedVertexIndex + 3].position, pTriangleVertices[clippedVertexIndex + 4].position};
            pTriangles[2] = {pTriangleVertices[clippedVertexIndex + 4].position, pTriangleVertices[clippedVertexIndex + 0].position, pTriangleVertices[clippedVertexIndex + 2].position};

            clippedVertexIndex += 5;
        }
        else if(remainingVertexCount >= 4u && 
           pTriangleVertices[clippedVertexIndex + 1].triangleId == localTriangleIndex &&
           pTriangleVertices[clippedVertexIndex + 2].triangleId == localTriangleIndex &&
           pTriangleVertices[clippedVertexIndex + 3].triangleId == localTriangleIndex)
        {
            triangle4f_t* pTriangles = _k15_dynamic_buffer_push_back(pClippedTriangleBuffer, 2u);
            pTriangles[0] = {pTriangleVertices[clippedVertexIndex + 0].position, pTriangleVertices[clippedVertexIndex + 1].position, pTriangleVertices[clippedVertexIndex + 2].position};
            pTriangles[1] = {pTriangleVertices[clippedVertexIndex + 2].position, pTriangleVertices[clippedVertexIndex + 3].position, pTriangleVertices[clippedVertexIndex + 0].position};

            clippedVertexIndex += 4;
        }
        else if(pTriangleVertices[clippedVertexIndex + 1].triangleId == localTriangleIndex && pTriangleVertices[clippedVertexIndex + 2].triangleId == localTriangleIndex)
        {
            triangle4f_t* pTriangle = _k15_dynamic_buffer_push_back(pClippedTriangleBuffer, 1u);
            pTriangle[0] = {pTriangleVertices[clippedVertexIndex + 0].position, pTriangleVertices[clippedVertexIndex + 1].position, pTriangleVertices[clippedVertexIndex + 2].position};

            clippedVertexIndex += 3;
        }
        else
        {
            RuntimeAssert(false);
        }
    }
}

void k15_project_triangles_into_screenspace(dynamic_buffer_t<triangle4f_t>* restrict_modifier pClippedTriangleBuffer, dynamic_buffer_t<screenspace_triangle2i_t>* restrict_modifier pScreenspaceTriangleBuffer, uint32_t backBufferWidth, uint32_t backBufferHeight)
{
    const float width   = (float)backBufferWidth-1u;
    const float height  = (float)backBufferHeight-1u;

    screenspace_triangle2i_t* pScreenspaceTriangles = _k15_dynamic_buffer_push_back(pScreenspaceTriangleBuffer, pClippedTriangleBuffer->count);

    for(uint32_t triangleIndex = 0; triangleIndex < pClippedTriangleBuffer->count; ++triangleIndex)
    {
        const triangle4f_t* pTriangle = pClippedTriangleBuffer->pData + triangleIndex;
        screenspace_triangle2i_t* pScreenspaceTriangle = pScreenspaceTriangles + triangleIndex;
        RuntimeAssert(pScreenspaceTriangle != nullptr);        

        pScreenspaceTriangle->points[0].x = (int)(((1.0f + pTriangle->points[0].x / pTriangle->points[0].w) / 2.0f) * width);
        pScreenspaceTriangle->points[0].y = (int)(((1.0f + pTriangle->points[0].y / pTriangle->points[0].w) / 2.0f) * height);

        pScreenspaceTriangle->points[1].x = (int)(((1.0f + pTriangle->points[1].x / pTriangle->points[1].w) / 2.0f) * width);
        pScreenspaceTriangle->points[1].y = (int)(((1.0f + pTriangle->points[1].y / pTriangle->points[1].w) / 2.0f) * height);

        pScreenspaceTriangle->points[2].x = (int)(((1.0f + pTriangle->points[2].x / pTriangle->points[2].w) / 2.0f) * width);
        pScreenspaceTriangle->points[2].y = (int)(((1.0f + pTriangle->points[2].y / pTriangle->points[2].w) / 2.0f) * height);

        pScreenspaceTriangle->boundingBox.x1 = get_min(pScreenspaceTriangle->points[0].x, get_min(pScreenspaceTriangle->points[1].x, pScreenspaceTriangle->points[2].x));
        pScreenspaceTriangle->boundingBox.y1 = get_min(pScreenspaceTriangle->points[0].y, get_min(pScreenspaceTriangle->points[1].y, pScreenspaceTriangle->points[2].y));
        pScreenspaceTriangle->boundingBox.y2 = get_max(pScreenspaceTriangle->points[0].y, get_max(pScreenspaceTriangle->points[1].y, pScreenspaceTriangle->points[2].y));
        pScreenspaceTriangle->boundingBox.x2 = get_max(pScreenspaceTriangle->points[0].x, get_max(pScreenspaceTriangle->points[1].x, pScreenspaceTriangle->points[2].x));
    }
}

void k15_draw_text(uint8_t* pColorBuffer, const bitmap_font_t* pFont, uint32_t colorBufferWidth, uint32_t colorBufferHeight, uint32_t colorBufferStride, int32_t x, int32_t y, const char* pText)
{
    RuntimeAssert(pFont->channels == 3);

    const uint32_t charPixelWidth = 16;
    const uint32_t charPixelHeight = 16;
    const uint32_t textLength = (uint32_t)strlen(pText);
    const uint32_t fontBitmapWidth = pFont->width;
    const char* pTextEnd = pText + textLength;
    const char* pTextRunning = pText;
    uint32_t colorBufferPixelX = x;

    while( pTextRunning != pTextEnd )
    {
        const uint32_t charIndex = (uint32_t)(pTextRunning - pText);
        const char charToDraw = *pTextRunning++;

        const uint32_t charPixelStartX = ( charPixelWidth * charToDraw ) & 0xFF;
        const uint32_t charPixelStartY = ( ( charPixelWidth * charToDraw ) / 0xFF ) * charPixelHeight;
        const uint32_t charPixelEndX = get_min( colorBufferWidth, charPixelStartX + 16 );
        const uint32_t charPixelEndY = get_min( colorBufferHeight, charPixelStartY + 16 );

        uint32_t charPixelY = charPixelStartY;
        uint32_t charPixelX = charPixelStartX;
        uint32_t colorBufferPixelY = y;

        while( charPixelY < charPixelEndY )
        {
            charPixelX = charPixelStartX;
            colorBufferPixelX = x + charIndex * charPixelWidth;
            while( charPixelX < charPixelEndX )
            {
                const uint32_t colorBufferIndex = ( colorBufferPixelX + colorBufferPixelY * colorBufferStride ) * 4;
                const uint32_t fontBufferIndex = ( charPixelX + charPixelY * fontBitmapWidth ) * 3;
                pColorBuffer[colorBufferIndex + 0] = pFont->pRGBPixels[fontBufferIndex + 0];
                pColorBuffer[colorBufferIndex + 1] = pFont->pRGBPixels[fontBufferIndex + 1];
                pColorBuffer[colorBufferIndex + 2] = pFont->pRGBPixels[fontBufferIndex + 2];
                pColorBuffer[colorBufferIndex + 3] = 0xFF;

                ++charPixelX;
                ++colorBufferPixelX;
            }
            
            ++charPixelY;
            ++colorBufferPixelY;
        }
    }
}

void k15_draw_text_formatted(uint8_t* pColorBuffer, const bitmap_font_t* pFont, uint32_t colorBufferWidth, uint32_t colorBufferHeight, uint32_t colorBufferStride, int32_t x, int32_t y, const char* pText, ...)
{
    char textBuffer[512];

    va_list args;
    va_start(args, pText);
    vsprintf(textBuffer, pText, args);
    va_end(args);

    k15_draw_text(pColorBuffer, pFont, colorBufferWidth, colorBufferHeight, colorBufferStride, x, y, textBuffer);
}

void k15_draw_frame(software_rasterizer_context_t* pContext)
{
    k15_transform_triangles(&pContext->projectionMatrix, &pContext->viewMatrix, &pContext->triangles);
    k15_cull_triangles(&pContext->triangles, &pContext->visibleTriangles, pContext->settings.backFaceCullingEnabled);
    k15_clip_triangles(&pContext->visibleTriangles, &pContext->clippedTriangles);
    k15_project_triangles_into_screenspace(&pContext->clippedTriangles, &pContext->screenSpaceTriangles, pContext->backBufferWidth, pContext->backBufferHeight);
    k15_draw_triangles(pContext->pColorBuffer[pContext->currentColorBufferIndex], pContext->backBufferStride, &pContext->screenSpaceTriangles);

    pContext->triangles.count = 0;
    pContext->visibleTriangles.count = 0;
    pContext->clippedTriangles.count = 0;
    pContext->screenSpaceTriangles.count = 0;
}

void k15_change_color_buffers(software_rasterizer_context_t* pContext, void** restrict_modifier pColorBuffers, uint8_t colorBufferCount, uint32_t widthInPixels, uint32_t heightInPixels, uint32_t strideInBytes)
{
    for(uint8_t colorBufferIndex = 0; colorBufferIndex < colorBufferCount; ++colorBufferIndex)
    {
        pContext->pColorBuffer[colorBufferIndex] = pColorBuffers[colorBufferIndex];
    }

    pContext->colorBufferCount = colorBufferCount;
    pContext->backBufferWidth = widthInPixels;
    pContext->backBufferHeight = heightInPixels;
    pContext->backBufferStride = strideInBytes;
}

bool k15_begin_geometry(software_rasterizer_context_t* pContext, topology_t topology)
{
    RuntimeAssertMsg(pContext->currentTriangleVertexIndex == 0, "Ill defined triangle");
    pContext->currentTopology = topology;
    return true;
}

void k15_end_geometry(software_rasterizer_context_t* pContext)
{
    RuntimeAssertMsg(pContext->currentTopology != topology_t::none, "Didn't call k15_begin_geometry before.");
    RuntimeAssertMsg(pContext->currentTriangleVertexIndex == 3, "Ill defined triangle");
    pContext->currentTopology = topology_t::none;
    pContext->currentTriangleVertexIndex = 0;
}

void k15_set_camera_pos(software_rasterizer_context_t* pContext, const vector3f_t& cameraPos)
{
    pContext->viewMatrix.m03 = -cameraPos.x;
    pContext->viewMatrix.m13 = -cameraPos.y;
    pContext->viewMatrix.m23 = -cameraPos.z;
}

void k15_vertex_position(software_rasterizer_context_t* pContext, float x, float y, float z)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssertMsg(pContext->currentTopology != topology_t::none, "Didn't call k15_being_geometry before.");

    triangle4f_t* pCurrentTriangle = pContext->triangles.pData + (pContext->triangles.count - 1u);

    if(pContext->currentTriangleVertexIndex == 0u || pContext->currentTriangleVertexIndex == 3u)
    {
        pCurrentTriangle = _k15_dynamic_buffer_push_back(&pContext->triangles, 1u);
        RuntimeAssert(pCurrentTriangle != nullptr);

        pContext->currentTriangleVertexIndex = 0;
    }

    pCurrentTriangle->points[pContext->currentTriangleVertexIndex++] = k15_create_vector4f(x, y, z, 1.0f);
}

#endif //K15_SOFTWARE_RASTERIZER_IMPLEMENTATION
#endif //K15_SOFTWARE_RASTERIZER_INCLUDE