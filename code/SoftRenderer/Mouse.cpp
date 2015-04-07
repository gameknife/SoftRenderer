#include "StdAfx.h"
#include "Mouse.h"

#include "mmgr/mmgr.h"
Mouse::Mouse( SrInputManager* creator ):InputDeviceBase(creator, "mouse", GUID_SysMouse)
{
	m_deviceId = eDI_Mouse;
}

Mouse::~Mouse(void)
{
}

bool Mouse::Init()
{
	if (!CreateDirectInputDevice(&c_dfDIMouse2, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND, 4096))
		return false;

	// Set mouse axis mode.
	DIPROPDWORD dipdw = {{sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE}, DIPROPAXISMODE_REL};

	HRESULT hr = GetDevice()->SetProperty( DIPROP_AXISMODE,&dipdw.diph );
	if  (FAILED(hr))
	{
		return false;
	}

	m_deltas.zero();
	m_oldDeltas.zero();
	m_deltasInertia.zero();
	m_mouseWheel = 0.0f;

	Acquire();

	MapDIK2EKI();

	return true;
}

void Mouse::Update( bool bFocus )
{
	HRESULT hr;

	if (!GetDevice()) 
		return;

	m_deltas.zero();
	m_mouseWheel = 0;

	SKeyState* pState = 0;
	EInputState newState;

	DIMOUSESTATE2 dims;
	memset (&dims, 0, sizeof(dims));

	hr = GetDevice()->GetDeviceState(sizeof(DIMOUSESTATE2), &dims);	

	if (FAILED(hr) && Acquire())
	{
		// try again
		hr = GetDevice()->GetDeviceState(sizeof(DIMOUSESTATE2), &dims);
	}

	if (SUCCEEDED(hr))
	{
		m_deltas.set((float)dims.lX, (float)dims.lY);
		m_mouseWheel = (float)dims.lZ;

		// mouse1 - mouse8
		for (int i=0; i<8; ++i)
		{
			newState = (dims.rgbButtons[i]&0x80) ? eIS_Pressed : eIS_Released;
			PostOnlyIfChanged(&(dik2ekiMap[i]), newState);
		}
	}		
	
	//mouse wheel
	newState = (m_mouseWheel > 0.0f) ? eIS_Pressed : eIS_Released;
	PostOnlyIfChanged(&(dik2ekiMap[8]), newState);

	newState = (m_mouseWheel < 0.0f) ? eIS_Pressed : eIS_Released;
	PostOnlyIfChanged(&(dik2ekiMap[9]), newState);

	// mouse movements
	if (m_deltas.lengthsq()>0.0f || m_mouseWheel)
	{
		pState = &(dik2ekiMap[10]);
		pState->state = eIS_Changed;
		pState->value = m_deltas.x;
		PostEvent(pState);

		pState =  &(dik2ekiMap[11]);
		pState->state = eIS_Changed;
		pState->value = m_deltas.y;
		PostEvent(pState);

		pState =  &(dik2ekiMap[12]);
		pState->state = eIS_Changed;
		pState->value = m_mouseWheel;
		PostEvent(pState);
	}
}

//////////////////////////////////////////////////////////////////////////
void Mouse::PostEvent(SKeyState* pSymbol)
{
	// Post Input events.
	
	SInputEvent event;
	pSymbol->MakeEvent(event);
	GetInputManager()->PostInputEvent(event);
}

void Mouse::PostOnlyIfChanged(SKeyState* pState, EInputState newState)
{
	if (pState->state != eIS_Released && newState == eIS_Released )
	{
		pState->state = newState;
		pState->value = 0.0f;
	}
	else if (pState->state == eIS_Released && newState == eIS_Pressed)
	{
		pState->state = newState;
		pState->value = 1.0f;
	}
	else
	{
		return;
	}

	PostEvent(pState);
}

void Mouse::MapDIK2EKI()
{
	dik2ekiMap.clear();
	dik2ekiMap.assign(0xff, SKeyState(eKI_Unknown));

	// ”≥…‰
	dik2ekiMap[0] = SKeyState(eKI_Mouse1);
	dik2ekiMap[1] = SKeyState(eKI_Mouse2);
	dik2ekiMap[2] = SKeyState(eKI_Mouse3);
	dik2ekiMap[3] = SKeyState(eKI_Mouse4);
	dik2ekiMap[4] = SKeyState(eKI_Mouse5);
	dik2ekiMap[5] =  SKeyState(eKI_Mouse6);
	dik2ekiMap[6] = SKeyState(eKI_Mouse7);
	dik2ekiMap[7] = SKeyState(eKI_Mouse8);
	dik2ekiMap[8] = SKeyState(eKI_MouseWheelUp);
	dik2ekiMap[9] = SKeyState(eKI_MouseWheelDown);
	dik2ekiMap[10] =  SKeyState(eKI_MouseX);
	dik2ekiMap[11] = SKeyState(eKI_MouseY);
	dik2ekiMap[12] = SKeyState(eKI_MouseZ);

}