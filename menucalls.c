/*----------------------------------------------------------------------*/
/* menucalls.c --- callback routines from the menu buttons, and 	*/
/*		   associated routines (either Tcl/Tk routines or	*/
/*		   non-specific;  Xt routines split off in file		*/
/*		   xtfuncs.c 3/28/06)					*/
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	*/
/*----------------------------------------------------------------------*/

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
#endif

#ifdef TCL_WRAPPER
#include <tk.h>
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

#ifdef TCL_WRAPPER
extern Tcl_Interp *xcinterp;
#endif

/*----------------------------------------------------------------------*/
/* Local Variable definitions						*/
/*----------------------------------------------------------------------*/

u_short *fontnumbers;
u_char nfontnumbers;

/*----------------------------------------------*/
/* Set Poly and Arc line styles and fill styles */
/*----------------------------------------------*/

#define BORDERS  (NOBORDER | DOTTED | DASHED)
#define ALLFILLS (FILLSOLID | FILLED)

/*----------------------------------------------------------------*/
/* setgrid, getgridspace are for grid and snap spacing sizes;	  */
/* include routines to parse fractions				  */
/*----------------------------------------------------------------*/

void setgrid(xcWidget w, float *dataptr)
{
   float oldvalue = *dataptr;
   float oscale, iscale = (float)xobjs.pagelist[areawin->page]->drawingscale.y /
        (float)xobjs.pagelist[areawin->page]->drawingscale.x;
   float fval;

   /* For now, assume that the value is in the current display style. */
   /* Might be nice in the future to make it accept any input. . .    */

   switch (xobjs.pagelist[areawin->page]->coordstyle) {
      case INTERNAL:
	 if (sscanf(_STR2, "%f", &fval) == 0) {
	    *dataptr = oldvalue;
	    Wprintf("Illegal value");
	 }
	 else *dataptr = fval / iscale;
	 break;
      case CM:
         oscale = xobjs.pagelist[areawin->page]->outscale * CMSCALE;
	 if (sscanf(_STR2, "%f", &fval) == 0) {
	    *dataptr = oldvalue;
	    Wprintf("Illegal value");
	 }
	 else *dataptr = fval * IN_CM_CONVERT / (iscale * oscale);
	 break;
      case DEC_INCH: case FRAC_INCH: {
	 short parts;
	 char *sptr;
	 int f2, f3;

         oscale = xobjs.pagelist[areawin->page]->outscale * INCHSCALE;
	 for (sptr = _STR2; *sptr != '\0'; sptr++)
	    if (*sptr == '/') *sptr = ' ';
	 parts = sscanf(_STR2, "%f %d %d", &fval, &f2, &f3);
	 if ((parts == 0) || (parts != 1 && (fval != (float)((int)fval)))) {
	    *dataptr = oldvalue;
	    Wprintf("Illegal value");
	    break;
	 }
	 if (parts == 2) fval /= (float)f2;
	 else if (parts == 3) fval += ((float)f2 / (float)f3);
	 *dataptr = fval * 72.0 / (iscale * oscale);
	 } break;
   }
   if (oldvalue != *dataptr) drawarea(NULL, NULL, NULL);
}

/*----------------------------------------------------------------*/
/* Write a measurement value into string "buffer" dependant on    */
/* the current default units of measure (centimeters or inches).  */
/*----------------------------------------------------------------*/

void measurestr(float value, char *buffer)
{  
   float oscale, iscale;
   iscale = (float)(xobjs.pagelist[areawin->page]->drawingscale.y) /
        (float)(xobjs.pagelist[areawin->page]->drawingscale.x);

   switch (xobjs.pagelist[areawin->page]->coordstyle) {
      case INTERNAL:
	 sprintf(buffer, "%5.3f", value * iscale);
         break; 
      case CM:
         oscale = xobjs.pagelist[areawin->page]->outscale * CMSCALE;
         sprintf(buffer, "%5.3f cm", value * iscale * oscale / IN_CM_CONVERT);
         break; 
      case DEC_INCH:
         oscale = xobjs.pagelist[areawin->page]->outscale * INCHSCALE;
         sprintf(buffer, "%5.3f in", value * iscale * oscale / 72.0);
         break;
      case FRAC_INCH:
         oscale = xobjs.pagelist[areawin->page]->outscale * INCHSCALE;
         fraccalc(((value * iscale * oscale) / 72.0), buffer);
         strcat(buffer, " in");
         break;
   }
}        

/*----------------------------------------------------------------*/
/* set the global default line width.  The unit used internally   */
/* is twice the value passed through pointer "dataptr".		  */
/*----------------------------------------------------------------*/

void setwidth(xcWidget w, float *dataptr)
{
   float oldvalue = *dataptr;
   if (sscanf(_STR2, "%f", dataptr) == 0) {
      *dataptr = oldvalue;
      Wprintf("Illegal value");
      return;
   }
   (*dataptr) *= 2.0;
   if (oldvalue != *dataptr) drawarea(NULL, NULL, NULL);
}

/*--------------------------------------------------------------*/
/* Set text scale.						*/
/*--------------------------------------------------------------*/

void changetextscale(float newscale)
{
   short *osel;
   labelptr settext;
   stringpart *strptr, *nextptr;

   /* In edit mode, add font scale change. */

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      settext = *((labelptr *)EDITPART);
      if (areawin->textpos > 0 || areawin->textpos < stringlength(settext->string, True,
		areawin->topinstance)) {
	 undrawtext(settext);
	 strptr = findstringpart(areawin->textpos - 1, NULL, settext->string,
			areawin->topinstance);
	 nextptr = findstringpart(areawin->textpos, NULL, settext->string,
			areawin->topinstance);
	 if (strptr->type == FONT_SCALE)
	    strptr->data.scale = newscale;
	 else if (nextptr && nextptr->type == FONT_SCALE)
	    nextptr->data.scale = newscale;
	 else
	    labeltext(FONT_SCALE, (char *)&newscale);
	 redrawtext(settext);
      }
      else if (stringlength(settext->string, True, areawin->topinstance) > 0)
	 labeltext(FONT_SCALE, (char *)&newscale);
      else (settext->scale = newscale);
   }

   /* Change scale on all selected text objects */

   else if (areawin->selects > 0) {
      float oldscale;
      Boolean waschanged = FALSE;
      for (osel = areawin->selectlist; osel < areawin->selectlist +
	     areawin->selects; osel++) {
	 if (SELECTTYPE(osel) == LABEL) {
	    settext = SELTOLABEL(osel);
	    oldscale = settext->scale;
	    if (oldscale != newscale) {
	       undrawtext(settext);
               settext->scale = newscale;
	       redrawtext(settext);
	       register_for_undo(XCF_Rescale, UNDO_MORE, areawin->topinstance,
			(genericptr)settext, (double)oldscale);
	       waschanged = TRUE;
	    }
	 }
      }
      if (waschanged) undo_finish_series();
   }
}

