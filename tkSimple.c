/* 
 *-----------------------------------------------------------------------
 * tkSimple.c --
 *
 *   Implementation of a Very simple window which relies on C code for
 *   almost all of its event handlers.
 *
 *-----------------------------------------------------------------------
 */

#ifdef TCL_WRAPPER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tk.h>
/* 
#include <tkInt.h>
#include <tkIntDecls.h>     not portable! (jdk)  
    this trick seems very broken on my machine so:
declare this thing here, and hope the linker can resolve it
*/
EXTERN int		TkpUseWindow _ANSI_ARGS_((Tcl_Interp * interp, 
				Tk_Window tkwin, CONST char * string));

/* Backwards compatibility to tk8.3 and earlier */
#if TK_MAJOR_VERSION == 8
  #if TK_MINOR_VERSION <= 3
    #define Tk_SetClassProcs(a,b,c) TkSetClassProcs(a,b,c)
  #endif
#endif

#ifndef CONST84
#define CONST84
#endif

/*
 * A data structure of the following type is kept for each
 * simple that currently exists for this process:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the simple.  NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up. */
    Display *display;		/* Display containing widget.  Used, among
				 * other things, so that resources can be
				 * freed even after tkwin has gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with widget.  Used
				 * to delete widget command. */
    Tcl_Command widgetCmd;	/* Token for simple's widget command. */
    char *className;		/* Class name for widget (from configuration
				 * option).  Malloc-ed. */
    int width;			/* Width to request for window.  <= 0 means
				 * don't request any size. */
    int height;			/* Height to request for window.  <= 0 means
				 * don't request any size. */
    XColor *background;		/* background pixel used by XClearArea */
    char *useThis;		/* If the window is embedded, this points to
				 * the name of the window in which it is
				 * embedded (malloc'ed).  For non-embedded
				 * windows this is NULL. */
    char *exitProc;		/* Callback procedure upon window deletion. */
    char *commandProc;		/* Callback procedure for commands sent to the window */
    char *mydata;		/* This space for hire. */
    int flags;			/* Various flags;  see below for
				 * definitions. */
} Simple;

/*
 * Flag bits for simples:
 *
 * GOT_FOCUS:	non-zero means this widget currently has the input focus.
 */

#define GOT_FOCUS		1

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_COLOR, "-background", "background", "Background",
	"White", Tk_Offset(Simple, background), 0},
    {TK_CONFIG_SYNONYM, "-bg", "background", (char *)NULL,
	(char *)NULL, 0, 0},
    {TK_CONFIG_PIXELS, "-height", "height", "Height",
	"0", Tk_Offset(Simple, height), 0},
    {TK_CONFIG_PIXELS, "-width", "width", "Width",
	"0", Tk_Offset(Simple, width), 0},
    {TK_CONFIG_STRING, "-use", "use", "Use",
	"", Tk_Offset(Simple, useThis), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-exitproc", "exitproc", "ExitProc",
	"", Tk_Offset(Simple, exitProc), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-commandproc", "commandproc", "CommandProc",
	"", Tk_Offset(Simple, commandProc), TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-data", "data", "Data",
	"", Tk_Offset(Simple, mydata), TK_CONFIG_NULL_OK},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		ConfigureSimple _ANSI_ARGS_((Tcl_Interp *interp,
			    Simple *simplePtr, int objc, Tcl_Obj *CONST objv[],
			    int flags));
static void		DestroySimple _ANSI_ARGS_((char *memPtr));
static void		SimpleCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static void		SimpleEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		SimpleWidgetObjCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));


/*
 *--------------------------------------------------------------
 *
 * Tk_SimpleObjCmd --
 *
 *	This procedure is invoked to process the "simple"
 *	Tcl command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.  These procedures are just wrappers;
 *	they call ButtonCreate to do all of the real work.
 *
 *--------------------------------------------------------------
 */

