/*----------------------------------------------------------------------*/
/* xcircuit.c --- An X-windows program for drawing circuit diagrams	*/
/* Copyright (c) 2002  R. Timothy Edwards				*/
/*									*/
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/*									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/*									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to:				*/
/*	Free Software Foundation, Inc.					*/
/*	59 Temple Place, Suite 330					*/
/*	Boston, MA  02111-1307  USA					*/
/*									*/
/* See file ./README for contact information				*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*      Uses the Xw widget set (see directory Xw)			*/
/*      Xcircuit written by Tim Edwards beginning 8/13/93 		*/
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <ctype.h>
#ifndef XC_WIN32
#include <unistd.h>   /* for unlink() */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#endif

#ifdef TCL_WRAPPER
#include <tk.h>
#else
#ifndef XC_WIN32
#include "Xw/Xw.h"
#include "Xw/Form.h"
#include "Xw/WorkSpace.h"
#include "Xw/PButton.h"
#include "Xw/SText.h"
#include "Xw/Cascade.h"
#include "Xw/PopupMgr.h"
#include "Xw/MenuBtn.h"
#include "Xw/BBoard.h"
#include "Xw/TextEdit.h"
#include "Xw/Toggle.h"
#endif
#endif

#if defined(XC_WIN32) && defined(TCL_WRAPPER)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#endif

#ifndef P_tmpdir
#define P_tmpdir TMPDIR
#endif

/*----------------------------------------------------------------------*/
/* Local includes							*/
/*----------------------------------------------------------------------*/

#include "xcircuit.h"
#include "cursors.h"
#include "colordefs.h"
#include "menudep.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*----------------------------------------------------------------------*/
/* Global Variable definitions						*/
/*----------------------------------------------------------------------*/

char	 _STR2[250];  /* Specifically for text returned from the popup prompt */
char	 _STR[150];   /* Generic multipurpose string */
int	 pressmode;   /* Whether we are in a press & hold state */
Display  *dpy = NULL;        /* Works well to make this globally accessible */
Colormap cmap;
Pixmap   STIPPLE[STIPPLES];  /* Polygon fill-style stipple patterns */
Cursor	appcursors[NUM_CURSORS];
ApplicationData appdata;
XCWindowData *areawin;
Globaldata xobjs;
int number_colors;
colorindex *colorlist;
short menusize;
xcIntervalId printtime_id;
short beeper;
short fontcount;
fontinfo *fonts;
short	 popups;      /* total number of popup widgets on the screen */

/*----------------------------------------------------------------------*/
/* Externally defined variables						*/
/*----------------------------------------------------------------------*/

extern aliasptr aliastop;
extern char version[];

#ifdef TCL_WRAPPER
extern Tcl_Interp *xcinterp;
#else
extern XtAppContext app;
#endif

#if !defined(HAVE_CAIRO)
extern Pixmap dbuf;
#endif

/* Bad hack for problems with the DECstation. . . don't know why */
#ifdef UniqueContextProblem
#undef XUniqueContext
XContext XUniqueContext()
{
   return XrmUniqueQuark();
}
#endif
/* End of bad hack. . . */

/*----------------------------------------------------------------------*/
/* Update the list of colors in the colormap				*/
/*----------------------------------------------------------------------*/

void addtocolorlist(xcWidget button, int cvalue)
{
   number_colors++;
   colorlist = (colorindex *)realloc(colorlist, number_colors *
		sizeof(colorindex));
   colorlist[number_colors - 1].cbutton = button;
   colorlist[number_colors - 1].color.pixel = cvalue; 

   /* Get and store the RGB values of the new color */
   
   if (areawin->area == NULL) {
      colorlist[number_colors - 1].color.red = 256 * (cvalue & 0xff);
      colorlist[number_colors - 1].color.green = 256 * ((cvalue >> 8) & 0xff);
      colorlist[number_colors - 1].color.blue = 256 * ((cvalue >> 16) & 0xff);
   }
   else
      XQueryColors(dpy, cmap, &(colorlist[number_colors - 1].color), 1);
}

/*----------------------------------------------------------------------*/
/* Add a new color button to the color menu				*/
/* 	called if new color button needs to be made 			*/
/*----------------------------------------------------------------------*/

#ifdef TCL_WRAPPER

int addnewcolorentry(int ccolor)
{
  xcWidget newbutton = (xcWidget)NULL;
  int i;

   /* check to see if entry is already in the color list */

   for (i = NUMBER_OF_COLORS; i < number_colors; i++)
      if (colorlist[i].color.pixel == ccolor) break;

   /* make new entry in the menu */

   if (i == number_colors) {
      addtocolorlist(newbutton, ccolor);
      sprintf(_STR2,
	"xcircuit::newcolorbutton %d %d %d %d",
	    colorlist[i].color.red, colorlist[i].color.green,
	    colorlist[i].color.blue, i);
      Tcl_Eval(xcinterp, _STR2);
   }
   return i;
}

#endif /* TCL_WRAPPER */

/*---------------*/
/* Quit xcircuit */
/*---------------*/

void quit(xcWidget w, caddr_t nulldata)
{
   int i;
   Matrixptr curmatrix, dmatrix;

   /* free up CTM Matrix Stack */
   if (areawin != NULL) {
      curmatrix = areawin->MatStack;
      while (curmatrix != NULL) {
         dmatrix = curmatrix->nextmatrix;
         free(curmatrix);
         curmatrix = dmatrix;
      }
      areawin->MatStack = NULL;
   }

   /* free the colormap if a new one has been installed */

   if (dpy != NULL)
      if (cmap != DefaultColormap(dpy, DefaultScreen(dpy)))
         XFreeColormap(dpy, cmap);

   /* exit ghostscript if background rendering was enabled */

   exit_gs();

#ifdef TCL_WRAPPER
   /* exit ngspice if the simulator is still running */

   exit_spice();
#endif

   /* remove any temporary files created for background rendering */

   for (i = 0; i < xobjs.pages; i++) {
      if (xobjs.pagelist[i]->pageinst != NULL)
         if (xobjs.pagelist[i]->background.name != (char *)NULL)
	    if (*(xobjs.pagelist[i]->background.name) == '@')
	       unlink(xobjs.pagelist[i]->background.name + 1);
   }

   /* remove the temporary file and free the filename memory	*/
   /* if w = NULL, quit() was reached from Ctrl-C.  Don't 	*/
   /* remove the temporary file;  instead, notify user of the 	*/
   /* filename.							*/

   if (xobjs.tempfile != NULL) {
      if (w != NULL) {
#ifndef XC_WIN32
         if (unlink(xobjs.tempfile) < 0)
	    Fprintf(stderr, "Error %d unlinking file \"%s\"\n", errno, xobjs.tempfile);
#else
         if (DeleteFile(xobjs.tempfile) != TRUE)
	    Fprintf(stderr, "Error %d unlinking file \"%s\"\n",
			GetLastError(), xobjs.tempfile);
#endif
      }
      else
	 Fprintf(stderr, "Ctrl-C exit:  reload workspace from \"%s\"\n",
		xobjs.tempfile);
      free(xobjs.tempfile);
      xobjs.tempfile = NULL;
   }

#if defined(HAVE_PYTHON)
   /* exit by exiting the Python interpreter, if enabled */
   exit_interpreter();
#elif defined(TCL_WRAPPER)
   /* Let Tcl/Tk handle exit */
   return;
#elif defined(XC_WIN32)
   PostQuitMessage(0);
#else
   exit(0);
#endif

}

