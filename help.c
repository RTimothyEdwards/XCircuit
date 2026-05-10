/*----------------------------------------------------------------------*/
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Spun off from xcircuit.c 10/4/98					*/
/* Functionality will not be expanded in the Xt version.  All new	*/
/* capabilities will be developed in the TCL version of help.		*/
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef XC_WIN32
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#endif

#ifdef TCL_WRAPPER
#include <tk.h>
#else
#ifndef XC_WIN32
#include "Xw/Xw.h"
#include "Xw/Form.h"
#include "Xw/WorkSpace.h"
#include "Xw/MenuBtn.h"
#endif
#endif

/*----------------------------------------------------------------------*/
/* Local includes							*/
/*----------------------------------------------------------------------*/

#include "colordefs.h"
#include "xcircuit.h"
#include "menudep.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*----------------------------------------------------------------------*/
/* Global Variable definitions						*/
/*----------------------------------------------------------------------*/

#ifdef TCL_WRAPPER
extern Tcl_Interp *xcinterp;
#endif

#ifndef TCL_WRAPPER

extern Display    *dpy;
extern GC	  hgc;
extern XCWindowData *areawin;
extern ApplicationData appdata;
extern xcWidget     top;
extern colorindex *colorlist;
extern short	  popups;
extern xcWidget	  menuwidgets[];
extern char *function_names[NUM_FUNCTIONS];

Pixmap   helppix = (Pixmap)NULL;     /* For help window */
Dimension helpwidth, helpheight, hheight;
int helptop;
short help_up;

/*-----------------------------------------*/
/* Print help list into a pixmap           */
/* Return width and height of map through  */
/* global variables helpwidth, helpheight. */
/*-----------------------------------------*/

typedef struct {
   int function;
   char *text;
} helpstruct;

