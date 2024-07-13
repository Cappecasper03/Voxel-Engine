﻿#pragma once

#include <glm/vec2.hpp>
#include <string>
#include <vulkan/vulkan.hpp>

#include "engine/misc/Misc.h"
#include "engine/rendering/iFramebuffer.h"
#include "misc/Types_vulkan.h"

namespace df::vulkan
{
	class cFramebuffer_vulkan : public iFramebuffer
	{
	public:
		DF_DISABLE_COPY_AND_MOVE( cFramebuffer_vulkan )

		explicit cFramebuffer_vulkan( std::string _name, uint32_t _num_render_textures = 0, uint32_t _frames_in_flight = 1, const glm::ivec2& _size = glm::ivec2( -1, -1 ) );
		~cFramebuffer_vulkan() override;

	private:
		std::vector< vk::UniqueFramebuffer >  m_buffers;
		std::vector< sAllocatedImage_vulkan > m_images;
		std::vector< vk::UniqueRenderPass >   m_render_passes;
	};
}
