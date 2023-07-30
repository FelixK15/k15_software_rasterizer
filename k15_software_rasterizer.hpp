#ifndef K15_SOFTWARE_RASTERIZER_INCLUDE
#define K15_SOFTWARE_RASTERIZER_INCLUDE

#include "k15_base.hpp"

#define _USE_MATH_DEFINES
#include <math.h>

namespace k15
{
    struct vector3
    {
        float x, y, z;
    };

    struct vector2i
    {
        int x, y;
    };

    enum class topology
    {
        triangle
    };

    struct software_rasterizer_context;

    struct software_rasterizer_context_init_parameters
    {
        uint32_t backBufferWidth;
        uint32_t backBufferHeight;
        uint32_t backBufferStride;
        void* pBackBuffer;
    };

    result< void >      create_software_rasterizer_context( software_rasterizer_context** pOutContextPtr, const software_rasterizer_context_init_parameters& parameters );
    void*               set_back_buffer( software_rasterizer_context* pContext, void* pBackBuffer, uint32_t width, uint32_t height, uint32_t stride );
    bool                begin_geometry( software_rasterizer_context* pContext, topology topology );
    void                end_geometry( software_rasterizer_context* pContext );
    void                vertex_position( software_rasterizer_context* pContext, float32 x, float32 y, float32 z );
}

#ifdef K15_SOFTWARE_RASTERIZER_IMPLEMENTATION

namespace k15
{
    constexpr float identityMat44[] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    enum class vertex_id
    {
        position,
        color,

        count
    };

    struct geometry
    {
        topology  topology;
        uint8_t   vertexCount;
        uint32_t  startIndices[ (uint32_t)vertex_id::count ];
    };

    struct software_rasterizer_context 
    {
        uint32_t                    backBufferWidth;
        uint32_t                    backBufferHeight;
        uint32_t                    backBufferStride;

        float                       projectionMatrix[16];
        float                       viewMatrix[16];

        geometry*                   pCurrentGeometry;
        uint32_t*                   pBackBuffer;
        topology                    topology;

        dynamic_array< geometry >   geometry;
        dynamic_array< vector3 >    position;
        dynamic_array< vector3 >    color;
    };

    //https://jsantell.com/3d-projection/
    void create_projection_matrix( float* pOutMatrix, uint32_t width, uint32_t height, float near, float far, float fov )
    {
        fillMemory<float>(pOutMatrix, 0.0f, 16);
        const float fovRad = fov*((float)M_PI/180.f);
        const float aspect = (float)width/(float)height;
        const float top = near * ::tanf(fovRad / 2.0f);
        const float bottom = -top;
        const float right = aspect * top;
        const float left = -right;
        const float e = 1.0f/::tanf(fovRad/2.0f);

        pOutMatrix[ 0] = e/aspect;
        pOutMatrix[ 5] = e;
        pOutMatrix[10] = (far + near)/(near - far);
        pOutMatrix[12] = (2.0f*far*near)/(near - far);
        pOutMatrix[15] = -1.0f;
    }

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
        pContext->backBufferStride  = parameters.backBufferStride;
        pContext->pBackBuffer       = ( uint32_t* )parameters.pBackBuffer;

        create_projection_matrix( pContext->projectionMatrix, parameters.backBufferHeight, parameters.backBufferWidth, 1.0f, 10.f, 90.0f );
        copyMemoryNonOverlapping8( pContext->viewMatrix, sizeof(pContext->viewMatrix), identityMat44, sizeof(identityMat44));

        *pOutContextPtr = pContext;

