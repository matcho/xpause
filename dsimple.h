    /* Simple helper macros */
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif /* MAX */
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif /* MIN */

    /* Global variables used by routines in dsimple.c */

extern char *program_name;                   /* Name of this program */
extern Display *dpy;                         /* The current display */
extern int screen;                           /* The current screen */

#define INIT_NAME program_name=argv[0]        /* use this in main to setup
                                                 program_name */

    /* Declaritions for functions in dsimple.c */

char *Get_Display_Name(int *, char **);
Display *Open_Display(char *);
void Setup_Display_And_Screen(int *, char **);
void Close_Display(void);
void usage(void);

#define X_USAGE "[host:display]"              /* X arguments handled by
						 Get_Display_Name */

Window Select_Window(Display *);
#ifdef __GNUC__
void Fatal_Error(char *, ...) __attribute__((__noreturn__));
#else
void Fatal_Error(char *, ...);
#endif