/*--------------------------------------------------------------*/
/* Check for changes in an object and all of its descendents	*/
/*--------------------------------------------------------------*/

u_short getchanges(objectptr thisobj)
{
   genericptr *pgen;
   objinstptr pinst;
   u_short changes = thisobj->changes;

   for (pgen = thisobj->plist; pgen < thisobj->plist + thisobj->parts; pgen++) {
      if (IS_OBJINST(*pgen)) {
	 pinst = TOOBJINST(pgen);
	 changes += getchanges(pinst->thisobject);
      }
   }
   return changes;
}

/*--------------------------------------------------------------*/
/* Check to see if any objects in xcircuit have been modified	*/
/* without saving.						*/
/*--------------------------------------------------------------*/

u_short countchanges(char **promptstr)
{
  int slen = 1, i; /* , j; (jdk) */
   u_short locchanges, changes = 0, words = 1;
   objectptr thisobj;
   char *fname;
   TechPtr ns;

   if (promptstr != NULL) slen += strlen(*promptstr);

   for (i = 0; i < xobjs.pages; i++) {
      if (xobjs.pagelist[i]->pageinst != NULL) {
	 thisobj = xobjs.pagelist[i]->pageinst->thisobject;
	 locchanges = getchanges(thisobj);
         if (locchanges > 0) {
	    if (promptstr != NULL) {
	       slen += strlen(thisobj->name) + 2;
	       *promptstr = (char *)realloc(*promptstr, slen);
	       if ((words % 8) == 0) strcat(*promptstr, ",\n");
	       else if (changes > 0) strcat(*promptstr, ", ");
	       strcat(*promptstr, thisobj->name);
	       words++;
	    }
	    changes += locchanges;
	 }
      }
   }

   /* Check all library objects for unsaved changes */

   for (ns = xobjs.technologies; ns != NULL; ns = ns->next) {
      tech_set_changes(ns);
      if ((ns->flags & TECH_CHANGED) != 0) {
	 changes++;
	 if (promptstr != NULL) {
	    fname = ns->filename;
	    if (fname != NULL) {
	       slen += strlen(fname) + 2;
	       *promptstr = (char *)realloc(*promptstr, slen);
	       if ((words % 8) == 0) strcat(*promptstr, ",\n");
	       else if (changes > 0) strcat(*promptstr, ", ");
	       strcat(*promptstr, fname);
	       words++;
	    }
	 }
      }
   }
   return changes;
}

/*----------------------------------------------*/
/* Check for conditions to approve program exit */
/* Return 0 if prompt "Okay" button ends the	*/
/* program, 1 if the program should exit	*/
/* immediately.					*/
/*----------------------------------------------*/

#ifdef TCL_WRAPPER

int quitcheck(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   char *promptstr;
   Boolean doprompt = False;

   /* enable default interrupt signal handler during this time, so that */
   /* a double Control-C will ALWAYS exit.				*/

   signal(SIGINT, SIG_DFL);

   promptstr = (char *)malloc(60);
   strcpy(promptstr, ".query.title.field configure -text \"Unsaved changes in: ");

   /* Check all page objects for unsaved changes */

   doprompt = (countchanges(&promptstr) > 0) ? True : False;

   /* If any changes have not been saved, generate a prompt */

   if (doprompt) {
      promptstr = (char *)realloc(promptstr, strlen(promptstr) + 15);
      strcat(promptstr, "\nQuit anyway?");

      strcat(promptstr, "\"");
      Tcl_Eval(xcinterp, promptstr);
      Tcl_Eval(xcinterp, ".query.bbar.okay configure -command {quitnocheck}");
      Tcl_Eval(xcinterp, "wm deiconify .query");
      Tcl_Eval(xcinterp, "raise .query");
      free(promptstr);
      return 0;
   }
   else {
      free(promptstr);
      quit(w, NULL);				// preparation for quit
      Tcl_Eval(xcinterp, "quitnocheck");	// actual quit
      return 1;					// not reached
   }
}

#endif

/*--------------------------------------*/
/* A gentle Ctrl-C shutdown		*/
/*--------------------------------------*/

void dointr(int signum)
{
   quitcheck(NULL, NULL, NULL);
}

/*--------------*/
/* Null routine */
/*--------------*/

void DoNothing(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   /* Nothing here! */
}

/*-----------------------------------------------------------------------*/
/* Write page scale (overall scale, and X and Y dimensions) into strings */
/*-----------------------------------------------------------------------*/

void writescalevalues(char *scdest, char *xdest, char *ydest)
{
   float oscale, psscale;
   int width, height;
   Pagedata    *curpage;

   curpage = xobjs.pagelist[areawin->page];
   oscale = curpage->outscale;
   psscale = getpsscale(oscale, areawin->page);

   width = toplevelwidth(curpage->pageinst, NULL);
   height = toplevelheight(curpage->pageinst, NULL);

   sprintf(scdest, "%6.5f", oscale);
   if (curpage->coordstyle == CM) {
      sprintf(xdest, "%6.5f", (width * psscale) / IN_CM_CONVERT);
      sprintf(ydest, "%6.5f", (height * psscale) / IN_CM_CONVERT);
   }
   else {
      sprintf(xdest, "%6.5f", (width * psscale) / 72.0);
      sprintf(ydest, "%6.5f", (height * psscale) / 72.0);
   }
}

/*-------------------------------------------------------------------------*/
/* Destroy an interactive text-editing popup box 			   */
/*-------------------------------------------------------------------------*/

#ifdef TCL_WRAPPER

/* Just pop it down. . . */

void destroypopup(xcWidget button, popupstruct *callstruct, caddr_t calldata)
{
   Tk_UnmapWindow(callstruct->popup);
   popups--;

   /* free the allocated structure space */

   free(callstruct->buttonptr);
   if (callstruct->filter != NULL) free(callstruct->filter);
   free(callstruct);

   /* in the case of "quitcheck", we want to make sure that the signal	*/
   /* handler is reset to default behavior if the quit command is	*/
   /* canceled from inside the popup prompt window.			*/

   signal(SIGINT, dointr);
}

/*-------------------------------------------------------------------------*/
/* Create a popup window with "OK" and "Cancel" buttons,		   */
/* and text and label fields.						   */
/*-------------------------------------------------------------------------*/

void popupprompt(xcWidget button, char *request, char *current, void (*function)(),
	buttonsave *datastruct, const char *filter)
{
    Tk_Window popup;

    popup = Tk_NameToWindow(xcinterp, ".dialog", Tk_MainWindow(xcinterp));
    Tk_MapWindow(popup);
}

