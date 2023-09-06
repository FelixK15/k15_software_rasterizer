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

struct vector2f_t
{
    float x, y;
};

struct vector2u_t
{
    uint32_t x, y;
};

struct bounding_box_t
{
    uint32_t x1, x2, y1, y2;
};

struct vertex_buffer_handle_t
{
    void* pHandle;
};

struct texture_handle_t
{
    void* pHandle;
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

extern vertex_buffer_handle_t   k15_invalid_vertex_buffer_handle;
extern texture_handle_t         k15_invalid_texture_handle;

bool                    k15_create_software_rasterizer_context(software_rasterizer_context_t** pOutContextPtr, const software_rasterizer_context_init_parameters_t* pParameters);

void                    k15_swap_color_buffers(software_rasterizer_context_t* pContext);
void                    k15_draw_frame(software_rasterizer_context_t* pContext);

void                    k15_change_color_buffers(software_rasterizer_context_t* pContext, void* pColorBuffers[3], uint8_t colorBufferCount, uint32_t widthInPixels, uint32_t heightInPixels, uint32_t strideInBytes);

bool                    k15_is_valid_vertex_buffer(const vertex_buffer_handle_t vertexBuffer);
bool                    k15_is_valid_texture(const texture_handle_t texture);

vertex_buffer_handle_t k15_create_vertex_buffer(software_rasterizer_context_t* pContext, uint32_t vertexSizeInBytes, const void* pVertexData, uint32_t vertexDataSizeInBytes);
texture_handle_t       k15_create_texture(software_rasterizer_context_t* pContext, uint32_t width, uint32_t height, uint32_t stride, uint8_t componentCount, const void* pTextureData);

void k15_bind_vertex_buffer(software_rasterizer_context_t* pContext, vertex_buffer_handle_t vertexBuffer);
void k15_bind_texture(software_rasterizer_context_t* pContext, texture_handle_t texture, uint32_t slot);
void k15_bind_model_matrix(software_rasterizer_context_t* pContext, const matrix4x4f_t* pModelMatrix);
void k15_bind_view_matrix(software_rasterizer_context_t* pContext, const matrix4x4f_t* pViewMatrix);
bool k15_draw(software_rasterizer_context_t* pContext, uint32_t vertexCount);

uint32_t mouseX = 0;
uint32_t mouseY = 0;

#ifdef K15_SOFTWARE_RASTERIZER_IMPLEMENTATION

#ifdef _MSC_BUILD
#include <intrin.h>
#define restrict_modifier       __restrict
#define RuntimeAssert(x)        if(!(x)){  __debugbreak(); }
#define BreakpointHook()        __nop()
#else
#warning No support for this compiler
#define restrict_modifier
#define RuntimeAssert(x)
#define CompiletimeAssert(x)
#define BreakpointHook()
#endif

#define CompiletimeAssert(x)        static_assert(x)
#define RuntimeAssertMsg(x, msg)    RuntimeAssert(x)
#define UnusedVariable(x)           (void)(x)

#define get_min(a,b) (a)>(b)?(b):(a)
#define get_max(a,b) (a)>(b)?(a):(b)

constexpr uint32_t MaxColorBuffer = 3u;
constexpr uint32_t DefaultTriangleBufferCapacity    = 1024u;
constexpr uint32_t DefaultVertexBufferCapacity      = 64u;
constexpr uint32_t DefaultTextureCapacity           = 256u;
constexpr uint32_t DefaultDrawCallCapacity          = 512u;

constexpr uint32_t DrawCallMaxVertexBuffer          = 4u;
constexpr uint32_t DrawCallMaxTextures              = 4u;

constexpr float pi = 3.141f;

constexpr matrix4x4f_t IdentityMatrix4x4[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

vertex_buffer_handle_t  k15_invalid_vertex_buffer_handle    = {nullptr};
texture_handle_t        k15_invalid_texture_handle          = {nullptr};

enum vertex_id_t : uint32_t
{
    position,
    color,

    count
};

struct vertex_t
{
    vector4f_t position;
    vector3f_t normal;
    vector2f_t texcoord;
};

struct vertex_buffer_t
{
    const vertex_t* pData;
    uint32_t vertexCount;
};

struct texture_t
{
    const void* pData;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t componentCount;
};

struct draw_call_t
{
    matrix4x4f_t        modelMatrix;
    vertex_buffer_t*    pVertexBuffer;
    texture_t*          textures[DrawCallMaxTextures];
    uint32_t            vertexCount;
    uint32_t            vertexOffset;
};

struct clipped_vertex_t : vertex_t
{
    uint32_t triangleIndex;
    const draw_call_t* pDrawCall;
};

struct triangle4f_t
{
    vertex_t vertices[3];
    const draw_call_t* pDrawCall;
};

struct screenspace_triangle2i_t
{
    vector3f_t      normals[3];
    vector2f_t      positions[3];
    vector2f_t      texcoords[3];
    bounding_box_t  boundingBox;

    const draw_call_t* pDrawCall;
};

template<typename T>
struct base_static_buffer_t
{
protected:
    base_static_buffer_t() 
    {
    };

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
        capacity = SIZE;
        count = 0;
    }

    T data[SIZE];
};

struct clip_vertices_t
{
    dynamic_buffer_t<float> components;
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

