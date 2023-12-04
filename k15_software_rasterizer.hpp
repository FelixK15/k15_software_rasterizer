#ifndef K15_SOFTWARE_RASTERIZER_INCLUDE
#define K15_SOFTWARE_RASTERIZER_INCLUDE

#include <stdint.h>
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

struct uniform_buffer_handle_t
{
    void* pHandle;
};

struct texture_handle_t
{
    void* pHandle;
};

struct vertex_shader_handle_t
{
    void* pHandle;
};

struct pixel_shader_handle_t
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

enum class sample_addressing_mode_t
{
    repeat = 0,
    clamp,
    mirror
};

struct software_rasterizer_context_t;

struct software_rasterizer_context_init_parameters_t
{
    uint32_t    backBufferWidth;
    uint32_t    backBufferHeight;
    uint32_t    colorBufferStride;
    uint32_t    depthBufferStride;
    uint8_t     redShift;
    uint8_t     greenShift;
    uint8_t     blueShift;
    void*       pColorBuffers[3];
    void*       pDepthBuffers[3];
    uint8_t     colorBufferCount;
};

constexpr uint32_t PixelShaderTileSize     = 256u;
constexpr uint32_t PixelShaderInputCount   = PixelShaderTileSize*PixelShaderTileSize;
constexpr uint32_t VertexShaderInputCount  = 30u;

static_assert(( VertexShaderInputCount % 3u ) == 0);

struct vertex_shader_input_t
{
    vector4f_t positions[VertexShaderInputCount];
    vector4f_t normals[VertexShaderInputCount];
    vector4f_t colors[VertexShaderInputCount];
    vector2f_t texcoords[VertexShaderInputCount];
};

struct vertex_t
{
    vector4f_t position;
    vector4f_t normal;
    vector4f_t color;
    vector2f_t texcoord;
};

struct stack_allocator_t;

struct pixel_shader_input_t
{
    stack_allocator_t*  pStackAllocator;
    vertex_t*           pVertexData;
    float*              pDepth;
    uint32_t*           pScreenspaceX;
    uint32_t*           pScreenspaceY;
    const void*         pUniformData;

    uint32_t            pixelCount;
};

struct pixel_shader_output_t
{
    vector4f_t*     pColor;
    const uint32_t* pScreenspaceX;
    const uint32_t* pScreenspaceY;
};

struct texture_samples_t
{
    vector4f_t* pColors;
};

struct pixel_t
{
    float r;
    float g;
    float b;

    uint32_t screenX;
    uint32_t screenY;
};

typedef void(*vertex_shader_fnc_t)(vertex_shader_input_t* pInOutVertices, uint32_t vertexCount, const void* pUniformData);
typedef void(*pixel_shader_fnc_t)(const pixel_shader_input_t* pPixelShaderInput, pixel_shader_output_t* pPixelShaderOutput, uint32_t pixelCount, const void* pUniformData);

vertex_buffer_handle_t  k15_invalid_vertex_buffer_handle    = {nullptr};
uniform_buffer_handle_t k15_invalid_uniform_buffer_handle   = {nullptr};
texture_handle_t        k15_invalid_texture_handle          = {nullptr};
vertex_shader_handle_t  k15_invalid_vertex_shader_handle    = {nullptr};
pixel_shader_handle_t   k15_invalid_pixel_shader_handle     = {nullptr};

software_rasterizer_context_init_parameters_t   k15_create_default_software_rasterizer_context_parameters();

bool                                            k15_create_software_rasterizer_context(software_rasterizer_context_t** pOutContextPtr, const software_rasterizer_context_init_parameters_t* pParameters);

void                                            k15_create_projection_matrix(matrix4x4f_t* pOutMatrix, uint32_t width, uint32_t height, float near, float far, float fov);
void                                            k15_create_orthographic_matrix(matrix4x4f_t* pOutMatrix, uint32_t width, uint32_t height, float near, float far);
void                                            k15_set_identity_matrix4x4f(matrix4x4f_t* pMatrix);

void                                            k15_swap_color_buffers(software_rasterizer_context_t* pContext);
void                                            k15_draw_frame(software_rasterizer_context_t* pContext);

void                                            k15_change_color_buffers(software_rasterizer_context_t* pContext, void* pColorBuffers[3], uint8_t colorBufferCount, uint32_t widthInPixels, uint32_t heightInPixels, uint32_t strideInBytes);

bool                                            k15_is_valid_vertex_buffer(const vertex_buffer_handle_t vertexBuffer);
bool                                            k15_is_valid_texture(const texture_handle_t texture);

vertex_shader_handle_t                          k15_create_vertex_shader(software_rasterizer_context_t* pContext, vertex_shader_fnc_t vertexShaderFnc);
pixel_shader_handle_t                           k15_create_pixel_shader(software_rasterizer_context_t* pContext, pixel_shader_fnc_t vertexShaderFnc);
vertex_buffer_handle_t                          k15_create_vertex_buffer(software_rasterizer_context_t* pContext, uint32_t vertexSizeInBytes, const vertex_t* pVertexData);
uniform_buffer_handle_t                         k15_create_uniform_buffer(software_rasterizer_context_t* pContext, uint32_t uniformBufferSizeInBytes);
texture_handle_t                                k15_create_texture(software_rasterizer_context_t* pContext, const char* pName, uint32_t width, uint32_t height, uint32_t stride, uint8_t componentCount, const void* pTextureData);

void                                            k15_set_uniform_buffer_data(uniform_buffer_handle_t uniformBufferHandle, const void* pData, uint32_t uniformBufferSizeInBytes, uint32_t uniformBufferOffsetInBytes);

void                                            k15_bind_vertex_shader(software_rasterizer_context_t* pContext, vertex_shader_handle_t vertexShaderHandle);
void                                            k15_bind_pixel_shader(software_rasterizer_context_t* pContext, pixel_shader_handle_t pixelShaderHandle);
void                                            k15_bind_vertex_buffer(software_rasterizer_context_t* pContext, vertex_buffer_handle_t vertexBuffer);
void                                            k15_bind_uniform_buffer(software_rasterizer_context_t* pContext, uniform_buffer_handle_t uniformBuffer);
void                                            k15_bind_texture(software_rasterizer_context_t* pContext, texture_handle_t texture, uint32_t slot);
bool                                            k15_draw(software_rasterizer_context_t* pContext, uint32_t vertexCount);

template<sample_addressing_mode_t ADDRESSING_MODE>
texture_samples_t                               k15_sample_texture(texture_handle_t texture, const pixel_shader_input_t* pPixelShaderInput, uint32_t texcoordCount);

constexpr vertex_t                              k15_create_vertex(vector4f_t position, vector4f_t normal, vector4f_t color, vector2f_t texcoord);

#ifdef K15_SOFTWARE_RASTERIZER_IMPLEMENTATION

#ifdef _MSC_BUILD
#include <intrin.h>
#define restrict_modifier       __restrict
#define NativeDebugBreak()      __debugbreak()
#define BreakpointHook()        __nop()
#else
#warning No support for this compiler
#define restrict_modifier
#define NativeDebugBreak()
#define CompiletimeAssert(x)
#define BreakpointHook()
#endif

#if defined (_M_X64) || defined (_M_AMD64 ) || defined(__x86_64__)
#include <immintrin.h>
#include <smmintrin.h>
#define MemoryPrefetch0(ptr)    _mm_prefetch((const char*)(ptr), _MM_HINT_T0)
#define MemoryPrefetchNTA(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_NTA)
#endif

#define CompiletimeAssert(x)        static_assert(x)
#define RuntimeAssertMsg(x, msg)    RuntimeAssert(x)
#define UnusedVariable(x)           (void)(x)
#define MultiplyBy2(x)              ((x)<<1)

#define get_min(a,b) (a)>(b)?(b):(a)
#define get_max(a,b) (a)>(b)?(a):(b)

#define clamp(v, min, max) (v)>(max)?(max):(v)<(min)?(min):(v)
#define clamp01f(v) clamp(v, 0.0f, 1.0f)

#ifdef K15_RELEASE_BUILD
#define RuntimeAssert(x)
#else
#define RuntimeAssert(x)        if(!(x)){  NativeDebugBreak(); }
#endif

#define internal static

#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>

constexpr uint32_t MaxColorBuffer                               = 3u;
constexpr uint32_t DefaultTriangleBufferCapacity                = 1024u;
constexpr uint32_t DefaultVertexBufferCapacity                  = 64u;
constexpr uint32_t DefaultShaderCapacity                        = 32u;
constexpr uint32_t DefaultTextureCapacity                       = 256u;
constexpr uint32_t DefaultDrawCallCapacity                      = 512u;

constexpr uint32_t DrawCallMaxVertexBuffer                      = 4u;
constexpr uint32_t DrawCallMaxTextures                          = 4u;

constexpr uint32_t DebugLineCapacity                            = 128u;

constexpr uint32_t DefaultBlockCapacityInBytes                  = 1024u * 10u;
constexpr uint32_t DefaultUniformDataStackAllocatorSizeInBytes  = 1024u * 1024u;

constexpr float pi = 3.141f;

