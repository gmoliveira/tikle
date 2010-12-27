#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

int main(int argc, char *argv[]) {
	int x, j ,altura, w, total=0;
	float y,i;

	XEvent event;

	Display *display;

	display = XOpenDisplay(0);
	XWarpPointer(display, None, None, 0, 0, 0, 0, -999, -999);
	XCloseDisplay(display);

	if (argc > 1) {
		if (strcmp(argv[1],"passat")==0)
			altura = 70;
		else if (strcmp(argv[1],"fusca")==0)
			altura = 150;
		else if (strcmp(argv[1],"dkv")==0)
			altura = 230;
		else if (strcmp(argv[1],"porsche")==0)
			altura = 310;

		printf("Running Workload for node %s\n", argv[1]);
		printf("This will take almost a minute.\n\n");

		j = 5;
		for (x=0;x<70;x++) {
			y = (30*cos(x)+altura);
			i = (30*cos(x+1)+altura);
			if (i > y) {
				for(;y<i;y++) {
					usleep(40000);
					if ((int)y%4==0) j++;
					display = XOpenDisplay(0);
					XWarpPointer(display, None, None, 0, 0, 0, 0, -999, -999);
					XCloseDisplay(display);

					display = XOpenDisplay(0);
					XWarpPointer(display, None, None, 0, 0, 0, 0, j, (int)y);
					XCloseDisplay(display);
					total++;
				}
			} else {
				for(;y>i;y--) {
					usleep(40000);
					if ((int)y%3==0) j++;
					display = XOpenDisplay(0);
					XWarpPointer(display, None, None, 0, 0, 0, 0, -999, -999);
					XCloseDisplay(display);

					total++;
					display = XOpenDisplay(0);
					XWarpPointer(display, None, None, 0, 0, 0, 0, j, (int)y);
					XCloseDisplay(display);
				}
			}
		}
		printf("Execution ended\nExiting.\n\n");
		printf("Total de movimentos: %d\n",total);
	}
	return 0;
}
