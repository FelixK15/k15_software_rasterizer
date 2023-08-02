#ifndef K15_SOFTWARE_RASTERIZER_INCLUDE
#define K15_SOFTWARE_RASTERIZER_INCLUDE

#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include <math.h>

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

#define RuntimeAssert(x) if(!(x)){  __debugbreak(); }
#define RuntimeAssertMsg(x, msg) RuntimeAssert(x)
#define UnusedVariable(x) (void)(x)

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

struct triangle2i_t
{
    vector2i_t points[3];
};

struct dynamic_buffer_t
{
    uint32_t    count;
    uint32_t    capacity;
    uint32_t    elementSizeInBytes;
    void*       pData;
};

struct software_rasterizer_settings_t
{
    uint8_t backFaceCullingEnabled : 1;
};

struct software_rasterizer_context_t
{
    software_rasterizer_settings_t      settings;
    uint8_t                             colorBufferCount;
    uint8_t                             currentColorBufferIndex;
    uint8_t                             currentTriangleVertexIndex;

    topology_t                          currentTopology;

    uint32_t                            backBufferWidth;
    uint32_t                            backBufferHeight;
    uint32_t                            backBufferStride;

    void*                               pColorBuffer[MaxColorBuffer];

    dynamic_buffer_t                    triangles;
    dynamic_buffer_t                    culledTriangles;
    dynamic_buffer_t                    screenSpaceTriangles;

    matrix4x4f_t                        projectionMatrix;
    matrix4x4f_t                        viewMatrix;
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

bool _k15_matrix4x4f_is_equal(const matrix4x4f_t* pA, const matrix4x4f_t* pB)
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
void _k15_draw_line(const software_rasterizer_context_t* pContext, const vector2i_t* pVectorA, const vector2i_t* pVectorB, uint32_t color)
{
    int x = pVectorA->x;
    int y = pVectorA->y;
    int x2 = pVectorB->x;
    int y2 = pVectorB->y;

    bool yLonger=false;
    int shortLen=y2-y;
    int longLen=x2-x;

    uint32_t* pBackBuffer = (uint32_t*)pContext->pColorBuffer[ pContext->currentColorBufferIndex ];

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
                pBackBuffer[localX + localY * pContext->backBufferStride] = color;
                j+=decInc;
            }
            return;
        }
        longLen+=y;
        for (int j=0x8000+(x<<16);y>=longLen;--y) {
            const uint32_t localX = j >> 16;
            const uint32_t localY = y;
            pBackBuffer[localX + localY * pContext->backBufferStride] = color;
            j-=decInc;
        }
        return;	
    }

    if (longLen>0) {
        longLen+=x;
        for (int j=0x8000+(y<<16);x<=longLen;++x) {
            const uint32_t localX = x;
            const uint32_t localY = j >> 16;
            pBackBuffer[localX + localY * pContext->backBufferStride] = color;
            j+=decInc;
        }
        return;
    }
    longLen+=x;
    for (int j=0x8000+(y<<16);x>=longLen;--x) {
        const uint32_t localX = x;
        const uint32_t localY = j >> 16;
        pBackBuffer[localX + localY * pContext->backBufferStride] = color;
        j-=decInc;
    }
}

void k15_draw_triangles(const software_rasterizer_context_t* pContext)
{
    for(uint32_t triangleIndex = 0; triangleIndex < pContext->screenSpaceTriangles.count; ++triangleIndex)
    {
        const triangle2i_t* pTriangle = (const triangle2i_t*)pContext->screenSpaceTriangles.pData + triangleIndex;
        _k15_draw_line(pContext, &pTriangle->points[0], &pTriangle->points[1], 0xFFFFFFFF);
        _k15_draw_line(pContext, &pTriangle->points[1], &pTriangle->points[2], 0xFFFFFFFF);
        _k15_draw_line(pContext, &pTriangle->points[2], &pTriangle->points[0], 0xFFFFFFFF);
    }
}