internal constexpr const matrix4x4f_t IdentityMatrix4x4[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

internal constexpr alignas(16) const uint32_t OutputBitMaskLUT4x[5][4] = {
    {0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000},
    {0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}
};

internal constexpr alignas(16) const uint32_t OutputBitMaskLUT8x[9][8] = {
    {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000},
    {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF},
};

internal constexpr const uint8_t ShuffleBitMaskLUT4x[16] = {
    0b00000000, 0b00000000, 0b01000000, 0b00010000, 0b10000000, 0b00100000, 0b01100000, 0b00011000,
    0b11000000, 0b00110000, 0b01110000, 0b00011100, 0b10110000, 0b00101100, 0b01101100, 0b00011011
};

internal constexpr const uint32_t ShuffleBitMaskLUT8x[256] = {
    0b000000000000000000000000, 0b000000000000000000000000, 0b001000000000000000000000, 0b000001000000000000000000, 0b010000000000000000000000, 0b000010000000000000000000, 0b001010000000000000000000, 0b000001010000000000000000,
    0b011000000000000000000000, 0b000011000000000000000000, 0b001011000000000000000000, 0b000001011000000000000000, 0b010011000000000000000000, 0b000010011000000000000000, 0b001010011000000000000000, 0b000001010011000000000000,
    0b100000000000000000000000, 0b000100000000000000000000, 0b001100000000000000000000, 0b000001100000000000000000, 0b010100000000000000000000, 0b000010100000000000000000, 0b001010100000000000000000, 0b000001010100000000000000,
    0b011100000000000000000000, 0b000011100000000000000000, 0b001011100000000000000000, 0b000001011100000000000000, 0b010011100000000000000000, 0b000010011100000000000000, 0b001010011100000000000000, 0b000001010011100000000000,
    0b101000000000000000000000, 0b000101000000000000000000, 0b001101000000000000000000, 0b000001101000000000000000, 0b010101000000000000000000, 0b000010101000000000000000, 0b001010101000000000000000, 0b000001010101000000000000,
    0b011101000000000000000000, 0b000011101000000000000000, 0b001011101000000000000000, 0b000001011101000000000000, 0b010011101000000000000000, 0b000010011101000000000000, 0b001010011101000000000000, 0b000001010011101000000000,
    0b100101000000000000000000, 0b000100101000000000000000, 0b001100101000000000000000, 0b000001100101000000000000, 0b010100101000000000000000, 0b000010100101000000000000, 0b001010100101000000000000, 0b000001010100101000000000,
    0b011100101000000000000000, 0b000011100101000000000000, 0b001011100101000000000000, 0b000001011100101000000000, 0b010011100101000000000000, 0b000010011100101000000000, 0b001010011100101000000000, 0b000001010011100101000000,
    0b110000000000000000000000, 0b000110000000000000000000, 0b001110000000000000000000, 0b000001110000000000000000, 0b010110000000000000000000, 0b000010110000000000000000, 0b001010110000000000000000, 0b000001010110000000000000,
    0b011110000000000000000000, 0b000011110000000000000000, 0b001011110000000000000000, 0b000001011110000000000000, 0b010011110000000000000000, 0b000010011110000000000000, 0b001010011110000000000000, 0b000001010011110000000000,
    0b100110000000000000000000, 0b000100110000000000000000, 0b001100110000000000000000, 0b000001100110000000000000, 0b010100110000000000000000, 0b000010100110000000000000, 0b001010100110000000000000, 0b000001010100110000000000,
    0b011100110000000000000000, 0b000011100110000000000000, 0b001011100110000000000000, 0b000001011100110000000000, 0b010011100110000000000000, 0b000010011100110000000000, 0b001010011100110000000000, 0b000001010011100110000000,
    0b101110000000000000000000, 0b000101110000000000000000, 0b001101110000000000000000, 0b000001101110000000000000, 0b010101110000000000000000, 0b000010101110000000000000, 0b001010101110000000000000, 0b000001010101110000000000,
    0b011101110000000000000000, 0b000011101110000000000000, 0b001011101110000000000000, 0b000001011101110000000000, 0b010011101110000000000000, 0b000010011101110000000000, 0b001010011101110000000000, 0b000001010011101110000000,
    0b100101110000000000000000, 0b000100101110000000000000, 0b001100101110000000000000, 0b000001100101110000000000, 0b010100101110000000000000, 0b000010100101110000000000, 0b001010100101110000000000, 0b000001010100101110000000,
    0b011100101110000000000000, 0b000011100101110000000000, 0b001011100101110000000000, 0b000001011100101110000000, 0b010011100101110000000000, 0b000010011100101110000000, 0b001010011100101110000000, 0b000001010011100101110000,
    0b111000000000000000000000, 0b000111000000000000000000, 0b001111000000000000000000, 0b000001111000000000000000, 0b010111000000000000000000, 0b000010111000000000000000, 0b001010111000000000000000, 0b000001010111000000000000,
    0b011111000000000000000000, 0b000011111000000000000000, 0b001011111000000000000000, 0b000001011111000000000000, 0b010011111000000000000000, 0b000010011111000000000000, 0b001010011111000000000000, 0b000001010011111000000000,
    0b100111000000000000000000, 0b000100111000000000000000, 0b001100111000000000000000, 0b000001100111000000000000, 0b010100111000000000000000, 0b000010100111000000000000, 0b001010100111000000000000, 0b000001010100111000000000,
    0b011100111000000000000000, 0b000011100111000000000000, 0b001011100111000000000000, 0b000001011100111000000000, 0b010011100111000000000000, 0b000010011100111000000000, 0b001010011100111000000000, 0b000001010011100111000000,
    0b101111000000000000000000, 0b000101111000000000000000, 0b001101111000000000000000, 0b000001101111000000000000, 0b010101111000000000000000, 0b000010101111000000000000, 0b001010101111000000000000, 0b000001010101111000000000,
    0b011101111000000000000000, 0b000011101111000000000000, 0b001011101111000000000000, 0b000001011101111000000000, 0b010011101111000000000000, 0b000010011101111000000000, 0b001010011101111000000000, 0b000001010011101111000000,
    0b100101111000000000000000, 0b000100101111000000000000, 0b001100101111000000000000, 0b000001100101111000000000, 0b010100101111000000000000, 0b000010100101111000000000, 0b001010100101111000000000, 0b000001010100101111000000,
    0b011100101111000000000000, 0b000011100101111000000000, 0b001011100101111000000000, 0b000001011100101111000000, 0b010011100101111000000000, 0b000010011100101111000000, 0b001010011100101111000000, 0b000001010011100101111000,
    0b110111000000000000000000, 0b000110111000000000000000, 0b001110111000000000000000, 0b000001110111000000000000, 0b010110111000000000000000, 0b000010110111000000000000, 0b001010110111000000000000, 0b000001010110111000000000,
    0b011110111000000000000000, 0b000011110111000000000000, 0b001011110111000000000000, 0b000001011110111000000000, 0b010011110111000000000000, 0b000010011110111000000000, 0b001010011110111000000000, 0b000001010011110111000000,
    0b100110111000000000000000, 0b000100110111000000000000, 0b001100110111000000000000, 0b000001100110111000000000, 0b010100110111000000000000, 0b000010100110111000000000, 0b001010100110111000000000, 0b000001010100110111000000, 
    0b011100110111000000000000, 0b000011100110111000000000, 0b001011100110111000000000, 0b000001011100110111000000, 0b010011100110111000000000, 0b000010011100110111000000, 0b001010011100110111000000, 0b000001010011100110111000,
    0b101110111000000000000000, 0b000101110111000000000000, 0b001101110111000000000000, 0b000001101110111000000000, 0b010101110111000000000000, 0b000010101110111000000000, 0b001010101110111000000000, 0b000001010101110111000000,
    0b011101110111000000000000, 0b000011101110111000000000, 0b001011101110111000000000, 0b000001011101110111000000, 0b010011101110111000000000, 0b000010011101110111000000, 0b001010011101110111000000, 0b000001010011101110111000,
    0b100101110111000000000000, 0b000100101110111000000000, 0b001100101110111000000000, 0b000001100101110111000000, 0b010100101110111000000000, 0b000010100101110111000000, 0b001010100101110111000000, 0b000001010100101110111000,
    0b011100101110111000000000, 0b000011100101110111000000, 0b001011100101110111000000, 0b000001011100101110111000, 0b010011100101110111000000, 0b000010011100101110111000, 0b001010011100101110111000, 0b000001010011100101110111
};

enum vertex_id_t : uint32_t
{
    position,
    color,
};

struct vertex_buffer_t
{
    const vertex_t* pData;
    uint32_t vertexCount;
};

struct uniform_buffer_t
{
    void* pData;
    uint32_t dataSizeInBytes;
};

struct texture_t
{
    char name[256];
    const void* pData;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t componentCount;
};

struct vertex_shader_t
{
    vertex_shader_fnc_t function;
};

struct pixel_shader_t
{
    pixel_shader_fnc_t  function;
};

struct draw_call_t
{
    vertex_buffer_t*    pVertexBuffer;
    void*               pUniformBufferData;
    vertex_shader_fnc_t vertexShader;
    pixel_shader_fnc_t  pixelShader;
    uint32_t            vertexCount;
    uint32_t            vertexOffset;
};

struct clipped_vertex_t : vertex_t
{
    uint32_t triangleIndex;
};

struct triangle_t
{
    vertex_t vertices[3];
};

struct screenspace_triangle_t
{
                vector3f_t      screenspaceVertexPositions[3];
    alignas(16) vertex_t        vertices[3];
                bounding_box_t  boundingBox;
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

struct software_rasterizer_settings_t
{
    uint8_t backFaceCullingEnabled  : 1;
    uint8_t drawWireframe           : 1;
    uint8_t drawDepthBuffer         : 1;
};

struct bitmap_font_t
{
    const uint8_t* pRGBPixels;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
};

struct draw_call_triangles_t
{
    vertex_shader_fnc_t     vertexShader;
    pixel_shader_fnc_t      pixelShader;
    void*                   pUniformData;
    screenspace_triangle_t* pScreenspaceTriangles;
    triangle_t*             pTriangles;
    uint32_t                triangleCount;
    uint32_t                screenspaceTriangleCount;
};

struct draw_call_screenspace_triangles_t
{
    pixel_shader_fnc_t  pixelShader;
    void*               pUniformData;
    triangle_t*         pTriangles;
    uint32_t            triangleCount;
};

struct block_allocator_t
{
    void* pNextBlock;
    uint8_t* pBasePointer;
    uint32_t capacityInBytes;
    uint32_t sizeInBytes;
};

struct stack_allocator_t
{
    uint8_t* pBasePointer;
    uint32_t capacityInBytes;
    uint32_t sizeInBytes;
};

struct barycentric_coordinates_buffer_t
{
    float* pU;
    float* pV;
};

struct software_rasterizer_context_t
{
    software_rasterizer_settings_t              settings;
    bitmap_font_t                               font;
    barycentric_coordinates_buffer_t            barycentricCoordinatesBuffer;

    uint8_t                                     colorBufferCount;
    uint8_t                                     currentColorBufferIndex;
    uint8_t                                     currentTriangleVertexIndex;

    uint8_t                                     redShift;
    uint8_t                                     greenShift;
    uint8_t                                     blueShift;

    uint32_t                                    backBufferWidth;
    uint32_t                                    backBufferHeight;
    uint32_t                                    colorBufferStride;
    uint32_t                                    depthBufferStride;

    void*                                       pColorBuffer[MaxColorBuffer];
    void*                                       pDepthBuffer[MaxColorBuffer];

    uniform_buffer_t*                           pBoundUniformBuffer;
    vertex_buffer_t*                            pBoundVertexBuffer;
    texture_t*                                  boundTextures[DrawCallMaxTextures];

    vertex_shader_t*                            pBoundVertexShader;
    pixel_shader_t*                             pBoundPixelShader;

    block_allocator_t*                          pUniformDataAllocator;
    stack_allocator_t*                          pDrawCallDataAllocator;

    pixel_shader_input_t                        bufferedPixelShaderInput;
    pixel_shader_output_t                       bufferedPixelShaderOutput;

    dynamic_buffer_t<draw_call_t>               drawCalls;

    dynamic_buffer_t<uniform_buffer_t>          uniformBuffers;
    dynamic_buffer_t<vertex_buffer_t>           vertexBuffers;
    dynamic_buffer_t<texture_t>                 textures;  

    dynamic_buffer_t<vertex_shader_t>           vertexShaders;
    dynamic_buffer_t<pixel_shader_t>            pixelShaders;

    dynamic_buffer_t<triangle_t>                triangles;
    dynamic_buffer_t<triangle_t>                visibleTriangles;
    dynamic_buffer_t<triangle_t>                clippedTriangles;
    dynamic_buffer_t<screenspace_triangle_t>    screenspaceTriangles;
};

template<typename T>
internal bool _k15_create_dynamic_buffer(dynamic_buffer_t<T>* pOutBuffer, uint32_t initialCapacity)
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
internal bool _k15_grow_dynamic_buffer(dynamic_buffer_t<T>* pBuffer, uint32_t newCapacity)
{
    if(pBuffer->capacity > newCapacity)
    {
        return true;
    }
    
    const uint32_t newPow2Size = _k15_get_next_pow2(newCapacity);
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

template<typename T, bool GROW>
internal T* _k15_base_dynamic_buffer_push_back(dynamic_buffer_t<T>* pBuffer, uint32_t elementCount)
{
    const uint32_t newTriangleCount = pBuffer->count + elementCount;
    if(newTriangleCount >= pBuffer->capacity)
    {
        if( GROW )
        {
            if(!_k15_grow_dynamic_buffer(pBuffer, newTriangleCount))
            {
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
    }

    uint32_t oldCount = pBuffer->count;
    pBuffer->count += elementCount;
    return pBuffer->pData + oldCount;
}

template<typename T>
internal T* _k15_dynamic_buffer_push_back(dynamic_buffer_t<T>* pBuffer, uint32_t elementCount)
{
    return _k15_base_dynamic_buffer_push_back<T, true>(pBuffer, elementCount);
}

template<typename T>
internal T* _k15_dynamic_buffer_push_back_dont_grow(dynamic_buffer_t<T>* pBuffer, uint32_t elementCount)
{
    return _k15_base_dynamic_buffer_push_back<T, false>(pBuffer, elementCount);
}

template<typename T>
internal T* _k15_dynamic_buffer_push_back(dynamic_buffer_t<T>* pBuffer, const T* pValues, uint32_t elementCount)
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

    memcpy(pBuffer->pData + oldCount, pValues, sizeof(T) * elementCount);
    return pBuffer->pData + oldCount;
}

template<typename T>
internal T* _k15_dynamic_buffer_push_back(dynamic_buffer_t<T>* pBuffer, T value)
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
internal void _k15_destroy_dynamic_buffer(dynamic_buffer_t<T>* pBuffer)
{
    free(pBuffer->pData);
    pBuffer->pData = nullptr;
    pBuffer->capacity = 0;
    pBuffer->count = 0;
}

template<typename T>
internal T* _k15_static_buffer_push_back(base_static_buffer_t<T>* pStaticBuffer, uint32_t elementCount)
{
    const uint32_t bufferCapacity = pStaticBuffer->capacity;
    RuntimeAssert(pStaticBuffer->count + elementCount <= bufferCapacity);

    const uint32_t oldCount = pStaticBuffer->count;
    pStaticBuffer->count += elementCount;
    return pStaticBuffer->pStaticData + oldCount;
}

template<typename T>
internal T* _k15_static_buffer_push_back(base_static_buffer_t<T>* pStaticBuffer, T value)
{
    T* pValue = _k15_static_buffer_push_back(pStaticBuffer, 1u);
    memcpy(pValue, &value, sizeof(T));
    return pValue;
}

template<typename T>
internal bool _k15_push_static_buffer_to_dynamic_buffer(base_static_buffer_t<T>* pStaticBuffer, dynamic_buffer_t<T>* pDynamicBuffer)
{
    T* pDataFromDynamicBuffer = _k15_dynamic_buffer_push_back<T>(pDynamicBuffer, pStaticBuffer->count);
    if(pDataFromDynamicBuffer == nullptr)
    {
        return false;
    }

    memcpy(pDataFromDynamicBuffer, pStaticBuffer->pStaticData, sizeof(T) * pStaticBuffer->count);
    return true;
}

internal void* _k15_allocate_from_block_allocator(block_allocator_t* pBlockAllocator, uint32_t sizeInBytes)
{
    block_allocator_t* pCurrentBlock = pBlockAllocator;

    while(1)
    {
        const uint32_t sizeLeftInBytes = pCurrentBlock->capacityInBytes - pCurrentBlock->sizeInBytes;
        if( sizeLeftInBytes >= sizeInBytes)
        {
            uint8_t* pAllocation = pCurrentBlock->pBasePointer + pCurrentBlock->sizeInBytes;
            pCurrentBlock->sizeInBytes += sizeInBytes;
            return pAllocation;
        }

        if( pCurrentBlock->pNextBlock == nullptr )
        {
            break;
        }

        pCurrentBlock = (block_allocator_t*)pCurrentBlock->pNextBlock;
    }

    uint32_t nextBlockCapacityInBytes = get_max(DefaultBlockCapacityInBytes, sizeInBytes);
    uint8_t* pNextBlockMemory = (uint8_t*)malloc(nextBlockCapacityInBytes + sizeof(block_allocator_t));
    if( pNextBlockMemory == nullptr )
    {
        return nullptr;
    }

    block_allocator_t* pNextBlock = (block_allocator_t*)pNextBlockMemory;
    pNextBlock->pBasePointer = pNextBlockMemory + sizeof(block_allocator_t);
    pNextBlock->capacityInBytes = nextBlockCapacityInBytes;
    pNextBlock->sizeInBytes = sizeInBytes;
    pCurrentBlock->pNextBlock = pNextBlock;

    return pNextBlock->pBasePointer;
}

internal bool _k15_create_block_allocator(block_allocator_t** ppBlockAllocator)
{
    uint8_t* pFirstBlockMemory = (uint8_t*)malloc(DefaultBlockCapacityInBytes + sizeof(block_allocator_t));
    if( pFirstBlockMemory == nullptr )
    {
        return false;
    }

    block_allocator_t* pFirstBlock = (block_allocator_t*)pFirstBlockMemory;
    pFirstBlock->pBasePointer = pFirstBlockMemory + sizeof(block_allocator_t);
    pFirstBlock->capacityInBytes = DefaultBlockCapacityInBytes;
    pFirstBlock->sizeInBytes = 0;
    pFirstBlock->pNextBlock = nullptr;

    *ppBlockAllocator = pFirstBlock;

    return true;
}

internal void* _k15_allocate_from_stack_allocator(stack_allocator_t* pStackAllocator, uint32_t sizeInBytes)
{
    RuntimeAssert(pStackAllocator->sizeInBytes + sizeInBytes <= pStackAllocator->capacityInBytes);
    void* pData = pStackAllocator->pBasePointer + pStackAllocator->sizeInBytes;
    pStackAllocator->sizeInBytes += sizeInBytes;

    return pData;
}

internal bool _k15_create_stack_allocator(stack_allocator_t** ppStackAllocator, uint32_t capacityInBytes)
{
    uint8_t* restrict_modifier pStackAllocatorMemory = (uint8_t*)malloc(capacityInBytes + sizeof(stack_allocator_t));
    if( pStackAllocatorMemory == nullptr )
    {
        return false;
    }

    stack_allocator_t* pAllocator = (stack_allocator_t*)pStackAllocatorMemory;
    pAllocator->pBasePointer = pStackAllocatorMemory + sizeof(stack_allocator_t);
    pAllocator->sizeInBytes = 0;
    pAllocator->capacityInBytes = capacityInBytes;

    *ppStackAllocator = pAllocator;

    return true;
}

internal bool _k15_create_barycentric_coordinate_buffer(barycentric_coordinates_buffer_t* pBarycentricCoordinateBuffer, uint32_t coordinateCount)
{
    float* pUBuffer = (float*)_mm_malloc(coordinateCount * sizeof(float), 16);
    float* pVBuffer = (float*)_mm_malloc(coordinateCount * sizeof(float), 16);
    if( pUBuffer == nullptr || pVBuffer == nullptr)
    {
        return false;
    }

    pBarycentricCoordinateBuffer->pU = pUBuffer;
    pBarycentricCoordinateBuffer->pV = pVBuffer;
    return true;
}

internal void _k15_reset_stack_allocator(stack_allocator_t* pStackAllocator)
{
    pStackAllocator->sizeInBytes = 0;
}

//https://jsantell.com/3d-projection/
void k15_create_projection_matrix(matrix4x4f_t* pOutMatrix, uint32_t width, uint32_t height, float near, float far, float fov)
{
    memset(pOutMatrix, 0, sizeof(matrix4x4f_t));

    const float widthF = (float)width;
    const float heightF = (float)height;
    const float fovRad = fov/180.f * pi;
    const float aspect = widthF / heightF;
    const float e = 1.0f/tanf(fovRad/2.0f);

    pOutMatrix->m00 = e/aspect;
    pOutMatrix->m11 = e;
    pOutMatrix->m22 = -((far + near)/(far - near));
    pOutMatrix->m23 = -((2.0f * far * near)/(far - near));
    pOutMatrix->m32 = -1.0f;
}

void k15_create_orthographic_matrix(matrix4x4f_t* pOutMatrix, uint32_t width, uint32_t height, float near, float far)
{
    memset(pOutMatrix, 0, sizeof(matrix4x4f_t));

    pOutMatrix->m00 = 1.0f / (float)width;
    pOutMatrix->m11 = 1.0f / (float)height;
    pOutMatrix->m22 = -2.0f / (far-near);
    pOutMatrix->m23 = -((far+near)/(far-near));
    pOutMatrix->m33 = 1.0f;
}

void k15_set_identity_matrix4x4f(matrix4x4f_t* pMatrix)
{
    memcpy(pMatrix, IdentityMatrix4x4, sizeof(IdentityMatrix4x4));
}

internal bool _k15_matrix4x4f_is_equal(const matrix4x4f_t* restrict_modifier pA, const matrix4x4f_t* restrict_modifier pB)
{
    CompiletimeAssert(sizeof(matrix4x4f_t) == sizeof(float[16]));
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
                const uint32_t index = x + y * pContext->colorBufferStride;
                pBackBuffer[index]=0xFF0000FF;
            }
        }
        else if((y%3)==0)
        {
            for(uint32_t x = 0; x < pContext->backBufferWidth; ++x)
            {
                const uint32_t index = x + y * pContext->colorBufferStride;
                pBackBuffer[index]=0xFFFF0000;
            }
        }
        else if((y%2)==0)
        {
            for(uint32_t x = 0; x < pContext->backBufferWidth; ++x)
            {
                const uint32_t index = x + y * pContext->colorBufferStride;
                pBackBuffer[index]=0xFF00FF00;
            }
        }
        else
        {
            for(uint32_t x = 0; x < pContext->backBufferWidth; ++x)
            {
                const uint32_t index = x + y * pContext->colorBufferStride;
                pBackBuffer[index]=0xFFFFFFFF;
            }
        }
    } 
}

internal vector4f_t _k15_mul_vector4_matrix44(const vector4f_t* pVector, const matrix4x4f_t* pMatrix)
{
    vector4f_t multipliedVector = {};
    multipliedVector.x = pVector->x * pMatrix->m00 + pVector->y * pMatrix->m01 + pVector->z * pMatrix->m02 + pVector->w * pMatrix->m03;
    multipliedVector.y = pVector->x * pMatrix->m10 + pVector->y * pMatrix->m11 + pVector->z * pMatrix->m12 + pVector->w * pMatrix->m13;
    multipliedVector.z = pVector->x * pMatrix->m20 + pVector->y * pMatrix->m21 + pVector->z * pMatrix->m22 + pVector->w * pMatrix->m23;
    multipliedVector.w = pVector->x * pMatrix->m30 + pVector->y * pMatrix->m31 + pVector->z * pMatrix->m32 + pVector->w * pMatrix->m33;
    return multipliedVector;
}

internal void _k15_mul_multiple_vector4_matrix44(vector4f_t* pVectors, const matrix4x4f_t* pMatrix, uint32_t vectorCount)
{
    for(uint32_t vectorIndex = 0; vectorIndex < vectorCount; ++vectorIndex)
    {
        pVectors[vectorIndex] = _k15_mul_vector4_matrix44(pVectors + vectorIndex, pMatrix);
    }
}

//http://www.edepot.com/linee.html
internal void _k15_draw_line(void* pColorBuffer, uint32_t colorBufferStride, const vector2f_t* pVectorA, const vector2f_t* pVectorB, uint32_t color)
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

internal inline uint32_t float_to_uint32_unsafe(float value)
{
    return (uint32_t)(value + 0.5f);
}

internal inline uint8_t float_to_uint8(float value)
{
    RuntimeAssert(value >= 0.0f && value <= (float)UINT8_MAX);
    return (uint8_t)value;
}

internal inline uint32_t float_to_uint32(float value)
{
    RuntimeAssert(value >= 0.0f && value <= (float)UINT_MAX);
    return (uint32_t)(value + 0.5f);
}

internal inline uint32_t float_to_int32(float value)
{
    RuntimeAssert(value >= (float)INT_MIN && value <= (float)INT_MAX)
    return (int32_t)(value + 0.5f);
}

internal vector4f_t k15_vector4f_hadamard(vector4f_t a, vector4f_t b)
{
    vector4f_t vector = a;
    vector.x *= b.x;
    vector.y *= b.y;
    vector.z *= b.z;
    vector.w *= b.w;

    return vector;
}

internal vector4f_t k15_vector4f_scale(vector4f_t a, float scale)
{
    vector4f_t vector = a;
    vector.x *= scale;
    vector.y *= scale;
    vector.z *= scale;
    vector.w *= scale;

    return vector;
}

internal vector4f_t k15_vector4f_add(vector4f_t a, vector4f_t b)
{
    vector4f_t vector;
    vector.x = a.x + b.x;
    vector.y = a.y + b.y;
    vector.z = a.z + b.z;
    vector.w = a.w + b.w;

    return vector;
}

internal vector4f_t k15_vector4f_clamp01(vector4f_t v)
{
    vector4f_t vector;
    vector.x = clamp(v.x, 0.0f, 1.0f);
    vector.y = clamp(v.y, 0.0f, 1.0f);
    vector.z = clamp(v.z, 0.0f, 1.0f);
    vector.w = clamp(v.w, 0.0f, 1.0f);

    return vector;
}

internal vector3f_t k15_vector3f_scale(vector3f_t a, float scale)
{
    vector3f_t vector = a;
    vector.x *= scale;
    vector.y *= scale;
    vector.z *= scale;

    return vector;
}

internal vector3f_t k15_vector3f_add(vector3f_t a, vector3f_t b)
{
    vector3f_t vector;
    vector.x = a.x + b.x;
    vector.y = a.y + b.y;
    vector.z = a.z + b.z;

    return vector;
}

internal vector2f_t k15_vector2f_scale(vector2f_t a, float scale)
{
    vector2f_t vector = a;
    vector.x *= scale;
    vector.y *= scale;

    return vector;
}

internal vector2f_t k15_vector2f_add(vector2f_t a, vector2f_t b)
{
    vector2f_t vector;
    vector.x = a.x + b.x;
    vector.y = a.y + b.y;

    return vector;
}

internal vector2f_t k15_vector2f_sub(vector2f_t a, vector2f_t b)
{
    vector2f_t vector;
    vector.x = a.x - b.x;
    vector.y = a.y - b.y;

    return vector;
}

internal float k15_vector2f_dot(vector2f_t a, vector2f_t b)
{
    return a.x * b.x + a.y * b.y;
}

internal float k15_vector3f_dot(vector3f_t a, vector3f_t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

internal inline constexpr vector4f_t k15_create_vector4f(float x, float y, float z, float w)
{
    return {x, y, z, w};
}

internal inline constexpr vector3f_t k15_create_vector3f(float x, float y, float z)
{
    return {x, y, z};
}

internal inline constexpr vector2f_t k15_create_vector2f(float x, float y)
{
    return {x, y};
}

internal inline void _k15_repeat_texcoords(const vertex_t* pVertices, vector2f_t* pTexcoords, uint32_t texcoordCount)
{
#ifndef K15_RELEASE_BUILD
    for( uint32_t texCoordIndex = 0u; texCoordIndex < texcoordCount; ++texCoordIndex)
    {
        RuntimeAssert(pVertices[texCoordIndex].texcoord.x < 2.0f);
        RuntimeAssert(pVertices[texCoordIndex].texcoord.y < 2.0f);
    }
#endif

    const uint32_t texcoordCountRest = texcoordCount & 0x3;
    const uint32_t texcoordCountSIMD = texcoordCount - texcoordCountRest;

    const uint32_t texcoordOffsetInFloats = offsetof(vertex_t, vertex_t::texcoord) / sizeof(float);
    const uint32_t vertexSizeInFloats = sizeof(vertex_t) / sizeof(float);
    for( uint32_t texcoordIndex = 0u; texcoordIndex < texcoordCountSIMD; texcoordIndex += 4u )
    {
        __m256i texcoordsIndices = _mm256_add_epi32(_mm256_set1_epi32(texcoordIndex), _mm256_set_epi32(3, 3, 2, 2, 1, 1, 0, 0));
        texcoordsIndices = _mm256_mullo_epi32(texcoordsIndices, _mm256_set1_epi32(vertexSizeInFloats));
        texcoordsIndices = _mm256_add_epi32(texcoordsIndices, _mm256_set_epi32(texcoordOffsetInFloats+1, texcoordOffsetInFloats, texcoordOffsetInFloats+1, texcoordOffsetInFloats, texcoordOffsetInFloats+1, texcoordOffsetInFloats ,texcoordOffsetInFloats+1, texcoordOffsetInFloats));
        __m256 vertexTexcoords = _mm256_i32gather_ps((const float*)pVertices, texcoordsIndices, 4);
        
        __m256 mask = _mm256_cmp_ps(vertexTexcoords, _mm256_set1_ps(1.0f), _CMP_GT_OQ);
        vertexTexcoords = _mm256_sub_ps(vertexTexcoords, _mm256_and_ps(_mm256_set1_ps(1.0f), mask));
        _mm256_store_ps((float*)(pTexcoords + texcoordIndex), vertexTexcoords);
    }

    for( uint32_t texCoordIndex = texcoordCountSIMD; texCoordIndex < texcoordCount; ++texCoordIndex)
    {
        if( pVertices[texCoordIndex].texcoord.x > 1.0f )
        {
            pTexcoords[texCoordIndex].x = (pVertices[texCoordIndex].texcoord.x - 1.0f);
        }

        if( pVertices[texCoordIndex].texcoord.y > 1.0f )
        {
            pTexcoords[texCoordIndex].y = (pVertices[texCoordIndex].texcoord.y - 1.0f);
        }
    }
}

internal inline void _k15_clamp_texcoords(const vertex_t* pVertices, vector2f_t* pTexcoords, uint32_t texcoordCount)
{
    const uint32_t texcoordCountRest = texcoordCount & 0x3;
    const uint32_t texcoordCountSIMD = texcoordCount - texcoordCountRest;

    const uint32_t texcoordOffsetInFloats = offsetof(vertex_t, vertex_t::texcoord) / sizeof(float);
    const uint32_t vertexSizeInFloats = sizeof(vertex_t) / sizeof(float);

    for( uint32_t texcoordIndex = 0u; texcoordIndex < texcoordCountSIMD; texcoordIndex += 4u )
    {
        __m256i texcoordsIndices = _mm256_add_epi32(_mm256_set1_epi32(texcoordIndex), _mm256_set_epi32(3, 3, 2, 2, 1, 1, 0, 0));
        texcoordsIndices = _mm256_mullo_epi32(texcoordsIndices, _mm256_set1_epi32(vertexSizeInFloats));
        texcoordsIndices = _mm256_add_epi32(texcoordsIndices, _mm256_set_epi32(texcoordOffsetInFloats+1, texcoordOffsetInFloats, texcoordOffsetInFloats+1, texcoordOffsetInFloats, texcoordOffsetInFloats+1, texcoordOffsetInFloats ,texcoordOffsetInFloats+1, texcoordOffsetInFloats));
        __m256 vertexTexcoords = _mm256_i32gather_ps((const float*)pVertices, texcoordsIndices, 4);
        
        __m256 minMask = _mm256_cmp_ps(vertexTexcoords, _mm256_set1_ps(0.0f), _CMP_LT_OQ);
        __m256 maxMask = _mm256_cmp_ps(vertexTexcoords, _mm256_set1_ps(1.0f), _CMP_GT_OQ);

        vertexTexcoords = _mm256_blendv_ps(vertexTexcoords, _mm256_set1_ps(0.0f), minMask);
        vertexTexcoords = _mm256_blendv_ps(vertexTexcoords, _mm256_set1_ps(1.0f), maxMask);
        _mm256_store_ps((float*)(pTexcoords + texcoordIndex), vertexTexcoords);
    }

    for( uint32_t texCoordIndex = texcoordCountSIMD; texCoordIndex < texcoordCount; ++texCoordIndex)
    {
        pTexcoords[texCoordIndex].x = clamp01f(pVertices[texCoordIndex].texcoord.x);
        pTexcoords[texCoordIndex].y = clamp01f(pVertices[texCoordIndex].texcoord.y);
    }
}

internal inline void _k15_mirror_texcoords(const vertex_t* pVertices, vector2f_t* pTexcoords, uint32_t texcoordCount)
{
#ifndef K15_RELEASE_BUILD
    for( uint32_t texCoordIndex = 0u; texCoordIndex < texcoordCount; ++texCoordIndex)
    {
        RuntimeAssert(pVertices[texCoordIndex].texcoord.x < 2.0f);
        RuntimeAssert(pVertices[texCoordIndex].texcoord.y < 2.0f);
    }
#endif

    const uint32_t texcoordCountRest = texcoordCount & 0x3;
    const uint32_t texcoordCountSIMD = texcoordCount - texcoordCountRest;

    const uint32_t texcoordOffsetInFloats = offsetof(vertex_t, vertex_t::texcoord) / sizeof(float);
    const uint32_t vertexSizeInFloats = sizeof(vertex_t) / sizeof(float);
    for( uint32_t texcoordIndex = 0u; texcoordIndex < texcoordCountSIMD; texcoordIndex += 4u )
    {
        __m256i texcoordsIndices = _mm256_add_epi32(_mm256_set1_epi32(texcoordIndex), _mm256_set_epi32(3, 3, 2, 2, 1, 1, 0, 0));
        texcoordsIndices = _mm256_mullo_epi32(texcoordsIndices, _mm256_set1_epi32(vertexSizeInFloats));
        texcoordsIndices = _mm256_add_epi32(texcoordsIndices, _mm256_set_epi32(texcoordOffsetInFloats+1, texcoordOffsetInFloats, texcoordOffsetInFloats+1, texcoordOffsetInFloats, texcoordOffsetInFloats+1, texcoordOffsetInFloats ,texcoordOffsetInFloats+1, texcoordOffsetInFloats));
        __m256 vertexTexcoords = _mm256_i32gather_ps((const float*)pVertices, texcoordsIndices, 4);
        
        __m256 mask = _mm256_cmp_ps(vertexTexcoords, _mm256_set1_ps(1.0f), _CMP_GT_OQ);
        vertexTexcoords = _mm256_sub_ps(_mm256_set1_ps(1.0f), _mm256_sub_ps(vertexTexcoords, _mm256_and_ps(_mm256_set1_ps(1.0f), mask)));
        _mm256_store_ps((float*)(pTexcoords + texcoordIndex), vertexTexcoords);
    }

    for( uint32_t texCoordIndex = texcoordCountSIMD; texCoordIndex < texcoordCount; ++texCoordIndex)
    {
        if( pVertices[texCoordIndex].texcoord.x > 1.0f )
        {
            pTexcoords[texCoordIndex].x = 1.0f - (pVertices[texCoordIndex].texcoord.x - 1.0f);
        }

        if( pVertices[texCoordIndex].texcoord.y > 1.0f )
        {
            pTexcoords[texCoordIndex].y = 1.0f - (pVertices[texCoordIndex].texcoord.y - 1.0f);
        }
    }
}

template<int TEXTURE_COMPONENT_COUNT, sample_addressing_mode_t ADDRESSING_MODE>
texture_samples_t _k15_sample_texture_components(const texture_t* restrict_modifier pTextureData, const pixel_shader_input_t* restrict_modifier pPixelShaderInput, uint32_t texcoordCount)
{
    constexpr uint32_t TexcoordBatchCount = 512;
    vector2f_t texCoords[TexcoordBatchCount];
    texture_samples_t samples = {};
    samples.pColors = (vector4f_t*)_k15_allocate_from_stack_allocator(pPixelShaderInput->pStackAllocator, sizeof(vector4f_t) * texcoordCount);
    RuntimeAssert(samples.pColors != nullptr);

    for(uint32_t texcoordIndex = 0u; texcoordIndex < texcoordCount; texcoordIndex += TexcoordBatchCount)
    {
        uint32_t currentTexCoordBatchCount = get_min(texcoordCount - texcoordIndex, TexcoordBatchCount);

        switch( ADDRESSING_MODE )
        {
            case sample_addressing_mode_t::repeat:
            _k15_repeat_texcoords(pPixelShaderInput->pVertexData + texcoordIndex, texCoords, currentTexCoordBatchCount);
            break;

            case sample_addressing_mode_t::clamp:
            _k15_clamp_texcoords(pPixelShaderInput->pVertexData + texcoordIndex, texCoords, currentTexCoordBatchCount);
            break;

            case sample_addressing_mode_t::mirror:
            _k15_mirror_texcoords(pPixelShaderInput->pVertexData + texcoordIndex, texCoords, currentTexCoordBatchCount);
            break;
        }

        const uint32_t width = pTextureData->width - 1u;
        const uint32_t height = pTextureData->height - 1u;
        const uint32_t stride = pTextureData->stride;
        const uint32_t currentTexcoordBatchRest = currentTexCoordBatchCount & 0x3;
        currentTexCoordBatchCount -= currentTexcoordBatchRest;

        const uint8_t* restrict_modifier pTextureImageData = (uint8_t*)pTextureData->pData;
        if( currentTexCoordBatchCount > 0u )
        {
            for( uint32_t batchTexcoordIndex = 0u; batchTexcoordIndex < currentTexCoordBatchCount; batchTexcoordIndex += 4u)
            {
                const uint32_t x[] = {
                    float_to_uint32(texCoords[batchTexcoordIndex + 0].x * (float)width),
                    float_to_uint32(texCoords[batchTexcoordIndex + 1].x * (float)width),
                    float_to_uint32(texCoords[batchTexcoordIndex + 2].x * (float)width),
                    float_to_uint32(texCoords[batchTexcoordIndex + 3].x * (float)width),
                };

                const uint32_t y[] = {
                    height - float_to_uint32_unsafe(texCoords[batchTexcoordIndex + 0].y * (float)height),
                    height - float_to_uint32_unsafe(texCoords[batchTexcoordIndex + 1].y * (float)height),
                    height - float_to_uint32_unsafe(texCoords[batchTexcoordIndex + 2].y * (float)height),
                    height - float_to_uint32_unsafe(texCoords[batchTexcoordIndex + 3].y * (float)height)
                };

                const uint32_t texelIndices[] = {
                    x[0] + y[0] * stride,
                    x[1] + y[1] * stride,
                    x[2] + y[2] * stride,
                    x[3] + y[3] * stride
                };

                switch(TEXTURE_COMPONENT_COUNT)
                {
                    case 4u:
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 0].w = (float)pTextureImageData[texelIndices[0] * TEXTURE_COMPONENT_COUNT + 3] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 1].w = (float)pTextureImageData[texelIndices[1] * TEXTURE_COMPONENT_COUNT + 3] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 2].w = (float)pTextureImageData[texelIndices[2] * TEXTURE_COMPONENT_COUNT + 3] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 3].w = (float)pTextureImageData[texelIndices[3] * TEXTURE_COMPONENT_COUNT + 3] / 255.f;
                    case 3u:
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 0].z = (float)pTextureImageData[texelIndices[0] * TEXTURE_COMPONENT_COUNT + 2] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 1].z = (float)pTextureImageData[texelIndices[1] * TEXTURE_COMPONENT_COUNT + 2] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 2].z = (float)pTextureImageData[texelIndices[2] * TEXTURE_COMPONENT_COUNT + 2] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 3].z = (float)pTextureImageData[texelIndices[3] * TEXTURE_COMPONENT_COUNT + 2] / 255.f;
                    case 2u:
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 0].y = (float)pTextureImageData[texelIndices[0] * TEXTURE_COMPONENT_COUNT + 1] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 1].y = (float)pTextureImageData[texelIndices[1] * TEXTURE_COMPONENT_COUNT + 1] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 2].y = (float)pTextureImageData[texelIndices[2] * TEXTURE_COMPONENT_COUNT + 1] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 3].y = (float)pTextureImageData[texelIndices[3] * TEXTURE_COMPONENT_COUNT + 1] / 255.f;
                    case 1u:
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 0].x = (float)pTextureImageData[texelIndices[0] * TEXTURE_COMPONENT_COUNT + 0] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 1].x = (float)pTextureImageData[texelIndices[1] * TEXTURE_COMPONENT_COUNT + 0] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 2].x = (float)pTextureImageData[texelIndices[2] * TEXTURE_COMPONENT_COUNT + 0] / 255.f;
                        samples.pColors[texcoordIndex + batchTexcoordIndex + 3].x = (float)pTextureImageData[texelIndices[3] * TEXTURE_COMPONENT_COUNT + 0] / 255.f;
                        break;

                    default:
                        RuntimeAssert(false);
                }
            }
        }

        for( uint32_t singleTexcoordIndex = 0u; singleTexcoordIndex < currentTexcoordBatchRest; ++singleTexcoordIndex )
        {
            const uint32_t globalTexcoordIndex = currentTexCoordBatchCount + texcoordIndex + singleTexcoordIndex;
            const uint32_t x = float_to_uint32(texCoords[singleTexcoordIndex].x * (float)width);
            const uint32_t y = height - float_to_uint32(texCoords[singleTexcoordIndex].y * (float)height);
            const uint32_t texelIndex = x + y * stride;

            switch(TEXTURE_COMPONENT_COUNT)
            {
                case 4u:
                    samples.pColors[globalTexcoordIndex].x = (float)pTextureImageData[texelIndex * TEXTURE_COMPONENT_COUNT + 0] / 255.f;
                    samples.pColors[globalTexcoordIndex].y = (float)pTextureImageData[texelIndex * TEXTURE_COMPONENT_COUNT + 1] / 255.f;
                    samples.pColors[globalTexcoordIndex].z = (float)pTextureImageData[texelIndex * TEXTURE_COMPONENT_COUNT + 2] / 255.f;
                    samples.pColors[globalTexcoordIndex].w = (float)pTextureImageData[texelIndex * TEXTURE_COMPONENT_COUNT + 3] / 255.f;
                    break;
                case 3u:
                    samples.pColors[globalTexcoordIndex].x = (float)pTextureImageData[texelIndex * TEXTURE_COMPONENT_COUNT + 0] / 255.f;
                    samples.pColors[globalTexcoordIndex].y = (float)pTextureImageData[texelIndex * TEXTURE_COMPONENT_COUNT + 1] / 255.f;
                    samples.pColors[globalTexcoordIndex].z = (float)pTextureImageData[texelIndex * TEXTURE_COMPONENT_COUNT + 2] / 255.f;
                    break;
                case 2u:
                    samples.pColors[globalTexcoordIndex].x = (float)pTextureImageData[texelIndex * TEXTURE_COMPONENT_COUNT + 0] / 255.f;
                    samples.pColors[globalTexcoordIndex].y = (float)pTextureImageData[texelIndex * TEXTURE_COMPONENT_COUNT + 1] / 255.f;
                    break;
                case 1u:
                    samples.pColors[globalTexcoordIndex].x = (float)pTextureImageData[texelIndex * TEXTURE_COMPONENT_COUNT + 0] / 255.f;
                    break;

                default:
                    RuntimeAssert(false);
            }
        }
    }

    return samples;
}