void printhelppix()
{
   static char *helptitle = "Macro Key Binding Summary:";
   static helpstruct helptext[] = {
    { XCF_Finish,	"Finish"},
    { XCF_Cancel,	"Cancel"},
    { XCF_Zoom_In,	"Zoom in 3/2"},
    { XCF_Zoom_Out,	"Zoom out 3/2"},
    { XCF_Pan,		"Pan (various modes)"},
    { XCF_Double_Snap,	"Double snap-to spacing"},
    { XCF_Halve_Snap,	"Halve snap-to spacing"},
    { XCF_Next_Library,	"Go To Next Library"},
    { XCF_Library_Pop,	"Return from Library"},
    { XCF_Push,		"Push object"},
    { XCF_Pop,		"Pop object"},
    { XCF_Redraw,	"Refresh screen"},
    { XCF_Page,		"Go To Page"},
    { XCF_Write,	"Popup Output dialog"},
#ifdef HAVE_PYTHON
    { XCF_Prompt,	"Python Command entry"},
#else
    { XCF_Prompt,	"Command entry"},
#endif
    { XCF_Exit,		"Quit XCircuit"},
    { XCF_SPACER,	NULL},
    { XCF_Wire,		"Begin Polygon"},
    { XCF_Box,		"Begin Box"},
    { XCF_Arc,		"Begin Arc"},
    { XCF_Text,		"Begin Text"},
    { XCF_Spline,	"Begin Spline"},
    { XCF_Select_Save,	"Make object"},
    { XCF_Virtual,	"Make library instance"},
    { XCF_Join,		"Join elements (make path)"},
    { XCF_Unjoin,	"Un-join elements"},
    { XCF_Dot,		"Place a dot"},
    { XCF_SPACER,	NULL},
    { XCF_Delete,	"Delete"},
    { XCF_Undo,		"Undo"},
    { XCF_Redo,		"Redo"},
    { XCF_Select,	"Select"},
    { XCF_Unselect,	"Deselect"},
    { XCF_Copy,		"Copy"},
    { XCF_Edit,		"Edit"},
    { XCF_SPACER,	NULL},
    { XCF_Rotate,	"Rotate"},
    { XCF_Flip_X,	"Flip horizontally"},
    { XCF_Flip_Y,	"Flip vertically"},
    { XCF_Snap,		"Snap to grid"},
    { XCF_Attach,	"Attach to"},
    { XCF_Dashed,	"Dashed line style"},
    { XCF_Dotted,	"Dotted line style"},
    { XCF_Solid,	"Solid line style"},
    { XCF_SPACER,	NULL},
    { XCF_Anchor,	"Text Anchoring"},
    { XCF_Superscript,	"Text Superscript"},
    { XCF_Subscript,	"Text Subscript"},
    { XCF_Font,		"Toggle text font"},
    { XCF_Boldfont,	"Begin Bold text"},
    { XCF_Italicfont,	"Begin Italic text"},
    { XCF_Normalfont,	"Resume normal text"},
    { XCF_ISO_Encoding,	"Begin Latin-1 encoding"},
    { XCF_Overline,	"Begin text overline"},
    { XCF_Underline,	"Begin text underline"},
    { XCF_Parameter,	"Insert parameter"},
    { XCF_Halfspace,	"Insert half-space"},
    { XCF_Quarterspace,	"Insert quarter-space"},
    { XCF_Linebreak,	"Insert return character"},
    { XCF_Special,	"Insert special character"},
    { XCF_TabStop,	"Set tab stop"},
    { XCF_TabForward,	"Forward tab"},
    { XCF_TabBackward,	"Backward tab"},
    { XCF_Text_Home,	"Go to label beginning"},
    { XCF_Text_End,	"Go to label end"},
    { XCF_Text_Left,	"Move left one position"},
    { XCF_Text_Right,	"Move right one position"},
    { XCF_Text_Up,	"Move up one line"},
    { XCF_Text_Down,	"Move down one line"},
    { XCF_Text_Delete,	"Delete character(s)"},
    { XCF_Text_Delete_Param,	"Delete parameter from text"},
    { XCF_Text_Return,	"End text edit"},
    { XCF_Text_Split,	"Split label at cursor"},
    { XCF_SPACER,	NULL},
    { XCF_Edit_Next,	"Edit: next position"},
    { XCF_Edit_Delete,	"Edit: delete point"},
    { XCF_Edit_Insert,	"Edit: insert point"},
    { XCF_Edit_Param,	"Edit: insert parameter"},
    { XCF_SPACER,	NULL},
    { XCF_Library_Edit,	"Library: name edit"},
    { XCF_Library_Move,	"Library: move object/page"},
    { XCF_Library_Delete, "Library: object delete"},
    { XCF_Library_Hide,	"Library: hide object"},
    { XCF_Library_Duplicate, "Library: copy object"},
    { XCF_Library_Virtual, "Library: copy instance"},
    { XCF_SPACER,	NULL},
    { XCF_Pin_Label,	"Make Pin Label"},
    { XCF_Pin_Global,	"Make Global Pin"},
    { XCF_Info_Label,	"Make Info Label"},
    { XCF_Swap,		"Go to Symbol or Schematic"},
    { XCF_Connectivity,	"See net connectivity"},
    { XCF_Sim,		"Generate Sim netlist"},
    { XCF_SPICE,	"Generate SPICE netlist"},
    { XCF_SPICEflat,	"Generate flattened SPICE"},
    { XCF_PCB,		"Generate PCB netlist"},
    { XCF_ENDDATA,	NULL},
    };

    XGCValues	values;
    Window hwin = DefaultRootWindow(dpy);
    Dimension	htmp, vtmp, lineheight, mwidth;
    int i, j, t1, t2, dum, numlines;
    XCharStruct csdum;
    char *bindings, *bptr, *cptr;

    if (hwin == 0) return;

    /* Set up the GC for drawing to the help window pixmap */

    if (hgc == NULL) {
       values.foreground = colorlist[FOREGROUND].color.pixel;
       values.background = colorlist[BACKGROUND].color.pixel;
       values.graphics_exposures = False;
       values.font = appdata.helpfont->fid;
       hgc = XCreateGC(dpy, hwin, GCForeground | GCBackground | GCFont
		| GCGraphicsExposures, &values);
    }

    /* Determine the dimensions of the help text */

    mwidth = helpwidth = lineheight = numlines = 0;

    for (i = 0; helptext[i].function != XCF_ENDDATA; i++) {
       if (helptext[i].function == XCF_SPACER) {
	  numlines++;
	  continue;
       }

       htmp = XTextWidth(appdata.helpfont, helptext[i].text, strlen(helptext[i].text));
       if (htmp > mwidth) mwidth = htmp;

       XTextExtents(appdata.helpfont, helptext[i].text, strlen(helptext[i].text),
		&dum, &t1, &t2, &csdum);
       vtmp = t1 + t2 + 5;
       if (vtmp > lineheight) lineheight = vtmp;

       bindings = function_binding_to_string(0, helptext[i].function);

       /* Limit list to three key bindings per line */
       bptr = bindings;
       while (bptr != NULL) {
	  cptr = bptr;
          for (j = 0; j < 3; j++) {
	     cptr = strchr(cptr + 1, ',');
	     if (cptr == NULL) break;
          }
	  if (cptr != NULL) *(++cptr) = '\0';

          htmp = XTextWidth(appdata.helpfont, bptr, strlen(bptr));
          if (htmp > helpwidth) helpwidth = htmp;

	  XTextExtents(appdata.helpfont, bptr, strlen(bptr),
		&dum, &t1, &t2, &csdum);
	  vtmp = t1 + t2 + 5;
	  if (vtmp > lineheight) lineheight = vtmp;
	  numlines++;

	  if (cptr == NULL) break;
	  bptr = cptr + 1;
       }
       free(bindings);
    }
    XTextExtents(appdata.helpfont, helptitle, strlen(helptitle), &dum, &t1,
	&t2, &csdum);
    t1 += t2;
    helpwidth += mwidth + 15;

    helpheight = lineheight * numlines + 15 + t1;  /* full height of help text */
    if (helppix != (Pixmap)NULL) {
       Wprintf("Error:  Help window not cancelled?");
       return;
    }
    helppix = XCreatePixmap(dpy, hwin, helpwidth, helpheight,
	   	  DefaultDepthOfScreen(DefaultScreenOfDisplay(dpy)));

    XSetForeground(dpy, hgc, colorlist[FOREGROUND].color.pixel);
    XFillRectangle(dpy, helppix, hgc, 0, 0, helpwidth, helpheight);

    XSetForeground(dpy, hgc, colorlist[BACKGROUND].color.pixel);
    XDrawString(dpy, helppix, hgc, (helpwidth - XTextWidth(appdata.helpfont,
	helptitle, strlen(helptitle))) >> 1, t1 + 2, helptitle, strlen(helptitle));
    vtmp = lineheight + 15;
    for (i = 0; helptext[i].function != XCF_ENDDATA; i++) {
       if (helptext[i].function == XCF_SPACER) {
	  vtmp += lineheight;
	  continue;
       }
       XDrawString(dpy, helppix, hgc, 7, vtmp, helptext[i].text,
		strlen(helptext[i].text));
       bindings = function_binding_to_string(0, helptext[i].function);
       bptr = bindings;
       while (bptr != NULL) {
	  cptr = bptr;
          for (j = 0; j < 3; j++) {
	     cptr = strchr(cptr + 1, ',');
	     if (cptr == NULL) break;
          }
	  if (cptr != NULL) *(++cptr) = '\0';
          XDrawString(dpy, helppix, hgc, 7 + mwidth, vtmp, bptr, strlen(bptr));
          vtmp += lineheight;
	  if (cptr == NULL) break;
	  bptr = cptr + 1;
       }
       free(bindings);
    }
    XSetForeground(dpy, hgc, colorlist[AUXCOLOR].color.pixel);
    XDrawLine(dpy, helppix, hgc, 0, t1 + 7, helpwidth, t1 + 7);
}

