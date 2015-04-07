/**
  @file Mouse.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef Mouse_h__
#define Mouse_h__

#include "InputManager.h"
class Mouse : public InputDeviceBase
{
public:
	Mouse(SrInputManager* creator);
	~Mouse(void);

	virtual const char* GetDeviceName() const {return "mouse";}

	virtual EDeviceId GetDeviceId() const {return eDI_Mouse;}

	virtual bool Init();

	virtual void Update( bool bFocus );

	void MapDIK2EKI();
	void PostEvent(SKeyState* pSymbol);
	void PostOnlyIfChanged(SKeyState* pSymbol, EInputState newState);
private:
	float2						m_deltas;
	float2						m_oldDeltas;
	float2						m_deltasInertia;
	float						m_mouseWheel;

	std::vector<SKeyState> dik2ekiMap;
};

#endif


