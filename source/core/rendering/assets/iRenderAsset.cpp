﻿#include "iRenderAsset.h"

#include <glad/glad.h>

#include "core/profiling/Profiling.h"

namespace df
{
    iRenderAsset::iRenderAsset( std::string _name )
    : iAsset( std::move( _name ) ),
      color( color::white ),
      render_callback( nullptr )
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        glGenVertexArrays( 1, &vertex_array );
        glGenBuffers( 1, &m_vertex_buffer );
        glGenBuffers( 1, &m_element_buffer );
    }

    iRenderAsset::~iRenderAsset()
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        glDeleteBuffers( 1, &m_element_buffer );
        glDeleteBuffers( 1, &m_vertex_buffer );
        glDeleteVertexArrays( 1, &vertex_array );
    }
}
