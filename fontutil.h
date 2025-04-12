#ifndef FONTUTIL_H
#define FONTUTIL_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>

typedef struct {
    XftFont **fonts;
    int num_fonts;
} FontSet;

FontSet* init_fontset(Display *display, int screen, const char **patterns, int num_patterns);

void free_fontset(Display *display, FontSet *fontset);

void draw_text(Display *display, XftDraw *draw, XftColor *color,
                            FontSet *fontset, int x, int y,
                            const char *text, int len);

XftFont* get_primary_font(FontSet *fontset);

#endif /* FONTUTIL_H */
