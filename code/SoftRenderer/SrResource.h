/**
  @file SrResource.h
  
  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrResource_h__
#define SrResource_h__

#include "prerequisite.h"

class SrResource
{
public:
	SrResource(const char* name, EResourceType type):m_name(name),m_type(type) {}
	virtual ~SrResource(void) {}

	const char* getName() const {return m_name.c_str();} 
	EResourceType getType() const {return m_type;}

protected:
	std::string m_name;
	EResourceType m_type;
};


#endif // SrResource_h__

