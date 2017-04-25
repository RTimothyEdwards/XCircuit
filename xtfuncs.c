/*----------------------------------------------------------------------*/
/* xtfuncs.c --- Functions associated with the XCircuit Xt GUI		*/
/*	(no Tcl/Tk interpreter)						*/
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	*/
/*----------------------------------------------------------------------*/

#ifndef TCL_WRAPPER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

#ifndef XC_WIN32
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>

#include "Xw/Xw.h"
#include "Xw/MenuBtn.h"
#include "Xw/PopupMgr.h"
#endif

/*----------------------------------------------------------------------*/
/* Local includes							*/
/*----------------------------------------------------------------------*/

#include "colordefs.h"
#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*----------------------------------------------------------------------*/
/* External Variable definitions					*/
/*----------------------------------------------------------------------*/

extern char	 _STR2[250];
extern char	 _STR[150];          /* Generic multipurpose string */
extern xcWidget	 top;
extern Display   *dpy;
extern Globaldata xobjs;
extern XCWindowData *areawin;
extern int	  number_colors;
extern colorindex *colorlist;
extern ApplicationData appdata;
extern Cursor	  appcursors[NUM_CURSORS];
extern fontinfo *fonts;
extern short fontcount;
extern xcWidget    menuwidgets[];
extern xcWidget	 toolbar;
extern short	 popups;	     /* total number of popup windows */

/*----------------------------------------------------------------------*/
/* The rest of the local includes depend on the prototype declarations	*/
/* and some of the external global variable declarations.		*/
/*----------------------------------------------------------------------*/

#include "menus.h"
#include "menudep.h"

/*----------------------------------------------------------------------*/
/* External variable definitions					*/
/*----------------------------------------------------------------------*/

extern u_short *fontnumbers;
extern u_char nfontnumbers;

/*----------------------------------------------*/
/* Set Poly and Arc line styles and fill styles */
/*----------------------------------------------*/

#define BORDERS  (NOBORDER | DOTTED | DASHED)
#define ALLFILLS (FILLSOLID | FILLED)

/*--------------------------------------------------------------*/
/* Menu toggle button functions (keeps Xt functions out of the	*/
/* file menucalls.c).  These are wrappers for toggleexcl().	*/
/*--------------------------------------------------------------*/

void togglegridstyles(xcWidget button) {
   if (button != NULL)
      toggleexcl(button, GridStyles, XtNumber(GridStyles));
}

void toggleanchors(xcWidget button) {
   if (button != NULL)
      toggleexcl(button, Anchors, XtNumber(Anchors));
}

void togglefontstyles(xcWidget button) {
   if (button != NULL)
      toggleexcl(button, FontStyles, XtNumber(FontStyles));
}

void toggleencodings(xcWidget button) {
   if (button != NULL)
      toggleexcl(button, FontEncodings, XtNumber(FontEncodings));
}

/*--------------------------------------------------------------*/
/* Toggle a bit in the anchoring field of a label		*/
/*--------------------------------------------------------------*/

void doanchorbit(xcWidget w, labelptr settext, short bitfield)
{
   if (settext != NULL) {
      int oldanchor = (int)settext->anchor;

      undrawtext(settext);
      settext->anchor ^= bitfield;
      redrawtext(settext);
      pwriteback(areawin->topinstance);

      register_for_undo(XCF_Anchor, UNDO_MORE, areawin->topinstance,
		(genericptr)settext, oldanchor);
   }
   else
      areawin->anchor ^= bitfield;

   if (w != NULL) {
      Boolean boolval;
      if (settext)
	 boolval = (settext->anchor & bitfield) ? 0 : 1;
      else
	 boolval = (areawin->anchor & bitfield) ? 0 : 1;
      toggle(w, (pointertype)(-1), &boolval);
   }
}

/*--------------------------------------------------------------*/
/* Toggle a pin-related bit in the anchoring field of a		*/
/* label.  This differs from doanchorbit() in that the label	*/
/* must be a pin, and this function cannot change the default	*/
/* behavior set by areawin->anchor.				*/
/*--------------------------------------------------------------*/

void dopinanchorbit(xcWidget w, labelptr settext, short bitfield)
{
   if ((settext != NULL) && settext->pin) {
      int oldanchor = (int)settext->anchor;

      undrawtext(settext);
      settext->anchor ^= bitfield;
      redrawtext(settext);
      pwriteback(areawin->topinstance);

      register_for_undo(XCF_Anchor, UNDO_MORE, areawin->topinstance,
		(genericptr)settext, oldanchor);

      if (w != NULL) {
	 Boolean boolval = (settext->anchor & bitfield) ? 0 : 1;
	 toggle(w, (pointertype)(-1), &boolval);
      }
   }
}

/*----------------------------------------------------------------*/
/* Set the anchoring for the label passed as 3rd parameter	  */
/*----------------------------------------------------------------*/

void setanchor(xcWidget w, pointertype value, labelptr settext, short mode)
{
   short newanchor, oldanchor;

   if (settext != NULL) {

      if (mode == 1)
         newanchor = (settext->anchor & (NONANCHORFIELD | TBANCHORFIELD))
			| value;
      else
         newanchor = (settext->anchor & (NONANCHORFIELD | RLANCHORFIELD))
			| value;

      if (settext->anchor != newanchor) {
	  oldanchor = (int)settext->anchor;
          undrawtext(settext);
          settext->anchor = newanchor;
          redrawtext(settext);
          pwriteback(areawin->topinstance);

          register_for_undo(XCF_Anchor, UNDO_MORE, areawin->topinstance,
		(genericptr)settext, oldanchor);
      }
   }
   else {
      if (mode == 1)
         newanchor = (areawin->anchor & (NONANCHORFIELD | TBANCHORFIELD))
			| value;
      else
         newanchor = (areawin->anchor & (NONANCHORFIELD | RLANCHORFIELD))
			| value;
      areawin->anchor = newanchor;
   }
   if (w != NULL) toggleanchors(w);
}

/*--------------------------------------------------------------*/
/* Set vertical anchoring (top, middle, bottom) on all	  	*/
/* selected labels						*/
/*--------------------------------------------------------------*/

void setvanchor(xcWidget w, pointertype value, caddr_t nulldata)
{
   short *fselect;
   labelptr settext;
   short labelcount = 0;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      settext = *((labelptr *)EDITPART);
      setanchor(w, value, settext, 2);
   }
   else {
      for (fselect = areawin->selectlist; fselect < areawin->selectlist +
            areawin->selects; fselect++) {
         if (SELECTTYPE(fselect) == LABEL) {
            labelcount++;
            settext = SELTOLABEL(fselect);
            setanchor(NULL, value, settext, 2);
         }
      }
      if (labelcount == 0) setanchor(w, value, NULL, 2);
      else unselect_all();
   }
}

/*----------------------------------------------------------------*/
/* Set horizontal anchoring (left, center, right) on all	  */
/* selected labels						  */
/*----------------------------------------------------------------*/

void sethanchor(xcWidget w, pointertype value, caddr_t nulldata)
{
   short *fselect;
   labelptr settext;
   short labelcount = 0;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      settext = *((labelptr *)EDITPART);
      setanchor(w, value, settext, 1);
   }
   else {
      for (fselect = areawin->selectlist; fselect < areawin->selectlist +
            areawin->selects; fselect++) {
         if (SELECTTYPE(fselect) == LABEL) {
            labelcount++;
            settext = SELTOLABEL(fselect);
            setanchor(NULL, value, settext, 1);
         }
      }
      if (labelcount == 0) setanchor(w, value, NULL, 1);
      else unselect_all();
   }
}

