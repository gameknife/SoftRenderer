#include "StdAfx.h"
#include "Keyboard.h"

#include "mmgr/mmgr.h"
Keyboard::Keyboard( SrInputManager* creator ):InputDeviceBase(creator,"keyboard", GUID_SysKeyboard)
{
	m_deviceId = eDI_Keyboard;
}


Keyboard::~Keyboard(void)
{
}

bool Keyboard::Init()
{
	if (!CreateDirectInputDevice(&c_dfDIKeyboard, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND|DISCL_NOWINKEY, 32))
	{
		return false;
	}

	Acquire();	
	MapDIK2EKI();
	return true;
}

//////////////////////////////////////////////////////////////////////////
void Keyboard::ProcessKey(uint32 devSpecId, bool pressed)
{
	SKeyState& keyState = dik2ekiMap[devSpecId];

	if (keyState.keyId == eKI_Unknown) return;

	EInputState newState;

	if (pressed)
	{
		newState = eIS_Pressed;
		keyState.value = 1.0f;
	}
	else
	{
		newState = eIS_Released;
		keyState.value = 0.0f;
	}

	if (newState == keyState.state)
	{
		return;
	}
	keyState.state = newState;

	SInputEvent event;
	keyState.MakeEvent(event);
	GetInputManager()->PostInputEvent(event);
}

void Keyboard::Update( bool bFocus )
{
	HRESULT hr;    	
	DIDEVICEOBJECTDATA rgdod[256];	
	DWORD dwItems = 256;

	while (GetDevice())
	{
		dwItems = 256;

		hr = GetDevice()->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),rgdod,&dwItems,0 ); //0);

		if (SUCCEEDED(hr))
		{
			for (unsigned int k=0; k < dwItems; k++)
			{
				int key = rgdod[k].dwOfs;
				bool pressed = ((rgdod[k].dwData & 0x80) != 0);

				ProcessKey(key, pressed);
			}	
			break;
		}
		else 
		{
			if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
			{			

				if (FAILED(hr = GetDevice()->Acquire()))	
				{
					break;
				}
			}
			else
				break;
		}
	} 		  
}

