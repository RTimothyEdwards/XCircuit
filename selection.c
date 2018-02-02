/*-------------------------------------------------------------------------*/
/* selection.c --- xcircuit routines handling element selection etc.	   */
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University       	   */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/*      written by Tim Edwards, 8/13/93    				   */
/*-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

/*-------------------------------------------------------------------------*/
/* Local includes							   */
/*-------------------------------------------------------------------------*/

#include "xcircuit.h"
#include "colordefs.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*----------------------------------------------------------------------*/
/* Exported Variable definitions					*/
/*----------------------------------------------------------------------*/

extern Display	*dpy;
extern Cursor	appcursors[NUM_CURSORS];
extern XCWindowData *areawin;
extern Globaldata xobjs;
extern char _STR[150];
extern int number_colors;
extern colorindex *colorlist;

#ifdef TCL_WRAPPER 
extern Tcl_Interp *xcinterp;
#endif

/*----------------------------------------------------------------------*/
/* Prevent a list of elements from being selected.			*/
/*----------------------------------------------------------------------*/

void disable_selects(objectptr thisobject, short *selectlist, int selects)
{
   genericptr genptr;
   short *i;

   for (i = selectlist; i < selectlist + selects; i++) {
      genptr = *(thisobject->plist + *i);
      genptr->type |= SELECT_HIDE;
   }
}

/*----------------------------------------------------------------------*/
/* Allow a list of elements to be selected, if they were disabled using	*/
/* the disable_selects() routine.					*/
/*----------------------------------------------------------------------*/

void enable_selects(objectptr thisobject, short *selectlist, int selects)
{
   genericptr genptr;
   short *i;

   for (i = selectlist; i < selectlist + selects; i++) {
      genptr = *(thisobject->plist + *i);
      genptr->type &= ~(SELECT_HIDE);
   }
}

/*----------------------------------------------------------------------*/
/* Change filter to determine what types can be selected		*/
/*----------------------------------------------------------------------*/

void selectfilter(xcWidget w, pointertype value, caddr_t calldata)
{
   short bitwise = (short)value;
   Boolean bval = (areawin->filter & bitwise) ? True : False;

   if (bval)
      areawin->filter &= ~bitwise;
   else
      areawin->filter |= bitwise;

#ifndef TCL_WRAPPER
   toggle(w, (pointertype)&bval, calldata);
#endif
}

/*----------------------------------------------------------------------*/
/* Look at select stack to see if there are any selects; call select,	*/
/* if not.  If draw_selected is True, then the selected items are drawn	*/
/* in the select color.	 Otherwise, they are not redrawn.		*/
/*----------------------------------------------------------------------*/

Boolean checkselect_draw(short value, Boolean draw_selected)
{
   short *check;
 
   value &= areawin->filter;	/* apply the selection filter */

   if (areawin->selects == 0) {
      if (draw_selected)
         select_element(value);
      else {
         Boolean save_redraw = areawin->redraw_needed;
         select_element(value);
         areawin->redraw_needed = save_redraw;
      }
   }
   if (areawin->selects == 0) return False;
   for (check = areawin->selectlist; check < areawin->selectlist +
	areawin->selects; check++)
      if (SELECTTYPE(check) & value) break;
   if (check == areawin->selectlist + areawin->selects) return False;
   else return True;
}

/*----------------------------------------------------------------------------*/
/* Look at select stack to see if there are any selects; call select, if not. */
/*----------------------------------------------------------------------------*/

Boolean checkselect(short value)
{
    return checkselect_draw(value, False);
}

/*--------------------------------------------------------------*/
/* Select list numbering revision when an element is deleted	*/
/* from an object. 						*/
/*--------------------------------------------------------------*/

void reviseselect(short *slist, int selects, short *removed)
{
   short *chkselect;

   for (chkselect = slist; chkselect < slist + selects; chkselect++)
      if (*chkselect > *removed) (*chkselect)--;
}

/*----------------------*/
/* Draw a selected item */
/*----------------------*/

void geneasydraw(short instance, int mode, objectptr curobj, objinstptr curinst)
{
   genericptr elementptr = *(curobj->plist + instance);

#ifdef HAVE_CAIRO
   cairo_save(areawin->cr);
   cairo_reset_clip(areawin->cr);
#else
   // Note:  Setting areawin->clipped to -1 prevents the clipmask from being
   // applied to elements, 
   areawin->clipped = -1;
#endif /* HAVE_CAIRO */

   switch (ELEMENTTYPE(*(curobj->plist + instance))) {
      case ARC:
         UDrawArc((arcptr)elementptr, xobjs.pagelist[areawin->page]->wirewidth);
	 break;
      case POLYGON:
         UDrawPolygon((polyptr)elementptr, xobjs.pagelist[areawin->page]->wirewidth);
	 break;
      case SPLINE:
	 UDrawSpline((splineptr)elementptr, xobjs.pagelist[areawin->page]->wirewidth);
	 break;
      case PATH:
	 UDrawPath((pathptr)elementptr, xobjs.pagelist[areawin->page]->wirewidth);
	 break;
      case LABEL:
         UDrawString((labelptr)elementptr, mode, curinst);
	 break;
      case OBJINST:
         UDrawObject((objinstptr)elementptr, SINGLE, mode,
			xobjs.pagelist[areawin->page]->wirewidth, NULL);
         break;
      case GRAPHIC:
         UDrawGraphic((graphicptr)elementptr);
	 break;
   }
#ifdef HAVE_CAIRO
   cairo_restore(areawin->cr);
#else
   areawin->clipped = 0;
#endif /* HAVE_CAIRO */
}

/*-------------------------------------------------*/
/* Draw a selected item, including selection color */
/*-------------------------------------------------*/

void gendrawselected(short *newselect, objectptr curobj, objinstptr curinst)
{
   if (*newselect >= curobj->parts) return;	// Safety check

   XcSetForeground(SELECTCOLOR);
   geneasydraw(*newselect, DOFORALL, curobj, curinst);

   SetForeground(dpy, areawin->gc, AUXCOLOR);
   indicateparams(*(curobj->plist + *newselect));

   SetForeground(dpy, areawin->gc, areawin->gccolor);
}

/*---------------------------------------------------*/
/* Allocate or reallocate memory for a new selection */
/*---------------------------------------------------*/

short *allocselect()
{
   short *newselect;

   if (areawin->selects == 0)
      areawin->selectlist = (short *) malloc(sizeof(short));
   else
      areawin->selectlist = (short *) realloc(areawin->selectlist,
	 (areawin->selects + 1) * sizeof(short));

   newselect = areawin->selectlist + areawin->selects;
   areawin->selects++;

   return newselect;
}

/*-------------------------------------------------*/
/* Set Options menu according to 1st selection	   */
/*-------------------------------------------------*/