        return error_id::success;
    }

    void* set_back_buffer( software_rasterizer_context* pContext, void* pBackBuffer, uint32_t width, uint32_t height, uint32_t stride )
    {
        K15_ASSERT( pContext != nullptr );
        K15_ASSERT( pBackBuffer != nullptr );

        void* pPrevBackBuffer = pContext->pBackBuffer;
        pContext->pBackBuffer = (uint32_t*)pBackBuffer;
        pContext->backBufferWidth = width;
        pContext->backBufferHeight = height;
        pContext->backBufferStride = stride;

        return pPrevBackBuffer;
    }

    bool begin_geometry( software_rasterizer_context* pContext, topology topology )
    {
        geometry* pGeometry = pContext->geometry.pushBack();
        if( pGeometry == nullptr )
        {
            return false;
        }

        pGeometry->vertexCount = 0;
        pGeometry->topology = topology;
        pGeometry->startIndices[ ( uint32_t )vertex_id::position  ] = rangecheck_cast<uint32_t>( pContext->position.getSize() );
        pGeometry->startIndices[ ( uint32_t )vertex_id::color     ] = rangecheck_cast<uint32_t>( pContext->color.getSize() );

        pContext->pCurrentGeometry = pGeometry;

        return true;
    }

    void end_geometry( software_rasterizer_context* pContext )
    {
        K15_ASSERT( pContext != nullptr );
        K15_ASSERT( pContext->pCurrentGeometry != nullptr );

        pContext->pCurrentGeometry = nullptr;
    }

    void test_image( const software_rasterizer_context* pContext )
    {
        uint8_t* pBackBuffer = (uint8_t*)pContext->pBackBuffer;
        for(uint32_t y = 0; y < pContext->backBufferHeight; ++y)
        {
            if((y%4)==0)
            {
                for(uint32_t x = 0; x < pContext->backBufferWidth; ++x)
                {
                    const uint32_t index = x + y * pContext->backBufferStride;
                    pContext->pBackBuffer[index]=0xFF0000FF;
                }
            }
            else if((y%3)==0)
            {
                for(uint32_t x = 0; x < pContext->backBufferWidth; ++x)
                {
                    const uint32_t index = x + y * pContext->backBufferStride;
                    pContext->pBackBuffer[index]=0xFFFF0000;
                }
            }
            else if((y%2)==0)
            {
                for(uint32_t x = 0; x < pContext->backBufferWidth; ++x)
                {
                    const uint32_t index = x + y * pContext->backBufferStride;
                    pContext->pBackBuffer[index]=0xFF00FF00;
                }
            }
            else
            {
                for(uint32_t x = 0; x < pContext->backBufferWidth; ++x)
                {
                    const uint32_t index = x + y * pContext->backBufferStride;
                    pContext->pBackBuffer[index]=0xFFFFFFFF;
                }
            }
        } 
    }

    vector3 mul_vector3_matrix44( const vector3& vector, const float* pMatrix44 )
    {
        vector3 multipliedVector = {};
        multipliedVector.x = vector.x * pMatrix44[ 0] + vector.y * pMatrix44[ 1] + vector.z * pMatrix44[ 2] + pMatrix44[ 3];
        multipliedVector.y = vector.x * pMatrix44[ 4] + vector.y * pMatrix44[ 5] + vector.z * pMatrix44[ 6] + pMatrix44[ 7];
        multipliedVector.z = vector.x * pMatrix44[ 8] + vector.y * pMatrix44[ 9] + vector.z * pMatrix44[10] + pMatrix44[11];
        return multipliedVector;
    }

    vector2i project_vertex( const vector3& vector, const float* pProjectionMatrix, const float* pViewMatrix, uint32_t width, uint32_t height )
    {
        vector3 worldPosition = mul_vector3_matrix44(vector, pViewMatrix);
        vector3 projectedPosition = mul_vector3_matrix44(worldPosition, pProjectionMatrix);

        int x = rangecheck_cast<int>((((projectedPosition.x/projectedPosition.z + 1) * width) / 2.0f));
        int y = rangecheck_cast<int>((((-projectedPosition.y/projectedPosition.z + 1) * height) / 2.0f));

        return {x, y};
    }

    //http://www.edepot.com/linee.html
    void draw_line( const software_rasterizer_context* pContext, const vector3& a, const vector3& b, uint32_t color)
    {
        const vector2i aa = project_vertex(a, pContext->projectionMatrix, pContext->viewMatrix, pContext->backBufferWidth, pContext->backBufferHeight);
        const vector2i bb = project_vertex(b, pContext->projectionMatrix, pContext->viewMatrix, pContext->backBufferWidth, pContext->backBufferHeight);
        int x = (int)aa.x;
        int y = (int)aa.y;
        int x2 = (int)bb.x;
        int y2 = (int)bb.y;

        bool yLonger=false;
        int shortLen=y2-y;
        int longLen=x2-x;
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
                    pContext->pBackBuffer[localX + localY * pContext->backBufferStride] = color;
                    j+=decInc;
                }
                return;
            }
            longLen+=y;
            for (int j=0x8000+(x<<16);y>=longLen;--y) {
                const uint32_t localX = j >> 16;
                const uint32_t localY = y;
                pContext->pBackBuffer[localX + localY * pContext->backBufferStride] = color;
                j-=decInc;
            }
            return;	
        }

        if (longLen>0) {
            longLen+=x;
            for (int j=0x8000+(y<<16);x<=longLen;++x) {
                const uint32_t localX = x;
                const uint32_t localY = j >> 16;
                pContext->pBackBuffer[localX + localY * pContext->backBufferStride] = color;
                j+=decInc;
            }
            return;
        }
        longLen+=x;
        for (int j=0x8000+(y<<16);x>=longLen;--x) {
            const uint32_t localX = x;
            const uint32_t localY = j >> 16;
            pContext->pBackBuffer[localX + localY * pContext->backBufferStride] = color;
            j-=decInc;
        }
    }

    void set_camera_pos( software_rasterizer_context* pContext, const vector3& cameraPos )
    {
        pContext->viewMatrix[ 3] = -cameraPos.x;
        pContext->viewMatrix[ 7] = -cameraPos.y;
        pContext->viewMatrix[11] = -cameraPos.z;
    }

    void draw_geometry( const software_rasterizer_context* pContext )
    {
        const geometry* pCurrentGeometry = pContext->geometry.getStart();
        const geometry* pLastGeometry = pContext->geometry.getEnd();

        while( pCurrentGeometry != pLastGeometry )
        {
            uint32_t positionIndex = pCurrentGeometry->startIndices[ (uint32_t)vertex_id::position ];

            switch( pCurrentGeometry->topology )
            {
                case topology::triangle:
                {
                    const uint32_t triangleCount = pCurrentGeometry->vertexCount / 3;
                    for( uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex )
                    {
                        const vector3 a = pContext->position[positionIndex++];
                        const vector3 b = pContext->position[positionIndex++];
                        const vector3 c = pContext->position[positionIndex++];

                        draw_line(pContext, a, b, 0xFFFFFFFF);
                        draw_line(pContext, b, c, 0xFFFFFFFF);
                        draw_line(pContext, a, c, 0xFFFFFFFF);
                    }
                   
                    break;
                }
            }
            
            ++pCurrentGeometry;
        }
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

        pContext->pCurrentGeometry->vertexCount++;
    }
}

#endif //K15_SOFTWARE_RASTERIZER_IMPLEMENTATION
#endif //K15_SOFTWARE_RASTERIZER_INCLUDE