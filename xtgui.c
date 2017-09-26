/*----------------------------------------------------------------------*/
/* xtgui.c --- 								*/
/* XCircuit's graphical user interface using the Xw widget set		*/
/* (see directory Xw) (non-Tcl/Tk GUI)					*/
/* Copyright (c) 2002  R. Timothy Edwards				*/
/*----------------------------------------------------------------------*/

#ifndef TCL_WRAPPER

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
#ifndef XC_WIN32
#include <unistd.h>   /* for unlink() */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

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

#ifdef HAVE_CAIRO
#include <cairo/cairo-xlib.h>
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

#ifdef HAVE_XPM
#ifndef XC_WIN32
#include <X11/xpm.h>
#endif
#include "lib/pixmaps/xcircuit.xpm"
#endif

/*----------------------------------------------------------------------*/
/* Global Variable definitions						*/
/*----------------------------------------------------------------------*/

extern short popups;	/* total number of popup widgets on the screen */
extern int pressmode;	/* Whether we are in a press & hold state */

xcWidget   top;
xcWidget   message1, message2, message3, toolbar, overlay = NULL;
xcWidget   menuwidgets[MaxMenuWidgets];
xcWidget   wschema, wsymb, netbutton;
XtAppContext app;
Atom wprot, wmprop[2];

extern char _STR2[250];  /* Specifically for text returned from the popup prompt */
extern char _STR[150];   /* Generic multipurpose string */
extern Display *dpy;
extern Colormap cmap;
extern Pixmap STIPPLE[STIPPLES];
extern Cursor appcursors[NUM_CURSORS];
extern ApplicationData appdata;
extern XCWindowData *areawin;
extern Globaldata xobjs;
extern int number_colors;
extern colorindex *colorlist;
extern short menusize;
extern xcIntervalId printtime_id;
extern short beeper;
extern short fontcount;
extern fontinfo *fonts;
extern short help_up;
extern menustruct TopButtons[];
#ifdef HAVE_XPM
extern toolbarstruct ToolBar[];
extern short toolbuttons;
#endif
extern short maxbuttons;
extern Pixmap helppix;
extern aliasptr aliastop;
extern float version;

static char STIPDATA[STIPPLES][4] = {
   "\000\004\000\001",
   "\000\005\000\012",
   "\001\012\005\010",
   "\005\012\005\012",
   "\016\005\012\007",
   "\017\012\017\005",
   "\017\012\017\016",
   "\000\000\000\000"
};

extern Pixmap dbuf;

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
/* Initial Resource Management						*/
/*----------------------------------------------------------------------*/

#ifndef XC_WIN32