/*-------------------------------------------------------------------------*/
/* Create a popup window for property changes				   */
/*-------------------------------------------------------------------------*/

void outputpopup(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
    Tcl_Eval(xcinterp, "wm deiconify .output");
}

/*-------------------------------------------------*/
/* Print a string to the message widget. 	   */ 
/* Note: Widget message must be a global variable. */
/* For formatted strings, format first into _STR   */
/*-------------------------------------------------*/

/* XXX it looks like this routine is an orphan */
void clrmessage(caddr_t clientdata)
{
   char buf1[50], buf2[50];

   /* Don't write over the report of the edit string contents,	*/
   /* if we're in one of the label edit modes 			*/

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE)
      charreport(TOLABEL(EDITPART));
   else {
      measurestr(xobjs.pagelist[areawin->page]->gridspace, buf1);
      measurestr(xobjs.pagelist[areawin->page]->snapspace, buf2);
      Wprintf("Grid %.50s : Snap %.50s", buf1, buf2);
   }
}

#endif /* TCL_WRAPPER */

/*------------------------------------------------------------------------------*/
/* diagnostic tool for translating the event mode into a string and printing	*/
/* to stderr (for debugging only).						*/
/*------------------------------------------------------------------------------*/

void printeventmode() {
   Fprintf(stderr, "eventmode is \'");
   switch(eventmode) {
      case NORMAL_MODE:
	 Fprintf(stderr, "NORMAL");
	 break;
      case MOVE_MODE:
	 Fprintf(stderr, "MOVE");
	 break;
      case COPY_MODE:
	 Fprintf(stderr, "COPY");
	 break;
      case SELAREA_MODE:
	 Fprintf(stderr, "SELAREA");
	 break;
      case CATALOG_MODE:
	 Fprintf(stderr, "CATALOG");
	 break;
      case CATTEXT_MODE:
	 Fprintf(stderr, "CATTEXT");
	 break;
      case CATMOVE_MODE:
	 Fprintf(stderr, "CATMOVE");
	 break;
      case FONTCAT_MODE:
	 Fprintf(stderr, "FONTCAT");
	 break;
      case EFONTCAT_MODE:
	 Fprintf(stderr, "EFONTCAT");
	 break;
      case TEXT_MODE:
	 Fprintf(stderr, "TEXT");
	 break;
      case ETEXT_MODE:
	 Fprintf(stderr, "ETEXT");
	 break;
      case WIRE_MODE:
	 Fprintf(stderr, "WIRE");
	 break;
      case BOX_MODE:
	 Fprintf(stderr, "BOX");
	 break;
      case EPOLY_MODE:
	 Fprintf(stderr, "EPOLY");
	 break;
      case ARC_MODE:
	 Fprintf(stderr, "ARC");
	 break;
      case EARC_MODE:
	 Fprintf(stderr, "EARC");
	 break;
      case SPLINE_MODE:
	 Fprintf(stderr, "SPLINE");
	 break;
      case ESPLINE_MODE:
	 Fprintf(stderr, "ESPLINE");
	 break;
      case EPATH_MODE:
	 Fprintf(stderr, "EPATH");
	 break;
      case EINST_MODE:
	 Fprintf(stderr, "EINST");
	 break;
      case ASSOC_MODE:
	 Fprintf(stderr, "ASSOC");
	 break;
      case RESCALE_MODE:
	 Fprintf(stderr, "RESCALE");
	 break;
      case PAN_MODE:
	 Fprintf(stderr, "PAN");
	 break;
      default:
	 Fprintf(stderr, "(unknown)");
	 break;
   }
   Fprintf(stderr, "_MODE\'\n");
}

/*------------------------------------------------------------------------------*/

#ifdef TCL_WRAPPER

/*------------------------------------------------------------------------------*/
/* "docommand" de-iconifies the TCL console.					*/
/*------------------------------------------------------------------------------*/

void docommand()
{
   Tcl_Eval(xcinterp, "catch xcircuit::raiseconsole");
}

/*------------------------------------------------------------------------------*/
/* When all else fails, install your own colormap			 	*/
/*------------------------------------------------------------------------------*/

int installowncmap()
{
   Colormap newcmap;

   Fprintf(stdout, "Installing my own colormap\n");

   /* allocate a new colormap */

   newcmap = XCopyColormapAndFree(dpy, cmap);
   if (newcmap == (Colormap)NULL) return (-1);
   cmap = newcmap;
   return(1);
}

#endif /* TCL_WRAPPER */

/*------------------------------------------------------------------------------*/
/* Find the nearest color in the colormap to cvexact and return its pixel value */
/*------------------------------------------------------------------------------*/

#if defined(TCL_WRAPPER) || !defined(XC_WIN32)

int findnearcolor(XColor *cvexact)
{
   /* find the nearest-matching color in the colormap */
 
   int i, ncolors = DisplayCells(dpy, DefaultScreen(dpy));
   XColor *cmcolors;
   long rdist, bdist, gdist;
   u_long dist, mindist;
   int minidx;

   cmcolors = (XColor *)malloc(ncolors * sizeof(XColor));

   for (i = 0; i < ncolors; i++) {
      cmcolors[i].pixel = i;
      cmcolors[i].flags = DoRed | DoGreen | DoBlue;
   }
   XQueryColors(dpy, cmap, cmcolors, ncolors); 

   mindist = ULONG_MAX;
   for (i = 0; i < ncolors; i++) {
      rdist = (cmcolors[i].red - cvexact->red);
      bdist = (cmcolors[i].blue - cvexact->blue);
      gdist = (cmcolors[i].green - cvexact->green);
      dist = rdist * rdist + bdist * bdist + gdist * gdist;
      if (dist < mindist) {
	 mindist = dist;
	 minidx = i;
      }
   }
   free(cmcolors);

   /* if we can't get the right color or something reasonably close, */
   /* then allocate our own colormap.  If we've allocated so many    */
   /* colors that the new colormap is full, then we give up and use  */
   /* whatever color was closest, no matter how far away it was.     */

   /* findnearcolor already used 512 per component, so to match that, */
   /* 3 * (512 * 512) = 786432	~= 750000			      */

   if (dist > 750000) {
      if (installowncmap() > 0) {
         if (XAllocColor(dpy, cmap, cvexact) != 0)
	    minidx = cvexact->pixel;
      }
   }

   return minidx;
}

#endif

/*----------------------------------------------------------------------*/
/* Return the pixel value of a database color in the colorlist vector	*/
/* Since this is generally called by defined name, e.g.,		*/
/* xc_getlayoutcolor(RATSNESTCOLOR), it does not check for invalid	*/
/* values of cidx.							*/
/*----------------------------------------------------------------------*/

int xc_getlayoutcolor(int cidx)
{
   return colorlist[cidx].color.pixel;
}

/*----------------------------------------------------------------------*/
/* Obtain red, green and blue values from a color index                 */
/* the colour values range from 0 to 65535                              */
/*----------------------------------------------------------------------*/

