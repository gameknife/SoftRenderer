/**
  @file pathutil.h
  
  @brief 路径工具函数

  @author yikaiming

  更改日志 history
  ver:1.0
   
 */

#ifndef pathutil_h__
#define pathutil_h__

inline void getMediaPath(std::string& origin)
{
	char buffer[MAX_PATH];
	char* strLastSlash = NULL;
	GetModuleFileName( NULL, buffer, MAX_PATH );
	buffer[MAX_PATH - 1] = 0;

	strLastSlash = strrchr( buffer, '\\' );
	if (strLastSlash)
	{
		*(strLastSlash + 1) = '\0';
	}

	origin = buffer + origin;
}

#endif // pathutil_h__