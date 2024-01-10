﻿#include "cInputManager.h"

#include <GLFW/glfw3.h>

#include "cEventManager.h"
#include "core/rendering/cRenderer.h"

namespace df
{
    cInputManager::cInputManager()
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        GLFWwindow* window = cRenderer::getWindow();

        glfwSetKeyCallback( window, keyCallback );
        glfwSetMouseButtonCallback( window, mouseButtonCallback );
        glfwSetCursorPosCallback( window, cursorPositionCallback );
        glfwSetScrollCallback( window, scrollCallback );
        glfwSetCursorEnterCallback( window, cursorEnterCallback );
    }

    void cInputManager::update()
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        input::sInput& input = getInstance()->m_input;

        glfwPollEvents();

        if( input.keyboard.empty() && input.mouse_button.empty() && !input.mouse_cursor.updated && !input.mouse_scroll.updated )
            return;

        cEventManager::invoke( event::input, input );

        input.keyboard.clear();
        input.mouse_button.clear();
        input.mouse_cursor.updated = false;
        input.mouse_scroll.updated = false;
    }

    bool cInputManager::checkKey( const int& _key, const int& _action )
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        std::unordered_map< int, input::sKeyboard >& keyboard = getInstance()->m_input.keyboard;
        const auto                                   key_it   = keyboard.find( _key );

        if( key_it != keyboard.end() && key_it->second.action == _action )
            return true;

        return false;
    }

    input::eAction cInputManager::checkKey( const int& _key )
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        std::unordered_map< int, input::sKeyboard >& keyboard = getInstance()->m_input.keyboard;
        const auto                                   key_it   = keyboard.find( _key );

        if( key_it != keyboard.end() )
        {
            switch( key_it->second.action )
            {
                case GLFW_PRESS: return input::kPress;
                case GLFW_RELEASE: return input::kRelease;
                case GLFW_REPEAT: return input::kRepeat;
                default: return input::kNone;
            }
        }

        return input::kNone;
    }

    bool cInputManager::checkButton( const int& _key, const int& _action )
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        std::unordered_map< int, input::sMouseButton >& mouse_button = getInstance()->m_input.mouse_button;
        const auto                                      button_it    = mouse_button.find( _key );

        if( button_it != mouse_button.end() && button_it->second.action == _action )
            return true;

        return false;
    }

    input::eAction cInputManager::checkButton( const int& _key )
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        std::unordered_map< int, input::sMouseButton >& mouse_button = getInstance()->m_input.mouse_button;
        const auto                                      button_it    = mouse_button.find( _key );

        if( button_it != mouse_button.end() )
        {
            switch( button_it->second.action )
            {
                case GLFW_PRESS: return input::kPress;
                case GLFW_RELEASE: return input::kRelease;
                case GLFW_REPEAT: return input::kRepeat;
                default: return input::kNone;
            }
        }

        return input::kNone;
    }

    void cInputManager::keyCallback( GLFWwindow* /*_window*/, const int _key, const int _scancode, const int _action, const int _mods )
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        getInstance()->m_input.keyboard[ _key ] = { _scancode, _action, _mods };
    }

    void cInputManager::mouseButtonCallback( GLFWwindow* /*_window*/, const int _button, const int _action, const int _mods )
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        getInstance()->m_input.mouse_button[ _button ] = { _action, _mods };
    }

    void cInputManager::cursorPositionCallback( GLFWwindow* /*_window*/, const double _x_position, const double _y_position )
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        input::sMouseCursor& mouse_cursor = getInstance()->m_input.mouse_cursor;

        mouse_cursor.updated = true;

        if( mouse_cursor.on_window_current && mouse_cursor.on_window_previus )
        {
            mouse_cursor.x_delta = mouse_cursor.x_current - mouse_cursor.x_previus;
            mouse_cursor.y_delta = mouse_cursor.y_current - mouse_cursor.y_previus;

            mouse_cursor.x_previus = mouse_cursor.x_current;
            mouse_cursor.y_previus = mouse_cursor.y_current;
        }
        else
        {
            mouse_cursor.x_previus = _x_position;
            mouse_cursor.y_previus = _y_position;
        }

        mouse_cursor.x_current = _x_position;
        mouse_cursor.y_current = _y_position;

        mouse_cursor.on_window_previus = mouse_cursor.on_window_current;
    }

    void cInputManager::scrollCallback( GLFWwindow* /*_window*/, const double _x_offset, const double _y_offset )
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        input::sMouseScroll& mouse_scroll = getInstance()->m_input.mouse_scroll;

        mouse_scroll.updated = true;

        mouse_scroll.x_offset = _x_offset;
        mouse_scroll.y_offset = _y_offset;
    }

    void cInputManager::cursorEnterCallback( GLFWwindow* /*_window*/, const int _entered )
    {
#if defined( PROFILING )
        PROFILING_SCOPE( __FUNCTION__ );
#endif

        input::sMouseCursor& mouse_cursor = getInstance()->m_input.mouse_cursor;

        mouse_cursor.on_window_previus = mouse_cursor.on_window_current;
        mouse_cursor.on_window_current = _entered;
    }
}