template<sample_addressing_mode_t ADDRESSING_MODE>
texture_samples_t k15_sample_texture(texture_handle_t texture, const pixel_shader_input_t* pPixelShaderInput, uint32_t texcoordCount)
{
    RuntimeAssert(texcoordCount <= PixelShaderInputCount);
    texture_t* pTextureData = (texture_t*)texture.pHandle;

    texture_samples_t samples;
    switch(pTextureData->componentCount)
    {
        case 1u:
            samples = _k15_sample_texture_components<1u, ADDRESSING_MODE>(pTextureData, pPixelShaderInput, texcoordCount);
            break;

        case 2u:
            samples = _k15_sample_texture_components<2u, ADDRESSING_MODE>(pTextureData, pPixelShaderInput, texcoordCount);
            break;

        case 3u:
            samples = _k15_sample_texture_components<3u, ADDRESSING_MODE>(pTextureData, pPixelShaderInput, texcoordCount);
            break;

        case 4u:
            samples = _k15_sample_texture_components<4u, ADDRESSING_MODE>(pTextureData, pPixelShaderInput, texcoordCount);
            break;

        default:
            RuntimeAssert(false);
    }

    return samples;
}

constexpr vertex_t k15_create_vertex(vector4f_t position, vector4f_t normal, vector4f_t color, vector2f_t texcoord)
{
    vertex_t vertex = {};
    vertex.position = position;
    vertex.normal = normal;
    vertex.color = color;
    vertex.texcoord = texcoord;

    return vertex;
}