void setoptionmenu()
{
   short      *mselect;
   labelptr   mlabel;

   if (areawin->selects == 0) {
      setallstylemarks(areawin->style);
      setcolormark(areawin->color);
      setdefaultfontmarks();
      setparammarks(NULL);
      return;
   }

   for (mselect = areawin->selectlist; mselect < areawin->selectlist +
	areawin->selects; mselect++) {
      setcolormark(SELTOCOLOR(mselect));
      setparammarks(SELTOGENERIC(mselect));
      switch(SELECTTYPE(mselect)) {
	 case ARC:
	    setallstylemarks(SELTOARC(mselect)->style);
	    return;
	 case POLYGON:
	    setallstylemarks(SELTOPOLY(mselect)->style);
	    return;
	 case SPLINE:
            setallstylemarks(SELTOSPLINE(mselect)->style);
	    return;
	 case PATH:
            setallstylemarks(SELTOPATH(mselect)->style);
	    return;
	 case LABEL:
	    mlabel = SELTOLABEL(mselect);
	    setfontmarks(mlabel->string->data.font, mlabel->anchor);
	    return;
      }
   }
}

/*-------------------------------------*/
/* Test of point being inside of a box */
/*-------------------------------------*/

int test_insideness(int tx, int ty, XPoint *boxpoints)
{
   int i, stval = 0; 
   XPoint *pt1, *pt2;
   int stdir;   

   for (i = 0; i < 4; i++) {
      pt1 = boxpoints + i;
      pt2 = boxpoints + ((i + 1) % 4);
      stdir = (pt2->x - pt1->x) * (ty - pt1->y)
		- (pt2->y - pt1->y) * (tx - pt1->x);
      stval += sign(stdir);
   }
   return (abs(stval) == 4) ? 1 : 0;
}

/*--------------------------------------------*/
/* Search for selection among path components */
/*--------------------------------------------*/

#define RANGE_NARROW 11.5
#define RANGE_WIDE 50

Boolean pathselect(genericptr *curgen, short class, float range)
{
   /*----------------------------------------------------------------------*/
   /* wirelim is the distance, in user-space units, at which an element is */
   /* considered for selection.						   */
   /*									   */
   /* wirelim = A + B / (scale + C)					   */
   /*									   */
   /* where A = minimum possible distance (expands range at close scale)   */
   /*       C = minimum possible scale    (contract range at far scale)	   */
   /*	    B   makes wirelim approx. 25 at default scale of 0.5, which	   */
   /*		is an empirical result.					   */
   /*----------------------------------------------------------------------*/

   float	wirelim = 2 + range / (areawin->vscale + 0.05);
   long		sqrwirelim = (int)(wirelim * wirelim);

   long		newdist;
   Boolean	selected = False;

   class &= areawin->filter;	/* apply the selection filter */

   if ((*curgen)->type == (class & ARC)) {

      /* look among the arcs */

      fpointlist currentpt;
      XPoint nearpt[3];

      nearpt[2].x = nearpt[0].x = (short)(TOARC(curgen)->points[0].x);
      nearpt[2].y = nearpt[0].y = (short)(TOARC(curgen)->points[0].y);
      for (currentpt = TOARC(curgen)->points + 1; currentpt < TOARC(curgen)->points
              + TOARC(curgen)->number; currentpt++) {
	 nearpt[1].x = nearpt[0].x;
	 nearpt[1].y = nearpt[0].y;
	 nearpt[0].x = (short)(currentpt->x);
	 nearpt[0].y = (short)(currentpt->y);
	 newdist = finddist(&nearpt[0], &nearpt[1], &areawin->save);
         if (newdist <= sqrwirelim) break;
      }
      if ((!(TOARC(curgen)->style & UNCLOSED)) && (newdist > sqrwirelim))
	 newdist = finddist(&nearpt[0], &nearpt[2], &areawin->save);

      if (newdist <= sqrwirelim) selected = True;
   }

   else if ((*curgen)->type == (class & SPLINE)) {

      /* look among the splines --- look at polygon representation */

      fpointlist currentpt;
      XPoint nearpt[2];

      nearpt[0].x = (short)(TOSPLINE(curgen)->points[0].x);
      nearpt[0].y = (short)(TOSPLINE(curgen)->points[0].y);
      newdist = finddist(&(TOSPLINE(curgen)->ctrl[0]), &(nearpt[0]),
		   &areawin->save);
      if (newdist > sqrwirelim) {
         for (currentpt = TOSPLINE(curgen)->points; currentpt <
		  TOSPLINE(curgen)->points + INTSEGS; currentpt++) {
	    nearpt[1].x = nearpt[0].x;
	    nearpt[1].y = nearpt[0].y;
	    nearpt[0].x = (short)(currentpt->x);
	    nearpt[0].y = (short)(currentpt->y);
	    newdist = finddist(&nearpt[0], &nearpt[1], &areawin->save);
            if (newdist <= sqrwirelim) break;
	 }
	 if (newdist > sqrwirelim) {
	    newdist = finddist(&nearpt[0], &(TOSPLINE(curgen)->ctrl[3]),
			&areawin->save);
            if ((!(TOSPLINE(curgen)->style & UNCLOSED)) && (newdist > sqrwirelim))
	       newdist = finddist(&(TOSPLINE(curgen)->ctrl[0]),
		     &(TOSPLINE(curgen)->ctrl[3]), &areawin->save);
	 }
      }

      if (newdist <= sqrwirelim) selected = True;
   }

   else if ((*curgen)->type == (class & POLYGON)) {

      /* finally, look among the polygons */

      pointlist currentpt;

      for (currentpt = TOPOLY(curgen)->points; currentpt < TOPOLY(curgen)
		->points + TOPOLY(curgen)->number - 1; currentpt++) {
	 newdist = finddist(currentpt, currentpt + 1, &areawin->save);
	 if (newdist <= sqrwirelim) break;
      }
      if ((!(TOPOLY(curgen)->style & UNCLOSED)) && (newdist > sqrwirelim))
	 newdist = finddist(currentpt, TOPOLY(curgen)->points,
		&areawin->save);

      if (newdist <= sqrwirelim) selected = True;
   }
   return selected;
}

/*------------------------------------------------------*/
/* Check to see if any selection has registered cycles	*/
/*------------------------------------------------------*/

Boolean  checkforcycles(short *selectlist, int selects)
{
   genericptr pgen;
   pointselect *cycptr;
   short *ssel;

   for (ssel = selectlist; ssel < selectlist + selects; ssel++) {
      pgen = SELTOGENERIC(ssel);
      switch(pgen->type) {
         case POLYGON:
	    cycptr = ((polyptr)pgen)->cycle;
	    break;
         case ARC:
	    cycptr = ((arcptr)pgen)->cycle;
	    break;
         case SPLINE:
	    cycptr = ((splineptr)pgen)->cycle;
	    break;
         case LABEL:
	    cycptr = ((labelptr)pgen)->cycle;
	    break;
      }
      if (cycptr != NULL)
	 if (cycptr->number != -1)
	    return True;
   }
   return False;
}

