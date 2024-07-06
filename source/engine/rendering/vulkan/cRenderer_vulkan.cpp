#include "cRenderer_vulkan.h"

#define GLFW_INCLUDE_VULKAN

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <set>
#include <VkBootstrap.h>

#include "descriptor/sDescriptorLayoutBuilder_vulkan.h"
#include "engine/managers/assets/cCameraManager.h"
#include "engine/managers/cEventManager.h"
#include "framework/application/cApplication.h"
#include "misc/Helper_vulkan.h"

namespace df::vulkan
{
	cRenderer_vulkan::cRenderer_vulkan()
		: m_frame_number( 0 )
		, m_frames( frame_overlap )
	{
		ZoneScoped;

		glfwInit();

		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

		m_window = glfwCreateWindow( m_window_size.x, m_window_size.y, cApplication::getName().data(), nullptr, nullptr );
		if( !m_window )
			DF_LOG_ERROR( "Failed to create window" );
		else
			DF_LOG_MESSAGE( fmt::format( "Created window [{}, {}]", m_window_size.x, m_window_size.y ) );

		glfwSetWindowUserPointer( m_window, this );
		glfwSetFramebufferSizeCallback( m_window, framebufferSizeCallback );

		vkb::InstanceBuilder builder;
		builder.set_app_name( cApplication::getName().data() );
		builder.set_debug_callback( debugMessageCallback );
		builder.require_api_version( VK_API_VERSION_1_3 );

#ifdef DEBUG
		builder.request_validation_layers( true );
#else
		builder.request_validation_layers( false );
#endif

		vkb::Instance instance = builder.build().value();
		m_instance             = vk::UniqueInstance( instance.instance );
		m_debug_messenger      = vk::UniqueDebugUtilsMessengerEXT( instance.debug_messenger );

		VkSurfaceKHR temp_surface;
		glfwCreateWindowSurface( m_instance.get(), m_window, nullptr, &temp_surface );
		m_surface = temp_surface;

		VkPhysicalDeviceVulkan12Features features12{
			.descriptorIndexing  = true,
			.bufferDeviceAddress = true,
		};

		VkPhysicalDeviceVulkan13Features features13{
			.synchronization2 = true,
			.dynamicRendering = true,
		};

		vkb::PhysicalDeviceSelector selector( instance );
		selector.set_minimum_version( 1, 3 );
		selector.set_required_features_12( features12 );
		selector.set_required_features_13( features13 );
		selector.set_surface( m_surface );

		vkb::PhysicalDevice vkb_physical_device = selector.select().value();

		vkb::DeviceBuilder device_builder( vkb_physical_device );
		vkb::Device        vkb_logical_device = device_builder.build().value();

		m_physical_device = vkb_physical_device.physical_device;
		m_logical_device  = vk::UniqueDevice( vkb_logical_device.device );

		m_graphics_queue        = vkb_logical_device.get_queue( vkb::QueueType::graphics ).value();
		m_graphics_queue_family = vkb_logical_device.get_queue_index( vkb::QueueType::graphics ).value();

		createMemoryAllocator();
		createSwapchain( m_window_size.x, m_window_size.y );
		createFrameDatas();
		createSubmitContext();

		sDescriptorLayoutBuilder_vulkan layout_builder;
		layout_builder.addBinding( 0, vk::DescriptorType::eUniformBuffer );
		m_vertex_scene_uniform_layout = vk::UniqueDescriptorSetLayout( layout_builder.build( m_logical_device, vk::ShaderStageFlagBits::eVertex ) );

		m_sampler_linear  = m_logical_device->createSamplerUnique( vk::SamplerCreateInfo( {}, vk::Filter::eLinear, vk::Filter::eLinear ) ).value;
		m_sampler_nearest = m_logical_device->createSamplerUnique( vk::SamplerCreateInfo( {}, vk::Filter::eNearest, vk::Filter::eNearest ) ).value;

		DF_LOG_MESSAGE( "Initialized renderer" );
	}

	cRenderer_vulkan::~cRenderer_vulkan()
	{
		ZoneScoped;

		if( m_logical_device->waitIdle() != vk::Result::eSuccess )
			DF_LOG_ERROR( "Failed to wait for device idle" );

		if( ImGui::GetCurrentContext() )
		{
			ImGui_ImplGlfw_Shutdown();
			ImGui_ImplVulkan_Shutdown();
			ImGui::DestroyContext();
		}

		vmaDestroyImage( memory_allocator, m_render_image.image.get(), m_render_image.allocation );
		vmaDestroyImage( memory_allocator, m_depth_image.image.get(), m_depth_image.allocation );

		vmaDestroyAllocator( memory_allocator );

		glfwDestroyWindow( m_window );

		glfwTerminate();

		DF_LOG_MESSAGE( "Deinitialized renderer" );
	}