internal void _k15_generate_barycentric_vertices(pixel_shader_input_t* pOutVertex, barycentric_coordinates_buffer_t barycentricCoordinates, uint32_t barycentricCoordinateCount, const vertex_t* pTriangleVertices)
{
    float* restrict_modifier pOutputVertexAttributes = (float*)pOutVertex->pVertexData;
    const float* restrict_modifier pInputVertexAttributes[3] = {
        (const float*)&pTriangleVertices[0],
        (const float*)&pTriangleVertices[1],
        (const float*)&pTriangleVertices[2]
    };

    const uint32_t attributeCount = sizeof(vertex_t) / sizeof(float);
    uint32_t globalAttributeIndex = 0u;
    for( uint32_t baryIndex = 0u; baryIndex < barycentricCoordinateCount; ++baryIndex )
    {
        const __m128 uWide = _mm_load1_ps(barycentricCoordinates.pU + baryIndex);
        const __m128 vWide = _mm_load1_ps(barycentricCoordinates.pV + baryIndex);
        const __m128 wWide = _mm_sub_ps(_mm_set1_ps(1.0f), _mm_add_ps(uWide, vWide));

        const uint32_t scalarAttributeCount = attributeCount & 0x3;
        const uint32_t simdAttributeCount = attributeCount - scalarAttributeCount; 
        for( uint32_t attributeIndex = 0u; attributeIndex < simdAttributeCount; attributeIndex += 4u )
        {
            const __m128 vertexAttributes[] = {
                _mm_load_ps(pInputVertexAttributes[0] + attributeIndex),
                _mm_load_ps(pInputVertexAttributes[1] + attributeIndex),
                _mm_load_ps(pInputVertexAttributes[2] + attributeIndex)
            };

            __m128 transformedVertexAttributes = _mm_mul_ps(vertexAttributes[2], uWide);
            transformedVertexAttributes = _mm_fmadd_ps(vertexAttributes[1], wWide, transformedVertexAttributes);
            transformedVertexAttributes = _mm_fmadd_ps(vertexAttributes[0], vWide, transformedVertexAttributes);
            
            _mm_store_ps(pOutputVertexAttributes + globalAttributeIndex, transformedVertexAttributes);
            globalAttributeIndex += 4u;
        }

        for( uint32_t attributeIndex = simdAttributeCount; attributeIndex < attributeCount; ++attributeIndex )
        {
            const float attributes[3] = {
                pInputVertexAttributes[0][attributeIndex],
                pInputVertexAttributes[1][attributeIndex],
                pInputVertexAttributes[2][attributeIndex]
            };

            const float u = barycentricCoordinates.pU[baryIndex];
            const float v = barycentricCoordinates.pV[baryIndex];
            const float w = 1.0f - u - v;

            const float transformedAttribute = attributes[0] * v + attributes[1] * w + attributes[2] * u;
            pOutputVertexAttributes[globalAttributeIndex] = transformedAttribute;
            ++globalAttributeIndex;
        }
    }
}

internal inline float _k15_edge_function(vector2f_t a, vector2f_t b, vector2f_t c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

internal void _k15_write_color_to_color_buffer(const pixel_shader_output_t* pPixelShaderOutput, uint32_t pixelCount, uint32_t* pColorBufferContent, uint32_t colorBufferStride, uint8_t redShift, uint8_t greenShift, uint8_t blueShift)
{
    MemoryPrefetch0(pPixelShaderOutput->pColor);
    MemoryPrefetch0(pPixelShaderOutput->pScreenspaceX);
    MemoryPrefetch0(pPixelShaderOutput->pScreenspaceY);

    const uint32_t restPixelCount = pixelCount & 0x3;
    const uint32_t widePixelCount = pixelCount - restPixelCount;

    for( uint32_t pixelIndex = 0; pixelIndex < widePixelCount; pixelIndex += 4u)
    {
        const uint8_t red[4u] = {
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 0].x) * 255.f),
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 1].x) * 255.f),
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 2].x) * 255.f),
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 3].x) * 255.f),
        };

        const uint8_t green[4u] = {
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 0].y) * 255.f),
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 1].y) * 255.f),
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 2].y) * 255.f),
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 3].y) * 255.f),
        };

        const uint8_t blue[4u] = {
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 0].z) * 255.f),
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 1].z) * 255.f),
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 2].z) * 255.f),
            float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex + 3].z) * 255.f),
        };
        
        const uint32_t colors[4u] = {
            (uint32_t)(red[0] << redShift | green[0] << greenShift | blue[0] << blueShift),
            (uint32_t)(red[1] << redShift | green[1] << greenShift | blue[1] << blueShift),
            (uint32_t)(red[2] << redShift | green[2] << greenShift | blue[2] << blueShift),
            (uint32_t)(red[3] << redShift | green[3] << greenShift | blue[3] << blueShift),
        };

        const uint32_t colorMapIndices[4u] = {
            pPixelShaderOutput->pScreenspaceX[pixelIndex + 0] + pPixelShaderOutput->pScreenspaceY[pixelIndex + 0] * colorBufferStride,
            pPixelShaderOutput->pScreenspaceX[pixelIndex + 1] + pPixelShaderOutput->pScreenspaceY[pixelIndex + 1] * colorBufferStride,
            pPixelShaderOutput->pScreenspaceX[pixelIndex + 2] + pPixelShaderOutput->pScreenspaceY[pixelIndex + 2] * colorBufferStride,
            pPixelShaderOutput->pScreenspaceX[pixelIndex + 3] + pPixelShaderOutput->pScreenspaceY[pixelIndex + 3] * colorBufferStride,
        };

        pColorBufferContent[colorMapIndices[0]] = colors[0];
        pColorBufferContent[colorMapIndices[1]] = colors[1];
        pColorBufferContent[colorMapIndices[2]] = colors[2];
        pColorBufferContent[colorMapIndices[3]] = colors[3];
    }

    for(uint32_t pixelIndex = widePixelCount; pixelIndex < pixelCount; ++pixelIndex)
    {
        const uint8_t red   = float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex].x) * 255.f);
        const uint8_t green = float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex].y) * 255.f);
        const uint8_t blue  = float_to_uint8(clamp01f(pPixelShaderOutput->pColor[pixelIndex].z) * 255.f);

        const uint32_t color = red << redShift | green << greenShift | blue << blueShift;
        const uint32_t colorMapIndex = pPixelShaderOutput->pScreenspaceX[pixelIndex] + pPixelShaderOutput->pScreenspaceY[pixelIndex] * colorBufferStride;
        pColorBufferContent[colorMapIndex] = color;
    }
}