/*--------------------------------------------------------------*/
/* Auto-scale the drawing to fit the declared page size.	*/
/*								*/
/* If the page is declared encapsulated, then do nothing.	*/
/* If a frame box is on the page, then scale to fit the frame	*/
/* to the declared page size, not the whole object.		*/
/*--------------------------------------------------------------*/

void autoscale(int page)
{
   float newxscale, newyscale;
   float scalefudge = (xobjs.pagelist[page]->coordstyle
	== CM) ? CMSCALE : INCHSCALE;
   int width, height;
   polyptr framebox;

   /* Check if auto-fit flag is selected */
   if (!(xobjs.pagelist[page]->pmode & 2)) return;
   /* Ignore auto-fit flag in EPS mode */
   if (!(xobjs.pagelist[page]->pmode & 1)) return;

   else if (topobject->bbox.width == 0 || topobject->bbox.height == 0) {
      // Wprintf("Cannot auto-fit empty page");
      return;
   }

   newxscale = (xobjs.pagelist[page]->pagesize.x -
	(2 * xobjs.pagelist[page]->margins.x)) / scalefudge;
   newyscale = (xobjs.pagelist[page]->pagesize.y -
	(2 * xobjs.pagelist[page]->margins.y)) / scalefudge;

   if ((framebox = checkforbbox(topobject)) != NULL) {
      int i, minx, miny, maxx, maxy;
      
      minx = maxx = framebox->points[0].x;
      miny = maxy = framebox->points[0].y;
      for (i = 1; i < framebox->number; i++) {
	 if (framebox->points[i].x < minx) minx = framebox->points[i].x;
	 else if (framebox->points[i].x > maxx) maxx = framebox->points[i].x;
	 if (framebox->points[i].y < miny) miny = framebox->points[i].y;
	 else if (framebox->points[i].y > maxy) maxy = framebox->points[i].y;
      }
      width = (maxx - minx);
      height = (maxy - miny);
   }
   else {

      width = toplevelwidth(areawin->topinstance, NULL);
      height = toplevelheight(areawin->topinstance, NULL);
   }

   if (xobjs.pagelist[page]->orient == 0) {	/* Portrait */
      newxscale /= width;
      newyscale /= height;
   }
   else {
      newxscale /= height;
      newyscale /= width;
   }
   xobjs.pagelist[page]->outscale = min(newxscale, newyscale);
}

/*--------------------------------------------------------------*/
/* Parse a string for possible units of measure.  Convert to	*/ 
/* current units of measure, if necessary.  Return the value	*/
/* in current units, as a type float.				*/
/*--------------------------------------------------------------*/

float parseunits(char *strptr)
{
   short curtype;
   Boolean inchunits = True;
   float pv;
   char units[12];

   curtype = xobjs.pagelist[areawin->page]->coordstyle;

   if (sscanf(strptr, "%f %11s", &pv, units) < 2)
      return pv;
   else {
      if (!strncmp(units, "cm", 2) || !strncmp(units, "centimeters", 11))
	 inchunits = False;
      switch(curtype) {
         case CM:
	    return ((inchunits) ? (pv * 2.54) : pv);
         default:
	    return ((inchunits) ? pv : (pv / 2.54));
      }
   }
}

/*--------------------------------------------------------------*/
/* Set the output page size, in the current unit of measure	*/
/* Return value:  TRUE if _STR2 values were in inches, FALSE	*/
/* if in centimeters.						*/
/* XXX This API gives no good way to signal errors.             */
/*--------------------------------------------------------------*/

Boolean setoutputpagesize(XPoint *dataptr)
{
   float px, py;
   char units[10], *expos;
#ifndef TCL_WRAPPER
   Arg wargs[1];
#endif

   strcpy(units, "in");

   if (sscanf(_STR2, "%f %*c %f %9s", &px, &py, units) < 4) {
      if (sscanf(_STR2, "%f %*c %f", &px, &py) < 3) {
	 if ((expos = strchr(_STR2, 'x')) == NULL) {
            Wprintf("Illegal Form for page size.");
	    return FALSE;
	 }
	 else {
	    *expos = '\0';
	    if (sscanf(_STR2, "%f", &px) == 0 ||
		  sscanf(expos + 1, "%f %9s", &py, units) == 0) {
               Wprintf("Illegal Form for page size.");
	       return FALSE;
	    }
	 }
      }
   }

   /* Don't reduce page to less than the margins (1") or negative	*/
   /* scales result.							*/

   if ((px <= 2.0) || (py <= 2.0)) {
      Wprintf("Page size too small for margins.");
      return FALSE;
   }

   dataptr->x = (short)(px * 72.0);
   dataptr->y = (short)(py * 72.0);

   if (!strcmp(units, "cm")) {
      dataptr->x /= 2.54;
      dataptr->y /= 2.54;
      return FALSE;
   }
   return TRUE;
}

/*--------------------------------------------------------------*/
/* Get the text size (global or selected, depending on mode	*/
/*--------------------------------------------------------------*/

labelptr gettextsize(float **floatptr)
{
   labelptr settext = NULL;
   short    *osel;
   stringpart *strptr, *nextptr;
   const float f_one = 1.00;

   if (floatptr) *floatptr = &areawin->textscale;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      if (areawin->textpos > 0 || areawin->textpos < stringlength(settext->string,
		True, areawin->topinstance)) {
         settext = *((labelptr *)EDITPART);
	 strptr = findstringpart(areawin->textpos - 1, NULL, settext->string,
			areawin->topinstance);
	 nextptr = findstringpart(areawin->textpos, NULL, settext->string,
			areawin->topinstance);
	 if (strptr->type == FONT_SCALE) {
	    if (floatptr) *floatptr = &strptr->data.scale;
	 }
	 else if (nextptr && nextptr->type == FONT_SCALE) {
	    if (floatptr) *floatptr = &nextptr->data.scale;
	 }
	 else if (floatptr) *floatptr = (float *)(&f_one);
      }
      else {
         settext = *((labelptr *)EDITPART);
         if (floatptr) *floatptr = &(settext->scale);
      }
   }
   else if (areawin->selects > 0) {
      for (osel = areawin->selectlist; osel < areawin->selectlist +
		areawin->selects; osel++) {
	 if (SELECTTYPE(osel) == LABEL) {
            settext = SELTOLABEL(osel);
            if (floatptr) *floatptr = &(settext->scale);
	    break;
	 }
      }
   }
   return settext;
}