	void cRenderer_vulkan::render()
	{
		ZoneScoped;

		sFrameData&                    frame_data     = getCurrentFrame();
		const vk::UniqueCommandBuffer& command_buffer = frame_data.command_buffer;

		vk::Result result = m_logical_device->waitForFences( 1, &frame_data.render_fence.get(), true, std::numeric_limits< uint64_t >::max() );
		frame_data.descriptors.clear();
		if( result != vk::Result::eSuccess )
			DF_LOG_ERROR( "Failed to wait for fences" );

		uint32_t swapchain_image_index;
		result = m_logical_device->acquireNextImageKHR( m_swapchain.get(), std::numeric_limits< uint64_t >::max(), frame_data.swapchain_semaphore.get(), nullptr, &swapchain_image_index );

		if( result == vk::Result::eErrorOutOfDateKHR )
		{
			resize();
			return;
		}

		if( result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR )
		{
			DF_LOG_ERROR( "Failed to acquire next image" );
			return;
		}

		result = m_logical_device->resetFences( 1, &frame_data.render_fence.get() );
		if( result != vk::Result::eSuccess )
			DF_LOG_ERROR( "Failed to reset fences" );

		m_logical_device->resetCommandPool( frame_data.command_pool.get() );

		const vk::CommandBufferBeginInfo command_buffer_begin_info = helper::init::commandBufferBeginInfo();

		if( command_buffer->begin( &command_buffer_begin_info ) != vk::Result::eSuccess )
		{
			DF_LOG_ERROR( "Failed to begin command buffer" );
			return;
		}

		helper::util::transitionImage( command_buffer.get(), m_render_image.image.get(), vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral );

		cEventManager::invoke( event::render_3d );
		cEventManager::invoke( event::render_2d );

		helper::util::transitionImage( command_buffer.get(), m_render_image.image.get(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal );
		helper::util::transitionImage( command_buffer.get(), m_swapchain_images[ swapchain_image_index ].get(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal );

		helper::util::copyImageToImage( command_buffer.get(), m_render_image.image.get(), m_swapchain_images[ swapchain_image_index ].get(), m_render_extent, m_swapchain_extent );

		if( ImGui::GetCurrentContext() )
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			cEventManager::invoke( event::imgui );
			ImGui::Render();

			const vk::RenderingAttachmentInfo color_attachment
				= helper::init::attachmentInfo( m_swapchain_image_views[ swapchain_image_index ].get(), nullptr, vk::ImageLayout::eColorAttachmentOptimal );
			const vk::RenderingInfo render_info = helper::init::renderingInfo( m_swapchain_extent, &color_attachment, nullptr );

			command_buffer->beginRendering( &render_info );
			ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), command_buffer.get() );
			command_buffer->endRendering();
		}

		helper::util::transitionImage( command_buffer.get(), m_swapchain_images[ swapchain_image_index ].get(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR );

		if( command_buffer->end() != vk::Result::eSuccess )
			DF_LOG_ERROR( "Failed to end command buffer" );

		vk::CommandBufferSubmitInfo command_buffer_submit_info   = helper::init::commandBufferSubmitInfo( command_buffer.get() );
		vk::SemaphoreSubmitInfo     wait_semaphore_submit_info   = helper::init::semaphoreSubmitInfo( vk::PipelineStageFlagBits2::eColorAttachmentOutput, frame_data.swapchain_semaphore.get() );
		vk::SemaphoreSubmitInfo     signal_semaphore_submit_info = helper::init::semaphoreSubmitInfo( vk::PipelineStageFlagBits2::eAllGraphics, frame_data.render_semaphore.get() );
		const vk::SubmitInfo2       submit_info                  = helper::init::submitInfo( &command_buffer_submit_info, &signal_semaphore_submit_info, &wait_semaphore_submit_info );

		if( m_graphics_queue.submit2( 1, &submit_info, frame_data.render_fence.get() ) != vk::Result::eSuccess )
		{
			DF_LOG_WARNING( "Failed to submit render queue" );
			return;
		}

		const vk::PresentInfoKHR present_info = helper::init::presentInfo( frame_data.render_semaphore.get(), m_swapchain.get(), swapchain_image_index );

		result = m_graphics_queue.presentKHR( &present_info );
		if( result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_window_resized )
			resize();
		else if( result != vk::Result::eSuccess )
		{
			DF_LOG_ERROR( "Queue present failed" );
			return;
		}

		m_frame_number++;
	}

