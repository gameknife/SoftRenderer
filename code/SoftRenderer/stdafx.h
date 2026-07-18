// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// C RunTime Header Files
#include <stdlib.h>
#ifdef _WIN32
#include <malloc.h>
#endif
#include <memory.h>
//#include <tchar.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

//#include <atlbase.h>
//#include <atlstr.h>

// TODO: reference additional headers your program requires here
#include "prerequisite.h"
#include "RendererBase.h"
#include "SrRenderContext.h"
#include "SrResourceManager.h"
#include "SrScene.h"
