#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <errno.h>
#include <mqueue.h>

#include "config.h"
#include "fontutil.h"

#include "herbe.h"

static Display *display;
XftFont *font;
FontSet *fontset;
static Window window;
int num_of_lines;
char **lines;
static int exit_code = EXIT_DISMISS;
static volatile sig_atomic_t sig_recieved;

struct mq_object {
	pid_t pid;
	long timestamp;
	char  buffer[1024];
};
long lastTimestamp;

const char herbe_usage_string[] =
"herbe [OPTION...] <BODY>\n"
"Options:\n"
"    -h        This help text\n"
"    -v        Prints current version\n"
"    -t TIME   Set notification duration to TIME seconds"
;

static void print_version(void)
{
	printf("herbe v%s\n", VERSION_STRING);
}

static void print_help(void)
{
	print_version();
	printf("Usage: %s\n", herbe_usage_string);
	exit(EXIT_ACTION);
}

static void usage(const char *err)
{
	fprintf(stderr, "Usage: %s\n", err);
	exit(EXIT_FAIL);
}

static void die(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(EXIT_FAIL);
}

static int handle_options(const char ***argv, int *argc)
{
    int skip_next_arg = 0;
    int param_count = 0;

    char **argv_in = (char **) *argv;	// original argv
    char **argv_out = argv_in;			// trimmed argv

    for (int i = 0; i < *argc; i++) {
        const char *arg = argv_in[i];

        if (skip_next_arg) {
            skip_next_arg = 0;
            continue;
        }

        if (arg && arg[0] == '-') {
            if (!strcmp(arg, "-h")) {
                print_help();
                continue;
            } else if (!strcmp(arg, "-v")) {
                print_version();
                exit(EXIT_ACTION);
            }

			// options with params
            if (i + 1 < *argc && argv_in[i + 1] && argv_in[i + 1][0] != '-') {
                skip_next_arg = 1;

				// time duration option
				if (!strcmp(arg, "-t")) {
					char *endptr;
					errno = 0;
					long val = strtol(argv_in[i + 1], &endptr, 10);

					// making sure it's a number
					if (*endptr != '\0' || errno == ERANGE || val <= 0) {
						fprintf(stderr, "Error: -t requires a positive whole number\n");
						exit(EXIT_FAIL);
					}
					duration = (int)val;
				}

				// TODO: -r to change notification ID (instead of using HERBE_ID variable)
				// TODO: -u to set urgency leve (will be customizable via config.h file). Just a visual thing ig?
				// TODO: -i to set icons
				// TODO: -h hints thing. Only usage I know for it is the percentage bar? will have to use --help for help option later
            }
            continue; // skip option
        }

        // copy valid non-option argument
        *argv_out++ = argv_in[i];
        param_count++;
    }

    *argv_out = NULL;
    return *argc = param_count;
}


static int get_max_len(char *string, XftFont *font, int max_text_width)
{
	int eol = strlen(string);
	XGlyphInfo info;
	XftTextExtentsUtf8(display, font, (FcChar8 *)string, eol, &info);

	if (info.width > max_text_width)
	{
		eol = max_text_width / font->max_advance_width;
		info.width = 0;

		while (info.width < max_text_width)
		{
			eol++;
			XftTextExtentsUtf8(display, font, (FcChar8 *)string, eol, &info);
		}

		eol--;
	}

	for (int i = 0; i < eol; i++)
		if (string[i] == '\n')
		{
			string[i] = ' ';
			return ++i;
		}

	while (eol && (string[eol] & 0xC0) == 0x80)
		--eol;

	if (info.width <= max_text_width)
		return eol;

	int temp = eol;

	while (string[eol] != ' ' && eol)
		--eol;

	if (eol == 0)
		return temp;
	else
		return ++eol;
}

void freeLines() {
	if(lines) {
		for (int i = 0; i < num_of_lines; i++)
			free(lines[i]);
		free(lines);
	}
}