	void cRenderer_vulkan::beginRendering( const int /*_buffers*/, const cColor& _color )
	{
		ZoneScoped;

		const vk::ImageSubresourceRange clear_range = helper::init::imageSubresourceRange( vk::ImageAspectFlagBits::eColor );
		const vk::ClearColorValue       clear_value( _color.r, _color.g, _color.b, _color.a );

		const vk::UniqueCommandBuffer& command_buffer = getCurrentFrame().command_buffer;
		command_buffer->clearColorImage( m_render_image.image.get(), vk::ImageLayout::eGeneral, &clear_value, 1, &clear_range );

		helper::util::transitionImage( command_buffer.get(), m_render_image.image.get(), vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal );
		helper::util::transitionImage( command_buffer.get(), m_depth_image.image.get(), vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal );

		const vk::RenderingAttachmentInfo color_attachment = helper::init::attachmentInfo( m_render_image.image_view.get(), nullptr, vk::ImageLayout::eColorAttachmentOptimal );
		const vk::RenderingAttachmentInfo depth_attachment = helper::init::attachmentInfo( m_depth_image.image_view.get(), nullptr, vk::ImageLayout::eDepthAttachmentOptimal );
		const vk::RenderingInfo           rendering_info   = helper::init::renderingInfo( m_render_extent, &color_attachment, &depth_attachment );

		command_buffer->beginRendering( &rendering_info );
	}

	void cRenderer_vulkan::endRendering()
	{
		ZoneScoped;

		const vk::UniqueCommandBuffer& command_buffer = getCurrentFrame().command_buffer;
		command_buffer->endRendering();
	}

	void cRenderer_vulkan::immediateSubmit( std::function< void( vk::CommandBuffer ) >&& _function ) const
	{
		ZoneScoped;

		if( m_logical_device->resetFences( 1, &m_submit_context.fence.get() ) != vk::Result::eSuccess )
			DF_LOG_ERROR( "Failed to reset fences" );

		if( m_logical_device->resetCommandPool( m_submit_context.command_pool.get() ) != vk::Result::eSuccess )
			DF_LOG_ERROR( "Failed to reset command pool" );

		const vk::UniqueCommandBuffer&   command_buffer            = m_submit_context.command_buffer;
		const vk::CommandBufferBeginInfo command_buffer_begin_info = helper::init::commandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eOneTimeSubmit );

		if( command_buffer->begin( &command_buffer_begin_info ) != vk::Result::eSuccess )
		{
			DF_LOG_ERROR( "Failed to begin command buffer" );
			return;
		}

		_function( command_buffer.get() );

		if( command_buffer->end() != vk::Result::eSuccess )
			DF_LOG_ERROR( "Failed to end command buffer" );

		vk::CommandBufferSubmitInfo buffer_submit_info = helper::init::commandBufferSubmitInfo( command_buffer.get() );
		const vk::SubmitInfo2       submit_info        = helper::init::submitInfo( &buffer_submit_info, nullptr, nullptr );

		if( m_graphics_queue.submit2( 1, &submit_info, m_submit_context.fence.get() ) != vk::Result::eSuccess )
		{
			DF_LOG_WARNING( "Failed to submit render queue" );
			return;
		}