/*--------------------------------------------------------------*/
/* Set a character kern value					*/ 
/*--------------------------------------------------------------*/

void setkern(xcWidget w, stringpart *kpart)
{
   char *sptr;
   short kd[2];

   kd[0] = kd[1] = 0;

   if ((sptr = strchr(_STR2, ',')) == NULL)
      Wprintf("Use notation X,Y");
   else {
      *sptr = '\0';
      sscanf(_STR2, "%hd", &kd[0]);
      sscanf(sptr + 1, "%hd", &kd[1]);
      if (kpart == NULL)
         labeltext(KERN, (char *)kd);
      else {
         labelptr curlabel = TOLABEL(EDITPART);
	 undrawtext(curlabel);
	 kpart->data.kern[0] = kd[0];
	 kpart->data.kern[1] = kd[1];
	 redrawtext(curlabel);
      }
   }
}

/*----------------------------------------------------------------*/
/* Set the drawing scale (specified as ratio X:Y)		  */
/*----------------------------------------------------------------*/

void setdscale(xcWidget w, XPoint *dataptr)
{
   char *sptr;

   if ((sptr = strchr(_STR2, ':')) == NULL)
      Wprintf("Use ratio X:Y");
   else {
      *sptr = '\0';
      sscanf(_STR2, "%hd", &(dataptr->x));
      sscanf(sptr + 1, "%hd", &(dataptr->y));
      Wprintf("New scale is %hd:%hd", dataptr->x, dataptr->y);
      W1printf(" ");
   }
}

/*----------------------------------------------------------------*/
/* Set the scale of an object or group of selected objects	  */
/*----------------------------------------------------------------*/

void setosize(xcWidget w, objinstptr dataptr)
{
   float tmpres, oldsize;
   Boolean waschanged = FALSE;
   short *osel;
   objinstptr nsobj;
   int res = sscanf(_STR2, "%f", &tmpres);

   // Negative values are flips---deal with them independently

   if (tmpres < 0)
      tmpres = -tmpres;

   if (res == 0 || tmpres == 0) {
      Wprintf("Illegal value");
      return;
   }
   for (osel = areawin->selectlist; osel < areawin->selectlist +
	     areawin->selects; osel++) {
      if (SELECTTYPE(osel) == OBJINST) {
	 nsobj = SELTOOBJINST(osel);
	 oldsize = nsobj->scale;
         nsobj->scale = (oldsize < 0) ? -tmpres : tmpres;

         if (oldsize != tmpres) {
            register_for_undo(XCF_Rescale, UNDO_MORE, areawin->topinstance,
			SELTOGENERIC(osel), (double)oldsize);
	    waschanged = TRUE;
	 }
      }
   }
   /* unselect_all(); */
   if (waschanged) undo_finish_series();
   pwriteback(areawin->topinstance);
   drawarea(NULL, NULL, NULL);
}

/*----------------------------------------------------------------*/
/* Set the linewidth of all selected arcs, polygons, splines, and */
/* paths.							  */
/*----------------------------------------------------------------*/

void setwwidth(xcWidget w, void *dataptr)
{
   float     tmpres, oldwidth;
   short     *osel;
   arcptr    nsarc;
   polyptr   nspoly;
   splineptr nsspline;
   pathptr   nspath;

   if (sscanf(_STR2, "%f", &tmpres) == 0) {
      Wprintf("Illegal value");
      return;
   }
   else if (areawin->selects == 0) {
      areawin->linewidth = tmpres;
   }
   else {
      for (osel = areawin->selectlist; osel < areawin->selectlist +
	     areawin->selects; osel++) {
         if (SELECTTYPE(osel) == ARC) {
	    nsarc = SELTOARC(osel);
	    oldwidth = nsarc->width;
            nsarc->width = tmpres;
         }
         else if (SELECTTYPE(osel) == POLYGON) {
	    nspoly = SELTOPOLY(osel);
	    oldwidth = nspoly->width;
            nspoly->width = tmpres;
         }
         else if (SELECTTYPE(osel) == SPLINE) {
	    nsspline = SELTOSPLINE(osel);
	    oldwidth = nsspline->width;
            nsspline->width = tmpres;
         }
         else if (SELECTTYPE(osel) == PATH) {
	    nspath = SELTOPATH(osel);
	    oldwidth = nspath->width;
            nspath->width = tmpres;
         }

	 if (oldwidth != tmpres)
            register_for_undo(XCF_Rescale, UNDO_MORE, areawin->topinstance,
			SELTOGENERIC(osel), (double)oldwidth);
      }
      unselect_all();
      pwriteback(areawin->topinstance);
      drawarea(NULL, NULL, NULL);
   }
}

/*--------------------------------------------------------------*/
/* Add a new font name to the list of known fonts		*/
/* Register the font number for the Alt-F cycling mechanism	*/
/* Tcl: depends on command tag mechanism for GUI menu update.	*/
/*--------------------------------------------------------------*/

#ifdef TCL_WRAPPER

void makenewfontbutton()
{
   nfontnumbers++;
   if (nfontnumbers == 1)
      fontnumbers = (u_short *)malloc(sizeof(u_short)); 
   else
      fontnumbers = (u_short *)realloc(fontnumbers, nfontnumbers
		* sizeof(u_short));
   fontnumbers[nfontnumbers - 1] = fontcount - 1;
}

#endif /* TCL_WRAPPER */

/*---------------------------------------------------------------*/
/* Some Xcircuit routines using toggle and toggleexcl, 		 */
/* put here because they reference the menu structures directly. */
/*								 */
/* Note that by bypassing addnewcolorentry(), the new colors in	 */
/* colorlist are NOT added to the GUI list of colors.		 */
/*---------------------------------------------------------------*/