void constructLines(char* strList[], int numberOfStrings) {
	freeLines();
	int max_text_width = width - 2 * padding;
	num_of_lines = 0;
	int lines_size = 5;
	lines = malloc(lines_size * sizeof(char *));
	if (!lines)
		die("malloc failed");

	for (int i = 0; i < numberOfStrings; i++)
	{
		for (unsigned int eol = get_max_len(strList[i], font, max_text_width); eol;
				strList[i] += eol, num_of_lines++, eol = get_max_len(strList[i], font, max_text_width))
		{
			if (lines_size <= num_of_lines)
			{
				lines = realloc(lines, (lines_size += 5) * sizeof(char *));
				if (!lines)
					die("realloc failed");
			}
			lines[num_of_lines] = malloc((eol + 1) * sizeof(char));
			if (!lines[num_of_lines])
				die("malloc failed");

			strncpy(lines[num_of_lines], strList[i], eol);
			lines[num_of_lines][eol] = '\0';
		}
	}
}

void reload(union sigval sv);
void readAllEvents(mqd_t mqd) {
	struct sigevent event = {.sigev_notify=SIGEV_THREAD, .sigev_signo=SIGHUP, .sigev_value.sival_int=mqd, .sigev_notify_function=reload};

	if(mq_notify(mqd, &event) == -1) {
		perror("mq_notify failed");
		exit(EXIT_FAIL);
	}

	struct mq_object object;

	while(1) {
		int ret = mq_receive(mqd, (char*)&object, sizeof(object), NULL);
		if(ret==-1) {
			if(errno == EAGAIN)
				return;
			perror("mq_receive");
			exit(EXIT_FAIL);
		}
		if(object.timestamp && lastTimestamp > object.timestamp)
			return;
		if(object.timestamp)
			lastTimestamp = object.timestamp;
		char *buffer = object.buffer;

		constructLines(&buffer, 1);
		kill(object.pid, SIGTERM);
	}
}
void reload(union sigval sv) {
	// we've already timed out
	if(alarm(duration) == 0)
		return;
	readAllEvents(sv.sival_int);
	XEvent event;
	event.type = Expose;
	XSendEvent(display, window, 0, 0, &event);
	XFlush(display);
}


static void expire(int sig)
{
	sig_recieved = sig_recieved ? sig_recieved : sig;
}


void exitSuccess() {
       exit(EXIT_ACTION);
}

void read_y_offset(unsigned int **offset, int *id) {
    int shm_id = shmget(8432, sizeof(unsigned int), IPC_CREAT | 0660);
    if (shm_id == -1) die("shmget failed");

    *offset = (unsigned int *)shmat(shm_id, 0, 0);
    if (*offset == (unsigned int *)-1) die("shmat failed\n");
    *id = shm_id;
}

void free_y_offset(int id) {
    shmctl(id, IPC_RMID, NULL);
}