/*----------------------------------------------*/
/* Create the help popup window	(Xt version)	*/
/*----------------------------------------------*/

#ifndef XC_WIN32

void starthelp(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   Arg		wargs[11];
   xcWidget	popup, cancelbutton, hspace, help2, hsb;
   short 	n = 0;
   popupstruct  *okaystruct;
   buttonsave   *savebutton;
   Dimension    areawidth, bwidth, pheight;
   Position	xpos, ypos;
   u_int	xmax, ymax;
   char		*scale;
   int		sbarsize;

   if (help_up) return;  /* no multiple help windows */

#ifdef TCL_WRAPPER
   scale = Tcl_GetVar2(xcinterp, "XCOps", "scale", TCL_GLOBAL_ONLY);
   sbarsize = SBARSIZE * atoi(scale);
#else
   sbarsize = SBARSIZE;
#endif

   /* for positioning the help window outside of the xcircuit    */
   /* window, get information about the display width and height */
   /* and the xcircuit window.					  */

   /* The "- 50" leaves space for the Windows-95-type title bar that */
   /* runs across the bottom of the screen in some window managers   */
   /* (specifically, fvwm95 which is the default for RedHat Linux)   */

   xmax = DisplayWidth(dpy, DefaultScreen(dpy)) - 100;
   ymax = DisplayHeight(dpy, DefaultScreen(dpy)) - 50;

   XtnSetArg(XtNwidth, &areawidth);
   XtGetValues(areawin->area, wargs, n); n = 0;
   XtTranslateCoords(areawin->area, (Position) (areawidth + 10), -50,
	&xpos, &ypos);

   savebutton = getgeneric(button, starthelp, NULL);

   /* Generate the pixmap and write the help text to it */

   if (helppix == (Pixmap)NULL) printhelppix();

   /* Use the pixmap size to size the help window */

   if (xpos + helpwidth + sbarsize > xmax)  xpos = xmax - helpwidth - sbarsize - 4;
   if (ypos + helpheight > ymax) ypos = ymax - helpheight - 4;
   if (ypos < 4) ypos = 4;

   XtnSetArg(XtNx, xpos);
   XtnSetArg(XtNy, ypos);
   popup = XtCreatePopupShell("help", transientShellWidgetClass,
	button, wargs, n); n = 0;
   popups++;
   help_up = True;
   helptop = 0;

   XtnSetArg(XtNyResizable, True);
   XtnSetArg(XtNxResizable, False);
   help2 = XtCreateManagedWidget("help2", XwformWidgetClass,
	popup, wargs, n); n = 0;

   XtnSetArg(XtNfont, appdata.xcfont);
   cancelbutton = XtCreateManagedWidget("Dismiss", XwmenuButtonWidgetClass,
	help2, wargs, n); n = 0;

   XtnSetArg(XtNwidth, helpwidth);
   XtnSetArg(XtNheight, areawin->height);
   XtnSetArg(XtNyRefWidget, cancelbutton);
   XtnSetArg(XtNyAddHeight, True);
   XtnSetArg(XtNyAttachBottom, True);
   XtnSetArg(XtNyResizable, True);
   XtnSetArg(XtNborderWidth, 0);
   XtnSetArg(XtNxAttachRight, False);
   hspace = XtCreateManagedWidget("HSpace", XwworkSpaceWidgetClass,
	help2, wargs, n); n = 0;

   /* Create scrollbar */
   XtnSetArg(XtNwidth, sbarsize);
   XtnSetArg(XtNxRefWidget, hspace);
   XtnSetArg(XtNxAddWidth, True);
   XtnSetArg(XtNyRefWidget, cancelbutton);
   XtnSetArg(XtNyAddHeight, True);
   XtnSetArg(XtNyResizable, True);
   XtnSetArg(XtNyAttachBottom, True);
   XtnSetArg(XtNborderWidth, 1);
   hsb = XtCreateManagedWidget("HSB", XwworkSpaceWidgetClass,
		help2, wargs, n); n = 0;

   okaystruct = (popupstruct *) malloc(sizeof(popupstruct));
   okaystruct->buttonptr = savebutton;
   okaystruct->popup = popup;
   okaystruct->filter = NULL;

   XtPopup(popup, XtGrabNone);

   /* reposition the "Dismiss" button to center */

   XtSetArg(wargs[0], XtNwidth, &bwidth);
   XtGetValues(cancelbutton, wargs, 1);
   XtnSetArg(XtNx, ((helpwidth - bwidth) >> 1));
   XtSetValues(cancelbutton, wargs, n); n = 0;

   XtSetArg(wargs[0], XtNheight, &pheight);
   XtGetValues(help2, wargs, 1);

   if (pheight > (ymax - 8)) {
      XtnSetArg(XtNheight, ymax - 8);
      XtSetValues(help2, wargs, n); n = 0;
   }

   XtAddEventHandler(hsb, ButtonMotionMask | ButtonPressMask, False,
		(XtEventHandler)simplescroll, hspace);

   /* Expose and End callbacks */

   XtAddCallback(cancelbutton, XtNselect, (XtCallbackProc)destroypopup, okaystruct); 
   XtAddCallback(hspace, XtNexpose, (XtCallbackProc)exposehelp, NULL);
   XtAddCallback(hsb, XtNexpose, (XtCallbackProc)showhsb, NULL);
}

