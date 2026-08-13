#pragma once

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)((lp) & 0xffff))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)(((lp) >> 16) & 0xffff))
#endif
