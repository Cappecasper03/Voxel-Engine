﻿#pragma once

#include <glm/ext/quaternion_transform.hpp>

#include "cApplication.h"
#include "engine/managers/assets/cModelManager.h"
#include "engine/managers/assets/cQuadManager.h"
#include "engine/managers/cInputManager.h"
#include "engine/rendering/assets/cameras/cFreeFlightCamera.h"
#include "engine/rendering/cRenderer.h"
#include "engine/rendering/iRenderer.h"
#include "engine/rendering/OpenGL/assets/cQuad_opengl.h"
#include "engine/rendering/vulkan/assets/cTexture_vulkan.h"
#include "engine/rendering/vulkan/cRenderer_vulkan.h"
#include "engine/rendering/vulkan/descriptor/sDescriptorLayoutBuilder_vulkan.h"
#include "engine/rendering/vulkan/descriptor/sDescriptorWriter_vulkan.h"
#include "engine/rendering/vulkan/pipeline/cPipeline_vulkan.h"
#include "imgui.h"

class cTesting
{
public:
	cTesting();
	~cTesting();

	void render();
	void imgui();
	void input( const df::input::sInput& _input );

	df::cFreeFlightCamera*        camera;
	df::vulkan::cPipeline_vulkan* pipeline;
};

inline cTesting::cTesting()
{
	// auto quad = df::cQuadManager::load( "quad", glm::vec3( 0, 0, 0 ), glm::vec2( 6, 4 ), df::color::blue );
	// quad->loadTexture( "data/resources/window.png" );
	df::cModelManager::load( "backpack", "data/models/survival-guitar-backpack" );

	camera = new df::cFreeFlightCamera( "freeflight", 1, .1f );
	camera->setActive( true );

	df::cEventManager::subscribe( df::event::update, camera, &df::cFreeFlightCamera::update );
	df::cEventManager::subscribe( df::event::render_3d, this, &cTesting::render );
	df::cEventManager::subscribe( df::event::imgui, this, &cTesting::imgui );
	df::cEventManager::subscribe( df::event::input, this, &cTesting::input );

	df::cRenderer::getRenderInstance()->setCursorInputMode( GLFW_CURSOR_DISABLED );
}

inline cTesting::~cTesting()
{
	df::cEventManager::unsubscribe( df::event::input, this );
	df::cEventManager::unsubscribe( df::event::imgui, this );
	df::cEventManager::unsubscribe( df::event::render_3d, this );
	df::cEventManager::unsubscribe( df::event::update, camera );
}

inline void cTesting::render()
{
	camera->beginRender( df::cCamera::eColor | df::cCamera::eDepth );

	// df::cQuadManager::render();
	df::cModelManager::render();

	camera->endRender();
}

inline void cTesting::imgui()
{}

inline void cTesting::input( const df::input::sInput& /*_input*/ )
{
	if( df::cInputManager::checkKey( GLFW_KEY_ESCAPE ) == df::input::ePress )
		cApplication::quit();
}
