/**
  @file pathutil.h
  
  @brief ·�����ߺ���

  @author yikaiming

  ������־ history
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
	origin = g_rootPath + "/" + origin;
}

#endif // pathutil_h__