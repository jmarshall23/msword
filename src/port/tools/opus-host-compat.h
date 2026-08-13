#pragma once

#if defined(__GNUC__) && !defined(_MSC_VER)
#include <strings.h>

#define _stricmp strcasecmp
#define strcmpi _stricmp
#define fcloseall() ((void)0)
#endif
