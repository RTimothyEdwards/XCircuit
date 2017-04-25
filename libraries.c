/*-------------------------------------------------------------------------*/
/* libraries.c --- xcircuit routines for the builtin and user libraries    */
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	   */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/*      written by Tim Edwards, 8/13/93    				   */
/*-------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef XC_WIN32
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif
#ifdef TCL_WRAPPER 
#include <tk.h>
#else
#ifndef XC_WIN32
#include "Xw/Xw.h"
#endif
#endif

#include <math.h>

/*-------------------------------------------------------------------------*/
/* Local includes							   */
/*-------------------------------------------------------------------------*/

#include "colordefs.h"
#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*-------------------------------------------------------------------------*/
/* Global Variable definitions						   */
/*-------------------------------------------------------------------------*/

extern Display	*dpy;   /* Works well to make this globally accessible */
extern Cursor	appcursors[NUM_CURSORS];
extern Globaldata xobjs;
extern XCWindowData *areawin;
extern char _STR[150];
extern short fontcount;
extern fontinfo *fonts;
extern Boolean was_preselected;
extern int number_colors;
extern colorindex *colorlist;

/*---------------------------------------------------------*/
/* Find the Helvetica font for use in labeling the objects */
/*---------------------------------------------------------*/

short findhelvetica()
{
   short fval;

   if (fontcount == 0) loadfontfile("Helvetica");

   for (fval = 0; fval < fontcount; fval++)
      if (!strcmp(fonts[fval].psname, "Helvetica"))
	 break; 

   /* If not there, use the first Helvetica font */

   if (fval == fontcount) {
      for (fval = 0; fval < fontcount; fval++)
         if (!strcmp(fonts[fval].family, "Helvetica"))
	    break; 
   }

   /* If still not there, use the first non-Symbol font */
   /* If this doesn't work, then the libraries are probably misplaced. . .*/

   if (fval == fontcount) {
      for (fval = 0; fval < fontcount; fval++)
         if (strcmp(fonts[fval].family, "Symbol"))
	    break; 
   }

   return fval;
}

/*-------------------------------------------*/
/* Return to drawing window from the library */
/*-------------------------------------------*/

void catreturn()
{
   /* Pop the object being edited from the push stack. */

   popobject(NULL, (pointertype)1, NULL);
}

/*------------------------------------------------------*/
/* Find page number from cursor position		*/
/* Mode = 0:  Look for exact corresponding page number  */
/*   and return -1 if out-of-bounds			*/
/* Mode = 1:  Look for position between pages, return	*/
/*   page number of page to the right.  		*/
/*------------------------------------------------------*/

int pageposition(short libmode, int x, int y, int mode)
{
   int xin, yin, bpage, pages;
   int gxsize, gysize, xdel, ydel;

   pages = (libmode == PAGELIB) ? xobjs.pages : xobjs.numlibs;
   computespacing(libmode, &gxsize, &gysize, &xdel, &ydel);
   window_to_user(x, y, &areawin->save);

   if (mode == 0) {	/* On-page */
      if (areawin->save.x >= 0 && areawin->save.y <= 0) {
         xin = areawin->save.x / xdel;
         yin = areawin->save.y / ydel; 
         if (xin < gxsize && yin > -gysize) {
            bpage = (xin % gxsize) - (yin * gxsize);
            if (bpage < pages)
	       return bpage;
         }
      }
      return -1;
   }
   else {		/* Between-pages */
      xin = (areawin->save.x + (xdel >> 1)) / xdel;
      if (xin > gxsize) xin = gxsize;
      if (xin < 0) xin = 0;
      yin = areawin->save.y  / ydel; 
      if (yin > 0) yin = 0;
      if (yin < -gysize) yin = -gysize;
      bpage = (xin % (gxsize + 1)) + 1 - (yin * gxsize);
      if (bpage > pages + 1) bpage = pages + 1;
      return bpage;
   }
}

/*------------------------------------------------------*/
/* Find the number of other pages linked to the		*/
/* indicated page (having the same filename, and	*/
/* ignoring empty pages).  result is the total number	*/
/* of pages in the output file.				*/
/*------------------------------------------------------*/

short pagelinks(int page)
{
   int i;
   short count = 0;

   for (i = 0; i < xobjs.pages; i++)
      if (xobjs.pagelist[i]->pageinst != NULL)
	 if (xobjs.pagelist[i]->pageinst->thisobject->parts > 0)
	    if ((i == page) || (xobjs.pagelist[i]->filename &&
			xobjs.pagelist[page]->filename &&
			(!filecmp(xobjs.pagelist[i]->filename,
			xobjs.pagelist[page]->filename))))
	       count++;

   return count;
}

/*------------------------------------------------------*/
/* This is an expanded version of pagelinks() (above),	*/
/* to deal with the separate issues of independent top-	*/
/* level schematics and subcircuits.  For the indicated	*/
/* page, return a list of pages depending on the mode:	*/
/*							*/
/* mode = INDEPENDENT: independent top-level pages	*/
/* mode = DEPENDENT: dependent pages (subcircuits)	*/
/* mode = PAGE_DEPEND: subcircuits of the current page,	*/
/* mode = LINKED_PAGES: subcircuits of the current	*/
/*	page, including parameter links			*/
/* mode = TOTAL_PAGES: independent pages + subcircuits  */
/* mode = ALL_PAGES: all pages in xcircuit		*/
/*							*/
/* The list is the size of the number of pages, and	*/
/* entries corresponding to the requested mode are set	*/
/* nonzero (the actual number indicates the number of	*/
/* references to the page, which may or may not be	*/
/* useful to know).					*/
/*							*/
/* It is the responsibility of the calling routine to	*/
/* free the memory allocated for the returned list.	*/
/*------------------------------------------------------*/

short *pagetotals(int page, short mode)
{
   int i;
   short *counts, *icount;

   if (xobjs.pagelist[page]->pageinst == NULL) return NULL;

   counts = (short *)malloc(xobjs.pages * sizeof(short));
   icount = (short *)malloc(xobjs.pages * sizeof(short));
   for (i = 0; i < xobjs.pages; i++) {
      *(counts + i) = 0;
      *(icount + i) = 0;
   }

   /* Find all the subcircuits of this page */

   if (mode != ALL_PAGES)
      findsubschems(page, xobjs.pagelist[page]->pageinst->thisobject,
		0, counts, (mode == LINKED_PAGES) ? TRUE : FALSE);

   /* Check independent entries (top-level pages which are not	*/
   /* subcircuits of another page, but have the same filename	*/
   /* as the page we started from).  Set the counts entry to -1	*/
   /* to mark each independent page.				*/

   if (mode != PAGE_DEPEND)
      for (i = 0; i < xobjs.pages; i++)
         if (xobjs.pagelist[i]->pageinst != NULL)
	    if (xobjs.pagelist[i]->pageinst->thisobject->parts > 0)
	    {
	       if (mode == ALL_PAGES)
		  (*(counts + i)) = 1;
	       else
	       {
	          if ((i == page) || (xobjs.pagelist[i]->filename
			&& xobjs.pagelist[page]->filename
			&& (!filecmp(xobjs.pagelist[i]->filename,
			xobjs.pagelist[page]->filename))))
		     if ((mode == INDEPENDENT) || (*(counts + i) == 0))
		        (*(icount + i))++;
	       }
	    }

   /* Check other dependent entries (top-level pages which are 	*/
   /* subcircuits of any independent page).			*/

   if ((mode == DEPENDENT) || (mode == TOTAL_PAGES) || (mode == LINKED_PAGES))
   {
      for (i = 0; i < xobjs.pages; i++)
	 if ((i != page) && (*(icount + i) > 0))
	    findsubschems(i, xobjs.pagelist[i]->pageinst->thisobject,
		0, counts, (mode == LINKED_PAGES) ? TRUE : FALSE);
   }

   if (mode == INDEPENDENT)
   {
      free((char *)counts);
      return icount;
   }
   else
   {
      if ((mode == TOTAL_PAGES) || (mode == LINKED_PAGES)) {
	 /* merge dependent and independent */
	 for (i = 0; i < xobjs.pages; i++)
	    if (*(icount + i) > 0)
	       (*(counts + i))++;
      }
      free((char *)icount);
      return counts;
   }
}

