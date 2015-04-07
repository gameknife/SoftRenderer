/**
  @file SrAppFramework.h
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef SrAppFramework_h__
#define SrAppFramework_h__

/**
 *@breif SrAppFramework结构
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