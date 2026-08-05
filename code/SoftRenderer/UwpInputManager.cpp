#include "stdafx.h"
#include "InputManager.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Input.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
    using winrt::Windows::System::VirtualKey;
    using winrt::Windows::UI::Core::KeyEventArgs;
    using winrt::Windows::UI::Core::PointerEventArgs;
    using winrt::Windows::UI::Input::PointerUpdateKind;
    using winrt::Windows::Gaming::Input::Gamepad;
    using winrt::Windows::Gaming::Input::GamepadButtons;
    using winrt::Windows::Gaming::Input::GamepadReading;

    SrInputManager* g_inputManager = nullptr;

    void UwpInputKeyDown(
        winrt::Windows::UI::Core::CoreWindow const& sender,
        KeyEventArgs const& args)
    {
        if (g_inputManager)
            g_inputManager->OnKeyDown(sender, args);
    }

    void UwpInputKeyUp(
        winrt::Windows::UI::Core::CoreWindow const& sender,
        KeyEventArgs const& args)
    {
        if (g_inputManager)
            g_inputManager->OnKeyUp(sender, args);
    }

    void UwpInputPointerMoved(
        winrt::Windows::UI::Core::CoreWindow const& sender,
        PointerEventArgs const& args)
    {
        if (g_inputManager)
            g_inputManager->OnPointerMoved(sender, args);
    }

    void UwpInputPointerPressed(
        winrt::Windows::UI::Core::CoreWindow const& sender,
        PointerEventArgs const& args)
    {
        if (g_inputManager)
            g_inputManager->OnPointerPressed(sender, args);
    }

    void UwpInputPointerReleased(
        winrt::Windows::UI::Core::CoreWindow const& sender,
        PointerEventArgs const& args)
    {
        if (g_inputManager)
            g_inputManager->OnPointerReleased(sender, args);
    }

    void UwpInputPointerWheelChanged(
        winrt::Windows::UI::Core::CoreWindow const& sender,
        PointerEventArgs const& args)
    {
        if (g_inputManager)
            g_inputManager->OnPointerWheelChanged(sender, args);
    }

    EKeyId MouseButton(PointerUpdateKind kind)
    {
        switch (kind)
        {
        case PointerUpdateKind::LeftButtonPressed:
        case PointerUpdateKind::LeftButtonReleased:
            return eKI_Mouse1;
        case PointerUpdateKind::RightButtonPressed:
        case PointerUpdateKind::RightButtonReleased:
            return eKI_Mouse2;
        case PointerUpdateKind::MiddleButtonPressed:
        case PointerUpdateKind::MiddleButtonReleased:
            return eKI_Mouse3;
        default:
            return eKI_Unknown;
        }
    }

    float ApplyGamepadDeadZone(double value)
    {
        constexpr float deadZone = 0.20f;
        const float normalized = static_cast<float>(value);
        const float magnitude = std::abs(normalized);
        if (magnitude <= deadZone)
            return 0.0f;

        const float rescaled = (magnitude - deadZone) / (1.0f - deadZone);
        return std::copysign((std::min)(rescaled, 1.0f), normalized);
    }

    struct GamepadButtonBinding
    {
        EKeyId key;
        GamepadButtons button;
    };

    constexpr GamepadButtonBinding gamepadButtonMap[] =
    {
        { eKI_Xbox_DPadUp, GamepadButtons::DPadUp },
        { eKI_Xbox_DPadDown, GamepadButtons::DPadDown },
        { eKI_Xbox_DPadLeft, GamepadButtons::DPadLeft },
        { eKI_Xbox_DPadRight, GamepadButtons::DPadRight },
        { eKI_Xbox_Start, GamepadButtons::Menu },
        { eKI_Xbox_Back, GamepadButtons::View },
        { eKI_Xbox_ThumbL, GamepadButtons::LeftThumbstick },
        { eKI_Xbox_ThumbR, GamepadButtons::RightThumbstick },
        { eKI_Xbox_ShoulderL, GamepadButtons::LeftShoulder },
        { eKI_Xbox_ShoulderR, GamepadButtons::RightShoulder },
        { eKI_Xbox_A, GamepadButtons::A },
        { eKI_Xbox_B, GamepadButtons::B },
        { eKI_Xbox_X, GamepadButtons::X },
        { eKI_Xbox_Y, GamepadButtons::Y },
    };
}

SrInputManager::SrInputManager()
    : m_window(nullptr), m_mouseX(0), m_mouseY(0)
{
}

SrInputManager::~SrInputManager() = default;