/*--------------------------------------------------------------*/
/* Copy a cycle selection list from one element to another	*/
/*--------------------------------------------------------------*/

void copycycles(pointselect **new, pointselect **old)
{
   pointselect *pptr;
   short cycles = 0;

   if (*old == NULL) {
      *new = NULL;
      return;
   }

   for (pptr = *old; !(pptr->flags & LASTENTRY); pptr++, cycles++);
   cycles += 2;
   *new = (pointselect *)malloc(cycles * sizeof(pointselect));
   memcpy(*new, *old, cycles * sizeof(pointselect));
}

/*--------------------------------------------------------------*/
/* Create a selection record of selected points in an element.	*/
/* If a record already exists, and "cycle" is not already in	*/
/* the list, add it.						*/
/* "flags" may be set to EDITX or EDITY.  If "flags" is zero,	*/
/* then flags = EDITX | EDITY is assumed.			*/
/*--------------------------------------------------------------*/

pointselect *addcycle(genericptr *pgen, short cycle, u_char flags)
{
   polyptr ppoly;
   arcptr parc;
   splineptr pspline;
   labelptr plabel;
   pointselect *pptr, **cycptr;
   short cycles = 0;

   switch((*pgen)->type) {
      case POLYGON:
	 ppoly = TOPOLY(pgen);
	 cycptr = &ppoly->cycle;
	 break;
      case ARC:
	 parc = TOARC(pgen);
	 cycptr = &parc->cycle;
	 break;
      case SPLINE:
	 pspline = TOSPLINE(pgen);
	 cycptr = &pspline->cycle;
	 break;
      case LABEL:
         plabel = TOLABEL(pgen);
	 cycptr = &plabel->cycle;
	 break;
   }

   switch((*pgen)->type) {
      case POLYGON:
      case ARC:
      case SPLINE:
      case LABEL:		// To-do:  Handle labels separately

	 if (*cycptr == NULL) {
	    *cycptr = (pointselect *)malloc(sizeof(pointselect));
	    pptr = *cycptr;
	    pptr->number = cycle;
	    pptr->flags = (flags == 0) ? EDITX | EDITY : flags;
	    pptr->flags |= LASTENTRY;
	 }
	 else {
	    for (pptr = *cycptr; !(pptr->flags & LASTENTRY); pptr++, cycles++) {
	       if (pptr->number == cycle)
		  break;
	       pptr->flags &= ~LASTENTRY;
	    }
	    if (pptr->number != cycle) {
	       pptr->flags &= ~LASTENTRY;
	       *cycptr = (pointselect *)realloc(*cycptr,
			(cycles + 2) * sizeof(pointselect));
	       pptr = *cycptr + cycles + 1;
	       pptr->number = cycle;
	       pptr->flags = (flags == 0) ? EDITX | EDITY : flags;
	       pptr->flags |= LASTENTRY;
	    }
	    else {
	       pptr->flags |= (flags == 0) ? EDITX | EDITY : flags;
	    }
	 }
         break;
   }
   return pptr;
}

/*--------------------------------------------------------------*/
/* If we edit the position of a control point, and the global	*/
/* pathedit mode is set to TANGENTS, then we track the angle of	*/
/* the adjoining curve, if there is one, by settings its cycle	*/
/* flags to ANTIXY.						*/
/*--------------------------------------------------------------*/

void addanticycle(pathptr thispath, splineptr thisspline, short cycle)
{
   genericptr *ggen, *rgen;
   splineptr otherspline;

   if (areawin->pathedit == TANGENTS) {
      for (ggen = thispath->plist; ggen < thispath->plist + thispath->parts;
		ggen++)
	 if (*ggen == (genericptr)thisspline) break;

      if (*ggen != (genericptr)thisspline) return;	/* Something went wrong */

      if (cycle == 1) {
	 if (ggen > thispath->plist) {
	    if (ELEMENTTYPE(*(ggen - 1)) == SPLINE) {
	       addcycle(ggen - 1, 2, ANTIXY);
	    }
	 }
	 else if (!(thispath->style & UNCLOSED)) {
	    rgen = thispath->plist + thispath->parts - 1;
	    if (ELEMENTTYPE(*rgen) == SPLINE) {
	       otherspline = TOSPLINE(rgen);
	       if (thisspline->ctrl[0].x == otherspline->ctrl[3].x &&
			thisspline->ctrl[0].y == otherspline->ctrl[3].y)
	          addcycle(rgen, 2, ANTIXY);
	    }
	 }
      }
      else if (cycle == 2) {	/* cycle should be only 1 or 2 */
	 if (ggen < thispath->plist + thispath->parts - 1) {
	    if (ELEMENTTYPE(*(ggen + 1)) == SPLINE) {
	       addcycle(ggen + 1, 1, ANTIXY);
	    }
	 }
	 else if (!(thispath->style & UNCLOSED)) {
	    rgen = thispath->plist;
	    if (ELEMENTTYPE(*rgen) == SPLINE) {
	       otherspline = TOSPLINE(rgen);
	       if (thisspline->ctrl[3].x == otherspline->ctrl[0].x &&
			thisspline->ctrl[3].y == otherspline->ctrl[0].y)
	          addcycle(rgen, 1, ANTIXY);
	    }
	 }
      }
   }
}

/*--------------------------------------------------------------*/
/* Find the cycle numbered "cycle", and mark it as the		*/
/* reference point.						*/
/*--------------------------------------------------------------*/

void makerefcycle(pointselect *cycptr, short cycle)
{
   pointselect *pptr, *sptr;

   for (pptr = cycptr;; pptr++) {
      if (pptr->flags & REFERENCE) {
	 pptr->flags &= ~REFERENCE;
	 break;
      }
      if (pptr->flags & LASTENTRY) break;
   }

   for (sptr = cycptr;; sptr++) {
      if (sptr->number == cycle) {
	 sptr->flags |= REFERENCE;
	 break;
      }
      if (sptr->flags & LASTENTRY) break;
   }

   /* If something went wrong, revert to the original reference */

   if (!(sptr->flags & REFERENCE)) {
      pptr->flags |= REFERENCE;
   }
}
      
/* Original routine, used 1st entry as reference (deprecated) */

void makefirstcycle(pointselect *cycptr, short cycle)
{
   pointselect *pptr, tmpp;

   for (pptr = cycptr;; pptr++) {
      if (pptr->number == cycle) {
	 /* swap with first entry */
	 tmpp = *cycptr;
	 *cycptr = *pptr;
	 *pptr = tmpp;
	 if (cycptr->flags & LASTENTRY) {
	    cycptr->flags &= ~LASTENTRY;
	    pptr->flags |= LASTENTRY;
	 }
	 return;
      }
      if (pptr->flags & LASTENTRY) break;
   }
}

