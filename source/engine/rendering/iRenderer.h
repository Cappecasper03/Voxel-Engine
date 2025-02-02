﻿#pragma once

#include <glm/vec2.hpp>

#include "engine/misc/cColor.h"
#include "engine/misc/Misc.h"

struct SDL_Window;

namespace df
{
	class iFramebuffer;
	class iQuad;

	class iRenderer
	{
	public:
		DF_DISABLE_COPY_AND_MOVE( iRenderer )

		iRenderer()          = default;
		virtual ~iRenderer() = default;

		virtual void render() = 0;

		virtual void beginRendering( int _clear_buffers, const cColor& _color = color::black ) = 0;
		virtual void endRendering() {}

		SDL_Window*       getWindow() const { return m_window; }
		const glm::ivec2& getWindowSize() const { return m_window_size; }

		void resizeWindow( int _width = -1, int _height = -1 ) const;

		virtual void initializeImGui() = 0;

		const iFramebuffer* getDeferredFramebuffer() const { return m_deferred_framebuffer; }

		virtual void initializeDeferred() {}

	protected:
		SDL_Window* m_window      = nullptr;
		glm::ivec2  m_window_size = { 1200, 800 };

		iFramebuffer* m_deferred_framebuffer = nullptr;
		iQuad*        m_deferred_screen_quad = nullptr;

		bool m_window_resized = false;
	};
}
