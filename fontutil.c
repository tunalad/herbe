#include "fontutil.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// init fonts from array
FontSet* init_fontset(Display *display, int screen, const char **patterns, int num_patterns) {
    FontSet *fontset = malloc(sizeof(FontSet));
    if (!fontset) {
        fprintf(stderr, "malloc failed for fontset\n");
        return NULL;
    }

    fontset->num_fonts = num_patterns;
    fontset->fonts = malloc(num_patterns * sizeof(XftFont *));

    if (!fontset->fonts) {
        fprintf(stderr, "malloc failed for fonts array\n");
        free(fontset);
        return NULL;
    }

    // open each font
    for (int i = 0; i < num_patterns; i++) {
        fontset->fonts[i] = XftFontOpenName(display, screen, patterns[i]);
    }

    return fontset;
}

// free the memory
void free_fontset(Display *display, FontSet *fontset) {
    if (!fontset) return;

    if (fontset->fonts) {
        for (int i = 0; i < fontset->num_fonts; i++) {
            if (fontset->fonts[i]) {
                XftFontClose(display, fontset->fonts[i]);
            }
        }
        free(fontset->fonts);
    }
    free(fontset);
}

// find font for each char
static XftFont* find_font_for_char(FontSet *fontset, Display *display, FcChar32 character) {
    for (int i = 0; i < fontset->num_fonts; i++) {
        if (fontset->fonts[i] && XftCharExists(display, fontset->fonts[i], character)) {
            return fontset->fonts[i];
        }
    }

    return NULL;
}

// draw text w/ fallbacks
void draw_text(Display *display, XftDraw *draw, XftColor *color,
        FontSet *fontset, int x, int y,
        const char *text, int len) {
    if (!fontset || !fontset->fonts || !text) return;

    int i = 0;
    int curr_x = x;

    while (i < len) {
        FcChar32 character;
        int char_len = FcUtf8ToUcs4((const FcChar8 *)&text[i], &character, len - i);
        if (char_len <= 0) break;

        // find font for char
        XftFont *font = find_font_for_char(fontset, display, character);
        if (!font) break;

        // draw char w/ font
        XftDrawStringUtf8(draw, color, font, curr_x, y, (const FcChar8 *)&text[i], char_len);

        // calculate width character
        XGlyphInfo extents;
        XftTextExtentsUtf8(display, font, (const FcChar8 *)&text[i], char_len, &extents);
        curr_x += extents.xOff;

        i += char_len;
    }
}

XftFont* get_primary_font(FontSet *fontset) {
    if (!fontset || !fontset->fonts) return NULL;

    for (int i = 0; i < fontset->num_fonts; i++) {
        if (fontset->fonts[i]) {
            return fontset->fonts[i];
        }
    }

    return NULL;
}