bool _k15_create_dynamic_buffer(dynamic_buffer_t* pOutBuffer, uint32_t initialCapacity, uint32_t elementSizeInBytes)
{
    pOutBuffer->pData = malloc(initialCapacity * elementSizeInBytes);
    if(pOutBuffer->pData == nullptr)
    {
        return false;
    }

    pOutBuffer->elementSizeInBytes = elementSizeInBytes;
    pOutBuffer->capacity = initialCapacity;
    pOutBuffer->count = 0;

    return true;
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

bool _k15_grow_dynamic_buffer(dynamic_buffer_t* pBuffer, uint32_t newCapacity)
{
    if(pBuffer->capacity > newCapacity)
    {
        return true;
    }
    
    const uint32_t newPow2Size = _k15_is_pow2(newCapacity) ? newCapacity : _k15_get_next_pow2(newCapacity);
    const uint32_t newTriangleBufferCapacity = pBuffer->capacity * 2;
    void* pNewTriangleBuffer = malloc(newTriangleBufferCapacity * pBuffer->elementSizeInBytes);
    if(pNewTriangleBuffer == nullptr)
    {
        return false;
    }

    memcpy(pNewTriangleBuffer, pBuffer->pData, pBuffer->capacity * pBuffer->elementSizeInBytes);

    free(pBuffer->pData);
    pBuffer->pData = pNewTriangleBuffer;
    pBuffer->capacity = newTriangleBufferCapacity;

    return true;
}

void* _k15_dynamic_buffer_push_back(dynamic_buffer_t* pBuffer, uint32_t elementCount)
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
    return (uint8_t*)pBuffer->pData + (oldCount * pBuffer->elementSizeInBytes);
}

vector4f_t k15_create_vector4f(float x, float y, float z, float w)
{
    return {x, y, z, w};
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

    pContext->settings.backFaceCullingEnabled = 1;
    for(uint8_t colorBufferIndex = 0; colorBufferIndex < pParameters->colorBufferCount; ++colorBufferIndex)
    {
        pContext->pColorBuffer[colorBufferIndex] = pParameters->pColorBuffers[colorBufferIndex];
    }

    pContext->colorBufferCount = pParameters->colorBufferCount;

    if(!_k15_create_dynamic_buffer(&pContext->triangles, DefaultTriangleBufferCapacity, sizeof(triangle4f_t)))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer(&pContext->culledTriangles, DefaultTriangleBufferCapacity, sizeof(triangle4f_t)))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer(&pContext->screenSpaceTriangles, DefaultTriangleBufferCapacity, sizeof(triangle2i_t)))
    {
        return false;
    }

    _k15_create_projection_matrix(&pContext->projectionMatrix, pParameters->backBufferWidth, pParameters->backBufferHeight, 1.0f, 10.f, 90.0f);
    //_k15_create_orthographic_matrix(&pContext->projectionMatrix, pParameters->backBufferWidth, pParameters->backBufferHeight,  1.0f, 10.f);
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

matrix4x4f_t _k15_mul_matrix4x4f(const matrix4x4f_t* pMatA, const matrix4x4f_t* pMatB)
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