/*--------------------------------------------------------------*/
/* Set an anchor field bit on all selected labels		*/ 
/* (flip invariance, latex mode, etc.)				*/
/*--------------------------------------------------------------*/

void setanchorbit(xcWidget w, pointertype value, caddr_t nulldata)
{
   short *fselect;
   labelptr settext;
   short labelcount = 0;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      settext = *((labelptr *)EDITPART);
      doanchorbit(w, settext, (short)value);
   }
   else {
      for (fselect = areawin->selectlist; fselect < areawin->selectlist +
            areawin->selects; fselect++) {
         if (SELECTTYPE(fselect) == LABEL) {
            labelcount++;
            settext = SELTOLABEL(fselect);
            doanchorbit(NULL, settext, (short)value);
         }
      }
      if (labelcount == 0) doanchorbit(w, NULL, (short)value);
      else unselect_all();
   }
}

/*--------------------------------------------------------------*/
/* Set pin-related bit field of "anchor" on all selected pins	*/ 
/* (e.g., pin visibility)					*/
/*--------------------------------------------------------------*/

void setpinanchorbit(xcWidget w, pointertype value, caddr_t nulldata)
{
   short *fselect;
   labelptr settext;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      settext = *((labelptr *)EDITPART);
      if (settext->pin)
	 dopinanchorbit(w, settext, (short)value);
   }
   else {
      for (fselect = areawin->selectlist; fselect < areawin->selectlist +
            areawin->selects; fselect++) {
         if (SELECTTYPE(fselect) == LABEL) {
            settext = SELTOLABEL(fselect);
	    if (settext->pin)
               dopinanchorbit(NULL, settext, (short)value);
         }
      }
      unselect_all();
   }
}

/*--------------------------------------------------------------*/
/* Enable or Disable the toolbar				*/
/*--------------------------------------------------------------*/

#ifdef HAVE_XPM
void dotoolbar(xcWidget w, caddr_t clientdata, caddr_t calldata)   
{
   Arg wargs[1];

   if (areawin->toolbar_on) {
      areawin->toolbar_on = False;
      XtUnmanageChild(toolbar);
      XtSetArg(wargs[0], XtNlabel, "Enable Toolbar");
      XtSetValues(OptionsDisableToolbarButton, wargs, 1);
      XtRemoveCallback(areawin->area, XtNresize, (XtCallbackProc)resizetoolbar, NULL);
   }
   else {
      areawin->toolbar_on = True;
      XtManageChild(toolbar);
      XtSetArg(wargs[0], XtNlabel, "Disable Toolbar");
      XtSetValues(OptionsDisableToolbarButton, wargs, 1);
      XtAddCallback(areawin->area, XtNresize, (XtCallbackProc)resizetoolbar, NULL);
   }
}  

/*--------------------------------------------------------------*/
/* Overwrite the toolbar pixmap for color or stipple entries	*/
/*--------------------------------------------------------------*/

#ifndef XC_WIN32

void overdrawpixmap(xcWidget button)
{
   xcWidget pbutton;
   Arg args[3];
   int ltype, color;
   Pixmap stippix;

   if (button == NULL) return;

   XtSetArg(args[0], XtNlabelType, &ltype);
   XtGetValues(button, args, 1);

   if (ltype != XwRECT && button != ColorInheritColorButton) return;

   XtSetArg(args[0], XtNrectColor, &color);
   XtSetArg(args[1], XtNrectStipple, &stippix);
   XtGetValues(button, args, 2);

   if (stippix == (Pixmap)NULL && button != FillBlackButton)
      pbutton = ColorsToolButton;
   else
      pbutton = FillsToolButton;

   if (button == ColorInheritColorButton) {
      XtSetArg(args[0], XtNlabelType, XwIMAGE);
      XtSetValues(pbutton, args, 1);
   }
   else if (button == FillBlackButton) {
      XtSetArg(args[0], XtNlabelType, XwRECT);
      XtSetArg(args[1], XtNrectColor, color);
      XtSetArg(args[2], XtNrectStipple, (Pixmap)NULL);
      XtSetValues(pbutton, args, 3);
   }
   else if (button != FillOpaqueButton) {
      XtSetArg(args[0], XtNlabelType, XwRECT);
      if (stippix == (Pixmap)NULL)
         XtSetArg(args[1], XtNrectColor, color);
      else
         XtSetArg(args[1], XtNrectStipple, stippix);

      XtSetValues(pbutton, args, 2);
   }
}

#endif

#else
void overdrawpixmap(xcWidget button) {}
#endif	/* XPM */

/*--------------------------------------------------------------*/
/* Generic routine for use by all other data handling routines 	*/
/*--------------------------------------------------------------*/

buttonsave *getgeneric(xcWidget button, void (*getfunction)(), void *dataptr)
{
   Arg  wargs[1];
   buttonsave *saveptr;

   saveptr = (buttonsave *)malloc(sizeof(buttonsave)); 
   saveptr->button = button;
   saveptr->buttoncall = getfunction;
   saveptr->dataptr = dataptr;

   if (button != NULL) {
      XtSetArg(wargs[0], XtNforeground, &saveptr->foreground);
      XtGetValues(button, wargs, 1);
      XtSetArg(wargs[0], XtNforeground, colorlist[OFFBUTTONCOLOR].color.pixel);
      XtSetValues(button, wargs, 1);
      XtRemoveAllCallbacks(button, XtNselect);
   }
   return saveptr;
}

/*--------------------------------------------------------------*/
/* Generate popup dialog for snap space value input		*/
/*--------------------------------------------------------------*/

void getsnapspace(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   char buffer[50];
   buttonsave *savebutton;
   float *floatptr = &xobjs.pagelist[areawin->page]->snapspace;

   savebutton = getgeneric(button, getsnapspace, (void *)floatptr);
   measurestr(*floatptr, buffer);
   popupprompt(button, "Enter value:", buffer, setgrid, savebutton, NULL);
}

/*--------------------------------------------------------------*/
/* Generate popup dialog for grid space value input		*/
/*--------------------------------------------------------------*/

void getgridspace(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   char buffer[50];
   buttonsave *savebutton;
   float *floatptr = &xobjs.pagelist[areawin->page]->gridspace;

   savebutton = getgeneric(button, getgridspace, (void *)floatptr);
   measurestr(*floatptr, buffer);
   popupprompt(button, "Enter value:", buffer, setgrid, savebutton, NULL);
}

/*----------------------------------------------------------------*/
/* Generic routine for setting a floating-point value (through a  */
/* (float *) pointer passed as the second parameter)              */
/*----------------------------------------------------------------*/

void setfloat(xcWidget w, float *dataptr)
{
   float oldvalue = *dataptr;
   int res = sscanf(_STR2, "%f", dataptr);

   if (res == 0 || *dataptr <= 0) {
      *dataptr = oldvalue;
      Wprintf("Illegal value");
   }  
   if (oldvalue != *dataptr) drawarea(NULL, NULL, NULL);
}

/*----------------------------------------------------------------*/
/* Set text scale.                                                */
/*----------------------------------------------------------------*/

void settsize(xcWidget w, labelptr settext)
{
   float tmpres;

   int res = sscanf(_STR2, "%f", &tmpres);

   if (res == 0 || tmpres <= 0) {  /* can't interpret value or bad value */
      Wprintf("Illegal value");
      return;
   }
   changetextscale(tmpres);

   if (areawin->selects > 0) unselect_all();
}

