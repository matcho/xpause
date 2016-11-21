/**
 * Écrit par Mathias en 2010, inspiré et réutilisant du code de xkill (xutils).
 * Dernière modification: 04 XII 2010
 */

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/extensions/shape.h>
#include <X11/Xmu/WinUtil.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/* Include routines to handle parsing defaults */
#include "dsimple.h"

typedef struct {
	long code;
	const char *name;
} binding;

static int bad_window_handler(Display *, XErrorEvent *);
int main(int, char **);
static int GetWindowPID(Window window, Display *dpy);
static char *window_id_format = "0x%lx";
static char GetProcessState(int pid);
static void PausePID(int pid, int pause);

void usage(void) {}

#define getdsp(var,fn) var = fn(dpy, DefaultScreen(dpy))

/* This handler is enabled when we are checking
   to see if the -id the user specified is valid. */
static int bad_window_handler(Display *disp, XErrorEvent *err) {

	char badid[20];

	snprintf(badid, sizeof(badid), window_id_format, err->resourceid);
	Fatal_Error("No such window with id %s.", badid);
	exit (1);
	return 0;
}

int main(int argc, char **argv) {

	int frame = 0;
	int pid;
	char process_state;
	Window window;

	INIT_NAME;

	/* Open display, handle command line arguments */
	Setup_Display_And_Screen(&argc, argv);

	printf("\n");
	printf("xpause: Please select the window whose process you\n");
	printf("          would like to (un)pause by clicking the\n");
	printf("          mouse in that window.\n");

	/* Sélection de la fenêtre par curseur en croix */
	window = Select_Window(dpy);
	/* Permet de quitter par un clic autre que gauche (droit, centre...) */
	if (window == None) exit(0);

	/*
	static XID
	get_window_id(Display *dpy, int screen, int button, char *msg)
	{
		Cursor cursor;		// cursor to use when selecting
		Window root;		// the current root
		Window retwin = None;	// the window that got selected
		int retbutton = -1;		// button used to select window
		int pressed = 0;		// count of number of buttons pressed

	#define MASK (ButtonPressMask | ButtonReleaseMask)

		root = RootWindow (dpy, screen);
		cursor = XCreateFontCursor (dpy, XC_pirate);
		if (cursor == None) {
		fprintf (stderr, "%s:  unable to create selection cursor\n",
			 ProgramName);
		Exit (1);
		}

		printf ("Select %s with ", msg);
		if (button == -1)
		  printf ("any button");
		else
		  printf ("button %d", button);
		printf ("....\n");
		XSync (dpy, 0);

		if (XGrabPointer (dpy, root, False, MASK, GrabModeSync, GrabModeAsync,
					  None, cursor, CurrentTime) != GrabSuccess) {
		fprintf (stderr, "%s:  unable to grab cursor\n", ProgramName);
		Exit (1);
		}

		while (retwin == None || pressed != 0) {
		XEvent event;

		XAllowEvents (dpy, SyncPointer, CurrentTime);
		XWindowEvent (dpy, root, MASK, &event);
		switch (event.type) {
		  case ButtonPress:
			if (retwin == None) {
			retbutton = event.xbutton.button;
			retwin = ((event.xbutton.subwindow != None) ?
				  event.xbutton.subwindow : root);
			}
			pressed++;
			continue;
		  case ButtonRelease:
			if (pressed > 0) pressed--;
			continue;
		}
		}

		XUngrabPointer (dpy, CurrentTime);
		XFreeCursor (dpy, cursor);
		XSync (dpy, 0);

		return ((button == -1 || retbutton == button) ? retwin : None);
	}
	*/

	if (window && !frame) {
		Window root;
		int dummyi;
		unsigned int dummy;

		if (XGetGeometry (dpy, window, &root, &dummyi, &dummyi,
			&dummy, &dummy, &dummy, &dummy) && window != root)
				window = XmuClientWindow (dpy, window);
	}

	/* make sure that the window is valid */
	{
		Window root;
		int x, y;
		unsigned width, height, bw, depth;
		XErrorHandler old_handler;

		old_handler = XSetErrorHandler(bad_window_handler);
		XGetGeometry (dpy, window, &root, &x, &y, &width, &height, &bw, &depth);
		XSync (dpy, False);
		(void) XSetErrorHandler(old_handler);
	}

	/* Récupération du PID à partir de la fenêtre X */
	pid = GetWindowPID(window, dpy);

	if (pid < 0) {
		/* printf("GetWindowPID foiré\n"); */
		return -3;
	}

	process_state = GetProcessState(pid);

	if (process_state == 'T') {
		printf("Resuming process %d ...\n",pid);
		PausePID(pid, 1);
	} else {
		printf("Pausing process %d ...\n",pid);
		PausePID(pid, 0);
	}

	exit(0);
}

/**
 * Retourne le PID du processus affichant la fenêtre window, à partir de la
 * propriété _NET_WM_PID de la fenêtre X
 */
static int GetWindowPID(Window window, Display *dpy) {

	Atom atom,actual_type;
	int screen, status, pid, actual_format;
	char *atom_name;
	unsigned long nitems, bytes_after;
	unsigned char *prop;

	screen = DefaultScreen(dpy);
	atom = XInternAtom(dpy, "_NET_WM_PID", True);
	atom_name = XGetAtomName (dpy,atom);

	/* Requête de statut à X */
	status = XGetWindowProperty(dpy, window, atom, 0, 1024, False, AnyPropertyType,
				&actual_type, &actual_format, &nitems, &bytes_after, &prop
	);

	if (status!=0) {
		printf("Unable to read _NET_WM_PID property\n");
		return -2;
	}

	if (prop == 0) {
		printf("Unable to get window PID\n");
		return -3;
	}

	/* Calcul du PID */
	pid = prop[1] * 256;
	pid += prop[0];
	/* printf("pid of window 0x%x = %d\n",window,pid); */

	return pid;
}

/**
 * Lit dans /proc/<pid>/status pour déterminer le statut du processus, et le retourne;
 * T: stoppé, S: en attente.
 */
static char GetProcessState(int pid) {

	char fic[18];
	char line[1600];
	FILE *pps;

	/* Ouverture du fichier "status" pour le processus ciblé, dans /proc */
	sprintf(fic, "/proc/%d/status", pid);

	pps = fopen(fic,"r");

	if(pps == 0) {
		fprintf(stderr,"Unable to read file %s\n", fic);
		return '0';
	}

	/* Lecture de la deuxième ligne */
	fgets(line, 1600, pps);
	fgets(line, 1600, pps);

	fclose(pps);

	/* Renvoi du huitième caractère */
	return line[7];
}

/**
 * Envoie un signal au processus pid : SIGSTOP si pause == 0, SIGCONT si pause == 1
 */
static void PausePID(int pid, int pause) {

	int sig, ret;

	/* Définition du signal à envoyer */
	if (pause == 0) {
		sig = SIGSTOP;
	} else {
		sig = SIGCONT;
	}
	/* Envoi du signal */
	ret = kill(pid, sig);
	/* Message */
	if (ret == 0) {
		printf("Signal successfully sent\n");
	} else {
		fprintf(stderr,"Unable to send signal (are you the owner of the target process?)\n");
	}
}