void setcolorscheme(Boolean boolvalue)
{
   int i;

   if (boolvalue) {
      colorlist[PARAMCOLOR].color.pixel = appdata.parampix;
      colorlist[AUXCOLOR].color.pixel = appdata.auxpix;
      colorlist[OFFBUTTONCOLOR].color.pixel = appdata.buttonpix;
      colorlist[SELECTCOLOR].color.pixel = appdata.selectpix;
      colorlist[GRIDCOLOR].color.pixel = appdata.gridpix;
      colorlist[SNAPCOLOR].color.pixel = appdata.snappix;
      colorlist[AXESCOLOR].color.pixel = appdata.axespix;
      colorlist[BACKGROUND].color.pixel = appdata.bg;
      colorlist[FOREGROUND].color.pixel = appdata.fg;
   }
   else {
      colorlist[PARAMCOLOR].color.pixel = appdata.parampix2;
      colorlist[AUXCOLOR].color.pixel = appdata.auxpix2;
      colorlist[OFFBUTTONCOLOR].color.pixel = appdata.buttonpix2;
      colorlist[SELECTCOLOR].color.pixel = appdata.selectpix2;
      colorlist[GRIDCOLOR].color.pixel = appdata.gridpix2;
      colorlist[SNAPCOLOR].color.pixel = appdata.snappix2;
      colorlist[AXESCOLOR].color.pixel = appdata.axespix2;
      colorlist[BACKGROUND].color.pixel = appdata.bg2;
      colorlist[FOREGROUND].color.pixel = appdata.fg2;
   }

   colorlist[BARCOLOR].color.pixel = appdata.barpix;
   colorlist[FILTERCOLOR].color.pixel = appdata.filterpix;

   colorlist[LOCALPINCOLOR].color.pixel = appdata.localcolor;
   colorlist[GLOBALPINCOLOR].color.pixel = appdata.globalcolor;
   colorlist[INFOLABELCOLOR].color.pixel = appdata.infocolor;
   colorlist[RATSNESTCOLOR].color.pixel = appdata.ratsnestcolor;
   colorlist[BBOXCOLOR].color.pixel = appdata.bboxpix;
   colorlist[CLIPMASKCOLOR].color.pixel = appdata.clipcolor;
   colorlist[FIXEDBBOXCOLOR].color.pixel = appdata.fixedbboxpix;

   /* Fill in pixel information */

   for (i = 0; i < NUMBER_OF_COLORS; i++) {
      unsigned short r, g, b;

      /* Get the color the hard way by querying the X server colormap */
      xc_get_color_rgb(colorlist[i].color.pixel, &r, &g, &b);

      /* Store this information locally so we don't have to do	*/
      /* the lookup the hard way in the future.			*/

      colorlist[i].color.red = r;
      colorlist[i].color.green = g;
      colorlist[i].color.blue = b;
   }
   areawin->redraw_needed = True;
   drawarea(NULL, NULL, NULL);
}

/*----------------------------------------------------------------*/
/* Change menu selection for reported measurement units		  */
/*----------------------------------------------------------------*/

#ifdef TCL_WRAPPER

void togglegrid(u_short type)
{
   static char *stylenames[] = {
      "decimal inches",
      "fractional inches",
      "centimeters",
      "internal units", NULL
   };

   XcInternalTagCall(xcinterp, 3, "config", "coordstyle", stylenames[type]);
}

#endif /* TCL_WRAPPER */

/*----------------------------------------------------------------*/
/* Called by setgridtype() to complete setting the reported 	  */
/* measurement units						  */
/*----------------------------------------------------------------*/

void getgridtype(xcWidget button, pointertype value, caddr_t calldata)
{
   short oldtype = xobjs.pagelist[areawin->page]->coordstyle;
   float scalefac = getpsscale(1.0, areawin->page) / INCHSCALE;

#ifndef TCL_WRAPPER
   togglegridstyles(button);
#endif
   xobjs.pagelist[areawin->page]->coordstyle = (short)value;

   switch(value) {
      case FRAC_INCH: case DEC_INCH: case INTERNAL:
	 if (oldtype == CM) {
            xobjs.pagelist[areawin->page]->outscale *= scalefac;
#ifndef TCL_WRAPPER
	    /* Note:  Tcl defines a method for selecting standard  */
	    /* page sizes.  We really DON'T want to reset the size */
	    /* just because we switched measurement formats!	   */

	    xobjs.pagelist[areawin->page]->pagesize.x = 612;
	    xobjs.pagelist[areawin->page]->pagesize.y = 792; /* letter */
#endif
	 }
	 break;
      case CM:
	 if (oldtype != CM) {
            xobjs.pagelist[areawin->page]->outscale *= scalefac;
#ifndef TCL_WRAPPER
	    xobjs.pagelist[areawin->page]->pagesize.x = 595;
	    xobjs.pagelist[areawin->page]->pagesize.y = 842; /* A4 */
#endif
	 }
	 break;
   }
   if (oldtype != xobjs.pagelist[areawin->page]->coordstyle) {
      drawarea(NULL, NULL, NULL);
      W1printf(" ");
   }
}

/*------------------------------------------------------*/
/* Make new library, add new button to the "Libraries"	*/
/* cascaded menu, compose the library page, update the  */
/* library directory, and go to that library page.	*/
/*------------------------------------------------------*/

void newlibrary(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   int libnum = createlibrary(FALSE);
   startcatalog(w, libnum, NULL);
}

/*----------------------------------------------*/
/* Find an empty library, and return its page	*/
/* number if it exists.  Otherwise, return -1.	*/
/* This search does not include the so-called	*/
/* "User Library" (last library in list).	*/
/*----------------------------------------------*/

int findemptylib()
{
  int i;

  for (i = 0; i < xobjs.numlibs - 1; i++) {
     if (xobjs.userlibs[i].number == 0)
	return i;
  }
  return -1;
}

/*----------------------------------------------*/
/* Make new page; goto that page		*/
/* (wrapper for routine events.c:newpage())	*/
/*----------------------------------------------*/

void newpagemenu(xcWidget w, pointertype value, caddr_t nulldata)
{
   newpage((short)value);
}

#ifdef TCL_WRAPPER

/*----------------------------------------------*/
/* Make new library and add a new button to the */
/* "Libraries" cascaded menu.			*/
/*----------------------------------------------*/

int createlibrary(Boolean force)
{
  /* xcWidget libmenu, newbutton, oldbutton; (jdk) */
#ifndef TCL_WRAPPER
   Arg wargs[2];
#endif
   /* char libstring[20]; (jdk) */
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

   sprintf(_STR2, "xcircuit::newlibrarybutton \"%s\"", newlibobj->name);
   Tcl_Eval(xcinterp, _STR2);

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
  /* xcWidget pagemenu, newbutton; (jdk) */
  /* char pagestring[10]; (jdk) */

   /* make new entry in the menu */

   sprintf(_STR2, "newpagebutton \"Page %d\"", xobjs.pages);
   Tcl_Eval(xcinterp, _STR2);

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
   objinstptr thisinst = xobjs.pagelist[pagenumber]->pageinst;
   char *pname, *plabel;

   if ((pagenumber >= 0) && (pagenumber < xobjs.pages - 1) &&
	    (thisinst != NULL)) {
      plabel = thisinst->thisobject->name;
      pname = (char *)malloc(28 + strlen(plabel));
      sprintf(pname, "xcircuit::renamepage %d {%s}", pagenumber + 1, plabel);
      Tcl_Eval(xcinterp, pname);
      free(pname);
   }
}

/*--------------------------------------------------------------*/
/* Same routine as above, for Library page menu buttons		*/
/*--------------------------------------------------------------*/