void xc_get_color_rgb(unsigned long cidx, unsigned short *red, 
      unsigned short *green, unsigned short *blue)
{
   XColor loccolor = {
         .pixel = cidx,
         .flags = DoRed | DoGreen | DoBlue
   };
   if (areawin->area == NULL) {
      loccolor.red = cidx & 0xff;
      loccolor.green = (cidx >> 8) & 0xff;
      loccolor.blue = (cidx >> 16) & 0xff;
   }
   else
      XQueryColors(dpy, cmap, &loccolor, 1);
   *red = loccolor.red;
   *green = loccolor.green;
   *blue = loccolor.blue;
}


/*-------------------------------------------------------------------------*/
/* Hack on the resource database Color allocation			   */
/*									   */
/* This overrides the built-in routine.  The difference is that if a	   */
/* color cell cannot be allocated (colormap is full), then the color	   */
/* map is searched for the closest RGB value to the named color.	   */
/*									   */
/* This code depends on Display *dpy being set:  Do not install the	   */
/* converter until after XtInitialize(), when using the Xt (not Tcl) GUI.  */
/*-------------------------------------------------------------------------*/

#if defined(TCL_WRAPPER) || !defined(XC_WIN32)

caddr_t CvtStringToPixel(XrmValuePtr args, int *nargs, XrmValuePtr fromVal,
	XrmValuePtr toVal)
{
   static XColor cvcolor;
   XColor cvexact;

   if (dpy == NULL) return NULL;

   if (*nargs != 0)
      Fprintf(stderr, "String to Pixel conversion takes no arguments");

   /* Color defaults to black if name is not found */

   if (XAllocNamedColor(dpy, cmap, (char *)fromVal->addr, &cvcolor, &cvexact)
	 == 0) {
      if (XLookupColor(dpy, cmap, (char *)fromVal->addr, &cvexact, &cvcolor) == 0)
	 cvcolor.pixel = BlackPixel(dpy, DefaultScreen(dpy));
      else
	 cvcolor.pixel = findnearcolor(&cvexact);
   }

   toVal->size = sizeof(u_long);
   toVal->addr = (caddr_t) &(cvcolor.pixel);
   return NULL;
}

#endif

/*-------------------------------------------------------------------------*/
/* Allocate new color using the CvtStringToPixel routine		   */
/*-------------------------------------------------------------------------*/

#if defined(XC_WIN32) && !defined(TCL_WRAPPER)

int xc_alloccolor(char *name)
{
   return WinNamedColor(name);
}

#else

int xc_alloccolor(char *name)
{
   XrmValue fromC, toC;
   int zval = 0, pixval;

   fromC.size = strlen(name);
   fromC.addr = name;

   if (areawin->area) {
      CvtStringToPixel(NULL, &zval, &fromC, &toC);
      pixval = (int)(*((u_long *)toC.addr));
   }
   else {
      int r, g, b;

      /* Graphics-free batch mode.  Handle basic colors plus RGB */
      if (name[0] == '#' && (strlen(name) == 7)) {
         sscanf(&name[1], "%2x", &r);
         sscanf(&name[3], "%2x", &g);
         sscanf(&name[5], "%2x", &b);
         pixval = r + g * 256 + b * 65536;
      }
      else if (name[0] == '#' && (strlen(name) == 13)) {
         sscanf(&name[1], "%4x", &r);
         sscanf(&name[5], "%4x", &g);
         sscanf(&name[9], "%4x", &b);
         pixval = (r >> 8) + (g >> 8) * 256 + (b >> 8) * 65536;
      }
      else if (sscanf(name, "%d", &pixval) != 1) {
	 if (!strcasecmp(name, "red")) {
	    r = 255;
	    g = 0;
	    b = 0;
	 }
	 else if (!strcasecmp(name, "green")) {
	    r = 0;
	    g = 255;
	    b = 0;
	 }
	 else if (!strcasecmp(name, "blue")) {
	    r = 0;
	    g = 0;
	    b = 255;
	 }
	 else if (!strcasecmp(name, "white")) {
	    r = 255;
	    g = 255;
	    b = 255;
	 }
	 else if (!strcasecmp(name, "gray")) {
	    r = 128;
	    g = 128;
	    b = 128;
	 }
	 else if (!strcasecmp(name, "yellow")) {
	    r = 255;
	    g = 255;
	    b = 0;
	 }
	 else if (!strcasecmp(name, "gray40")) {
	    r = 102;
	    g = 102;
	    b = 102;
	 }
	 else if (!strcasecmp(name, "gray60")) {
	    r = 153;
	    g = 153;
	    b = 153;
	 }
	 else if (!strcasecmp(name, "gray80")) {
	    r = 204;
	    g = 204;
	    b = 204;
	 }
	 else if (!strcasecmp(name, "gray90")) {
	    r = 229;
	    g = 229;
	    b = 229;
	 }
	 else if (!strcasecmp(name, "green2")) {
	    r = 0;
	    g = 238;
	    b = 0;
	 }
	 else if (!strcasecmp(name, "purple")) {
	    r = 160;
	    g = 32;
	    b = 240;
	 }
	 else if (!strcasecmp(name, "steelblue2")) {
	    r = 92;
	    g = 172;
	    b = 238;
	 }
	 else if (!strcasecmp(name, "red3")) {
	    r = 205;
	    g = 0;
	    b = 0;
	 }
	 else if (!strcasecmp(name, "tan")) {
	    r = 210;
	    g = 180;
	    b = 140;
	 }
	 else if (!strcasecmp(name, "brown")) {
	    r = 165;
	    g = 42;
	    b = 42;
	 }
	 else if (!strcasecmp(name, "pink")) {
	    r = 255;
	    g = 192;
	    b = 203;
	 }
         else {
	    // Default to black
	    r = 0;
	    g = 0;
	    b = 0;
	 }
         pixval = r + g * 256 + b * 65536;
      }
   }

   return pixval;
}

#endif

/*----------------------------------------------------------------------*/
/* Check if color within RGB roundoff error exists in xcircuit's color	*/
/* table.  Assume 24-bit color, in which resolution can be no less than	*/
/* 256 for each color component.  Visual acuity is a bit less than 24-	*/
/* bit color, so assume difference should be no less than 512 per	*/
/* component for colors to be considered "different".  Psychologically,	*/
/* we should really find the just-noticable-difference for each color	*/
/* component separately!  But that's too complicated for this simple	*/
/* routine.								*/
/*									*/
/* Return the table entry of the color, if it is in xcircuit's color	*/
/* table, or ERRORCOLOR if not.  If it is in the color table, then	*/
/* return the actual pixel value from the table in the "pixval" pointer	*/
/*----------------------------------------------------------------------*/

