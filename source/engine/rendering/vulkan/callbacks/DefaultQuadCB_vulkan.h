﻿#pragma once

#include <tracy/Tracy.hpp>

#include "engine/rendering/vulkan/assets/cQuad_vulkan.h"
#include "engine/rendering/vulkan/cPipeline_vulkan.h"
#include "engine/rendering/vulkan/cRenderer_vulkan.h"

namespace df::vulkan::render_callback
{
	inline void defaultQuad( const cPipeline_vulkan* _pipeline, const cQuad_vulkan* _quad )
	{
		ZoneScoped;

		const cRenderer_vulkan* render_instance = reinterpret_cast< cRenderer_vulkan* >( cRenderer::getRenderInstance() );
		const VkCommandBuffer   command_buffer  = render_instance->current_render_command_buffer;
		VkExtent2D              render_extent   = render_instance->getRenderExtent();
		const cCamera*          camera          = cCameraManager::getInstance()->current;

		vkCmdBindPipeline( command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->pipeline );

		const VkViewport viewport{
			.x        = 0,
			.y        = 0,
			.width    = static_cast< float >( render_extent.width ),
			.height   = static_cast< float >( render_extent.height ),
			.minDepth = 0,
			.maxDepth = 1,
		};

		vkCmdSetViewport( command_buffer, 0, 1, &viewport );

		const VkRect2D scissor{
			.offset = {
				.x = 0,
				.y = 0,
			},
			.extent = {
				.width  = render_extent.width,
				.height = render_extent.height,
			},
		};

		vkCmdSetScissor( command_buffer, 0, 1, &scissor );

		const cQuad_vulkan::sVertexConstants vertex_constants{
			.u_world_matrix           = _quad->transform->world,
			.u_projection_view_matrix = camera->projection_view,
			.vertex_buffer            = _quad->vertex_buffer_address,
		};

		vkCmdPushConstants( command_buffer, _pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( vertex_constants ), &vertex_constants );
		vkCmdBindIndexBuffer( command_buffer, _quad->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32 );

		vkCmdDrawIndexed( command_buffer, static_cast< uint32_t >( _quad->getIndices().size() ), 1, 0, 0, 0 );
	}

	inline void defaultQuadDeferred( const cPipeline_vulkan* /*_pipeline*/, const cQuad_vulkan* /*_quad*/ )
	{
		ZoneScoped;

		// const cFramebuffer* render_framebuffer = reinterpret_cast< const cFramebuffer* >( cRenderer::getRenderInstance()->getFramebuffer() );
		// const cCamera*      camera             = cCameraManager::getInstance()->current;
	}
}