/*--------------------------------------------------------------*/
/* Auto-set:  Enable automatic scaling                          */
/*--------------------------------------------------------------*/

void autoset(xcWidget w, xcWidgetList entertext, caddr_t nulldata)
{
   xobjs.pagelist[areawin->page]->pmode |= 2;
   updatetext(w, entertext, nulldata);
}

/*--------------------------------------------------------------*/

void autostop(xcWidget w, caddr_t clientdata, caddr_t nulldata)
{
   xobjs.pagelist[areawin->page]->pmode &= 1;
}

/*--------------------------------------------------------------*/
/* Set the denomenator value of the drawing scale 		*/ 
/*--------------------------------------------------------------*/

void setscaley(xcWidget w, float *dataptr)
{
   float oldvalue = *dataptr;
   int res = sscanf(_STR2, "%f", dataptr);

   if (res == 0 || *dataptr <= 0 || topobject->bbox.height == 0) {
      *dataptr = oldvalue;
      Wprintf("Illegal value");
   }
   else {
      *dataptr = (*dataptr * 72) / topobject->bbox.height;
      *dataptr /= getpsscale(1.0, areawin->page);
   }
}

/*----------------------------------------------------------------*/
/* Set the numerator value of the drawing scale			  */
/*----------------------------------------------------------------*/

void setscalex(xcWidget w, float *dataptr)
{
   float oldvalue = *dataptr;
   int res = sscanf(_STR2, "%f", dataptr);

   if (res == 0 || *dataptr <= 0 || topobject->bbox.width == 0) {
      *dataptr = oldvalue;
      Wprintf("Illegal value");
   }
   else {
      *dataptr = (*dataptr * 72) / topobject->bbox.width;
      *dataptr /= getpsscale(1.0, areawin->page);
   }
}

/*----------------------------------------------------------------*/
/* Set the page orientation (either Landscape or Portrait)	  */
/*----------------------------------------------------------------*/

void setorient(xcWidget w, short *dataptr)
{
   Arg wargs[1];

   if (*dataptr == 0) {
      *dataptr = 90;
      XtSetArg(wargs[0], XtNlabel, "Landscape");
   }
   else {
      *dataptr = 0;
      XtSetArg(wargs[0], XtNlabel, "Portrait");
   }
   XtSetValues(w, wargs, 1);
}

/*----------------------------------------------------------------*/
/* Set the output mode to "Full Page" (unencapsulated PostScript) */
/* or "Embedded" (encapsulated PostScript)			  */
/*----------------------------------------------------------------*/

void setpmode(xcWidget w, short *dataptr)
{
   Arg wargs[1];
   xcWidget pwidg = XtParent(w);
   xcWidget autowidg = XtNameToWidget(pwidg, "Auto-fit");

   if (!(*dataptr & 1)) {
      *dataptr = 1;
      XtSetArg(wargs[0], XtNlabel, "Full Page");

      XtManageChild(XtNameToWidget(pwidg, "fpedit"));
      XtManageChild(XtNameToWidget(pwidg, "fpokay"));
      XtManageChild(autowidg);
   }
   else {
      *dataptr = 0;	/* This also turns off auto-fit */
      XtSetArg(wargs[0], XtNset, False);
      XtSetValues(autowidg, wargs, 1);
      XtSetArg(wargs[0], XtNlabel, "Embedded");

      XtUnmanageChild(XtNameToWidget(pwidg, "fpedit"));
      XtUnmanageChild(XtNameToWidget(pwidg, "fpokay"));
      XtUnmanageChild(autowidg);
   }
   XtSetValues(w, wargs, 1);
}

/*--------------------------------------------------------------*/
/* Set the output page size, in the current unit of measure	*/
/*--------------------------------------------------------------*/

void setpagesize(xcWidget w, XPoint *dataptr)
{
   char fpedit[75];
   Arg wargs[1];
   Boolean is_inches;

   is_inches = setoutputpagesize(dataptr);
   sprintf(fpedit, "%3.2f x %3.2f %s",
	(float)xobjs.pagelist[areawin->page]->pagesize.x / 72.0,
	(float)xobjs.pagelist[areawin->page]->pagesize.y / 72.0,
	(is_inches) ? "in" : "cm");

   XtSetArg(wargs[0], XtNstring, fpedit);
   XtSetValues(XtNameToWidget(XtParent(w), "fpedit"), wargs, 1);
}

/*--------------------------------------------------------------*/
/* Generate popup dialog to get a character kern value, which	*/
/* is two integer numbers in the range -128 to +127 (size char)	*/
/*--------------------------------------------------------------*/

void getkern(xcWidget button, caddr_t nulldata, caddr_t calldata)
{
   char buffer[50];
   buttonsave *savebutton;
   int kx, ky;
   stringpart *strptr, *nextptr;

   strcpy(buffer, "0,0");

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      labelptr curlabel = TOLABEL(EDITPART);
      strptr = findstringpart(areawin->textpos - 1, NULL, curlabel->string,
			areawin->topinstance);
      nextptr = findstringpart(areawin->textpos, NULL, curlabel->string,
			areawin->topinstance);
      if (strptr->type == KERN) {
	 kx = strptr->data.kern[0];
	 ky = strptr->data.kern[1];
         sprintf(buffer, "%d,%d", kx, ky);
      }
      else if (nextptr && nextptr->type == KERN) {
	 strptr = nextptr;
	 kx = strptr->data.kern[0];
	 ky = strptr->data.kern[1];
         sprintf(buffer, "%d,%d", kx, ky);
      }
      else strptr = NULL;
   }

   savebutton = getgeneric(button, getkern, strptr);
   popupprompt(button, "Enter Kern X,Y:", buffer, setkern, savebutton, NULL);
}

/*----------------------------------------------------------------*/
/* Generate popup dialog to get the drawing scale, specified as a */
/* whole-number ratio X:Y					  */
/*----------------------------------------------------------------*/

void getdscale(xcWidget button, caddr_t nulldata, caddr_t calldata)
{
   char buffer[50];
   buttonsave *savebutton;
   XPoint *ptptr = &(xobjs.pagelist[areawin->page]->drawingscale);

   savebutton = getgeneric(button, getdscale, ptptr);
   sprintf(buffer, "%d:%d", ptptr->x, ptptr->y);
   popupprompt(button, "Enter Scale:", buffer, setdscale, savebutton, NULL);
}

/*--------------------------------------------------------------*/
/* Generate the popup dialog for getting text scale.		*/ 
/*--------------------------------------------------------------*/

void gettsize(xcWidget button, caddr_t nulldata, caddr_t calldata)
{
   char buffer[50];
   buttonsave *savebutton;
   float *floatptr;
   Boolean local;
   labelptr settext;

   settext = gettextsize(&floatptr);
   sprintf(buffer, "%5.2f", *floatptr);

   if (settext) {
      savebutton = getgeneric(button, gettsize, settext);
      popupprompt(button, "Enter text scale:", buffer, settsize, savebutton, NULL);
   }
   else {
      savebutton = getgeneric(button, gettsize, floatptr);
      popupprompt(button,
	    "Enter default text scale:", buffer, setfloat, savebutton, NULL);
   }
}

/*----------------------------------------------------------------*/
/* Generate popup dialog for getting object scale		  */
/*----------------------------------------------------------------*/

