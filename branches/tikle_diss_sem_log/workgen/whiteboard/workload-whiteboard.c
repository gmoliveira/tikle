#include <X11/extensions/XTest.h>
#define XK_LATIN1
#define XK_MISCELLANY 
#define XK_XKB_KEYS
#include <X11/keysymdef.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

Display *display;
KeySym fusca[] = { XK_f, XK_u, XK_s, XK_c, XK_a, XK_Tab, XK_space };
KeySym dkv[] = { XK_d, XK_k, XK_v, XK_Tab, XK_space };
KeySym porsche[] = { XK_p, XK_o, XK_r, XK_s, XK_c, XK_h, XK_e, XK_Tab, XK_space };
KeySym passat[] = { XK_p, XK_a, XK_s, XK_t, XK_Tab, XK_space };

void ffusca(void) {
	display = XOpenDisplay(0);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, fusca[0]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, fusca[1]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, fusca[2]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, fusca[3]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, fusca[4]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, fusca[5]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, fusca[6]), False, 1);
	XCloseDisplay(display);
}

void fdkv(void) {
	display = XOpenDisplay(0);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, dkv[0]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, dkv[1]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, dkv[2]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, dkv[3]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, dkv[4]), False, 1);
	XCloseDisplay(display);
}

void fporsche(void) {
	display = XOpenDisplay(0);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, porsche[0]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, porsche[1]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, porsche[2]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, porsche[3]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, porsche[4]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, porsche[5]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, porsche[6]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, porsche[7]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, porsche[8]), False, 1);
	XCloseDisplay(display);
}

void fpassat(void) {
	display = XOpenDisplay(0);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, passat[0]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, passat[1]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, passat[2]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, passat[2]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, passat[1]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, passat[3]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, passat[4]), False, 1);
	XTestFakeKeyEvent(display, XKeysymToKeycode(display, passat[5]), False, 1);
	XCloseDisplay(display);
}

int main(int argc, char *argv[])
{
	display = XOpenDisplay(0);
	XWarpPointer(display, None, None, 0, 0, 0, 0, -999, -999);
	XWarpPointer(display, None, None, 0, 0, 0, 0, 50, 30);
	XCloseDisplay(display);
	if (argc < 2)
		printf("Argumento invalido\n");
	else if (strcmp(argv[1], "fusca") == 0)
		ffusca();
	else if (strcmp(argv[1], "dkv") == 0)
		fdkv();
	else if (strcmp(argv[1], "porsche") == 0)
		fporsche();
	else if (strcmp(argv[1], "passat") == 0)
		fpassat();
	else
		printf("Argumento invalido\n");

	return 0;
}