/*------------------------------------------------------------------------------*/
/* Advance a cycle (point) selection from value "cycle" to value "newvalue"	*/
/* If "newvalue" is < 0 then remove the cycle.					*/
/*										*/
/* If there is only one cycle point on the element, then advance its point	*/
/* number.  If there are multiple points on the element, then change the	*/
/* reference point by moving the last item in the list to the front.		*/
/*------------------------------------------------------------------------------*/

void advancecycle(genericptr *pgen, short newvalue)
{
   polyptr ppoly;
   arcptr parc;
   splineptr pspline;
   labelptr plabel;
   pointselect *pptr, *endptr, *fcycle, **cycptr, tmpcyc;
   short cycles = 0;

   if (newvalue < 0) {
      removecycle(pgen);
      return;
   }

   switch((*pgen)->type) {
      case POLYGON:
	 ppoly = TOPOLY(pgen);
	 cycptr = &ppoly->cycle;
	 break;
      case ARC:
	 parc = TOARC(pgen);
	 cycptr = &parc->cycle;
	 break;
      case SPLINE:
	 pspline = TOSPLINE(pgen);
	 cycptr = &pspline->cycle;
	 break;
      case LABEL:
         plabel = TOLABEL(pgen);
	 cycptr = &plabel->cycle;
	 break;
   }
   if (*cycptr == NULL) return;

   /* Remove any cycles that have only X or Y flags set.	*/
   /* "Remove" them by shuffling them to the end of the list,	*/
   /* and marking the one in front as the last entry.		*/

   for (endptr = *cycptr; !(endptr->flags & LASTENTRY); endptr++);
   pptr = *cycptr;
   while (pptr < endptr) {
      if ((pptr->flags & (EDITX | EDITY)) != (EDITX | EDITY)) {
	 tmpcyc = *endptr; 
	 *endptr = *pptr;
	 *pptr = tmpcyc;
	 pptr->flags &= ~LASTENTRY;
	 endptr->number = -1;
	 endptr--;
	 endptr->flags |= LASTENTRY;
      }
      else
	 pptr++;
   }

   if (pptr->flags & LASTENTRY) {
      if ((pptr->flags & (EDITX | EDITY)) != (EDITX | EDITY)) {
	 pptr->flags &= ~LASTENTRY;
	 pptr->number = -1;
	 endptr--;
	 endptr->flags |= LASTENTRY;
      }
   }

   /* Now advance the cycle */

   pptr = *cycptr;
   if (pptr->flags & LASTENTRY) {
      pptr->number = newvalue;
   }
   else {
      fcycle = *cycptr;
      for (pptr = fcycle + 1;; pptr++) {
	 if (pptr->flags & (EDITX | EDITY))
	    fcycle = pptr;
	 if (pptr->flags & LASTENTRY) break;
      }
      makerefcycle(*cycptr, fcycle->number);
   }
}

/*--------------------------------------*/
/* Remove a cycle (point) selection	*/
/*--------------------------------------*/

void removecycle(genericptr *pgen)
{
   polyptr ppoly;
   pathptr ppath;
   arcptr parc;
   splineptr pspline;
   labelptr plabel;
   pointselect *pptr, **cycptr = NULL;
   genericptr *pathgen;

   switch((*pgen)->type) {
      case POLYGON:
	 ppoly = TOPOLY(pgen);
	 cycptr = &ppoly->cycle;
	 break;
      case ARC:
	 parc = TOARC(pgen);
	 cycptr = &parc->cycle;
	 break;
      case SPLINE:
	 pspline = TOSPLINE(pgen);
	 cycptr = &pspline->cycle;
	 break;
      case LABEL:
         plabel = TOLABEL(pgen);
	 cycptr = &plabel->cycle;
	 break;
      case PATH:
         ppath = TOPATH(pgen);
	 for (pathgen = ppath->plist; pathgen < ppath->plist + ppath->parts;
			pathgen++)
	    removecycle(pathgen);
	 break;
   }
   if (cycptr == NULL) return;
   if (*cycptr == NULL) return;
   free(*cycptr);
   *cycptr = NULL;
}

/*--------------------------------------*/
/* Remove cycles from all parts of a	*/
/* path other than the one passed	*/
/*--------------------------------------*/

void removeothercycles(pathptr ppath, genericptr pathpart)
{
   genericptr *pathgen;
   for (pathgen = ppath->plist; pathgen < ppath->plist + ppath->parts;
		pathgen++)
      if (*pathgen != pathpart)
	 removecycle(pathgen);
}

/*--------------------------------------*/
/* Select one of the elements on-screen */
/*--------------------------------------*/

selection *genselectelement(short class, u_char mode, objectptr selobj,
		objinstptr selinst)
{
   selection	*rselect = NULL;
   /* short	*newselect; (jdk) */
   genericptr	*curgen;
   XPoint 	newboxpts[4];
   Boolean	selected;
   float	range = RANGE_NARROW;

   if (mode == MODE_RECURSE_WIDE)
      range = RANGE_WIDE;

   /* Loop through all elements found underneath the cursor */

   for (curgen = selobj->plist; curgen < selobj->plist + selobj->parts; curgen++) {

      selected = False;

      /* Check among polygons, arcs, and curves */

      if (((*curgen)->type == (class & POLYGON)) ||
		((*curgen)->type == (class & ARC)) ||
		((*curgen)->type == (class & SPLINE))) {
	  selected = pathselect(curgen, class, range);
      }

      else if ((*curgen)->type == (class & LABEL)) {

         /* Look among the labels */

	 labelptr curlab = TOLABEL(curgen);

	 /* Don't select temporary labels from schematic capture system */
	 if (curlab->string->type != FONT_NAME) continue;

	 labelbbox(curlab, newboxpts, selinst);

	 /* Need to check for zero-size boxes or test_insideness()	*/
	 /* fails.  Zero-size boxes happen when labels are parameters	*/
	 /* set to a null string.					*/

	 if ((newboxpts[0].x != newboxpts[1].x) || (newboxpts[0].y !=
		newboxpts[1].y)) {
	
            /* check for point inside bounding box, as for objects */

	    selected = test_insideness(areawin->save.x, areawin->save.y,
		newboxpts);
	    if (selected) areawin->textpos = areawin->textend = 0;
	 }
      }

      else if ((*curgen)->type == (class & GRAPHIC)) {

         /* Look among the graphic images */

	 graphicptr curg = TOGRAPHIC(curgen);
	 graphicbbox(curg, newboxpts);

         /* check for point inside bounding box, as for objects */
	 selected = test_insideness(areawin->save.x, areawin->save.y,
		newboxpts);
      }

      else if ((*curgen)->type == (class & PATH)) {

         /* Check among the paths */

	 genericptr *pathp;

	 /* Accept path if any subcomponent of the path is accepted */

 	 for (pathp = TOPATH(curgen)->plist; pathp < TOPATH(curgen)->plist +
		TOPATH(curgen)->parts; pathp++)
	    if (pathselect(pathp, SPLINE|ARC|POLYGON, range)) {
	       selected = True;
	       break;
	    }
      }

      else if ((*curgen)->type == (class & OBJINST)) {

	 objinstbbox(TOOBJINST(curgen), newboxpts, range);

         /* Look for an intersect of the boundingbox and pointer position. */
         /* This is a simple matter of rotating the pointer position with  */
         /*  respect to the origin of the bounding box segment, as if the  */
         /*  segment were rotated to 0 degrees.  The sign of the resulting */
         /*  point's y-position is the same for all bbox segments if the   */
         /*  pointer position is inside the bounding box.		   */

	 selected = test_insideness(areawin->save.x, areawin->save.y,
		newboxpts);
      }

      /* Add this object to the list of things found under the cursor */

      if (selected) {
         if (rselect == NULL) {
	    rselect = (selection *)malloc(sizeof(selection));
	    rselect->selectlist = (short *)malloc(sizeof(short));
	    rselect->selects = 0;
	    rselect->thisinst = selinst;
	    rselect->next = NULL;
	 }
         else {
            rselect->selectlist = (short *)realloc(rselect->selectlist,
			(rselect->selects + 1) * sizeof(short));
	 }
	 *(rselect->selectlist + rselect->selects) = (short)(curgen -
		selobj->plist);
	 rselect->selects++;
      }
   }
   return rselect;
}