void Keyboard::MapDIK2EKI()
{
	dik2ekiMap.clear();
	dik2ekiMap.assign(0xff, SKeyState(eKI_Unknown));

	dik2ekiMap[DIK_ESCAPE] = SKeyState(eKI_Escape);
	dik2ekiMap[DIK_1] = SKeyState(eKI_1);
	dik2ekiMap[DIK_2] = SKeyState(eKI_2);
	dik2ekiMap[DIK_3] = SKeyState(eKI_3);
	dik2ekiMap[DIK_4] = SKeyState(eKI_4);
	dik2ekiMap[DIK_5] = SKeyState(eKI_5);
	dik2ekiMap[DIK_6] = SKeyState(eKI_6);
	dik2ekiMap[DIK_7] = SKeyState(eKI_7);
	dik2ekiMap[DIK_8] = SKeyState(eKI_8);
	dik2ekiMap[DIK_9] = SKeyState(eKI_9);
	dik2ekiMap[DIK_0] = SKeyState(eKI_0);
	dik2ekiMap[DIK_MINUS] = SKeyState(eKI_Minus);
	dik2ekiMap[DIK_EQUALS] = SKeyState(eKI_Equals);
	dik2ekiMap[DIK_BACK] = SKeyState(eKI_Backspace);
	dik2ekiMap[DIK_TAB] = SKeyState(eKI_Tab);
	dik2ekiMap[DIK_Q] = SKeyState(eKI_Q);
	dik2ekiMap[DIK_W] = SKeyState(eKI_W);
	dik2ekiMap[DIK_E] = SKeyState(eKI_E);
	dik2ekiMap[DIK_R] = SKeyState(eKI_R);
	dik2ekiMap[DIK_T] = SKeyState(eKI_T);
	dik2ekiMap[DIK_Y] = SKeyState(eKI_Y);
	dik2ekiMap[DIK_U] = SKeyState(eKI_U);
	dik2ekiMap[DIK_I] = SKeyState(eKI_I);
	dik2ekiMap[DIK_O] = SKeyState(eKI_O);
	dik2ekiMap[DIK_P] = SKeyState(eKI_P);
	dik2ekiMap[DIK_LBRACKET] = SKeyState(eKI_LBracket);
	dik2ekiMap[DIK_RBRACKET] = SKeyState(eKI_RBracket);
	dik2ekiMap[DIK_RETURN] = SKeyState(eKI_Enter);
	dik2ekiMap[DIK_LCONTROL] = SKeyState(eKI_LCtrl);
	dik2ekiMap[DIK_A] = SKeyState(eKI_A);
	dik2ekiMap[DIK_S] = SKeyState(eKI_S);
	dik2ekiMap[DIK_D] = SKeyState(eKI_D);
	dik2ekiMap[DIK_F] = SKeyState(eKI_F);
	dik2ekiMap[DIK_G] = SKeyState(eKI_G);
	dik2ekiMap[DIK_H] = SKeyState(eKI_H);
	dik2ekiMap[DIK_J] = SKeyState(eKI_J);
	dik2ekiMap[DIK_K] = SKeyState(eKI_K);
	dik2ekiMap[DIK_L] = SKeyState(eKI_L);
	dik2ekiMap[DIK_SEMICOLON] = SKeyState(eKI_Semicolon);
	dik2ekiMap[DIK_APOSTROPHE] = SKeyState(eKI_Apostrophe);
	dik2ekiMap[DIK_GRAVE] = SKeyState(eKI_Tilde);
	dik2ekiMap[DIK_LSHIFT] = SKeyState(eKI_LShift);
	dik2ekiMap[DIK_BACKSLASH] = SKeyState(eKI_Backslash);
	dik2ekiMap[DIK_Z] = SKeyState(eKI_Z);
	dik2ekiMap[DIK_X] = SKeyState(eKI_X);
	dik2ekiMap[DIK_C] = SKeyState(eKI_C);
	dik2ekiMap[DIK_V] = SKeyState(eKI_V);
	dik2ekiMap[DIK_B] = SKeyState(eKI_B);
	dik2ekiMap[DIK_N] = SKeyState(eKI_N);
	dik2ekiMap[DIK_M] = SKeyState(eKI_M);
	dik2ekiMap[DIK_COMMA] = SKeyState(eKI_Comma);
	dik2ekiMap[DIK_PERIOD] = SKeyState(eKI_Period);
	dik2ekiMap[DIK_SLASH] = SKeyState(eKI_Slash);
	dik2ekiMap[DIK_RSHIFT] = SKeyState(eKI_RShift);
	dik2ekiMap[DIK_MULTIPLY] = SKeyState(eKI_NP_Multiply);
	dik2ekiMap[DIK_LALT] = SKeyState(eKI_LAlt);
	dik2ekiMap[DIK_SPACE] = SKeyState(eKI_Space);
	dik2ekiMap[DIK_CAPSLOCK] = SKeyState(eKI_CapsLock);
	dik2ekiMap[DIK_F1] = SKeyState(eKI_F1);
	dik2ekiMap[DIK_F2] = SKeyState(eKI_F2);
	dik2ekiMap[DIK_F3] = SKeyState(eKI_F3);
	dik2ekiMap[DIK_F4] = SKeyState(eKI_F4);
	dik2ekiMap[DIK_F5] = SKeyState(eKI_F5);
	dik2ekiMap[DIK_F6] = SKeyState(eKI_F6);
	dik2ekiMap[DIK_F7] = SKeyState(eKI_F7);
	dik2ekiMap[DIK_F8] = SKeyState(eKI_F8);
	dik2ekiMap[DIK_F9] = SKeyState(eKI_F9);
	dik2ekiMap[DIK_F10] = SKeyState(eKI_F10);
	dik2ekiMap[DIK_NUMLOCK] = SKeyState(eKI_NumLock);
	dik2ekiMap[DIK_SCROLL] = SKeyState(eKI_ScrollLock);
	dik2ekiMap[DIK_NUMPAD7] = SKeyState(eKI_NP_7);
	dik2ekiMap[DIK_NUMPAD8] = SKeyState(eKI_NP_8);
	dik2ekiMap[DIK_NUMPAD9] = SKeyState(eKI_NP_9);
	dik2ekiMap[DIK_SUBTRACT] = SKeyState(eKI_NP_Substract);
	dik2ekiMap[DIK_NUMPAD4] = SKeyState(eKI_NP_4);
	dik2ekiMap[DIK_NUMPAD5] = SKeyState(eKI_NP_5);
	dik2ekiMap[DIK_NUMPAD6] = SKeyState(eKI_NP_6);
	dik2ekiMap[DIK_ADD] = SKeyState( eKI_NP_Add);
	dik2ekiMap[DIK_NUMPAD1] = SKeyState(eKI_NP_1);
	dik2ekiMap[DIK_NUMPAD2] = SKeyState(eKI_NP_2);
	dik2ekiMap[DIK_NUMPAD3] = SKeyState(eKI_NP_3);
	dik2ekiMap[DIK_NUMPAD0] = SKeyState(eKI_NP_0);
	dik2ekiMap[DIK_DECIMAL] = SKeyState(eKI_NP_Period);
	dik2ekiMap[DIK_F11] = SKeyState(eKI_F11);
	dik2ekiMap[DIK_F12] = SKeyState(eKI_F12);
	dik2ekiMap[DIK_F13] = SKeyState(eKI_F13);
	dik2ekiMap[DIK_F14] = SKeyState(eKI_F14);
	dik2ekiMap[DIK_F15] = SKeyState(eKI_F15);
	dik2ekiMap[DIK_COLON] = SKeyState(eKI_Colon);
	dik2ekiMap[DIK_UNDERLINE] = SKeyState(eKI_Underline);
	dik2ekiMap[DIK_NUMPADENTER] = SKeyState(eKI_NP_Enter);
	dik2ekiMap[DIK_RCONTROL] = SKeyState(eKI_RCtrl);
	dik2ekiMap[DIK_DIVIDE] = SKeyState( eKI_NP_Divide);
	dik2ekiMap[DIK_SYSRQ] = SKeyState(eKI_Print);
	dik2ekiMap[DIK_RALT] = SKeyState(eKI_RAlt);
	dik2ekiMap[DIK_PAUSE] = SKeyState(eKI_Pause);
	dik2ekiMap[DIK_HOME] = SKeyState(eKI_Home);
	dik2ekiMap[DIK_UP] = SKeyState(eKI_Up);
	dik2ekiMap[DIK_PGUP] = SKeyState(eKI_PgUp);
	dik2ekiMap[DIK_LEFT] = SKeyState(eKI_Left);
	dik2ekiMap[DIK_RIGHT] = SKeyState(eKI_Right);
	dik2ekiMap[DIK_END] = SKeyState(eKI_End);
	dik2ekiMap[DIK_DOWN] = SKeyState(eKI_Down);
	dik2ekiMap[DIK_PGDN] = SKeyState(eKI_PgDn);
	dik2ekiMap[DIK_INSERT] = SKeyState(eKI_Insert);
	dik2ekiMap[DIK_DELETE] = SKeyState(eKI_Delete);
}