HRESULT SrInputManager::Init(HWND window)
{
    m_window = window;
    m_coreWindow = CoreWindow::GetForCurrentThread();
    if (m_coreWindow == nullptr)
        return S_OK;

    g_inputManager = this;
    m_keyDownRevoker = m_coreWindow.KeyDown(
        winrt::auto_revoke,
        UwpInputKeyDown);
    m_keyUpRevoker = m_coreWindow.KeyUp(
        winrt::auto_revoke,
        UwpInputKeyUp);
    m_pointerMovedRevoker = m_coreWindow.PointerMoved(
        winrt::auto_revoke,
        UwpInputPointerMoved);
    m_pointerPressedRevoker = m_coreWindow.PointerPressed(
        winrt::auto_revoke,
        UwpInputPointerPressed);
    m_pointerReleasedRevoker = m_coreWindow.PointerReleased(
        winrt::auto_revoke,
        UwpInputPointerReleased);
    m_pointerWheelChangedRevoker = m_coreWindow.PointerWheelChanged(
        winrt::auto_revoke,
        UwpInputPointerWheelChanged);
    return S_OK;
}

void SrInputManager::Destroy()
{
    g_inputManager = nullptr;
    m_coreWindow = nullptr;
    m_down.clear();
    m_listeners.clear();
    m_keys.clear();
    m_keyDown.clear();
    m_mouseDown.clear();
    m_downDevices.clear();
    m_gamepadButtons = 0;
    m_gamepadConnected = false;
}

void SrInputManager::SetKey(EKeyId key, bool down, EDeviceId device)
{
    if (key == eKI_Unknown)
        return;

    const bool wasDown = m_down[key];
    if (wasDown == down)
        return;

    m_down[key] = down;
    m_downDevices[key] = device;
    SKeyState state(key);
    PostState(state, device, down ? eIS_Pressed : eIS_Released, down ? 1.0f : 0.0f);
}

void SrInputManager::PostMouseDelta(EKeyId key, float value)
{
    if (value == 0.0f)
        return;

    SKeyState state(key);
    PostState(state, eDI_Mouse, eIS_Changed, value);
}

void SrInputManager::PostState(
    SKeyState& state,
    EDeviceId device,
    EInputState inputState,
    float value)
{
    state.state = inputState;
    state.value = value;
    SInputEvent event;
    state.MakeEvent(event);
    event.deviceId = device;
    PostInputEvent(event);
}

void SrInputManager::Update()
{
    UpdateGamepad();

    for (auto const& entry : m_down)
    {
        if (entry.second)
        {
            const auto device = m_downDevices.find(entry.first);
            SKeyState state(entry.first);
            PostState(
                state,
                device != m_downDevices.end() ? device->second : eDI_Keyboard,
                eIS_Down,
                1.0f);
        }
    }
}

void SrInputManager::UpdateGamepad()
{
    GamepadReading reading{};
    bool connected = false;

    try
    {
        const auto gamepads = Gamepad::Gamepads();
        connected = gamepads.Size() > 0;
        if (connected)
            reading = gamepads.GetAt(0).GetCurrentReading();
    }
    catch (winrt::hresult_error const&)
    {
        connected = false;
    }

    const auto buttons = static_cast<std::uint32_t>(reading.Buttons);
    for (auto const& binding : gamepadButtonMap)
    {
        const bool down = connected &&
            (buttons & static_cast<std::uint32_t>(binding.button)) != 0;
        SetKey(binding.key, down, eDI_XBox);
    }

    const struct
    {
        EKeyId key;
        float value;
    }
    axes[] =
    {
        { eKI_Xbox_ThumbLX, ApplyGamepadDeadZone(reading.LeftThumbstickX) },
        { eKI_Xbox_ThumbLY, ApplyGamepadDeadZone(reading.LeftThumbstickY) },
        { eKI_Xbox_ThumbRX, ApplyGamepadDeadZone(reading.RightThumbstickX) },
        { eKI_Xbox_ThumbRY, ApplyGamepadDeadZone(reading.RightThumbstickY) },
        { eKI_Xbox_TriggerL, static_cast<float>(reading.LeftTrigger) },
        { eKI_Xbox_TriggerR, static_cast<float>(reading.RightTrigger) },
    };

    for (auto const& axis : axes)
    {
        SKeyState state(axis.key);
        PostState(state, eDI_XBox, eIS_Changed, connected ? axis.value : 0.0f);
    }

    m_gamepadButtons = connected ? buttons : 0;
    m_gamepadConnected = connected;
}

bool SrInputManager::PostInputEvent(SInputEvent& event)
{
    for (auto* listener : m_listeners)
    {
        if (listener)
            listener->OnInputEvent(event);
    }
    return true;
}

void SrInputManager::AddListener(IInputEventListener* listener)
{
    if (listener && std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end())
        m_listeners.push_back(listener);
}

void SrInputManager::RemoveListener(IInputEventListener* listener)
{
    const auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
    if (it != m_listeners.end())
        m_listeners.erase(it);
}

