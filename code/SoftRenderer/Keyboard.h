/**
  @file Keyboard.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef Keyboard_h__
#define Keyboard_h__

#include "InputManager.h"
class Keyboard : public InputDeviceBase
{
public:
	Keyboard(SrInputManager* creator);
	~Keyboard(void);

	virtual const char* GetDeviceName() const {return "keyboard";}

	virtual EDeviceId GetDeviceId() const {return eDI_Keyboard; }

	virtual bool Init();

	virtual void Update( bool bFocus );

	virtual void MapDIK2EKI();
	
private:
	void ProcessKey(uint32 devSpecId, bool pressed);

	std::vector<SKeyState> dik2ekiMap;
};

#endif