		if( m_logical_device->waitForFences( 1, &m_submit_context.fence.get(), true, std::numeric_limits< uint64_t >::max() ) != vk::Result::eSuccess )
			DF_LOG_ERROR( "Failed to wait for fences" );
	}

	void cRenderer_vulkan::setViewport()
	{
		const vk::Viewport viewport( 0, 0, static_cast< float >( m_render_extent.width ), static_cast< float >( m_render_extent.height ), 0, 1 );
		getCurrentFrame().command_buffer->setViewport( 0, 1, &viewport );
	}

	void cRenderer_vulkan::setScissor()
	{
		const vk::Rect2D scissor( {}, vk::Extent2D( m_render_extent.width, m_render_extent.height ) );
		getCurrentFrame().command_buffer->setScissor( 0, 1, &scissor );
	}

	void cRenderer_vulkan::setViewportScissor()
	{
		ZoneScoped;

		setViewport();
		setScissor();
	}

	void cRenderer_vulkan::initializeImGui()
	{
		ZoneScoped;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io     = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

		ImGui_ImplGlfw_InitForOpenGL( m_window, true );

		std::vector pool_sizes = {
			vk::DescriptorPoolSize( vk::DescriptorType::eSampler, 1000 ),
			vk::DescriptorPoolSize( vk::DescriptorType::eCombinedImageSampler, 1000 ),
			vk::DescriptorPoolSize( vk::DescriptorType::eSampledImage, 1000 ),
			vk::DescriptorPoolSize( vk::DescriptorType::eStorageImage, 1000 ),
			vk::DescriptorPoolSize( vk::DescriptorType::eUniformTexelBuffer, 1000 ),
			vk::DescriptorPoolSize( vk::DescriptorType::eStorageTexelBuffer, 1000 ),
			vk::DescriptorPoolSize( vk::DescriptorType::eUniformBuffer, 1000 ),
			vk::DescriptorPoolSize( vk::DescriptorType::eStorageBuffer, 1000 ),
			vk::DescriptorPoolSize( vk::DescriptorType::eUniformBufferDynamic, 1000 ),
			vk::DescriptorPoolSize( vk::DescriptorType::eStorageBufferDynamic, 1000 ),
			vk::DescriptorPoolSize( vk::DescriptorType::eInputAttachment, 1000 ),
		};

		const vk::DescriptorPoolCreateInfo create_info( vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		                                                static_cast< uint32_t >( 1000 ),
		                                                static_cast< uint32_t >( pool_sizes.size() ),
		                                                pool_sizes.data() );

		m_imgui_descriptor_pool = m_logical_device->createDescriptorPoolUnique( create_info ).value;

		ImGui_ImplVulkan_InitInfo init_info{
			.Instance                    = m_instance.get(),
			.PhysicalDevice              = m_physical_device,
			.Device                      = m_logical_device.get(),
			.QueueFamily                 = m_graphics_queue_family,
			.Queue                       = m_graphics_queue,
			.DescriptorPool              = m_imgui_descriptor_pool.get(),
			.MinImageCount               = 3,
			.ImageCount                  = 3,
			.MSAASamples                 = static_cast< VkSampleCountFlagBits >( vk::SampleCountFlagBits::e1 ),
			.UseDynamicRendering         = true,
			.PipelineRenderingCreateInfo = {
				.sType					 = static_cast< VkStructureType >( vk::StructureType::ePipelineRenderingCreateInfo ),
				.colorAttachmentCount	 = 1,
				.pColorAttachmentFormats = reinterpret_cast< const VkFormat* >( &m_swapchain_format ),
			},
		};
		ImGui_ImplVulkan_Init( &init_info );
	}

	void cRenderer_vulkan::createSwapchain( const uint32_t _width, const uint32_t _height )
	{
		ZoneScoped;

		vkb::SwapchainBuilder builder( m_physical_device, m_logical_device.get(), m_surface );
		builder.use_default_format_selection();
		builder.set_desired_format( VkSurfaceFormatKHR{
			.format     = static_cast< VkFormat >( vk::Format::eB8G8R8Unorm ),
			.colorSpace = static_cast< VkColorSpaceKHR >( vk::ColorSpaceKHR::eSrgbNonlinear ),
		} );
		builder.set_desired_present_mode( static_cast< VkPresentModeKHR >( vk::PresentModeKHR::eMailbox ) );
		builder.set_desired_extent( _width, _height );
		builder.add_image_usage_flags( static_cast< VkImageUsageFlags >( vk::ImageUsageFlagBits::eTransferDst ) );
		builder.set_desired_min_image_count( frame_overlap );

		vkb::Swapchain swapchain = builder.build().value();

		m_swapchain        = vk::UniqueSwapchainKHR( swapchain.swapchain );
		m_swapchain_extent = swapchain.extent;
		m_swapchain_format = static_cast< vk::Format >( swapchain.image_format );
		for( auto image: swapchain.get_images().value() )
			m_swapchain_images.emplace_back( image );

		for( auto image_view: swapchain.get_image_views().value() )
			m_swapchain_image_views.emplace_back( image_view );

		m_render_extent = vk::Extent2D( m_swapchain_extent.width, m_swapchain_extent.height );
		m_depth_image   = {
			  .extent = vk::Extent3D( m_render_extent.width, m_render_extent.height, 1 ),
			  .format = vk::Format::eD32Sfloat,
		};
		m_render_image = {
			.extent = vk::Extent3D( m_render_extent.width, m_render_extent.height, 1 ),
			.format = vk::Format::eR16G16B16A16Sfloat,
		};

		constexpr vk::ImageUsageFlags depth_usage_flags       = vk::ImageUsageFlagBits::eDepthStencilAttachment;
		const VkImageCreateInfo       depth_image_create_info = helper::init::imageCreateInfo( m_depth_image.format, depth_usage_flags, m_depth_image.extent );

		constexpr vk::ImageUsageFlags render_usage_flags
			= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment;
		const VkImageCreateInfo render_image_create_info = helper::init::imageCreateInfo( m_render_image.format, render_usage_flags, m_render_image.extent );

		constexpr VmaAllocationCreateInfo allocation_create_info{
			.usage         = VMA_MEMORY_USAGE_GPU_ONLY,
			.requiredFlags = static_cast< VkMemoryPropertyFlags >( vk::MemoryPropertyFlagBits::eDeviceLocal ),
		};

		vmaCreateImage( memory_allocator, &depth_image_create_info, &allocation_create_info, reinterpret_cast< VkImage* >( &m_depth_image.image.get() ), &m_depth_image.allocation, nullptr );
		vmaCreateImage( memory_allocator, &render_image_create_info, &allocation_create_info, reinterpret_cast< VkImage* >( &m_render_image.image.get() ), &m_render_image.allocation, nullptr );

		vk::ImageViewCreateInfo depth_image_view_create_info  = helper::init::imageViewCreateInfo( m_depth_image.format, m_depth_image.image.get(), vk::ImageAspectFlagBits::eDepth );
		vk::ImageViewCreateInfo render_image_view_create_info = helper::init::imageViewCreateInfo( m_render_image.format, m_render_image.image.get(), vk::ImageAspectFlagBits::eColor );

		m_depth_image.image_view  = m_logical_device->createImageViewUnique( depth_image_view_create_info ).value;
		m_render_image.image_view = m_logical_device->createImageViewUnique( render_image_view_create_info ).value;
	}

	void cRenderer_vulkan::createFrameDatas()
	{
		ZoneScoped;

		const vk::CommandPoolCreateInfo command_pool_create_info = helper::init::commandPoolCreateInfo( m_graphics_queue_family );
		const vk::SemaphoreCreateInfo   semaphore_create_info    = helper::init::semaphoreCreateInfo();
		const vk::FenceCreateInfo       fence_create_info        = helper::init::fenceCreateInfo();

		for( sFrameData& frame_data: m_frames )
		{
			frame_data.command_pool = m_logical_device->createCommandPoolUnique( command_pool_create_info ).value;

			vk::CommandBufferAllocateInfo command_buffer_allocate_info = helper::init::commandBufferAllocateInfo( frame_data.command_pool.get() );
			frame_data.command_buffer.swap( m_logical_device->allocateCommandBuffersUnique( command_buffer_allocate_info ).value.front() );

			frame_data.swapchain_semaphore = m_logical_device->createSemaphoreUnique( semaphore_create_info ).value;
			frame_data.render_semaphore    = m_logical_device->createSemaphoreUnique( semaphore_create_info ).value;
			frame_data.render_fence        = m_logical_device->createFenceUnique( fence_create_info ).value;

			frame_data.vertex_scene_uniform_buffer
				= helper::util::createBuffer( sizeof( sVertexSceneUniforms ), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, memory_allocator );

			std::vector< sDescriptorAllocator_vulkan::sPoolSizeRatio > frame_sizes{
				{vk::DescriptorType::eStorageImage,          3},
				{ vk::DescriptorType::eStorageBuffer,        3},
				{ vk::DescriptorType::eUniformBuffer,        3},
				{ vk::DescriptorType::eCombinedImageSampler, 3},
			};

			frame_data.descriptors.create( m_logical_device.get(), 1000, frame_sizes );
		}
	}

	void cRenderer_vulkan::createMemoryAllocator()
	{
		ZoneScoped;

		const VmaAllocatorCreateInfo create_info{
			.flags            = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			.physicalDevice   = m_physical_device,
			.device           = m_logical_device.get(),
			.pVulkanFunctions = nullptr,
			.instance         = m_instance.get(),
			.vulkanApiVersion = VK_API_VERSION_1_3,
		};

		if( vmaCreateAllocator( &create_info, &memory_allocator ) != static_cast< VkResult >( vk::Result::eSuccess ) )
			DF_LOG_ERROR( "Failed to create memory allocator" );
		else
			DF_LOG_MESSAGE( "Created memory allocator" );
	}

	void cRenderer_vulkan::createSubmitContext()
	{
		ZoneScoped;

		m_submit_context.fence = m_logical_device->createFenceUnique( helper::init::fenceCreateInfo() ).value;

		const vk::CommandPoolCreateInfo create_info = helper::init::commandPoolCreateInfo( m_graphics_queue_family );
		m_submit_context.command_pool               = m_logical_device->createCommandPoolUnique( create_info ).value;

		const vk::CommandBufferAllocateInfo allocate_info = helper::init::commandBufferAllocateInfo( m_submit_context.command_pool.get() );
		m_submit_context.command_buffer.swap( m_logical_device->allocateCommandBuffersUnique( allocate_info ).value.front() );
		DF_LOG_MESSAGE( "Created submit context" );
	}

	void cRenderer_vulkan::resize()
	{
		ZoneScoped;

		int width = 0, height = 0;
		while( width == 0 || height == 0 )
		{
			glfwGetFramebufferSize( m_window, &width, &height );
			glfwWaitEvents();
		}

		if( m_logical_device->waitIdle() != vk::Result::eSuccess )
			DF_LOG_ERROR( "Failed to wait for device idle" );

		m_window_size.x = width;
		m_window_size.y = height;

		for( vk::UniqueImageView& image_view: m_swapchain_image_views )
			image_view.release();

		m_swapchain.release();
		createSwapchain( width, height );

		m_window_resized = false;
		DF_LOG_MESSAGE( fmt::format( "Resized window [{}, {}]", m_window_size.x, m_window_size.y ) );
	}

	void cRenderer_vulkan::framebufferSizeCallback( GLFWwindow* _window, int /*_width*/, int /*_height*/ )
	{
		ZoneScoped;

		cRenderer_vulkan* renderer = static_cast< cRenderer_vulkan* >( glfwGetWindowUserPointer( _window ) );
		renderer->m_window_resized = true;
	}

	VkBool32 cRenderer_vulkan::debugMessageCallback( const VkDebugUtilsMessageSeverityFlagBitsEXT _message_severity,
	                                                 const VkDebugUtilsMessageTypeFlagsEXT        _message_type,
	                                                 const VkDebugUtilsMessengerCallbackDataEXT*  _callback_data,
	                                                 void* /*_user_data*/
	)
	{
		ZoneScoped;

		std::string type = "None";
		if( _message_type >= static_cast< VkDebugUtilsMessageTypeFlagsEXT >( vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance ) )
			type = "Performance";
		else if( _message_type >= static_cast< VkDebugUtilsMessageTypeFlagsEXT >( vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation ) )
			type = "Validation";
		else if( _message_type >= static_cast< VkDebugUtilsMessageTypeFlagsEXT >( vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral ) )
			type = "General";

		if( _message_severity >= static_cast< VkDebugUtilsMessageTypeFlagsEXT >( vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ) )
		{
			DF_LOG_ERROR( fmt::format( "Vulkan, "
			                           "Type: {}, "
			                           "Severity: Error, "
			                           "Message: {}",
			                           type,
			                           _callback_data->pMessage ) );
		}
		else if( _message_severity >= static_cast< VkDebugUtilsMessageTypeFlagsEXT >( vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning ) )
		{
			DF_LOG_WARNING( fmt::format( "Vulkan, "
			                             "Type: {}, "
			                             "Severity: Warning, "
			                             "Message: {}",
			                             type,
			                             _callback_data->pMessage ) );
		}
		else if( _message_severity >= static_cast< VkDebugUtilsMessageTypeFlagsEXT >( vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo ) )
		{
			DF_LOG_MESSAGE( fmt::format( "Vulkan, "
			                             "Type: {}, "
			                             "Severity: Info, "
			                             "Message: {}",
			                             type,
			                             _callback_data->pMessage ) );
		}
		else if( _message_severity >= static_cast< VkDebugUtilsMessageTypeFlagsEXT >( vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose ) )
		{
			DF_LOG_MESSAGE( fmt::format( "Vulkan, "
			                             "Type: {}, "
			                             "Severity: Verbose, "
			                             "Message: {}",
			                             type,
			                             _callback_data->pMessage ) );
		}
		else
		{
			DF_LOG_MESSAGE( fmt::format( "Vulkan, "
			                             "Type: {}, "
			                             "Severity: None, "
			                             "Message: {}",
			                             type,
			                             _callback_data->pMessage ) );
		}

		return false;
	}
}