int rgb_querycolor(int red, int green, int blue, int *pixval)
{
   int i;

   for (i = NUMBER_OF_COLORS; i < number_colors; i++) {
      if (abs(colorlist[i].color.red - red) < 512 &&
	     abs(colorlist[i].color.green - green) < 512 &&
	     abs(colorlist[i].color.blue - blue) < 512) {
	 if (pixval)
	    *pixval = colorlist[i].color.pixel;
	 return i;
	 break;
      }
   }
   return ERRORCOLOR;
}

/*-----------------------------------------------------------------------*/
/* Allocate new color from RGB values (e.g., from PS file "scb" command) */
/*-----------------------------------------------------------------------*/

#if defined(XC_WIN32) && !defined(TCL_WRAPPER)

int rgb_alloccolor(int red, int green, int blue)
{
   ERROR FIXME!
   // XCircuit version 3.9:  This no longer works.  Need to
   // allocate the color and return an index into colorlist[].

   return RGB(red >> 8, green >> 8, blue >> 8);
}

#else

int rgb_alloccolor(int red, int green, int blue)
{
   XColor newcolor;
   int pixval, tableidx; /* i, (jdk) */

   /* if color is not already in list, try to allocate it; if allocation */
   /* fails, grab the closest match in the colormap.			 */

   tableidx = rgb_querycolor(red, green, blue, &pixval);

   if (tableidx < 0) {
      newcolor.red = red;
      newcolor.green = green;
      newcolor.blue = blue;
      newcolor.flags = DoRed | DoGreen | DoBlue;
      if (areawin->area) {
         if (XAllocColor(dpy, cmap, &newcolor) == 0)
            pixval = findnearcolor(&newcolor);
         else
	    pixval = newcolor.pixel;
      }
      else {
	 // No graphics mode:  Force pixel to be 24 bit RGB
	 pixval = (red & 0xff) | ((blue & 0xff) << 8) | ((green & 0xff) << 16);
      }

      // Add this to the colormap
      tableidx = addnewcolorentry(pixval);
   }
   return tableidx;
}

#endif

/*----------------------------------------------------------------------*/
/* Query a color by name to see if it is in the color table.		*/
/* Return the table index (NOT the pixel value) of the entry, 		*/
/* ERRORCOLOR if the color is not in the table, and BADCOLOR if the	*/
/* color name could not be parsed by XLookupColor().			*/
/*----------------------------------------------------------------------*/

int query_named_color(char *cname)
{
   XColor cvcolor, cvexact;
   int tableidx, result = 0;

   if (areawin->area)
      result = XLookupColor(dpy, cmap, cname, &cvexact, &cvcolor);
     
   if (result == 0) return BADCOLOR;

   tableidx = rgb_querycolor(cvcolor.red, cvcolor.green, cvcolor.blue, NULL);
   return tableidx;
}

/*-------------------------------------------------------------------------*/
/* Make the cursors from the cursor bit data				   */
/*-------------------------------------------------------------------------*/

void makecursors()
{
   XColor fgcolor, bgcolor;
   Window win = areawin->window;
#ifdef TCL_WRAPPER
   Tk_Uid fg_uid, bg_uid;
#endif

   if (areawin->area == NULL) return;

   bgcolor.pixel = colorlist[BACKGROUND].color.pixel;
   fgcolor.pixel = colorlist[FOREGROUND].color.pixel;
   XQueryColors(dpy, cmap, &fgcolor, 1);
   XQueryColors(dpy, cmap, &bgcolor, 1);

#ifdef TCL_WRAPPER

#ifdef XC_WIN32
#define Tk_GetCursorFromData CreateW32Cursor
#endif

   fg_uid = Tk_GetUid(Tk_NameOfColor(&fgcolor));
   bg_uid = Tk_GetUid(Tk_NameOfColor(&bgcolor));

   ARROW = (Cursor)Tk_GetCursorFromData(xcinterp, Tk_IdToWindow(dpy, win),
	arrow_bits, arrowmask_bits, arrow_width, arrow_height, arrow_x_hot, arrow_y_hot,
	fg_uid, bg_uid);
   CROSS = (Cursor)Tk_GetCursorFromData(xcinterp, Tk_IdToWindow(dpy, win),
	cross_bits, crossmask_bits, cross_width, cross_height, cross_x_hot, cross_y_hot,
	fg_uid, bg_uid);
   SCISSORS = (Cursor)Tk_GetCursorFromData(xcinterp, Tk_IdToWindow(dpy, win),
	scissors_bits, scissorsmask_bits, scissors_width, scissors_height,
	scissors_x_hot, scissors_y_hot, fg_uid, bg_uid);
   EDCURSOR = (Cursor)Tk_GetCursorFromData(xcinterp, Tk_IdToWindow(dpy, win),
	exx_bits, exxmask_bits, exx_width, exx_height, exx_x_hot, exx_y_hot,
	fg_uid, bg_uid);
   COPYCURSOR = (Cursor)Tk_GetCursorFromData(xcinterp, Tk_IdToWindow(dpy, win),
	copy_bits, copymask_bits, copy_width, copy_height, copy_x_hot, copy_y_hot,
	fg_uid, bg_uid);
   ROTATECURSOR = (Cursor)Tk_GetCursorFromData(xcinterp, Tk_IdToWindow(dpy, win),
	rot_bits, rotmask_bits, rot_width, rot_height, circle_x_hot, circle_y_hot,
	fg_uid, bg_uid);
   QUESTION = (Cursor)Tk_GetCursorFromData(xcinterp, Tk_IdToWindow(dpy, win),
	question_bits, questionmask_bits, question_width, question_height,
	question_x_hot, question_y_hot, fg_uid, bg_uid);
   CIRCLE = (Cursor)Tk_GetCursorFromData(xcinterp, Tk_IdToWindow(dpy, win),
	circle_bits, circlemask_bits, circle_width, circle_height, circle_x_hot,
	circle_y_hot, fg_uid, bg_uid);
   HAND = (Cursor)Tk_GetCursorFromData(xcinterp, Tk_IdToWindow(dpy, win),
	hand_bits, handmask_bits, hand_width, hand_height, hand_x_hot, hand_y_hot,
	fg_uid, bg_uid);

#ifdef XC_WIN32
#undef Tk_GetCursorFromData
#endif

#else
   ARROW = XCreatePixmapCursor(dpy, XCreateBitmapFromData(dpy, win, arrow_bits,
	arrow_width, arrow_height), XCreateBitmapFromData(dpy, win, arrowmask_bits,
	arrow_width, arrow_height), &fgcolor, &bgcolor, arrow_x_hot, arrow_y_hot);
   CROSS = XCreatePixmapCursor(dpy, XCreateBitmapFromData(dpy, win, cross_bits,
	cross_width, cross_height), XCreateBitmapFromData(dpy, win, crossmask_bits,
	cross_width, cross_height), &fgcolor, &bgcolor, cross_x_hot, cross_y_hot);
   SCISSORS = XCreatePixmapCursor(dpy, XCreateBitmapFromData(dpy, win, scissors_bits,
	scissors_width, scissors_height), XCreateBitmapFromData(dpy, win,
	scissorsmask_bits, scissors_width, scissors_height), &fgcolor,
	&bgcolor, scissors_x_hot, scissors_y_hot);
   EDCURSOR = XCreatePixmapCursor(dpy, XCreateBitmapFromData(dpy, win, exx_bits,
	exx_width, exx_height), XCreateBitmapFromData(dpy, win, exxmask_bits,
	exx_width, exx_height), &fgcolor, &bgcolor, exx_x_hot, exx_y_hot);
   COPYCURSOR = XCreatePixmapCursor(dpy, XCreateBitmapFromData(dpy, win, copy_bits,
	copy_width, copy_height), XCreateBitmapFromData(dpy, win, copymask_bits,
	copy_width, copy_height), &fgcolor, &bgcolor, copy_x_hot, copy_y_hot);
   ROTATECURSOR = XCreatePixmapCursor(dpy, XCreateBitmapFromData(dpy, win, rot_bits,
	rot_width, rot_height), XCreateBitmapFromData(dpy, win, rotmask_bits,
	rot_width, rot_height), &fgcolor, &bgcolor, circle_x_hot, circle_y_hot);
   QUESTION = XCreatePixmapCursor(dpy, XCreateBitmapFromData(dpy, win, question_bits,
	question_width, question_height), XCreateBitmapFromData(dpy, win,
	questionmask_bits, question_width, question_height),
	&fgcolor, &bgcolor, question_x_hot, question_y_hot);
   CIRCLE = XCreatePixmapCursor(dpy, XCreateBitmapFromData(dpy, win, circle_bits,
	circle_width, circle_height), XCreateBitmapFromData(dpy, win, circlemask_bits,
	circle_width, circle_height), &fgcolor, &bgcolor, circle_x_hot, circle_y_hot);
   HAND = XCreatePixmapCursor(dpy, XCreateBitmapFromData(dpy, win, hand_bits,
	hand_width, hand_height), XCreateBitmapFromData(dpy, win, handmask_bits,
	hand_width, hand_height), &fgcolor, &bgcolor, hand_x_hot, hand_y_hot);
#endif

   TEXTPTR = XCreateFontCursor(dpy, XC_xterm);
   WAITFOR = XCreateFontCursor(dpy, XC_watch);

   XRecolorCursor(dpy, TEXTPTR, &fgcolor, &bgcolor);
}

