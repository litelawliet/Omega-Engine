#pragma once
#include <OgRendering/Export.h>

#include <GLFW/glfw3.h>
#include <GPM/GPM.h>
#include <memory>

namespace OgEngine
{
	enum RENDERING_API KeyCode : int
	{
		A = GLFW_KEY_A,
		B = GLFW_KEY_B,
		C = GLFW_KEY_C,
		D = GLFW_KEY_D,
		E = GLFW_KEY_E,
		F = GLFW_KEY_F,
		G = GLFW_KEY_G,
		H = GLFW_KEY_H,
		I = GLFW_KEY_I,
		J = GLFW_KEY_J,
		K = GLFW_KEY_K,
		L = GLFW_KEY_L,
		M = GLFW_KEY_M,
		N = GLFW_KEY_N,
		O = GLFW_KEY_O,
		P = GLFW_KEY_P,
		Q = GLFW_KEY_Q,
		R = GLFW_KEY_R,
		S = GLFW_KEY_S,
		T = GLFW_KEY_T,
		U = GLFW_KEY_U,
		V = GLFW_KEY_V,
		W = GLFW_KEY_W,
		X = GLFW_KEY_X,
		Y = GLFW_KEY_Y,
		Z = GLFW_KEY_Z,
		NUM_0 = GLFW_KEY_0,
		NUM_1 = GLFW_KEY_1,
		NUM_2 = GLFW_KEY_2,
		NUM_3 = GLFW_KEY_3,
		NUM_4 = GLFW_KEY_4,
		NUM_5 = GLFW_KEY_5,
		NUM_6 = GLFW_KEY_6,
		NUM_7 = GLFW_KEY_7,
		NUM_8 = GLFW_KEY_8,
		NUM_9 = GLFW_KEY_9,
		RIGHT = GLFW_KEY_RIGHT,
		LEFT = GLFW_KEY_LEFT,
		UP = GLFW_KEY_UP,
		DOWN = GLFW_KEY_DOWN,
		LEFT_CTRL = GLFW_KEY_LEFT_CONTROL,
		RIGHT_CTRL = GLFW_KEY_RIGHT_CONTROL,
		LEFT_ALT = GLFW_KEY_LEFT_ALT,
		RIGHT_ALT = GLFW_KEY_RIGHT_ALT,
		SPACE = GLFW_KEY_SPACE,
		ESCAPE = GLFW_KEY_ESCAPE,
		ENTER = GLFW_KEY_ENTER,
		END = GLFW_KEY_END,
		BEGIN = GLFW_KEY_HOME,
		DELETE = GLFW_KEY_DELETE,
		BACKSPACE = GLFW_KEY_BACKSPACE,
		INSERT = GLFW_KEY_INSERT,
		PAGE_DOWN = GLFW_KEY_PAGE_DOWN,
		PAGE_UP = GLFW_KEY_PAGE_UP,
		SLASH = GLFW_KEY_SLASH,
		BACKSLASH = GLFW_KEY_BACKSLASH,
		LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT,
		RIGHT_SHIFT = GLFW_KEY_RIGHT_SHIFT,
		RIGHT_SUPER = GLFW_KEY_RIGHT_SUPER,
		LEFT_SUPER = GLFW_KEY_LEFT_SUPER,
		TAB = GLFW_KEY_TAB,
		CAPS_LOCK = GLFW_KEY_CAPS_LOCK,
		NUM_LOCK = GLFW_KEY_NUM_LOCK,
		F1 = GLFW_KEY_F1,
		F2 = GLFW_KEY_F1,
		F3 = GLFW_KEY_F1,
		F4 = GLFW_KEY_F1,
		F5 = GLFW_KEY_F1,
		F6 = GLFW_KEY_F1,
		F7 = GLFW_KEY_F1,
		F8 = GLFW_KEY_F1,
		F9 = GLFW_KEY_F1,
		F10 = GLFW_KEY_F1,
		F11 = GLFW_KEY_F1,
		F12 = GLFW_KEY_F1,
		PRINT_SCREEN = GLFW_KEY_PRINT_SCREEN,
		SCROLL_LOCK = GLFW_KEY_SCROLL_LOCK,
		PAUSE = GLFW_KEY_PAUSE,
		APOSTROPHE = GLFW_KEY_APOSTROPHE,
		COMMA = GLFW_KEY_COMMA,
		PERIOD = GLFW_KEY_PERIOD,
		MINUS = GLFW_KEY_MINUS,
		SEMICOLON = GLFW_KEY_SEMICOLON,
		EQUAL = GLFW_KEY_EQUAL,
		LEFT_BRACKET = GLFW_KEY_LEFT_BRACKET,
		RIGHT_BRACKET = GLFW_KEY_RIGHT_BRACKET,
		GRAVE_ACCENT = GLFW_KEY_GRAVE_ACCENT,
		MENU = GLFW_KEY_MENU,
		KEYPAD_0 = GLFW_KEY_KP_0,
		KEYPAD_1 = GLFW_KEY_KP_1,
		KEYPAD_2 = GLFW_KEY_KP_2,
		KEYPAD_3 = GLFW_KEY_KP_3,
		KEYPAD_4 = GLFW_KEY_KP_4,
		KEYPAD_5 = GLFW_KEY_KP_5,
		KEYPAD_6 = GLFW_KEY_KP_6,
		KEYPAD_7 = GLFW_KEY_KP_7,
		KEYPAD_8 = GLFW_KEY_KP_8,
		KEYPAD_9 = GLFW_KEY_KP_9,
		KEYPAD_DECIMAL = GLFW_KEY_KP_DECIMAL,
		KEYPAD_DIVIDE = GLFW_KEY_KP_DIVIDE,
		KEYPAD_MULTIPLY = GLFW_KEY_KP_MULTIPLY,
		KEYPAD_ADD = GLFW_KEY_KP_ADD,
		KEYPAD_SUBTRACT = GLFW_KEY_KP_SUBTRACT,
		KEYPAD_ENTER = GLFW_KEY_KP_ENTER,
		KEYPAD_EQUAL = GLFW_KEY_KP_EQUAL,
		UNKNOWN_KEY = 0
	};