/*----------------------------------------------------------------*/
/* select arc, curve, and polygon objects from a defined box area */
/*----------------------------------------------------------------*/

Boolean areaelement(genericptr *curgen, XPoint *boxpts, Boolean is_path, short level)
{
   Boolean selected;
   pointlist    currentpt;
   short	cycle;

   switch(ELEMENTTYPE(*curgen)) {

      case(ARC):
	   /* check center of arcs */

	   selected = test_insideness(TOARC(curgen)->position.x,
		TOARC(curgen)->position.y, boxpts);
	   break;

      case(POLYGON):
	   /* check each point of the polygons */

	   selected = False;
	   cycle = 0;
           for (currentpt = TOPOLY(curgen)->points; currentpt <
		TOPOLY(curgen)->points + TOPOLY(curgen)->number;
		currentpt++, cycle++) {
	      if (test_insideness(currentpt->x, currentpt->y, boxpts)) {
	         selected = True;
		 if (level == 0) addcycle(curgen, cycle, 0);
	      }
	   }
	   break;

      case(SPLINE):
	   /* check each control point of the spline */
	
	   selected = False;
	   if (test_insideness(TOSPLINE(curgen)->ctrl[0].x,
			TOSPLINE(curgen)->ctrl[0].y, boxpts)) {
	      selected = True;
	      if (level == 0) addcycle(curgen, 0, 0);
	   }

	   if (test_insideness(TOSPLINE(curgen)->ctrl[3].x,
			TOSPLINE(curgen)->ctrl[3].y, boxpts)) {
	      selected = True;
	      if (level == 0) addcycle(curgen, 3, 0);
	   }
	   break;
   }
   return selected;
}

/*--------------------------------------------*/
/* select all objects from a defined box area */
/*--------------------------------------------*/

Boolean selectarea(objectptr selobj, XPoint *boxpts, short level)
{
   short	*newselect;
   genericptr   *curgen, *pathgen;
   Boolean	selected;
   stringpart	*strptr;
   int		locpos, cx, cy, hwidth, hheight;
   objinstptr	curinst;
   XPoint	newboxpts[4];

   if (selobj == topobject) {
      areawin->textpos = areawin->textend = 0;
   } 

   for (curgen = selobj->plist; curgen < selobj->plist + selobj->parts; curgen++) {

      /* apply the selection filter */
      if (!((*curgen)->type & areawin->filter)) continue;

      switch(ELEMENTTYPE(*curgen)) {
	case(OBJINST):
	    curinst = TOOBJINST(curgen);

	    /* An object instance is selected if any part of it is	*/
	    /* selected on a recursive area search.			*/

            InvTransformPoints(boxpts, newboxpts, 4, curinst->position,
			curinst->scale, curinst->rotation);
            selected = selectarea(curinst->thisobject, newboxpts, level + 1);
	    break;

	case(GRAPHIC):
           /* check for graphic image center point inside area box */
	   selected = test_insideness(TOGRAPHIC(curgen)->position.x,
		TOGRAPHIC(curgen)->position.y, boxpts);
	   break;

        case(LABEL): {
	   XPoint adj;
	   labelptr slab = TOLABEL(curgen);
	   short j, state, isect, tmpl1, tmpl2;
	   int padding;
	   TextExtents tmpext;
	   TextLinesInfo tlinfo;

	   selected = False;

	   /* Ignore temporary labels created by the netlist generator */
	   if (slab->string->type != FONT_NAME) break;

	   /* Ignore info and pin labels that are not on the top level */
	   if ((selobj != topobject) && (slab->pin != False)) break;

   	   /* translate select box into the coordinate system of the label */
	   InvTransformPoints(boxpts, newboxpts, 4, slab->position,
		  slab->scale, slab->rotation);

	   if (slab->pin) {
	      for (j = 0; j < 4; j++) 
		 pinadjust(slab->anchor, &(newboxpts[j].x),
			&(newboxpts[j].y), -1);
	   }

	   tlinfo.dostop = 0;
	   tlinfo.tbreak = NULL;
	   tlinfo.padding = NULL;

	   tmpext = ULength(slab, areawin->topinstance, &tlinfo);
	   adj.x = (slab->anchor & NOTLEFT ? (slab->anchor & RIGHT ? 
			tmpext.maxwidth : tmpext.maxwidth >> 1) : 0);
	   adj.y = (slab->anchor & NOTBOTTOM ? (slab->anchor & TOP ? 
			tmpext.ascent : (tmpext.ascent + tmpext.base) >> 1)
			: tmpext.base);

	   /* Label selection:  For each character in the label string, */
	   /* do an insideness test with the select box.		*/

	   state = tmpl2 = 0;
	   for (j = 0; j < stringlength(slab->string, True, areawin->topinstance); j++) {
	      strptr = findstringpart(j, &locpos, slab->string, areawin->topinstance);
	      if (locpos < 0) continue;	  /* only look at printable characters */
	      if (strptr->type == RETURN) tmpl2 = 0;
	      tmpl1 = tmpl2;
	      tlinfo.dostop = j + 1;
	      tmpext = ULength(slab, areawin->topinstance, &tlinfo);
	      tmpl2 = tmpext.maxwidth;
	      if ((slab->anchor & JUSTIFYRIGHT) && tlinfo.padding)
	         padding = (int)tlinfo.padding[tlinfo.line];
	      else if ((slab->anchor & TEXTCENTERED) && tlinfo.padding)
	         padding = (int)(0.5 * tlinfo.padding[tlinfo.line]);
	      else
		 padding = 0;
	      isect = test_insideness(((tmpl1 + tmpl2) >> 1) - adj.x + padding,
			(tmpext.base + (BASELINE >> 1)) - adj.y, newboxpts);

	      /* tiny state machine */
	      if (state == 0) {
		 if (isect) {
		    state = 1;
		    selected = True;
		    areawin->textend = j;
		    if ((areawin->textend > 1) && strptr->type != TEXT_STRING)
		       areawin->textend--;
		 }
	      }
	      else {
		 if (!isect) {
		    areawin->textpos = j;
		    state = 2;
		    break;
		 }
	      }
	   }
	   if (state == 1) areawin->textpos = j;   /* selection goes to end of string */

	   if (tlinfo.padding != NULL) free(tlinfo.padding);

	   /* If a label happens to be empty (can happen in the case of	*/
	   /* a label with parameters that are all empty strings), then	*/
	   /* check if the bounding box surrounds the label position.	*/

	   else if (tmpext.width == 0) {
	      isect = test_insideness(0, 0, newboxpts);
	      if (isect) {
		 selected = True;
		 areawin->textend = 1;
	      }
	   }
	  
	   } break;
	   
	case(PATH):
	   /* check position point of each subpart of the path */

	   selected = False;
	   for (pathgen = TOPATH(curgen)->plist; pathgen < TOPATH(curgen)->plist
		  + TOPATH(curgen)->parts; pathgen++) {
	      if (areaelement(pathgen, boxpts, True, level)) selected = True;
	   }
	   break;

	default:
	   selected = areaelement(curgen, boxpts, False, level);
	   break;
      }

      /* on recursive searches, return as soon as we find something */

      if ((selobj != topobject) && selected) return TRUE;

      /* check if this part has already been selected */

      if (selected)
         for (newselect = areawin->selectlist; newselect <
              areawin->selectlist + areawin->selects; newselect++)
            if (*newselect == (short)(curgen - topobject->plist))
                  selected = False;

      /* add to list of selections */

      if (selected) {
         newselect = allocselect();
         *newselect = (short)(curgen - topobject->plist);
      }
   }
   if (selobj != topobject) return FALSE;
   setoptionmenu();

   /* if none or > 1 label has been selected, cancel any textpos placement */

   if (!checkselect(LABEL) || areawin->selects != 1 ||
	(areawin->selects == 1 && SELECTTYPE(areawin->selectlist) != LABEL)) {
      areawin->textpos = areawin->textend = 0;
   }

   /* Register the selection as an undo event */
   register_for_undo(XCF_Select, UNDO_DONE, areawin->topinstance,
		areawin->selectlist, areawin->selects);

   /* Drawing of selected objects will take place when drawarea() is */
   /* executed after the button release.			     */

#ifdef TCL_WRAPPER
   if (xobjs.suspend < 0)
      XcInternalTagCall(xcinterp, 2, "select", "here");
#endif

   return selected;
}