/*---------------------------------------------------------*/
/* Test whether a library instance is a "virtual" instance */
/*---------------------------------------------------------*/

Boolean is_virtual(objinstptr thisinst) {
   int libno;
   liblistptr ilist;

   libno = libfindobject(thisinst->thisobject, NULL);

   for (ilist = xobjs.userlibs[libno].instlist; ilist != NULL; ilist = ilist->next)
      if ((ilist->thisinst == thisinst) && (ilist->virtual == TRUE))
	 return TRUE;

   return FALSE;
}

/*------------------------------------------------------*/
/* Test whether an object is a page, and return the	*/
/* page number if it is.  Otherwise, return -1.		*/
/*------------------------------------------------------*/

int is_page(objectptr thisobj)
{
   int i;
   
   for (i = 0; i < xobjs.pages; i++)
      if (xobjs.pagelist[i]->pageinst != NULL)
         if (xobjs.pagelist[i]->pageinst->thisobject == thisobj) return i;

   return -1;
}

/*------------------------------------------------------*/
/* Test whether an object is a library, and return the	*/
/* library number if it is.  Otherwise, return -1.	*/
/*------------------------------------------------------*/

int is_library(objectptr thisobj)
{
   int i;
   
   for (i = 0; i < xobjs.numlibs; i++)
      if (xobjs.libtop[i + LIBRARY]->thisobject == thisobj) return i;

   return -1;
}

/*------------------------------------------------------*/
/* Check for library name (string).  Because XCircuit   */
/* generates the text "Library: <filename>" for		*/
/* library names, we also check against <filename>	*/
/* only in these names (this library name syntax is	*/
/* deprecated, but the check is retained for backwards	*/
/* compatibility).					*/
/*							*/
/* If no library matches the given name, return -1.	*/
/*------------------------------------------------------*/

int NameToLibrary(char *libname)
{
   char *slib;
   int i;

   for (i = 0; i < xobjs.numlibs; i++) {
      slib = xobjs.libtop[i + LIBRARY]->thisobject->name;
      if (!strcmp(libname, slib)) {
	 return i;
      }
      else if (!strncmp(slib, "Library: ", 9)) {
	 if (!strcmp(libname, slib + 9)) {
	    return i;
         }
      }
   }
   return -1;
}

/*------------------------------------------------------*/
/* Move an object and all of its virtual instances from	*/
/* one library to another.				*/
/*------------------------------------------------------*/

int libmoveobject(objectptr thisobject, int libtarget)
{
   int j, libsource;
   liblistptr spec, slast, srch;

   libsource = libfindobject(thisobject, &j);

   if (libsource == libtarget) return libsource; /* nothing to do */
   if (libsource < 0) return libsource;		 /* object not in the library */

   /* Move the object from library "libsource" to library "libtarget" */

   xobjs.userlibs[libtarget].library = (objectptr *)
		realloc(xobjs.userlibs[libtarget].library,
		(xobjs.userlibs[libtarget].number + 1) * sizeof(objectptr));

   *(xobjs.userlibs[libtarget].library + xobjs.userlibs[libtarget].number) = thisobject;
   xobjs.userlibs[libtarget].number++;

   for (; j < xobjs.userlibs[libsource].number; j++)
      *(xobjs.userlibs[libsource].library + j) =
		*(xobjs.userlibs[libsource].library + j + 1);
      xobjs.userlibs[libsource].number--;

   /* Move all instances from library "libsource" to library "libtarget" */

   slast = NULL;
   for (spec = xobjs.userlibs[libsource].instlist; spec != NULL;) {
      if (spec->thisinst->thisobject == thisobject) {

	 /* Add to end of spec list in target */
	 srch = xobjs.userlibs[libtarget].instlist;
	 if (srch == NULL)
	    xobjs.userlibs[libtarget].instlist = spec;
	 else {
	    for (; srch->next != NULL; srch = srch->next);
	    spec->next = srch->next;
	    srch->next = spec;
	 }
	 
	 if (slast != NULL) {
	    slast->next = spec->next;
	    spec = slast->next;
	 }
	 else {
	    xobjs.userlibs[libsource].instlist = spec->next;
	    spec = xobjs.userlibs[libsource].instlist;
	 }
      }
      else {
	 slast = spec;
	 spec = spec->next;
      }
   }

   return libsource;
}

/*------------------------------------------------------*/
/* Determine which library contains the specified	*/
/* object.  If found, return the library number, or	*/
/* else return -1 if the object was not found in any	*/
/* library.  If "partidx" is non-null, fill with the	*/
/* integer offset of the object from the beginning of	*/
/* the library.						*/
/*------------------------------------------------------*/

int libfindobject(objectptr thisobject, int *partidx)
{
   int i, j;
   objectptr libobj;

   for (i = 0; i < xobjs.numlibs; i++) {
      for (j = 0; j < xobjs.userlibs[i].number; j++) {
         libobj = *(xobjs.userlibs[i].library + j);
	 if (libobj == thisobject) {
	    if (partidx != NULL) *partidx = j;
	    return i;
	 }
      }
   }
   return -1;
}

/*------------------------------------------------------*/
/* ButtonPress handler during page catalog viewing mode */
/*------------------------------------------------------*/

void pagecat_op(int op, int x, int y)
{
   int bpage;
   short mode;

   for (mode = 0; mode < LIBRARY; mode++) {
      if (areawin->topinstance == xobjs.libtop[mode]) break;
   }
   if (mode == LIBRARY) return;  /* Something went wrong if this happens */

   if (op != XCF_Cancel) {
      if ((bpage = pageposition(mode, x, y, 0)) >= 0) {
		  
	 if (eventmode == ASSOC_MODE) {
	    if (mode == PAGELIB) {
	       /* using changepage() allows use of new page for schematic */
	       changepage(bpage);
	       /* associate the new schematic */
	       schemassoc(topobject, areawin->stack->thisinst->thisobject);
	       /* pop back to calling (symbol) page */
	       catreturn();
	       eventmode = NORMAL_MODE;
	    }
	    else {
	       areawin->lastlibrary = bpage;
	       startcatalog(NULL, (pointertype)(LIBRARY + bpage), NULL);
	    }
	    return;
         }
	 else if (op == XCF_Select) {
	    if (mode == PAGELIB)    /* No such method for LIBLIB is defined. */
	       select_add_element(OBJINST);
	 }
	 else if ((op == XCF_Library_Pop) || (op == XCF_Finish)) {

	    /* like catreturn(), but don't actually go to the popped page */
	    unselect_all();
	    eventmode = NORMAL_MODE;
	    if (mode == PAGELIB) {
	       newpage(bpage);
	    }
	    else {
	       startcatalog(NULL, (pointertype)(LIBRARY + bpage), NULL);
	    }
	    return;
	 }
      }
   }
   else {
      eventmode = NORMAL_MODE;
      catreturn();
   }
}

/*------------------------------------------------------------------------------*/
/* Subroutine to find the correct scale and position of the object instance	*/
/* representing an entire page in the page directory.				*/
/*------------------------------------------------------------------------------*/