void renamelib(short libnumber)
{
   if (libnumber <= xobjs.numlibs) return;

   sprintf(_STR2, "xcircuit::renamelib %d \"%s\"", libnumber - LIBRARY + 1,
	xobjs.libtop[libnumber]->thisobject->name);
   Tcl_Eval(xcinterp, _STR2);
}

/*--------------------------------------------------------------*/
/* Set the menu checkmarks on the color menu			*/
/*--------------------------------------------------------------*/

void setcolormark(int colorval)
{
   /* Set GUI variables and execute any command tags associated */
   /* with the "color" command */
  
#ifdef TCL_WRAPPER
   char cstr[6];

   if (colorval != DEFAULTCOLOR)
      sprintf(cstr, "%5d", colorval);

   XcInternalTagCall(xcinterp, 3, "color", "set", (colorval == DEFAULTCOLOR) ?
	"inherit" : cstr);
#endif
}

/*----------------------------------------------------------------*/
/* Set the checkmarks on the element styles menu		  */
/*----------------------------------------------------------------*/

void setallstylemarks(u_short styleval)
{
   /* Execute any command tags associated	*/
   /* with the "fill" and "border" commands.	*/

   char fstr[10];
   int fillfactor;
   const char *bptr;

   const char *borders[] = {"solid", "unbordered", "dashed", "dotted", NULL};
   enum BorderIdx { SolidIdx, UnborderedIdx, DashedIdx, DottedIdx };

   if (styleval & FILLED) {
      fillfactor = (int)(12.5 * (float)(1 + ((styleval & FILLSOLID) >> 5)));
      if (fillfactor < 100)
	 sprintf(fstr, "%d", fillfactor);
      else
	 strcpy(fstr, "solid");
   }
   else
      strcpy(fstr, "unfilled");

   switch (styleval & BORDERS) {
      case DASHED:
	 bptr = borders[DashedIdx];
	 break;
      case DOTTED:
	 bptr = borders[DottedIdx];
	 break;
      case NOBORDER:
	 bptr = borders[UnborderedIdx];
	 break;
      default:
	 bptr = borders[SolidIdx];
	 break;
   }

   XcInternalTagCall(xcinterp, 3, "fill", fstr,
		(styleval & OPAQUE) ? "opaque" : "transparent");
   XcInternalTagCall(xcinterp, 3, "border", "bbox", (styleval & BBOX) ? "true" :
		"false");
   XcInternalTagCall(xcinterp, 3, "border", "clipmask", (styleval & CLIPMASK) ?
		"true" : "false");
   XcInternalTagCall(xcinterp, 2, "border", (styleval & UNCLOSED) ? "unclosed" :
		"closed");
   XcInternalTagCall(xcinterp, 2, "border", bptr);
}

#endif /* TCL_WRAPPER */

/*--------------------------------------------------------------*/
/* Check for a bounding box polygon				*/
/*--------------------------------------------------------------*/

polyptr checkforbbox(objectptr localdata)
{
   polyptr *cbbox;

   for (cbbox = (polyptr *)localdata->plist; cbbox <
	(polyptr *)localdata->plist + localdata->parts; cbbox++)
      if (IS_POLYGON(*cbbox))
         if ((*cbbox)->style & BBOX) return *cbbox;

   return NULL;
}

/*--------------------------------------------------------------*/
/* Set a value for element style.  "Mask" determines the bits	*/
/* to be affected, so that "value" may turn bits either on or	*/
/* off.								*/
/*--------------------------------------------------------------*/

int setelementstyle(xcWidget w, u_short value, u_short mask)
{
   Boolean preselected, selected = False;
   short *sstyle;
   u_short newstyle, oldstyle;

   if (areawin->selects == 0) {
      preselected = FALSE;
      if (value & BBOX)
	 checkselect(POLYGON);
      else
	 checkselect(ARC | SPLINE | POLYGON | PATH);
   }
   else preselected = TRUE;

   if (areawin->selects > 0) {
      if (value & BBOX) {
	 polyptr ckp;
	 if (areawin->selects != 1) {
	    Wprintf("Choose only one polygon to be the bounding box");
	    return -1;
	 }
	 else if (SELECTTYPE(areawin->selectlist) != POLYGON) {
	    Wprintf("Bounding box can only be a polygon");
	    return -1;
	 }
	 else if (((ckp = checkforbbox(topobject)) != NULL) &&
		(ckp != SELTOPOLY(areawin->selectlist))) {
	    Wprintf("Only one bounding box allowed per page");
	    return -1;
	 }
      }

      for (sstyle = areawin->selectlist; sstyle < areawin->selectlist
	   + areawin->selects; sstyle++) {
	 short stype = SELECTTYPE(sstyle);
	 if (stype & (ARC | POLYGON | SPLINE | PATH)) {
	    short *estyle;
	    switch (stype) {
	       case ARC:
	          estyle = &((SELTOARC(sstyle))->style);
		  break;
	       case SPLINE:
	          estyle = &((SELTOSPLINE(sstyle))->style);
		  break;
	       case POLYGON:
	          estyle = &((SELTOPOLY(sstyle))->style);
		  break;
	       case PATH:
	          estyle = &((SELTOPATH(sstyle))->style);
		  break;
	    }
	    oldstyle = newstyle = *estyle;
	    newstyle &= ~(mask);
	    newstyle |= value;

	    if (oldstyle != newstyle) {
	       if ((newstyle & NOBORDER) && !(newstyle & FILLED)) {
	          Wprintf("Must have either a border or filler");
	          continue;
	       }

	       SetForeground(dpy, areawin->gc, BACKGROUND);
	       easydraw(*sstyle, DOFORALL);

	       *estyle = newstyle;
	       if (mask & BBOX)
	          (SELTOPOLY(sstyle))->color = (value & BBOX) ? BBOXCOLOR
				: DEFAULTCOLOR;

	       SetForeground(dpy, areawin->gc, SELECTCOLOR);
	       easydraw(*sstyle, DOFORALL);

#ifdef TCL_WRAPPER
	       register_for_undo(XCF_ChangeStyle,
			(sstyle == areawin->selectlist + areawin->selects - 1) ?
			UNDO_DONE : UNDO_MORE, areawin->topinstance,
			SELTOGENERIC(sstyle), (int)oldstyle);
#else
	       /* Tcl version differs in that the element is not deselected after */

	       register_for_undo(XCF_ChangeStyle, UNDO_MORE, areawin->topinstance,
			SELTOGENERIC(sstyle), (int)oldstyle);
#endif
	    }
	    selected = True;
	 }
      }
   }
   if (selected)
      pwriteback(areawin->topinstance);
   else {
      newstyle = areawin->style;
      if (value & BBOX) {
	 Wprintf("Cannot set default style to Bounding Box");
	 newstyle &= ~(BBOX);
	 return -1;
      }
      else if (value & CLIPMASK) {
	 Wprintf("Cannot set default style to Clip Mask");
	 newstyle &= ~(CLIPMASK);
	 return -1;
      }
      else {
	 newstyle &= ~mask;
	 newstyle |= value;
      }

      if ((newstyle & NOBORDER) && !(newstyle & FILLED)) {
	 Wprintf("Must have either a border or filler");
	 return -1;
      }
      areawin->style = newstyle;
#ifndef TCL_WRAPPER
      overdrawpixmap(w);
#endif
   }
#ifndef TCL_WRAPPER
   setallstylemarks(newstyle);
#endif
   if (!preselected)
      unselect_all();

   return (int)newstyle;
}