/*------------------------*/
/* start deselection mode */
/*------------------------*/

void startdesel(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   if (eventmode == NORMAL_MODE) {
      if (areawin->selects == 0)
	 Wprintf("Nothing to deselect!");
      else if (areawin->selects == 1)
	 unselect_all();
   }
}

/*------------------------------------------------------*/
/* Redraw all the selected objects in the select color.	*/
/*------------------------------------------------------*/

void draw_all_selected()
{
   int j;
   
   if (areawin->hierstack != NULL) return;

   for (j = 0; j < areawin->selects; j++)
      gendrawselected(areawin->selectlist + j, topobject, areawin->topinstance);
}

/*---------------------------------------------------------*/
/* Redraw all the selected objects in their normal colors. */
/*---------------------------------------------------------*/

void draw_normal_selected(objectptr thisobj, objinstptr thisinst)
{
   short saveselects;

   if (areawin->selects == 0) return;
   else if (areawin->hierstack != NULL) return;

   saveselects = areawin->selects;

   areawin->selects = 0;
   drawarea(NULL, NULL, NULL);
   areawin->selects = saveselects;
}

/*----------------------------------------------------------------------*/
/* Free a selection linked-list structure				*/
/* (don't confuse with freeselects)					*/
/*----------------------------------------------------------------------*/

static void freeselection(selection *rselect)
{
   selection *nextselect;

   while (rselect != NULL) {
      nextselect = rselect->next;
      free(rselect->selectlist);
      free(rselect);
      rselect = nextselect;
   }
}

/*--------------------------------------------------------------*/
/* Free memory from the previous selection list, copy the	*/
/* current selection list to the previous selection list, and	*/
/* zero out the current selection list.				*/
/* Normally one would use clearselects();  use freeselects()	*/
/* only if the menu/toolbars are going to be updated later in	*/
/* the call.							*/
/*--------------------------------------------------------------*/

void freeselects()
{
   if (areawin->selects > 0) {
      free(areawin->selectlist);
      areawin->redraw_needed =True;
   }
   areawin->selects = 0;
   free_stack(&areawin->hierstack);
}

/*--------------------------------------------------------------*/
/* Free memory from the selection list and set menu/toolbar	*/
/* items back to default values.				*/
/*--------------------------------------------------------------*/

void clearselects_noundo()
{
   if (areawin->selects > 0) {
      reset_cycles();
      freeselects();
      if (xobjs.suspend < 0) {
         setallstylemarks(areawin->style);
         setcolormark(areawin->color);
         setdefaultfontmarks();
	 setparammarks(NULL);
      }

#ifdef TCL_WRAPPER
      if (xobjs.suspend < 0)
         XcInternalTagCall(xcinterp, 2, "unselect", "all");
#endif
   }
}

/*--------------------------------------------------------------*/
/* Same as above, but registers an undo event.			*/
/*--------------------------------------------------------------*/

void clearselects()
{
   if (areawin->selects > 0) {
      register_for_undo(XCF_Select, UNDO_DONE, areawin->topinstance,
		NULL, 0);
      clearselects_noundo();
   }
}

/*--------------------------------------------------------------*/
/* Unselect all the selected elements and free memory from the	*/
/* selection list.						*/
/*--------------------------------------------------------------*/

void unselect_all()
{
   if (xobjs.suspend < 0)
      draw_normal_selected(topobject, areawin->topinstance);
   clearselects();
}

/*----------------------------------------------------------------------*/
/* Select the nearest element, searching the hierarchy if necessary.	*/
/* Return an pushlist pointer to a linked list containing the 		*/
/* hierarchy of objects, with the topmost pushlist also containing a	*/
/* pointer to the polygon found.					*/
/* Allocates memory for the returned linked list which must be freed by	*/
/* the calling routine.							*/
/*----------------------------------------------------------------------*/