void pageinstpos(short mode, short tpage, objinstptr drawinst, int gxsize,
	int gysize, int xdel, int ydel)
{
   objectptr libobj = drawinst->thisobject;
   float scalex, scaley;

   drawinst->position.x = (tpage % gxsize) * xdel;
   drawinst->position.y = -(tpage / gxsize + 1) * ydel;

   /* center the object on its page bounding box */

   if (drawinst->bbox.width == 0 || drawinst->bbox.height == 0) {
      drawinst->scale = 0.45 * libobj->viewscale;
      drawinst->position.x += 0.05 * xdel - libobj->pcorner.x * drawinst->scale;
      drawinst->position.y += 0.05 * ydel - libobj->pcorner.y * drawinst->scale;
   }
   else {
      scalex = (0.9 * xdel) / drawinst->bbox.width;
      scaley = (0.9 * ydel) / drawinst->bbox.height;
      if (scalex > scaley) {
         drawinst->scale = scaley;
	 drawinst->position.x -= (drawinst->bbox.lowerleft.x * scaley);
         drawinst->position.x += (xdel - (drawinst->bbox.width * scaley)) / 2;
         drawinst->position.y += 0.05 * ydel - drawinst->bbox.lowerleft.y
			* drawinst->scale;
      }
      else {
         drawinst->scale = scalex;
	 drawinst->position.y -= (drawinst->bbox.lowerleft.y * scalex);
         drawinst->position.y += (ydel - (drawinst->bbox.height * scalex)) / 2;
         drawinst->position.x += 0.05 * xdel - drawinst->bbox.lowerleft.x
			* drawinst->scale;
      }
   }
}

/*--------------------------------------------------------------*/
/* Make a new instance for inserting into the page directory	*/
/*--------------------------------------------------------------*/

objinstptr newpageinst(objectptr pageobj)
{
   objinstptr newinst = (objinstptr) malloc(sizeof(objinst));
   instancedefaults(newinst, pageobj, 0, 0);
   newinst->type = OBJINST;
   newinst->color = DEFAULTCOLOR;
   newinst->style = NORMAL;	/* Do not scale linewidth with page scale */
   return newinst;
}

/*-----------------------------------------------------------*/
/* Find spacing of objects for pages in the page directories */
/*-----------------------------------------------------------*/

void computespacing(short mode, int *gxsize, int *gysize, int *xdel, int *ydel)
{
   int pages = (mode == PAGELIB) ? xobjs.pages : xobjs.numlibs;

   *gxsize = (int)sqrt((double)pages) + 1;
   *gysize = 1 + pages / (*gxsize);

   /* 0.5 is the default vscale;  g#size is the number of pages per line */

   *xdel = areawin->width / (0.5 * (*gxsize));
   *ydel = areawin->height / (0.5 * (*gysize));
}

/*-------------------------------------------------------------------*/
/* Draw the catalog of page ordering or the library master directory */
/*-------------------------------------------------------------------*/

void composepagelib(short mode)
{
   genericptr *pgen;
   objinstptr drawinst;
   objectptr libobj, directory = xobjs.libtop[mode]->thisobject;
   short i;
   polyptr *drawbox;
   labelptr *pagelabel;
   stringpart *strptr;
   pointlist pointptr;
   int margin, xdel, ydel, gxsize, gysize;
   int pages = (mode == PAGELIB) ? xobjs.pages : xobjs.numlibs;
   short fval = findhelvetica();

   /* Like the normal libraries, instances come from a special list, so	 */
   /* they should not be destroyed, but will be null'd out and retrieved */
   /* from the list.							 */

   for (pgen = directory->plist; pgen < directory->plist + directory->parts; pgen++)
      if (IS_OBJINST(*pgen)) *pgen = NULL;

   reset(directory, NORMAL);

   /* generate the list of object instances */

   directory->plist = (genericptr *)malloc(sizeof(genericptr));
   directory->parts = 0;

   computespacing(mode, &gxsize, &gysize, &xdel, &ydel);
   margin = xdel / 40;	/* margin between pages */

   for (i = 0; i < pages; i++) {
      drawinst = (mode == PAGELIB) ? xobjs.pagelist[i]->pageinst :
		xobjs.libtop[i + LIBRARY];
      if (drawinst != NULL) {
	 libobj = drawinst->thisobject;

	 /* This is a stop-gap measure. . . should be recalculating the bounds of */
	 /* the instance on every action, not just before arranging the library.  */
	 drawinst->bbox.lowerleft.x = libobj->bbox.lowerleft.x;
	 drawinst->bbox.lowerleft.y = libobj->bbox.lowerleft.y;
	 drawinst->bbox.width = libobj->bbox.width;
	 drawinst->bbox.height = libobj->bbox.height;
	 /* End stop-gap measure */

	 PLIST_INCR(directory);
	 *(directory->plist + directory->parts) = (genericptr)drawinst;
	 directory->parts++;
         pageinstpos(mode, i, drawinst, gxsize, gysize, xdel, ydel);
      }

      /* separate pages (including empty ones) with bounding boxes */

      NEW_POLY(drawbox, directory);
      (*drawbox)->color = LOCALPINCOLOR;   /* default red */
      (*drawbox)->style = NORMAL;      	   /* CLOSED */
      (*drawbox)->width = 1.0;
      (*drawbox)->number = 4;
      (*drawbox)->points = (pointlist) malloc(4 * sizeof(XPoint));
      (*drawbox)->passed = NULL;
      (*drawbox)->cycle = NULL;
      pointptr = (*drawbox)->points;
      pointptr->x = (i % gxsize) * xdel + margin;
      pointptr->y = -(i / gxsize) * ydel - margin;
      pointptr = (*drawbox)->points + 1;
      pointptr->x = ((i % gxsize) + 1) * xdel - margin;
      pointptr->y = -(i / gxsize) * ydel - margin;
      pointptr = (*drawbox)->points + 2;
      pointptr->x = ((i % gxsize) + 1) * xdel - margin;
      pointptr->y = -((i / gxsize) + 1) * ydel + margin;
      pointptr = (*drawbox)->points + 3;
      pointptr->x = (i % gxsize) * xdel + margin;
      pointptr->y = -((i / gxsize) + 1) * ydel + margin;

      /* each page gets its name printed at the bottom */

      if (drawinst != NULL) {
	 NEW_LABEL(pagelabel, directory);
	 labeldefaults(*pagelabel, False, (pointptr->x + (pointptr-1)->x) / 2,
			pointptr->y - 5);
	 (*pagelabel)->color = DEFAULTCOLOR;
	 (*pagelabel)->scale = 0.75;
	 (*pagelabel)->string->data.font = fval;
         (*pagelabel)->passed = NULL;
	 strptr = makesegment(&((*pagelabel)->string), NULL);
	 strptr->type = TEXT_STRING;
	 strptr->data.string = (char *) malloc(1 + strlen(libobj->name));
	 strcpy(strptr->data.string, libobj->name);
	 (*pagelabel)->anchor = TOP | NOTBOTTOM | NOTLEFT;
      }
   }

   /* calculate a bounding box for this display */
   /* and center it in its window */

   calcbbox(xobjs.libtop[mode]);
   centerview(xobjs.libtop[mode]);
}

/*------------------------------------------------------------*/
/* Update the page or library directory based on new bounding */
/* box information for the page or library passed in "tpage". */
/*------------------------------------------------------------*/