/*----------------------------------------------*/
/* Very simple scroll mechanism	 (grab-and-pan)	*/
/*----------------------------------------------*/

void simplescroll(xcWidget hsb, xcWidget hspace, XMotionEvent *event)
{
   Dimension oldtop = helptop;

   helptop = (((int)(event->y) * helpheight) / hheight) - (hheight / 2);

   if (helptop < 0) helptop = 0;
   else if (helptop > helpheight - hheight) helptop = helpheight - hheight;

   if (helptop != oldtop) {
      showhsb(hsb, NULL, NULL);
      printhelp(hspace);
   }
}
 
/*----------------------------------------------*/
/* Expose callback for the help scrollbar	*/
/*----------------------------------------------*/

void showhsb(xcWidget hsb, caddr_t clientdata, caddr_t calldata)
{
   Window hwin = xcWindow(hsb);
   Dimension sheight;
   int pstart, pheight;
   short n = 0;
   int sbarsize;

#ifdef TCL_WRAPPER
   char *scale;
   scale = Tcl_GetVar2(xcinterp, "XCOps", "scale", TCL_GLOBAL_ONLY);
   sbarsize = SBARSIZE * atoi(scale);
#else
   sbarsize = SBARSIZE;
#endif

   if (helppix == (Pixmap)NULL) printhelppix();
   if (helpheight == 0) helpheight = 1;

   pstart = (helptop * hheight) / helpheight;
   pheight = (hheight * hheight) / helpheight;

   if (pheight < 3) pheight = 3;

   XClearArea(dpy, hwin, 0, 0, sbarsize, pstart, False);
   XClearArea(dpy, hwin, 0, pstart + pheight, sbarsize,
		hheight - (pstart + pheight), False);

   XSetForeground(dpy, hgc, colorlist[BARCOLOR].color.pixel);
   XFillRectangle(dpy, hwin, hgc, 0, pstart, sbarsize, pheight);
}