template<bool DEPTH_WRITE_ENABLED = true>
internal void _k15_draw_triangles_4_step(draw_call_triangles_t* pDrawCallTriangles, pixel_shader_input_t pixelShaderInput, pixel_shader_output_t pixelShaderOutput, barycentric_coordinates_buffer_t barycentricCoordinates, void* pColorBuffer, void* pDepthBuffer, uint32_t colorBufferStride, uint32_t depthBufferStride, uint8_t redShift, uint8_t greenShift, uint8_t blueShift)
{
    const void* pUniformData = pDrawCallTriangles->pUniformData;
    pixel_shader_fnc_t pixelShader = pDrawCallTriangles->pixelShader;

    uint32_t* pColorBufferContent = (uint32_t*)pColorBuffer;
    float* pDepthBufferContent = (float*)pDepthBuffer;

    for(uint32_t triangleIndex = 0; triangleIndex < pDrawCallTriangles->screenspaceTriangleCount; ++triangleIndex)
    {
        const screenspace_triangle_t* pTriangle = pDrawCallTriangles->pScreenspaceTriangles + triangleIndex;

        const vector2f_t v0 = k15_create_vector2f(pTriangle->screenspaceVertexPositions[0].x, pTriangle->screenspaceVertexPositions[0].y);
        const vector2f_t v1 = k15_create_vector2f(pTriangle->screenspaceVertexPositions[1].x, pTriangle->screenspaceVertexPositions[1].y);
        const vector2f_t v2 = k15_create_vector2f(pTriangle->screenspaceVertexPositions[2].x, pTriangle->screenspaceVertexPositions[2].y);
        const float triangleArea = _k15_edge_function(v2, v1, v0);
        const float oneOverTriangleArea = 1.0f / triangleArea;

        const __m128 v0WideX = _mm_set_ps1(v0.x);
        const __m128 v0WideY = _mm_set_ps1(v0.y);
        const __m128 v1WideX = _mm_set_ps1(v1.x);
        const __m128 v1WideY = _mm_set_ps1(v1.y);
        const __m128 v2WideX = _mm_set_ps1(v2.x);
        const __m128 v2WideY = _mm_set_ps1(v2.y);

        const __m128 v0WideZ = _mm_set_ps1(pTriangle->screenspaceVertexPositions[0].z);
        const __m128 v1WideZ = _mm_set_ps1(pTriangle->screenspaceVertexPositions[1].z);
        const __m128 v2WideZ = _mm_set_ps1(pTriangle->screenspaceVertexPositions[2].z);

        const __m128 edge0Term0 = _mm_sub_ps(v0WideX, v1WideX);
        const __m128 edge0Term2 = _mm_sub_ps(v0WideY, v1WideY);
        const __m128 edge1Term0 = _mm_sub_ps(v1WideX, v2WideX);
        const __m128 edge1Term2 = _mm_sub_ps(v1WideY, v2WideY);
        
        const __m128 triangleAreaWide = _mm_fmsub_ps(_mm_sub_ps(v1WideX, v2WideX), _mm_sub_ps(v0WideY, v2WideY), _mm_mul_ps(_mm_sub_ps(v1WideY, v2WideY), _mm_sub_ps(v0WideX, v2WideX)));
        const __m128 oneOverTriangleAreaWide = _mm_div_ps(_mm_set1_ps(1.0f), triangleAreaWide);

        for(uint32_t y = pTriangle->boundingBox.y1; y < pTriangle->boundingBox.y2; y += PixelShaderTileSize)
        {
            const uint32_t yDelta = (pTriangle->boundingBox.y2 - y);
            const uint32_t yStep = get_min(PixelShaderTileSize, yDelta);
            const uint32_t tileYEnd = y + yStep;

            for(uint32_t x = pTriangle->boundingBox.x1; x < pTriangle->boundingBox.x2; x += PixelShaderTileSize)
            {   
                const uint32_t xDelta = (pTriangle->boundingBox.x2 - x);
                const uint32_t xStep = get_min(PixelShaderTileSize, xDelta);
                const uint32_t tileXEnd = x + xStep;

                uint32_t pixelIndex = 0;
                for( uint32_t tileY = y; tileY < tileYEnd; ++tileY)
                {
                    for( uint32_t tileX = x; tileX < tileXEnd; tileX += 4u)
                    {
                        const uint32_t depthBufferOffset = tileX + tileY * depthBufferStride;
                        const __m128 oldDepthBufferZ = _mm_load_ps(pDepthBufferContent + depthBufferOffset);

                        const __m128i pixelCoordinatesX = _mm_add_epi32(_mm_set1_epi32(tileX), _mm_set_epi32(3, 2, 1, 0));
                        const __m128i pixelCoordinatesY = _mm_set1_epi32(tileY);

                        const __m128 tileXWide = _mm_cvtepi32_ps(pixelCoordinatesX);
                        const __m128 tileYWide = _mm_cvtepi32_ps(pixelCoordinatesY);

                        const __m128 edge0Term1 = _mm_sub_ps(tileYWide, v1WideY);
                        const __m128 edge0Term3 = _mm_sub_ps(tileXWide, v1WideX);
                        const __m128 w0Wide     = _mm_fmsub_ps(edge0Term0, edge0Term1, _mm_mul_ps(edge0Term2, edge0Term3));
                        const __m128i w0Mask    = _mm_castps_si128(_mm_cmpgt_ps(w0Wide, _mm_setzero_ps()));

                        const __m128 edge1Term1 = _mm_sub_ps(tileYWide, v2WideY);
                        const __m128 edge1Term3 = _mm_sub_ps(tileXWide, v2WideX);
                        const __m128 w1Wide     = _mm_fmsub_ps(edge1Term0, edge1Term1, _mm_mul_ps(edge1Term2, edge1Term3));
                        const __m128i w1Mask    = _mm_castps_si128(_mm_cmpgt_ps(w1Wide,  _mm_setzero_ps()));

                        const __m128 w2Wide     = _mm_sub_ps(triangleAreaWide, _mm_add_ps(w0Wide, w1Wide));
                        const __m128i w2Mask    = _mm_castps_si128(_mm_cmpgt_ps(w2Wide,  _mm_setzero_ps()));

                        const __m128i pixelMask = _mm_and_si128(_mm_and_si128(w0Mask, w1Mask), w2Mask);
                        if(_mm_movemask_epi8(pixelMask) == 0)
                        {
                            continue;
                        }

                        const __m128 uWide = _mm_mul_ps(w0Wide, oneOverTriangleAreaWide);
                        const __m128 vWide = _mm_mul_ps(w1Wide, oneOverTriangleAreaWide);
                        const __m128 wWide = _mm_sub_ps(_mm_set1_ps(1.0f), _mm_add_ps(uWide, vWide));

                        const __m128 newDepthBufferZ = _mm_sub_ps(_mm_set1_ps(1.0f), _mm_fmadd_ps(v0WideZ, uWide, _mm_fmadd_ps(v1WideZ, vWide, _mm_mul_ps(v2WideZ, wWide))));

                        __m128i depthBufferMask = _mm_castps_si128(_mm_cmpgt_ps(newDepthBufferZ, oldDepthBufferZ));
                        depthBufferMask = _mm_and_si128(depthBufferMask, pixelMask);

                        if(_mm_movemask_epi8(depthBufferMask) == 0)
                        {
                            continue;
                        }

                        if( DEPTH_WRITE_ENABLED )
                        {
                            _mm_maskstore_ps(pDepthBufferContent + depthBufferOffset, depthBufferMask, newDepthBufferZ);
                        }

                        //FK: Extract 4-bit bit mask from depthBufferMask
                        __m128i pixelBits = _mm_srlv_epi32(depthBufferMask, _mm_set1_epi32(31));
                        pixelBits = _mm_sllv_epi32(pixelBits, _mm_set_epi32(3, 2, 1, 0));

                        __m128i pixelBitsMerge = _mm_hadd_epi32(pixelBits, _mm_setzero_si128());
                        pixelBitsMerge = _mm_hadd_epi32(pixelBitsMerge, _mm_setzero_si128());
                        const int outputBitMask = _mm_extract_epi32(pixelBitsMerge, 0);

                        const int outputBitMaskPopCnt = __popcnt(outputBitMask);
                        RuntimeAssert(outputBitMaskPopCnt > 0);

                        const int outputMaskLUTIndex = outputBitMaskPopCnt;
                        const int shuffleBitMaskLUTIndex = outputBitMask;
                        RuntimeAssert(outputMaskLUTIndex < 5);
                        RuntimeAssert(shuffleBitMaskLUTIndex < 16);
                        
                        const __m128i outputMask = _mm_load_si128((const __m128i*)(OutputBitMaskLUT4x[outputMaskLUTIndex]));
                        const uint8_t blendMask = ShuffleBitMaskLUT4x[shuffleBitMaskLUTIndex];

                        const __m128i blendMaskShift = _mm_set_epi32( 0, 2, 4, 6);
                        const __m128i blendMaskWide = _mm_and_si128(_mm_srav_epi32(_mm_set1_epi32(blendMask), blendMaskShift), _mm_set1_epi32(0b11));

                        const __m128 uWideShuffled = _mm_permutevar_ps(uWide, blendMaskWide);
                        const __m128 vWideShuffled = _mm_permutevar_ps(vWide, blendMaskWide);
                        const __m128i pixelCoordinatesXShuffled = _mm_cvtps_epi32(_mm_permutevar_ps(tileXWide, blendMaskWide));

                        _mm_maskstore_epi32((int*)(pixelShaderInput.pScreenspaceX + pixelIndex), outputMask, pixelCoordinatesXShuffled);
                        _mm_maskstore_epi32((int*)(pixelShaderInput.pScreenspaceY + pixelIndex), outputMask, pixelCoordinatesY);
                        _mm_maskstore_ps((barycentricCoordinates.pU + pixelIndex), outputMask, uWideShuffled);
                        _mm_maskstore_ps((barycentricCoordinates.pV + pixelIndex), outputMask, vWideShuffled);
                        const uint32_t pixelAddedThisIteration = outputBitMaskPopCnt;
                        pixelIndex += pixelAddedThisIteration;
                    }
                }

                const uint32_t pixelCount = pixelIndex;
                pixelIndex = 0;

                if( pixelCount == 0u )
                {
                    continue;
                }

                _k15_generate_barycentric_vertices(&pixelShaderInput, barycentricCoordinates, pixelCount, pTriangle->vertices);
                pixelShader(&pixelShaderInput, &pixelShaderOutput, pixelCount, pUniformData);
                _k15_reset_stack_allocator(pixelShaderInput.pStackAllocator);
                _k15_write_color_to_color_buffer(&pixelShaderOutput, pixelCount, pColorBufferContent, colorBufferStride, redShift, greenShift, blueShift);
                
            }
        }
    }
}

template<bool DEPTH_WRITE_ENABLED = true>
internal void _k15_draw_triangle_lines(draw_call_triangles_t* pDrawCallTriangles, pixel_shader_input_t pixelShaderInput, pixel_shader_output_t pixelShaderOutput, barycentric_coordinates_buffer_t barycentricCoordinates, void* pColorBuffer, void* pDepthBuffer, uint32_t colorBufferStride, uint32_t depthBufferStride, uint8_t redShift, uint8_t greenShift, uint8_t blueShift)
{
    const void* restrict_modifier pUniformData = pDrawCallTriangles->pUniformData;
    pixel_shader_fnc_t pixelShader = pDrawCallTriangles->pixelShader;

    uint32_t pixelCount = 0;
    uint32_t* restrict_modifier pColorBufferContent = (uint32_t* restrict_modifier)pColorBuffer;
    float* restrict_modifier pDepthBufferContent = (float* restrict_modifier)pDepthBuffer;

    for(uint32_t triangleIndex = 0; triangleIndex < pDrawCallTriangles->screenspaceTriangleCount; ++triangleIndex)
    {
        const screenspace_triangle_t* restrict_modifier pTriangle = pDrawCallTriangles->pScreenspaceTriangles + triangleIndex;

        for(uint32_t triangleVertexIndex = 0; triangleVertexIndex < 3u; ++triangleVertexIndex)
        {
            const uint32_t nextTriangleVertexIndex = (triangleVertexIndex + 1) % 3;
            const vector3f_t screenspaceVertexPositions[] = {
                pTriangle->screenspaceVertexPositions[triangleVertexIndex],
                pTriangle->screenspaceVertexPositions[nextTriangleVertexIndex]
            };

            int x = (int)screenspaceVertexPositions[0].x;
            int y = (int)screenspaceVertexPositions[0].y;
            int x2 = (int)screenspaceVertexPositions[1].x;
            int y2 = (int)screenspaceVertexPositions[1].y;

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
                        pixelShaderInput.pScreenspaceX[pixelCount] = localX;
                        pixelShaderInput.pScreenspaceY[pixelCount] = localY;
                        barycentricCoordinates.pU[pixelCount] = 0.0f;
                        barycentricCoordinates.pV[pixelCount] = 0.0f;
                        ++pixelCount;
                        j+=decInc;
                    }
                    continue;
                }
                longLen+=y;
                for (int j=0x8000+(x<<16);y>=longLen;--y) {
                    const uint32_t localX = j >> 16;
                    const uint32_t localY = y;
                    pixelShaderInput.pScreenspaceX[pixelCount] = localX;
                    pixelShaderInput.pScreenspaceY[pixelCount] = localY;
                    barycentricCoordinates.pU[pixelCount] = 0.0f;
                    barycentricCoordinates.pV[pixelCount] = 0.0f;
                    ++pixelCount;
                    j-=decInc;
                }
                continue;	
            }

            if (longLen>0) {
                longLen+=x;
                for (int j=0x8000+(y<<16);x<=longLen;++x) {
                    const uint32_t localX = x;
                    const uint32_t localY = j >> 16;
                    pixelShaderInput.pScreenspaceX[pixelCount] = localX;
                    pixelShaderInput.pScreenspaceY[pixelCount] = localY;
                    barycentricCoordinates.pU[pixelCount] = 0.0f;
                    barycentricCoordinates.pV[pixelCount] = 0.0f;
                    ++pixelCount;
                    j+=decInc;
                }
                continue;
            }
            longLen+=x;
            for (int j=0x8000+(y<<16);x>=longLen;--x) {
                const uint32_t localX = x;
                const uint32_t localY = j >> 16;
                pixelShaderInput.pScreenspaceX[pixelCount] = localX;
                pixelShaderInput.pScreenspaceY[pixelCount] = localY;
                barycentricCoordinates.pU[pixelCount] = 0.0f;
                barycentricCoordinates.pV[pixelCount] = 0.0f;
                ++pixelCount;
                j-=decInc;
            }
        }

        _k15_generate_barycentric_vertices(&pixelShaderInput, barycentricCoordinates, pixelCount, pTriangle->vertices);
        pixelShader(&pixelShaderInput, &pixelShaderOutput, pixelCount, pUniformData);
        _k15_reset_stack_allocator(pixelShaderInput.pStackAllocator);
        _k15_write_color_to_color_buffer(&pixelShaderOutput, pixelCount, pColorBufferContent, colorBufferStride, redShift, greenShift, blueShift);
    }
}

template<bool DEPTH_WRITE_ENABLED = true>
internal void _k15_draw_triangles_8_step(draw_call_triangles_t* pDrawCallTriangles, pixel_shader_input_t pixelShaderInput, pixel_shader_output_t pixelShaderOutput, barycentric_coordinates_buffer_t barycentricCoordinates, void* pColorBuffer, void* pDepthBuffer, uint32_t colorBufferStride, uint32_t depthBufferStride, uint8_t redShift, uint8_t greenShift, uint8_t blueShift)
{
    const void* restrict_modifier pUniformData = pDrawCallTriangles->pUniformData;
    pixel_shader_fnc_t pixelShader = pDrawCallTriangles->pixelShader;

    uint32_t* restrict_modifier pColorBufferContent = (uint32_t* restrict_modifier)pColorBuffer;
    float* restrict_modifier pDepthBufferContent = (float* restrict_modifier)pDepthBuffer;

    for(uint32_t triangleIndex = 0; triangleIndex < pDrawCallTriangles->screenspaceTriangleCount; ++triangleIndex)
    {
        const screenspace_triangle_t* restrict_modifier pTriangle = pDrawCallTriangles->pScreenspaceTriangles + triangleIndex;

        const vector2f_t v0 = k15_create_vector2f(pTriangle->screenspaceVertexPositions[0].x, pTriangle->screenspaceVertexPositions[0].y);
        const vector2f_t v1 = k15_create_vector2f(pTriangle->screenspaceVertexPositions[1].x, pTriangle->screenspaceVertexPositions[1].y);
        const vector2f_t v2 = k15_create_vector2f(pTriangle->screenspaceVertexPositions[2].x, pTriangle->screenspaceVertexPositions[2].y);
        const float triangleArea = _k15_edge_function(v2, v1, v0);
        const float oneOverTriangleArea = 1.0f / triangleArea;

        const __m256 v0WideX = _mm256_set1_ps(v0.x);
        const __m256 v0WideY = _mm256_set1_ps(v0.y);
        const __m256 v1WideX = _mm256_set1_ps(v1.x);
        const __m256 v1WideY = _mm256_set1_ps(v1.y);
        const __m256 v2WideX = _mm256_set1_ps(v2.x);
        const __m256 v2WideY = _mm256_set1_ps(v2.y);

        const __m256 v0WideZ = _mm256_set1_ps(pTriangle->screenspaceVertexPositions[0].z);
        const __m256 v1WideZ = _mm256_set1_ps(pTriangle->screenspaceVertexPositions[1].z);
        const __m256 v2WideZ = _mm256_set1_ps(pTriangle->screenspaceVertexPositions[2].z);

        const __m256 edge0Term0 = _mm256_sub_ps(v0WideX, v1WideX);
        const __m256 edge0Term2 = _mm256_sub_ps(v0WideY, v1WideY);
        const __m256 edge1Term0 = _mm256_sub_ps(v1WideX, v2WideX);
        const __m256 edge1Term2 = _mm256_sub_ps(v1WideY, v2WideY);
        
        const __m256 triangleAreaWide = _mm256_fmsub_ps(_mm256_sub_ps(v1WideX, v2WideX), _mm256_sub_ps(v0WideY, v2WideY), _mm256_mul_ps(_mm256_sub_ps(v1WideY, v2WideY), _mm256_sub_ps(v0WideX, v2WideX)));
        const __m256 oneOverTriangleAreaWide = _mm256_div_ps(_mm256_set1_ps(1.0f), triangleAreaWide);

        for(uint32_t y = pTriangle->boundingBox.y1; y < pTriangle->boundingBox.y2; y += PixelShaderTileSize)
        {
            MemoryPrefetchNTA(pDepthBufferContent + pTriangle->boundingBox.x1 + y * depthBufferStride);

            const uint32_t yDelta = (pTriangle->boundingBox.y2 - y);
            const uint32_t yStep = get_min(PixelShaderTileSize, yDelta);
            const uint32_t tileYEnd = y + yStep;

            for(uint32_t x = pTriangle->boundingBox.x1; x < pTriangle->boundingBox.x2; x += PixelShaderTileSize)
            {   
                const uint32_t xDelta = (pTriangle->boundingBox.x2 - x);
                const uint32_t xStep = get_min(PixelShaderTileSize, xDelta);
                const uint32_t tileXEnd = x + xStep;

                uint32_t pixelIndex = 0;
                for( uint32_t tileY = y; tileY < tileYEnd; ++tileY)
                {
                    for( uint32_t tileX = x; tileX < tileXEnd; tileX += 8u)
                    {
                        const uint32_t depthBufferOffset = tileX + tileY * depthBufferStride;

                        const __m256 pixelCoordinatesX = _mm256_add_ps(_mm256_set1_ps((float)tileX), _mm256_set_ps( 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f));
                        const __m256 pixelCoordinatesY = _mm256_set1_ps((float)tileY);

                        const __m256 edge0Term1 = _mm256_sub_ps(pixelCoordinatesY, v1WideY);
                        const __m256 edge0Term3 = _mm256_sub_ps(pixelCoordinatesX, v1WideX);
                        const __m256 w0Wide     = _mm256_fmsub_ps(edge0Term0, edge0Term1, _mm256_mul_ps(edge0Term2, edge0Term3));
                        const __m256i w0Mask    = _mm256_castps_si256(_mm256_cmp_ps(w0Wide, _mm256_setzero_ps(), _CMP_GT_OQ));

                        const __m256 edge1Term1 = _mm256_sub_ps(pixelCoordinatesY, v2WideY);
                        const __m256 edge1Term3 = _mm256_sub_ps(pixelCoordinatesX, v2WideX);
                        const __m256 w1Wide     = _mm256_fmsub_ps(edge1Term0, edge1Term1, _mm256_mul_ps(edge1Term2, edge1Term3));
                        const __m256i w1Mask    = _mm256_castps_si256(_mm256_cmp_ps(w1Wide,  _mm256_setzero_ps(), _CMP_GT_OQ));

                        const __m256 w2Wide     = _mm256_sub_ps(triangleAreaWide, _mm256_add_ps(w0Wide, w1Wide));
                        const __m256i w2Mask    = _mm256_castps_si256(_mm256_cmp_ps(w2Wide,  _mm256_setzero_ps(), _CMP_GT_OQ));

                        const __m256i pixelMask = _mm256_and_si256(_mm256_and_si256(w0Mask, w1Mask), w2Mask);
                        if( _mm256_movemask_epi8(pixelMask) == 0 )
                        {
                            continue;
                        }

                        const __m256 uWide = _mm256_mul_ps(w0Wide, oneOverTriangleAreaWide);
                        const __m256 vWide = _mm256_mul_ps(w1Wide, oneOverTriangleAreaWide);
                        const __m256 wWide = _mm256_sub_ps(_mm256_set1_ps(1.0f), _mm256_add_ps(uWide, vWide));

                        const __m256 newDepthBufferZ = _mm256_sub_ps(_mm256_set1_ps(1.0f), _mm256_fmadd_ps(v0WideZ, uWide, _mm256_fmadd_ps(v1WideZ, vWide, _mm256_mul_ps(v2WideZ, wWide))));
                        const __m256 oldDepthBufferZ = _mm256_load_ps(pDepthBufferContent + depthBufferOffset);

                        __m256i depthBufferMask = _mm256_castps_si256(_mm256_cmp_ps(newDepthBufferZ, oldDepthBufferZ, _CMP_GT_OQ));
                        depthBufferMask = _mm256_and_si256(depthBufferMask, pixelMask);
                        if( _mm256_movemask_epi8(depthBufferMask) == 0 )
                        {
                            continue;
                        }

                        if( DEPTH_WRITE_ENABLED )
                        {
                            _mm256_maskstore_ps(pDepthBufferContent + depthBufferOffset, depthBufferMask, newDepthBufferZ);
                        }

                        //FK: Extract 4-bit bit mask from depthBufferMask
                        __m256i pixelBits = _mm256_srlv_epi32(depthBufferMask, _mm256_set1_epi32(31));
                        pixelBits = _mm256_sllv_epi32(pixelBits, _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));

                        pixelBits = _mm256_hadd_epi32(pixelBits, _mm256_setzero_si256());
                        pixelBits = _mm256_hadd_epi32(pixelBits, _mm256_setzero_si256());
                        const int outputBitMask = _mm256_extract_epi32(pixelBits, 0) + _mm256_extract_epi32(pixelBits, 4);
                        RuntimeAssert(outputBitMask < 256);

                        const int outputBitMaskPopCnt = __popcnt(outputBitMask);
                        const int outputMaskLUTIndex = outputBitMaskPopCnt;
                        const int shuffleBitMaskLUTIndex = outputBitMask;
                        RuntimeAssert(outputMaskLUTIndex < 9);
                        RuntimeAssert(shuffleBitMaskLUTIndex < 256);

                        const __m256i outputMask = _mm256_load_si256((const __m256i*)(OutputBitMaskLUT8x[outputMaskLUTIndex]));
                        const uint32_t blendMask = ShuffleBitMaskLUT8x[shuffleBitMaskLUTIndex];

                        const __m256i blendMaskShift = _mm256_set_epi32( 0, 3, 6, 9, 12, 15, 18, 21 );
                        const __m256i blendMaskWide = _mm256_and_si256(_mm256_srav_epi32(_mm256_set1_epi32(blendMask), blendMaskShift), _mm256_set1_epi32(0b111));

                        const __m256 uWideShuffled = _mm256_permutevar8x32_ps(uWide, blendMaskWide);
                        const __m256 vWideShuffled = _mm256_permutevar8x32_ps(vWide, blendMaskWide);
                        const __m256i pixelCoordinatesXShuffled = _mm256_cvtps_epi32(_mm256_permutevar8x32_ps(pixelCoordinatesX, blendMaskWide));

                        _mm256_maskstore_epi32((int*)(pixelShaderInput.pScreenspaceX + pixelIndex), outputMask, pixelCoordinatesXShuffled);
                        _mm256_maskstore_epi32((int*)(pixelShaderInput.pScreenspaceY + pixelIndex), outputMask, _mm256_cvtps_epi32(pixelCoordinatesY));
                        _mm256_maskstore_ps((barycentricCoordinates.pU + pixelIndex), outputMask, uWideShuffled);
                        _mm256_maskstore_ps((barycentricCoordinates.pV + pixelIndex), outputMask, vWideShuffled);

                        const uint32_t pixelAddedThisIteration = outputBitMaskPopCnt;
                        pixelIndex += pixelAddedThisIteration;
                    }
                }

                const uint32_t pixelCount = pixelIndex;
                pixelIndex = 0;

                if( pixelCount == 0u )
                {
                    continue;
                }

                _k15_generate_barycentric_vertices(&pixelShaderInput, barycentricCoordinates, pixelCount, pTriangle->vertices);
                pixelShader(&pixelShaderInput, &pixelShaderOutput, pixelCount, pUniformData);
                _k15_reset_stack_allocator(pixelShaderInput.pStackAllocator);
                _k15_write_color_to_color_buffer(&pixelShaderOutput, pixelCount, pColorBufferContent, colorBufferStride, redShift, greenShift, blueShift);
            }
        }
    }
}

