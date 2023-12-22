﻿#pragma once

#include <string>

#include "core/misc/Misc.h"

namespace df
{
    class cTexture
    {
    public:
        DISABLE_COPY_AND_MOVE( cTexture );

        explicit cTexture( std::string _name, const int& _target, const std::string& _file = {}, const int& _mipmaps = 0, const bool& _generate_mipmaps = true );
        ~cTexture();

        bool load( const std::string& _file, const int& _mipmaps = 0, const bool& _generate_mipmaps = true );

        void setTextureParameterI( const int& _name, const int& _param ) const;
        void setPixelStoreI( const int& _name, const int& _param ) const;

        void bind( const int& _index = 0 ) const;
        void unbind( const int& _index = 0 ) const;

        unsigned getTexture() const { return m_texture; }

        std::string name;

    protected:
        unsigned    m_texture;
        int         m_target;
        std::string m_path;
    };
}