EKeyId SrInputManager::ToKeyId(VirtualKey key)
{
    const auto value = static_cast<int32_t>(key);
    if (value >= static_cast<int32_t>(VirtualKey::A) &&
        value <= static_cast<int32_t>(VirtualKey::Z))
    {
        return static_cast<EKeyId>(eKI_A + value - static_cast<int32_t>(VirtualKey::A));
    }

    switch (key)
    {
    case VirtualKey::Escape: return eKI_Escape;
    case VirtualKey::Up: return eKI_Up;
    case VirtualKey::Down: return eKI_Down;
    case VirtualKey::Left: return eKI_Left;
    case VirtualKey::Right: return eKI_Right;
    case VirtualKey::LeftShift: return eKI_LShift;
    case VirtualKey::RightShift: return eKI_RShift;
    case VirtualKey::LeftControl: return eKI_LCtrl;
    case VirtualKey::RightControl: return eKI_RCtrl;
    case VirtualKey::LeftMenu: return eKI_LAlt;
    case VirtualKey::RightMenu: return eKI_RAlt;
    case VirtualKey::Space: return eKI_Space;
    case VirtualKey::Enter: return eKI_Enter;
    case VirtualKey::P: return eKI_P;
    case VirtualKey::J: return eKI_J;
    case VirtualKey::K: return eKI_K;
    case VirtualKey::N: return eKI_N;
    case VirtualKey::Number0: return eKI_0;
    case VirtualKey::Number1: return eKI_1;
    case VirtualKey::Number2: return eKI_2;
    case VirtualKey::Number3: return eKI_3;
    case VirtualKey::Number4: return eKI_4;
    case VirtualKey::Number5: return eKI_5;
    case VirtualKey::Number6: return eKI_6;
    case VirtualKey::Number7: return eKI_7;
    case VirtualKey::Number8: return eKI_8;
    case VirtualKey::Number9: return eKI_9;
    case VirtualKey::F1: return eKI_F1;
    case VirtualKey::F2: return eKI_F2;
    case VirtualKey::F3: return eKI_F3;
    case VirtualKey::F4: return eKI_F4;
    case VirtualKey::F5: return eKI_F5;
    case VirtualKey::F6: return eKI_F6;
    case VirtualKey::F7: return eKI_F7;
    case VirtualKey::F8: return eKI_F8;
    case VirtualKey::F9: return eKI_F9;
    case VirtualKey::F10: return eKI_F10;
    case VirtualKey::F11: return eKI_F11;
    case VirtualKey::F12: return eKI_F12;
    case VirtualKey::CapitalLock: return eKI_CapsLock;
    case VirtualKey::NumberKeyLock: return eKI_NumLock;
    case VirtualKey::Scroll: return eKI_ScrollLock;
    case VirtualKey::Back: return eKI_Backspace;
    default: return eKI_Unknown;
    }
}

void SrInputManager::OnKeyDown(CoreWindow const&, KeyEventArgs const& args)
{
    SetKey(ToKeyId(args.VirtualKey()), true, eDI_Keyboard);
}

void SrInputManager::OnKeyUp(CoreWindow const&, KeyEventArgs const& args)
{
    SetKey(ToKeyId(args.VirtualKey()), false, eDI_Keyboard);
}

void SrInputManager::OnPointerMoved(CoreWindow const&, PointerEventArgs const& args)
{
    const auto point = args.CurrentPoint().Position();
    if (!m_mousePositionInitialized)
    {
        m_mouseX = static_cast<int>(point.X);
        m_mouseY = static_cast<int>(point.Y);
        m_mousePositionInitialized = true;
        return;
    }

    PostMouseDelta(eKI_MouseX, point.X - static_cast<float>(m_mouseX));
    PostMouseDelta(eKI_MouseY, point.Y - static_cast<float>(m_mouseY));
    m_mouseX = static_cast<int>(point.X);
    m_mouseY = static_cast<int>(point.Y);
}

void SrInputManager::OnPointerPressed(CoreWindow const&, PointerEventArgs const& args)
{
    SetKey(MouseButton(args.CurrentPoint().Properties().PointerUpdateKind()), true, eDI_Mouse);
}

void SrInputManager::OnPointerReleased(CoreWindow const&, PointerEventArgs const& args)
{
    SetKey(MouseButton(args.CurrentPoint().Properties().PointerUpdateKind()), false, eDI_Mouse);
}

void SrInputManager::OnPointerWheelChanged(CoreWindow const&, PointerEventArgs const& args)
{
    const int delta = args.CurrentPoint().Properties().MouseWheelDelta();
    if (delta == 0)
        return;

    SKeyState state(delta > 0 ? eKI_MouseWheelUp : eKI_MouseWheelDown);
    PostState(state, eDI_Mouse, eIS_Pressed, static_cast<float>(delta) / 120.0f);
}
