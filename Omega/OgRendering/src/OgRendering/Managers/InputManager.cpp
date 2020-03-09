#include <OgRendering/Managers/InputManager.h>

GLFWwindow** OgEngine::InputManager::m_window = nullptr;
GPM::Vector2D OgEngine::InputManager::m_scrollOffsets = Vector2D::zero;
bool OgEngine::InputManager::m_windowOnFocus = true;


OgEngine::InputManager& OgEngine::InputManager::Instance()
{
	static InputManager m_input;

	return m_input;
}

inline void OgEngine::InputManager::SetWindow(GLFWwindow** p_window)
{
	m_window = p_window;
}

void OgEngine::InputManager::SetAllCallbacks()
{
	if (m_window)
	{
		glfwSetScrollCallback(*m_window, ScrollCallback);
		glfwSetCursorEnterCallback(*m_window, EnterCallback);
	}
}

inline bool OgEngine::InputManager::IsKeyPressed(const KeyCode p_keyCode)
{
	if (m_window)
	{
		const int state = glfwGetKey(*m_window, p_keyCode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}
	return UNKNOWN_KEY;
}

bool OgEngine::InputManager::GetKeyUp(const KeyCode p_keyCode)
{
	if (m_window)
	{
		const int state = glfwGetKey(*m_window, p_keyCode);
		return state == GLFW_RELEASE;
	}
	return UNKNOWN_KEY;
}

bool OgEngine::InputManager::GetKeyDown(const KeyCode p_keyCode)
{
	if (m_window)
	{
		const int state = glfwGetKey(*m_window, p_keyCode);
		return state == GLFW_PRESS;
	}
	return UNKNOWN_KEY;
}

bool OgEngine::InputManager::GetMouseButton(const MouseButton p_mouseButton)
{
	if (m_window)
	{
		const int state = glfwGetMouseButton(*m_window, p_mouseButton);
		return state == GLFW_PRESS;
	}

	return 0;
}

inline GPM::Vector2D OgEngine::InputManager::CursorPosition()
{
	if (m_window)
	{
		Vector2D position;

		glfwGetCursorPos(*m_window, &position.x, &position.y);

		return position;
	}

	return Vector2D::zero;
}

GPM::Vector2D OgEngine::InputManager::Scroll()
{
	Vector2D resultScrollOffset = m_scrollOffsets;
	m_scrollOffsets = Vector2D::zero;
	return resultScrollOffset;
}

bool OgEngine::InputManager::OnFocus()
{
	return m_windowOnFocus;
}

void OgEngine::InputManager::ScrollCallback(GLFWwindow* p_window, double p_xOffset, double p_yOffset)
{
	m_scrollOffsets.x = p_xOffset;
	m_scrollOffsets.y = p_yOffset;
}

void OgEngine::InputManager::EnterCallback(GLFWwindow* p_window, int p_entered)
{
	if (p_entered)
	{
		m_windowOnFocus = true;
	}
	else
	{
		m_windowOnFocus = false;
	}
}

void OgEngine::InputManager::KeyCallback(GLFWwindow* p_window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		// Key is pressed
	}
	else if (action == GLFW_RELEASE)
	{
		// Key is released
	}
}
