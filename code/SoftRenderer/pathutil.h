/**
  @file pathutil.h
  
  @brief 路径工具函数

  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef pathutil_h__
#define pathutil_h__

extern std::string g_rootPath;

inline void srSetRootPath(const char* root)
{
	g_rootPath = root;
}

inline void getMediaPath(std::string& origin)
{
	origin = g_rootPath + "\\" + origin;
}

#endif // pathutil_h__