void updatepagelib(short mode, short tpage)
{
   objectptr compobj, libinst = xobjs.libtop[mode]->thisobject;
   objinstptr pinst;
   genericptr *gelem;
   int i, xdel, ydel, gxsize, gysize, lpage;

   /* lpage is the number of the page as found on the directory page */
   lpage = (mode == PAGELIB) ? tpage : tpage - LIBRARY;
   compobj = (mode == PAGELIB) ? xobjs.pagelist[tpage]->pageinst->thisobject
		: xobjs.libtop[tpage]->thisobject;

   computespacing(mode, &gxsize, &gysize, &xdel, &ydel);

   for (i = 0; i < libinst->parts; i++) {
      gelem = libinst->plist + i;
      if (IS_OBJINST(*gelem)) {
	 pinst = TOOBJINST(gelem);
         if (pinst->thisobject == compobj) {
	    /* recalculate scale and position of the object instance */
            pageinstpos(mode, lpage, pinst, gxsize, gysize, xdel, ydel);
	    break;
	 }
      }
   }

   /* if there is no instance for this page, then recompose the whole library */

   if (i == libinst->parts) composelib(mode);
}

/*----------------------*/
/* Rearrange pages	*/
/*----------------------*/

void pagecatmove(int x, int y)
{
   int bpage;
   objinstptr exchobj;
   Pagedata *ipage, **testpage, **tpage2;

   if (areawin->selects == 0) return;
   else if (areawin->selects > 2) {
      Wprintf("Select maximum of two objects.");
      return;
   }

   /* Get the page corresponding to the first selected object */

   exchobj = SELTOOBJINST(areawin->selectlist);
   for (testpage = xobjs.pagelist; testpage < xobjs.pagelist + xobjs.pages; testpage++)
      if (*testpage != NULL && (*testpage)->pageinst == exchobj)
	 break;

   /* If two objects are selected, then exchange their order */

   if (areawin->selects == 2) {
      exchobj = SELTOOBJINST(areawin->selectlist + 1);
      for (tpage2 = xobjs.pagelist; tpage2 < xobjs.pagelist + xobjs.pages; tpage2++)
	 if (*tpage2 != NULL && (*tpage2)->pageinst == exchobj)
	    break;

      ipage = *testpage;
      *testpage = *tpage2;
      *tpage2 = ipage;
   }

   /* If one object selected; find place to put from cursor position */

   else if ((bpage = pageposition(PAGELIB, x, y, 1)) >= 0) {
      int k, epage;
      Pagedata *eptr;

      /* Find page number of the original page */

      epage = (int)(testpage - xobjs.pagelist);
      eptr = *(xobjs.pagelist + epage);

      /* move page (epage) to position between current pages */
      /* (bpage - 2) and (bpage - 1) by shifting pointers.   */

      if ((bpage - 1) < epage) {
	 for (k = epage - 1; k >= bpage - 1; k--) {
	    *(xobjs.pagelist + k + 1) = *(xobjs.pagelist + k);
	    renamepage(k + 1);
	 }
	 *(xobjs.pagelist + bpage - 1) = eptr;
	 renamepage(bpage - 1);
      }
      else if ((bpage - 2) > epage) {
	 for (k = epage + 1; k <= bpage - 2; k++) {
	    *(xobjs.pagelist + k - 1) = *(xobjs.pagelist + k);
	    renamepage(k - 1);
	 }
	 *(xobjs.pagelist + bpage - 2) = eptr;
	 renamepage(bpage - 2);
      }
   }

   unselect_all();
   composelib(PAGELIB);
   drawarea(NULL, NULL, NULL);
}

/*-----------------------------------------*/
/* Draw the catalog of predefined elements */
/*-----------------------------------------*/

void composelib(short mode)
{
   genericptr *pgen;
   objinstptr drawinst;
   labelptr *drawname;
   objectptr libobj, libpage = xobjs.libtop[mode]->thisobject;
   liblistptr spec;
   int xpos = 0, ypos = areawin->height << 1;
   int nypos = 220, nxpos;
   short fval;
   short llx, lly, urx, ury, width, height, xcenter;

   int totalarea, targetwidth;
   double scale, savescale;
   short savemode;
   XPoint savepos;

   /* Also make composelib() a wrapper for composepagelib() */
   if ((mode > FONTLIB) && (mode < LIBRARY)) {
      composepagelib(mode);
      return;
   }

   /* The instances on the library page come from the library's		*/
   /* "instlist".  So that we don't destroy the actual instance when we	*/
   /* call reset(), we find the pointer to the instance and NULL it.	*/
   
   for (pgen = libpage->plist; pgen < libpage->plist + libpage->parts; pgen++)
      if (IS_OBJINST(*pgen)) *pgen = NULL;

   /* Before resetting, save the position and scale.  We will restore	*/
   /* them at the end.							*/

   savepos = libpage->pcorner;
   savescale = libpage->viewscale;
   reset(libpage, NORMAL);

   /* Return if library defines no objects or virtual instances */

   if (xobjs.userlibs[mode - LIBRARY].instlist == NULL) return;

   /* Find the Helvetica font for use in labeling the objects */

   fval = findhelvetica();

   /* Attempt to produce a library with the same aspect ratio as the	*/
   /* drawing window.  This is only approximate, and does not take	*/
   /* into account factors such as the length of the name string.	*/

   totalarea = 0;
   for (spec = xobjs.userlibs[mode - LIBRARY].instlist; spec != NULL;
                spec = spec->next) {
      libobj = spec->thisinst->thisobject;

      /* "Hidden" objects are not drawn */
      if (libobj->hidden == True) continue;

      drawinst = spec->thisinst;
      drawinst->position.x = 0;
      drawinst->position.y = 0;

      /* Get the bounding box of the instance in the page's coordinate system */
      calcinstbbox((genericptr *)(&drawinst), &llx, &lly, &urx, &ury);
      width = urx - llx;
      height = ury - lly;
      width += 30;	/* space padding */
      height += 30;	/* height padding */
      if (width < 200) width = 200;	/* minimum box width */
      if (height < 220) height = 220;	/* minimum box height */
      totalarea += (width * height);
   }

   scale = (double)totalarea / (double)(areawin->width * areawin->height);
   targetwidth = (int)(sqrt(scale) * (double)areawin->width);

   /* generate the list of object instances and their labels */

   savemode = eventmode;
   eventmode = CATALOG_MODE;
   for (spec = xobjs.userlibs[mode - LIBRARY].instlist; spec != NULL;
		spec = spec->next) {
      libobj = spec->thisinst->thisobject;

      /* "Hidden" objects are not drawn */
      if (libobj->hidden == True) continue;

      drawinst = spec->thisinst;
      drawinst->position.x = 0;
      drawinst->position.y = 0;

      /* Generate the part;  unlike the usual NEW_OBJINST, the	*/
      /* instance record isn't allocated.			*/

      PLIST_INCR(libpage); 
      *(libpage->plist + libpage->parts) = (genericptr)drawinst;
      libpage->parts++;

      /* Get the bounding box of the instance in the page's coordinate system */
      calcinstbbox((genericptr *)(&drawinst), &llx, &lly, &urx, &ury);
      xcenter = (llx + urx) >> 1;
      width = urx - llx;
      height = ury - lly;

      /* Add an ad-hoc spacing rule of 30 for padding space between objects */
      width += 30;

      /* Prepare the object name and determine its width.  Adjust width	*/
      /* needed for object on the library page if necessary.		*/

      if (fval < fontcount) {
	 stringpart *strptr;
	 TextExtents tmpext;

	 NEW_LABEL(drawname, libpage);
	 labeldefaults(*drawname, False, 0, 0);
	 (*drawname)->color = (spec->virtual) ?
			OFFBUTTONCOLOR : DEFAULTCOLOR;
         (*drawname)->scale = 0.75;
	 (*drawname)->string->data.font = fval;
	 strptr = makesegment(&((*drawname)->string), NULL);
	 strptr->type = TEXT_STRING;

         strptr->data.string = strdup(libobj->name);
         (*drawname)->anchor = TOP | NOTBOTTOM | NOTLEFT;

	 /* If the label is longer than the object width, then	*/
	 /* adjust positions accordingly.  Note that as an ad-	*/
	 /* hoc spacing rule, a padding of 30 is put between	*/
	 /* objects; this padding is reduced to 5 		*/

	 tmpext = ULength(*drawname, drawinst, NULL);

	 /* Ad-hoc spacing rule is 5 for labels */
	 tmpext.width += 5;

	 if (tmpext.width > width)
	    width = tmpext.width;
      }

      /* Minimum allowed width is 200 */
      if (width < 200) width = 200;

      /* Determine the area needed on the page to draw the object */

      nxpos = xpos + width;
      if ((nxpos > targetwidth) && (xpos > 0)) {
	 nxpos -= xpos; 
	 xpos = 0;
	 ypos -= nypos;
	 nypos = 200;
      }

      if (height > (nypos - 50)) nypos = height + 50;

      drawinst->position.x = xpos + (width >> 1) - xcenter;
      drawinst->position.y = ypos - (height + lly);
      if (height <= 170) drawinst->position.y -= ((170 - height) >> 1);
      drawinst->color = DEFAULTCOLOR;

      if (fval < fontcount) {
         (*drawname)->position.x = xpos + (width >> 1);

         if (height > 170)
            (*drawname)->position.y = drawinst->position.y + lly - 10;
         else
            (*drawname)->position.y = ypos - 180;

      }
      xpos = nxpos;
   }
   eventmode = savemode;

   /* Compute the bounding box of the library page */
   calcbbox(xobjs.libtop[mode]);

   /* Update the library directory */
   updatepagelib(LIBLIB, mode);

   /* Restore original view position */
   libpage->pcorner = savepos;
   libpage->viewscale = savescale;
}