void getosize(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   char buffer[50];
   float flval;
   buttonsave *savebutton;
   short *osel = areawin->selectlist;
   short selects = 0;
   objinstptr setobj = NULL;

   for (; osel < areawin->selectlist + areawin->selects; osel++)
      if (SELECTTYPE(osel) == OBJINST) {
	 setobj = SELTOOBJINST(osel);
	 selects++;
	 break;
      }
   if (setobj == NULL) {
      Wprintf("No objects were selected for scaling.");
      return;
   }
   flval = setobj->scale;
   savebutton = getgeneric(button, getosize, setobj);
   sprintf(buffer, "%4.2f", flval);
   popupprompt(button, "Enter object scale:", buffer, setosize, savebutton, NULL);
}

/*----------------------------------------------------------------*/
/* Generate popup prompt for getting global linewidth		  */
/*----------------------------------------------------------------*/

void getwirewidth(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   char buffer[50];
   buttonsave *savebutton;
   float *widthptr;

   widthptr = &(xobjs.pagelist[areawin->page]->wirewidth);
   savebutton = getgeneric(button, getwirewidth, widthptr);
   sprintf(buffer, "%4.2f", *widthptr / 2.0);
   popupprompt(button, "Enter new global linewidth:", buffer, setwidth,
	savebutton, NULL);
}

/*----------------------------------------------------------------*/
/* Generate popup dialong for getting linewidths of elements	  */ 
/*----------------------------------------------------------------*/

void getwwidth(xcWidget button, caddr_t clientdata, caddr_t calldata)
{
   char buffer[50];
   buttonsave *savebutton;
   short *osel = areawin->selectlist;
   genericptr setel;
   float flval;

   for (; osel < areawin->selectlist + areawin->selects; osel++) {
      setel = *(topobject->plist + (*osel));
      if (IS_ARC(setel)) {
	 flval = ((arcptr)setel)->width;
	 break;
      }
      else if (IS_POLYGON(setel)) {
	 flval = ((polyptr)setel)->width;
	 break;
      }
      else if (IS_SPLINE(setel)) {
	 flval = ((splineptr)setel)->width;
	 break;
      }
      else if (IS_PATH(setel)) {
	 flval = ((pathptr)setel)->width;
	 break;
      }
   }
   savebutton = getgeneric(button, getwwidth, setel);
   if (osel == areawin->selectlist + areawin->selects) {
      sprintf(buffer, "%4.2f", areawin->linewidth);
      popupprompt(button, "Enter new default line width:", buffer, setwwidth,
		savebutton, NULL);
   }
   else {
      sprintf(buffer, "%4.2f", flval);
      popupprompt(button, "Enter new line width:", buffer, setwwidth,
		savebutton, NULL);
   }
}

/*----------------------------------------------------------------*/
/* Generic popup prompt for getting a floating-point value	  */
/*----------------------------------------------------------------*/

void getfloat(xcWidget button, float *floatptr, caddr_t calldata)
{
   char buffer[50];
   buttonsave *savebutton;

   savebutton = getgeneric(button, getfloat, floatptr);
   sprintf(buffer, "%4.2f", *floatptr);
   popupprompt(button, "Enter value:", buffer, setfloat, savebutton, NULL);
}

/*----------------------------------------------------------------*/
/* Set the filename for the current page			  */
/*----------------------------------------------------------------*/

void setfilename(xcWidget w, char **dataptr)
{
   short cpage, depend = 0;
   objectptr checkpage;
   char *oldstr = xobjs.pagelist[areawin->page]->filename;

   if ((*dataptr != NULL) && !strcmp(*dataptr, _STR2))
      return;   /* no change in string */

   /* Make the change to the current page */
   xobjs.pagelist[areawin->page]->filename = strdup(_STR2);

   /* All existing filenames which match the old string should also be changed */
   for (cpage = 0; cpage < xobjs.pages; cpage++) {
      if ((xobjs.pagelist[cpage]->pageinst != NULL) && (cpage != areawin->page)) {
	 if ((oldstr != NULL) && (!strcmp(xobjs.pagelist[cpage]->filename, oldstr))) {
	    free(xobjs.pagelist[cpage]->filename);
	    xobjs.pagelist[cpage]->filename = strdup(_STR2);
	 }
      }
   }
   free(oldstr);
}

/*----------------------------------------------------------------*/
/* Set the page label for the current page			  */
/*----------------------------------------------------------------*/

void setpagelabel(xcWidget w, char *dataptr)
{
   short i;

   /* Whitespace and non-printing characters not allowed */

   for (i = 0; i < strlen(_STR2); i++) {
      if ((!isprint(_STR2[i])) || (isspace(_STR2[i]))) {
         _STR2[i] = '_';
         Wprintf("Replaced illegal whitespace in name with underscore");
      }
   }

   if (!strcmp(dataptr, _STR2)) return; /* no change in string */
   if (strlen(_STR2) == 0)
      sprintf(topobject->name, "Page %d", areawin->page + 1);
   else
      sprintf(topobject->name, "%.79s", _STR2);

   /* For schematics, all pages with associations to symbols must have	*/
   /* unique names.							*/
   if (topobject->symschem != NULL) checkpagename(topobject);

   printname(topobject);
   renamepage(areawin->page);
}

/*--------------------------------------------------------------*/
/* Change the Button1 binding for a particular mode (non-Tcl)	*/
/*--------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* Change to indicated tool (key binding w/o value) */
/*--------------------------------------------------------------*/

void changetool(xcWidget w, pointertype value, caddr_t nulldata)
{
   mode_rebinding((int)value, (short)-1);
   highlightexcl(w, (int)value, (int)-1);
}

/*--------------------------------------------------------------*/
/* Execute function binding for the indicated tool if something	*/
/* is selected;  otherwise, change to the indicated tool mode.	*/
/*--------------------------------------------------------------*/

void exec_or_changetool(xcWidget w, pointertype value, caddr_t nulldata)
{
   if (areawin->selects > 0)
      mode_tempbinding((int)value, -1);
   else {
      mode_rebinding((int)value, -1);
      highlightexcl(w, (int)value, -1);
   }
}

/*--------------------------------------------------------------*/
/* Special case of exec_or_changetool(), where an extra value	*/
/* is passed to the routine indication amount of rotation, or	*/
/* type of flip operation.					*/
/*--------------------------------------------------------------*/

void rotatetool(xcWidget w, pointertype value, caddr_t nulldata)
{
   if (areawin->selects > 0)
      mode_tempbinding(XCF_Rotate, (int)value);
   else {
      mode_rebinding(XCF_Rotate, (int)value);
      highlightexcl(w, (int)XCF_Rotate, (int)value);
   }
}

/*--------------------------------------------------------------*/
/* Special case of changetool() for pan mode, where a value is	*/
/* required to pass to the key binding routine.			*/
/*--------------------------------------------------------------*/

void pantool(xcWidget w, pointertype value, caddr_t nulldata)
{
   mode_rebinding(XCF_Pan, (int)value);
   highlightexcl(w, (int)XCF_Pan, (int)value);
}

/*--------------------------------------------------------------*/
/* Add a new font name to the list of known fonts		*/
/* Register the font number for the Alt-F cycling mechanism	*/
/* Tcl: depends on command tag mechanism for GUI menu update.	*/
/*--------------------------------------------------------------*/

