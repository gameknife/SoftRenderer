/**
  @file SrAppFramework.h
  
  @author Kaiming

history
  ver:1.0
   
 */

#ifndef SrAppFramework_h__
#define SrAppFramework_h__

/**
 *@breif SrAppFramework
 */
SR_ALIGN class SrAppFramework
{
public:
	virtual ~SrAppFramework() {}
	virtual void OnInit() =0;
	virtual void OnUpdate() =0;
	virtual void OnDestroy() =0;
};
typedef std::vector<SrAppFramework*> SrApps;

#endif