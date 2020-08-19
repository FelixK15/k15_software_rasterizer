#ifndef K15_SOFTWARE_RASTERIZER_INCLUDE
#define K15_SOFTWARE_RASTERIZER_INCLUDE

#include "k15_base.hpp"

namespace k15_software_rasterizer
{
    struct context;

    struct context_init_parameters
    {
        int backBufferWidth;
        int backBufferHeight;
    };

    k15::result< void >  create_context( context** pOutContextPtr, const context_init_parameters& parameters );
}

#ifdef K15_SOFTWARE_RASTERIZER_IMPLEMENTATION

namespace k15_software_rasterizer
{
    k15::result< void > create_context( context** pOutContextPtr, const context_init_parameters& parameters )
    {
        K15_UNUSED_VARIABLE(pOutContextPtr);
        K15_UNUSED_VARIABLE(parameters);

        return k15::error_id::success;
    }
}

#endif //K15_SOFTWARE_RASTERIZER_IMPLEMENTATION
#endif //K15_SOFTWARE_RASTERIZER_INCLUDE