void makenewfontbutton()
{

   Arg	wargs[1];
   int  n = 0;
   xcWidget newbutton, cascade;

   if (fontcount == 0) return;

   cascade = XtParent(FontAddNewFontButton);
   XtnSetArg(XtNfont, appdata.xcfont);
   newbutton = XtCreateWidget(fonts[fontcount - 1].family, XwmenubuttonWidgetClass,
	 cascade, wargs, n);

   XtAddCallback (newbutton, XtNselect, (XtCallbackProc)setfont,
		Number(fontcount - 1));
   XtManageChild(newbutton);

   nfontnumbers++;
   if (nfontnumbers == 1)
      fontnumbers = (u_short *)malloc(sizeof(u_short)); 
   else
      fontnumbers = (u_short *)realloc(fontnumbers, nfontnumbers
		* sizeof(u_short));
   fontnumbers[nfontnumbers - 1] = fontcount - 1;
}

/*--------------------------------------------------------------*/
/* Make new encoding menu button				*/
/*--------------------------------------------------------------*/

void makenewencodingbutton(char *ename, char value)
{
   Arg	wargs[1];
   int  n = 0;
   xcWidget newbutton, cascade;

   cascade = XtParent(EncodingStandardButton);

   /* return if button has already been made */
   newbutton = XtNameToWidget(cascade, ename);
   if (newbutton != NULL) return;

   XtnSetArg(XtNfont, appdata.xcfont);
   newbutton = XtCreateWidget(ename, XwmenubuttonWidgetClass,
	 cascade, wargs, n);

   XtAddCallback (newbutton, XtNselect, (XtCallbackProc)setfontencoding,
		Number(value));
   XtManageChild(newbutton);
}

/*--------------------------------------------------------------*/
/* Set the menu checkmarks on the font menu			*/
/*--------------------------------------------------------------*/

void togglefontmark(int fontval)
{
   Arg args[1];
   xcWidget widget, cascade, sibling;
   short i;

   cascade = XtParent(FontAddNewFontButton);
   widget = XtNameToWidget(cascade, fonts[fontval].family);

   /* Remove checkmark from all widgets in the list */

   XtSetArg(args[0], XtNsetMark, False);
   for (i = 0; i < fontcount; i++) {
      if (i != fontval) {
         sibling = XtNameToWidget(cascade, fonts[i].family);
         XtSetValues(sibling, args, 1);
      }
   }

   /* Add checkmark to designated font */

   XtSetArg(args[0], XtNsetMark, True);
   XtSetValues(widget, args, 1);
}

/*--------------------------------------------------------------------*/
/* Toggle one of a set of menu items, only one of which can be active */
/*--------------------------------------------------------------------*/

void toggleexcl(xcWidget widget, menuptr menu, int menulength)
{
   Arg          args[1];
   xcWidget     parent = xcParent(widget);
   xcWidget     sibling;
   menuptr	mitem, mtest;
   short	i;

   /* find the menu item which corresponds to the widget which was pushed */

   for (mtest = menu; mtest < menu + menulength; mtest++) {
      sibling = XtNameToWidget(parent, mtest->name);
      if (sibling == widget) break;
   }

   /* remove checkmark from other widgets in the list */

   XtSetArg(args[0], XtNsetMark, False);
   if (menu == Fonts) {     		/* special action for font list */
      for (i = 0; i < fontcount; i++) {
	 sibling = XtNameToWidget(parent, fonts[i].family);
	 if (sibling != widget)
	    XtSetValues(sibling, args, 1);
      }
      mtest = &Fonts[3];   /* so that mtest->func has correct value below */
   }
   else if (mtest == menu + menulength) return;  /* something went wrong? */

   for (mitem = menu; mitem < menu + menulength; mitem++) {
      sibling = XtNameToWidget(parent, mitem->name);
      if (mitem->func == mtest->func)
         XtSetValues(sibling, args, 1);
   }

   /* Now set the currently pushed widget */

   XtSetArg(args[0], XtNsetMark, True);
   XtSetValues(widget, args, 1);
}

/*----------------------------------------------------------------------*/
/* Cursor changes based on toolbar mode					*/
/*----------------------------------------------------------------------*/

void toolcursor(int mode)
{
   /* Some cursor types for different modes */
   switch(mode) {
      case XCF_Spline:
      case XCF_Move:
      case XCF_Join:
      case XCF_Unjoin:
	 XDefineCursor (dpy, areawin->window, ARROW);
	 areawin->defaultcursor = &ARROW;
	 break;
      case XCF_Pan:
	 XDefineCursor (dpy, areawin->window, HAND);
	 areawin->defaultcursor = &HAND;
	 break;
      case XCF_Delete:
	 XDefineCursor (dpy, areawin->window, SCISSORS);
	 areawin->defaultcursor = &SCISSORS;
	 break;
      case XCF_Copy:
	 XDefineCursor (dpy, areawin->window, COPYCURSOR);
	 areawin->defaultcursor = &COPYCURSOR;
	 break;
      case XCF_Push:
      case XCF_Select_Save:
	 XDefineCursor (dpy, areawin->window, QUESTION);
	 areawin->defaultcursor = &QUESTION;
	 break;
      case XCF_Rotate:
      case XCF_Flip_X:
      case XCF_Flip_Y:
	 XDefineCursor (dpy, areawin->window, ROTATECURSOR);
	 areawin->defaultcursor = &ROTATECURSOR;
	 break;
      case XCF_Edit:
	 XDefineCursor (dpy, areawin->window, EDCURSOR);
	 areawin->defaultcursor = &EDCURSOR;
	 break;
      case XCF_Arc:
	 XDefineCursor (dpy, areawin->window, CIRCLE);
	 areawin->defaultcursor = &CIRCLE;
	 break;
      case XCF_Text:
      case XCF_Pin_Label:
      case XCF_Pin_Global:
      case XCF_Info_Label:
	 XDefineCursor (dpy, areawin->window, TEXTPTR);
	 areawin->defaultcursor = &TEXTPTR;
	 break;
      default:
	 XDefineCursor (dpy, areawin->window, CROSS);
	 areawin->defaultcursor = &CROSS;
	 break;
   }
}

/*--------------------------------------------------------------------------*/
/* Highlight one of a set of toolbar items, only one of which can be active */
/*--------------------------------------------------------------------------*/

void highlightexcl(xcWidget widget, int func, int value)
{
#ifdef HAVE_XPM
   Arg          args[2];
   xcWidget     parent = xcParent(PanToolButton);
   xcWidget     sibling, self = (xcWidget)NULL;
   toolbarptr	titem, ttest;

   /* remove highlight from all widgets in the toolbar */

   XtSetArg(args[0], XtNbackground, colorlist[BACKGROUND].color.pixel);
   XtSetArg(args[1], XtNborderColor, colorlist[SNAPCOLOR].color.pixel);
   for (titem = ToolBar; titem < ToolBar + toolbuttons; titem++) {
      sibling = XtNameToWidget(parent, titem->name);
      if (sibling == widget)
	 self = sibling;
      else
         XtSetValues(sibling, args, 2);
   }

   if (self == (xcWidget)NULL) {
      /* We invoked a toolbar button from somewhere else.  Highlight	*/
      /* the toolbar associated with the function value.		*/
      switch (func) {
	 case XCF_Pan:
	    self = PanToolButton;
	    break;

	 case XCF_Rotate:
	    if (value > 0)
	       self = RotPToolButton;
	    else
	       self = RotNToolButton;
	    break;

	 default:
	    for (titem = ToolBar; titem < ToolBar + toolbuttons; titem++) {
	       if (func == (pointertype)(titem->passeddata)) {
                  self = XtNameToWidget(parent, titem->name);
	          break;
	       }
	    }
	    break;
      }
   }

   /* Now highlight the currently pushed widget */

   if (self != (xcWidget)NULL) {
      XtSetArg(args[0], XtNbackground, colorlist[RATSNESTCOLOR].color.pixel);
      XtSetArg(args[1], XtNborderColor, colorlist[RATSNESTCOLOR].color.pixel);
      XtSetValues(self, args, 2);
   }
#endif /* HAVE_XPM */
}