selection *recurselect(short class, u_char mode, pushlistptr *seltop)
{
   selection *rselect, *rcheck, *lastselect;
   genericptr rgen;
   short i;
   objectptr selobj;
   objinstptr selinst;
   XPoint savesave, tmppt;
   pushlistptr selnew;
   short j, unselects;
   u_char locmode = (mode == MODE_CONNECT) ? UNDO_DONE : mode;
   u_char recmode = (mode != MODE_CONNECT) ? MODE_RECURSE_WIDE : MODE_RECURSE_NARROW;

   if (*seltop == NULL) {
      Fprintf(stderr, "Error: recurselect called with NULL pushlist pointer\n");
      return NULL;
   }

   selinst = (*seltop)->thisinst;
   selobj = selinst->thisobject;

   class &= areawin->filter;		/* apply the selection filter */

   unselects = 0;
   rselect = genselectelement(class, locmode, selobj, selinst);
   if (rselect == NULL) return NULL;

   for (i = 0; i < rselect->selects; i++) {
      rgen = *(selobj->plist + (*(rselect->selectlist + i)));
      if (rgen->type == OBJINST) {
         selinst = TOOBJINST(selobj->plist + (*(rselect->selectlist + i)));	

         /* Link hierarchy information to the pushlist linked list */
         selnew = (pushlistptr)malloc(sizeof(pushlist));
         selnew->thisinst = selinst;
         selnew->next = NULL;
         (*seltop)->next = selnew;
	 
         /* Translate areawin->save into object's own coordinate system */
         savesave.x = areawin->save.x;
         savesave.y = areawin->save.y;
         InvTransformPoints(&areawin->save, &tmppt, 1, selinst->position,
			selinst->scale, selinst->rotation);
         areawin->save.x = tmppt.x;
         areawin->save.y = tmppt.y;
         /* Fprintf(stdout, "objinst %s found in object %s; searching recursively\n",
			selinst->thisobject->name, selobj->name); */
         /* Fprintf(stdout, "cursor position originally (%d, %d); "
			"in new object is (%d, %d)\n",
			savesave.x, savesave.y,
			areawin->save.x, areawin->save.y); */

         rcheck = recurselect(ALL_TYPES, recmode, &selnew);
         areawin->save.x = savesave.x;
         areawin->save.y = savesave.y;

	 /* If rgen is NULL, remove selected object from the list, and	*/
	 /* remove the last entry from the pushlist stack.		*/

	 if (rcheck == NULL) {
	    *(rselect->selectlist + i) = -1;
	    unselects++;
	    (*seltop)->next = NULL;
	    if (selnew->next != NULL)
	       Fprintf(stderr, "Error: pushstack was freed, but was not empty!\n");
	    free(selnew);
	 }
	 else {
	    for (lastselect = rselect; lastselect->next != NULL; lastselect =
			lastselect->next);
	    lastselect->next = rcheck;
	 }
      }
   }

   /* Modify the selection list */

   for (i = 0, j = 0; i < rselect->selects; i++) {
      if (*(rselect->selectlist + i) >= 0) {
	 if (i != j)
	    *(rselect->selectlist + j) = *(rselect->selectlist + i);
	 j++;
      }
   }
   rselect->selects -= unselects;
   if (rselect->selects == 0) {
      freeselection(rselect);
      rselect = NULL;
   }
   return rselect;
}

/*----------------------------------*/
/* Start drawing a select area box. */
/*----------------------------------*/

void startselect()
{
   eventmode = SELAREA_MODE;
   areawin->origin.x = areawin->save.x;
   areawin->origin.y = areawin->save.y;
   selarea_mode_draw(xcDRAW_INIT, NULL);

#ifdef TCL_WRAPPER
   Tk_CreateEventHandler(areawin->area, ButtonMotionMask |
		PointerMotionMask, (Tk_EventProc *)xctk_drag,
		NULL);
#else
   xcAddEventHandler(areawin->area, ButtonMotionMask |
		PointerMotionMask, False, (xcEventHandler)xlib_drag,
		NULL);
#endif

}

/*-------------------------*/
/* Track a select area box */
/*-------------------------*/

void trackselarea()
{
   XPoint newpos;
   /* u_int  nullui; (jdk) */

   newpos = UGetCursorPos();
   if (newpos.x == areawin->save.x && newpos.y == areawin->save.y) return;

   areawin->save.x = newpos.x;
   areawin->save.y = newpos.y;
   selarea_mode_draw(xcDRAW_EDIT, NULL);
}

/*----------------------*/
/* Track a rescale box	*/
/*----------------------*/

void trackrescale()
{
   XPoint newpos;

   newpos = UGetCursorPos();
   if (newpos.x == areawin->save.x && newpos.y == areawin->save.y) return;

   areawin->save.x = newpos.x;
   areawin->save.y = newpos.y;
   rescale_mode_draw(xcDRAW_EDIT, NULL);
}

/*----------------------------------------------------------------------*/
/* Polygon distance comparison function for qsort			*/
/*----------------------------------------------------------------------*/

int dcompare(const void *a, const void *b)
{
   XPoint cpt;
   genericptr agen, bgen;
   short j, k, adist, bdist;

   cpt.x = areawin->save.x;
   cpt.y = areawin->save.y;

   j = *((short *)a);
   k = *((short *)b);

   agen = *(topobject->plist + j);
   bgen = *(topobject->plist + k);

   if (agen->type != POLYGON || bgen->type != POLYGON) return 0;

   adist = closedistance((polyptr)agen, &cpt);
   bdist = closedistance((polyptr)bgen, &cpt);

   if (adist == bdist) return 0;
   return (adist < bdist) ? 1 : -1;
}

/*----------------------------------------------------------------------*/
/* Compare two selection linked lists					*/
/*----------------------------------------------------------------------*/

Boolean compareselection(selection *sa, selection *sb)
{
   int i, j, match;
   short n1, n2;

   if ((sa == NULL) || (sb == NULL)) return False;
   if (sa->selects != sb->selects) return False;
   match = 0;
   for (i = 0; i < sa->selects; i++) {
      n1 = *(sa->selectlist + i);
      for (j = 0; j < sb->selects; j++) {
         n2 = *(sb->selectlist + j);
	 if (n1 == n2) {
	    match++;
	    break;
	 }
      }
   }
   return (match == sa->selects) ? True : False;
}

/*----------------------------------------------------------------------*/
/* Add pin cycles connected to selected labels				*/
/*----------------------------------------------------------------------*/

void label_connect_cycles(labelptr thislab)
{
   genericptr *pgen;
   Boolean is_selected;
   XPoint *testpt;
   polyptr cpoly;
   short *stest, cycle;

   if (thislab->pin == LOCAL || thislab->pin == GLOBAL) {
      for (pgen = topobject->plist; pgen < topobject->plist +
			topobject->parts; pgen++) {
	 /* Ignore any wires that are already selected */
	 is_selected = FALSE;
	 for (stest = areawin->selectlist; stest < areawin->selectlist +
			areawin->selects; stest++) {
	    if (SELTOGENERIC(stest) == *pgen) {
	       is_selected = TRUE;
	       break;
	    }
	 }
	 if (ELEMENTTYPE(*pgen) == POLYGON) {
	    cpoly = TOPOLY(pgen);
	    if (!is_selected) {
	       cycle = 0;
	       for (testpt = cpoly->points; testpt < cpoly->points +
			cpoly->number; testpt++) {
		  if (testpt->x == thislab->position.x
			&& testpt->y == thislab->position.y) {
		     addcycle(pgen, cycle, 0);
		     break;
		  }
		  else
		     cycle++;
	       }
	    }
	    else {
	       /* Make sure that this polygon's cycle is not set! */
	       removecycle(pgen);
	    }
	 }
      }
   }
}

