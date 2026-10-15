/*******************************************************************************
* bitwisedom [tests]                                        win32_window_style.c
*
* The window-style example against winuser.h: every style, its second
* names and its combinations, compared with the platform's own at
* compile time. Built where the build targets Windows.
*
* path:      /tests/djinterp/tools/bitwisedom/platform/win32_window_style.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.10
*******************************************************************************/

#include <stdint.h>              // uint32_t
#include <windows.h>             // winuser.h
#include "win32_window_style.h"  // the example, as bitwisedom-gen wrote it


// SAME
//   ours is theirs, compared as the 32 unsigned bits both are.
#define SAME(_ours, _theirs)                                                   \
    _Static_assert((uint32_t)(_ours) == (uint32_t)(_theirs),                   \
                   #_ours " is " #_theirs)


// every window style
SAME(WIN_WS_OVERLAPPED, WS_OVERLAPPED);
SAME(WIN_WS_POPUP, WS_POPUP);
SAME(WIN_WS_CHILD, WS_CHILD);
SAME(WIN_WS_MINIMIZE, WS_MINIMIZE);
SAME(WIN_WS_VISIBLE, WS_VISIBLE);
SAME(WIN_WS_DISABLED, WS_DISABLED);
SAME(WIN_WS_CLIPSIBLINGS, WS_CLIPSIBLINGS);
SAME(WIN_WS_CLIPCHILDREN, WS_CLIPCHILDREN);
SAME(WIN_WS_MAXIMIZE, WS_MAXIMIZE);
SAME(WIN_WS_BORDER, WS_BORDER);
SAME(WIN_WS_DLGFRAME, WS_DLGFRAME);
SAME(WIN_WS_VSCROLL, WS_VSCROLL);
SAME(WIN_WS_HSCROLL, WS_HSCROLL);
SAME(WIN_WS_SYSMENU, WS_SYSMENU);
SAME(WIN_WS_THICKFRAME, WS_THICKFRAME);
SAME(WIN_WS_GROUP, WS_GROUP);
SAME(WIN_WS_TABSTOP, WS_TABSTOP);

// the title bar's second names
SAME(WIN_WS_MINIMIZEBOX, WS_MINIMIZEBOX);
SAME(WIN_WS_MAXIMIZEBOX, WS_MAXIMIZEBOX);

// the combinations, and the older names
SAME(WIN_WS_CAPTION, WS_CAPTION);
SAME(WIN_WS_OVERLAPPEDWINDOW, WS_OVERLAPPEDWINDOW);
SAME(WIN_WS_POPUPWINDOW, WS_POPUPWINDOW);
SAME(WIN_WS_TILED, WS_TILED);
SAME(WIN_WS_ICONIC, WS_ICONIC);
SAME(WIN_WS_SIZEBOX, WS_SIZEBOX);
SAME(WIN_WS_TILEDWINDOW, WS_TILEDWINDOW);
SAME(WIN_WS_CHILDWINDOW, WS_CHILDWINDOW);
