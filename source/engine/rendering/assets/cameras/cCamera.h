﻿#pragma once

#include "engine/misc/cColor.h"
#include "engine/misc/cTransform.h"
#include "engine/rendering/assets/AssetTypes.h"

namespace df
{
    class cCamera : public iAsset
    {
    public:
        DF_DISABLE_COPY_AND_MOVE( cCamera );

        enum eType
        {
            kPerspective,
            kOrthographic
        };

        explicit cCamera( std::string _name, eType _type, const cColor& _clear_color, float _fov, float _near_clip = .1f, float _far_clip = 100 );
        ~cCamera() override = default;

        void update( float _delta_time = 0 ) override;

        void beginRender( const int& _clear_buffers );
        void endRender();

        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 projection_view;

        cColor clear_color;

        eType       type;
        cTransform* transform;
        float       fov;
        float       aspect_ratio;
        float       near_clip;
        float       far_clip;
        glm::vec2   ortographic_size;

    protected:
        void calculateProjection();

    private:
        void onWindowResize( int _width, int _height );

        cCamera* m_previus;
    };
}
