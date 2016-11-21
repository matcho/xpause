#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
/*
 * Other_stuff.h: Definitions of routines in other_stuff.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 */

#include "dsimple.h"

/*
 * Just_display: A group of routines designed to make the writing of simple
 *               X11 applications which open a display but do not open
 *               any windows much faster and easier.  Unless a routine says
 *               otherwise, it may be assumed to require program_name, dpy,
 *               and screen already defined on entry.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 */

/* This stuff is defined in the calling program by just_display.h */
char    *program_name = "unknown_program";
Display *dpy = NULL;
int      screen = 0;

/*
 * Get_Display_Name (argc, argv) Look for -display, -d, or host:dpy (obselete)
 * If found, remove it from command line.  Don't go past a lone -.
 */
char *Get_Display_Name(
    int *pargc,  /* MODIFIED */
    char **argv) /* MODIFIED */
{
    int argc = *pargc;
    char **pargv = argv+1;
    char *displayname = NULL;
    int i;

    for (i = 1; i < argc; i++) {
	char *arg = argv[i];

	if (!strcmp (arg, "-display") || !strcmp (arg, "-d")) {
	    if (++i >= argc) usage ();

	    displayname = argv[i];
	    *pargc -= 2;
	    continue;
	}
	if (!strcmp(arg,"-")) {
		while (i<argc)
			*pargv++ = argv[i++];
		break;
	}
	*pargv++ = arg;
    }

    *pargv = NULL;
    return (displayname);
}

/*
 * Open_Display: Routine to open a display with correct error handling.
 *               Does not require dpy or screen defined on entry.
 */
Display *Open_Display(char *display_name)
{
	Display *d;

	d = XOpenDisplay(display_name);
	if (d == NULL) {
	    fprintf (stderr, "%s:  unable to open display '%s'\n",
		     program_name, XDisplayName (display_name));
	    usage ();
	    /* doesn't return */
	}

	return(d);
}

/*
 * Setup_Display_And_Screen: This routine opens up the correct display (i.e.,
 *                           it calls Get_Display_Name) and then stores a
 *                           pointer to it in dpy.  The default screen
 *                           for this display is then stored in screen.
 *                           Does not require dpy or screen defined.
 */
void Setup_Display_And_Screen(
    int *argc,      /* MODIFIED */
    char **argv)    /* MODIFIED */
{
	char *displayname = NULL;
        
        displayname = Get_Display_Name(argc, argv);
	dpy = Open_Display (displayname);
	screen = XDefaultScreen(dpy);
}

/*
 * Close_Display: Close display
 */
void Close_Display(void)
{
    if (dpy == NULL)
      return;
      
    XCloseDisplay(dpy);
    dpy = NULL;
}

/*
 * Other_stuff: A group of routines which do common X11 tasks.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 */

/*
 * Routine to let user select a window using the mouse
 */
Window Select_Window(Display *dpy)
{
  int status;
  Cursor cursor;
  XEvent event;
  Window target_win = None, root = RootWindow(dpy,screen);
  int buttons = 0;
  int cancel = 0;

  /* Make the target cursor */
  cursor = XCreateFontCursor(dpy, XC_crosshair);

  /* Grab the pointer using target cursor, letting it room all over */
  status = XGrabPointer(dpy, root, False,
			ButtonPressMask|ButtonReleaseMask, GrabModeSync,
			GrabModeAsync, root, cursor, CurrentTime);
  if (status != GrabSuccess) Fatal_Error("Can't grab the mouse.");

  /* Let the user select a window... */
  while (((target_win == None) || (buttons != 0)) && (cancel == 0)) {
    /* allow one more event */
    XAllowEvents(dpy, SyncPointer, CurrentTime);
    XWindowEvent(dpy, root, ButtonPressMask|ButtonReleaseMask, &event);
    switch (event.type) {
    case ButtonPress:
	  if (event.xbutton.button == 1) {
		if (target_win == None) {
		  target_win = event.xbutton.subwindow; /* window selected */
		  if (target_win == None) target_win = root;
		}
	  } else {
		  printf("Cancelling\n");
		  target_win = None;
		  cancel = 1;
	  }
      buttons++;
      break;
    case ButtonRelease:
      if (buttons > 0) /* there may have been some down before we started */
	buttons--;
       break;
    }
  } 

  XUngrabPointer(dpy, CurrentTime);      /* Done with pointer */

  return(target_win);
}

/*
 * Standard fatal error routine - call like printf
 * Does not require dpy or screen defined.
 */
void Fatal_Error(char *msg, ...)
{
	va_list args;
	fflush(stdout);
	fflush(stderr);
	fprintf(stderr, "%s: error: ", program_name);
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, "\n");
        Close_Display();
	exit(EXIT_FAILURE);
}