/*-----------------------------------------------*/
/* Set the color value for all selected elements */
/*-----------------------------------------------*/

#ifdef TCL_WRAPPER

void setcolor(xcWidget w, int cindex)
{
   short *scolor;
   int *ecolor, oldcolor;
   Boolean selected = False;
   stringpart *strptr, *nextptr;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      labelptr curlabel = TOLABEL(EDITPART);
      strptr = findstringpart(areawin->textpos - 1, NULL, curlabel->string,
			areawin->topinstance);
      nextptr = findstringpart(areawin->textpos, NULL, curlabel->string,
			areawin->topinstance);
      if (strptr && strptr->type == FONT_COLOR) {
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
	 oldcolor = *ecolor;
	 *ecolor = cindex;
	 selected = True;

	 register_for_undo(XCF_Color, (scolor == areawin->selectlist
			+ areawin->selects - 1) ?  UNDO_DONE : UNDO_MORE,
			areawin->topinstance,
			SELTOGENERIC(scolor), (int)oldcolor);
      }
   }

   setcolormark(cindex);
   if (!selected) {
      if (eventmode != TEXT_MODE && eventmode != ETEXT_MODE)
         areawin->color = cindex;      
   }
   else pwriteback(areawin->topinstance);
}

/*--------------------------------------------------------------*/
/* Set the menu checkmarks on the font menu			*/
/*--------------------------------------------------------------*/

void togglefontmark(int fontval)
{
   if (fonts[fontval].family != NULL)
      XcInternalTagCall(xcinterp, 3, "label", "family", fonts[fontval].family);
}

/*------------------------------------------------------*/
/* Set checkmarks on label style menu 			*/
/* fvalue is for font, jvalue is for anchoring		*/
/*------------------------------------------------------*/

void togglestylemark(int styleval)
{
   char *cstyle = translatestyle(styleval);
   if (cstyle != NULL)
      XcInternalTagCall(xcinterp, 3, "label", "style", cstyle);
}

/*------------------------------------------------------*/
/* Set checkmarks on label encoding menu		*/
/*------------------------------------------------------*/

void toggleencodingmark(int encodingval)
{
   char *cenc = translateencoding(encodingval);
   if (cenc != NULL)
      XcInternalTagCall(xcinterp, 3, "label", "encoding", cenc);
}

/*------------------------------------------------------*/
/* Set checkmarks on label anchoring & flags menu	*/
/*------------------------------------------------------*/

void toggleanchormarks(int anchorvalue)
{
   XcInternalTagCall(xcinterp, 4, "label", "anchor", (anchorvalue & RIGHT) ? "right" :
	(anchorvalue & NOTLEFT) ? "center" : "left",
	(anchorvalue & TOP) ? "top" : (anchorvalue & NOTBOTTOM) ? "middle" : "bottom");
   XcInternalTagCall(xcinterp, 3, "label", "justify", (anchorvalue & JUSTIFYRIGHT) ?
	"right" : (anchorvalue & TEXTCENTERED) ? "center" : (anchorvalue & JUSTIFYBOTH)
	? "both" : "left");
   XcInternalTagCall(xcinterp, 3, "label", "flipinvariant", (anchorvalue & FLIPINV) ?
	"true" : "false");
   XcInternalTagCall(xcinterp, 3, "label", "latex", (anchorvalue & LATEXLABEL) ?
	"true" : "false");
   XcInternalTagCall(xcinterp, 3, "label", "visible", (anchorvalue & PINVISIBLE) ?
	"true" : "false");
}

/*-------------------------------------------------------------*/
/* Simultaneously set all relevant checkmarks for a text label */
/*-------------------------------------------------------------*/

void setfontmarks(short fvalue, short jvalue)
{
   if ((fvalue >= 0) && (fvalue < fontcount)) {
      toggleencodingmark(fvalue);
      togglestylemark(fvalue);
      togglefontmark(fvalue);
   }
   toggleanchormarks(jvalue);
}

#endif /* TCL_WRAPPER */

/*--------------------------------------------------------------*/
/* Parameterize a label string (wrapper for parameterize()).	*/
/* Assumes that the name has been generated by the popup prompt	*/
/* and is therefore held in variable _STR2			*/
/*--------------------------------------------------------------*/

void stringparam(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   genericptr *settext;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      settext = (genericptr *)EDITPART;
      makeparam(TOLABEL(settext), _STR2);
      unselect_all();
      setparammarks(NULL);
   }
   else if (checkselect(LABEL)) parameterize(P_SUBSTRING, _STR2, -1);
}

/*--------------------------------------------------------------*/
/* Numerical parameterization (wrapper for parameterize()).	*/
/*--------------------------------------------------------------*/

void startparam(xcWidget w, pointertype value, caddr_t calldata)
{
   if (value == (pointertype)P_SUBSTRING) {
      strcpy(_STR2, (calldata != NULL) ? (char *)calldata : "substring");
      stringparam(w, NULL, NULL);
   }
   else if ((eventmode != NORMAL_MODE) || (areawin->selects > 0))
	 parameterize((int)value, (char *)calldata, -1);
}

/*---------------------------------------------------------------*/
/* Unparameterize a label string (wrapper for unparameterize()). */
/*---------------------------------------------------------------*/

void startunparam(xcWidget w, pointertype value, caddr_t calldata)
{
   if (areawin->selects > 0)
      unparameterize((int)value);
   unselect_all();
   setparammarks(NULL);
}

/*----------------------------------------------------------------*/
/* Set checkmarks on font menu according to the global defaults	  */
/*----------------------------------------------------------------*/

void setdefaultfontmarks()
{
   setfontmarks(areawin->psfont, areawin->anchor);
}

/*----------------------------------------------------------------*/
/* Pick up font name from _STR2 and pass it to loadfontfile()	  */
/*----------------------------------------------------------------*/