/*--------------------*/
/* Toggle a menu item */
/*--------------------*/

void toggle(xcWidget w, pointertype soffset, Boolean *setdata)
{
   Arg	wargs[1];
   Boolean *boolvalue;

   if (soffset == -1)
      boolvalue = setdata;
   else
      boolvalue = (Boolean *)(areawin + soffset);

   *boolvalue = !(*boolvalue);
   XtSetArg(wargs[0], XtNsetMark, *boolvalue);
   XtSetValues(w, wargs, 1);
   drawarea(w, NULL, NULL);
}

/*----------------------------------------------------------------*/
/* Invert the color scheme used for the background/foreground	  */
/*----------------------------------------------------------------*/

void inversecolor(xcWidget w, pointertype soffset, caddr_t calldata)
{
   Boolean *boolvalue = (Boolean *)(areawin + soffset);

   /* change color scheme */

   setcolorscheme(*boolvalue);

   /* toggle checkmark in menu on "Alt colors" */

   if (w == NULL) w = OptionsAltColorsButton;
   if (w != NULL) toggle(w, soffset, calldata);
   if (eventmode == NORMAL_MODE)
      XDefineCursor (dpy, areawin->window, DEFAULTCURSOR);
}

/*----------------------------------------------------------------*/
/* Change menu selection for reported measurement units		  */
/*----------------------------------------------------------------*/

void togglegrid(u_short type)
{
   xcWidget button, bparent = XtParent(GridtypedisplayDecimalInchesButton);

   if (type == CM) button = XtNameToWidget(bparent, "Centimeters");
   else if (type == FRAC_INCH) button = XtNameToWidget(bparent, "Fractional Inches");
   else if (type == DEC_INCH) button = XtNameToWidget(bparent, "Decimal Inches");
   else if (type == INTERNAL) button = XtNameToWidget(bparent, "Internal Units");
   else button = XtNameToWidget(bparent, "Coordinates");
   if (button) {
      toggleexcl(button, GridStyles, XtNumber(GridStyles));
      W1printf(" ");
   }
}

/*----------------------------------------------------------------*/
/* Set the default reported grid units to inches or centimeters   */
/*----------------------------------------------------------------*/

void setgridtype(char *string)
{
   xcWidget button, bparent = XtParent(GridtypedisplayDecimalInchesButton);

   if (!strcmp(string, "inchscale")) {
      button = XtNameToWidget(bparent, "Fractional Inches");
      getgridtype(button, FRAC_INCH, NULL);
   }
   else if (!strcmp(string, "cmscale")) {
      button = XtNameToWidget(bparent, "Centimeters");
      getgridtype(button, CM, NULL);
   }
}

/*----------------------------------------------*/
/* Make new library and add a new button to the */
/* "Libraries" cascaded menu.			*/
/*----------------------------------------------*/

int createlibrary(Boolean force)
{
   xcWidget libmenu, newbutton, oldbutton;
   Arg wargs[2];
   char libstring[20];
   int libnum;
   objectptr newlibobj;

   /* If there's an empty library, return its number */
   if ((!force) && (libnum = findemptylib()) >= 0) return (libnum + LIBRARY);
   libnum = (xobjs.numlibs++) + LIBRARY;
   xobjs.libtop = (objinstptr *)realloc(xobjs.libtop,
		(libnum + 1) * sizeof(objinstptr));
   xobjs.libtop[libnum] = xobjs.libtop[libnum - 1];
   libnum--;

   newlibobj = (objectptr) malloc(sizeof(object));
   initmem(newlibobj);
   xobjs.libtop[libnum] = newpageinst(newlibobj);

   sprintf(newlibobj->name, "Library %d", libnum - LIBRARY + 1);

   /* Create the library */

   xobjs.userlibs = (Library *) realloc(xobjs.userlibs, xobjs.numlibs
	* sizeof(Library));
   xobjs.userlibs[libnum + 1 - LIBRARY] = xobjs.userlibs[libnum - LIBRARY];
   xobjs.userlibs[libnum - LIBRARY].library = (objectptr *) malloc(sizeof(objectptr));
   xobjs.userlibs[libnum - LIBRARY].number = 0;
   xobjs.userlibs[libnum - LIBRARY].instlist = NULL;

   /* To-do:  initialize technology list */
   /*
   xobjs.userlibs[libnum - LIBRARY].filename = NULL;
   xobjs.userlibs[libnum - LIBRARY].flags = (char)0;
   */

   /* Previously last button becomes new library pointer */

   oldbutton = GotoLibraryLibrary2Button;
   XtRemoveAllCallbacks (oldbutton, XtNselect);
   XtAddCallback (oldbutton, XtNselect, (XtCallbackProc)startcatalog,
	Number(libnum));
   XtSetArg(wargs[0], XtNlabel, xobjs.libtop[libnum]->thisobject->name);
   XtSetValues(oldbutton, wargs, 1);

   /* Make new entry in the menu to replace the User Library button */
   /* xcWidget name is unique so button can be found later.  Label is */
   /* always set to "User Library"                                  */

   sprintf(libstring, "Library %d", libnum - LIBRARY + 2);
   libmenu = XtParent(GotoLibraryAddNewLibraryButton);
   XtSetArg(wargs[0], XtNfont, appdata.xcfont);
   XtSetArg(wargs[1], XtNlabel, "User Library");
   newbutton = XtCreateWidget(libstring, XwmenubuttonWidgetClass,
	libmenu, wargs, 2);
   XtAddCallback (newbutton, XtNselect, (XtCallbackProc)startcatalog,
	Number(libnum + 1));
   XtManageChild(newbutton);
   GotoLibraryLibrary2Button = newbutton;

   /* Update the library directory to include the new page */

   composelib(LIBLIB);

   return libnum;
}

/*--------------------------------------------------------------*/
/* Routine called by newpage() if new button needs to be made 	*/
/* to add to the "Pages" cascaded menu.				*/
/*--------------------------------------------------------------*/

void makepagebutton()
{
   xcWidget pagemenu, newbutton;
   Arg wargs[1];
   char pagestring[10];

   /* make new entry in the menu */

   pagemenu = XtParent(GotoPageAddNewPageButton);
   XtSetArg(wargs[0], XtNfont, appdata.xcfont);
   sprintf(pagestring, "Page %d", xobjs.pages);
   newbutton = XtCreateWidget(pagestring, XwmenubuttonWidgetClass,
         pagemenu, wargs, 1);
   XtAddCallback (newbutton, XtNselect, (XtCallbackProc)newpagemenu,
	Number(xobjs.pages - 1));
   XtManageChild(newbutton);

   /* Update the page directory */

   composelib(PAGELIB);
}

/*----------------------------------------------------------------*/
/* Find the Page menu button associated with the page number	  */
/* (passed parameter) and set the label of that button to the	  */
/* object name (= page label)					  */
/*----------------------------------------------------------------*/

