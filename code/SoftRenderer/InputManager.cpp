#include "StdAfx.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "InputManager.h"
#include "Keyboard.h"
#include "Mouse.h"

#include "mmgr/mmgr.h"
SrInputManager::SrInputManager(void)
{
}


SrInputManager::~SrInputManager(void)
{
}

//-----------------------------------------------------------------------------
// Name: InitDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
HRESULT SrInputManager::Init( HWND hDlg )
{
	HRESULT hr;

	m_hWnd = hDlg;

	if( FAILED( hr = DirectInput8Create( GetModuleHandle( NULL ), DIRECTINPUT_VERSION,
		IID_IDirectInput8, ( VOID** )&m_pDI, NULL ) ) )
		return hr;

	if (!AddInputDevice(new Keyboard(this))) return false;
	if (!AddInputDevice(new Mouse(this))) return false;

	//ClearKeyState();

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: FreeDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
void SrInputManager::FreeDirectInput()
{
	// Release any DirectInput objects.
	//SAFE_RELEASE( m_pMouse );
	m_pDI->Release();
}

InputDeviceBase::InputDeviceBase( SrInputManager* creator, const char* deviceName, const GUID& guid ):m_creator(creator)
	,m_guid(guid)
{

}

bool InputDeviceBase::CreateDirectInputDevice(const DIDATAFORMAT* dataFormat, DWORD coopLevel, DWORD bufSize)
{
	HRESULT hr = m_creator->getDI()->CreateDevice(m_guid, &m_pDevice, 0);

	if (FAILED(hr))
	{
		return false;
	}

	// get capabilities
	DIDEVCAPS caps;
	caps.dwSize = sizeof(DIDEVCAPS);
	m_pDevice->GetCapabilities(&caps);

	if (caps.dwFlags & DIDC_POLLEDDEVICE)
	{
		m_bNeedsPoll = true;
	}

	if (!dataFormat)
	{
		// build a custom one, here
	}

	hr = m_pDevice->SetDataFormat(dataFormat);
	if (FAILED(hr)) 
	{
		return false;
	}				

	m_pDataFormat	= dataFormat;
	m_dwCoopLevel	= coopLevel;

	hr = m_pDevice->SetCooperativeLevel( m_creator->getHwnd(), m_dwCoopLevel);
	if (FAILED(hr)) 
	{
		return false;
	}						

	DIPROPDWORD dipdw = {{sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE}, bufSize};
	hr = m_pDevice->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);	
	if (FAILED(hr)) 
	{
		return false;
	}

	return true;
}

bool InputDeviceBase::Acquire()
{
	if (!m_pDevice) return false;

	HRESULT hr = m_pDevice->Acquire();

	unsigned char maxAcquire = 10;

	while (hr == DIERR_INPUTLOST && maxAcquire > 0)
	{
		hr = m_pDevice->Acquire();
		--maxAcquire;
	}

	if (FAILED(hr) || maxAcquire == 0) return false;

	return true;
}

///////////////////////////////////////////
bool InputDeviceBase::Unacquire()
{
	return (m_pDevice && SUCCEEDED(m_pDevice->Unacquire()));
}

InputDeviceBase::~InputDeviceBase()
{
	Unacquire();
	m_pDevice->Release();
}

bool SrInputManager::PostInputEvent( SInputEvent& event )
{
	InputEventListeners::iterator it = m_listeners.begin();

	for (; it != m_listeners.end(); ++it)
	{
		if (*it)
		{
			(*it)->OnInputEvent( event );
		}
	}

	// 处理按住的情况
	if (event.state == eIS_Pressed)
	{
		event.keyStatePtr->state = eIS_Down;
		m_holdKeys.push_back(event.keyStatePtr);
	}
	else if (event.keyStatePtr && event.keyStatePtr->state == eIS_Released && !m_holdKeys.empty())
	{
		// remove hold key
		int slot = -1;
		int last = m_holdKeys.size()-1;

		for (int i=last; i>=0; --i)
		{
			if (m_holdKeys[i] == event.keyStatePtr)
			{
				slot = i;
				break;
			}
		}
		if (slot != -1)
		{
			m_holdKeys[slot] = m_holdKeys[last];
			m_holdKeys.pop_back();
		}
	}

	return true;
}

void SrInputManager::AddListener( IInputEventListener* listener )
{
	InputEventListeners::iterator it = m_listeners.begin();

	for (; it != m_listeners.end(); ++it)
	{
		if (*it == listener)
		{
			return;
		}
	}

	m_listeners.push_back(listener);
}

void SrInputManager::RemoveListener( IInputEventListener* listener )
{
	InputEventListeners::iterator it = m_listeners.begin();

	for (; it != m_listeners.end(); ++it)
	{
		if (*it == listener)
		{
			m_listeners.erase(it);
			break;
		}
	}
}

bool SrInputManager::AddInputDevice(InputDeviceBase* pDevice)
{
	if (pDevice)
	{
		if (pDevice->Init())
		{
			m_inputDevices.push_back(pDevice);
			return true;
		}
		delete pDevice;
	}
	return false;
}

void SrInputManager::Destroy()
{
	InputDevices::iterator it = m_inputDevices.begin();
	for (; it != m_inputDevices.end(); ++it)
	{
		if ( *it )
		{
			delete (*it);
		}
	}

	FreeDirectInput();
}

void SrInputManager::Update()
{
	for (uint32 i=0; i < m_holdKeys.size(); ++i)
	{
		SInputEvent event;
		m_holdKeys[i]->MakeEvent(event);
		PostInputEvent( event );
	}

	InputDevices::iterator it = m_inputDevices.begin();
	for (; it != m_inputDevices.end(); ++it)
	{
		if ( *it )
		{
			(*it)->Update(true);
		}
	}
}