static XtResource resources[] = {

  /* schematic layout colors */

  { "globalpincolor", "GlobalPinColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, globalcolor), XtRString, "Orange2"},
  { "localpincolor", "LocalPinColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, localcolor), XtRString, "Red"},
  { "infolabelcolor", "InfoLabelColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, infocolor), XtRString, "SeaGreen"},
  { "ratsnestcolor", "RatsNestColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, ratsnestcolor), XtRString, "Tan4"},

  /* non-schematic layout colors */

  { "bboxcolor", "BBoxColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, bboxpix), XtRString, "greenyellow"},
  { "fixedbboxcolor", "FixedBBoxColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, fixedbboxpix), XtRString, "Pink"},
  { "clipcolor", "ClipColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, clipcolor), XtRString, "powderblue"},

  /* GUI Color scheme 1 */

  { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, fg), XtRString, "Black"},
  { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, bg), XtRString, "White"},
  { "gridcolor", "GridColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, gridpix), XtRString, "Gray95"},
  { "snapcolor", "SnapColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, snappix), XtRString, "Red"},
  { "selectcolor", "SelectColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, selectpix), XtRString, "Gold3"},
  { "filtercolor", "FilterColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, filterpix), XtRString, "SteelBlue3"},
  { "axescolor", "AxesColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, axespix), XtRString, "Antique White"},
  { "offbuttoncolor", "OffButtonColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, buttonpix), XtRString, "Gray85"},
  { "auxiliarycolor", "AuxiliaryColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, auxpix), XtRString, "Green3"},
  { "barcolor", "BarColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, barpix), XtRString, "Tan"},
  { "paramcolor", "ParamColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, parampix), XtRString, "Plum3"},

  /* GUI Color scheme 2 */

  { "foreground2", XtCForeground, XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, fg2), XtRString, "White"},
  { "background2", XtCBackground, XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, bg2), XtRString, "DarkSlateGray"},
  { "gridcolor2", "GridColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, gridpix2), XtRString, "Gray40"},
  { "snapcolor2", "SnapColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, snappix2), XtRString, "Red"},
  { "selectcolor2", "SelectColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, selectpix2), XtRString, "Gold"},
  { "axescolor2", "AxesColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, axespix2), XtRString, "NavajoWhite4"},
  { "offbuttoncolor2", "OffButtonColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, buttonpix2), XtRString, "Gray50"},
  { "auxiliarycolor2", "AuxiliaryColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, auxpix2), XtRString, "Green"},
  { "paramcolor2", "ParamColor", XtRPixel, sizeof(Pixel),
      XtOffset(ApplicationDataPtr, parampix2), XtRString, "Plum3"},

  /* Other XDefaults-set properties */

  { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
      XtOffset(ApplicationDataPtr, xcfont), XtRString,
		"-*-times-bold-r-normal--14-*"},
  { "helpfont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
      XtOffset(ApplicationDataPtr, helpfont), XtRString,
		"-*-helvetica-medium-r-normal--10-*"},
  { "filelistfont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
      XtOffset(ApplicationDataPtr, filefont), XtRString,
		"-*-helvetica-medium-r-normal--14-*"},
  { "textfont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
      XtOffset(ApplicationDataPtr, textfont), XtRString,
		"-*-courier-medium-r-normal--14-*"},
  { "titlefont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
      XtOffset(ApplicationDataPtr, titlefont), XtRString,
		"-*-times-bold-i-normal--14-*"},
  { XtNwidth, XtCWidth, XtRInt, sizeof(int),
      XtOffset(ApplicationDataPtr, width), XtRString, "950"},
  { XtNheight, XtCHeight, XtRInt, sizeof(int),
      XtOffset(ApplicationDataPtr, height), XtRString, "760"},
  { "timeout", "TimeOut", XtRInt, sizeof(int),
      XtOffset(ApplicationDataPtr, timeout), XtRString, "10"}
};

#endif

/*----------------------------------------------------------------------*/
/* Add a new color button to the color menu				*/
/* 	called if new color button needs to be made 			*/
/*----------------------------------------------------------------------*/

int addnewcolorentry(int ccolor)
{
   xcWidget colormenu, newbutton;
   Arg wargs[2];
   int i, n = 0;

   /* check to see if entry is already in the color list */

   for (i = NUMBER_OF_COLORS; i < number_colors; i++)
      if (colorlist[i].color.pixel == ccolor) break;

   /* make new entry in the menu */

   if (i == number_colors) {

      colormenu = xcParent(ColorAddNewColorButton);
      XtnSetArg(XtNlabelType, XwRECT);
      XtnSetArg(XtNrectColor, ccolor);

      newbutton = XtCreateWidget("NewColor", XwmenubuttonWidgetClass,
         colormenu, wargs, n);
      XtAddCallback (newbutton, XtNselect, (XtCallbackProc)setcolor, NULL);
      XtManageChild(newbutton);

      addtocolorlist(newbutton, ccolor);
   }
   return i;
}

/*----------------------------------------------------------------------*/
/* This recursive function looks down the button menu hierarchy and     */
/*   creates the necessary buttons and submenus.			*/
/*   Menu entries are marked if the corresponding "size" entry in the	*/
/*   menu structure is > 0.						*/
/*----------------------------------------------------------------------*/

void makesubmenu(char *menuname, char *attachname, menuptr buttonmenu,
		int arraysize, xcWidget manager)
{
   short i, n = 0;
   int cval;
   xcWidget popupshell, cascade;
   Arg	wargs[6];
   menuptr p;
   char popupname[30];

   sprintf(popupname, "popup%s", menuname);
   popupshell = XtCreatePopupShell (popupname, transientShellWidgetClass,
	manager,  NULL, 0);

   XtnSetArg(XtNattachTo, attachname);
   XtnSetArg(XtNfont, appdata.titlefont);
   cascade = XtCreateManagedWidget (menuname, XwcascadeWidgetClass,
	popupshell, wargs, n);
   
   for (p = buttonmenu, i = 0; p < buttonmenu + arraysize; p++, i++) {
      n = 0;
      if (p->size > 0 && p->submenu == NULL) { /* This denotes a marked entry */
	 XtnSetArg(XtNsetMark, True);
      }
      XtnSetArg(XtNfont, appdata.xcfont);

      if (p->submenu != NULL) {
         xcWidget newbutton = XtCreateWidget(p->name, XwmenubuttonWidgetClass,
	   	cascade, wargs, n);
	 makesubmenu(p->name, p->name, p->submenu, p->size, manager);
         XtManageChild (newbutton);
      }
      else if (p->name[0] == ' ') {
         /* This is a separator, made from a PushButton widget */

         xcWidget newbutton = XtCreateWidget(p->name, XwmenuButtonWidgetClass,
	   cascade, wargs, n); n = 0;
         XtManageChild (newbutton);

         XtnSetArg(XtNheight, 5);
	 XtnSetArg(XtNsensitive, False);
         XtSetValues(newbutton, wargs, n);
      }
      else {
         if (p->name[0] == '_') {  /* Color button */
	    cval = xc_alloccolor(p->name + 1);
	    XtnSetArg(XtNlabelType, XwRECT);
	    XtnSetArg(XtNrectColor, cval);
         }
         else if (p->name[0] == ':') {  /* Stipple button */
	    XtnSetArg(XtNlabelType, XwRECT);
            if (((pointertype)(p->passeddata) == (OPAQUE | FILLED | FILLSOLID))) {
	       XtnSetArg(XtNrectColor, BlackPixel(dpy,DefaultScreen(dpy)));
	    }
	    else {
	       XtnSetArg(XtNrectStipple, STIPPLE[((pointertype)(p->passeddata) &
	   	     FILLSOLID) >> 5]);
	    }
         }
         menuwidgets[++menusize] = XtCreateWidget(p->name, XwmenubuttonWidgetClass,
	      cascade, wargs, n);
	 XtAddCallback (menuwidgets[menusize], XtNselect, (XtCallbackProc)p->func,
		   p->passeddata);
	 if (p->name[0] == '_') {
            /* For color buttons, maintain a list of Widgets and color values */
            addtocolorlist(menuwidgets[menusize], cval);
	 }

         XtManageChild (menuwidgets[menusize]);
      }
   }
}

#ifdef HAVE_XPM

/*----------------------------------------------------------------------*/
/* Toolbar Creator							*/
/*----------------------------------------------------------------------*/

#ifndef XC_WIN32

void createtoolbar (xcWidget abform, Widget aform)
{
   int i, n = 0;
   Arg	wargs[12];
   XImage *iret;
   XpmAttributes attr;

   XtnSetArg(XtNxRefWidget, aform);
   XtnSetArg(XtNxAddWidth, True);
   XtnSetArg(XtNxAttachRight, True);
   XtnSetArg(XtNyAttachBottom, True);
   XtnSetArg(XtNborderWidth, 0);
   XtnSetArg(XtNxOffset, 2);
   XtnSetArg(XtNyOffset, 2);
   XtnSetArg(XtNxResizable, False);
   XtnSetArg(XtNyResizable, True);

   XtnSetArg(XtNlayout, XwIGNORE);
   toolbar = XtCreateManagedWidget("ToolBar", XwbulletinWidgetClass, abform,
	wargs, n); n = 0;

   /* Fix for limited-color capability video.  Thanks to */
   /* Frankie Liu <frankliu@stanford.edu> 		*/

   attr.valuemask = XpmSize | XpmCloseness;
   attr.closeness = 65536;

   for (i = 0; i < toolbuttons; i++) {
      XpmCreateImageFromData(dpy, ToolBar[i].icon_data, &iret, NULL, &attr);
      XtnSetArg(XtNlabelType, XwIMAGE);
      XtnSetArg(XtNlabelImage, iret);
      XtnSetArg(XtNwidth, attr.width + 4);
      XtnSetArg(XtNheight, attr.height + 4);
      XtnSetArg(XtNborderWidth, TBBORDER);
      XtnSetArg(XtNnoPad, True);
      XtnSetArg(XtNhint, ToolBar[i].hint);
      XtnSetArg(XtNhintProc, Wprintf);

      menuwidgets[++menusize] = XtCreateManagedWidget(ToolBar[i].name,
	 XwmenuButtonWidgetClass, toolbar, wargs, n); n = 0;
      XtAddCallback(menuwidgets[menusize], XtNselect,
		(XtCallbackProc)ToolBar[i].func, ToolBar[i].passeddata);
   }
}

#endif

/*----------------------------------------------------------------------*/
/* Toolbar Resize							*/
/*----------------------------------------------------------------------*/

void resizetoolbar()
{
   int i, n = 0, bcol = 0; 
   int max_width = 0, max_height = 0, tot_width = 0;
   Arg	wargs[5];
   xcWidget bwptr, lbwptr, bmax, lbmax;
   Dimension t_height, bheight, bwidth;
   xcWidget *tool_list = NULL;
   int pytools = 0;

   if (!xcIsRealized(toolbar)) return;

   /* Avoid recursive calls to self and extra calls to the main draw routine */

   XtRemoveCallback(areawin->area, XtNresize, (XtCallbackProc)resizetoolbar, NULL);
   XtRemoveCallback(areawin->area, XtNresize, (XtCallbackProc)resizearea, NULL);

   /* Find the height of the toolbar from the parent widget */

   XtnSetArg(XtNheight, &t_height);
   XtGetValues(xcParent(toolbar), wargs, n); n = 0;

#ifdef HAVE_PYTHON
   tool_list = pytoolbuttons(&pytools);
#endif

   /* Realign the tool buttons inside the fixed space */

   for (i = 0; i < toolbuttons + pytools; i++) {
#ifdef HAVE_PYTHON
      if (i >= toolbuttons)
	 bwptr = tool_list[i - toolbuttons];
      else
#endif
         bwptr = XtNameToWidget(toolbar, ToolBar[i].name);
      if (bwptr == (Widget)NULL) break;

      XtnSetArg(XtNheight, &bheight);
      XtnSetArg(XtNwidth, &bwidth);
      XtGetValues(bwptr, wargs, n); n = 0;
      bheight += (TBBORDER << 1);
      max_height += bheight;
      if (max_height > t_height) {
	 bcol++;
         lbmax = bmax;
	 tot_width += max_width;
	 max_width = 0;
	 max_height = (int)bheight;
      }
      if (bwidth > max_width) {
	 max_width = (int)bwidth;
	 bmax = bwptr;
      }
      
      XtnSetArg(XtNx, tot_width);
      XtnSetArg(XtNy, max_height - bheight);
      XtSetValues(bwptr, wargs, n); n = 0;
      lbwptr = bwptr;
   }

   XtnSetArg(XtNwidth, tot_width + max_width);
   XtSetValues(toolbar, wargs, n); n = 0;

   /* Reinstate callbacks */
   XtAddCallback(areawin->area, XtNresize, (XtCallbackProc)resizetoolbar, NULL);
   XtAddCallback(areawin->area, XtNresize, (XtCallbackProc)resizearea, NULL);

#ifdef HAVE_PYTHON
   if (tool_list != NULL) free(tool_list);
#endif
}

#endif /* HAVE_XPM */

/*----------------------------------------------------------------------*/
/* Hierarchical Menu Creator						*/
/*   This function creates the top level of buttons which are arranged  */
/*   across the top starting at the left edge.  For each button 	*/
/*   that has a submenu, a Popup manager is created, and then menu	*/
/*   panes are attached to the manager in a hierarchical fashion.	*/
/*   Note: Returns widget for last button on top level			*/
/*----------------------------------------------------------------------*/

void createmenus (xcWidget form, xcWidget *firstbutton, xcWidget *lastbutton)
{
   int i, maxmgrs = 0, n = 0, j = 0;
   WidgetList buttonw, mgr_shell, menu_mgr;
   Arg	wargs[6];

   menusize = -1;

   for (i = 0; i < maxbuttons; i++) 
      if (TopButtons[i].submenu != NULL) maxmgrs++;

   buttonw = (WidgetList) XtMalloc(maxbuttons * sizeof(Widget));
   mgr_shell = (WidgetList) XtMalloc(maxmgrs * sizeof(Widget));
   menu_mgr = (WidgetList) XtMalloc(maxmgrs * sizeof(Widget));

   for (i = 0; i < maxbuttons; i++) {

      XtnSetArg(XtNheight, ROWHEIGHT);
      XtnSetArg(XtNlabel, TopButtons[i].name);
      XtnSetArg(XtNfont, appdata.xcfont);
      if (i > 0) {
	 XtnSetArg(XtNxRefWidget, buttonw[i - 1]);
	 XtnSetArg(XtNxAddWidth, True);
      }
      buttonw[i] = XtCreateWidget(TopButtons[i].name, 
	 	XwmenuButtonWidgetClass, form, wargs, n); n = 0;

      if (!strcmp(TopButtons[i].name, "Netlist")) {
	 netbutton = buttonw[i];
         XtManageChild(buttonw[i]);
      }
      else
         XtManageChild(buttonw[i]);

      if(TopButtons[i].submenu == NULL) 
	 XtAddCallback(buttonw[i], XtNselect,
		(XtCallbackProc)TopButtons[i].func, NULL);
      else {
         mgr_shell[j] = XtCreatePopupShell("mgr_shell", shellWidgetClass,
	    buttonw[i], NULL, 0);
         menu_mgr[j] = XtCreateManagedWidget("menu_mgr", XwpopupmgrWidgetClass,
	    mgr_shell[j], NULL, 0);
	 makesubmenu(TopButtons[i].name, "menu_mgr", TopButtons[i].submenu, 
	    TopButtons[i].size, menu_mgr[j]);
	 j++;
      }
   }
   *firstbutton = buttonw[0];
   *lastbutton = buttonw[i - 1];
}

/*----------------------------------------------*/
/* Check for conditions to approve program exit */
/*----------------------------------------------*/

int quitcheck(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   char *promptstr;
   Boolean doprompt = False;
   buttonsave *savebutton;

   /* enable default interrupt signal handler during this time, so that */
   /* a double Control-C will ALWAYS exit.				*/

   signal(SIGINT, SIG_DFL);
   promptstr = (char *)malloc(22);
   strcpy(promptstr, "Unsaved changes in: ");

   /* Check all page objects for unsaved changes */

   doprompt = (countchanges(&promptstr) > 0) ? True : False;

   /* If any changes have not been saved, generate a prompt */

   if (doprompt) {
      promptstr = (char *)realloc(promptstr, strlen(promptstr) + 15);
      strcat(promptstr, "\nQuit anyway?");
      savebutton = getgeneric(w, quitcheck, NULL);
      popupprompt(w, promptstr, NULL, quit, savebutton, NULL);
      free(promptstr);
      return 0;
   }
   else {
      free(promptstr);
      quit(areawin->area, NULL);
      return 1;
   }
}

/*-------------------------------------------------------------------------*/
/* Popup dialog box routines						   */
/*-------------------------------------------------------------------------*/
/* Propogate any key event from the dialog box into the textedit widget    */
/*-------------------------------------------------------------------------*/

#ifndef XC_WIN32

void propevent(xcWidget w, xcWidget editwidget, XEvent *event)
{
   Window ewin = xcWindow(editwidget);

   event->xany.window = ewin;
   XSendEvent(dpy, ewin, False, KeyPressMask, event);
}

#endif

/*-------------------------------------------------------------------------*/
/* Destroy an interactive text-editing popup box 			   */
/*-------------------------------------------------------------------------*/

void destroypopup(xcWidget button, popupstruct *callstruct, caddr_t calldata)
{
   Arg	wargs[1];

   if(XtNameToWidget(callstruct->popup, "help2") != NULL) {
      help_up = False;
      XFreePixmap(dpy, helppix);
      helppix = (Pixmap)NULL;
   }

   if (callstruct->buttonptr->button != NULL) {  

      /* return the button to its normal state */

      XtSetArg(wargs[0], XtNforeground, callstruct->buttonptr->foreground);
      XtSetValues(callstruct->buttonptr->button, wargs, 1);
   
      XtAddCallback(callstruct->buttonptr->button, XtNselect, 
	  (XtCallbackProc)callstruct->buttonptr->buttoncall,
	  callstruct->buttonptr->dataptr);
   }

   XtDestroyWidget(callstruct->popup);
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

/*----------------------------------------------------------------------*/
/* Toggle button showing number of pages of output.			*/
/* Erase the filename and set everything accordingly.			*/
/*----------------------------------------------------------------------*/

void linkset(xcWidget button, propstruct *callstruct, caddr_t calldata)
{
   Arg wargs[1];

   free(xobjs.pagelist[areawin->page]->filename);
   xobjs.pagelist[areawin->page]->filename = (char *)malloc(1);
   xobjs.pagelist[areawin->page]->filename[0] = '\0';

   XwTextClearBuffer(callstruct->textw);
   getproptext(button, callstruct, calldata);

   /* Change the select button back to "Apply" */
   XtSetArg(wargs[0], XtNlabel, "Apply");
   XtSetValues(callstruct->buttonw, wargs, 1);

   /* Pop down the toggle button */
   XtUnmanageChild(button);
}

/*----------------------------------------------------------------------*/
/* Pull text from the popup prompt buffer into a global string variable */
/*----------------------------------------------------------------------*/

#ifndef XC_WIN32

void xcgettext(xcWidget button, popupstruct *callstruct, caddr_t calldata)
{
   if (callstruct->textw != NULL) {
      sprintf(_STR2, "%.249s", XwTextCopyBuffer(callstruct->textw)); 

      /* functions which use the file selector should look for directory name */
      /* in string rather than a file name, and take appropriate action.      */

      if (callstruct->filew != NULL) {
         if (lookdirectory(_STR2, 249)) {
	    newfilelist(callstruct->filew, callstruct);
	    return;
         }
      }
   }

   /* Pop down the widget now (for functions like execscript() which    */
   /* may want to interactively control the window contents), but do    */
   /* not destroy it until the function has returned.                   */

   XtPopdown(callstruct->popup);

   /* call the function which sets the variable according to type */
   /* This is in format (function)(calling-widget, ptr-to-data)   */

   (*(callstruct->setvalue))(callstruct->buttonptr->button,
	 callstruct->buttonptr->dataptr);

   if (callstruct->filew != NULL)
      newfilelist(callstruct->filew, callstruct);

   destroypopup(button, callstruct, calldata);
}

#endif

/*-------------------------------------------------------------------------*/
/* Grab text from the "output properties" window			   */
/*-------------------------------------------------------------------------*/

void getproptext(xcWidget button, propstruct *callstruct, caddr_t calldata)
{
   /* xobjs.pagelist[areawin->page]->filename can be realloc'd by the */
   /* call to *(callstruct->setvalue), so callstruct->dataptr may no    */
   /* longer be pointing to the data.					*/

   Arg wargs[1];
   short file_yes = (callstruct->setvalue == setfilename);

   sprintf(_STR2, "%.249s", XwTextCopyBuffer(callstruct->textw)); 
   (*(callstruct->setvalue))(button, callstruct->dataptr);

   /* special stuff for filename changes */

   if (file_yes) {
      char blabel[1024];
      short num_linked;
      xcWidget wrbutton, ltoggle;
      struct stat statbuf;

      /* get updated file information */

      if (strstr(xobjs.pagelist[areawin->page]->filename, ".") == NULL)
         sprintf(blabel, "%s.ps", xobjs.pagelist[areawin->page]->filename);
      else sprintf(blabel, "%s", xobjs.pagelist[areawin->page]->filename);
      if (stat(blabel, &statbuf) == 0) {
         sprintf(blabel, " Overwrite File ");
         if (beeper) XBell(dpy, 100);
         Wprintf("    Warning:  File exists");
      }
      else {
         sprintf(blabel, " Write File ");
         if (errno == ENOTDIR)
            Wprintf("Error:  Incorrect pathname");
         else if (errno == EACCES)
            Wprintf("Error:  Path not readable");
	 W3printf("  ");
      }

      wrbutton = XtNameToWidget(xcParent(button), "Write File");
      XtSetArg(wargs[0], XtNlabel, blabel);
      XtSetValues(wrbutton, wargs, 1);

      num_linked = pagelinks(areawin->page);
      if (num_linked > 1) {
	 ltoggle = XtNameToWidget(xcParent(button), "LToggle");
	 sprintf(blabel, "%d Pages", num_linked);
	 XtSetArg(wargs[0], XtNlabel, blabel);
	 XtSetValues(ltoggle, wargs, 1);
	 XtManageChild(ltoggle);
      }
   }

   /* topobject->name is not malloc'd, so is not changed by call to */
   /* *(callstruct->setvalue).					   */

   else if (callstruct->dataptr == topobject->name) {
      printname(topobject);
      renamepage(areawin->page);
   }

   /* Button title changes from "Apply" to "Okay" */

   XtSetArg(wargs[0], XtNlabel, "Okay");
   XtSetValues(callstruct->buttonw, wargs, 1);
}

/*----------------------------------------------------------------------*/
/* Update scale, width, and height in response to change of one of them	*/
/*----------------------------------------------------------------------*/

void updatetext(xcWidget button, xcWidgetList callstruct, caddr_t calldata)
{
   float oscale, psscale;
   char  edit[3][50];
   short i, n, posit;
   char  *pdptr;
   Arg	 wargs[2];
   int   width, height;

   /* auto-fit may override any changes to the scale */

   autoscale(areawin->page);
   writescalevalues(edit[0], edit[1], edit[2]);
   for (i = 0; i < 3; i++) {
      n = 0;
      XtnSetArg(XtNstring, edit[i]);
      pdptr = strchr(edit[i], '.');
      posit = (pdptr != NULL) ? (short)(pdptr - edit[i]) : strlen(edit[i]);
      XtnSetArg(XtNinsertPosition, posit);
      XtSetValues(callstruct[i + 2], wargs, n);
   }
}

/*-------------------------------------------------------------------------*/
/* Update the object name in response to a change in filename		   */
/*-------------------------------------------------------------------------*/

void updatename(xcWidget button, xcWidgetList callstruct, caddr_t calldata)
{
   short n, posit;
   char  *rootptr;
   Arg   wargs[2]; 
      
   if (strstr(topobject->name, "Page ") != NULL || strstr(topobject->name,
	"Page_") != NULL || topobject->name[0] == '\0') {

      rootptr = strrchr(xobjs.pagelist[areawin->page]->filename, '/');
      if (rootptr == NULL) rootptr = xobjs.pagelist[areawin->page]->filename;
      else rootptr++;

      sprintf(topobject->name, "%.79s", rootptr);
  
      n = 0;
      posit = strlen(topobject->name);
      XtnSetArg(XtNstring, topobject->name);
      XtnSetArg(XtNinsertPosition, posit);
      XtSetValues(callstruct[1], wargs, n);
      printname(topobject);
      renamepage(areawin->page);
   }
}

/*-------------------------------------------------------------------------*/
/* Create a popup window with "OK" and "Cancel" buttons,		   */
/* and text and label fields.						   */
/*-------------------------------------------------------------------------*/

#ifndef XC_WIN32

void popupprompt(xcWidget button, char *request, char *current, void (*function)(),
	buttonsave *datastruct, const char *filter)
{
    Arg         wargs[9];
    xcWidget      popup, dialog, okbutton, cancelbutton, entertext;
    xcWidget	staticarea;
    XWMHints	*wmhints;	/* for proper input focus */
    Position    xpos, ypos;
    short	n = 0;
    Dimension	height, width, areawidth, areaheight, bwidth, owidth;
    static char defaultTranslations[] = "<Key>Return:	execute()";
    popupstruct	*okaystruct;

    height = (current == NULL) ? ROWHEIGHT * 4 : ROWHEIGHT * 5;
    if (filter) height += LISTHEIGHT;

    width = XTextWidth(appdata.xcfont, request, strlen(request)) + 20;
    bwidth = XTextWidth(appdata.xcfont, "Cancel", strlen("Cancel")) + 50;
    owidth = XTextWidth(appdata.xcfont, "Okay", strlen("Okay")) + 50;
    if (width < 400) width = 400;

    XtnSetArg(XtNwidth, &areawidth);
    XtnSetArg(XtNheight, &areaheight);
    XtGetValues(areawin->area, wargs, n); n = 0;
    XtTranslateCoords(areawin->area, (Position) (areawidth / 2 - width 
	/ 2 + popups * 20), (Position) (areaheight / 2 - height / 2 +
	popups * 20), &xpos, &ypos);
    XtnSetArg(XtNx, xpos);
    XtnSetArg(XtNy, ypos);
    popup = XtCreatePopupShell("prompt", transientShellWidgetClass,
        button == NULL ? areawin->area : button, wargs, n); n = 0;
    popups++;

    XtnSetArg(XtNlayout, XwIGNORE);
    XtnSetArg(XtNwidth, width);
    XtnSetArg(XtNheight, height);
    dialog = XtCreateManagedWidget("dialog", XwbulletinWidgetClass,
        popup, wargs, n); n = 0;

    XtnSetArg(XtNx, 20);
    XtnSetArg(XtNy, ROWHEIGHT - 10 + (filter ? LISTHEIGHT : 0));
    XtnSetArg(XtNstring, request);
    XtnSetArg(XtNborderWidth, 0);
    XtnSetArg(XtNgravity, WestGravity);
    XtnSetArg(XtNfont, appdata.xcfont);
    staticarea = XtCreateManagedWidget("static", XwstaticTextWidgetClass,
	dialog, wargs, n); n = 0;

    XtnSetArg(XtNx, 20);
    XtnSetArg(XtNy, height - ROWHEIGHT - 10);
    XtnSetArg(XtNwidth, owidth); 
    XtnSetArg(XtNfont, appdata.xcfont);
    okbutton = XtCreateManagedWidget("Okay", XwmenuButtonWidgetClass, 
	dialog, wargs, n); n = 0;

    okaystruct = (popupstruct *) malloc(sizeof(popupstruct));
    okaystruct->buttonptr = datastruct;
    okaystruct->popup = popup;
    okaystruct->filter = (filter == NULL) ? NULL : strdup(filter);
    okaystruct->setvalue = function;
    okaystruct->textw = NULL;
    okaystruct->filew = NULL;

    XtnSetArg(XtNx, width - bwidth - 20);
    XtnSetArg(XtNy, height - ROWHEIGHT - 10);
    XtnSetArg(XtNwidth, bwidth);
    XtnSetArg(XtNfont, appdata.xcfont);
    cancelbutton = XtCreateManagedWidget("Cancel", XwmenuButtonWidgetClass, 
	   dialog, wargs, n); n = 0; 

    XtAddCallback(cancelbutton, XtNselect, (XtCallbackProc)destroypopup, okaystruct);

    /* Event handler for WM_DELETE_WINDOW message. */
    XtAddEventHandler(popup, NoEventMask, True, (XtEventHandler)delwin, okaystruct);

    if (current != NULL) {   /* A Text Edit widget is required */
       char		*pdptr;
       short		posit;

       XtnSetArg(XtNx, 20);
       XtnSetArg(XtNy, ROWHEIGHT + 10 + (filter ? LISTHEIGHT : 0));
       XtnSetArg(XtNheight, ROWHEIGHT + 5);
       XtnSetArg(XtNwidth, width - 40);
       XtnSetArg(XtNstring, current);
       pdptr = strchr(current, '.');
       posit = (pdptr != NULL) ? (short)(pdptr - current) : strlen(current);
       XtnSetArg(XtNinsertPosition, posit);
       XtnSetArg(XtNscroll, XwAutoScrollHorizontal);
       XtnSetArg(XtNwrap, XwWrapOff);
       XtnSetArg(XtNfont, appdata.textfont);
       entertext = XtCreateManagedWidget("Edit", XwtextEditWidgetClass,
	   dialog, wargs, n); n = 0; 

       okaystruct->textw = entertext;

       XtAddEventHandler(dialog, KeyPressMask, False,
	  (XtEventHandler)propevent, entertext);
       XtAddEventHandler(staticarea, KeyPressMask, False,
	  (XtEventHandler)propevent, entertext);
       XtOverrideTranslations(entertext, XtParseTranslationTable
	  (defaultTranslations));
       XtAddCallback(entertext, XtNexecute, (XtCallbackProc)xcgettext, okaystruct);

       /* Generate file prompting widget */

       if (filter) genfilelist(dialog, okaystruct, width);
    }
    XtAddCallback(okbutton, XtNselect, (XtCallbackProc)xcgettext, okaystruct);

    XtPopup(popup, XtGrabNone);

    /* set the input focus for the window */

    wmhints = XGetWMHints(dpy, xcWindow(popup));
    wmhints->flags |= InputHint;
    wmhints->input = True;
    XSetWMHints(dpy, xcWindow(popup), wmhints);
    XSetTransientForHint(dpy, xcWindow(popup), xcWindow(top));
    XFree(wmhints);

    if (current != NULL) XDefineCursor(dpy, xcWindow(entertext), 
	TEXTPTR);
}

#endif

/*-------------------------------------------------------------------------*/
/* Create a popup window for property changes				   */
/*-------------------------------------------------------------------------*/

#define MAXPROPS 7
#define MARGIN 15

propstruct okstruct[MAXPROPS], fpokstruct;

#ifndef XC_WIN32

void outputpopup(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   buttonsave  *savebutton;
   Arg         wargs[9];
   xcWidget      popup, dialog, okbutton, titlearea, wrbutton;
   xcWidget      fpentertext, fpokay, autobutton, allpages;
   xcWidgetList  staticarea, entertext, okays;
   XWMHints    *wmhints;	/* for proper input focus */
   short       num_linked;
   Position    xpos, ypos;
   short       n = 0;
   Dimension   height, width, areawidth, areaheight, bwidth, owidth, wwidth;
   Pagedata    *curpage;
   char	       *pdptr;
   short       posit, i;
   popupstruct *donestruct;
   void		(*function[MAXPROPS])();
   void		(*update[MAXPROPS])();
   char	statics[MAXPROPS][50], edit[MAXPROPS][75], request[150];
   char fpedit[75], outname[75], pstr[20];
   void	*data[MAXPROPS];
   struct stat statbuf;
   static char defaultTranslations[] = "<Key>Return:	execute()";

   if (is_page(topobject) == -1) {
      Wprintf("Can only save a top-level page!");
      return;
   }
   if (button == NULL) button = FileWriteXcircuitPSButton;
   savebutton = getgeneric(button, outputpopup, NULL); 

   curpage = xobjs.pagelist[areawin->page];

   sprintf(request, "PostScript output properties (Page %d):", 
	areawin->page + 1);
   sprintf(statics[0], "Filename:");
   sprintf(statics[1], "Page label:");
   sprintf(statics[2], "Scale:");
   if (curpage->coordstyle == CM) {
      sprintf(statics[3], "X Size (cm):");
      sprintf(statics[4], "Y Size (cm):");
   }
   else {
      sprintf(statics[3], "X Size (in):");
      sprintf(statics[4], "Y Size (in):");
   }
   sprintf(statics[5], "Orientation:");
   sprintf(statics[6], "Mode:");

   if (curpage->filename)
      sprintf(edit[0], "%s", curpage->filename);
   else
      sprintf(edit[0], "Page %d", areawin->page + 1);
   sprintf(edit[1], "%s", topobject->name);

   /* recompute bounding box and auto-scale, if set */
   calcbbox(areawin->topinstance);
   if (curpage->pmode & 2) autoscale(areawin->page);
   writescalevalues(edit[2], edit[3], edit[4]);
   sprintf(edit[5], "%s", (curpage->orient == 0) ? "Portrait" : "Landscape");
   sprintf(edit[6], "%s", (curpage->pmode & 1)
	? "Full page" : "Embedded (EPS)");
   function[0] = setfilename;
   function[1] = setpagelabel;
   function[2] = setfloat;
   function[3] = setscalex;
   function[4] = setscaley;
   function[5] = setorient;
   function[6] = setpmode;
   update[0] = updatename;
   update[1] = update[6] = NULL;
   update[2] = updatetext;
   update[3] = updatetext;
   update[4] = updatetext;
   update[5] = updatetext;
   data[0] = &(curpage->filename);
   data[1] = topobject->name;
   data[2] = data[3] = data[4] = &(curpage->outscale);
   data[5] = &(curpage->orient);
   data[6] = &(curpage->pmode);

   entertext = (xcWidgetList) XtMalloc (7 * sizeof (xcWidget));
   staticarea = (xcWidgetList) XtMalloc (7 * sizeof (xcWidget));
   okays = (xcWidgetList) XtMalloc (6 * sizeof (xcWidget));

   /* get file information */

   if (strstr(edit[0], ".") == NULL)
      sprintf(outname, "%s.ps", edit[0]);  
   else sprintf(outname, "%s", edit[0]);
   if (stat(outname, &statbuf) == 0) {
      sprintf(outname, "Overwrite File");
      Wprintf("  Warning:  File exists");
   }
   else {
      sprintf(outname, "Write File");
      if (errno == ENOTDIR)
	 Wprintf("Error:  Incorrect pathname");
      else if (errno == EACCES)
	 Wprintf("Error:  Path not readable");
      else
         W3printf("  ");
   }

   height = ROWHEIGHT * 17;  /* 3 + (2 * MAXPROPS) */
   width = XTextWidth(appdata.xcfont, request, strlen(request)) + 20;
   bwidth = XTextWidth(appdata.xcfont, "Close", strlen("Close")) + 50;
   owidth = XTextWidth(appdata.xcfont, "Apply", strlen("Apply")) + 50;
   wwidth = XTextWidth(appdata.xcfont, outname, strlen(outname)) + 80;
   if (width < 500) width = 500;

   XtnSetArg(XtNwidth, &areawidth);
   XtnSetArg(XtNheight, &areaheight);
   XtGetValues(areawin->area, wargs, n); n = 0;
   XtTranslateCoords(areawin->area, (Position) (areawidth / 2 - width 
	/ 2 + popups * 20), (Position) (areaheight / 2 - height / 2 +
	popups * 20), &xpos, &ypos);
   XtnSetArg(XtNx, xpos);
   XtnSetArg(XtNy, ypos);
   popup = XtCreatePopupShell("prompt", transientShellWidgetClass,
        areawin->area, wargs, n); n = 0;
   popups++;

   XtnSetArg(XtNlayout, XwIGNORE);
   XtnSetArg(XtNwidth, width);
   XtnSetArg(XtNheight, height);
   dialog = XtCreateManagedWidget("dialog", XwbulletinWidgetClass,
        popup, wargs, n); n = 0;

   XtnSetArg(XtNx, 20);
   XtnSetArg(XtNy, ROWHEIGHT - 10);
   XtnSetArg(XtNstring, request);
   XtnSetArg(XtNborderWidth, 0);
   XtnSetArg(XtNgravity, WestGravity);
   XtnSetArg(XtNbackground, colorlist[BARCOLOR].color.pixel);
   XtnSetArg(XtNfont, appdata.xcfont);
   titlearea = XtCreateManagedWidget("title", XwstaticTextWidgetClass,
	dialog, wargs, n); n = 0;

   XtnSetArg(XtNx, 20);
   XtnSetArg(XtNy, height - ROWHEIGHT - 10);
   XtnSetArg(XtNwidth, owidth); 
   XtnSetArg(XtNfont, appdata.xcfont);
   okbutton = XtCreateManagedWidget("Close", XwmenuButtonWidgetClass, 
	dialog, wargs, n); n = 0;

   XtnSetArg(XtNx, width - wwidth - 20);
   XtnSetArg(XtNy, height - ROWHEIGHT - 10);
   XtnSetArg(XtNwidth, wwidth);
   XtnSetArg(XtNfont, appdata.xcfont);
   XtnSetArg(XtNlabel, outname);
   wrbutton = XtCreateManagedWidget("Write File", XwmenuButtonWidgetClass,
	dialog, wargs, n); n = 0;

   for (i = 0; i < MAXPROPS; i++) {
      XtnSetArg(XtNx, 20);
      XtnSetArg(XtNy, ROWHEIGHT + MARGIN + 5 + (i * 2 * ROWHEIGHT));
      XtnSetArg(XtNstring, statics[i]);
      XtnSetArg(XtNborderWidth, 0);
      XtnSetArg(XtNgravity, WestGravity);
      XtnSetArg(XtNfont, appdata.xcfont);
      staticarea[i] = XtCreateManagedWidget("static", XwstaticTextWidgetClass,
	   dialog, wargs, n); n = 0;

      XtnSetArg(XtNx, 150);
      XtnSetArg(XtNy, ROWHEIGHT + MARGIN + (i * 2 * ROWHEIGHT));
      if (i < 5) {
         XtnSetArg(XtNheight, ROWHEIGHT + 5);
         XtnSetArg(XtNstring, edit[i]);
         XtnSetArg(XtNwidth, width - owidth - 190);
         pdptr = strchr(edit[i], '.');
         posit = (pdptr != NULL) ? (short)(pdptr - edit[i]) : strlen(edit[i]);
         XtnSetArg(XtNinsertPosition, posit);
         XtnSetArg(XtNscroll, XwAutoScrollHorizontal);
         XtnSetArg(XtNwrap, XwWrapOff);
         XtnSetArg(XtNfont, appdata.textfont);
         entertext[i] = XtCreateManagedWidget("Edit", XwtextEditWidgetClass,
	     dialog, wargs, n); n = 0; 

         XtnSetArg(XtNx, width - owidth - 20);
         XtnSetArg(XtNy, ROWHEIGHT + MARGIN + (i * 2 * ROWHEIGHT));
         XtnSetArg(XtNwidth, owidth); 
         XtnSetArg(XtNfont, appdata.xcfont);
         okays[i] = XtCreateManagedWidget("Apply", XwmenuButtonWidgetClass,
			dialog, wargs, n); n = 0;

         okstruct[i].textw = entertext[i];
	 okstruct[i].buttonw = okays[i];
         okstruct[i].setvalue = function[i];
         okstruct[i].dataptr = data[i];

         XtAddCallback(okays[i], XtNselect, (XtCallbackProc)getproptext, &okstruct[i]);
	 if (update[i] != NULL)
            XtAddCallback(okays[i], XtNselect, (XtCallbackProc)update[i], entertext);
         XtOverrideTranslations(entertext[i], XtParseTranslationTable
              	(defaultTranslations));
         XtAddCallback(entertext[i], XtNexecute, (XtCallbackProc)getproptext,
		&okstruct[i]);
         if (update[i] != NULL) XtAddCallback(entertext[i], XtNexecute,
		(XtCallbackProc)update[i], entertext);

      }
      else {
	 XtnSetArg(XtNlabel, edit[i]);
         XtnSetArg(XtNfont, appdata.xcfont);
         entertext[i] = XtCreateManagedWidget("Toggle", XwpushButtonWidgetClass,
	    dialog, wargs, n); n = 0;
	 XtAddCallback(entertext[i], XtNselect, (XtCallbackProc)function[i], data[i]);
	 if (update[i] != NULL)
            XtAddCallback(entertext[i], XtNselect, (XtCallbackProc)update[i], entertext);
      }
   }

   /* If this filename is linked to other pages (multi-page output), add a button */
   /* which will unlink the page name from the other pages when toggled.	  */

   num_linked = pagelinks(areawin->page);
   XtnSetArg(XtNx, width - wwidth - 20);
   XtnSetArg(XtNy, ROWHEIGHT - 10);
   XtnSetArg(XtNset, True);
   XtnSetArg(XtNsquare, True);
   XtnSetArg(XtNborderWidth, 0);
   XtnSetArg(XtNfont, appdata.xcfont);
   sprintf(pstr, "%d Pages", num_linked);
   XtnSetArg(XtNlabel, pstr);
   allpages = XtCreateWidget("LToggle", XwtoggleWidgetClass, dialog, wargs, n); n = 0;
   XtAddCallback(allpages, XtNrelease, (XtCallbackProc)linkset, &okstruct[0]);

   /* If full-page pmode is chosen, there is an additional text structure.
      Make this text structure always but allow it to be managed and
      unmanaged as necessary. */

   XtnSetArg(XtNx, 240);
   XtnSetArg(XtNy, ROWHEIGHT + MARGIN + (10 * ROWHEIGHT));
   XtnSetArg(XtNset, (curpage->pmode & 2) ? True : False);
   XtnSetArg(XtNsquare, True);
   XtnSetArg(XtNborderWidth, 0);
   XtnSetArg(XtNfont, appdata.xcfont);
   autobutton = XtCreateWidget("Auto-fit", XwtoggleWidgetClass,
       dialog, wargs, n); n = 0;

   if (curpage->coordstyle == CM) {
      sprintf(fpedit, "%3.2f x %3.2f cm",
	 (float)curpage->pagesize.x / IN_CM_CONVERT,
	 (float)curpage->pagesize.y / IN_CM_CONVERT);
   }
   else {
      sprintf(fpedit, "%3.2f x %3.2f in",
	 (float)curpage->pagesize.x / 72.0,
	 (float)curpage->pagesize.y / 72.0);
   }
   XtnSetArg(XtNx, 240);
   XtnSetArg(XtNy, ROWHEIGHT + MARGIN + (12 * ROWHEIGHT));
   XtnSetArg(XtNheight, ROWHEIGHT + 5);
   XtnSetArg(XtNstring, fpedit);
   XtnSetArg(XtNwidth, width - owidth - 280);
   pdptr = strchr(fpedit, '.');
   posit = (pdptr != NULL) ? (short)(pdptr - fpedit) : strlen(fpedit);
   XtnSetArg(XtNscroll, XwAutoScrollHorizontal);
   XtnSetArg(XtNwrap, XwWrapOff);
   XtnSetArg(XtNfont, appdata.textfont);
   XtnSetArg(XtNinsertPosition, posit);
   fpentertext = XtCreateWidget("fpedit", XwtextEditWidgetClass,
       dialog, wargs, n); n = 0;

   XtnSetArg(XtNx, width - owidth - 20);
   XtnSetArg(XtNy, ROWHEIGHT + MARGIN + (12 * ROWHEIGHT));
   XtnSetArg(XtNwidth, owidth);
   XtnSetArg(XtNfont, appdata.xcfont);
   XtnSetArg(XtNlabel, "Apply");
   fpokay = XtCreateWidget("fpokay", XwmenuButtonWidgetClass,
      dialog, wargs, n); n = 0;

   fpokstruct.textw = fpentertext;
   fpokstruct.buttonw = fpokay;
   fpokstruct.setvalue = setpagesize;
   fpokstruct.dataptr = &(curpage->pagesize);

   XtAddCallback(fpokay, XtNselect, (XtCallbackProc)getproptext, &fpokstruct);
   XtAddCallback(fpokay, XtNselect, (XtCallbackProc)updatetext, entertext);
   XtOverrideTranslations(fpentertext, XtParseTranslationTable
        (defaultTranslations));
   XtAddCallback(fpentertext, XtNexecute, (XtCallbackProc)getproptext, &fpokstruct);
   XtAddCallback(fpentertext, XtNexecute, (XtCallbackProc)updatetext, entertext);
   XtAddCallback(autobutton, XtNselect, (XtCallbackProc)autoset, entertext);
   XtAddCallback(autobutton, XtNrelease, (XtCallbackProc)autostop, NULL);

   if (curpage->pmode & 1) {
      XtManageChild(fpentertext);
      XtManageChild(fpokay);
      XtManageChild(autobutton);
   }

   if (num_linked > 1) {
      XtManageChild(allpages);
   }

   /* end of pagesize extra Widget definitions */

   donestruct = (popupstruct *) malloc(sizeof(popupstruct));
   donestruct->popup = popup;
   donestruct->buttonptr = savebutton;
   donestruct->filter = NULL;
   XtAddCallback(okbutton, XtNselect, (XtCallbackProc)destroypopup, donestruct);

   /* Send setfile() the widget entertext[0] in case because user sometimes
      forgets to type "okay" but buffer contains the expected filename */

   XtAddCallback(wrbutton, XtNselect, (XtCallbackProc)setfile, entertext[0]);

   /* Begin Popup */

   XtPopup(popup, XtGrabNone);

   /* set the input focus for the window */

   wmhints = XGetWMHints(dpy, xcWindow(popup));
   wmhints->flags |= InputHint;
   wmhints->input = True;
   XSetWMHints(dpy, xcWindow(popup), wmhints);
   XSetTransientForHint(dpy, xcWindow(popup), xcWindow(top));
   XFree(wmhints);

   for (i = 0; i < 5; i++)
      XDefineCursor(dpy, xcWindow(entertext[i]), TEXTPTR);
}

#endif

/*-------------------------------------------------*/
/* Print a string to the message widget. 	   */ 
/* Note: Widget message must be a global variable. */
/* For formatted strings, format first into _STR   */
/*-------------------------------------------------*/

void clrmessage(XtPointer clientdata, xcIntervalId *id)
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

/*----------------------------------------------------------------------*/
/* This is the non-Tcl version of tcl_vprintf()				*/
/*----------------------------------------------------------------------*/

void xc_vprintf(xcWidget widget, const char *fmt, va_list args_in)
{
   va_list args;
   static char outstr[128];
   char *outptr, *bigstr = NULL;
   int nchars;
   Arg	wargs[1];

   outstr[0] = '\0';
   outptr = outstr;

   /* This mess circumvents problems with systems which do not have	*/
   /* va_copy() defined.  Some define __va_copy();  otherwise we must	*/
   /* assume that args = args_in is valid.				*/

   va_copy(args, args_in);
   nchars = vsnprintf(outptr, 127, fmt, args);
   va_end(args);

   if (nchars >= 127) {
      va_copy(args, args_in);
      bigstr = (char *)malloc(nchars + 2);
      bigstr[0] = '\0';
      outptr = bigstr;
      vsnprintf(outptr, nchars + 2, fmt, args);
      va_end(args);
   }
   else if (nchars == -1) nchars = 126;

   XtSetArg(wargs[0], XtNstring, outptr);
   XtSetValues(widget, wargs, 1);
   XtSetArg(wargs[0], XtNheight, ROWHEIGHT);
   XtSetValues(widget, wargs, 1);

   if (bigstr != NULL) free(bigstr);

   if (widget == message3) {
      if (printtime_id != 0) {
         xcRemoveTimeOut(printtime_id);
         printtime_id = 0;
      }
   }

   /* 10 second timeout */
   if (widget == message3) {
      printtime_id = xcAddTimeOut(app, 10000, clrmessage, NULL);
   }
}
    
/*------------------------------------------------------------------------------*/
/* W3printf is the same as Wprintf because the non-Tcl based version does not	*/
/* duplicate output to stdout/stderr.						*/
/*------------------------------------------------------------------------------*/

void W3printf(char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  xc_vprintf(message3, format, ap);
  va_end(ap);
}

/*------------------------------------------------------------------------------*/

void Wprintf(char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  xc_vprintf(message3, format, ap);
  va_end(ap);
}

/*------------------------------------------------------------------------------*/

void W1printf(char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  xc_vprintf(message1, format, ap);
  va_end(ap);
}   

/*------------------------------------------------------------------------------*/

void W2printf(char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  xc_vprintf(message2, format, ap);
  va_end(ap);
}   

/*------------------------------------------------------------------------------*/

#ifndef XC_WIN32

void getcommand(xcWidget cmdw, caddr_t clientdata, caddr_t calldata)
{
   sprintf(_STR2, "%.249s", XwTextCopyBuffer(cmdw));
#ifdef HAVE_PYTHON
   execcommand(0, _STR2 + 4);
#else
   execcommand(0, _STR2 + 2);
#endif
   XtRemoveEventHandler(areawin->area, KeyPressMask, False,
	(XtEventHandler)propevent, cmdw);
   XtAddCallback(areawin->area, XtNkeyDown, (XtCallbackProc)keyhandler, NULL);
   XtAddCallback(areawin->area, XtNkeyUp, (XtCallbackProc)keyhandler, NULL);
   XtUnmanageChild(cmdw);
}

/*------------------------------------------------------------------------------*/
/* "docommand" overlays the message3 widget temporarily with a TextEdit widget. */
/*------------------------------------------------------------------------------*/

void docommand()
{
   Arg wargs[12];
   int n = 0;
   Dimension w;
   Position x, y;
   static char defaultTranslations[] = "<Key>Return:   execute()";

   if (overlay == NULL) {
      XtnSetArg(XtNy, &y);
      XtnSetArg(XtNx, &x);
      XtnSetArg(XtNwidth, &w);
      XtGetValues(message3, wargs, n); n = 0;

      XtnSetArg(XtNy, y);
      XtnSetArg(XtNx, x);
      XtnSetArg(XtNheight, ROWHEIGHT);
      XtnSetArg(XtNwidth, w);

      XtnSetArg(XtNfont, appdata.xcfont);
      XtnSetArg(XtNwrap, XwWrapOff);
      overlay = XtCreateManagedWidget("Command", XwtextEditWidgetClass,
	   top, wargs, n); n = 0;

      XtOverrideTranslations(overlay, XtParseTranslationTable(defaultTranslations));
      XtAddCallback(overlay, XtNexecute, (XtCallbackProc)getcommand, NULL);
   }
   else {
      XtManageChild(overlay);
   }

   XwTextClearBuffer(overlay);
#ifdef HAVE_PYTHON
   XwTextInsert(overlay, ">>> ");
#else
   XwTextInsert(overlay, "? ");
#endif

   /* temporarily redirect all text into the overlay widget */

   XtRemoveCallback(areawin->area, XtNkeyDown, (XtCallbackProc)keyhandler, NULL);
   XtRemoveCallback(areawin->area, XtNkeyUp, (XtCallbackProc)keyhandler, NULL);
   XtAddEventHandler(areawin->area, KeyPressMask, False,
	(XtEventHandler)propevent, overlay);
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

   if (areawin->area != (xcWidget)NULL) {
      if (xcIsRealized(areawin->area)) {
         xcWidget colormenu = xcParent(ColorAddNewColorButton);

         XSetWindowColormap(dpy, xcWindow(top), cmap);
         XSetWindowColormap(dpy, areawin->window, cmap);
         if (colormenu != (xcWidget)NULL)
            XSetWindowColormap(dpy, xcWindow(colormenu), cmap);
      }
   }
   return(1);
}

#endif

/*----------------------------------------------------------------------*/
/* Event handler for input focus					*/
/*----------------------------------------------------------------------*/

#ifdef INPUT_FOCUS

void mappinghandler(xcWidget w, caddr_t clientdata, XEvent *event)
{
   if (!xcIsRealized(w)) return;
   switch(event->type) {
      case MapNotify:
	 /* Fprintf(stderr, "Window top was mapped.  Setting input focus\n"); */
	 areawin->mapped = True;
         XSetInputFocus(dpy, xcWindow(w), RevertToPointerRoot, CurrentTime);  
	 break;
      case UnmapNotify:
	 /* Fprintf(stderr, "Window top was unmapped\n"); */
	 areawin->mapped = False;
	 break;
   }
}

#endif

/*----------------------------------------------------------------------*/

void clientmessagehandler(xcWidget w, caddr_t clientdata, XEvent *event)
{
   if (!xcIsRealized(w)) return;

   if (event->type == ClientMessage) {
      if (render_client(event) == False)
	 return;

#ifdef INPUT_FOCUS
      if (areawin->mapped == True) {
         /* Fprintf(stderr, "Forcing input focus\n"); */
         XSetInputFocus(dpy, xcWindow(w), RevertToPointerRoot, CurrentTime);  
      }
#endif
   }
}

/*----------------------------------------------------------------------*/
/* Event handler for WM_DELETE_WINDOW message.                          */
/*----------------------------------------------------------------------*/

void delwin(xcWidget w, popupstruct *bstruct, XClientMessageEvent *event)
{
   if (event->type != ClientMessage)
      return;

   if((event->message_type == wprot) && (event->data.l[0] == wmprop[0])) {
      if (w == top)
	 quitcheck(w, NULL, NULL);
      else
         destroypopup(w, bstruct, NULL);
   }
}

/*----------------------------------------------------------------------*/
/* GUI_init() --- generate the widget structures, allocate colormaps	*/
/* and graphics contexts, generate menus and toolbar, and assign	*/
/* callback functions and event handlers.				*/
/*----------------------------------------------------------------------*/

#ifdef XC_WIN32

XCWindowData* GUI_init(int argc, char *argv[]);

#else

XCWindowData *GUI_init(int argc, char *argv[])
{

#ifdef HAVE_XPM
   xcWidget abform;
#endif
   xcWidget form, aform, firstbutton, lastbutton, corner;
   XGCValues    values;
   XWMHints	*wmhints;	/* for proper input focus */
   Arg		wargs[12];
   Pixmap	icon;
   Window	win;
   XCWindowData *newwin;
   short i, n = 0;

   /* Things to do the first time around */
   if (dpy == NULL) {
      
      char *argfb[] = {			/* Fallback resources */
	"xcircuit*foreground : brown4", /* These are the values that 	*/
	"xcircuit*background : beige",  /* not or cannot be explicitly	*/
	"xcircuit.foreground : black",	/* initialized by		*/
	"xcircuit.background : white",  /* XtGetApplicationResources()	*/
	"xcircuit*borderWidth : 2",	/* below.			*/
	"xcircuit*borderColor : Red",
	NULL				/* Sentinel */
      };

      XtSetLanguageProc(NULL, NULL, NULL);

      /*-------------------------------------------------------------------*/
      /* Set pointer to own XDefaults file, but allow an external override */
      /*-------------------------------------------------------------------*/

#ifdef HAVE_PUTENV
      if (getenv("XAPPLRESDIR") == NULL)
         putenv("XAPPLRESDIR=" RESOURCES_DIR);
#else
      setenv("XAPPLRESDIR", RESOURCES_DIR, 0);
#endif

      /*-----------------------------*/
      /* Create the widget hierarchy */
      /*-----------------------------*/

      top = XtOpenApplication(&app, "XCircuit", NULL, 0, &argc, argv,
		argfb, applicationShellWidgetClass, NULL, 0);

      dpy = XtDisplay(top);
      win = DefaultRootWindow(dpy);
      cmap = DefaultColormap(dpy, DefaultScreen(dpy));

      /*-------------------------*/
      /* Create stipple patterns */
      /*-------------------------*/

      for (i = 0; i < STIPPLES; i++)
         STIPPLE[i] = XCreateBitmapFromData(dpy, win, STIPDATA[i], 4, 4);

      /*----------------------------------------*/
      /* Allocate space for the basic color map */
      /*----------------------------------------*/

      number_colors = NUMBER_OF_COLORS;
      colorlist = (colorindex *)malloc(NUMBER_OF_COLORS * sizeof(colorindex));

      /*-----------------------------------------------------------*/
      /* Xw must add these translations for the popup manager      */
      /*-----------------------------------------------------------*/
      XwAppInitialize(app);

      /*-------------------------------*/
      /* Get the application resources */
      /*-------------------------------*/

      XtAppAddConverter(app, XtRString, XtRPixel, (XtConverter)CvtStringToPixel,
		NULL, 0);
      XtGetApplicationResources(top, &appdata, resources, XtNumber(resources), 
		NULL, 0);
   }
   else {	/* Not the first time---display has already been opened */

      top = XtAppCreateShell("XCircuit", "XCircuit", applicationShellWidgetClass,
		dpy, NULL, 0);
   }

   n = 0;
   XtnSetArg(XtNwidth, appdata.width);
   XtnSetArg(XtNheight, appdata.height);
   XtnSetArg(XtNforeground, appdata.fg);
   XtnSetArg(XtNbackground, appdata.bg);
   XtnSetArg(XtNcolormap, cmap);
   XtSetValues(top, wargs, n); n = 0;

   form = XtCreateManagedWidget("Form", XwformWidgetClass, top, NULL, 0);

   /* Generate a new window data structure */

   newwin = create_new_window();

   /* Set up the buttons and Graphics drawing area */

   createmenus(form, &firstbutton, &lastbutton);

   XtnSetArg(XtNxRefWidget, lastbutton);
   XtnSetArg(XtNyRefWidget, form);
   XtnSetArg(XtNxAddWidth, True);
   XtnSetArg(XtNxAttachRight, True);
   XtnSetArg(XtNheight, ROWHEIGHT);
   sprintf(_STR, "   Welcome to Xcircuit Version %2.1f", PROG_VERSION);
   XtnSetArg(XtNstring, _STR);
   XtnSetArg(XtNxResizable, True);
   XtnSetArg(XtNgravity, WestGravity);
   XtnSetArg(XtNfont, appdata.xcfont);
   XtnSetArg(XtNwrap, False);
   XtnSetArg(XtNstrip, False);
   message1 = XtCreateManagedWidget("Message1", XwstaticTextWidgetClass,
	form, wargs, n); n = 0;

#ifdef HAVE_XPM
   /*-------------------------------------------------------------------*/
   /* An extra form divides the main window from the toolbar		*/
   /*-------------------------------------------------------------------*/

   XtnSetArg(XtNyRefWidget, firstbutton);
   XtnSetArg(XtNyAddHeight, True);
   XtnSetArg(XtNyOffset, 2);
   XtnSetArg(XtNxAttachRight, True);
   XtnSetArg(XtNyResizable, True);
   XtnSetArg(XtNxResizable, True);
   XtnSetArg(XtNborderWidth, 0);
   abform = XtCreateManagedWidget("ABForm", XwformWidgetClass, form, wargs, n);
   n = 0;

   /*-------------------------------------------------------------------*/
   /* The main window and its scrollbars rest in a separate form window */
   /*-------------------------------------------------------------------*/

   XtnSetArg(XtNyResizable, True);
   XtnSetArg(XtNxResizable, True);
   XtnSetArg(XtNyAttachBottom, True);
   aform = XtCreateManagedWidget("AForm", XwformWidgetClass, abform, wargs, n);
   n = 0;
#else
#define abform aform

   /*-------------------------------------------------------------------*/
   /* The main window and its scrollbars rest in a separate form window */
   /*-------------------------------------------------------------------*/

   XtnSetArg(XtNyRefWidget, firstbutton);
   XtnSetArg(XtNyAddHeight, True);
   XtnSetArg(XtNxAttachRight, True);
   XtnSetArg(XtNyResizable, True);
   XtnSetArg(XtNxResizable, True);
   aform = XtCreateManagedWidget("AForm", XwformWidgetClass, form, wargs, n);
   n = 0;

#endif

   /*------------------------*/
   /* add scrollbar widget   */
   /*------------------------*/

   XtnSetArg(XtNxResizable, False);
   XtnSetArg(XtNyResizable, True);
   XtnSetArg(XtNwidth, SBARSIZE);
   XtnSetArg(XtNborderWidth, 1);
   newwin->scrollbarv = XtCreateManagedWidget("SBV", XwworkSpaceWidgetClass, 
	aform, wargs, n); n = 0;
   
   /*----------------------------------------------------------*/
   /* A button in the scrollbar corner for the sake of beauty. */
   /*----------------------------------------------------------*/

   XtnSetArg(XtNyRefWidget, newwin->scrollbarv);
   XtnSetArg(XtNyAddHeight, True);
   XtnSetArg(XtNyAttachBottom, True);
   XtnSetArg(XtNheight, SBARSIZE);
   XtnSetArg(XtNwidth, SBARSIZE);
   XtnSetArg(XtNborderWidth, 1);
   XtnSetArg(XtNlabel, "");
   corner = XtCreateManagedWidget("corner", XwpushButtonWidgetClass,
        aform, wargs, n); n = 0;

   /*-------------------------*/
   /* The main drawing window */
   /*-------------------------*/

   XtnSetArg(XtNxOffset, SBARSIZE);
   XtnSetArg(XtNxResizable, True);
   XtnSetArg(XtNyResizable, True);
   XtnSetArg(XtNxAttachRight, True);
   newwin->area = XtCreateManagedWidget("Area", XwworkSpaceWidgetClass, 
	aform, wargs, n); n = 0;

   /*-------------------------*/
   /* and the other scrollbar */
   /*-------------------------*/

   XtnSetArg(XtNyRefWidget, newwin->area);
   XtnSetArg(XtNyAddHeight, True);
   XtnSetArg(XtNxRefWidget, newwin->scrollbarv);
   XtnSetArg(XtNxAddWidth, True);
   XtnSetArg(XtNxAttachRight, True);
   XtnSetArg(XtNheight, SBARSIZE);
   XtnSetArg(XtNyResizable, False);
   XtnSetArg(XtNxResizable, True);
   XtnSetArg(XtNborderWidth, 1);
   newwin->scrollbarh = XtCreateManagedWidget("SBH", XwworkSpaceWidgetClass, 
	aform, wargs, n); n = 0;

   /*------------------------------------------------*/
   /* Supplementary message widgets go at the bottom */
   /*------------------------------------------------*/

   XtnSetArg(XtNxResizable, False);
   XtnSetArg(XtNyRefWidget, abform);
   XtnSetArg(XtNyAddHeight, True);
   XtnSetArg(XtNyOffset, -5);
   XtnSetArg(XtNheight, ROWHEIGHT);
   XtnSetArg(XtNfont, appdata.xcfont);
   XtnSetArg(XtNforeground, appdata.buttonpix);
   XtnSetArg(XtNbackground, appdata.buttonpix);
   wsymb = XtCreateWidget("Symbol", XwpushButtonWidgetClass,
	form, wargs, n); n = 0;
   XtManageChild(wsymb);

   XtnSetArg(XtNxRefWidget, wsymb);
   XtnSetArg(XtNxAddWidth, True);
   XtnSetArg(XtNxResizable, False);
   XtnSetArg(XtNyRefWidget, abform);
   XtnSetArg(XtNyAddHeight, True);
   XtnSetArg(XtNyOffset, -5);
   XtnSetArg(XtNheight, ROWHEIGHT);
   XtnSetArg(XtNfont, appdata.xcfont);
   XtnSetArg(XtNforeground, appdata.bg);
   XtnSetArg(XtNbackground, appdata.snappix);
   wschema = XtCreateWidget("Schematic", XwpushButtonWidgetClass,
	form, wargs, n); n = 0;
   XtManageChild(wschema);


   XtnSetArg(XtNxRefWidget, wschema);
   XtnSetArg(XtNxAddWidth, True);

   XtnSetArg(XtNyRefWidget, abform);
   XtnSetArg(XtNyAddHeight, True);
   XtnSetArg(XtNyOffset, -5);
   XtnSetArg(XtNheight, ROWHEIGHT);
   XtnSetArg(XtNstring, "Editing: Page 1");
   XtnSetArg(XtNxResizable, False);
   XtnSetArg(XtNgravity, WestGravity);
   XtnSetArg(XtNfont, appdata.xcfont);
   XtnSetArg(XtNwrap, False);
   message2 = XtCreateManagedWidget("Message2", XwstaticTextWidgetClass,
	form, wargs, n); n = 0;

   XtnSetArg(XtNyRefWidget, abform);
   XtnSetArg(XtNyAddHeight, True);
   XtnSetArg(XtNyOffset, -5);
   XtnSetArg(XtNxAttachRight, True);
   XtnSetArg(XtNxRefWidget, message2);
   XtnSetArg(XtNxAddWidth, True);
   XtnSetArg(XtNheight, ROWHEIGHT);
   XtnSetArg(XtNxResizable, True);
   XtnSetArg(XtNfont, appdata.xcfont);
   XtnSetArg(XtNwrap, False);
   XtnSetArg(XtNgravity, WestGravity);
   XtnSetArg(XtNstring, "Don't Panic");
   message3 = XtCreateManagedWidget("Message3", XwstaticTextWidgetClass,
	form, wargs, n); n = 0;

   /*-------------------------------*/
   /* optional Toolbar on the right */
   /*-------------------------------*/

#ifdef HAVE_XPM
   createtoolbar(abform, aform);
   XtAddCallback(newwin->area, XtNresize, (XtCallbackProc)resizetoolbar, NULL);
#endif

   /* Setup callback routines for the area widget */
   /* Use Button1Press event to add the callback which tracks motion;  this */
   /*   will reduce the number of calls serviced during normal operation */ 

   XtAddCallback(newwin->area, XtNexpose, (XtCallbackProc)drawarea, NULL);
   XtAddCallback(newwin->area, XtNresize, (XtCallbackProc)resizearea, NULL);

   XtAddCallback(newwin->area, XtNselect, (XtCallbackProc)buttonhandler, NULL);
   XtAddCallback(newwin->area, XtNrelease, (XtCallbackProc)buttonhandler, NULL);
   XtAddCallback(newwin->area, XtNkeyDown, (XtCallbackProc)keyhandler, NULL);
   XtAddCallback(newwin->area, XtNkeyUp, (XtCallbackProc)keyhandler, NULL);

   XtAddEventHandler(newwin->area, Button1MotionMask | Button2MotionMask,
		False, (XtEventHandler)xlib_drag, NULL);

   /* Setup callback routines for the scrollbar widgets */

   XtAddEventHandler(newwin->scrollbarh, ButtonMotionMask, False,
	(XtEventHandler)panhbar, NULL);
   XtAddEventHandler(newwin->scrollbarv, ButtonMotionMask, False,
	(XtEventHandler)panvbar, NULL);

   XtAddCallback(newwin->scrollbarh, XtNrelease, (XtCallbackProc)endhbar, NULL);
   XtAddCallback(newwin->scrollbarv, XtNrelease, (XtCallbackProc)endvbar, NULL);

   XtAddCallback(newwin->scrollbarh, XtNexpose, (XtCallbackProc)drawhbar, NULL);
   XtAddCallback(newwin->scrollbarv, XtNexpose, (XtCallbackProc)drawvbar, NULL);
   XtAddCallback(newwin->scrollbarh, XtNresize, (XtCallbackProc)drawhbar, NULL);
   XtAddCallback(newwin->scrollbarv, XtNresize, (XtCallbackProc)drawvbar, NULL);

   /* Event handler for WM_DELETE_WINDOW message. */
   XtAddEventHandler(top, NoEventMask, True, (XtEventHandler)delwin, NULL);

   XtAddCallback(corner, XtNselect, (XtCallbackProc)zoomview, Number(1));
   XtAddCallback (wsymb, XtNselect, (XtCallbackProc)xlib_swapschem, Number(0));
   XtAddCallback (wschema, XtNselect, (XtCallbackProc)xlib_swapschem, Number(0));

   /*--------------------*/
   /* Realize the Widget */
   /*--------------------*/

   areawin = newwin;
   XtRealizeWidget(top);

   wprot = XInternAtom(dpy, "WM_PROTOCOLS", False);
   wmprop[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
   wmprop[1] = XInternAtom(dpy, "WM_TAKE_FOCUS", False); 
   XSetWMProtocols(dpy, xcWindow(top), wmprop, 2);

   /*----------------------------------------------------*/
   /* Let the window manager set the input focus for the */
   /* window and inform the window manager of the icon   */
   /* pixmap (which may or may not be useful, depending  */
   /* on the particular window manager).		 */
   /*----------------------------------------------------*/

   wmhints = XGetWMHints(dpy, xcWindow(top));
   wmhints->input = True;

#ifdef HAVE_XPM
   /* Create the xcircuit icon pixmap */
   XpmCreatePixmapFromData(dpy, win, xcircuit_xpm, &icon, NULL, NULL);

   wmhints->flags |= InputHint | IconPixmapHint;
   wmhints->icon_pixmap = icon;
#else
   wmhints->flags |= InputHint;
#endif

   XSetWMHints(dpy, xcWindow(top), wmhints);
   XFree(wmhints);

/* Don't know why this is necessary, but otherwise keyboard input focus    */
/* is screwed up under the WindowMaker window manager and possibly others. */

#ifdef INPUT_FOCUS
   XtAddEventHandler(top, SubstructureNotifyMask,
	TRUE, (XtEventHandler)mappinghandler, NULL);
#endif

   XtAddEventHandler(top, NoEventMask, TRUE,
	(XtEventHandler)clientmessagehandler, NULL);

   /* Set the area widget width and height, center userspace (0, 0) on screen */
   XtSetArg(wargs[0], XtNwidth, &newwin->width);
   XtSetArg(wargs[1], XtNheight, &newwin->height);
   XtGetValues(newwin->area, wargs, 2);

   /*---------------------------------------------------*/
   /* Define basic display variables 			*/
   /* Redefine win to be just the drawing area window   */
   /*---------------------------------------------------*/

   newwin->window = xcWindow(newwin->area);

   /*-----------------------------*/
   /* Create the Graphics Context */
   /*-----------------------------*/

   values.foreground = BlackPixel(dpy, DefaultScreen(dpy));
   values.background = WhitePixel(dpy, DefaultScreen(dpy));
   values.font = appdata.xcfont->fid;

   newwin->gc = XCreateGC(dpy, newwin->window,
		GCForeground | GCBackground | GCFont, &values);

#ifdef HAVE_CAIRO
   newwin->surface = cairo_xlib_surface_create(dpy, newwin->window,
         DefaultVisual(dpy, 0), newwin->width, newwin->height);
   newwin->cr = cairo_create(newwin->surface);
#else
   newwin->clipmask = XCreatePixmap(dpy, newwin->window, newwin->width,
		newwin->height, 1); 

   values.foreground = 0;
   values.background = 0;
   newwin->cmgc = XCreateGC(dpy, newwin->clipmask, GCForeground
		| GCBackground, &values);
#endif /* HAVE_CAIRO */

   return newwin;
}

#endif

/*----------------------------------------------------------------------*/
/* When not using ToolScript, this is the standard X loop (XtMainLoop())*/
/*----------------------------------------------------------------------*/

int local_xloop()
{
   XtAppMainLoop(app);
   return EXIT_SUCCESS;
}

/*----------------------------------------------------------------------*/
/* Main entry point when used as a standalone program			*/
/*----------------------------------------------------------------------*/

int main(int argc, char **argv)
{
   char  *argv0;		/* find root of argv[0] */
   short initargc = argc;	/* because XtInitialize() absorbs the     */
				/* -schem flag and renumbers argc! (bug?) */
   short k = 0;

   /*-----------------------------------------------------------*/
   /* Find the root of the command called from the command line */
   /*-----------------------------------------------------------*/

   argv0 = strrchr(argv[0], '/');
   if (argv0 == NULL)
      argv0 = argv[0];
   else
      argv0++;

   pre_initialize();

   /*---------------------------*/
   /* Check for schematic flag  */
   /*---------------------------*/

   for (k = argc - 1; k > 0; k--) {
      if (!strncmp(argv[k], "-2", 2)) {
	 pressmode = 1;		/* 2-button mouse indicator */
	 break;
      }
   }

   areawin = GUI_init(argc, argv);
   post_initialize();

   /*-------------------------------------*/
   /* Initialize the ghostscript renderer */
   /*-------------------------------------*/

   ghostinit();

   /*----------------------------------------------------------*/
   /* Check home directory for initial settings	& other loads; */
   /* Load the (default) built-in set of objects 	       */
   /*----------------------------------------------------------*/

#ifdef HAVE_PYTHON
   init_interpreter();
#endif

   loadrcfile();
   pressmode = 0;	/* Done using this to mark 2-button mouse mode */

   composelib(PAGELIB);	/* make sure we have a valid page list */
   composelib(LIBLIB);	/* and library directory */

   /*----------------------------------------------------*/
   /* Parse the command line for initial file to load.   */
   /* Otherwise, look for possible crash-recovery files. */
   /*----------------------------------------------------*/

   if (argc == 2 + (k != 0) || initargc == 2 + (k != 0)) {
      strcpy(_STR2, argv[(k == 1) ? 2 : 1]);
      startloadfile(-1);  /* change the argument to load into library other
			     than the User Library */
   }
   else {
      findcrashfiles();
   }

   xobjs.suspend = -1;
   return local_xloop();   /* No return---exit through quit() callback */
}

/*----------------------------------------------------------------------*/
#endif /* !TCL_WRAPPER */