    uint32_t                                    backBufferWidth;
    uint32_t                                    backBufferHeight;
    uint32_t                                    backBufferStride;

    clip_vertices_t                             clipVertices;

    void*                                       pColorBuffer[MaxColorBuffer];

    vertex_buffer_t*                            pBoundVertexBuffer;
    texture_t*                                  boundTextures[DrawCallMaxTextures];

    dynamic_buffer_t<draw_call_t>               drawCalls;

    dynamic_buffer_t<vertex_buffer_t>           vertexBuffers;
    dynamic_buffer_t<texture_t>                 textures;  

    dynamic_buffer_t<triangle4f_t>              triangles;
    dynamic_buffer_t<triangle4f_t>              visibleTriangles;
    dynamic_buffer_t<triangle4f_t>              clippedTriangles;
    dynamic_buffer_t<screenspace_triangle2i_t>  screenSpaceTriangles;

    matrix4x4f_t                                projectionMatrix;
    matrix4x4f_t                                viewMatrix;
    matrix4x4f_t                                modelMatrix;
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
void _k15_draw_line(void* pColorBuffer, uint32_t colorBufferStride, const vector2f_t* pVectorA, const vector2f_t* pVectorB, uint32_t color)
{
    int x = (int)pVectorA->x;
    int y = (int)pVectorA->y;
    int x2 = (int)pVectorB->x;
    int y2 = (int)pVectorB->y;

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

template<typename T>
bool k15_is_in_range_inclusive(T value, T min, T max)
{
    return value >= min && value <= max;
}

vector3f_t _k15_vector3f_add(vector3f_t a, vector3f_t b)
{
    vector3f_t vector;
    vector.x = a.x + b.x;
    vector.y = a.y + b.y;
    vector.z = a.z + b.z;

    return vector;
}

vector2f_t _k15_vector2f_sub(vector2f_t a, vector2f_t b)
{
    vector2f_t vector;
    vector.x = a.x - b.x;
    vector.y = a.y - b.y;

    return vector;
}

float _k15_vector2f_dot(vector2f_t a, vector2f_t b)
{
    return a.x * b.x + a.y * b.y;
}

vector4f_t k15_create_vector4f(float x, float y, float z, float w)
{
    return {x, y, z, w};
}

vector3f_t k15_create_vector3f(float x, float y, float z)
{
    return {x, y, z};
}

vector2f_t k15_create_vector2f(float x, float y)
{
    return {x, y};
}

void k15_draw_triangles(void* pColorBuffer, uint32_t colorBufferStride, const dynamic_buffer_t<screenspace_triangle2i_t>* pScreenSpaceTriangleBuffer)
{
    uint32_t* pColorBufferContent = (uint32_t*)pColorBuffer;
    for(uint32_t triangleIndex = 0; triangleIndex < pScreenSpaceTriangleBuffer->count; ++triangleIndex)
    {
        const screenspace_triangle2i_t* pTriangle = pScreenSpaceTriangleBuffer->pData + triangleIndex;

#if 0
        _k15_draw_line(pColorBuffer, colorBufferStride, &pTriangle->points[0], &pTriangle->points[1], 0xFFFFFFFF);
        _k15_draw_line(pColorBuffer, colorBufferStride, &pTriangle->points[1], &pTriangle->points[2], 0xFFFFFFFF);
        _k15_draw_line(pColorBuffer, colorBufferStride, &pTriangle->points[2], &pTriangle->points[0], 0xFFFFFFFF);
        
        const vector2f_t a = k15_create_vector2f((float)pTriangle->boundingBox.x1, (float)pTriangle->boundingBox.y1);
        const vector2f_t b = k15_create_vector2f((float)pTriangle->boundingBox.x2, (float)pTriangle->boundingBox.y1);
        const vector2f_t c = k15_create_vector2f((float)pTriangle->boundingBox.x1, (float)pTriangle->boundingBox.y2);
        const vector2f_t d = k15_create_vector2f((float)pTriangle->boundingBox.x2, (float)pTriangle->boundingBox.y2);
       
        _k15_draw_line(pColorBuffer, colorBufferStride, &a, &b, 0xFFFFFFFF);
        _k15_draw_line(pColorBuffer, colorBufferStride, &b, &d, 0xFFFFFFFF);
        _k15_draw_line(pColorBuffer, colorBufferStride, &d, &c, 0xFFFFFFFF);
        _k15_draw_line(pColorBuffer, colorBufferStride, &c, &a, 0xFFFFFFFF);
#elif 1

#if 0
        _k15_draw_line(pColorBuffer, colorBufferStride, &pTriangle->positions[0], &pTriangle->positions[1], 0xFFFFFFFF);
        _k15_draw_line(pColorBuffer, colorBufferStride, &pTriangle->positions[1], &pTriangle->positions[2], 0xFFFFFFFF);
        _k15_draw_line(pColorBuffer, colorBufferStride, &pTriangle->positions[2], &pTriangle->positions[0], 0xFFFFFFFF);
#endif
        for(uint32_t y = pTriangle->boundingBox.y1; y < pTriangle->boundingBox.y2; ++y)
        {
            for(uint32_t x = pTriangle->boundingBox.x1; x < pTriangle->boundingBox.x2; ++x)
            {   
                const vector2f_t pixel = {(float)x, (float)y};
                const vector2f_t v0 = _k15_vector2f_sub(pTriangle->positions[1], pTriangle->positions[0]);
                const vector2f_t v1 = _k15_vector2f_sub(pTriangle->positions[2], pTriangle->positions[0]);
                const vector2f_t v2 = _k15_vector2f_sub(pixel, pTriangle->positions[0]);

                const float d00 = _k15_vector2f_dot(v0, v0);
                const float d01 = _k15_vector2f_dot(v0, v1);
                const float d11 = _k15_vector2f_dot(v1, v1);
                const float d20 = _k15_vector2f_dot(v2, v0);
                const float d21 = _k15_vector2f_dot(v2, v1);
                const float denom = d00 * d11 - d01 * d01;
                const float v = (d11 * d20 - d01 * d21) / denom;
                const float w = (d00 * d21 - d01 * d20) / denom;
                const float u = 1.0f - v - w;

                if( !k15_is_in_range_inclusive(v, 0.0f, 1.0f) ||
                    !k15_is_in_range_inclusive(w, 0.0f, 1.0f) ||
                    !k15_is_in_range_inclusive(u, 0.0f, 1.0f) )
                {
                    continue;
                }

                const vector2f_t bary_uv = { pTriangle->texcoords[0].x * u + pTriangle->texcoords[1].x * v + pTriangle->texcoords[2].x * w,
                    pTriangle->texcoords[0].y * u + pTriangle->texcoords[1].y * v + pTriangle->texcoords[2].y * w };

                texture_t* pTexture = pTriangle->pDrawCall->textures[0];
                uint32_t imageX = (uint32_t)(bary_uv.x * pTexture->width);
                uint32_t imageY = (uint32_t)(bary_uv.y * pTexture->height);

                const uint8_t* pTextureData = (const uint8_t*)pTexture->pData;
                uint8_t red    = pTextureData[(imageX + imageY * pTexture->stride) * 3 + 0];
                uint8_t green  = pTextureData[(imageX + imageY * pTexture->stride) * 3 + 1];
                uint8_t blue   = pTextureData[(imageX + imageY * pTexture->stride) * 3 + 2];

                uint32_t color = 0xFF000000 | red << 16 | green << 8 | blue;
                uint32_t pixelIndex = (uint32_t)pixel.x + (uint32_t)pixel.y * colorBufferStride;
                pColorBufferContent[pixelIndex] = color;
            }
        }
#endif
    }
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
    RuntimeAssert(pStaticBuffer->count + elementCount < bufferCapacity);

    const uint32_t oldCount = pStaticBuffer->count;
    pStaticBuffer->count += elementCount;
    return pStaticBuffer->pStaticData + oldCount;
}

template<typename T>
T* _k15_static_buffer_push_back(base_static_buffer_t<T>* pStaticBuffer, T value)
{
    T* pValue = _k15_static_buffer_push_back(pStaticBuffer, 1u);
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
    pContext->backBufferHeight              = pParameters->backBufferHeight;
    pContext->backBufferWidth               = pParameters->backBufferWidth;
    pContext->backBufferStride              = pParameters->backBufferStride;
    pContext->currentColorBufferIndex       = 0;
    pContext->currentTriangleVertexIndex    = 0;
    pContext->pBoundVertexBuffer            = nullptr;
    memcpy(&pContext->modelMatrix, &IdentityMatrix4x4, sizeof(pContext->modelMatrix));

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

    if(!_k15_create_dynamic_buffer<vertex_buffer_t>(&pContext->vertexBuffers, DefaultVertexBufferCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<texture_t>(&pContext->textures, DefaultTextureCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<draw_call_t>(&pContext->drawCalls, DefaultDrawCallCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<float>(&pContext->clipVertices.components, DefaultTriangleBufferCapacity))
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

void k15_transform_triangles(const matrix4x4f_t* restrict_modifier pViewProjectionMatrix, const matrix4x4f_t* restrict_modifier pModelMatrix, triangle4f_t* pTriangles, uint32_t triangleCount)
{
    matrix4x4f_t positionMatrix     = _k15_mul_matrix4x4f(pViewProjectionMatrix, pModelMatrix);
    //matrix4x4f_t invPositionMatrix  = _k15_inverse_matrix4x4f(&positionMatrix);
    matrix4x4f_t normalMatrix       = positionMatrix;
    
    for(uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
    {
        triangle4f_t* pTriangle = pTriangles + triangleIndex;
        pTriangle->vertices[0].position = _k15_mul_vector4_matrix44(&pTriangle->vertices[0].position, &positionMatrix);
        pTriangle->vertices[1].position = _k15_mul_vector4_matrix44(&pTriangle->vertices[1].position, &positionMatrix);
        pTriangle->vertices[2].position = _k15_mul_vector4_matrix44(&pTriangle->vertices[2].position, &positionMatrix);

        vector4f_t normals[] = {
            k15_create_vector4f(pTriangle->vertices[0].normal.x, pTriangle->vertices[0].normal.y, pTriangle->vertices[0].normal.z, 0.0f),
            k15_create_vector4f(pTriangle->vertices[1].normal.x, pTriangle->vertices[1].normal.y, pTriangle->vertices[1].normal.z, 0.0f),
            k15_create_vector4f(pTriangle->vertices[2].normal.x, pTriangle->vertices[2].normal.y, pTriangle->vertices[2].normal.z, 0.0f)
        };

        normals[0] = _k15_mul_vector4_matrix44(&normals[0], &normalMatrix);
        normals[1] = _k15_mul_vector4_matrix44(&normals[1], &normalMatrix);
        normals[2] = _k15_mul_vector4_matrix44(&normals[2], &normalMatrix);

        pTriangle->vertices[0].normal.x = normals[0].x;
        pTriangle->vertices[0].normal.y = normals[0].y;
        pTriangle->vertices[0].normal.z = normals[0].z;

        pTriangle->vertices[1].normal.x = normals[1].x;
        pTriangle->vertices[1].normal.y = normals[1].y;
        pTriangle->vertices[1].normal.z = normals[1].z;

        pTriangle->vertices[2].normal.x = normals[2].x;
        pTriangle->vertices[2].normal.y = normals[2].y;
        pTriangle->vertices[2].normal.z = normals[2].z;
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
        const vector4f_t ab = _k15_vector4f_sub(pTriangle->vertices[1].position, pTriangle->vertices[0].position);
        const vector4f_t ac = _k15_vector4f_sub(pTriangle->vertices[2].position, pTriangle->vertices[0].position);
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
        const bool triangleOutsideFrustum = _k15_is_position_outside_frustum(pTriangle->vertices[0].position) && 
        _k15_is_position_outside_frustum(pTriangle->vertices[1].position) && 
        _k15_is_position_outside_frustum(pTriangle->vertices[2].position);

        if(!triangleOutsideFrustum)
        {
            if(APPLY_BACKFACE_CULLING)
            {
                const vector4f_t ab = _k15_vector4f_sub(pTriangle->vertices[1].position, pTriangle->vertices[0].position);
                const vector4f_t ac = _k15_vector4f_sub(pTriangle->vertices[2].position, pTriangle->vertices[0].position);
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

void _k15_get_clipping_flags(const vertex_t* ppPoints, uint8_t* ppClippingFlags)
{
    for( uint8_t pointIndex = 0; pointIndex < 2u; ++pointIndex )
    {
        const vector4f_t point = ppPoints[pointIndex].position;
        uint8_t* pClippingFlags = ppClippingFlags + pointIndex;

        const float posW = point.w;
        const float negW = -posW;

        *pClippingFlags = 0u;

        if( point.x < negW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Left;
        }
        else if( point.x > posW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Right;
        }

        if( point.y < negW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Top;
        }
        else if( point.y > posW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Bottom;
        }

        if( point.z < negW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Far;
        }
        else if( point.z > posW )
        {
            *pClippingFlags |= (uint8_t)clip_flag_t::Near;
        }
    }
}

vertex_t k15_interpolate_vertex(const vertex_t* pStart, const vertex_t* pEnd, float t)
{
    vertex_t interpolatedVertex = {};
    interpolatedVertex.position.x = pStart->position.x + (pEnd->position.x - pStart->position.x) * t;
    interpolatedVertex.position.y = pStart->position.y + (pEnd->position.y - pStart->position.y) * t;
    interpolatedVertex.position.z = pStart->position.z + (pEnd->position.z - pStart->position.z) * t;
    interpolatedVertex.position.w = pStart->position.w + (pEnd->position.w - pStart->position.w) * t;

    interpolatedVertex.normal.x = pStart->normal.x + (pEnd->normal.x - pStart->normal.x) * t;
    interpolatedVertex.normal.y = pStart->normal.y + (pEnd->normal.y - pStart->normal.y) * t;
    interpolatedVertex.normal.z = pStart->normal.z + (pEnd->normal.z - pStart->normal.z) * t;

    interpolatedVertex.texcoord.x = pStart->texcoord.x + (pEnd->texcoord.x - pStart->texcoord.x) * t;
    interpolatedVertex.texcoord.y = pStart->texcoord.y + (pEnd->texcoord.y - pStart->texcoord.y) * t;

    return interpolatedVertex;
}

clipped_vertex_t _k15_create_clipped_vertex(const vertex_t* pVertex, uint32_t triangleIndex, const draw_call_t* pDrawCall)
{
    clipped_vertex_t clippedVertex;
    memcpy(&clippedVertex, pVertex, sizeof(vertex_t));
    clippedVertex.triangleIndex = triangleIndex;
    clippedVertex.pDrawCall     = pDrawCall;

    return clippedVertex;
}

void k15_clip_triangles(dynamic_buffer_t<triangle4f_t>* restrict_modifier pVisibleTriangles, dynamic_buffer_t<triangle4f_t>* pClippedTriangles)
{
    const uint32_t triangleCount = pVisibleTriangles->count;
    static_buffer_t<clipped_vertex_t, 128u> localClippedVertices;

    for(uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
    {
        const triangle4f_t* pCurrentTriangle = pVisibleTriangles->pData + triangleIndex;

        bool clipped = false;
        bool currentVertexClipped = false;
        vertex_t intersectionVertex = {};
        for(uint32_t vertexIndex = 0; vertexIndex < 3u; ++vertexIndex)
        {
            const uint32_t nextVertexIndex = ( vertexIndex + 1u ) % 3u;

            vertex_t edgeVertices[2] = {
                pCurrentTriangle->vertices[vertexIndex],
                pCurrentTriangle->vertices[nextVertexIndex]
            };

            uint8_t clippingFlags[2] = {0};
            _k15_get_clipping_flags(edgeVertices, clippingFlags);

        clipStart:
            if( ( clippingFlags[0] | clippingFlags[1] ) == 0 )
            {
                if( !currentVertexClipped )
                {
                    _k15_static_buffer_push_back(&localClippedVertices, _k15_create_clipped_vertex(&edgeVertices[0], triangleIndex, pCurrentTriangle->pDrawCall));
                }

                currentVertexClipped = false;

                if( clipped )
                {
                    _k15_static_buffer_push_back(&localClippedVertices, _k15_create_clipped_vertex(&intersectionVertex, triangleIndex, pCurrentTriangle->pDrawCall));
                    clipped = false;
                }
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
                const vector4f_t delta = _k15_vector4f_sub(edgeVertices[!vertexIndex].position, edgeVertices[vertexIndex].position);

                currentVertexClipped = clippingFlags[0] != 0u;
                intersectionVertex = edgeVertices[vertexIndex];
                if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Left )
                {
                    const float t = ( -edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.x ) / ( ( -edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.x ) - ( -edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.x ) );
                    intersectionVertex = k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Right )
                {
                    const float t = ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.x ) / ( ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.x ) - ( edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.x ) );
                    intersectionVertex = k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Top )
                {
                    const float t = ( -edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.y ) / ( ( -edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.y ) - ( -edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.y ) );
                    intersectionVertex = k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Bottom )
                {
                    const float t = ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.y ) / ( ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.y ) - ( edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.y ) );
                    intersectionVertex = k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Far )
                {
                    const float t = ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.y ) / ( ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.y ) - ( edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.y ) );
                    intersectionVertex = k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Near )
                {
                    const float t = ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.z ) / ( ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.z ) - ( edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.z ) );
                    intersectionVertex = k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }

                edgeVertices[vertexIndex] = intersectionVertex;
                _k15_get_clipping_flags(edgeVertices, clippingFlags);
                clipped = true;
                goto clipStart;
            }
        }
    }

    if(localClippedVertices.count < 3u)
    {
        return;
    }

    clipped_vertex_t* pTriangleVertices = localClippedVertices.pStaticData;
    for(uint32_t clippedVertexIndex = 0; clippedVertexIndex < localClippedVertices.count;)
    {
        const uint32_t remainingVertexCount = localClippedVertices.count - clippedVertexIndex;
        RuntimeAssert(remainingVertexCount >= 3u);
        
        const uint32_t localTriangleIndex = pTriangleVertices[clippedVertexIndex].triangleIndex;
        if(remainingVertexCount >= 5u &&
           pTriangleVertices[clippedVertexIndex + 1].triangleIndex == localTriangleIndex &&
           pTriangleVertices[clippedVertexIndex + 2].triangleIndex == localTriangleIndex &&
           pTriangleVertices[clippedVertexIndex + 3].triangleIndex == localTriangleIndex &&
           pTriangleVertices[clippedVertexIndex + 4].triangleIndex == localTriangleIndex)
        {
            triangle4f_t* pTriangles = _k15_dynamic_buffer_push_back(pClippedTriangles, 3u);
            pTriangles[0] = {pTriangleVertices[clippedVertexIndex + 0], pTriangleVertices[clippedVertexIndex + 1], pTriangleVertices[clippedVertexIndex + 2], pTriangleVertices[0].pDrawCall};
            pTriangles[1] = {pTriangleVertices[clippedVertexIndex + 2], pTriangleVertices[clippedVertexIndex + 3], pTriangleVertices[clippedVertexIndex + 4], pTriangleVertices[0].pDrawCall};
            pTriangles[2] = {pTriangleVertices[clippedVertexIndex + 4], pTriangleVertices[clippedVertexIndex + 0], pTriangleVertices[clippedVertexIndex + 2], pTriangleVertices[0].pDrawCall};
            
            clippedVertexIndex += 5;
        }
        else if(remainingVertexCount >= 4u && 
           pTriangleVertices[clippedVertexIndex + 1].triangleIndex == localTriangleIndex &&
           pTriangleVertices[clippedVertexIndex + 2].triangleIndex == localTriangleIndex &&
           pTriangleVertices[clippedVertexIndex + 3].triangleIndex == localTriangleIndex)
        {
            triangle4f_t* pTriangles = _k15_dynamic_buffer_push_back(pClippedTriangles, 2u);
            pTriangles[0] = {pTriangleVertices[clippedVertexIndex + 0], pTriangleVertices[clippedVertexIndex + 1], pTriangleVertices[clippedVertexIndex + 2], pTriangleVertices[0].pDrawCall};
            pTriangles[1] = {pTriangleVertices[clippedVertexIndex + 2], pTriangleVertices[clippedVertexIndex + 3], pTriangleVertices[clippedVertexIndex + 0], pTriangleVertices[0].pDrawCall};

            clippedVertexIndex += 4;
        }
        else if(pTriangleVertices[clippedVertexIndex + 1].triangleIndex == localTriangleIndex && pTriangleVertices[clippedVertexIndex + 2].triangleIndex == localTriangleIndex)
        {
            triangle4f_t* pTriangle = _k15_dynamic_buffer_push_back(pClippedTriangles, 1u);
            pTriangle[0] = {pTriangleVertices[clippedVertexIndex + 0], pTriangleVertices[clippedVertexIndex + 1], pTriangleVertices[clippedVertexIndex + 2], pTriangleVertices[0].pDrawCall};

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

        pScreenspaceTriangle->positions[0].x = (((1.0f + pTriangle->vertices[0].position.x / pTriangle->vertices[0].position.w) / 2.0f) * width);
        pScreenspaceTriangle->positions[0].y = (((1.0f + pTriangle->vertices[0].position.y / pTriangle->vertices[0].position.w) / 2.0f) * height);

        pScreenspaceTriangle->positions[1].x = (((1.0f + pTriangle->vertices[1].position.x / pTriangle->vertices[1].position.w) / 2.0f) * width);
        pScreenspaceTriangle->positions[1].y = (((1.0f + pTriangle->vertices[1].position.y / pTriangle->vertices[1].position.w) / 2.0f) * height);

        pScreenspaceTriangle->positions[2].x = (((1.0f + pTriangle->vertices[2].position.x / pTriangle->vertices[2].position.w) / 2.0f) * width);
        pScreenspaceTriangle->positions[2].y = (((1.0f + pTriangle->vertices[2].position.y / pTriangle->vertices[2].position.w) / 2.0f) * height);

        pScreenspaceTriangle->normals[0] = pTriangle->vertices[0].normal;
        pScreenspaceTriangle->normals[1] = pTriangle->vertices[1].normal;
        pScreenspaceTriangle->normals[2] = pTriangle->vertices[2].normal;

        pScreenspaceTriangle->texcoords[0] = pTriangle->vertices[0].texcoord;
        pScreenspaceTriangle->texcoords[1] = pTriangle->vertices[1].texcoord;
        pScreenspaceTriangle->texcoords[2] = pTriangle->vertices[2].texcoord;

        pScreenspaceTriangle->boundingBox.x1 = (uint32_t)(get_min(pScreenspaceTriangle->positions[0].x, get_min(pScreenspaceTriangle->positions[1].x, pScreenspaceTriangle->positions[2].x)));
        pScreenspaceTriangle->boundingBox.y1 = (uint32_t)(get_min(pScreenspaceTriangle->positions[0].y, get_min(pScreenspaceTriangle->positions[1].y, pScreenspaceTriangle->positions[2].y)));
        pScreenspaceTriangle->boundingBox.y2 = (uint32_t)(get_max(pScreenspaceTriangle->positions[0].y, get_max(pScreenspaceTriangle->positions[1].y, pScreenspaceTriangle->positions[2].y)));
        pScreenspaceTriangle->boundingBox.x2 = (uint32_t)(get_max(pScreenspaceTriangle->positions[0].x, get_max(pScreenspaceTriangle->positions[1].x, pScreenspaceTriangle->positions[2].x)));

        pScreenspaceTriangle->pDrawCall = pTriangle->pDrawCall;
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

triangle4f_t* k15_generate_triangles(dynamic_buffer_t<triangle4f_t>* pTriangleBuffer, const draw_call_t* pDrawCall, uint32_t triangleCount)
{
    const uint32_t vertexCount = triangleCount * 3u;
    triangle4f_t* pTriangles = _k15_dynamic_buffer_push_back(pTriangleBuffer, triangleCount);
    if(pTriangles == nullptr)
    {
        return nullptr;
    }

    for( uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex )
    {
        const uint32_t vertexBufferOffset = triangleIndex * 3u;
        memcpy(pTriangles[triangleIndex].vertices, pDrawCall->pVertexBuffer->pData + vertexBufferOffset, sizeof(vertex_t) * 3);
        pTriangles[triangleIndex].pDrawCall = pDrawCall;
    }

    return pTriangles;
}

void k15_transform_triangles_from_draw_calls(dynamic_buffer_t<draw_call_t>* restrict_modifier pDrawCalls, dynamic_buffer_t<triangle4f_t>* restrict_modifier pTriangleBuffer, const matrix4x4f_t* restrict_modifier pProjectionMatrix, const matrix4x4f_t* restrict_modifier pViewMatrix)
{
    matrix4x4f_t viewProjectionMatrix = _k15_mul_matrix4x4f(pProjectionMatrix, pViewMatrix);
    for( uint32_t drawCallIndex = 0; drawCallIndex < pDrawCalls->count; ++drawCallIndex)
    {
        const draw_call_t* pDrawCall = pDrawCalls->pData + drawCallIndex;
        const uint32_t triangleCount = pDrawCall->vertexCount / 3u;

        triangle4f_t* pTriangles = k15_generate_triangles(pTriangleBuffer, pDrawCall, triangleCount);
        RuntimeAssert(pTriangles != nullptr);

        k15_transform_triangles(&viewProjectionMatrix, &pDrawCall->modelMatrix, pTriangles, triangleCount);
    }
}

void k15_draw_frame(software_rasterizer_context_t* pContext)
{
    k15_transform_triangles_from_draw_calls(&pContext->drawCalls, &pContext->triangles, &pContext->projectionMatrix, &pContext->viewMatrix);
    k15_cull_triangles(&pContext->triangles, &pContext->visibleTriangles, pContext->settings.backFaceCullingEnabled);
    k15_clip_triangles(&pContext->visibleTriangles, &pContext->clippedTriangles);
    k15_project_triangles_into_screenspace(&pContext->clippedTriangles, &pContext->screenSpaceTriangles, pContext->backBufferWidth, pContext->backBufferHeight);
    k15_draw_triangles(pContext->pColorBuffer[pContext->currentColorBufferIndex], pContext->backBufferStride, &pContext->screenSpaceTriangles);

    pContext->triangles.count = 0;
    pContext->visibleTriangles.count = 0;
    pContext->clippedTriangles.count = 0;
    pContext->screenSpaceTriangles.count = 0;
    pContext->drawCalls.count = 0;
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

bool k15_is_valid_vertex_buffer(const vertex_buffer_handle_t vertexBuffer)
{
    return vertexBuffer.pHandle != nullptr;
}

bool k15_is_valid_texture(const texture_handle_t texture)
{
    return texture.pHandle != nullptr;
}

vertex_buffer_handle_t k15_create_vertex_buffer(software_rasterizer_context_t* pContext, const vertex_t* pVertexData, uint32_t vertexCount)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(pVertexData != nullptr);
    RuntimeAssert(vertexCount > 0u);
    
    vertex_buffer_t* pVertexBuffer = _k15_dynamic_buffer_push_back(&pContext->vertexBuffers, 1u);
    if( pVertexBuffer == nullptr )
    {
        return k15_invalid_vertex_buffer_handle;
    }

    pVertexBuffer->vertexCount  = vertexCount;
    pVertexBuffer->pData        = pVertexData;

    vertex_buffer_handle_t handle = {pVertexBuffer};
    return handle;
}

texture_handle_t k15_create_texture(software_rasterizer_context_t* pContext, uint32_t width, uint32_t height, uint32_t stride, uint8_t componentCount, const void* pTextureData)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(pTextureData != nullptr);
    RuntimeAssert(width > 0u);
    RuntimeAssert(height > 0u);
    RuntimeAssert(stride > 0u);
    RuntimeAssert(componentCount > 0u);
    RuntimeAssert(_k15_is_pow2(width));
    RuntimeAssert(_k15_is_pow2(height));

    texture_t* pTexture = _k15_dynamic_buffer_push_back(&pContext->textures, 1u);
    if( pTexture == nullptr )
    {
        return k15_invalid_texture_handle;
    }

    pTexture->width             = width;
    pTexture->height            = height;
    pTexture->stride            = stride;
    pTexture->componentCount    = componentCount;
    pTexture->pData             = pTextureData;

    texture_handle_t handle = {pTexture};
    return handle;
}

void k15_bind_vertex_buffer(software_rasterizer_context_t* pContext, vertex_buffer_handle_t vertexBuffer)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(k15_is_valid_vertex_buffer(vertexBuffer));

    pContext->pBoundVertexBuffer = (vertex_buffer_t*)vertexBuffer.pHandle;
}

void k15_bind_texture(software_rasterizer_context_t* pContext, texture_handle_t texture, uint32_t slot)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(k15_is_valid_texture(texture));
    RuntimeAssert(slot < DrawCallMaxTextures);

    pContext->boundTextures[slot] = (texture_t*)texture.pHandle;
}

void k15_bind_model_matrix(software_rasterizer_context_t* pContext, const matrix4x4f_t* pModelMatrix)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(pModelMatrix != nullptr);

    memcpy(&pContext->modelMatrix, pModelMatrix, sizeof(matrix4x4f_t));
}

void k15_bind_view_matrix(software_rasterizer_context_t* pContext, const matrix4x4f_t* pViewMatrix)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(pViewMatrix != nullptr);

    memcpy(&pContext->viewMatrix, pViewMatrix, sizeof(matrix4x4f_t));
}

bool k15_draw(software_rasterizer_context_t* pContext, uint32_t vertexCount, uint32_t vertexOffset)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(pContext->pBoundVertexBuffer != nullptr);
    RuntimeAssert(pContext->pBoundVertexBuffer->vertexCount >= vertexOffset + vertexCount);
    RuntimeAssert(vertexCount > 0u && ( vertexCount % 3u ) == 0);

    draw_call_t* pDrawCall = _k15_dynamic_buffer_push_back(&pContext->drawCalls, 1u);
    if( pDrawCall == nullptr )
    {
        return false;
    }

    CompiletimeAssert(sizeof(pDrawCall->textures) == sizeof(pContext->boundTextures));
    memcpy(pDrawCall->textures, pContext->boundTextures, sizeof(pDrawCall->textures));
    memcpy(&pDrawCall->modelMatrix, &pContext->modelMatrix, sizeof(pDrawCall->modelMatrix));

    pDrawCall->pVertexBuffer    = pContext->pBoundVertexBuffer;
    pDrawCall->vertexCount      = vertexCount;
    pDrawCall->vertexOffset     = vertexOffset;

    return true;
}

#endif //K15_SOFTWARE_RASTERIZER_IMPLEMENTATION
#endif //K15_SOFTWARE_RASTERIZER_INCLUDE