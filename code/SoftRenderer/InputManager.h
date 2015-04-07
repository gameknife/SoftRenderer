/**
  @file InputManager.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef InputManager_h__
#define InputManager_h__

#include "prerequisite.h"
#include <dinput.h>
#include <dinputd.h>

struct SInputSymbol;
struct SKeyState;

/**
 *@brief 输入状态
 */
enum EInputState
{
	eIS_Unknown		= 0,
	eIS_Pressed		= (1 << 0),
	eIS_Released	= (1 << 1),
	eIS_Down			= (1 << 2),
	eIS_Changed		= (1 << 3),
	eIS_UI				= (1 << 4),
};

#define KI_KEYBOARD_BASE	0
#define KI_MOUSE_BASE		256
#define KI_XINPUT_BASE		512

/**
 *@brief 抽象按键消息
 */
enum EKeyId
{
	eKI_Escape = KI_KEYBOARD_BASE,
	eKI_1,
	eKI_2,
	eKI_3,
	eKI_4,
	eKI_5,
	eKI_6,
	eKI_7,
	eKI_8,
	eKI_9,
	eKI_0,
	eKI_Minus,
	eKI_Equals,
	eKI_Backspace,
	eKI_Tab,
	eKI_Q,
	eKI_W,
	eKI_E,
	eKI_R,
	eKI_T,
	eKI_Y,
	eKI_U,
	eKI_I,
	eKI_O,
	eKI_P,
	eKI_LBracket,
	eKI_RBracket,
	eKI_Enter,
	eKI_LCtrl,
	eKI_A,
	eKI_S,
	eKI_D,
	eKI_F,
	eKI_G,
	eKI_H,
	eKI_J,
	eKI_K,
	eKI_L,
	eKI_Semicolon,
	eKI_Apostrophe,
	eKI_Tilde,
	eKI_LShift,
	eKI_Backslash,
	eKI_Z,
	eKI_X,
	eKI_C,
	eKI_V,
	eKI_B,
	eKI_N,
	eKI_M,
	eKI_Comma,
	eKI_Period,
	eKI_Slash,
	eKI_RShift,
	eKI_NP_Multiply,
	eKI_LAlt,
	eKI_Space,
	eKI_CapsLock,
	eKI_F1,
	eKI_F2,
	eKI_F3,
	eKI_F4,
	eKI_F5,
	eKI_F6,
	eKI_F7,
	eKI_F8,
	eKI_F9,
	eKI_F10,
	eKI_NumLock,
	eKI_ScrollLock,
	eKI_NP_7,
	eKI_NP_8,
	eKI_NP_9,
	eKI_NP_Substract,
	eKI_NP_4,
	eKI_NP_5,
	eKI_NP_6,
	eKI_NP_Add,
	eKI_NP_1,
	eKI_NP_2,
	eKI_NP_3,
	eKI_NP_0,
	eKI_F11,
	eKI_F12,
	eKI_F13,
	eKI_F14,
	eKI_F15,
	eKI_Colon,
	eKI_Underline,
	eKI_NP_Enter,
	eKI_RCtrl,
	eKI_NP_Period,
	eKI_NP_Divide,
	eKI_Print,
	eKI_RAlt,
	eKI_Pause,
	eKI_Home,
	eKI_Up,
	eKI_PgUp,
	eKI_Left,
	eKI_Right,
	eKI_End,
	eKI_Down,
	eKI_PgDn,
	eKI_Insert,
	eKI_Delete,
	eKI_LWin,
	eKI_RWin,
	eKI_Apps,
	eKI_OEM_102,

	// Mouse.
	eKI_Mouse1 = KI_MOUSE_BASE,
	eKI_Mouse2,
	eKI_Mouse3,
	eKI_Mouse4,
	eKI_Mouse5,
	eKI_Mouse6,
	eKI_Mouse7,
	eKI_Mouse8,
	eKI_MouseWheelUp, 
	eKI_MouseWheelDown,
	eKI_MouseX,
	eKI_MouseY,
	eKI_MouseZ,
	eKI_MouseLast,