/*----------------------------------------------------------------------*/
/* Remove a window structure and deallocate all memory used by it.	*/
/* If it is the last window, this is equivalent to calling "quit".	*/
/*----------------------------------------------------------------------*/

void delete_window(XCWindowData *window)
{
   XCWindowData *searchwin, *lastwin = NULL;

   if (xobjs.windowlist->next == NULL) {
      quitcheck((window == NULL) ? NULL : window->area, NULL, NULL);
      return;
   }

   for (searchwin = xobjs.windowlist; searchwin != NULL; searchwin =
		searchwin->next) {
      if (searchwin == window) {
	 Matrixptr thismat;

	 /* Free any select list */
	 if (searchwin->selects > 0) free(searchwin->selectlist);

	 /* Free the matrix and pushlist stacks */

	 while (searchwin->MatStack != NULL) {
	    thismat = searchwin->MatStack;
	    searchwin->MatStack = searchwin->MatStack->nextmatrix;
	    free(thismat);
	 }
	 free_stack(&searchwin->hierstack);
	 free_stack(&searchwin->stack);

	 /* Free the GC */
	 XFreeGC(dpy, searchwin->gc);

	 if (lastwin != NULL)
	    lastwin->next = searchwin->next;
	 else
	    xobjs.windowlist = searchwin->next;
	 break;
      }
      lastwin = searchwin;
   }

   if (searchwin == NULL) {
      Wprintf("No such window in list!\n");
   }
   else {
      if (areawin == searchwin) areawin = xobjs.windowlist;
      free(searchwin);
   }
}

/*----------------------------------------------------------------------*/
/* Create a new window structure and initialize it.			*/
/* Return a pointer to the new window.					*/
/*----------------------------------------------------------------------*/

XCWindowData *create_new_window()
{
   XCWindowData *newwindow;

   newwindow = (XCWindowData *)malloc(sizeof(XCWindowData));

#ifndef TCL_WRAPPER
#ifdef HAVE_XPM
   newwindow->toolbar_on = True;
#endif
#endif

   newwindow->area = (xcWidget)NULL;
   newwindow->mapped = False;
   newwindow->psfont = 0;
   newwindow->anchor = FLIPINV;
   newwindow->page = 0;
   newwindow->MatStack = NULL;
   newwindow->textscale = 1.0;
   newwindow->linewidth = 1.0;
   newwindow->zoomfactor = SCALEFAC;
   newwindow->style = UNCLOSED;
   newwindow->invert = False;
   newwindow->axeson = True;
   newwindow->snapto = True;
   newwindow->gridon = True;
   newwindow->center = True;
   newwindow->bboxon = False;
   newwindow->filter = ALL_TYPES;
   newwindow->editinplace = True;
   newwindow->selects = 0;
   newwindow->selectlist = NULL;
   newwindow->lastlibrary = 0;
   newwindow->manhatn = False;
   newwindow->boxedit = MANHATTAN;
   newwindow->pathedit = TANGENTS;
   newwindow->lastbackground = NULL;
   newwindow->editstack = (objectptr) malloc(sizeof(object));
   newwindow->stack = NULL;   /* at the top of the hierarchy */
   newwindow->hierstack = NULL;
   initmem(newwindow->editstack);
   newwindow->pinpointon = False;
   newwindow->showclipmasks = True;
   newwindow->pinattach = False;
   newwindow->buschar = '(';	/* Vector notation for buses */
   newwindow->defaultcursor = &CROSS;
   newwindow->event_mode = NORMAL_MODE;
   newwindow->attachto = -1;
   newwindow->color = DEFAULTCOLOR;
   newwindow->gccolor = 0;
   newwindow->time_id = 0;
   newwindow->redraw_needed = True;
   newwindow->redraw_ongoing = False;
#ifdef HAVE_CAIRO
   newwindow->fixed_pixmap = NULL;
   newwindow->cr = NULL;
#else /* HAVE_CAIRO */
   newwindow->clipmask = (Pixmap)NULL;
   newwindow->pbuf = (Pixmap)NULL;
   newwindow->cmgc = (GC)NULL;
   newwindow->clipped = 0;
   newwindow->fixed_pixmap = (Pixmap) NULL;
#endif /* !HAVE_CAIRO */
   newwindow->vscale = 1;
   newwindow->pcorner.x = newwindow->pcorner.y = 0;
   newwindow->topinstance = (objinstptr)NULL;
   newwindow->panx = newwindow->pany = 0;

   /* Prepend to linked window list in global data (xobjs) */
   newwindow->next = xobjs.windowlist;
   xobjs.windowlist = newwindow;

   return newwindow;
}

/*----------------------------------------------------------------------*/
/* Preparatory initialization (to be run before setting up the GUI)	*/
/*----------------------------------------------------------------------*/