/*----------------------------------------------------------------------*/
/* Add pin cycles connected to selected instances			*/
/*----------------------------------------------------------------------*/

void inst_connect_cycles(objinstptr thisinst)
{
   genericptr *ggen, *pgen;
   Boolean is_selected;
   XPoint refpoint, *testpt;
   labelptr clab;
   polyptr cpoly;
   short *stest, cycle;
   objectptr thisobj = thisinst->thisobject;

   for (ggen = thisobj->plist; ggen < thisobj->plist + thisobj->parts; ggen++) {
      if (ELEMENTTYPE(*ggen) == LABEL) {
	 clab = TOLABEL(ggen);
	 if (clab->pin == LOCAL || clab->pin == GLOBAL) {
	    ReferencePosition(thisinst, &clab->position, &refpoint);
	    for (pgen = topobject->plist; pgen < topobject->plist +
			topobject->parts; pgen++) {
	       /* Ignore any wires that are already selected */
	       is_selected = FALSE;
	       for (stest = areawin->selectlist; stest < areawin->selectlist +
			areawin->selects; stest++) {
		  if (SELTOGENERIC(stest) == *pgen) {
		     is_selected = TRUE;
		     break;
		  }
	       }
	       if (ELEMENTTYPE(*pgen) == POLYGON) {
		  cpoly = TOPOLY(pgen);
	          if (!is_selected) {
		     cycle = 0;
		     for (testpt = cpoly->points; testpt < cpoly->points +
				cpoly->number; testpt++) {
		        if (testpt->x == refpoint.x && testpt->y == refpoint.y) {
			   addcycle(pgen, cycle, 0);
			   break;
		        }
		        else
		           cycle++;
		     }
		  }
	          else {
		     /* Make sure that this polygon's cycle is not set! */
		     removecycle(pgen);
		  }
	       }
	    }
	 }
      }
   }
}

/*----------------------------------------------------------------------*/
/* Select connected pins on all selected object instances and labels	*/
/*----------------------------------------------------------------------*/

void select_connected_pins()
{
   short *selptr;
   objinstptr selinst;
   labelptr sellab;

   if (!areawin->pinattach) return;

   for (selptr = areawin->selectlist; selptr < areawin->selectlist +
		areawin->selects; selptr++) {
      switch (SELECTTYPE(selptr)) {
	 case LABEL:
            sellab = SELTOLABEL(selptr);
            label_connect_cycles(sellab);
	    break;
	 case OBJINST:
            selinst = SELTOOBJINST(selptr);
            inst_connect_cycles(selinst);
	    break;
      }
   }
}

/*----------------------------------------------------------------------*/
/* Reset all polygon cycles flagged during a move (polygon wires	*/
/* connected to pins of an object instance).				*/
/*----------------------------------------------------------------------*/

void reset_cycles()
{
   polyptr cpoly;
   genericptr *pgen;

   for (pgen = topobject->plist; pgen < topobject->plist +
			topobject->parts; pgen++)
      removecycle(pgen);
}

/*----------------------------------------------------------------------*/
/* Recursive selection mechanism					*/
/*----------------------------------------------------------------------*/

short *recurse_select_element(short class, u_char mode) {
   pushlistptr seltop, nextptr;
   selection *rselect;
   short *newselect, localpick; /* *desel, (jdk) */
   static short pick = 0;
   static selection *saveselect = NULL;
   int i, j, k, ilast, jlast;
   Boolean unselect = False;

   seltop = (pushlistptr)malloc(sizeof(pushlist));
   seltop->thisinst = areawin->topinstance;
   seltop->next = NULL;

   /* Definition for unselecting an element */

   if (class < 0) {
      unselect = True;
      class = -class;
   }
   rselect = recurselect(class, mode, &seltop);

   if (rselect) {
      /* Order polygons according to nearest point distance. */
      qsort((void *)rselect->selectlist, (size_t)rselect->selects,
		sizeof(short), dcompare);

      if (compareselection(rselect, saveselect))
	 pick++;
      else
	 pick = 0;

      localpick = pick % rselect->selects;
   }

   /* Mechanism for unselecting elements under the cursor	*/
   /* (Unselect all picked objects)				*/

   if (rselect && unselect) {

      ilast = -1;
      k = 0;
      for (i = 0; i < rselect->selects; i++) {
	 for (j = 0; j < areawin->selects; j++) {
	    if (*(areawin->selectlist + j) == *(rselect->selectlist + i)) {
	       jlast = j;
	       ilast = i;
	       if (++k == localpick)
	          break;
	    }
	 }
	 if (j < areawin->selects) break;
      }
      if (ilast >= 0) {
	 newselect = rselect->selectlist + ilast;
	 areawin->redraw_needed = True;
         areawin->selects--;
         for (k = jlast; k < areawin->selects; k++)
	    *(areawin->selectlist + k) = *(areawin->selectlist + k + 1);

         if (areawin->selects == 0) freeselects();

         /* Register the selection as an undo event */
         register_for_undo(XCF_Select, mode, areawin->topinstance,
		areawin->selectlist, areawin->selects);
      }
   }

   else if (rselect) {

      /* Mechanism for selecting objects:			*/
      /* Count all elements from rselect that are part of	*/
      /* the current selection.  Pick the "pick"th item (modulo	*/
      /* total number of items).				*/

      ilast = -1;
      k = 0;
      for (i = 0; i < rselect->selects; i++) {
	 for (j = 0; j < areawin->selects; j++) {
	    if (*(areawin->selectlist + j) == *(rselect->selectlist + i))
	       break;
	 }
	 if (j == areawin->selects) {
	    ilast = i;
	    if (++k == localpick)
	       break;
	 }
      }

      if (ilast >= 0) {
         newselect = allocselect();
         *newselect = *(rselect->selectlist + ilast);
	 areawin->redraw_needed = True;
         setoptionmenu();
         u2u_snap(&areawin->save);

         /* Register the selection as an undo event	*/
	 /* (only if selection changed)			*/

         register_for_undo(XCF_Select, mode, areawin->topinstance,
		areawin->selectlist, areawin->selects);
      }
   }

   /* Cleanup */

   while (seltop != NULL) {
      nextptr = seltop->next;
      free(seltop);
      seltop = nextptr;
   }

   freeselection(saveselect);
   saveselect = rselect;

#ifdef TCL_WRAPPER
   if (xobjs.suspend < 0)
      XcInternalTagCall(xcinterp, 2, "select", "here");
#endif

   return areawin->selectlist;
}