int
Tk_SimpleObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    Simple *simplePtr;
    Tk_Window new;
    char *arg, *useOption;
    int i, c;  /* , depth; (jdk) */
    size_t length;
    unsigned int mask;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?options?");
	return TCL_ERROR;
    }

    /*
     * Pre-process the argument list.  Scan through it to find any
     * "-use" option, or the "-main" option.  If the "-main" option
     * is selected, then the application will exit if this window
     * is deleted.
     */

    useOption = NULL;
    for (i = 2; i < objc; i += 2) {
	arg = Tcl_GetStringFromObj(objv[i], (int *) &length);
	if (length < 2) {
	    continue;
	}
	c = arg[1];
	if ((c == 'u') && (strncmp(arg, "-use", length) == 0)) {
	    useOption = Tcl_GetString(objv[i+1]);
	}
    }

    /*
     * Create the window, and deal with the special option -use.
     */

    if (tkwin != NULL) {
	new = Tk_CreateWindowFromPath(interp, tkwin, Tcl_GetString(objv[1]),
		NULL);
    }
    if (new == NULL) {
	goto error;
    }
    Tk_SetClass(new, "Simple");
    if (useOption == NULL) {
	useOption = (char *)Tk_GetOption(new, "use", "Use");
    }
    if (useOption != NULL) {
	if (TkpUseWindow(interp, new, useOption) != TCL_OK) {
	    goto error;
	}
    }

    /*
     * Create the widget record, process configuration options, and
     * create event handlers.  Then fill in a few additional fields
     * in the widget record from the special options.
     */

    simplePtr = (Simple *) ckalloc(sizeof(Simple));
    simplePtr->tkwin = new;
    simplePtr->display = Tk_Display(new);
    simplePtr->interp = interp;
    simplePtr->widgetCmd = Tcl_CreateObjCommand(interp,
	    Tk_PathName(new), SimpleWidgetObjCmd,
	    (ClientData) simplePtr, SimpleCmdDeletedProc);
    simplePtr->className = NULL;
    simplePtr->width = 0;
    simplePtr->height = 0;
    simplePtr->background = NULL;
    simplePtr->useThis = NULL;
    simplePtr->exitProc = NULL;
    simplePtr->commandProc = NULL;
    simplePtr->flags = 0;
    simplePtr->mydata = NULL;

    /*
     * Store backreference to simple widget in window structure.
     */
    Tk_SetClassProcs(new, NULL, (ClientData) simplePtr);

    /* We only handle focus and structure events, and even that might change. */
    mask = StructureNotifyMask|FocusChangeMask|NoEventMask;
    Tk_CreateEventHandler(new, mask, SimpleEventProc, (ClientData) simplePtr);

    if (ConfigureSimple(interp, simplePtr, objc-2, objv+2, 0) != TCL_OK) {
	goto error;
    }
    Tcl_SetResult(interp, Tk_PathName(new), TCL_STATIC);
    return TCL_OK;

    error:
    if (new != NULL) {
	Tk_DestroyWindow(new);
    }
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * SimpleWidgetObjCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a simple widget.  See the user
 *	documentation for details on what it does.  If the
 *	"-commandProc" option has been set for the window,
 *	then any unknown command (neither "cget" nor "configure")
 *	will execute the command procedure first, then attempt
 *	to execute the remainder of the command as an independent
 *	Tcl command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
SimpleWidgetObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Information about simple widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    static char *simpleOptions[] = {
	"cget", "configure", (char *) NULL
    };
    enum options {
	SIMPLE_CGET, SIMPLE_CONFIGURE
    };
    register Simple *simplePtr = (Simple *) clientData;
    int result = TCL_OK, index;
    size_t length;
    int c, i;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1],
		(CONST84 char **)simpleOptions, "option", 0,
		&index) != TCL_OK) {
	if (simplePtr->commandProc != NULL) {
	   Tcl_ResetResult(simplePtr->interp);
	   if (Tcl_EvalEx(simplePtr->interp, simplePtr->commandProc, -1, 0)
			!= TCL_OK)
	      return TCL_ERROR;
	   else
	      return Tcl_EvalObjv(simplePtr->interp, --objc, ++objv, TCL_EVAL_DIRECT);
	}
	else
	   return TCL_ERROR;
    }
    Tcl_Preserve((ClientData) simplePtr);
    switch ((enum options) index) {
      case SIMPLE_CGET: {
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    result = TCL_ERROR;
	    goto done;
	}
	result = Tk_ConfigureValue(interp, simplePtr->tkwin, configSpecs,
		(char *) simplePtr, Tcl_GetString(objv[2]), 0);
	break;
      }
      case SIMPLE_CONFIGURE: {
	if (objc == 2) {
	    result = Tk_ConfigureInfo(interp, simplePtr->tkwin, configSpecs,
		    (char *) simplePtr, (char *) NULL, 0);
	} else if (objc == 3) {
	    result = Tk_ConfigureInfo(interp, simplePtr->tkwin, configSpecs,
		    (char *) simplePtr, Tcl_GetString(objv[2]), 0);
	} else {
	    for (i = 2; i < objc; i++) {
		char *arg = Tcl_GetStringFromObj(objv[i], (int *) &length);
		if (length < 2) {
		    continue;
		}
		c = arg[1];
		if ((c == 'u') && (strncmp(arg, "-use", length) == 0)) {
		    Tcl_AppendResult(interp, "can't modify ", arg,
			    " option after widget is created", (char *) NULL);
		    result = TCL_ERROR;
		    goto done;
		}
	    }
	    result = ConfigureSimple(interp, simplePtr, objc-2, objv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
	break;
      }
    }

    done:
    Tcl_Release((ClientData) simplePtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroySimple --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a simple at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the simple is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroySimple(memPtr)
    char *memPtr;		/* Info about simple widget. */
{
    register Simple *simplePtr = (Simple *) memPtr;

    Tk_FreeOptions(configSpecs, (char *) simplePtr, simplePtr->display,
	    TK_CONFIG_USER_BIT);
    if (simplePtr->exitProc != NULL) {
	/* Call the exit procedure */
	Tcl_EvalEx(simplePtr->interp, simplePtr->exitProc, -1, 0);
    }
    ckfree((char *) simplePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureSimple --
 *
 *	This procedure is called to process an objv/objc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a simple widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for simplePtr;  old resources get freed, if there
 *	were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureSimple(interp, simplePtr, objc, objv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register Simple *simplePtr;	/* Information about widget;  may or may
				 * not already have values for some fields. */
    int objc;			/* Number of valid entries in objv. */
    Tcl_Obj *CONST objv[];	/* Arguments. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
  /* char *oldMenuName; (jdk) */
    
    if (Tk_ConfigureWidget(interp, simplePtr->tkwin, configSpecs,
	    objc, (CONST84 char **) objv, (char *) simplePtr,
	    flags | TK_CONFIG_OBJS) != TCL_OK) {
	return TCL_ERROR;
    }

    if ((simplePtr->width > 0) || (simplePtr->height > 0)) {
	Tk_GeometryRequest(simplePtr->tkwin, simplePtr->width,
		simplePtr->height);
    }

    if (simplePtr->background != NULL) {
	Tk_SetWindowBackground(simplePtr->tkwin, simplePtr->background->pixel);
    }

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * SimpleEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher on
 *	structure changes to a simple.  For simples with 3D
 *	borders, this procedure is also invoked for exposures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
SimpleEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    register XEvent *eventPtr;	/* Information about event. */
{
    register Simple *simplePtr = (Simple *) clientData;

    if (eventPtr->type == DestroyNotify) {
	if (simplePtr->tkwin != NULL) {

	    /*
	     * If this window is a container, then this event could be
	     * coming from the embedded application, in which case
	     * Tk_DestroyWindow hasn't been called yet.  When Tk_DestroyWindow
	     * is called later, then another destroy event will be generated.
	     * We need to be sure we ignore the second event, since the simple
	     * could be gone by then.  To do so, delete the event handler
	     * explicitly (normally it's done implicitly by Tk_DestroyWindow).
	     */
    
	    Tk_DeleteEventHandler(simplePtr->tkwin,
		    StructureNotifyMask | FocusChangeMask,
		    SimpleEventProc, (ClientData) simplePtr);
	    simplePtr->tkwin = NULL;
            Tcl_DeleteCommandFromToken(simplePtr->interp, simplePtr->widgetCmd);
	}
	Tcl_EventuallyFree((ClientData) simplePtr, DestroySimple);
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    simplePtr->flags |= GOT_FOCUS;
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    simplePtr->flags &= ~GOT_FOCUS;
	}
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * SimpleCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
SimpleCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    Simple *simplePtr = (Simple *) clientData;
    Tk_Window tkwin = simplePtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	simplePtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

#endif /* TCL_WRAPPER */