	// X360 controller.
	eKI_Xbox_DPadUp = KI_XINPUT_BASE,
	eKI_Xbox_DPadDown,
	eKI_Xbox_DPadLeft,
	eKI_Xbox_DPadRight,
	eKI_Xbox_Start,
	eKI_Xbox_Back,
	eKI_Xbox_ThumbL,
	eKI_Xbox_ThumbR,
	eKI_Xbox_ShoulderL,
	eKI_Xbox_ShoulderR,
	eKI_Xbox_A,
	eKI_Xbox_B,
	eKI_Xbox_X,
	eKI_Xbox_Y,
	eKI_Xbox_TriggerL,
	eKI_Xbox_TriggerR,
	eKI_Xbox_ThumbLX,
	eKI_Xbox_ThumbLY,
	eKI_Xbox_ThumbLUp,
	eKI_Xbox_ThumbLDown,
	eKI_Xbox_ThumbLLeft,
	eKI_Xbox_ThumbLRight,
	eKI_Xbox_ThumbRX,
	eKI_Xbox_ThumbRY,
	eKI_Xbox_ThumbRUp,
	eKI_Xbox_ThumbRDown,
	eKI_Xbox_ThumbRLeft,
	eKI_Xbox_ThumbRRight,
	eKI_Xbox_TriggerLBtn,
	eKI_Xbox_TriggerRBtn,
	eKI_Xbox_Connect,			
	eKI_Xbox_Disconnect,

	eKI_Android_Touch,
	eKI_Android_DragX,
	eKI_Android_DragY,

	// Terminator.
	eKI_Unknown		= 0xffffffff,
};

/**
 *@brief 抽象设备id
 */
enum EDeviceId
{
	eDI_Keyboard			= 0,
	eDI_Mouse				= 1,
	eDI_XBox				= 2,
	eDI_Kinect				= 3,
	eDI_Android				= 4,
	eDI_IOS					= 5,
	eDI_Unknown				= 0xff,
};

struct SInputEvent
{
	EDeviceId		deviceId;			
	EInputState		state;				
	EKeyId			keyId;				
	float			value;	
	SKeyState*		keyStatePtr;

	SInputEvent()
	{
		deviceId		= eDI_Unknown;
		state			= eIS_Released;
		keyId			= eKI_Unknown;
		value			= 0;
		keyStatePtr		= 0;
	}
};

struct SKeyState
{
	SKeyState(EKeyId id):keyId(id)
	{
		state = eIS_Released;
		value = 0;
	}

	void MakeEvent(SInputEvent& event)
	{
		event.keyId = keyId;
		event.state = state;
		event.value = value;
		event.keyStatePtr = this;
	}

	EKeyId		keyId;
	EInputState state;
	float		value;
};

struct IInputEventListener
{
	virtual bool OnInputEvent( const SInputEvent &event ) = 0;
	virtual bool OnInputEventUI( const SInputEvent &event ) { return false;	}
};
typedef std::vector<IInputEventListener*> InputEventListeners;

class SrInputManager;

struct InputDeviceBase
{
	InputDeviceBase( SrInputManager* creator, const char* deviceName, const GUID& guid );
	virtual ~InputDeviceBase();

	virtual const char*		GetDeviceName() const	= 0;
	virtual EDeviceId		GetDeviceId() const	= 0;

	virtual bool			Init() = 0;
	virtual void			Update(bool bFocus) = 0;

	bool CreateDirectInputDevice(const DIDATAFORMAT* dataFormat, DWORD coopLevel, DWORD bufSize);
	bool Acquire();
	bool Unacquire();
	IDirectInputDevice8*	GetDevice() {return m_pDevice;}
	SrInputManager*			GetInputManager() {return m_creator;}
	EDeviceId				m_deviceId;

protected:
	SrInputManager*			m_creator;
	IDirectInputDevice8*	m_pDevice;			
	const GUID&				m_guid;
	const DIDATAFORMAT*			m_pDataFormat;
	DWORD					m_dwCoopLevel;
	bool					m_bNeedsPoll;
};

typedef std::vector<InputDeviceBase*> InputDevices;

class SrInputManager
{
public:
	SrInputManager(void);
	~SrInputManager(void);
	HRESULT Init( HWND hDlg );
	void Destroy();
	void Update();

	void FreeDirectInput();

	IDirectInput8* getDI() {return m_pDI;}
	HWND getHwnd() {return m_hWnd;}

	bool PostInputEvent(SInputEvent& event);

	void AddListener( IInputEventListener* listener );
	void RemoveListener( IInputEventListener* listener );
	bool AddInputDevice(InputDeviceBase* pDevice);
private:
	HWND m_hWnd;

	IDirectInput8*          m_pDI; // DirectInput interface       
	InputEventListeners		m_listeners;
	InputDevices			m_inputDevices;

	std::vector<SKeyState*> m_holdKeys;
};

#endif