	enum RENDERING_API MouseButton : int
	{
		LEFT_BUTTON = 0,
		RIGHT_BUTTON = 1,
		MIDDLE_BUTTON = 2
	};

	class RENDERING_API InputManager
	{
	public:
		static InputManager& Instance();

		static void SetWindow(GLFWwindow** p_window);
		static void SetAllCallbacks();

		[[nodiscard]] static bool IsKeyPressed(const KeyCode p_keyCode);
		[[nodiscard]] static bool GetKeyUp(const KeyCode p_keyCode);
		[[nodiscard]] static bool GetKeyDown(const KeyCode p_keyCode);
		[[nodiscard]] static bool GetMouseButton(const MouseButton p_mouseButton);
		[[nodiscard]] static GPM::Vector2D CursorPosition();
		[[nodiscard]] static GPM::Vector2D Scroll();
		[[nodiscard]] static bool OnFocus();
		
		InputManager(InputManager const&) = delete;
		void operator=(InputManager const&) = delete;

		InputManager(InputManager&&) = delete;
		void operator=(InputManager&&) = delete;

	private:
		InputManager() = default;
		~InputManager() = default;

		static void ScrollCallback(GLFWwindow* p_window, double p_xOffset, double p_yOffset);
		static void EnterCallback(GLFWwindow* p_window, int p_entered);
		static void KeyCallback(GLFWwindow* p_window, int key, int scancode, int action, int mods);
		
		static GLFWwindow** m_window;
		static Vector2D m_scrollOffsets;
		static bool m_windowOnFocus;
	};
}