void pre_initialize()
{
   short i, page;

   /*-------------------------------------------------------------*/
   /* Force LC_NUMERIC locale to en_US for decimal point = period */
   /* notation.  The environment variable LC_NUMERIC overrides if */
   /* it is set explicitly, so it has to be unset first to allow  */
   /* setlocale() to work.					  */
   /*-------------------------------------------------------------*/

#ifdef HAVE_PUTENV
   putenv("LC_ALL=en_US");
   putenv("LC_NUMERIC=en_US");
   putenv("LANG=POSIX");
#else
   unsetenv("LC_ALL");
   unsetenv("LC_NUMERIC");
   setenv("LANG", "POSIX", 1);
#endif
   setlocale(LC_ALL, "en_US");

   /*---------------------------*/
   /* initialize user variables */
   /*---------------------------*/

   strcpy(version, PROG_VERSION);
   aliastop = NULL;
   xobjs.pagelist = (Pagedata **) malloc(PAGES * sizeof(Pagedata *));
   for (page = 0; page < PAGES; page++) {
      xobjs.pagelist[page] = (Pagedata *) malloc(sizeof(Pagedata));
      xobjs.pagelist[page]->pageinst = NULL;
      xobjs.pagelist[page]->filename = NULL;
   }
   /* Set values for the first page */
   xobjs.pagelist[0]->wirewidth = 2.0;
   xobjs.pagelist[0]->outscale = 1.0;
   xobjs.pagelist[0]->background.name = (char *)NULL;
   xobjs.pagelist[0]->pmode = 2;	/* set auto-fit ON by default */
   xobjs.pagelist[0]->orient = 0;
   xobjs.pagelist[0]->gridspace = DEFAULTGRIDSPACE;
   xobjs.pagelist[0]->snapspace = DEFAULTSNAPSPACE;
   xobjs.pagelist[0]->drawingscale.x = xobjs.pagelist[0]->drawingscale.y = 1;
   xobjs.pagelist[0]->coordstyle = INTERNAL;
   xobjs.pagelist[0]->pagesize.x = 612;
   xobjs.pagelist[0]->pagesize.y = 792;
   xobjs.pagelist[0]->margins.x = 72;
   xobjs.pagelist[0]->margins.y = 72;

   xobjs.hold = TRUE;
   xobjs.showtech = FALSE;
   xobjs.suspend = (signed char)0; /* Suspend graphics until finished with startup */
   xobjs.new_changes = 0;
   xobjs.filefilter = TRUE;
   xobjs.tempfile = NULL;
   xobjs.retain_backup = False;	/* default: remove backup after file write */
   signal(SIGINT, dointr);
   printtime_id = 0;

   xobjs.technologies = NULL;
   xobjs.undostack = NULL;
   xobjs.redostack = NULL;

   /* Set the temporary directory name as compiled, unless overridden by */
   /* environment variable "TMPDIR".					 */

   xobjs.tempdir = getenv("TMPDIR");
   if (xobjs.tempdir == NULL) xobjs.tempdir = strdup(TEMP_DIR);

   xobjs.windowlist = (XCWindowDataPtr)NULL;
   areawin = NULL;

   xobjs.numlibs = LIBS - LIBRARY - 1;
   xobjs.fontlib.number = 0;
   xobjs.userlibs = (Library *) malloc(xobjs.numlibs * sizeof(Library));
   for (i = 0; i < xobjs.numlibs; i++) {
      xobjs.userlibs[i].library = (objectptr *) malloc(sizeof(objectptr));
      xobjs.userlibs[i].instlist = NULL;
      xobjs.userlibs[i].number = 0;
   }
   xobjs.imagelist = NULL;
   xobjs.images = 0;
   xobjs.pages = PAGES;

   xobjs.libsearchpath = (char *)NULL;
   xobjs.filesearchpath = (char *)NULL;

   fontcount = 0;
   fonts = (fontinfo *) malloc(sizeof(fontinfo));
   fonts[0].encoding = NULL;	/* To prevent segfaults */
   fonts[0].psname = NULL;
   fonts[0].family = NULL;

   /* Initialization of objects requires values for the window width and height, */
   /* so set up the widgets and realize them first.				 */

   popups = 0;        /* no popup windows yet */
   beeper = 1;        /* Ring bell on certain warnings or errors */
   pressmode = 0;	/* not in a button press & hold mode yet */
   initsplines();	/* create lookup table of spline parameters */
}

#ifdef TCL_WRAPPER

/*----------------------------------------------------------------------*/
/* Create a new Handle object in Tcl */
/*----------------------------------------------------------------------*/

static void UpdateStringOfHandle _ANSI_ARGS_((Tcl_Obj *objPtr));
static int SetHandleFromAny _ANSI_ARGS_((Tcl_Interp *interp, Tcl_Obj *objPtr));

static Tcl_ObjType tclHandleType = {
    "handle",				/* name */
    (Tcl_FreeInternalRepProc *) NULL,	/* freeIntRepProc */
    (Tcl_DupInternalRepProc *) NULL,	/* dupIntRepProc */
    UpdateStringOfHandle,		/* updateStringProc */
    SetHandleFromAny			/* setFromAnyProc */
};

/*----------------------------------------------------------------------*/

static void
UpdateStringOfHandle(objPtr)
    Tcl_Obj *objPtr;   /* Int object whose string rep to update. */
{
    char buffer[TCL_INTEGER_SPACE];
    int len;

    sprintf(buffer, "H%08lX", objPtr->internalRep.longValue);
    len = strlen(buffer);

    objPtr->bytes = Tcl_Alloc((u_int)len + 1);
    strcpy(objPtr->bytes, buffer);
    objPtr->length = len;
}

/*----------------------------------------------------------------------*/

