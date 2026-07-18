#include "stdafx.h"
#include "InputManager.h"
#include <MiniFB.h>
#include <algorithm>

namespace
{
	struct KeyBinding { mfb_key source; EKeyId target; };
	static const KeyBinding kBindings[] = {
		{MFB_KB_KEY_ESCAPE, eKI_Escape}, {MFB_KB_KEY_UP, eKI_Up}, {MFB_KB_KEY_DOWN, eKI_Down},
		{MFB_KB_KEY_LEFT, eKI_Left}, {MFB_KB_KEY_RIGHT, eKI_Right},
		{MFB_KB_KEY_LEFT_SHIFT, eKI_LShift}, {MFB_KB_KEY_LEFT_ALT, eKI_LAlt},
		{MFB_KB_KEY_LEFT_CONTROL, eKI_LCtrl}, {MFB_KB_KEY_SPACE, eKI_Space},
		{MFB_KB_KEY_A, eKI_A}, {MFB_KB_KEY_D, eKI_D}, {MFB_KB_KEY_J, eKI_J},
		{MFB_KB_KEY_K, eKI_K}, {MFB_KB_KEY_N, eKI_N}, {MFB_KB_KEY_P, eKI_P},
		{MFB_KB_KEY_S, eKI_S}, {MFB_KB_KEY_W, eKI_W}, {MFB_KB_KEY_GRAVE_ACCENT, eKI_Tilde}
	};
}

SrInputManager::SrInputManager() : m_window(NULL), m_mouseX(0), m_mouseY(0) {}
SrInputManager::~SrInputManager() {}

HRESULT SrInputManager::Init(HWND window)
{
	m_window = window;
	m_keys.assign(MFB_KB_KEY_LAST + 1, SKeyState(eKI_Unknown));
	m_keyDown.assign(MFB_KB_KEY_LAST + 1, 0);
	m_mouseDown.assign(8, 0);
	for (size_t i = 0; i < sizeof(kBindings) / sizeof(kBindings[0]); ++i)
		m_keys[kBindings[i].source] = SKeyState(kBindings[i].target);
	if (window)
	{
		m_mouseX = mfb_get_mouse_x(reinterpret_cast<mfb_window*>(window));
		m_mouseY = mfb_get_mouse_y(reinterpret_cast<mfb_window*>(window));
	}
	return S_OK;
}

void SrInputManager::Destroy()
{
	m_listeners.clear();
	m_keys.clear();
	m_keyDown.clear();
	m_mouseDown.clear();
}

void SrInputManager::PostState(SKeyState& state, EDeviceId device, EInputState inputState, float value)
{
	if (state.keyId == eKI_Unknown) return;
	state.state = inputState;
	state.value = value;
	SInputEvent event;
	state.MakeEvent(event);
	event.deviceId = device;
	PostInputEvent(event);
}

void SrInputManager::Update()
{
	if (!m_window) return;
	mfb_window* window = reinterpret_cast<mfb_window*>(m_window);
	const uint8_t* keys = mfb_get_key_buffer(window);
	for (size_t i = 0; i < sizeof(kBindings) / sizeof(kBindings[0]); ++i)
	{
		const unsigned key = static_cast<unsigned>(kBindings[i].source);
		const bool down = keys && keys[key] != 0;
		if (down != (m_keyDown[key] != 0))
		{
			m_keyDown[key] = down ? 1 : 0;
			PostState(m_keys[key], eDI_Keyboard, down ? eIS_Pressed : eIS_Released, down ? 1.f : 0.f);
		}
		else if (down)
			PostState(m_keys[key], eDI_Keyboard, eIS_Down, 1.f);
	}

	const uint8_t* buttons = mfb_get_mouse_button_buffer(window);
	for (unsigned button = 1; button <= 3; ++button)
	{
		const bool down = buttons && buttons[button] != 0;
		if (down != (m_mouseDown[button] != 0))
		{
			m_mouseDown[button] = down ? 1 : 0;
			SKeyState state(static_cast<EKeyId>(eKI_Mouse1 + button - 1));
			PostState(state, eDI_Mouse, down ? eIS_Pressed : eIS_Released, down ? 1.f : 0.f);
		}
	}

	const int mouseX = mfb_get_mouse_x(window);
	const int mouseY = mfb_get_mouse_y(window);
	if (mouseX != m_mouseX) { SKeyState state(eKI_MouseX); PostState(state, eDI_Mouse, eIS_Changed, static_cast<float>(mouseX - m_mouseX)); }
	if (mouseY != m_mouseY) { SKeyState state(eKI_MouseY); PostState(state, eDI_Mouse, eIS_Changed, static_cast<float>(mouseY - m_mouseY)); }
	m_mouseX = mouseX;
	m_mouseY = mouseY;
	const float wheel = mfb_get_mouse_scroll_y(window);
	if (wheel != 0.f)
	{
		SKeyState state(wheel > 0.f ? eKI_MouseWheelUp : eKI_MouseWheelDown);
		PostState(state, eDI_Mouse, eIS_Pressed, wheel);
	}
}

bool SrInputManager::PostInputEvent(SInputEvent& event)
{
	for (InputEventListeners::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
		if (*it) (*it)->OnInputEvent(event);
	return true;
}

void SrInputManager::AddListener(IInputEventListener* listener)
{
	if (std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end()) m_listeners.push_back(listener);
}

void SrInputManager::RemoveListener(IInputEventListener* listener)
{
	InputEventListeners::iterator it = std::find(m_listeners.begin(), m_listeners.end(), listener);
	if (it != m_listeners.end()) m_listeners.erase(it);
}
