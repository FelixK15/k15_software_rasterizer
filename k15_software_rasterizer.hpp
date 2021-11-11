#ifndef K15_SOFTWARE_RASTERIZER_INCLUDE
#define K15_SOFTWARE_RASTERIZER_INCLUDE

#include "k15_base.hpp"

namespace k15
{
    struct vector3
    {
        float x, y, z;
    };

    enum class topology
    {
        triangle
    };

    struct software_rasterizer_context;

    struct software_rasterizer_context_init_parameters
    {
        int backBufferWidth;
        int backBufferHeight;
        void* pBackBuffer;
    };

    result< void >      create_software_rasterizer_context( software_rasterizer_context** pOutContextPtr, const software_rasterizer_context_init_parameters& parameters );
    void*               set_back_buffer( software_rasterizer_context* pContext, void* pBackBuffer, uint32 width, uint32 height );
    bool                begin_geometry( software_rasterizer_context* pContext, topology topology );
    void                end_geometry( software_rasterizer_context* pContext );
    void                vertex_position( software_rasterizer_context* pContext, float32 x, float32 y, float32 z );
}

#ifdef K15_SOFTWARE_RASTERIZER_IMPLEMENTATION

namespace k15
{
    enum class vertex_id
    {
        position,
        color,

        count
    };

    struct geometry
    {
        uint8   vertexCount;
        uint32  startIndices[ (size_t)vertex_id::count ];
    };

    struct software_rasterizer_context 
    {
        int                         backBufferWidth;
        int                         backBufferHeight;

        geometry*                   pCurrentGeometry;
        int*                        pBackBuffer;
        topology         topology;

        dynamic_array< geometry >   geometry;
        dynamic_array< vector3 >    position;
        dynamic_array< vector3 >    color;
    };

    result< void > create_software_rasterizer_context( software_rasterizer_context** pOutContextPtr, const software_rasterizer_context_init_parameters& parameters )
    {
        K15_UNUSED_VARIABLE(pOutContextPtr);
        K15_UNUSED_VARIABLE(parameters);

        software_rasterizer_context* pContext = new software_rasterizer_context();
        if( pContext == nullptr )
        {
            return error_id::out_of_memory;
        }

        pContext->backBufferHeight  = parameters.backBufferHeight;
        pContext->backBufferWidth   = parameters.backBufferWidth;
        pContext->pBackBuffer       = ( int* )parameters.pBackBuffer;

        *pOutContextPtr = pContext;

        return error_id::success;
    }

    void* set_back_buffer( software_rasterizer_context* pContext, void* pBackBuffer, uint32 width, uint32 height )
    {
        K15_ASSERT( pContext != nullptr );
        K15_ASSERT( pBackBuffer != nullptr );

        void* pPrevBackBuffer = pContext->pBackBuffer;
        pContext->pBackBuffer = (int*)pBackBuffer;
        pContext->backBufferWidth = width;
        pContext->backBufferHeight = height;

        return pPrevBackBuffer;
    }

    bool begin_geometry( software_rasterizer_context* pContext, topology topology )
    {
        geometry* pGeometry = pContext->geometry.pushBack();
        if( pGeometry == nullptr )
        {
            return false;
        }

        switch( topology )
        {
            case topology::triangle:
                pGeometry->vertexCount = 3u;
                break;
        }

        pGeometry->startIndices[ ( size_t )vertex_id::position  ] = pContext->position.getSize();
        pGeometry->startIndices[ ( size_t )vertex_id::color     ] = pContext->color.getSize();

        pContext->pCurrentGeometry = pGeometry;

        return true;
    }

    void end_geometry( software_rasterizer_context* pContext )
    {
        K15_ASSERT( pContext != nullptr );
        K15_ASSERT( pContext->pCurrentGeometry != nullptr );

        pContext->pCurrentGeometry = nullptr;
    }

    void vertex_position( software_rasterizer_context* pContext, float32 x, float32 y, float32 z )
    {
        K15_ASSERT( pContext != nullptr );
        K15_ASSERT( pContext->pCurrentGeometry != nullptr );

        vector3* pPosition = pContext->position.pushBack();
        if (pPosition == nullptr)
        {
            return;
        }

        pPosition->x = x;
        pPosition->y = y;
        pPosition->z = z;
    }
}

#endif //K15_SOFTWARE_RASTERIZER_IMPLEMENTATION
#endif //K15_SOFTWARE_RASTERIZER_INCLUDE