void locloadfont(xcWidget w, char *value)
{
   loadfontfile(_STR2);
   free(value);
}

/*----------------------------------------------------------------*/
/* Find the best matching font given new font, style, or encoding */
/*----------------------------------------------------------------*/
/* newfont, style, or encoding = -1 when not applicable		  */
/* Return the number of the best matching font.			  */  
/*----------------------------------------------------------------*/

short findbestfont(short curfont, short newfont, short style, short encoding) {

   char *newfamily;
   short i, newstyle, newenc;

   if (fontcount == 0) return -1;
   if (curfont < 0) curfont = 0;

   if (newfont < 0)
      newfamily = fonts[curfont].family;
   else if (newfont >= fontcount) {	/* move to next font family */
      short newidx;
      newfont = 0;
      while (strcmp(fonts[fontnumbers[newfont]].family, fonts[curfont].family))
	 newfont++;
      newidx = (newfont + 1) % nfontnumbers;
      while (!strcmp(fonts[curfont].family, fonts[fontnumbers[newidx]].family) &&
		newfont != newidx)
	 newidx = (newidx + 1) % nfontnumbers;
      newfamily = fonts[fontnumbers[newidx]].family;
      newfont = fontnumbers[newidx];
   }
   else
      newfamily = fonts[newfont].family;

   if (style < 0) 
      newstyle = fonts[curfont].flags & 0x03;
   else
      newstyle = style & 0x03;
 
   if (encoding < 0)
      newenc = fonts[curfont].flags & 0xf80;
   else
      newenc = encoding << 7;

   /* Best position is a match on all conditions */

   for (i = 0; i < fontcount; i++)
      if ((!strcmp(fonts[i].family, newfamily)) &&
		((fonts[i].flags & 0x03) == newstyle) &&
	 	((fonts[i].flags & 0xf80) == newenc))
	 return i;

   /* Fallback position 1:  Match requested property and one other. 	*/
   /* order of preference:  					   	*/
   /*   Requested property	Priority 1	Priority 2		*/
   /*		Font		  Style		  Encoding		*/
   /*		Style		  Font		  (none)		*/
   /*		Encoding	  Font		  (none)		*/

   for (i = 0; i < fontcount; i++) {
      if (newfont >= 0) {
	 if ((!strcmp(fonts[i].family, newfamily)) &&
		(fonts[i].flags & 0x03) == newstyle) return i;
      }
      else if (style >= 0) {
	 if (((fonts[i].flags & 0x03) == newstyle) &&
	 	(!strcmp(fonts[i].family, newfamily))) return i;
      }
      else if (encoding >= 0) {
	 if (((fonts[i].flags & 0xf80) == newenc) &&
	 	(!strcmp(fonts[i].family, newfamily))) return i;
      }
   }

   for (i = 0; i < fontcount; i++) {
      if (newfont >= 0) {
	 if ((!strcmp(fonts[i].family, newfamily)) &&
	    ((fonts[i].flags & 0xf80) >> 7) == newenc) return i;
      }
   }

   /* Fallback position 2:  Match only the requested property.  */
   /* For font selection only:  Don't want to select a new font */
   /* just because a certain style or encoding wasn't available.*/

   for (i = 0; i < fontcount; i++) {
      if (newfont >= 0) {
	 if (!strcmp(fonts[i].family, newfamily)) return i;
      }
   }      

   /* Failure to find matching font property */

   if (style >= 0) {
      Wprintf("Font %s not available in this style", newfamily);
   }
   else {
      Wprintf("Font %s not available in this encoding", newfamily);
   }
   return (-1);
}

/*----------------------------------------------------------------*/
/* Set the font.  This depends on the system state as to whether  */
/* font is set at current position in label, for an entire label, */
/* or as the default font to begin new labels.			  */
/*----------------------------------------------------------------*/

void setfontval(xcWidget w, pointertype value, labelptr settext)
{
   int newfont;
   short i, tc;
   stringpart *strptr;

   if (settext != NULL) {

      /* if last byte was a font designator, use it */

      if (areawin->textpos > 0 || areawin->textpos < stringlength(settext->string,
			True, areawin->topinstance)) {
	 strptr = findstringpart(areawin->textpos - 1, NULL, settext->string,
			areawin->topinstance);
	 if (strptr->type == FONT_NAME) {
	    tc = strptr->data.font;
	    i = findbestfont(tc, (short)value, -1, -1);
	    if (i >= 0) {
	       undrawtext(settext);
	       strptr->data.font = i;
	       redrawtext(settext);
	       if (w != NULL) {
	          charreport(settext);
      		  togglefontmark(i);
	       }
	    }
	    return;
         }
      }

      /* otherwise, look for the last style used in the string */
      tc = findcurfont(areawin->textpos, settext->string, areawin->topinstance);
   }
   else tc = areawin->psfont;

   /* Font change command will always find a value since at least one	*/
   /* font has to exist for the font menu button to exist.		*/

   if ((newfont = (int)findbestfont(tc, (short)value, -1, -1)) < 0) return;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      Wprintf("Font is now %s", fonts[newfont].psname);
      sprintf(_STR2, "%d", newfont);
      labeltext(FONT_NAME, (char *)&newfont);
   }
   else {
      Wprintf("Default font is now %s", fonts[newfont].psname);
      areawin->psfont = newfont;
   }

   if (w != NULL) togglefontmark(newfont);
}

/*----------------------------------------------------------------*/
/* Wrapper for routine setfontval()				  */
/*----------------------------------------------------------------*/

void setfont(xcWidget w, pointertype value, caddr_t calldata)
{
   short *fselect;
   labelptr settext;
   short labelcount = 0;
   Boolean preselected;

   if (eventmode == CATALOG_MODE || eventmode == FONTCAT_MODE ||
		eventmode == EFONTCAT_MODE) return;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      settext = *((labelptr *)EDITPART);
      setfontval(w, value, settext);
      charreport(settext);
   }
   else {
      if (areawin->selects == 0) {
	 checkselect(LABEL);
	 preselected = FALSE;
      }
      else preselected = TRUE;
      areawin->textpos = 1;
      for (fselect = areawin->selectlist; fselect < areawin->selectlist +
	    areawin->selects; fselect++) {
         if (SELECTTYPE(fselect) == LABEL) {
	    labelcount++;
	    settext = SELTOLABEL(fselect);
            setfontval(NULL, value, settext);
         }
      }
      if (labelcount == 0) setfontval(w, value, NULL);
      else if (!preselected) unselect_all();
   }
}

/*----------------------------------------------------------------*/
/* Set the style (bold, italic, normal) for the font, similarly   */
/* to the above routine setfontval().				  */
/*----------------------------------------------------------------*/

