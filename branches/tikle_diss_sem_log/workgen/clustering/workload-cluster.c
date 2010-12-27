#include <X11/extensions/XTest.h>
#define XK_LATIN1
#define XK_MISCELLANY 
#define XK_XKB_KEYS
#include <X11/keysymdef.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

Display *display;
KeySym key[] = { XK_f, XK_u, XK_s, XK_c, XK_a, XK_Tab, XK_space, XK_Return };

void tasks(void) {
	display = XOpenDisplay(0);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, key[0]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, key[1]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, key[2]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, key[3]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, key[4]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, key[5]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, key[6]), False, 1);
	XCloseDisplay(display);
}

int main(int argc, char *argv[])
{
	display = XOpenDisplay(0);
	XWarpPointer(display, None, None, 0, 0, 0, 0, -999, -999);
	XWarpPointer(display, None, None, 0, 0, 0, 0, 50, 30);
	XCloseDisplay(display);

	tasks();

	return 0;
}
