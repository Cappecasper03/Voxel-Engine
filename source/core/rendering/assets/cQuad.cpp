﻿#include "cQuad.h"

#include <glad/glad.h>
#include <glm/ext/matrix_transform.hpp>

#include "core/managers/cQuadManager.h"
#include "core/managers/cRenderCallbackManager.h"
#include "core/memory/Memory.h"
#include "core/rendering/assets/cTexture.h"

namespace df
{
    cQuad::cQuad( const glm::vec3& _position, const glm::vec2& _size, const cColor& _color, const std::string& _texture_file )
    : color( _color ),
      texture( nullptr ),
      m_vertices{},
      m_indices{ 0, 1, 3, 1, 2, 3 }
    {
        transform.local = translate( transform.world, _position );
        transform.update();

        m_vertices[ 0 ] = { glm::vec3( _size.x / 2, _size.y / 2, 0 ), glm::vec2( 1, 1 ) };
        m_vertices[ 1 ] = { glm::vec3( _size.x / 2, -_size.y / 2, 0 ), glm::vec2( 1, 0 ) };
        m_vertices[ 2 ] = { glm::vec3( -_size.x / 2, -_size.y / 2, 0 ), glm::vec2( 0, 0 ) };
        m_vertices[ 3 ] = { glm::vec3( -_size.x / 2, _size.y / 2, 0 ), glm::vec2( 0, 1 ) };

        glBindVertexArray( vertex_array_object );

        glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
        glBufferData( GL_ARRAY_BUFFER, sizeof( m_vertices ), m_vertices, GL_STATIC_DRAW );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_ebo );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( m_indices ), m_indices, GL_STATIC_DRAW );

        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( *m_vertices ), nullptr );
        glEnableVertexAttribArray( 0 );

        glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( *m_vertices ), reinterpret_cast< void* >( sizeof( m_vertices->position ) ) );
        glEnableVertexAttribArray( 1 );

        if( !_texture_file.empty() )
        {
            texture = MEMORY_ALLOC( cTexture, 1, GL_TEXTURE_2D );
            texture->bind();
            texture->load( _texture_file );
            texture->setTextureParameterI( GL_TEXTURE_WRAP_S, GL_REPEAT );
            texture->setTextureParameterI( GL_TEXTURE_WRAP_T, GL_REPEAT );
            texture->setTextureParameterI( GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
            texture->setTextureParameterI( GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            texture->unbind();
        }
    }

    cQuad::~cQuad()
    {
        if( texture )
            MEMORY_FREE( texture );
    }

    void cQuad::render()
    {
        if( cQuadManager::getForcedRenderCallback() )
            cRenderCallbackManager::render( cQuadManager::getForcedRenderCallback(), this );
        else if( render_callback )
            cRenderCallbackManager::render( render_callback, this );
        else
            cRenderCallbackManager::render( cQuadManager::getDefaultRenderCallback(), this );
    }

    void cQuad::bindTexture( const int& _index ) const
    {
        if( texture )
            texture->bind( _index );
    }
}
