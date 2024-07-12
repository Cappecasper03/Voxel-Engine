﻿#include "cModelManager.h"

#include "engine/rendering/cRenderer.h"
#include "engine/rendering/opengl/assets/cModel_opengl.h"
#include "engine/rendering/opengl/callbacks/DefaultMeshCB_opengl.h"
#include "engine/rendering/vulkan/assets/cModel_vulkan.h"
#include "engine/rendering/vulkan/callbacks/DefaultMeshCB_vulkan.h"

namespace df
{
	cModelManager::cModelManager()
	{
		ZoneScoped;

		switch( cRenderer::getInstanceType() )
		{
			case cRenderer::kOpenGL:
			{
				m_default_render_callback = opengl::cModel_opengl::createDefaultRenderCallback();
				break;
			}
			case cRenderer::kVulkan:
			{
				m_default_render_callback = vulkan::cModel_vulkan::createDefaultRenderCallback();
				break;
			}
		}
	}

	iModel* cModelManager::load( const std::string& _name, const std::string& _folder_path, const unsigned _load_flags )
	{
		ZoneScoped;

		iModel* model = nullptr;
		switch( cRenderer::getInstanceType() )
		{
			case cRenderer::kOpenGL:
			{
				model = create< opengl::cModel_opengl >( _name );
				break;
			}
			case cRenderer::kVulkan:
			{
				model = create< vulkan::cModel_vulkan >( _name );
				break;
			}
		}
		if( model )
			model->load( _folder_path, _load_flags );

		return model;
	}
}