/*----------------------------------------------------------------*/
/* Find any dependencies on an object.				  */
/*   Return values:  0 = no dependency, 1 = dependency on page,	  */
/*	2 = dependency in another library object.		  */
/*   Object/Page with dependency (if any) returned in "compobjp". */
/*----------------------------------------------------------------*/

short finddepend(objinstptr libobj, objectptr **compobjp)
{
   genericptr *testobj;
   short page, i, j;
   objectptr *compobj;
  
   for (i = 0; i < xobjs.numlibs; i++) {
      for (j = 0; j < xobjs.userlibs[i].number; j++) {
	 compobj = xobjs.userlibs[i].library + j;
         *compobjp = compobj;
		     
         for (testobj = (*compobj)->plist; testobj < (*compobj)->plist
	          + (*compobj)->parts; testobj++) {
	    if (IS_OBJINST(*testobj)) {
	       if (TOOBJINST(testobj)->thisobject == libobj->thisobject) return 2;
	    }
	 }
      }
   }

   /* also look in the xobjs.pagelist */

   for (page = 0; page < xobjs.pages; page++) {
      if (xobjs.pagelist[page]->pageinst == NULL) continue;
      compobj = &(xobjs.pagelist[page]->pageinst->thisobject);
      *compobjp = compobj;
      for (testobj = (*compobj)->plist; testobj < (*compobj)->plist
	       + (*compobj)->parts; testobj++) {
	 if (IS_OBJINST(*testobj)) {
	    if (TOOBJINST(testobj)->thisobject == libobj->thisobject) return 1;
	 }
      }
   }
   return 0;
}

/*--------------------------------------------------------------*/
/* Virtual copy:  Make a separate copy of an object on the same	*/
/* library page as the original, representing an instance of	*/
/* the object with different parameters.  The object must have	*/
/* parameters for this to make sense, so check for parameters	*/
/* before allowing the virtual copy.				*/
/*--------------------------------------------------------------*/

void catvirtualcopy()
{
   short i, *newselect;
   objinstptr libobj, libinst;

   if (areawin->selects == 0) return;
   else if ((i = is_library(topobject)) < 0) return;

   /* Check for existance of parameters in the object for each */
   /* selected instance */

   for (newselect = areawin->selectlist; newselect < areawin->selectlist
	  + areawin->selects; newselect++) {
      libobj = SELTOOBJINST(newselect);
      libinst = addtoinstlist(i, libobj->thisobject, TRUE);
      instcopy(libinst, libobj);
      tech_mark_changed(GetObjectTechnology(libobj->thisobject));
   }

   clearselects();
   composelib(LIBRARY + i);
   drawarea(NULL, NULL, NULL);
}

/*----------------------------------------------------------------*/
/* "Hide" an object (must have a dependency or else it disappears)*/
/*----------------------------------------------------------------*/

void cathide()
{
   int i;
   short *newselect;
   objectptr *compobj;
   objinstptr libobj;

   if (areawin->selects == 0) return;

   /* Can only hide objects which are instances in other objects; */
   /* Otherwise, object would be "lost".			  */

   for (newselect = areawin->selectlist; newselect < areawin->selectlist
	  + areawin->selects; newselect++) {
      libobj = SELTOOBJINST(newselect);

      if (finddepend(libobj, &compobj) == 0) {
	 Wprintf("Cannot hide: no dependencies");
      }
      else { 		/* All's clear to hide. */
	 libobj->thisobject->hidden = True;
      }
   }

   clearselects();

   if ((i = is_library(topobject)) >= 0) composelib(LIBRARY + i);

   drawarea(NULL, NULL, NULL);
}

/*----------------------------------------------------------------*/
/* Delete an object from the library if there are no dependencies */
/*----------------------------------------------------------------*/

void catdelete()
{
   short *newselect, *libpobjs;
   int i;
   /* genericptr *testobj, *tobj; (jdk) */
   objinstptr libobj;
   liblistptr ilist, llist;
   objectptr *libpage, *compobj, *tlib, *slib;

   if (areawin->selects == 0) return;

   if ((i = is_library(topobject)) >= 0) {
      libpage = xobjs.userlibs[i].library;
      libpobjs = &xobjs.userlibs[i].number;
   }
   else
      return;  /* To-do: Should have a mechanism here for deleting pages! */

   for (newselect = areawin->selectlist; newselect < areawin->selectlist
	  + areawin->selects; newselect++) {
      libobj = SELTOOBJINST(newselect);

      /* If this is just a "virtual copy", simply remove it from the list */

      llist = NULL;
      for (ilist = xobjs.userlibs[i].instlist; ilist != NULL;
		llist = ilist, ilist = ilist->next) {
	 if ((ilist->thisinst == libobj) && (ilist->virtual == TRUE)) {
	    if (llist == NULL)
	       xobjs.userlibs[i].instlist = ilist->next;
	    else
	       llist->next = ilist->next;
	    break;
	 }
      }
      if (ilist != NULL) {
         free(ilist);
	 continue;
      }

      /* Cannot delete an object if another object uses an instance of it, */
      /* or if the object is used on a page.				   */

      if (finddepend(libobj, &compobj)) {
	 Wprintf("Cannot delete: dependency in \"%s\"", (*compobj)->name);
      }
      else { 		/* All's clear to delete.		     	   */

         /* Clear the undo stack so that any references to this object	*/
	 /* won't screw up the database (this could be kinder & gentler	*/
	 /* by first checking if there are any references to the object	*/
	 /* in the undo stack. . .					*/

	 flush_undo_stack();

	 /* Next, remove the object from the library page. */

	 for (tlib = libpage; tlib < libpage + *libpobjs; tlib++)
	    if ((*tlib) == libobj->thisobject) {
	       for (slib = tlib; slib < libpage + *libpobjs - 1; slib++)
		  (*slib) = (*(slib + 1));
	       (*libpobjs)--;
	       break;
	    }
	 
	 /* Next, remove all instances of the object on	the library page. */
	  
         llist = NULL;
         for (ilist = xobjs.userlibs[i].instlist; ilist != NULL;
		llist = ilist, ilist = ilist->next) {
	    if (ilist->thisinst->thisobject == libobj->thisobject) {
	       if (llist == NULL) {
	          xobjs.userlibs[i].instlist = ilist->next;
	          free(ilist);
	          if (!(ilist = xobjs.userlibs[i].instlist)) break;
	       }
	       else {
	          llist->next = ilist->next;
	          free(ilist);
	          if (!(ilist = llist)) break;
	       }
	    }
	 }

	 /* Finally, delete the object (permanent---no undoing this!) */
         tech_mark_changed(GetObjectTechnology(libobj->thisobject));
	 reset(libobj->thisobject, DESTROY);
      }
   }

   clearselects();

   if ((i = is_library(topobject)) >= 0) {
      composelib(LIBRARY + i);
   }

   drawarea(NULL, NULL, NULL);
}