void setfontstyle(xcWidget w, pointertype value, labelptr settext)
{
   int newfont;
   short i, tc;
   stringpart *strptr;

   if (settext != NULL) {

      /* if last byte was a font designator, use it */

      if (areawin->textpos > 0 || areawin->textpos < stringlength(settext->string,
			True, areawin->topinstance)) {
	 strptr = findstringpart(areawin->textpos - 1, NULL, settext->string,
			areawin->topinstance);
	 if (strptr->type == FONT_NAME) {
	    tc = strptr->data.font;

	    /* find font which matches family and style, if available */

	    i = findbestfont(tc, -1, (short)value, -1);
	    if (i >= 0) {
	       undrawtext(settext);
	       strptr->data.font = i;
	       redrawtext(settext);
	       if (w != NULL) {
	          charreport(settext);
#ifndef TCL_WRAPPER
	          togglefontstyles(w);
#endif
	       }
	    }
            return;
         }
      }

      /* Otherwise, look for the last font used in the string */
      /* Fix by Dimitri Princen.  Was areawin->textpos - 2; was there a */
      /* reason for that?						*/

      tc = findcurfont(areawin->textpos, settext->string, areawin->topinstance);
   }
   else tc = areawin->psfont;

   if ((newfont = (int)findbestfont(tc, -1, (short)value, -1)) < 0) return;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      Wprintf("Font is now %s", fonts[newfont].psname);
      sprintf(_STR2, "%d", newfont);
      labeltext(FONT_NAME, (char *)&newfont);
   }
   else {
      Wprintf("Default font is now %s", fonts[newfont].psname);
      areawin->psfont = newfont;
   }
#ifdef TCL_WRAPPER
   toggleencodingmark(value);
#else
   togglefontstyles(w);
#endif
}

/*----------------------------------------------------------------*/
/* Wrapper for routine setfontstyle()				  */
/*----------------------------------------------------------------*/

void fontstyle(xcWidget w, pointertype value, caddr_t nulldata)
{
   short *fselect;
   labelptr settext;
   short labelcount = 0;
   Boolean preselected;

   if (eventmode == CATALOG_MODE || eventmode == FONTCAT_MODE ||
		eventmode == EFONTCAT_MODE) return;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      settext = *((labelptr *)EDITPART);
      setfontstyle(w, value, settext);
      charreport(settext);
   }
   else {
      if (areawin->selects == 0) {
	 checkselect(LABEL);
	 preselected = FALSE;
      }
      else preselected = TRUE;
      areawin->textpos = 1;
      for (fselect = areawin->selectlist; fselect < areawin->selectlist +
	    areawin->selects; fselect++) {
         if (SELECTTYPE(fselect) == LABEL) {
	    labelcount++;
	    settext = SELTOLABEL(fselect);
            setfontstyle(NULL, value, settext);
         }
      }
      if (labelcount == 0) setfontstyle(w, value, NULL);
      else if (!preselected) unselect_all();
   }
}

/*----------------------------------------------------------------*/
/* Set the encoding (standard, ISO-Latin1, special) for the font, */
/* similarly to the above routine setfontval().			  */
/*----------------------------------------------------------------*/

void setfontencoding(xcWidget w, pointertype value, labelptr settext)
{
   int newfont;
   short i, tc;
   stringpart *strptr;

   if (settext != NULL) {

      /* if last byte was a font designator, use it */

      if (areawin->textpos > 0 || areawin->textpos < stringlength(settext->string,
		True, areawin->topinstance)) {
	 strptr = findstringpart(areawin->textpos - 1, NULL, settext->string,
			areawin->topinstance);
	 if (strptr->type == FONT_NAME) {
	    tc = strptr->data.font;

	    i = findbestfont(tc, -1, -1, (short)value);
	    if (i >= 0) {
	       undrawtext(settext);
	       strptr->data.font = i;
	       redrawtext(settext);
	       if (w != NULL) {
	          charreport(settext);
#ifdef TCL_WRAPPER
		  toggleencodingmark(value);
#else
   	          toggleencodings(w);
#endif
	       }
	    }
	    return;
	 }
      }

      /* otherwise, look for the last style used in the string */
      tc = findcurfont(areawin->textpos - 2, settext->string, areawin->topinstance);
   }
   else tc = areawin->psfont;

   if ((newfont = (int)findbestfont(tc, -1, -1, (short)value)) < 0) return;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      Wprintf("Font is now %s", fonts[newfont].psname);
      sprintf(_STR2, "%d", newfont);
      labeltext(FONT_NAME, (char *)&newfont);
   }
   else {
      Wprintf("Default font is now %s", fonts[newfont].psname);
      areawin->psfont = newfont;
   }

#ifndef TCL_WRAPPER
   toggleencodings(w);
#endif
}

/*----------------------------------------------------------------*/
/* Wrapper for routine setfontencoding()			  */
/*----------------------------------------------------------------*/

void fontencoding(xcWidget w, pointertype value, caddr_t nulldata)
{
   short *fselect;
   labelptr settext;
   short labelcount = 0;
   Boolean preselected;

   if (eventmode == CATALOG_MODE || eventmode == FONTCAT_MODE ||
		eventmode == EFONTCAT_MODE) return;

   if (eventmode == TEXT_MODE || eventmode == ETEXT_MODE) {
      settext = *((labelptr *)EDITPART);
      setfontencoding(w, value, settext);
      charreport(settext);
   }
   else {
      if (areawin->selects == 0) {
  	 checkselect(LABEL);
	 preselected = FALSE; 
      }
      else preselected = TRUE;
      areawin->textpos = 1;
      for (fselect = areawin->selectlist; fselect < areawin->selectlist +
	    areawin->selects; fselect++) {
         if (SELECTTYPE(fselect) == LABEL) {
	    labelcount++;
	    settext = SELTOLABEL(fselect);
            setfontencoding(NULL, value, settext);
         }
      }
      if (labelcount == 0) setfontencoding(w, value, NULL);
      else if (!preselected) unselect_all();
   }
}

/*----------------------------------------------*/
/* Generate the table of special characters	*/
/*						*/
/* Return FALSE if the edited label is LaTeX-	*/
/* compatible, since LaTeX text does not	*/
/* usually contain special characters, but it	*/
/* does contain a lot of backslashes.		*/
/*----------------------------------------------*/

Boolean dospecial()
{
   labelptr curlabel;
   int cfont;

   curlabel = TOLABEL(EDITPART);
   if (curlabel->anchor & LATEXLABEL) return False;

   cfont = findcurfont(areawin->textpos, curlabel->string, areawin->topinstance);
   composefontlib(cfont);
   startcatalog(NULL, FONTLIB, NULL);
   return True;
}

/*-------------------------------------------------------------------------*/
