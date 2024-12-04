#ifndef FONT_H
#define FONT_H

#include <windows.h>

namespace Font {
    extern HFONT hFont;

    void ApplyFontToControl(HWND hwnd);
}

#endif