void renamepage(short pagenumber)
{
   int page;
   objinstptr thisinst = xobjs.pagelist[pagenumber]->pageinst;
   Arg wargs[1];
   xcWidget parent = XtParent(GotoPageAddNewPageButton);
   xcWidget button;
   char bname[10];

   sprintf(bname, "Page %d", pagenumber + 1);
   button = XtNameToWidget(parent, bname);

   if ((button != NULL) && (thisinst != NULL)) {
      if (thisinst->thisobject->name != NULL)
         XtSetArg(wargs[0], XtNlabel,
		thisinst->thisobject->name);
      else
         XtSetArg(wargs[0], XtNlabel, bname);
      XtSetValues(button, wargs, 1);
   }
   else if (button == NULL)
      Fprintf(stderr, "Error:  No Button Widget named \"%9s\"\n", bname);
}

/*--------------------------------------------------------------*/
/* Same routine as above, for Library page menu buttons		*/
/*--------------------------------------------------------------*/

void renamelib(short libnumber)
{
   Arg wargs[1];
   xcWidget parent = XtParent(GotoLibraryAddNewLibraryButton);
   xcWidget button;
   char bname[13];

   sprintf(bname, "Library %d", libnumber - LIBRARY + 1);
   button = XtNameToWidget(parent, bname);

   if (button != NULL) {
      if (xobjs.libtop[libnumber]->thisobject->name != NULL)
         XtSetArg(wargs[0], XtNlabel, xobjs.libtop[libnumber]->thisobject->name);
      else
         XtSetArg(wargs[0], XtNlabel, bname);
      XtSetValues(button, wargs, 1);
   }
   else
      Fprintf(stderr, "Error:  No Button Widget named \"%12s\"\n", bname);
}

/*--------------------------------------------------------------*/
/* Set the menu checkmarks on the color menu			*/
/*--------------------------------------------------------------*/

void setcolormark(int colorval)
{
   Arg args[1];
   xcWidget w = NULL;
   short i;

   if (colorval == DEFAULTCOLOR)
      w = ColorInheritColorButton;
   else
      w = colorlist[colorval].cbutton;

   /* Remove mark from all menu items */

   XtSetArg(args[0], XtNsetMark, False);
   for (i = NUMBER_OF_COLORS; i < number_colors; i++)
      XtSetValues(colorlist[i].cbutton, args, 1);
   XtSetValues(ColorInheritColorButton, args, 1);

   /* Add mark to the menu button for the chosen color */

   if (w != (xcWidget)NULL) {
      overdrawpixmap(w);
      XtSetArg(args[0], XtNsetMark, True);
      XtSetValues(w, args, 1);
   }
}

/*----------------------------------------------------------------*/
/* Set the checkmarks on the element styles menu		  */
/*----------------------------------------------------------------*/

void setallstylemarks(u_short styleval)
{
   xcWidget w;
   Arg	wargs[1];

   XtSetArg(wargs[0], XtNsetMark, (styleval & UNCLOSED) ? 0 : 1);
   XtSetValues(BorderClosedButton, wargs, 1);

   XtSetArg(wargs[0], XtNsetMark, (styleval & BBOX) ? 1 : 0);
   XtSetValues(BorderBoundingBoxButton, wargs, 1);

   if (styleval & NOBORDER)
      w = BorderUnborderedButton;
   else if (styleval & DASHED)
      w = BorderDashedButton;
   else if (styleval & DOTTED)
      w = BorderDottedButton;
   else
      w = BorderSolidButton;
   toggleexcl(w, BorderStyles, XtNumber(BorderStyles));

   if (styleval & OPAQUE)
      w = FillOpaqueButton;
   else
      w = FillTransparentButton;
   toggleexcl(w, Stipples, XtNumber(Stipples));

   if (!(styleval & FILLED))
      w = FillWhiteButton;
   else {
      styleval &= FILLSOLID;
      styleval /= STIP0;
      switch(styleval) {
	 case 0: w = FillGray87Button; break;
	 case 1: w = FillGray75Button; break;
	 case 2: w = FillGray62Button; break;
	 case 3: w = FillGray50Button; break;
	 case 4: w = FillGray37Button; break;
	 case 5: w = FillGray25Button; break;
	 case 6: w = FillGray12Button; break;
	 case 7: w = FillBlackButton;  break;
      }
   }
   toggleexcl(w, Stipples, XtNumber(Stipples));
}

/*--------------------------------------------------------------*/
/* The following five routines are all wrappers for		*/
/* setelementstyle(),	  					*/
/* used in menudefs to differentiate between sections, each of  */
/* which has settings independent of the others.		*/
/*--------------------------------------------------------------*/

void setfill(xcWidget w, pointertype value, caddr_t calldata)
{
   setelementstyle(w, (u_short)value, OPAQUE | FILLED | FILLSOLID);
}

/*--------------------------------------------------------------*/

void makebbox(xcWidget w, pointertype value, caddr_t calldata)
{
   setelementstyle(w, (u_short)value, BBOX);
}

/*--------------------------------------------------------------*/

void setclosure(xcWidget w, pointertype value, caddr_t calldata)
{
   setelementstyle(w, (u_short)value, UNCLOSED);
}

/*----------------------------------------------------------------*/

void setopaque(xcWidget w, pointertype value, caddr_t calldata)
{
   setelementstyle(w, (u_short)value, OPAQUE);
}
   
/*----------------------------------------------------------------*/

void setline(xcWidget w, pointertype value, caddr_t calldata)
{
   setelementstyle(w, (u_short)value, BORDERS);
}
   
/*-----------------------------------------------*/
/* Set the color value for all selected elements */
/*-----------------------------------------------*/

void setcolor(xcWidget w, pointertype value, caddr_t calldata)
{
   short *scolor;
   int *ecolor, cindex, cval;
   Arg wargs[1];
   Boolean selected = False;
   stringpart *strptr, *nextptr;

   /* Get the color index value from the menu button widget itself */

   if (value == 1)
      cindex = cval = -1;
   else {
      XtSetArg(wargs[0], XtNrectColor, &cval);
      XtGetValues(w, wargs, 1);

      for (cindex = NUMBER_OF_COLORS; cindex < number_colors; cindex++)
         if (colorlist[cindex].color.pixel == cval)
	    break;
      if (cindex >= number_colors) {
	 Wprintf("Error: No such color!");
	 return;
      }
   }

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      labelptr curlabel = TOLABEL(EDITPART);
      strptr = findstringpart(areawin->textpos - 1, NULL, curlabel->string,
			areawin->topinstance);
      nextptr = findstringpart(areawin->textpos, NULL, curlabel->string,
			areawin->topinstance);
      if (strptr->type == FONT_COLOR) {
	 undrawtext(curlabel);
	 strptr->data.color = cindex;
	 redrawtext(curlabel);
      }
      else if (nextptr && nextptr->type == FONT_COLOR) {
	    undrawtext(curlabel);
	    nextptr->data.color = cindex;
	    redrawtext(curlabel);
      }
      else {
	 sprintf(_STR2, "%d", cindex);
	 labeltext(FONT_COLOR, (char *)&cindex);
      }
   }

   else if (areawin->selects > 0) {
      for (scolor = areawin->selectlist; scolor < areawin->selectlist
	   + areawin->selects; scolor++) {
	 ecolor = &(SELTOCOLOR(scolor));

	 *ecolor = cindex;
	 selected = True;
      }
   }

   setcolormark(cindex);
   if (!selected) {
      if (eventmode != TEXT_MODE && eventmode != ETEXT_MODE)
         areawin->color = cindex;      
      overdrawpixmap(w);
   }
}

/*----------------------------------------------------------------*/
/* Parse a new color entry and add it to the color list.	  */
/*----------------------------------------------------------------*/

