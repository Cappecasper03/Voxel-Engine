﻿#include "cTexture_vulkan.h"

#include <fmt/format.h>
#include <stb_image.h>
#include <tracy/Tracy.hpp>

#include "engine/filesystem/cFileSystem.h"
#include "engine/log/Log.h"
#include "engine/rendering/vulkan/misc/Helper_vulkan.h"

namespace df::vulkan
{
	cTexture_vulkan::cTexture_vulkan( std::string _name )
		: iTexture( std::move( _name ) )
	{
		constexpr uint32_t white = 0xFFFFFFFF;
		m_texture                = helper::util::createImage( &white, vk::Extent3D{ 1, 1, 1 }, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled );
	}

	bool cTexture_vulkan::load( const std::string& _file, const bool _mipmapped, const int _mipmaps, const bool _flip_vertically_on_load )
	{
		ZoneScoped;

		stbi_set_flip_vertically_on_load( _flip_vertically_on_load );
		int            width, height, nr_channels;
		unsigned char* data = stbi_load( filesystem::getPath( _file ).data(), &width, &height, &nr_channels, 0 );

		if( !data )
		{
			DF_LOG_WARNING( fmt::format( "Failed to load texture: {}", _file ) );
			return false;
		}

		const VkExtent3D size{
			.width  = static_cast< uint32_t >( width ),
			.height = static_cast< uint32_t >( height ),
			.depth  = 1,
		};

		helper::util::destroyImage( m_texture );

		if( nr_channels == 3 )
			m_texture = helper::util::createImage( data, size, vk::Format::eR8G8B8Unorm, vk::ImageUsageFlagBits::eSampled, _mipmapped, _mipmaps );
		else if( nr_channels == 4 )
			m_texture = helper::util::createImage( data, size, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled, _mipmapped, _mipmaps );

		stbi_image_free( data );
		m_path = _file;
		return true;
	}
}