/*------------------------------------------------------*/
/* Linked list rearrangement routines			*/
/*------------------------------------------------------*/

void linkedlistswap(liblistptr *spec, int o1, int o2)
{
   liblistptr s1, s1m, s2, s2m, stmp;
   int j;

   if (o1 == o2) return;

   s1m = NULL;
   s1 = *spec;
   for (j = 0; j < o1; j++) {
      s1m = s1;
      s1 = s1->next;
   }

   s2m = NULL;
   s2 = *spec;
   for (j = 0; j < o2; j++) {
      s2m = s2;
      s2 = s2->next;
   }

   if (s2m)
      s2m->next = s1;
   else
      *spec = s1;

   if (s1m)
      s1m->next = s2;
   else
      *spec = s2;

   stmp = s1->next;
   s1->next = s2->next;
   s2->next = stmp;
}

/*------------------------------------------------------*/

void linkedlistinsertafter(liblistptr *spec, int o1, int o2)
{
  liblistptr s1, s1m, s2; /* , s2m, stmp; (jdk) */
   int j;

   if ((o1 == o2) || (o1 == (o2 + 1))) return;

   s1m = NULL;
   s1 = *spec;
   for (j = 0; j < o1; j++) {
      s1m = s1;
      s1 = s1->next;
   }

   s2 = *spec;
   for (j = 0; j < o2; j++)
      s2 = s2->next;

   if (s1m)
      s1m->next = s1->next;
   else
      *spec = s1->next;

   if (o2 == -1) {   /* move s1 to front */
      s1->next = *spec;
      *spec = s1;
   }
   else {
      s1->next = s2->next;
      s2->next = s1;
   }
}

/*------------------------------------------------------*/
/* Set the "changed" flag in a library if any object	*/
/* in that library has changed.				*/
/*							*/
/* If "technology" is NULL, check all objects,		*/
/* otherwise only check objects with a matching		*/
/* technology.						*/
/*------------------------------------------------------*/

void tech_set_changes(TechPtr refns)
{
   TechPtr ns;
   int i, j;
   objectptr thisobj;

   for (i = 0; i < xobjs.numlibs; i++) {
      for (j = 0; j < xobjs.userlibs[i].number; j++) {
         thisobj = *(xobjs.userlibs[i].library + j);
         if (getchanges(thisobj) > 0) {
            ns = GetObjectTechnology(thisobj);
	    if ((refns == NULL) || (refns == ns)) 
	       ns->flags |= TECH_CHANGED;
	 }
      }
   }
}

/*------------------------------------------------------*/
/* Mark a technology as having been modified.		*/
/*------------------------------------------------------*/

void tech_mark_changed(TechPtr ns)
{
   if (ns != NULL) ns->flags |= TECH_CHANGED;
}

/*------------------------------------------------------*/
/* Rearrange objects in the library			*/
/*------------------------------------------------------*/

void catmove(int x, int y)
{
  int i, j, k, s1, s2, ocentx, ocenty, rangey, l; /* rangex, (jdk) */
   liblistptr spec;
   objinstptr exchobj, lobj;

   /* make catmove() a wrapper for pagecatmove() */

   if ((i = is_library(topobject)) < 0) {
      pagecatmove(x, y);
      return;
   }

   if (areawin->selects == 0) return;

   /* Add selected object or objects at the cursor position */

   window_to_user(x, y, &areawin->save);

   s2 = -1;
   for (j = 0, spec = xobjs.userlibs[i].instlist; spec != NULL;
		spec = spec->next, j++) {
      lobj = spec->thisinst;
      for (k = 0; k < areawin->selects; k++) {
	 exchobj = SELTOOBJINST(areawin->selectlist + k);
	 if (lobj == exchobj) break;
      }
      if (k < areawin->selects) continue;	/* ignore items being moved */

      ocentx = lobj->position.x + lobj->bbox.lowerleft.x
	        + (lobj->bbox.width >> 1);
      ocenty = lobj->position.y + lobj->bbox.lowerleft.y
	        + (lobj->bbox.height >> 1);
      rangey = (lobj->bbox.height > 200) ? 
	        (lobj->bbox.height >> 1) : 100;

      if ((areawin->save.y < ocenty + rangey) && (areawin->save.y 
	         > ocenty - rangey)) {
	 s2 = j - 1;
	 if (areawin->save.x < ocentx) break;
	 else s2 = j;
      }
   }
   if ((s2 == -1) && (spec == NULL)) {
      if (areawin->save.y <
		xobjs.libtop[i + LIBRARY]->thisobject->bbox.lowerleft.y)
	 s2 = j - 1;
      else if (areawin->save.y <=
		xobjs.libtop[i + LIBRARY]->thisobject->bbox.lowerleft.y +
		xobjs.libtop[i + LIBRARY]->thisobject->bbox.height) {
	 unselect_all();	
	 Wprintf("Could not find appropriate place to insert object");
	 return;
      }
   }

   /* Find object number s2 (because s2 value may change during insertion) */
   if (s2 > -1) {
      for (k = 0, spec = xobjs.userlibs[i].instlist; k < s2; spec = spec->next, k++);
      lobj = spec->thisinst;
   }
   else lobj = NULL;

   /* Move items; insert them after item s2 in order selected */

   j = i;
   for (k = 0; k < areawin->selects; k++) {

      /* Find number of lobj (may have changed) */

      if (lobj == NULL)
	 s2 = -1;
      else {
         for (s2 = 0, spec = xobjs.userlibs[i].instlist; spec != NULL;
		spec = spec->next, s2++)
	    if (spec->thisinst == lobj)
	       break;
      }

      exchobj = SELTOOBJINST(areawin->selectlist + k);
      for (s1 = 0, spec = xobjs.userlibs[i].instlist; spec != NULL;
		spec = spec->next, s1++)
         if (spec->thisinst == exchobj)
	    break;

      if (spec == NULL) {
	 /* Object came from another library */
         if ((l = libmoveobject(exchobj->thisobject, i)) >= 0) j = l;
      }
      else {
         linkedlistinsertafter(&(xobjs.userlibs[i].instlist), s1, s2);
      }
   }

   unselect_all();
   composelib(LIBRARY + i);
   if (j != i) {
      composelib(LIBRARY + j);
      centerview(xobjs.libtop[LIBRARY + j]);
   }

   drawarea(NULL, NULL, NULL);
}