internal void _k15_convert_depth_buffer_to_color_buffer(const void* pDepthBuffer, void* pColorBuffer, uint32_t backbufferWidth, uint32_t backbufferHeight, uint32_t colorBufferStride, uint32_t depthBufferStride, uint8_t redShift, uint8_t greenShift, uint8_t blueShift)
{
    uint32_t* restrict_modifier pColorBufferContent = (uint32_t* restrict_modifier)pColorBuffer;
    const float* restrict_modifier pDepthBufferContent = (const float* restrict_modifier)pDepthBuffer;

    const __m256i redShiftWide   = _mm256_set1_epi32(redShift);
    const __m256i greenShiftWide = _mm256_set1_epi32(greenShift);
    const __m256i blueShiftWide  = _mm256_set1_epi32(blueShift);

    for(uint32_t y = 0u; y < backbufferHeight; ++y)
    {
        for(uint32_t x = 0u; x < backbufferWidth; x += 8u)
        {
            const __m256 depth = _mm256_mul_ps(_mm256_load_ps(pDepthBufferContent + x + y * depthBufferStride),  _mm256_set1_ps(255.f));
            const __m256i depthInteger = _mm256_cvtps_epi32(depth);

            __m256i colorDepth = _mm256_or_si256(depthInteger, _mm256_sllv_epi32(depthInteger, redShiftWide));
            colorDepth = _mm256_or_si256(colorDepth, _mm256_sllv_epi32(depthInteger, greenShiftWide));
            colorDepth = _mm256_or_si256(colorDepth, _mm256_sllv_epi32(depthInteger, blueShiftWide));

            _mm256_store_si256((__m256i*)(pColorBufferContent + x + y * colorBufferStride), colorDepth);
        }
    }
}

//https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
internal inline bool _k15_is_pow2(uint32_t value)
{
    return (value & (value - 1)) == 0;
}

