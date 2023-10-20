﻿#include "cInputManager.h"

#include <GLFW/glfw3.h>

#include "cEventManager.h"

namespace df
{
    cInputManager::cInputManager( GLFWwindow* _window )
    {
        glfwSetKeyCallback( _window, keyCallback );
        glfwSetMouseButtonCallback( _window, mouseButtonCallback );
        glfwSetCursorPosCallback( _window, cursorPositionCallback );
        glfwSetScrollCallback( _window, scrollCallback );
    }

    void cInputManager::update()
    {
        glfwPollEvents();
        cEventManager::invoke( event::input, m_input );

        m_input.keyboard.clear();
        m_input.mouse_button.clear();
        m_input.mouse_cursor.updated = false;
        m_input.mouse_scroll.updated = false;
    }

    void cInputManager::keyCallback( GLFWwindow* /*_window*/, const int _key, const int _scancode, const int _action, const int _mods )
    {
        getInstance()->m_input.keyboard[ _key ] = { _scancode, _action, _mods };
    }

    void cInputManager::mouseButtonCallback( GLFWwindow* /*_window*/, const int _button, const int _action, const int _mods )
    {
        getInstance()->m_input.mouse_button[ _button ] = { _action, _mods };
    }

    void cInputManager::cursorPositionCallback( GLFWwindow* /*_window*/, const double _x_position, const double _y_position )
    {
        input::sMouseCursor& mouse_cursor = getInstance()->m_input.mouse_cursor;

        mouse_cursor.updated = true;

        mouse_cursor.x_delta = mouse_cursor.x_current - mouse_cursor.x_previus;
        mouse_cursor.y_delta = mouse_cursor.y_current - mouse_cursor.y_previus;

        mouse_cursor.x_previus = mouse_cursor.x_current;
        mouse_cursor.y_previus = mouse_cursor.y_current;

        mouse_cursor.x_current = _x_position;
        mouse_cursor.y_current = _y_position;
    }

    void cInputManager::scrollCallback( GLFWwindow* /*_window*/, const double _x_offset, const double _y_offset )
    {
        input::sMouseScroll& mouse_scroll = getInstance()->m_input.mouse_scroll;

        mouse_scroll.updated = true;

        mouse_scroll.x_offset = _x_offset;
        mouse_scroll.y_offset = _y_offset;
    }
}