/*----------------------------------------------*/
/* Expose callback for the help window		*/
/*----------------------------------------------*/

void exposehelp(xcWidget hspace, caddr_t clientdata, caddr_t calldata)
{
   Arg wargs[1];

   XtSetArg(wargs[0], XtNheight, &hheight);
   XtGetValues(hspace, wargs, 1);

   if (helppix == (Pixmap)NULL) printhelppix();
   if (hheight < 1) hheight = 1;

   printhelp(hspace);
}

/*----------------------------------------------*/
/* Expose callback for the help window		*/
/*----------------------------------------------*/

void printhelp(xcWidget hspace)
{
   Window hwin = xcWindow(hspace);
   XEvent discard;

   /* Draw the pixmap to the window */

   XCopyArea(dpy, helppix, hwin, hgc, 0, helptop - 5, helpwidth, helpheight,
	0, 0);

   /* flush out multiple expose events */

   while (XCheckWindowEvent(dpy, hwin, ExposureMask, &discard) == True);
}

#endif

/*----------------------------------------------------------------------*/
/* The TCL version assumes the existence of command "helpwindow".	*/
/*----------------------------------------------------------------------*/

#else

void starthelp(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   Tcl_Eval(xcinterp, "catch xcircuit::helpwindow");
}

#endif /* !TCL_WRAPPER */