//https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
internal inline uint32_t _k15_get_next_pow2(uint32_t value)
{
    if(_k15_is_pow2(value))
    {
        ++value;
    }

    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;

    return value;
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

software_rasterizer_context_init_parameters_t k15_create_default_software_rasterizer_context_parameters(uint32_t screenWidth, uint32_t screenHeight, void** pColorBuffers, void** pDepthBuffers, uint32_t colorBufferCount)
{
    software_rasterizer_context_init_parameters_t defaultParameters = {};
    defaultParameters.backBufferHeight  = screenHeight;
    defaultParameters.backBufferWidth   = screenWidth;
    defaultParameters.colorBufferStride = screenWidth;
    defaultParameters.depthBufferStride = screenWidth;
    defaultParameters.blueShift         = 16;
    defaultParameters.greenShift        = 8;
    defaultParameters.redShift          = 0;
    defaultParameters.pColorBuffers[0]  = pColorBuffers[0];
    defaultParameters.pDepthBuffers[0]  = pDepthBuffers[0];
    defaultParameters.pColorBuffers[1]  = pColorBuffers[1];
    defaultParameters.pDepthBuffers[1]  = pDepthBuffers[1];
    defaultParameters.pColorBuffers[2]  = pColorBuffers[2];
    defaultParameters.pDepthBuffers[2]  = pDepthBuffers[2];
    defaultParameters.colorBufferCount  = colorBufferCount;

    return defaultParameters;
}

bool _k15_create_pixel_shader_output_buffers(pixel_shader_output_t* pPixelShaderOutput, uint32_t outputCount)
{
    pPixelShaderOutput->pColor = (vector4f_t*)malloc(outputCount * sizeof(vector4f_t));
    if( pPixelShaderOutput->pColor == nullptr )
    {
        return false;
    }

    return true;
}

bool _k15_create_pixel_shader_input_buffers(pixel_shader_input_t* pPixelShaderInput, uint32_t inputCount)
{
    pPixelShaderInput->pDepth = (float*)malloc(inputCount * sizeof(float));
    if( pPixelShaderInput->pDepth == nullptr )
    {
        return false;
    }

    pPixelShaderInput->pScreenspaceX = (uint32_t*)malloc(inputCount * sizeof(uint32_t));
    if( pPixelShaderInput->pScreenspaceX == nullptr )
    {
        return false;
    }

    pPixelShaderInput->pScreenspaceY = (uint32_t*)malloc(inputCount * sizeof(uint32_t));
    if( pPixelShaderInput->pScreenspaceY == nullptr )
    {
        return false;
    }

    pPixelShaderInput->pVertexData = (vertex_t*)malloc(inputCount * sizeof(vertex_t));
    if( pPixelShaderInput->pVertexData == nullptr )
    {
        return false;
    }

    if( !_k15_create_stack_allocator(&pPixelShaderInput->pStackAllocator, 1024*1024 ) )
    {
        return false;
    }

    return true;
}

bool k15_create_software_rasterizer_context(software_rasterizer_context_t** pOutContextPtr, const software_rasterizer_context_init_parameters_t* pParameters)
{
    software_rasterizer_context_t* pContext = (software_rasterizer_context_t*)malloc(sizeof(software_rasterizer_context_t));
    pContext->backBufferHeight              = pParameters->backBufferHeight;
    pContext->backBufferWidth               = pParameters->backBufferWidth;
    pContext->colorBufferStride             = pParameters->colorBufferStride;
    pContext->depthBufferStride             = pParameters->depthBufferStride;
    pContext->redShift                      = pParameters->redShift;
    pContext->greenShift                    = pParameters->greenShift;
    pContext->blueShift                     = pParameters->blueShift;
    pContext->currentColorBufferIndex       = 0;
    pContext->currentTriangleVertexIndex    = 0;
    pContext->pBoundVertexBuffer            = nullptr;
    pContext->pBoundVertexShader            = nullptr;
    pContext->pBoundPixelShader             = nullptr;

    if(!_k15_create_font(&pContext->font))
    {
        return false;
    }

    pContext->settings.backFaceCullingEnabled = 1;
    for(uint8_t colorBufferIndex = 0; colorBufferIndex < pParameters->colorBufferCount; ++colorBufferIndex)
    {
        pContext->pColorBuffer[colorBufferIndex] = pParameters->pColorBuffers[colorBufferIndex];
    }

    for(uint8_t colorBufferIndex = 0; colorBufferIndex < pParameters->colorBufferCount; ++colorBufferIndex)
    {
        pContext->pDepthBuffer[colorBufferIndex] = pParameters->pDepthBuffers[colorBufferIndex];
    }

    pContext->colorBufferCount = pParameters->colorBufferCount;

    if(!_k15_create_dynamic_buffer<triangle_t>(&pContext->triangles, DefaultTriangleBufferCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<triangle_t>(&pContext->visibleTriangles, DefaultTriangleBufferCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<triangle_t>(&pContext->clippedTriangles, DefaultTriangleBufferCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<vertex_buffer_t>(&pContext->vertexBuffers, DefaultVertexBufferCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<screenspace_triangle_t>(&pContext->screenspaceTriangles, DefaultTriangleBufferCapacity))
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

    if(!_k15_create_dynamic_buffer<vertex_shader_t>(&pContext->vertexShaders, DefaultShaderCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<pixel_shader_t>(&pContext->pixelShaders, DefaultShaderCapacity))
    {
        return false;
    }

    if(!_k15_create_dynamic_buffer<uniform_buffer_t>(&pContext->uniformBuffers, DefaultVertexBufferCapacity))
    {
        return false;
    }

    if(!_k15_create_block_allocator(&pContext->pUniformDataAllocator))
    {
        return false;
    }

    if(!_k15_create_stack_allocator(&pContext->pDrawCallDataAllocator, DefaultUniformDataStackAllocatorSizeInBytes))
    {
        return false;
    }

    if(!_k15_create_barycentric_coordinate_buffer(&pContext->barycentricCoordinatesBuffer, PixelShaderInputCount))
    {
        return false;
    }

    if(!_k15_create_pixel_shader_input_buffers(&pContext->bufferedPixelShaderInput, PixelShaderInputCount))
    {
        return false;
    }

    if(!_k15_create_pixel_shader_output_buffers(&pContext->bufferedPixelShaderOutput, PixelShaderInputCount))
    {
        return false;
    }

    pContext->bufferedPixelShaderOutput.pScreenspaceX = pContext->bufferedPixelShaderInput.pScreenspaceX;
    pContext->bufferedPixelShaderOutput.pScreenspaceY = pContext->bufferedPixelShaderInput.pScreenspaceY;

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

void k15_copy_transformed_triangle_vertices(const vertex_shader_input_t* pVertices, triangle_t* pTriangles, uint32_t triangleCount)
{
    for(uint32_t triangleIndex = 0u; triangleIndex < triangleCount; ++triangleIndex)
    {
        const uint32_t vertexIndex = triangleIndex * 3;

        triangle_t* pTriangle = pTriangles + triangleIndex;
        pTriangle->vertices[0].position = pVertices->positions[vertexIndex + 0];
        pTriangle->vertices[1].position = pVertices->positions[vertexIndex + 1];
        pTriangle->vertices[2].position = pVertices->positions[vertexIndex + 2];
        pTriangle->vertices[0].normal = pVertices->normals[vertexIndex + 0];
        pTriangle->vertices[1].normal = pVertices->normals[vertexIndex + 1];
        pTriangle->vertices[2].normal = pVertices->normals[vertexIndex + 2];
        pTriangle->vertices[0].color = pVertices->colors[vertexIndex + 0];
        pTriangle->vertices[1].color = pVertices->colors[vertexIndex + 1];
        pTriangle->vertices[2].color = pVertices->colors[vertexIndex + 2];
        pTriangle->vertices[0].texcoord = pVertices->texcoords[vertexIndex + 0];
        pTriangle->vertices[1].texcoord = pVertices->texcoords[vertexIndex + 1];
        pTriangle->vertices[2].texcoord = pVertices->texcoords[vertexIndex + 2];
    }
}

void k15_copy_vertices_for_vertex_shader(vertex_shader_input_t* pTriangleVertices, const triangle_t* pTriangles, const uint32_t triangleCount)
{
    for( uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex )
    {
        const uint32_t vertexIndex = triangleIndex * 3;
        pTriangleVertices->positions[vertexIndex + 0] = pTriangles[triangleIndex].vertices[0].position;
        pTriangleVertices->positions[vertexIndex + 1] = pTriangles[triangleIndex].vertices[1].position;
        pTriangleVertices->positions[vertexIndex + 2] = pTriangles[triangleIndex].vertices[2].position;
        pTriangleVertices->normals[vertexIndex + 0] = pTriangles[triangleIndex].vertices[0].normal;
        pTriangleVertices->normals[vertexIndex + 1] = pTriangles[triangleIndex].vertices[1].normal;
        pTriangleVertices->normals[vertexIndex + 2] = pTriangles[triangleIndex].vertices[2].normal;
        pTriangleVertices->colors[vertexIndex + 0] = pTriangles[triangleIndex].vertices[0].color;
        pTriangleVertices->colors[vertexIndex + 1] = pTriangles[triangleIndex].vertices[1].color;
        pTriangleVertices->colors[vertexIndex + 2] = pTriangles[triangleIndex].vertices[2].color;
        pTriangleVertices->texcoords[vertexIndex + 0] = pTriangles[triangleIndex].vertices[0].texcoord;
        pTriangleVertices->texcoords[vertexIndex + 1] = pTriangles[triangleIndex].vertices[1].texcoord;
        pTriangleVertices->texcoords[vertexIndex + 2] = pTriangles[triangleIndex].vertices[2].texcoord;
    }
}

void _k15_transform_vertices(draw_call_triangles_t* pDrawCallTriangles)
{
    vertex_shader_fnc_t vertexShader = pDrawCallTriangles->vertexShader;
    void* pUniformData = pDrawCallTriangles->pUniformData;

    constexpr uint32_t TrianglesPerVertexShaderCount = VertexShaderInputCount / 3;
    static_buffer_t<vertex_shader_input_t, VertexShaderInputCount> vertices = {};
    static_buffer_t<triangle_t, TrianglesPerVertexShaderCount> triangles = {};

    for( uint32_t triangleIndex = 0; triangleIndex < pDrawCallTriangles->triangleCount; triangleIndex += TrianglesPerVertexShaderCount )
    {
        const uint32_t triangleCount = get_min(TrianglesPerVertexShaderCount, pDrawCallTriangles->triangleCount - triangleIndex);
        const uint32_t vertexCount = triangleCount * 3;
        triangle_t* pVertexShaderTriangles = _k15_static_buffer_push_back(&triangles, triangleCount);
        triangle_t* pTriangles = pDrawCallTriangles->pTriangles + triangleIndex;
        memcpy(pVertexShaderTriangles, pTriangles, triangleCount * sizeof(triangle_t));

        vertex_shader_input_t* pTriangleVertices = _k15_static_buffer_push_back(&vertices, vertexCount);
        k15_copy_vertices_for_vertex_shader(pTriangleVertices, pVertexShaderTriangles, triangleCount);

        if(vertices.count == vertices.capacity || triangleCount < TrianglesPerVertexShaderCount)
        {
            vertexShader(pTriangleVertices, vertexCount, pUniformData);
            k15_copy_transformed_triangle_vertices(pTriangleVertices, pTriangles, triangleCount);     
            triangles.count = 0;
            vertices.count = 0;
        }
    }
}

inline vector4f_t k15_vector4f_div(vector4f_t vector, float div)
{
    vector4f_t divVector = vector;
    divVector.x = vector.x / div;
    divVector.y = vector.y / div;
    divVector.z = vector.z / div;
    divVector.w = vector.w / div;

    return divVector;
}

inline vector3f_t k15_vector3f_normalize(vector3f_t vector)
{
    vector3f_t normalizedVector = {};
    float vectorLength = sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
    normalizedVector.x = vector.x / vectorLength;
    normalizedVector.y = vector.y / vectorLength;
    normalizedVector.z = vector.z / vectorLength;

    return normalizedVector;
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

inline vector3f_t k15_vector3f_cross(vector3f_t a, vector3f_t b)
{
    vector3f_t crossVector = {};
    crossVector.x = a.y * b.z - a.z * b.y;
    crossVector.y = a.z * b.x - a.x * b.z;
    crossVector.z = a.x * b.y - a.y * b.x;

    return crossVector;
}

inline vector4f_t k15_vector4f_cross(vector4f_t a, vector4f_t b)
{
    vector4f_t crossVector = {};
    crossVector.x = a.y * b.z - a.z * b.y;
    crossVector.y = a.z * b.x - a.x * b.z;
    crossVector.z = a.x * b.y - a.y * b.x;
    crossVector.w = 0.0f; //TODO

    return crossVector;
}

float k15_vector4f_dot(vector4f_t a, vector4f_t b)
{
    const float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return dot;
}

vector4f_t k15_vector4f_sub(vector4f_t a, vector4f_t b)
{
    vector4f_t v = {};
    v.x = a.x - b.x;
    v.y = a.y - b.y;
    v.z = a.z - b.z;
    v.w = a.w - b.w;

    return v;
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
bool k15_cull_outside_frustum_triangles(draw_call_triangles_t* pDrawCallTriangles, dynamic_buffer_t<triangle_t>* pCulledTriangleBuffer)
{
    const uint32_t startCulledTriangleCount = pCulledTriangleBuffer->count;
    triangle_t* pTriangles = pDrawCallTriangles->pTriangles;
    uint32_t numTrianglesCompletelyOutsideFrustum = 0;
    for(uint32_t triangleIndex = 0; triangleIndex < pDrawCallTriangles->triangleCount; ++triangleIndex)
    {
        const triangle_t* pTriangle = pTriangles + triangleIndex;
        const bool triangleOutsideFrustum = _k15_is_position_outside_frustum(pTriangle->vertices[0].position) && 
        _k15_is_position_outside_frustum(pTriangle->vertices[1].position) && 
        _k15_is_position_outside_frustum(pTriangle->vertices[2].position);

        if(!triangleOutsideFrustum)
        {
            if(APPLY_BACKFACE_CULLING)
            {
                const vector4f_t a = k15_vector4f_div(pTriangle->vertices[0].position, pTriangle->vertices[0].position.w);
                const vector4f_t b = k15_vector4f_div(pTriangle->vertices[1].position, pTriangle->vertices[1].position.w);
                const vector4f_t c = k15_vector4f_div(pTriangle->vertices[2].position, pTriangle->vertices[2].position.w);
                const vector4f_t ab = k15_vector4f_sub(a, b);
                const vector4f_t ac = k15_vector4f_sub(a, c);
                const vector4f_t normal = k15_vector4f_cross(ac, ab);

                const bool isTriangleBackfacing = normal.z <= 0.0f;
                if(isTriangleBackfacing)
                {
                    continue;
                }
            }

            if(!_k15_dynamic_buffer_push_back(pCulledTriangleBuffer, *pTriangle))
            {
                return false;
            }
        }
    }

    const uint32_t endCulledTriangleCount = pCulledTriangleBuffer->count;

    pDrawCallTriangles->pTriangles      = pCulledTriangleBuffer->pData;
    pDrawCallTriangles->triangleCount   = endCulledTriangleCount - startCulledTriangleCount;

    return true;
}

internal bool _k15_cull_triangles(draw_call_triangles_t* pDrawCallTriangles, dynamic_buffer_t<triangle_t>* pVisibleTriangleBuffer, bool backFaceCullingEnabeld)
{
    if(backFaceCullingEnabeld)
    {
        return k15_cull_outside_frustum_triangles<true>(pDrawCallTriangles, pVisibleTriangleBuffer);
    }
    else
    {
        return k15_cull_outside_frustum_triangles<false>(pDrawCallTriangles, pVisibleTriangleBuffer);
    }
}

internal inline float _k15_signf(float value)
{
    return value > 0.0f ? 1.0f : -1.0f;
}

internal inline float k15_vector4f_length_squared(vector4f_t vector)
{
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
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

internal void _k15_get_clipping_flags(const vertex_t* restrict_modifier ppPoints, uint8_t* restrict_modifier ppClippingFlags)
{
    for( uint8_t pointIndex = 0; pointIndex < 2u; ++pointIndex )
    {
        const vector4f_t point = ppPoints[pointIndex].position;
        uint8_t* restrict_modifier pClippingFlags = ppClippingFlags + pointIndex;

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

internal vertex_t _k15_interpolate_vertex(const vertex_t* restrict_modifier pStart, const vertex_t* restrict_modifier pEnd, float t)
{
    vertex_t interpolatedVertex = {};
    interpolatedVertex.position.x = pStart->position.x + (pEnd->position.x - pStart->position.x) * t;
    interpolatedVertex.position.y = pStart->position.y + (pEnd->position.y - pStart->position.y) * t;
    interpolatedVertex.position.z = pStart->position.z + (pEnd->position.z - pStart->position.z) * t;
    interpolatedVertex.position.w = pStart->position.w + (pEnd->position.w - pStart->position.w) * t;

    interpolatedVertex.normal.x = pStart->normal.x + (pEnd->normal.x - pStart->normal.x) * t;
    interpolatedVertex.normal.y = pStart->normal.y + (pEnd->normal.y - pStart->normal.y) * t;
    interpolatedVertex.normal.z = pStart->normal.z + (pEnd->normal.z - pStart->normal.z) * t;

    interpolatedVertex.color.x = pStart->color.x + (pEnd->color.x - pStart->color.x) * t;
    interpolatedVertex.color.y = pStart->color.y + (pEnd->color.y - pStart->color.y) * t;
    interpolatedVertex.color.z = pStart->color.z + (pEnd->color.z - pStart->color.z) * t;
    interpolatedVertex.color.w = pStart->color.w + (pEnd->color.w - pStart->color.w) * t;

    interpolatedVertex.texcoord.x = pStart->texcoord.x + (pEnd->texcoord.x - pStart->texcoord.x) * t;
    interpolatedVertex.texcoord.y = pStart->texcoord.y + (pEnd->texcoord.y - pStart->texcoord.y) * t;

    return interpolatedVertex;
}

internal clipped_vertex_t _k15_create_clipped_vertex(const vertex_t* pVertex, uint32_t triangleIndex)
{
    clipped_vertex_t clippedVertex;
    memcpy(&clippedVertex, pVertex, sizeof(vertex_t));
    clippedVertex.triangleIndex = triangleIndex;

    return clippedVertex;
}

internal bool _k15_generate_clip_triangles_from_clip_vertices(clipped_vertex_t* pClippedVertices, uint32_t vertexCount, dynamic_buffer_t<triangle_t>* pClippedTriangles)
{
    if( vertexCount < 3u )
    {
        return true;
    }

    for(uint32_t clippedVertexIndex = 0; clippedVertexIndex < vertexCount;)
    {
        const uint32_t remainingVertexCount = vertexCount - clippedVertexIndex;
        RuntimeAssert(remainingVertexCount >= 3u);
        
        const uint32_t localTriangleIndex = pClippedVertices[clippedVertexIndex].triangleIndex;
        if(remainingVertexCount >= 5u &&
           pClippedVertices[clippedVertexIndex + 1].triangleIndex == localTriangleIndex &&
           pClippedVertices[clippedVertexIndex + 2].triangleIndex == localTriangleIndex &&
           pClippedVertices[clippedVertexIndex + 3].triangleIndex == localTriangleIndex &&
           pClippedVertices[clippedVertexIndex + 4].triangleIndex == localTriangleIndex)
        {
            triangle_t* pTriangles = _k15_dynamic_buffer_push_back(pClippedTriangles, 3u);
            if( pTriangles == nullptr )
            {
                return false;
            }

            pTriangles[0] = {pClippedVertices[clippedVertexIndex + 0], pClippedVertices[clippedVertexIndex + 1], pClippedVertices[clippedVertexIndex + 2]};
            pTriangles[1] = {pClippedVertices[clippedVertexIndex + 2], pClippedVertices[clippedVertexIndex + 3], pClippedVertices[clippedVertexIndex + 4]};
            pTriangles[2] = {pClippedVertices[clippedVertexIndex + 4], pClippedVertices[clippedVertexIndex + 0], pClippedVertices[clippedVertexIndex + 2]};
            clippedVertexIndex += 5;
        }
        else if(remainingVertexCount >= 4u && 
           pClippedVertices[clippedVertexIndex + 1].triangleIndex == localTriangleIndex &&
           pClippedVertices[clippedVertexIndex + 2].triangleIndex == localTriangleIndex &&
           pClippedVertices[clippedVertexIndex + 3].triangleIndex == localTriangleIndex)
        {
            triangle_t* pTriangles = _k15_dynamic_buffer_push_back(pClippedTriangles, 2u);
            if( pTriangles == nullptr )
            {
                return false;
            }

            pTriangles[0] = {pClippedVertices[clippedVertexIndex + 0], pClippedVertices[clippedVertexIndex + 1], pClippedVertices[clippedVertexIndex + 2]};
            pTriangles[1] = {pClippedVertices[clippedVertexIndex + 2], pClippedVertices[clippedVertexIndex + 3], pClippedVertices[clippedVertexIndex + 0]};
            clippedVertexIndex += 4;
        }
        else if(pClippedVertices[clippedVertexIndex + 1].triangleIndex == localTriangleIndex && pClippedVertices[clippedVertexIndex + 2].triangleIndex == localTriangleIndex)
        {
            triangle_t* pTriangle = _k15_dynamic_buffer_push_back(pClippedTriangles, 1u);
            if( pTriangle == nullptr )
            {
                return false;
            }

            pTriangle[0] = {pClippedVertices[clippedVertexIndex + 0], pClippedVertices[clippedVertexIndex + 1], pClippedVertices[clippedVertexIndex + 2]};
            clippedVertexIndex += 3;
        }
        else
        {
            RuntimeAssert(false);
        }
    }

    return true;
}

internal bool _k15_clip_triangles(draw_call_triangles_t* pDrawCallTriangles, dynamic_buffer_t<triangle_t>* pClippedTriangles)
{
    const uint32_t clippedTrianglesCountStart = pClippedTriangles->count;

    const triangle_t* pTriangles = pDrawCallTriangles->pTriangles;
    const uint32_t triangleCount = pDrawCallTriangles->triangleCount;
    static_buffer_t<clipped_vertex_t, 128u> localClippedVertices = {};
    
    for(uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
    {
        if(localClippedVertices.count + 6u >= localClippedVertices.capacity)
        {
            if(!_k15_generate_clip_triangles_from_clip_vertices(localClippedVertices.pStaticData, localClippedVertices.count, pClippedTriangles))
            {
                return false;
            }
            localClippedVertices.count = 0;
        }

        const triangle_t* pCurrentTriangle = pTriangles + triangleIndex;

        bool clipped = false;
        bool startVertexClipped = false;
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
                if( !startVertexClipped )
                {
                    _k15_static_buffer_push_back(&localClippedVertices, _k15_create_clipped_vertex(&edgeVertices[0], triangleIndex));
                }

                startVertexClipped = false;

                if( clipped )
                {
                    _k15_static_buffer_push_back(&localClippedVertices, _k15_create_clipped_vertex(&intersectionVertex, triangleIndex));
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
                const vector4f_t delta = k15_vector4f_sub(edgeVertices[!vertexIndex].position, edgeVertices[vertexIndex].position);

                startVertexClipped = clippingFlags[0] != 0u;
                intersectionVertex = edgeVertices[vertexIndex];
                if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Left )
                {
                    const float t = ( -edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.x ) / ( ( -edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.x ) - ( -edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.x ) );
                    intersectionVertex = _k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Right )
                {
                    const float t = ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.x ) / ( ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.x ) - ( edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.x ) );
                    intersectionVertex = _k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Top )
                {
                    const float t = ( -edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.y ) / ( ( -edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.y ) - ( -edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.y ) );
                    intersectionVertex = _k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Bottom )
                {
                    const float t = ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.y ) / ( ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.y ) - ( edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.y ) );
                    intersectionVertex = _k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Far )
                {
                    const float t = ( -edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.z ) / ( ( -edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.z ) - ( -edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.z ) );
                    intersectionVertex = _k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }
                else if( clippingFlags[vertexIndex] & (uint8_t)clip_flag_t::Near )
                {
                    const float t = ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.z ) / ( ( edgeVertices[vertexIndex].position.w - edgeVertices[vertexIndex].position.z ) - ( edgeVertices[!vertexIndex].position.w - edgeVertices[!vertexIndex].position.z ) );
                    intersectionVertex = _k15_interpolate_vertex( &intersectionVertex, edgeVertices + !vertexIndex, t);
                }

                edgeVertices[vertexIndex] = intersectionVertex;
                _k15_get_clipping_flags(edgeVertices, clippingFlags);
                clipped = true;
                goto clipStart;
            }
        }
    }

    if(localClippedVertices.count > 0u)
    {
        if(!_k15_generate_clip_triangles_from_clip_vertices(localClippedVertices.pStaticData, localClippedVertices.count, pClippedTriangles))
        {
            return false;
        }
    }

    const uint32_t clippedTrianglesCountEnd = pClippedTriangles->count;

    pDrawCallTriangles->pTriangles = pClippedTriangles->pData;
    pDrawCallTriangles->triangleCount = clippedTrianglesCountEnd - clippedTrianglesCountStart;

    return true;
}

internal bool _k15_project_triangles_into_screenspace(draw_call_triangles_t* pDrawCallTriangles, dynamic_buffer_t<screenspace_triangle_t>* pScreenspaceTriangleBuffer, uint32_t backBufferWidth, uint32_t backBufferHeight)
{
    pDrawCallTriangles->pScreenspaceTriangles = _k15_dynamic_buffer_push_back(pScreenspaceTriangleBuffer, pDrawCallTriangles->triangleCount);
    if( pDrawCallTriangles->pScreenspaceTriangles == nullptr )
    {
        return false;
    }

    pDrawCallTriangles->screenspaceTriangleCount = pDrawCallTriangles->triangleCount;

    const float width   = (float)(backBufferWidth-1u);
    const float height  = (float)(backBufferHeight-1u);
    screenspace_triangle_t* pScreenspaceTriangles = pDrawCallTriangles->pScreenspaceTriangles;

    for(uint32_t triangleIndex = 0; triangleIndex < pDrawCallTriangles->screenspaceTriangleCount; ++triangleIndex)
    {
        triangle_t* pTriangle = pDrawCallTriangles->pTriangles + triangleIndex;

        memcpy(pScreenspaceTriangles[triangleIndex].vertices, pTriangle->vertices, sizeof(pTriangle->vertices));

        pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[0].x = (((1.0f + pTriangle->vertices[0].position.x / pTriangle->vertices[0].position.w) / 2.0f) * width);
        pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[0].y = (((1.0f + pTriangle->vertices[0].position.y / pTriangle->vertices[0].position.w) / 2.0f) * height);
        pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[0].z = pTriangle->vertices[0].position.z / pTriangle->vertices[0].position.w;

        pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[1].x = (((1.0f + pTriangle->vertices[1].position.x / pTriangle->vertices[1].position.w) / 2.0f) * width);
        pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[1].y = (((1.0f + pTriangle->vertices[1].position.y / pTriangle->vertices[1].position.w) / 2.0f) * height);
        pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[1].z = pTriangle->vertices[1].position.z / pTriangle->vertices[1].position.w;

        pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[2].x = (((1.0f + pTriangle->vertices[2].position.x / pTriangle->vertices[2].position.w) / 2.0f) * width);
        pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[2].y = (((1.0f + pTriangle->vertices[2].position.y / pTriangle->vertices[2].position.w) / 2.0f) * height);
        pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[2].z = pTriangle->vertices[2].position.z / pTriangle->vertices[2].position.w;

        pScreenspaceTriangles[triangleIndex].boundingBox.x1 = float_to_uint32(get_min(pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[0].x, get_min(pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[1].x, pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[2].x)));
        pScreenspaceTriangles[triangleIndex].boundingBox.x2 = float_to_uint32(get_max(pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[0].x, get_max(pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[1].x, pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[2].x)));
        pScreenspaceTriangles[triangleIndex].boundingBox.y1 = float_to_uint32(get_min(pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[0].y, get_min(pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[1].y, pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[2].y)));
        pScreenspaceTriangles[triangleIndex].boundingBox.y2 = float_to_uint32(get_max(pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[0].y, get_max(pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[1].y, pScreenspaceTriangles[triangleIndex].screenspaceVertexPositions[2].y)));

        pScreenspaceTriangles[triangleIndex].boundingBox.x1 = get_max(0u, pScreenspaceTriangles[triangleIndex].boundingBox.x1);
        pScreenspaceTriangles[triangleIndex].boundingBox.y1 = get_max(0u, pScreenspaceTriangles[triangleIndex].boundingBox.y1);

        //pScreenspaceTriangles[triangleIndex].boundingBox.x1 = get_max(0u, pScreenspaceTriangles[triangleIndex].boundingBox.x1 - 1u);
        //pScreenspaceTriangles[triangleIndex].boundingBox.y1 = get_max(0u, pScreenspaceTriangles[triangleIndex].boundingBox.y1 - 1u);
        pScreenspaceTriangles[triangleIndex].boundingBox.x2 = get_min((uint32_t)width, pScreenspaceTriangles[triangleIndex].boundingBox.x2 + 1u);
        pScreenspaceTriangles[triangleIndex].boundingBox.y2 = get_min((uint32_t)height, pScreenspaceTriangles[triangleIndex].boundingBox.y2 + 1u);
    }
    
    return true;
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

internal bool _k15_generate_triangles(draw_call_triangles_t* pOutDrawCallTriangles, dynamic_buffer_t<triangle_t>* pTriangleBuffer, const draw_call_t* pDrawCall)
{
    const uint32_t triangleCount = pDrawCall->vertexCount / 3;
    triangle_t* pTriangles = _k15_dynamic_buffer_push_back(pTriangleBuffer, triangleCount);
    if(pTriangles == nullptr)
    {
        return false;
    }

    for( uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex )
    {
        const uint32_t vertexBufferOffset = triangleIndex * 3u;
        memcpy(pTriangles[triangleIndex].vertices, pDrawCall->pVertexBuffer->pData + vertexBufferOffset, sizeof(vertex_t) * 3);
    }

    pOutDrawCallTriangles->pixelShader              = pDrawCall->pixelShader;
    pOutDrawCallTriangles->vertexShader             = pDrawCall->vertexShader;
    pOutDrawCallTriangles->pUniformData             = pDrawCall->pUniformBufferData;
    pOutDrawCallTriangles->pTriangles               = pTriangles;
    pOutDrawCallTriangles->triangleCount            = triangleCount;
    return true;
}

void _k15_clear_buffers(unsigned long* restrict_modifier pColorBuffer, unsigned long* restrict_modifier pDepthBuffer, uint32_t bufferHeight, uint32_t colorBufferStride, uint32_t depthBufferStride)
{
    __stosd(pDepthBuffer, 0u, bufferHeight * depthBufferStride);
	__stosd(pColorBuffer, 0u, bufferHeight * colorBufferStride);
}

void k15_draw_frame(software_rasterizer_context_t* pContext)
{
    _k15_clear_buffers((unsigned long* restrict_modifier)pContext->pColorBuffer[pContext->currentColorBufferIndex], (unsigned long* restrict_modifier)pContext->pDepthBuffer[pContext->currentColorBufferIndex], pContext->backBufferHeight, pContext->colorBufferStride, pContext->depthBufferStride);

    for(uint32_t drawCallIndex = 0; drawCallIndex < pContext->drawCalls.count; ++drawCallIndex)
    {
        draw_call_t* pDrawCall = pContext->drawCalls.pData + drawCallIndex;

        draw_call_triangles_t drawCallTriangles;
        if(!_k15_generate_triangles(&drawCallTriangles, &pContext->triangles, pDrawCall))
        {
            //TODO: log error
            continue;
        }
        
        _k15_transform_vertices(&drawCallTriangles);
        if(!_k15_cull_triangles(&drawCallTriangles, &pContext->visibleTriangles, pContext->settings.backFaceCullingEnabled))
        {
            //TODO: log error
            continue;
        }

        if(!_k15_clip_triangles(&drawCallTriangles, &pContext->clippedTriangles))
        {
            //TODO: log error
            continue;
        }

        if(!_k15_project_triangles_into_screenspace(&drawCallTriangles, &pContext->screenspaceTriangles, pContext->backBufferWidth, pContext->backBufferHeight))
        {
            //TODO: log error
            continue;
        }

        if( pContext->settings.drawWireframe)
        {
            _k15_draw_triangle_lines(&drawCallTriangles, pContext->bufferedPixelShaderInput, pContext->bufferedPixelShaderOutput, pContext->barycentricCoordinatesBuffer, pContext->pColorBuffer[pContext->currentColorBufferIndex], pContext->pDepthBuffer[pContext->currentColorBufferIndex], pContext->colorBufferStride, pContext->depthBufferStride, pContext->redShift, pContext->greenShift, pContext->blueShift);
        }
        else
        {
#if 0
            _k15_draw_triangles_4_step(&drawCallTriangles, pContext->bufferedPixelShaderInput, pContext->bufferedPixelShaderOutput, pContext->barycentricCoordinatesBuffer, pContext->pColorBuffer[pContext->currentColorBufferIndex], pContext->pDepthBuffer[pContext->currentColorBufferIndex], pContext->colorBufferStride, pContext->depthBufferStride, pContext->redShift, pContext->greenShift, pContext->blueShift);
#else
            _k15_draw_triangles_8_step(&drawCallTriangles, pContext->bufferedPixelShaderInput, pContext->bufferedPixelShaderOutput, pContext->barycentricCoordinatesBuffer, pContext->pColorBuffer[pContext->currentColorBufferIndex], pContext->pDepthBuffer[pContext->currentColorBufferIndex], pContext->colorBufferStride, pContext->depthBufferStride, pContext->redShift, pContext->greenShift, pContext->blueShift);
#endif
        }

        if( pContext->settings.drawDepthBuffer )
        {
            _k15_convert_depth_buffer_to_color_buffer(pContext->pDepthBuffer[pContext->currentColorBufferIndex], pContext->pColorBuffer[pContext->currentColorBufferIndex], pContext->backBufferWidth, pContext->backBufferHeight, pContext->colorBufferStride, pContext->depthBufferStride, pContext->redShift, pContext->greenShift, pContext->blueShift);
        }

        pContext->triangles.count = 0;
        pContext->visibleTriangles.count = 0;
        pContext->clippedTriangles.count = 0;
        pContext->screenspaceTriangles.count = 0;
    }

    pContext->drawCalls.count = 0;

    _k15_reset_stack_allocator(pContext->pDrawCallDataAllocator);
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
    pContext->colorBufferStride = strideInBytes;
}

bool k15_is_valid_vertex_buffer(const vertex_buffer_handle_t vertexBuffer)
{
    return vertexBuffer.pHandle != nullptr;
}

bool k15_is_valid_uniform_buffer(const uniform_buffer_handle_t uniformBuffer)
{
    return uniformBuffer.pHandle != nullptr;
}

bool k15_is_valid_pixel_shader(const pixel_shader_handle_t pixelShader)
{
    return pixelShader.pHandle != nullptr;
}

bool k15_is_valid_vertex_shader(const vertex_shader_handle_t vertexShader)
{
    return vertexShader.pHandle != nullptr;
}

bool k15_is_valid_texture(const texture_handle_t texture)
{
    return texture.pHandle != nullptr;
}

vertex_shader_handle_t k15_create_vertex_shader(software_rasterizer_context_t* pContext, vertex_shader_fnc_t vertexShaderFnc)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(vertexShaderFnc != nullptr);

    vertex_shader_t* pVertexShader = _k15_dynamic_buffer_push_back(&pContext->vertexShaders, 1u);
    if( pVertexShader == nullptr )
    {
        return k15_invalid_vertex_shader_handle;
    }

    pVertexShader->function = vertexShaderFnc;
    vertex_shader_handle_t handle = {pVertexShader};
    return handle;
}

pixel_shader_handle_t k15_create_pixel_shader(software_rasterizer_context_t* pContext, pixel_shader_fnc_t pixelShaderFnc)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(pixelShaderFnc != nullptr);

    pixel_shader_t* pPixelShader = _k15_dynamic_buffer_push_back(&pContext->pixelShaders, 1u);
    if( pPixelShader == nullptr )
    {
        return k15_invalid_pixel_shader_handle;
    }

    pPixelShader->function = pixelShaderFnc;
    pixel_shader_handle_t handle = {pPixelShader};
    return handle;
}

vertex_buffer_handle_t k15_create_vertex_buffer(software_rasterizer_context_t* pContext, const vertex_t* pVertexData, uint32_t vertexCount )
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

uniform_buffer_handle_t k15_create_uniform_buffer(software_rasterizer_context_t* pContext, uint32_t uniformBufferSizeInBytes)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(uniformBufferSizeInBytes > 0u);
    
    uniform_buffer_t* pUniformBuffer = _k15_dynamic_buffer_push_back(&pContext->uniformBuffers, 1u);
    if( pUniformBuffer == nullptr )
    {
        return k15_invalid_uniform_buffer_handle;
    }

    pUniformBuffer->dataSizeInBytes     = uniformBufferSizeInBytes;
    pUniformBuffer->pData               = _k15_allocate_from_block_allocator(pContext->pUniformDataAllocator, uniformBufferSizeInBytes);

    uniform_buffer_handle_t handle = {pUniformBuffer};
    return handle;
}

texture_handle_t k15_create_texture(software_rasterizer_context_t* pContext, const char* pName, uint32_t width, uint32_t height, uint32_t stride, uint8_t componentCount, const void* pTextureData)
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

    strcpy(pTexture->name, pName);
    pTexture->width             = width;
    pTexture->height            = height;
    pTexture->stride            = stride;
    pTexture->componentCount    = componentCount;
    pTexture->pData             = pTextureData;

    texture_handle_t handle = {pTexture};
    return handle;
}

void k15_set_uniform_buffer_data(uniform_buffer_handle_t uniformBufferHandle, const void* pData, uint32_t uniformBufferSizeInBytes, uint32_t uniformBufferOffsetInBytes)
{
    RuntimeAssert(k15_is_valid_uniform_buffer(uniformBufferHandle));
    RuntimeAssert(pData != nullptr);
    RuntimeAssert(uniformBufferSizeInBytes > 0u);

    uniform_buffer_t* pUniformBuffer = (uniform_buffer_t*)uniformBufferHandle.pHandle;
    RuntimeAssert(uniformBufferSizeInBytes + uniformBufferOffsetInBytes <= pUniformBuffer->dataSizeInBytes);

    uint8_t* pUniformBufferData = (uint8_t*)pUniformBuffer->pData;
    memcpy(pUniformBufferData + uniformBufferOffsetInBytes, pData, uniformBufferSizeInBytes);
}

void k15_bind_vertex_shader(software_rasterizer_context_t* pContext, vertex_shader_handle_t vertexShader)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(k15_is_valid_vertex_shader(vertexShader));

    pContext->pBoundVertexShader = (vertex_shader_t*)vertexShader.pHandle;
}

void k15_bind_pixel_shader(software_rasterizer_context_t* pContext, pixel_shader_handle_t pixelShader)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(k15_is_valid_pixel_shader(pixelShader));

    pContext->pBoundPixelShader = (pixel_shader_t*)pixelShader.pHandle;
}

void k15_bind_vertex_buffer(software_rasterizer_context_t* pContext, vertex_buffer_handle_t vertexBuffer)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(k15_is_valid_vertex_buffer(vertexBuffer));

    pContext->pBoundVertexBuffer = (vertex_buffer_t*)vertexBuffer.pHandle;
}