int main(int argc, char *argv[])
{
	const char **av = (const char **) argv;

	if (argc == 1)
		usage(herbe_usage_string);

	/* Look for flags.. */
    handle_options(&av, &argc);

	const char* id =getenv("HERBE_ID");
	mqd_t mqd=-1;
	if(id) {
		struct mq_attr attr = { .mq_maxmsg = 10, .mq_msgsize = sizeof(struct mq_object) };
		mqd = mq_open(id, O_RDWR|O_CREAT|O_NONBLOCK, 0722, &attr);
		if(mqd==-1){
			perror("mq_open");
			die("mq_open");
		}
		while (1) {
			if(flock(mqd, LOCK_EX|LOCK_NB) == 0) {
				// if we get the lock, register for events
				break;
			}
			if(errno != EWOULDBLOCK) {
				perror("flock");
				exit(EXIT_FAIL);
			}
			// someone else is listening for events
			char* ts_str = getenv("NOTIFICATION_ID");
			lastTimestamp = ts_str?atol(ts_str):0;
			struct mq_object object = {getpid(), lastTimestamp, {0}};
			char *buffer=object.buffer;
			for(int i=1;i<argc;i++) {
				strcat(buffer, argv[i]);
				strcat(buffer, "\n");
			}
			signal(SIGTERM, exitSuccess);
			if(mq_send(mqd, (char*)&object, sizeof(object), 1)==-1) {
				perror("mq_send");
				exit(EXIT_FAIL);
			}
			signal(SIGALRM, SIG_IGN);
			alarm(1);
			pause();
		}
	}

	struct sigaction act_expire, act_ignore;

	act_expire.sa_handler = expire;
	act_expire.sa_flags = SA_RESTART;
	sigemptyset(&act_expire.sa_mask);

	act_ignore.sa_handler = SIG_IGN;
	act_ignore.sa_flags = 0;
	sigemptyset(&act_ignore.sa_mask);

	sigaction(SIGALRM, &act_expire, 0);
	sigaction(SIGTERM, &act_expire, 0);
	sigaction(SIGINT, &act_expire, 0);

	sigaction(SIGUSR1, &act_ignore, 0);
	sigaction(SIGUSR2, &act_ignore, 0);

	if (!(display = XOpenDisplay(0)))
		die("Cannot open display");

	int screen = DefaultScreen(display);
	Visual *visual = DefaultVisual(display, screen);
	Colormap colormap = DefaultColormap(display, screen);

	int screen_width = DisplayWidth(display, screen);
	int screen_height = DisplayHeight(display, screen);

	XSetWindowAttributes attributes;
	attributes.event_mask = ExposureMask | ButtonPressMask;
	attributes.override_redirect = True;
	XftColor color;
	if (!XftColorAllocName(display, visual, colormap, background_color, &color))
		die("Failed to allocate background color");
	attributes.background_pixel = color.pixel;
	if (!XftColorAllocName(display, visual, colormap, border_color, &color))
		die("Failed to allocate border color");
	attributes.border_pixel = color.pixel;

	fontset = init_fontset(display, screen, font_pattern, sizeof(font_pattern) / sizeof(font_pattern[0]));
	if (!fontset)
		die("Failed to initialize fonts");
	font = get_primary_font(fontset); // Set global font to primary font for measurements


	constructLines(argv+1, argc-1);

    int y_offset_id;
    unsigned int *y_offset;
    read_y_offset(&y_offset, &y_offset_id);

	unsigned int text_height = font->ascent - font->descent;
	unsigned int height = (num_of_lines - 1) * line_spacing + num_of_lines * text_height + 2 * padding;
	unsigned int x = pos_x;
	unsigned int y = pos_y + *y_offset;

    unsigned int used_y_offset = (*y_offset) += height + padding;

	if (corner == TOP_RIGHT || corner == BOTTOM_RIGHT)
		x = screen_width - width - border_size * 2 - x;

	if (corner == BOTTOM_LEFT || corner == BOTTOM_RIGHT)
		y = screen_height - height - border_size * 2 - y;

	window = XCreateWindow(display, RootWindow(display, screen), x, y, width, height, border_size, DefaultDepth(display, screen),
						   CopyFromParent, visual, CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask, &attributes);

	XftDraw *draw = XftDrawCreate(display, window, visual, colormap);
	if (!draw)
		die("Failed to create Xft drawable object");
	XftColorAllocName(display, visual, colormap, font_color, &color);

	XMapWindow(display, window);

	sigaction(SIGUSR1, &act_expire, 0);
	sigaction(SIGUSR2, &act_expire, 0);

	if (duration != 0)
		alarm(duration);

	if(id) {
		readAllEvents(mqd);
	}

	for (;;)
	{
		XEvent event;
		struct pollfd pfd = {
			.fd = ConnectionNumber(display),
			.events = POLLIN,
		};
		int pending = XPending(display) > 0 || poll(&pfd, 1, -1) > 0;

		if (sig_recieved)
		{
			exit_code = sig_recieved == SIGUSR2 ? EXIT_ACTION : EXIT_DISMISS;
			break;
		}
		else if (pending)
			XNextEvent(display, &event);
		else
			continue;

		if (event.type == Expose)
		{
			XClearWindow(display, window);
			for (int i = 0; i < num_of_lines; i++)
				draw_text(display, draw, &color, fontset,
						padding, line_spacing * i + text_height * (i + 1) + padding,
						lines[i], strlen(lines[i]));
		}
		else if (event.type == ButtonPress)
		{
			if (event.xbutton.button == DISMISS_BUTTON)
				break;
			else if (event.xbutton.button == ACTION_BUTTON)
			{
				exit_code = EXIT_ACTION;
				break;
			}
		}
	}

    if (used_y_offset == *y_offset) free_y_offset(y_offset_id);

	freeLines();

	XftDrawDestroy(draw);
	XftColorFree(display, visual, colormap, &color);
	free_fontset(display, fontset);
	XCloseDisplay(display);

	if(id) {
		mq_close(mqd);
	}

	return exit_code;
}