static int
SetHandleFromAny(interp, objPtr)
    Tcl_Interp *interp;         /* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr;   /* The object to convert. */
{  
    Tcl_ObjType *oldTypePtr = (Tcl_ObjType *)objPtr->typePtr;
    char *string, *end;
    int length;
    char *p;
    long newLong;
    pushlistptr newstack = NULL;

    string = Tcl_GetStringFromObj(objPtr, &length);
    errno = 0;
    for (p = string;  isspace((u_char)(*p));  p++);

nexthier:

    if (*p++ != 'H') {
	if (interp != NULL) {
            Tcl_ResetResult(interp);
            Tcl_AppendToObj(Tcl_GetObjResult(interp), 
		"handle is identified by leading H and hexidecimal value only", -1);
        }
        free_stack(&newstack);
        return TCL_ERROR;
    } else {
        newLong = strtoul(p, &end, 16);
    }
    if (end == p) {
        badHandle:
        if (interp != NULL) {
            /*
             * Must copy string before resetting the result in case a caller
             * is trying to convert the interpreter's result to an int.
             */

            char buf[100];
            sprintf(buf, "expected handle but got \"%.50s\"", string);
            Tcl_ResetResult(interp);
            Tcl_AppendToObj(Tcl_GetObjResult(interp), buf, -1);
        }
        free_stack(&newstack);
        return TCL_ERROR;
    }
    if (errno == ERANGE) {
        if (interp != NULL) {
            char *s = "handle value too large to represent";
            Tcl_ResetResult(interp);
            Tcl_AppendToObj(Tcl_GetObjResult(interp), s, -1);
            Tcl_SetErrorCode(interp, "ARITH", "IOVERFLOW", s, (char *) NULL);
        }
        free_stack(&newstack);
        return TCL_ERROR;
    }
    /*
     * Make sure that the string has no garbage after the end of the handle.
     */
   
    while ((end < (string+length)) && isspace((u_char)(*end))) end++;
    if (end != (string+length)) {
       /* Check for handles separated by slashes.  If present,	*/      
       /* then generate a hierstack.				*/

	if ((end != NULL) && (*end == '/')) {
	   objinstptr refinst, chkinst;
	   genericptr *rgen;

	   *end = '\0';
           newLong = strtoul(p, &end, 16);
	   p = end + 1;
	   *end = '/';
	   refinst = (newstack == NULL) ? areawin->topinstance : newstack->thisinst;
	   chkinst = (objinstptr)((pointertype)(newLong));
	   /* Ensure that chkinst is in the plist of			*/
	   /* refinst->thisobject, and that it is type objinst.	*/
	   for (rgen = refinst->thisobject->plist; rgen < refinst->thisobject->plist
			+ refinst->thisobject->parts; rgen++) {
	      if ((objinstptr)(*rgen) == chkinst) {
		 if (ELEMENTTYPE(*rgen) != OBJINST) {
		    free_stack(&newstack);
		    Tcl_SetResult(interp, "Hierarchical element handle "
				"component is not an object instance.", NULL);
		    return TCL_ERROR;
		 }
		 break;
	      }
	   }
	   if (rgen == refinst->thisobject->plist + refinst->thisobject->parts) {
               Tcl_SetResult(interp, "Bad component in hierarchical "
			"element handle.", NULL);
	       free_stack(&newstack);
	       return TCL_ERROR;
	   }
	   push_stack(&newstack, chkinst, NULL);
	   goto nexthier;
        }
	else
	   goto badHandle;
    }
   
    /* Note that this check won't prevent a hierarchical selection from	*/
    /* being added to a non-hierarchical selection.			*/

    if (areawin->hierstack != NULL) {
       if ((newstack == NULL) || (newstack->thisinst !=
		areawin->hierstack->thisinst)) {
	  Tcl_SetResult(interp, "Attempt to select components in different "
			"objects.", NULL);
          free_stack(&newstack);
	  return TCL_ERROR;
       }
    }
    free_stack(&areawin->hierstack);
    areawin->hierstack = newstack;

    /*
     * The conversion to handle succeeded. Free the old internalRep before
     * setting the new one. We do this as late as possible to allow the
     * conversion code, in particular Tcl_GetStringFromObj, to use that old
     * internalRep.
     */
   
    if ((oldTypePtr != NULL) && (oldTypePtr->freeIntRepProc != NULL)) {
        oldTypePtr->freeIntRepProc(objPtr);
    }
   
    objPtr->internalRep.longValue = newLong;
    objPtr->typePtr = &tclHandleType;
    return TCL_OK;
}  

/*----------------------------------------------------------------------*/

Tcl_Obj *
Tcl_NewHandleObj(optr)
    void *optr;      /* Int used to initialize the new object. */
{
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewObj();
    objPtr->bytes = NULL;

    objPtr->internalRep.longValue = (long)(optr);
    objPtr->typePtr = &tclHandleType;
    return objPtr;
}

/*----------------------------------------------------------------------*/

int
Tcl_GetHandleFromObj(interp, objPtr, handlePtr)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr;	/* The object from which to get a int. */
    void **handlePtr;	/* Place to store resulting int. */
{
    long l;
    int result;

    if (objPtr->typePtr != &tclHandleType) {
        result = SetHandleFromAny(interp, objPtr);
        if (result != TCL_OK) {
            return result;
        }
    }
    l = objPtr->internalRep.longValue;
    if (((long)((int)l)) == l) {
        *handlePtr = (void *)objPtr->internalRep.longValue;
        return TCL_OK;
    }
    if (interp != NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendToObj(Tcl_GetObjResult(interp),
                "value too large to represent as handle", -1);
    }
    return TCL_ERROR;
}


#endif

/*----------------------------------------------------------------------*/
/* Routine to initialize variables after the GUI has been set up	*/
/*----------------------------------------------------------------------*/

void post_initialize()
{
   short i;

   /*--------------------------------------------------*/
   /* Setup the (simple) colormap and make the cursors */
   /*--------------------------------------------------*/

   setcolorscheme(True);
   makecursors();

   /* Now that we have values for the window width and height, we can initialize */
   /* the page objects.								 */

   xobjs.libtop = (objinstptr *)malloc(LIBS * sizeof(objinstptr));
   for (i = 0; i < LIBS; i++) {
      objectptr newlibobj = (objectptr) malloc(sizeof(object));
      initmem(newlibobj);
      xobjs.libtop[i] = newpageinst(newlibobj);
   }

   /* Give names to the five default libraries */
   strcpy(xobjs.libtop[FONTLIB]->thisobject->name, "Font Character List");
   strcpy(xobjs.libtop[PAGELIB]->thisobject->name, "Page Directory");
   strcpy(xobjs.libtop[LIBLIB]->thisobject->name,  "Library Directory");
   strcpy(xobjs.libtop[USERLIB]->thisobject->name, "User Library");
   renamelib(USERLIB);

   changepage(0);

   /* Centering the view is not required here because the default values */
   /* set in initmem() should correctly position the empty page in the	 */
   /* middle of the viewing window.					 */

#if !defined(HAVE_CAIRO)
   if (dbuf == (Pixmap)NULL)
      dbuf = XCreatePixmap(dpy, areawin->window, areawin->width,
		areawin->height, DefaultDepthOfScreen(xcScreen(areawin->area)));
#endif

   /* Set up fundamentally necessary colors black and white */

   addnewcolorentry(xc_alloccolor("Black"));
   addnewcolorentry(xc_alloccolor("White"));

#ifdef TCL_WRAPPER

   /* Set up new Tcl type "handle" for element handles */
   Tcl_RegisterObjType(&tclHandleType);

#endif

   /*-----------------------------------------------------*/
   /* Set the cursor as a crosshair for the area widget.  */
   /*-----------------------------------------------------*/

   if (areawin->area != NULL) {
      XDefineCursor (dpy, areawin->window, DEFAULTCURSOR);
   }

   /*---------------------------------------------------*/
   /* Set up a timeout for automatic save to a tempfile */
   /*---------------------------------------------------*/

   xobjs.save_interval = appdata.timeout;
   xobjs.timeout_id = xcAddTimeOut(app, 60000 * xobjs.save_interval,
	savetemp, NULL);
}

/*----------------------------------------------------------------------*/
