#include "font.h"

namespace Font {
    HFONT hFont = CreateFontA(
        16,                  // Font size
        0,                   // Width
        0,                   // Escapement
        0,                   // Orientation
        FW_NORMAL,           // Weight
        FALSE,               // Italic
        FALSE,               // Underline
        FALSE,               // StrikeOut
        ANSI_CHARSET,        // Character set
        OUT_DEFAULT_PRECIS,  // Output precision
        CLIP_DEFAULT_PRECIS, // Clipping precision
        DEFAULT_QUALITY,     // Quality
        DEFAULT_PITCH,       // Pitch and family
        "Segoe UI"           // Font name
    );

    void ApplyFontToControl(HWND hwnd) {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
}