void setnewcolor(xcWidget w, caddr_t nullptr)
{
   int ccolor, red, green, blue;
   char *ppos, *cpos;

   ppos = strchr(_STR2, '#');
   cpos = strchr(_STR2, ',');

   if (cpos != NULL || ppos != NULL) {
      if (cpos != NULL || strlen(ppos + 1) == 6) {
	 if (cpos != NULL)
            sscanf(_STR2, "%d, %d, %d", &red, &green, &blue);
	 else
            sscanf(ppos + 1, "%2x%2x%2x", &red, &green, &blue);
	 red *= 256;
	 green *= 256;
	 blue *= 256;
      }
      else if (sscanf(ppos + 1, "%4x%4x%4x", &red, &green, &blue) != 3) {
	 Wprintf("Bad color entry.  Use #rrggbb");
	 return;
      }
      ccolor = rgb_alloccolor(red, green, blue);
   }
   else {
      ccolor = xc_alloccolor(_STR2);
      addnewcolorentry(ccolor);
   }
}

/*----------------------------------------------------------------*/
/* Generate popup dialog for adding a new color name or RGB value */
/*----------------------------------------------------------------*/

void addnewcolor(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   buttonsave *savebutton;

   savebutton = getgeneric(w, addnewcolor, NULL);
   popupprompt(w, "Enter color name or #rgb or r,g,b:", "\0", setnewcolor,
	savebutton, NULL);
}

/*------------------------------------------------------*/

void setfontmarks(short fvalue, short jvalue)
{
   xcWidget w;
   Arg wargs[1];

   if ((fvalue >= 0) && (fontcount > 0)) {
      switch(fonts[fvalue].flags & 0x03) {
         case 0: w = StyleNormalButton; break;
         case 1: w = StyleBoldButton; break;
         case 2: w = StyleItalicButton; break;
         case 3: w = StyleBoldItalicButton; break;
      }
      toggleexcl(w, FontStyles, XtNumber(FontStyles));

      switch((fonts[fvalue].flags & 0xf80) >> 7) {
         case 0: w = EncodingStandardButton; break;
         case 2: w = EncodingISOLatin1Button; break;
	 default: w = NULL;
      }
      if (w != NULL) toggleexcl(w, FontEncodings, XtNumber(FontEncodings));

      togglefontmark(fvalue);
   }
   if (jvalue >= 0) {
      switch(jvalue & (RLANCHORFIELD)) {
         case NORMAL: w = AnchoringLeftAnchoredButton; break;
         case NOTLEFT: w = AnchoringCenterAnchoredButton; break;
         case RIGHT|NOTLEFT: w = AnchoringRightAnchoredButton; break;
      }
      toggleexcl(w, Anchors, XtNumber(Anchors));

      switch(jvalue & (TBANCHORFIELD)) {
         case NORMAL: w = AnchoringBottomAnchoredButton; break;
         case NOTBOTTOM: w = AnchoringMiddleAnchoredButton; break;
         case TOP|NOTBOTTOM: w = AnchoringTopAnchoredButton; break;
      }

      toggleexcl(w, Anchors, XtNumber(Anchors));

      /* Flip Invariance property */
      w = AnchoringFlipInvariantButton;
      if (jvalue & FLIPINV)
         XtSetArg(wargs[0], XtNsetMark, True);
      else
         XtSetArg(wargs[0], XtNsetMark, False);
      XtSetValues(w, wargs, 1);

      /* Pin visibility property */
      w = NetlistPinVisibilityButton;
      if (jvalue & PINVISIBLE)
         XtSetArg(wargs[0], XtNsetMark, True);
      else
         XtSetArg(wargs[0], XtNsetMark, False);
      XtSetValues(w, wargs, 1);
   }
}

/*----------------------------------------------*/
/* GUI wrapper for startparam()			*/
/*----------------------------------------------*/

void promptparam(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   buttonsave *popdata = (buttonsave *)malloc(sizeof(buttonsave));

   if (areawin->selects == 0) return;  /* nothing was selected */

   /* Get a name for the new object */

   eventmode = NORMAL_MODE;
   popdata->dataptr = NULL;
   popdata->button = NULL; /* indicates that no button is assc'd w/ the popup */
   popupprompt(w, "Enter name for new parameter:", "\0", stringparam, popdata, NULL);
}

/*---------------------------*/
/* Set polygon editing style */
/*---------------------------*/

void boxedit(xcWidget w, pointertype value, caddr_t nulldata)
{
   if (w == NULL) {
      switch (value) {
         case MANHATTAN: w = PolygonEditManhattanBoxEditButton; break;
         case RHOMBOIDX: w = PolygonEditRhomboidXButton; break;
	 case RHOMBOIDY: w = PolygonEditRhomboidYButton; break;
	 case RHOMBOIDA: w = PolygonEditRhomboidAButton; break;
	 case NORMAL: w = PolygonEditNormalButton; break;
      }
   }

   if (areawin->boxedit == value) return;

   toggleexcl(w, BoxEditStyles, XtNumber(BoxEditStyles));
   areawin->boxedit = value;
}

/*----------------------------------------------------*/
/* Generate popup dialog for entering a new font name */
/*----------------------------------------------------*/

void addnewfont(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   buttonsave *savebutton;
   char *tempstr = malloc(2 * sizeof(char));
   tempstr[0] = '\0';
   savebutton = getgeneric(w, addnewfont, tempstr);
   popupprompt(w, "Enter font name:", tempstr, locloadfont, savebutton, NULL);
}

/*-------------------------------------------------*/
/* Wrapper for labeltext when called from the menu */
/*-------------------------------------------------*/

void addtotext(xcWidget w, pointertype value, caddr_t nulldata)
{
   if (eventmode != TEXT_MODE && eventmode != ETEXT_MODE) return;
   if (value == (pointertype)SPECIAL)
      dospecial();
   else
      labeltext((int)value, (char *)1);
}

/*----------------------------------------------------------*/
/* Position a popup menu directly beside the toolbar button */
/*----------------------------------------------------------*/

void position_popup(xcWidget toolbutton, xcWidget menubutton)
{
   int n = 0;
   Arg wargs[2];
   Position pz, pw, ph;
   int dx, dy;

   xcWidget cascade = XtParent(menubutton);
   xcWidget pshell = XtParent(XtParent(cascade));

   XtnSetArg(XtNheight, &pz);
   XtGetValues(toolbutton, wargs, n); n = 0;

   XtnSetArg(XtNwidth, &pw);
   XtnSetArg(XtNheight, &ph);
   XtGetValues(cascade, wargs, n); n = 0;

   dx = -pw - 6;
   dy = (pz - ph) >> 1;

   XwPostPopup(pshell, cascade, toolbutton, dx, dy);
}

/*------------------------------------------------------*/
/* Functions which pop up a menu cascade in sticky mode */
/*------------------------------------------------------*/

void border_popup(xcWidget w, caddr_t clientdata, caddr_t calldata)
{ 
   position_popup(w, BorderLinewidthButton);
}

/*-------------------------------------------------------------------------*/

void color_popup(xcWidget w, caddr_t clientdata, caddr_t calldata)
{ 
   position_popup(w, ColorAddNewColorButton);
}

/*-------------------------------------------------------------------------*/

void fill_popup(xcWidget w, caddr_t clientdata, caddr_t calldata)
{ 
   position_popup(w, FillOpaqueButton);
}

/*-------------------------------------------------------------------------*/

void param_popup(xcWidget w, caddr_t clientdata, caddr_t calldata)
{ 
   position_popup(w, ParametersSubstringButton);
}

/*-------------------------------------------------------------------------*/

#endif /* TCL_WRAPPER */
