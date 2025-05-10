/* appearance */
static const char *font_pattern[] = { "DejaVu Sans Mono:size=8", "Joypixels:size=8", "DejaVuSansM Nerd Font Propo:size=8", "monospace:size=8" };
static const unsigned line_spacing = 5;
static const unsigned int padding = 8;

static const unsigned int width = 300;
static const unsigned int border_size = 2;
static const unsigned int pos_x = 15;
static const unsigned int pos_y = 55;

static const NotificationStyle herbe_colors[] = {
    /*                  bg          border      fg      */
    [SchemeNorm]    = { "#285577",  "#ececec",  "#ffffff" },
    [SchemeLow]     = { "#3e3e3e",  "#ececec",  "#ffffff" },
    [SchemeCrit]    = { "#900000",  "#ff0000",  "#ffffff" },
};

enum corners { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };
enum corners corner = TOP_RIGHT;

static int duration = 5; /* in seconds */
static int should_wait = 0; /* 0 means no waiting for notification to finish */

#define DISMISS_BUTTON Button1
#define ACTION_BUTTON Button3