/*------------------------------------------------------*/
/* Make a duplicate of an object, put in the User	*/
/* Library or the current library (if we're in one).	*/
/*							*/
/* Updated 2/8/2014 for use with technology namespaces:	*/
/* it is most likely that the copied object is to be	*/
/* modified but kept in the same namespace.  So, when	*/
/* renaming the object, prepend "_" to the object name	*/
/* instead of the namespace prefix.			*/
/*------------------------------------------------------*/

void copycat()
{
   short *newselect;
   objectptr *newobj, *curlib, oldobj;
   objinstptr libobj;
   oparamptr ops, newops;
   int i, libnum;
   char *cptr;

   libnum = is_library(topobject);
   if (libnum < 0) libnum = USERLIB - LIBRARY;  /* default */

   for (newselect = areawin->selectlist; newselect < areawin->selectlist
	  + areawin->selects; newselect++) {

      libobj = SELTOOBJINST(newselect);
      oldobj = libobj->thisobject;

      /* generate new entry in user library */

      curlib = (objectptr *) realloc(xobjs.userlibs[libnum].library,
	 (xobjs.userlibs[libnum].number + 1) * sizeof(objectptr));
      xobjs.userlibs[libnum].library = curlib;
      newobj = xobjs.userlibs[libnum].library + xobjs.userlibs[libnum].number;
      *newobj = (objectptr) malloc(sizeof(object));
      xobjs.userlibs[libnum].number++;

      /* give the new object a unique name */

      cptr = strstr(oldobj->name, "::");
      if (cptr == NULL)
	 sprintf((*newobj)->name, "_%s", oldobj->name);
      else {
	 strcpy((*newobj)->name, oldobj->name);
	 sprintf((*newobj)->name + (cptr - oldobj->name) + 2, "_%s", cptr + 2);
      }
      checkname(*newobj);

      /* copy other object properties */

      (*newobj)->bbox.width = oldobj->bbox.width;
      (*newobj)->bbox.height = oldobj->bbox.height;
      (*newobj)->bbox.lowerleft.x = oldobj->bbox.lowerleft.x;
      (*newobj)->bbox.lowerleft.y = oldobj->bbox.lowerleft.y;
      (*newobj)->pcorner.x = oldobj->pcorner.x;
      (*newobj)->pcorner.y = oldobj->pcorner.y;
      (*newobj)->viewscale = oldobj->viewscale;
      /* don't attach the same schematic. . . */
      (*newobj)->symschem = NULL;
      /* don't copy highlights */
      (*newobj)->highlight.netlist = NULL;
      (*newobj)->highlight.thisinst = NULL;

      /* Copy the parameter structure */
      (*newobj)->params = NULL;
      for (ops = oldobj->params; ops != NULL; ops = ops->next) {
	 newops = (oparamptr)malloc(sizeof(oparam));
	 newops->next = (*newobj)->params;
	 newops->key = strdup(ops->key);
	 (*newobj)->params = newops;
	 newops->type = ops->type;
	 newops->which = ops->which;
	 switch (ops->type) {
	    case XC_INT:
	       newops->parameter.ivalue = ops->parameter.ivalue;
	       break;
	    case XC_FLOAT:
	       newops->parameter.fvalue = ops->parameter.fvalue;
	       break;
	    case XC_STRING:
	       newops->parameter.string = stringcopy(ops->parameter.string);
	       break;
	    case XC_EXPR:
	       newops->parameter.expr = strdup(ops->parameter.expr);
	       break;
	 }
      }

      (*newobj)->schemtype = oldobj->schemtype;
      (*newobj)->netnames = NULL;
      (*newobj)->ports = NULL;
      (*newobj)->calls = NULL;
      (*newobj)->polygons = NULL;
      (*newobj)->labels = NULL;
      (*newobj)->valid = False;
      (*newobj)->traversed = False;
      (*newobj)->hidden = False;

      /* copy over all the elements of the original object */

      (*newobj)->parts = 0;
      (*newobj)->plist = (genericptr *)malloc(sizeof(genericptr));

      for (i = 0; i < oldobj->parts; i++) {
	 switch(ELEMENTTYPE(*(oldobj->plist + i))) {
	    case(PATH): {
	       register pathptr *npath, cpath = TOPATH(oldobj->plist + i);

	       NEW_PATH(npath, (*newobj));
	       pathcopy(*npath, cpath);
	       } break;

	    case(ARC): {
	       register arcptr *narc, carc = TOARC(oldobj->plist + i);

	       NEW_ARC(narc, (*newobj));
	       arccopy(*narc, carc);
               } break;
      
	    case(POLYGON): {
	       register polyptr *npoly, cpoly = TOPOLY(oldobj->plist + i);

	       NEW_POLY(npoly, (*newobj));
	       polycopy(*npoly, cpoly);
               } break;

	    case(SPLINE): {
	       register splineptr *nspl, cspl = TOSPLINE(oldobj->plist + i);

	       NEW_SPLINE(nspl, (*newobj));
	       splinecopy(*nspl, cspl);
      	       } break;

	    case(LABEL): {
	       register labelptr *nlabel, clabel = TOLABEL(oldobj->plist + i);

	       NEW_LABEL(nlabel, (*newobj));
	       labelcopy(*nlabel, clabel);
               } break;
      
	    case(OBJINST): {
	       register objinstptr *ninst, cinst = TOOBJINST(oldobj->plist + i);

	       NEW_OBJINST(ninst, (*newobj));
	       instcopy(*ninst, cinst);
               } break;
         }
      }
   }

   /* make instance for library and measure its bounding box */
   addtoinstlist(USERLIB - LIBRARY, *newobj, FALSE);

   composelib(USERLIB);
   unselect_all();

   if (areawin->topinstance == xobjs.libtop[USERLIB])
      drawarea(NULL, NULL, NULL);
   else startcatalog(NULL, (pointertype)USERLIB, NULL);
   zoomview(NULL, NULL, NULL);
}

/*--------------------------------------------------------*/
/* ButtonPress handler during normal catalog viewing mode */
/*--------------------------------------------------------*/