void k15_bind_uniform_buffer(software_rasterizer_context_t* pContext, uniform_buffer_handle_t uniformBuffer)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(k15_is_valid_uniform_buffer(uniformBuffer));
    
    pContext->pBoundUniformBuffer = (uniform_buffer_t*)uniformBuffer.pHandle;
}

void k15_bind_texture(software_rasterizer_context_t* pContext, texture_handle_t texture, uint32_t slot)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(k15_is_valid_texture(texture));
    RuntimeAssert(slot < DrawCallMaxTextures);

    pContext->boundTextures[slot] = (texture_t*)texture.pHandle;
}

bool k15_draw(software_rasterizer_context_t* pContext, uint32_t vertexCount, uint32_t vertexOffset)
{
    RuntimeAssert(pContext != nullptr);
    RuntimeAssert(pContext->pBoundVertexBuffer != nullptr);
    RuntimeAssert(pContext->pBoundVertexShader != nullptr);
    RuntimeAssert(pContext->pBoundPixelShader != nullptr);
    RuntimeAssert(pContext->pBoundVertexBuffer->vertexCount >= vertexOffset + vertexCount);
    RuntimeAssert(vertexCount > 0u && ( vertexCount % 3u ) == 0);

    draw_call_t* pDrawCall = _k15_dynamic_buffer_push_back(&pContext->drawCalls, 1u);
    if( pDrawCall == nullptr )
    {
        return false;
    }

    if( pContext->pBoundUniformBuffer != nullptr )
    {
        const uniform_buffer_t* pUniformBuffer = pContext->pBoundUniformBuffer;
        void* pDrawCallUniformBufferData = _k15_allocate_from_stack_allocator(pContext->pDrawCallDataAllocator, pUniformBuffer->dataSizeInBytes);
        if( pDrawCallUniformBufferData == nullptr )
        {
            return false;
        }

        memcpy(pDrawCallUniformBufferData, pUniformBuffer->pData, pUniformBuffer->dataSizeInBytes);

        pDrawCall->pUniformBufferData = pDrawCallUniformBufferData;
    }
    else
    {
        pDrawCall->pUniformBufferData = nullptr;
    }

    pDrawCall->vertexShader             = pContext->pBoundVertexShader->function;
    pDrawCall->pixelShader              = pContext->pBoundPixelShader->function;
    pDrawCall->pVertexBuffer            = pContext->pBoundVertexBuffer;
    pDrawCall->vertexCount              = vertexCount;
    pDrawCall->vertexOffset             = vertexOffset;

    return true;
}

#endif //K15_SOFTWARE_RASTERIZER_IMPLEMENTATION
#endif //K15_SOFTWARE_RASTERIZER_INCLUDE