void k15_transform_triangles(software_rasterizer_context_t* pContext)
{
    matrix4x4f_t viewProjMat = _k15_mul_matrix4x4f(&pContext->projectionMatrix, &pContext->viewMatrix);
    
    for(uint32_t triangleIndex = 0; triangleIndex < pContext->triangles.count; ++triangleIndex)
    {
        triangle4f_t* pTriangle = (triangle4f_t*)pContext->triangles.pData + triangleIndex;
        pTriangle->points[0] = _k15_mul_vector4_matrix44(&pTriangle->points[0], &viewProjMat);
        pTriangle->points[1] = _k15_mul_vector4_matrix44(&pTriangle->points[1], &viewProjMat);
        pTriangle->points[2] = _k15_mul_vector4_matrix44(&pTriangle->points[2], &viewProjMat);

        pTriangle->points[0].x /= pTriangle->points[0].w;
        pTriangle->points[0].y /= pTriangle->points[0].w;
        pTriangle->points[0].z /= pTriangle->points[0].w;
        pTriangle->points[0].w /= pTriangle->points[0].w;

        pTriangle->points[1].x /= pTriangle->points[1].w;
        pTriangle->points[1].y /= pTriangle->points[1].w;
        pTriangle->points[1].z /= pTriangle->points[1].w;
        pTriangle->points[1].w /= pTriangle->points[1].w;

        pTriangle->points[2].x /= pTriangle->points[2].w;
        pTriangle->points[2].y /= pTriangle->points[2].w;
        pTriangle->points[2].z /= pTriangle->points[2].w;
        pTriangle->points[2].w /= pTriangle->points[2].w;
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
    dynamic_buffer_t* pCulledTriangles = &pContext->culledTriangles;
    for(uint32_t triangleIndex = 0; triangleIndex < pContext->triangles.count; ++triangleIndex)
    {
        triangle4f_t* pTriangle = (triangle4f_t*)pContext->triangles.pData + triangleIndex;
        const vector4f_t ab = _k15_vector4f_sub(pTriangle->points[1], pTriangle->points[0]);
        const vector4f_t ac = _k15_vector4f_sub(pTriangle->points[2], pTriangle->points[0]);
        const vector4f_t sign = _k15_vector4f_cross(ab, ac);

        if(sign.z > 0.0f)
        {
            triangle4f_t* pNonCulledTriangle = (triangle4f_t*)_k15_dynamic_buffer_push_back(pCulledTriangles, 1u);
            if(pNonCulledTriangle == nullptr)
            {
                return false;
            }
            memcpy(pNonCulledTriangle, pTriangle, sizeof(triangle4f_t));
        }
    }

    return true;
}

void k15_cull_triangles(software_rasterizer_context_t* pContext)
{

}

void k15_project_triangles_into_screenspace(software_rasterizer_context_t* pContext)
{
    const float width   = (float)pContext->backBufferWidth-1u;
    const float height  = (float)pContext->backBufferHeight-1u;

    const float halfWidth = 0.0f;//width / 2.0f;
    const float halfHeight = 0.0f;//height / 2.0f;

    triangle2i_t* pScreenspaceTriangles = (triangle2i_t*)_k15_dynamic_buffer_push_back(&pContext->screenSpaceTriangles, pContext->culledTriangles.count);

    for(uint32_t triangleIndex = 0; triangleIndex < pContext->culledTriangles.count; ++triangleIndex)
    {
        const triangle4f_t* pTriangle = (triangle4f_t*)pContext->culledTriangles.pData + triangleIndex;
        triangle2i_t* pScreenspaceTriangle = pScreenspaceTriangles + triangleIndex;
        RuntimeAssert(pScreenspaceTriangle != nullptr);        

        pScreenspaceTriangle->points[0].x = (int)(((1.0f + pTriangle->points[0].x) / 2.0f) * width);
        pScreenspaceTriangle->points[0].y = (int)(((1.0f + pTriangle->points[0].y) / 2.0f) * height);

        pScreenspaceTriangle->points[1].x = (int)(((1.0f + pTriangle->points[1].x) / 2.0f) * width);
        pScreenspaceTriangle->points[1].y = (int)(((1.0f + pTriangle->points[1].y) / 2.0f) * height);

        pScreenspaceTriangle->points[2].x = (int)(((1.0f + pTriangle->points[2].x) / 2.0f) * width);
        pScreenspaceTriangle->points[2].y = (int)(((1.0f + pTriangle->points[2].y) / 2.0f) * height);
    }
}

void k15_draw_frame(software_rasterizer_context_t* pContext)
{
    bool outOfMemory = false;
    k15_transform_triangles(pContext);
    if(pContext->settings.backFaceCullingEnabled)
    {
        outOfMemory = !k15_cull_backfacing_triangles(pContext);
    }
    else
    {
        triangle4f_t* pCulledTriangles = (triangle4f_t*)_k15_dynamic_buffer_push_back(&pContext->culledTriangles, pContext->triangles.count);
        RuntimeAssert(pCulledTriangles != nullptr);

        memcpy(pCulledTriangles, pContext->triangles.pData, pContext->triangles.count * sizeof(triangle4f_t));
    }

    k15_cull_triangles(pContext);
    k15_project_triangles_into_screenspace(pContext);
    k15_draw_triangles(pContext);
    pContext->triangles.count = 0;
    pContext->culledTriangles.count = 0;
    pContext->screenSpaceTriangles.count = 0;
}

void k15_change_color_buffers(software_rasterizer_context_t* pContext, void** pColorBuffers, uint8_t colorBufferCount, uint32_t widthInPixels, uint32_t heightInPixels, uint32_t strideInBytes)
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

    triangle4f_t* pCurrentTriangle = (triangle4f_t*)pContext->triangles.pData + (pContext->triangles.count - 1u);

    if(pContext->currentTriangleVertexIndex == 0u || pContext->currentTriangleVertexIndex == 3u)
    {
        pCurrentTriangle = (triangle4f_t*)_k15_dynamic_buffer_push_back(&pContext->triangles, 1u);
        RuntimeAssert(pCurrentTriangle != nullptr);

        pContext->currentTriangleVertexIndex = 0;
    }

    pCurrentTriangle->points[pContext->currentTriangleVertexIndex++] = k15_create_vector4f(x, y, z, 1.0f);
}

#endif //K15_SOFTWARE_RASTERIZER_IMPLEMENTATION
#endif //K15_SOFTWARE_RASTERIZER_INCLUDE