void catalog_op(int op, int x, int y)
{
   short *newselect;
   objinstptr *newobject, *libobj;
   objectptr libpage = topobject;
   short ocentx, ocenty, rangex, rangey, xdiff, ydiff, flag = 0;
   XPoint oldpos;

   /* Make catalog_op() a wrapper for pagecat_op() */

   if (is_library(topobject) < 0) {
      pagecat_op(op, x, y);
      return;
   }

   /* If XCF_Cancel was invoked, return without a selection. */

   if (op == XCF_Cancel) {
      eventmode = NORMAL_MODE;
      catreturn();
   }
   else {

      window_to_user(x, y, &areawin->save);
	
      for (libobj = (objinstptr *)topobject->plist; libobj <
		(objinstptr *)topobject->plist + topobject->parts; libobj++) {
	 if (IS_OBJINST(*libobj)) {

 	    ocentx = (*libobj)->position.x + (*libobj)->bbox.lowerleft.x
	        + ((*libobj)->bbox.width >> 1);
 	    ocenty = (*libobj)->position.y + (*libobj)->bbox.lowerleft.y
	        + ((*libobj)->bbox.height >> 1);

	    rangex = ((*libobj)->bbox.width > 200) ? 
	        ((*libobj)->bbox.width >> 1) : 100;
	    rangey = ((*libobj)->bbox.height > 200) ? 
	        ((*libobj)->bbox.height >> 1) : 100;

	    if (areawin->save.x > ocentx - rangex && areawin->save.x <
		   ocentx + rangex && areawin->save.y < ocenty + rangey
		   && areawin->save.y > ocenty - rangey) {

	       /* setup to move object around and draw the selected object */

	       if (eventmode == ASSOC_MODE) {

	          /* revert to old page */
		  topobject->viewscale = areawin->vscale;
		  topobject->pcorner = areawin->pcorner;
	          areawin->topinstance = (areawin->stack == NULL) ?
			   xobjs.pagelist[areawin->page]->pageinst
			   : areawin->stack->thisinst;
		  /* associate the new object */
		  schemassoc(topobject, (*libobj)->thisobject); 
   	          setpage(TRUE);
		  catreturn();
		  eventmode = NORMAL_MODE;
	       }
	       else if ((op == XCF_Library_Pop) || (op == XCF_Library_Copy)) {
		  int saveselects;

	          /* revert to old page */

		  topobject->viewscale = areawin->vscale;
		  topobject->pcorner = areawin->pcorner;
	          areawin->topinstance = (areawin->stack == NULL) ?
			   xobjs.pagelist[areawin->page]->pageinst
			   : areawin->stack->thisinst;

	          /* retrieve drawing window state and set position of object to
	             be correct in reference to that window */

	          snap(x, y, &oldpos);

		  saveselects = areawin->selects;
		  areawin->selects = 0;
   	          setpage(FALSE);
		  areawin->selects = saveselects;
	    
	          snap(x, y, &areawin->save);
	          xdiff = areawin->save.x - oldpos.x;
	          ydiff = areawin->save.y - oldpos.y;

                  /* collect all of the selected items */

	          for (newselect = areawin->selectlist; newselect <
		     areawin->selectlist + areawin->selects; newselect++) {
		     NEW_OBJINST(newobject, topobject);
		     instcopy(*newobject, TOOBJINST(libpage->plist + *newselect));
		     /* color should be recast as current color */
		     (*newobject)->color = areawin->color;
		     /* position modified by (xdiff, ydiff) */
		     (*newobject)->position.x += xdiff;
		     (*newobject)->position.y += ydiff;

	             u2u_snap(&((*newobject)->position));
		     *newselect = (short)(newobject - (objinstptr *)topobject->plist);
		     if ((*newobject)->thisobject == (*libobj)->thisobject)
		        flag = 1;
	          }

                  /* add final object to the list of object instances */

	          if (!flag) {
		     NEW_OBJINST(newobject, topobject);
		     instcopy(*newobject, *libobj);
		     (*newobject)->color = areawin->color;
                     (*newobject)->position.x = areawin->save.x;
	             (*newobject)->position.y = areawin->save.y; 

	             /* add this object to the list of selected items */

	             newselect = allocselect();
                     *newselect = (short)(newobject - (objinstptr *)topobject->plist);

	          }
	          if (op == XCF_Library_Copy) {

		     /* Key "c" pressed for "copy" (default binding) */

                     XDefineCursor(dpy, areawin->window, COPYCURSOR);
                     eventmode = COPY_MODE;
#ifndef TCL_WRAPPER
                     xcAddEventHandler(areawin->area, PointerMotionMask |
				ButtonMotionMask, False,
				(xcEventHandler)xlib_drag, NULL);
#endif
	          }
	          else {
                     eventmode = MOVE_MODE;
		     was_preselected = FALSE;
	 	     register_for_undo(XCF_Library_Pop, UNDO_MORE, areawin->topinstance,
		  		areawin->selectlist, areawin->selects);
	          }
#ifdef TCL_WRAPPER
		  /* fprintf(stderr, "Creating event handler for xctk_drag: "); */
		  /* printeventmode();		*/
                  Tk_CreateEventHandler(areawin->area, PointerMotionMask |
			ButtonMotionMask, (Tk_EventProc *)xctk_drag, NULL);
#endif
                  catreturn();
               }

               /* Select the closest element and stay in the catalog.	   */
	       /* Could just do "select_element" here, but would not cover */
	       /* the entire area in the directory surrounding the object. */

	       else if (op == XCF_Select) {
		  short newinst = (short)(libobj - (objinstptr *)topobject->plist);
		  /* (ignore this object if it is already in the list of selects) */
		  for (newselect = areawin->selectlist; newselect <
			areawin->selectlist + areawin->selects; newselect++)
		     if (*newselect == newinst) break;
		  if (newselect == areawin->selectlist + areawin->selects) {
		     newselect = allocselect();
		     *newselect = newinst;
		     XcTopSetForeground(SELECTCOLOR);
		     UDrawObject(*libobj, SINGLE, SELECTCOLOR,
				xobjs.pagelist[areawin->page]->wirewidth, NULL);
		  }
	       }
	       break;
	    }
	 }
      }
   }
}

/*------------------------------*/
/* Switch to the next catalog	*/
/*------------------------------*/

void changecat()
{
   int i, j;

   if ((i = is_library(topobject)) < 0) {
      if (areawin->lastlibrary >= xobjs.numlibs) areawin->lastlibrary = 0;
      j = areawin->lastlibrary;
      eventmode = CATALOG_MODE;
   }
   else {
      j = (i + 1) % xobjs.numlibs;
      if (j == i) {
	 Wprintf("This is the only library.");
	 return;
      }
      areawin->lastlibrary = j;
   }

   if (eventmode == CATMOVE_MODE)
      delete_for_xfer(NORMAL, areawin->selectlist, areawin->selects);

   startcatalog(NULL, (pointertype)(j + LIBRARY), NULL);
}

/*--------------------------------------*/
/* Begin catalog viewing mode		*/
/*--------------------------------------*/

void startcatalog(xcWidget w, pointertype libmod, caddr_t nulldata)
{
   if (xobjs.libtop == NULL) return;	/* No libraries defined */

   if ((xobjs.libtop[libmod]->thisobject == NULL) ||
		(areawin->topinstance == xobjs.libtop[libmod])) return;

   if (libmod == FONTLIB) {
      XDefineCursor (dpy, areawin->window, DEFAULTCURSOR);
      if (eventmode == TEXT_MODE)
         eventmode = FONTCAT_MODE;
      else
         eventmode = EFONTCAT_MODE;
   }
   else if (eventmode == ASSOC_MODE) {
      XDefineCursor (dpy, areawin->window, DEFAULTCURSOR);
   }
   else if (libmod == PAGELIB || libmod == LIBLIB) {
      XDefineCursor (dpy, areawin->window, DEFAULTCURSOR);
      eventmode = CATALOG_MODE;
   }
   else if (eventmode != CATMOVE_MODE) {
      /* Don't know why I put this here---causes xcircuit to redraw	*/
      /* the schematic view when switching between library pages.	*/
      // finish_op(XCF_Cancel, 0, 0);
      eventmode = CATALOG_MODE;
   }

   /* Push the current page onto the push stack, unless	we're going */
   /* to a library from the library directory or vice versa, or	    */
   /* library to library.					    */

   if (!(((is_library(topobject) >= 0)
		|| (areawin->topinstance == xobjs.libtop[LIBLIB])
		|| (areawin->topinstance == xobjs.libtop[PAGELIB]))
		&& libmod >= PAGELIB)) {
      push_stack(&areawin->stack, areawin->topinstance, NULL);
   }

   /* set library as new object */

   topobject->viewscale = areawin->vscale;
   topobject->pcorner = areawin->pcorner;
   areawin->topinstance = xobjs.libtop[libmod];

   if (libmod == FONTLIB)
      setpage(FALSE);
   else {
      setpage(TRUE);
      transferselects();
   }

   /* draw the new screen */

   refresh(NULL, NULL, NULL);
}

/*-------------------------------------------------------------------------*/
