/*-------------------------------------------------------------------------*/
/* functions.c 								   */
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	   */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/*      written by Tim Edwards, 8/13/93    				   */
/*-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#ifndef _MSC_VER
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif

/*-------------------------------------------------------------------------*/
/* Local includes							   */
/*-------------------------------------------------------------------------*/

#ifdef TCL_WRAPPER 
#include <tk.h>
#endif

#include "colordefs.h"
#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*-------------------------------------------------------------------------*/
/* External Variable definitions					   */
/*-------------------------------------------------------------------------*/

extern Display *dpy;
extern Pixmap STIPPLE[8];
extern XCWindowData *areawin;
extern Globaldata xobjs;
extern int number_colors;
extern colorindex *colorlist;

/*------------------------------------------------------------------------*/
/* find the squared length of a wire (or distance between two points in   */
/* user space).								  */
/*------------------------------------------------------------------------*/

long sqwirelen(XPoint *userpt1, XPoint *userpt2)
{
  long xdist, ydist;

  xdist = (long)userpt2->x - (long)userpt1->x;
  ydist = (long)userpt2->y - (long)userpt1->y;
  return (xdist * xdist + ydist * ydist);
}

/*------------------------------------------------------------------------*/
/* floating-point version of the above					  */
/*------------------------------------------------------------------------*/

float fsqwirelen(XfPoint *userpt1, XfPoint *userpt2)
{
  float xdist, ydist;

  xdist = userpt2->x - userpt1->x;
  ydist = userpt2->y - userpt1->y;
  return (xdist * xdist + ydist * ydist);
}

/*------------------------------------------------------------------------*/
/* Find absolute distance between two points in user space		  */
/*------------------------------------------------------------------------*/

int wirelength(XPoint *userpt1, XPoint *userpt2)
{
  u_long xdist, ydist;

  xdist = (long)(userpt2->x) - (long)(userpt1->x);
  ydist = (long)(userpt2->y) - (long)(userpt1->y);
  return (int)sqrt((double)(xdist * xdist + ydist * ydist));
}

/*------------------------------------------------------------------------*/
/* Find the closest (squared) distance from a point to a line		  */
/*------------------------------------------------------------------------*/

long finddist(XPoint *linept1, XPoint *linept2, XPoint *userpt)
{
   long a, b, c, frac;
   float protod;

   c = sqwirelen(linept1, linept2);
   a = sqwirelen(linept1, userpt);
   b = sqwirelen(linept2, userpt);
   frac = a - b;
   if (frac >= c) return b;	  /* "=" is important if c = 0 ! */
   else if (-frac >= c) return a;
   else {
      protod = (float)(c + a - b);
      return (a - (long)((protod * protod) / (float)(c << 2)));
   }
}

/*----------------------------------------------------------------------*/
/* Decompose an arc segment into one to four bezier curves according	*/
/* the approximation algorithm lifted from the paper by  L. Maisonobe	*/
/* (spaceroots.org).  This decomposition is done when an arc in a path	*/
/* is read from an (older) xcircuit file, or when an arc is a selected	*/
/* item when a path is created.	 Because arcs are decomposed when	*/
/* encountered, we assume that the arc is the last element of the path.	*/
/*----------------------------------------------------------------------*/

void decomposearc(pathptr thepath)
{
   float fnc, ang1, ang2;
   short ncurves, i;
   arcptr thearc;
   genericptr *pgen;
   splineptr *newspline;
   double nu1, nu2, lambda1, lambda2, alpha, tansq;
   XfPoint E1, E2, Ep1, Ep2;
   Boolean reverse = FALSE;

   pgen = thepath->plist + thepath->parts - 1;
   if (ELEMENTTYPE(*pgen) != ARC) return;
   thearc = TOARC(pgen);

   if (thearc->radius < 0) {
      reverse = TRUE;
      thearc->radius = -thearc->radius;
   }

   fnc = (thearc->angle2 - thearc->angle1) / 90.0;
   ncurves = (short)fnc;
   if (fnc - (float)((int)fnc) > 0.01) ncurves++;

   thepath->parts--;  /* Forget the arc */

   for (i = 0; i < ncurves; i++) {
      if (reverse) {			/* arc path is reverse direction */
         if (i == 0)
	    ang1 = thearc->angle2;
         else
	    ang1 -= 90;

         if (i == ncurves - 1)
	    ang2 = thearc->angle1;
         else
	    ang2 = ang1 - 90;
      }
      else {				/* arc path is forward direction */
         if (i == 0)
	    ang1 = thearc->angle1;
         else
	    ang1 += 90;

         if (i == ncurves - 1)
	    ang2 = thearc->angle2;
         else
	    ang2 = ang1 + 90;
      }

      lambda1 = (double)ang1 * RADFAC;
      lambda2 = (double)ang2 * RADFAC;

      nu1 = atan2(sin(lambda1) / (double)thearc->yaxis,
		cos(lambda1) / (double)thearc->radius);
      nu2 = atan2(sin(lambda2) / (double)thearc->yaxis,
		cos(lambda2) / (double)thearc->radius);
      E1.x = (float)thearc->position.x + 
		(float)thearc->radius * (float)cos(nu1);
      E1.y = (float)thearc->position.y +
		(float)thearc->yaxis * (float)sin(nu1);
      E2.x = (float)thearc->position.x + 
		(float)thearc->radius * (float)cos(nu2);
      E2.y = (float)thearc->position.y +
		(float)thearc->yaxis * (float)sin(nu2);
      Ep1.x = -(float)thearc->radius * (float)sin(nu1);
      Ep1.y = (float)thearc->yaxis * (float)cos(nu1);
      Ep2.x = -(float)thearc->radius * (float)sin(nu2);
      Ep2.y = (float)thearc->yaxis * (float)cos(nu2);

      tansq = tan((nu2 - nu1) / 2.0);
      tansq *= tansq;
      alpha = sin(nu2 - nu1) * 0.33333 * (sqrt(4 + (3 * tansq)) - 1);

      NEW_SPLINE(newspline, thepath);
      splinedefaults(*newspline, 0, 0);
      (*newspline)->style = thearc->style;
      (*newspline)->color = thearc->color;
      (*newspline)->width = thearc->width;

      (*newspline)->ctrl[0].x = E1.x;
      (*newspline)->ctrl[0].y = E1.y;

      (*newspline)->ctrl[1].x = E1.x + alpha * Ep1.x;
      (*newspline)->ctrl[1].y = E1.y + alpha * Ep1.y;

      (*newspline)->ctrl[2].x = E2.x - alpha * Ep2.x;
      (*newspline)->ctrl[2].y = E2.y - alpha * Ep2.y;

      (*newspline)->ctrl[3].x = E2.x;
      (*newspline)->ctrl[3].y = E2.y;

      calcspline(*newspline);
   }

   /* Delete the arc */
   free_single((genericptr)thearc);
}

/*----------------------------------------------------------------------*/
/* Calculate points for an arc						*/
/*----------------------------------------------------------------------*/

void calcarc(arcptr thearc)
{
   short idx;
   int sarc;
   float theta, delta;

   /* assume that angle2 > angle1 always: must be guaranteed by other routines */

   sarc = (int)(thearc->angle2 - thearc->angle1) * RSTEPS;
   thearc->number = (sarc / 360) + 1;
   if (sarc % 360 != 0) thearc->number++;
	   
   delta = RADFAC * ((float)(thearc->angle2 - thearc->angle1) / (thearc->number - 1));
   theta = thearc->angle1 * RADFAC;

   for (idx = 0; idx < thearc->number - 1; idx++) {
      thearc->points[idx].x = (float)thearc->position.x + 
	   fabs((float)thearc->radius) * cos(theta);
      thearc->points[idx].y = (float)thearc->position.y +
	   (float)thearc->yaxis * sin(theta);
      theta += delta;
   }

   /* place last point exactly to avoid roundoff error */

   theta = thearc->angle2 * RADFAC;
   thearc->points[thearc->number - 1].x = (float)thearc->position.x + 
	   fabs((float)thearc->radius) * cos(theta);
   thearc->points[thearc->number - 1].y = (float)thearc->position.y +
	   (float)thearc->yaxis * sin(theta);

   if (thearc->radius < 0) reversefpoints(thearc->points, thearc->number);
}

/*------------------------------------------------------------------------*/
/* Create a Bezier curve approximation from control points		  */
/* (using PostScript formula for Bezier cubic curve)			  */
/*------------------------------------------------------------------------*/

float par[INTSEGS];
float parsq[INTSEGS];
float parcb[INTSEGS];

void initsplines()
{
   float t;
   short idx;

   for (idx = 0; idx < INTSEGS; idx++) {
      t = (float)(idx + 1) / (INTSEGS + 1);
      par[idx] = t;
      parsq[idx] = t * t;
      parcb[idx] = parsq[idx] * t;
   }
}

/*------------------------------------------------------------------------*/
/* Compute spline coefficients						  */
/*------------------------------------------------------------------------*/

void computecoeffs(splineptr thespline, float *ax, float *bx, float *cx,
	float *ay, float *by, float *cy)
{
   *cx = 3.0 * (float)(thespline->ctrl[1].x - thespline->ctrl[0].x);
   *bx = 3.0 * (float)(thespline->ctrl[2].x - thespline->ctrl[1].x) - *cx;
   *ax = (float)(thespline->ctrl[3].x - thespline->ctrl[0].x) - *cx - *bx;

   *cy = 3.0 * (float)(thespline->ctrl[1].y - thespline->ctrl[0].y);
   *by = 3.0 * (float)(thespline->ctrl[2].y - thespline->ctrl[1].y) - *cy;
   *ay = (float)(thespline->ctrl[3].y - thespline->ctrl[0].y) - *cy - *by;   
}

/*------------------------------------------------------------------------*/

void calcspline(splineptr thespline)
{
   float ax, bx, cx, ay, by, cy;
   short idx;

   computecoeffs(thespline, &ax, &bx, &cx, &ay, &by, &cy);
   for (idx = 0; idx < INTSEGS; idx++) {
      thespline->points[idx].x = ax * parcb[idx] + bx * parsq[idx] +
	 cx * par[idx] + (float)thespline->ctrl[0].x;
      thespline->points[idx].y = ay * parcb[idx] + by * parsq[idx] +
	 cy * par[idx] + (float)thespline->ctrl[0].y;
   }
}

/*------------------------------------------------------------------------*/
/* Find the (x,y) position and tangent rotation of a point on a spline    */
/*------------------------------------------------------------------------*/

void findsplinepos(splineptr thespline, float t, XPoint *retpoint, float *retrot)
{
   float ax, bx, cx, ay, by, cy;
   float tsq = t * t;
   float tcb = tsq * t;
   double dxdt, dydt;

   computecoeffs(thespline, &ax, &bx, &cx, &ay, &by, &cy);
   retpoint->x = (short)(ax * tcb + bx * tsq + cx * t + (float)thespline->ctrl[0].x);
   retpoint->y = (short)(ay * tcb + by * tsq + cy * t + (float)thespline->ctrl[0].y);

   if (retrot != NULL) {
      dxdt = (double)(3 * ax * tsq + 2 * bx * t + cx);
      dydt = (double)(3 * ay * tsq + 2 * by * t + cy);
      *retrot = INVRFAC * atan2(dxdt, dydt);  /* reversed y, x */
      if (*retrot < 0) *retrot += 360;
   }
}

/*------------------------------------------------------------------------*/
/* floating-point version of the above					  */
/*------------------------------------------------------------------------*/

void ffindsplinepos(splineptr thespline, float t, XfPoint *retpoint)
{
   float ax, bx, cx, ay, by, cy;
   float tsq = t * t;
   float tcb = tsq * t;

   computecoeffs(thespline, &ax, &bx, &cx, &ay, &by, &cy);
   retpoint->x = ax * tcb + bx * tsq + cx * t + (float)thespline->ctrl[0].x;
   retpoint->y = ay * tcb + by * tsq + cy * t + (float)thespline->ctrl[0].y;
}

/*------------------------------------------------------------------------*/
/* Find the closest distance between a point and a spline and return the  */
/* fractional distance along the spline of this point.			  */
/*------------------------------------------------------------------------*/

float findsplinemin(splineptr thespline, XPoint *upoint)
{
   XfPoint 	*spt, flpt, newspt;
   float	minval = 1000000, tval, hval, ndist;
   short	j, ival;

   flpt.x = (float)(upoint->x);
   flpt.y = (float)(upoint->y);

   /* get estimate from precalculated spline points */

   for (spt = thespline->points; spt < thespline->points + INTSEGS;
	spt++) {
      ndist = fsqwirelen(spt, &flpt);
      if (ndist < minval) {
	 minval = ndist;
	 ival = (short)(spt - thespline->points);
      }
   }
   tval = (float)(ival + 1) / (INTSEGS + 1);
   hval = 0.5 / (INTSEGS + 1);

   /* short fixed iterative loop to converge on minimum t */

   for (j = 0; j < 5; j++) {
      tval += hval;
      ffindsplinepos(thespline, tval, &newspt);
      ndist = fsqwirelen(&newspt, &flpt);
      if (ndist < minval) minval = ndist;
      else {
         tval -= hval * 2;
         ffindsplinepos(thespline, tval, &newspt);
         ndist = fsqwirelen(&newspt, &flpt);
         if (ndist < minval) minval = ndist;
	 else tval += hval;
      }
      hval /= 2;
   }

   if (tval < 0.1) {
      if ((float)sqwirelen(&(thespline->ctrl[0]), upoint) < minval) tval = 0;
   }
   else if (tval > 0.9) {
      if ((float)sqwirelen(&(thespline->ctrl[3]), upoint) < minval) tval = 1;
   }
   return tval;
}

/*----------------------------------------------------------------------*/
/* Convert a polygon to a Bezier curve path				*/
/* Curve must be selected and there must be only one selection.		*/
/*									*/
/* Note that this routine will draw inside the perimeter of a convex	*/
/* hull.  A routine that places spline endpoints on the polygon		*/
/* vertices will draw outside the perimeter of a convex hull.  An	*/
/* optimal algorithm presumably zeros the total area between the curve	*/
/* and the polygon (positive and negative), but I haven't worked out	*/
/* what that solution is.  The algorithm below seems good enough for	*/
/* most purposes.							*/
/*----------------------------------------------------------------------*/

void converttocurve()
{
   genericptr *ggen;
   splineptr *newspline;
   polyptr thispoly;
   pathptr *newpath;
   short *newselect;
   XPoint firstpoint, lastpoint, initpoint;
   int i, numpoints;

   if (areawin->selects != 1) return;

   thispoly = TOPOLY(topobject->plist + (*areawin->selectlist));
   if (ELEMENTTYPE(thispoly) != POLYGON) return;
   if (thispoly->number < 3) return;	/* Will not convert */

   standard_element_delete(ERASE);
   if ((thispoly->style & UNCLOSED) && (thispoly->number == 3)) {
      NEW_SPLINE(newspline, topobject);
      splinedefaults(*newspline, 0, 0);
      (*newspline)->ctrl[0] = thispoly->points[0];
      (*newspline)->ctrl[1] = thispoly->points[1];
      (*newspline)->ctrl[2] = thispoly->points[1];
      (*newspline)->ctrl[3] = thispoly->points[2];
   }
   else {
      numpoints = thispoly->number;

      /* If the polygon is closed but the first and last points	*/
      /* overlap, treat the last point as if it doesn't exist.	*/

      if (!(thispoly->style & UNCLOSED))
         if ((thispoly->points[0].x == thispoly->points[thispoly->number - 1].x)
		&& (thispoly->points[0].y ==
		thispoly->points[thispoly->number - 1].y))
	    numpoints--;

      NEW_PATH(newpath, topobject);
      pathdefaults(*newpath, 0, 0);
      (*newpath)->style = thispoly->style;

      if (!(thispoly->style & UNCLOSED)) {
	 lastpoint = thispoly->points[numpoints - 1];
	 initpoint.x = (lastpoint.x + thispoly->points[0].x) / 2;
	 initpoint.y = (lastpoint.y + thispoly->points[0].y) / 2;
	 firstpoint.x = (thispoly->points[0].x
		+ thispoly->points[1].x) / 2;
	 firstpoint.y = (thispoly->points[0].y
			+ thispoly->points[1].y) / 2;

         NEW_SPLINE(newspline, (*newpath));
	 splinedefaults(*newspline, 0, 0);
	 (*newspline)->ctrl[0] = initpoint;
	 (*newspline)->ctrl[1] = thispoly->points[0];
	 (*newspline)->ctrl[2] = thispoly->points[0];
	 (*newspline)->ctrl[3] = firstpoint;
         calcspline(*newspline);
      }
      else
         firstpoint = thispoly->points[0];

      for (i = 0; i < numpoints - ((!(thispoly->style & UNCLOSED)) ?
		2 : 3); i++) {
	 lastpoint.x = (thispoly->points[i + 1].x
		+ thispoly->points[i + 2].x) / 2;
	 lastpoint.y = (thispoly->points[i + 1].y
			+ thispoly->points[i + 2].y) / 2;

         NEW_SPLINE(newspline, (*newpath));
	 splinedefaults(*newspline, 0, 0);
	 (*newspline)->ctrl[0] = firstpoint;
	 (*newspline)->ctrl[1] = thispoly->points[i + 1];
	 (*newspline)->ctrl[2] = thispoly->points[i + 1];
	 (*newspline)->ctrl[3] = lastpoint;
	 firstpoint = lastpoint;
         calcspline(*newspline);
      }
      if (!(thispoly->style & UNCLOSED))
	 lastpoint = initpoint;
      else
	 lastpoint = thispoly->points[i + 2];

      NEW_SPLINE(newspline, (*newpath));
      splinedefaults(*newspline, 0, 0);
      (*newspline)->ctrl[0] = firstpoint;
      (*newspline)->ctrl[1] = thispoly->points[i + 1];
      (*newspline)->ctrl[2] = thispoly->points[i + 1];
      (*newspline)->ctrl[3] = lastpoint;
   }
   calcspline(*newspline);
   calcbbox(areawin->topinstance);
   setoptionmenu();
   drawarea(NULL, NULL, NULL);
}

/*----------------------------------------------------------------------*/
/* Find closest point of a polygon to the cursor			*/
/*----------------------------------------------------------------------*/

short closepointdistance(polyptr curpoly, XPoint *cursloc, short *mindist)
{
   short curdist;
   XPoint *curpt, *savept; 

   curpt = savept = curpoly->points;
   *mindist = wirelength(curpt, cursloc);
   while (++curpt < curpoly->points + curpoly->number) {
      curdist = wirelength(curpt, cursloc);
      if (curdist < *mindist) {
         *mindist = curdist;
         savept = curpt;
      }
   }
   return (short)(savept - curpoly->points);
}

/*----------------------------------------------------------------------------*/
/* Find closest point of a polygon to the cursor			      */
/*----------------------------------------------------------------------------*/

short closepoint(polyptr curpoly, XPoint *cursloc)
{
   short mindist;
   return closepointdistance(curpoly, cursloc, &mindist);
}

/*----------------------------------------------------------------------------*/
/* Find the distance to the closest point of a polygon to the cursor	      */
/*----------------------------------------------------------------------------*/

short closedistance(polyptr curpoly, XPoint *cursloc)
{
   short mindist;
   closepointdistance(curpoly, cursloc, &mindist);
   return mindist;
}

/*----------------------------------------------------------------------------*/
/* Coordinate system transformations 					      */
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*/
/*  Check screen bounds:  minimum, maximum scale and translation is determined	*/
/*  by values which fit in an X11 type XPoint (short int).  If the window	*/
/*  extremes exceed type short when mapped to user space, or if the page 	*/
/*  bounds exceed type short when mapped to X11 window space, return error.	*/ 
/*------------------------------------------------------------------------------*/

short checkbounds()
{
   long lval;

   /* check window-to-user space */

   lval = 2 * (long)((float) (areawin->width) / areawin->vscale) +
	(long)areawin->pcorner.x;
   if (lval != (long)((short)lval)) return -1;
   lval = 2 * (long)((float) (areawin->height) / areawin->vscale) +
	(long)areawin->pcorner.y;
   if (lval != (long)((short)lval)) return -1;

   /* check user-to-window space */

   lval = (long)((float)(topobject->bbox.lowerleft.x - areawin->pcorner.x) *
	areawin->vscale);
   if (lval != (long)((short)lval)) return -1;
   lval = (long)areawin->height - (long)((float)(topobject->bbox.lowerleft.y -
	areawin->pcorner.y) * areawin->vscale); 
   if (lval != (long)((short)lval)) return -1;

   lval = (long)((float)(topobject->bbox.lowerleft.x + topobject->bbox.width -
	areawin->pcorner.x) * areawin->vscale);
   if (lval != (long)((short)lval)) return -1;
   lval = (long)areawin->height - (long)((float)(topobject->bbox.lowerleft.y +
	topobject->bbox.height - areawin->pcorner.y) * areawin->vscale); 
   if (lval != (long)((short)lval)) return -1;

   return 0;
}

/*------------------------------------------------------------------------*/
/* Transform X-window coordinate to xcircuit coordinate system		  */
/*------------------------------------------------------------------------*/

void window_to_user(short xw, short yw, XPoint *upt)
{
  float tmpx, tmpy;

  tmpx = (float)xw / areawin->vscale + (float)areawin->pcorner.x;
  tmpy = (float)(areawin->height - yw) / areawin->vscale + 
	(float)areawin->pcorner.y;

  tmpx += (tmpx > 0) ? 0.5 : -0.5;
  tmpy += (tmpy > 0) ? 0.5 : -0.5;

  upt->x = (short)tmpx;
  upt->y = (short)tmpy;
}

/*------------------------------------------------------------------------*/
/* Transform xcircuit coordinate back to X-window coordinate system       */
/*------------------------------------------------------------------------*/

void user_to_window(XPoint upt, XPoint *wpt)
{
  float tmpx, tmpy;

  tmpx = (float)(upt.x - areawin->pcorner.x) * areawin->vscale;
  tmpy = (float)areawin->height - (float)(upt.y - areawin->pcorner.y)
	* areawin->vscale; 

  tmpx += (tmpx > 0) ? 0.5 : -0.5;
  tmpy += (tmpy > 0) ? 0.5 : -0.5;

  wpt->x = (short)tmpx;
  wpt->y = (short)tmpy;
}

/*----------------------------------------------------------------------*/
/* Transformations in the object hierarchy				*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Return rotation relative to a specific CTM				*/
/*----------------------------------------------------------------------*/

float UGetCTMRotation(Matrix *ctm)
{
   float rads = (float)atan2((double)(ctm->d), (double)(ctm->a));
   return rads / RADFAC;
}

/*----------------------------------------------------------------------*/
/* Return rotation relative to the top level				*/
/* Note that UTopRotation() is also the rotation relative to the window	*/
/* since the top-level drawing page is always upright relative to the	*/
/* window.  Thus, there is no routine UTopDrawingRotation().		*/
/*----------------------------------------------------------------------*/

float UTopRotation()
{
   return UGetCTMRotation(DCTM);
}

/*----------------------------------------------------------------------*/
/* Return scale relative to a specific CTM				*/
/*----------------------------------------------------------------------*/

float UGetCTMScale(Matrix *ctm)
{
   return (float)(sqrt((double)(ctm->a * ctm->a + ctm->d * ctm->d)));
}

/*----------------------------------------------------------------------*/
/* Return scale relative to window					*/
/*----------------------------------------------------------------------*/

float UTopScale()
{
   return UGetCTMScale(DCTM);
}

/*----------------------------------------------------------------------*/
/* Return scale multiplied by length					*/
/*----------------------------------------------------------------------*/

float UTopTransScale(float length)
{
   return (float)(length * UTopScale());
}

/*----------------------------------------------------------------------*/
/* Return scale relative to the top-level schematic (not the window)	*/
/*----------------------------------------------------------------------*/

float UTopDrawingScale()
{
   Matrix lctm, wctm;
   UCopyCTM(DCTM, &lctm);
   UResetCTM(&wctm);
   UMakeWCTM(&wctm);
   InvertCTM(&wctm);
   UPreMultCTMbyMat(&wctm, &lctm);
   return UGetCTMScale(&wctm);
}

/*----------------------------------------------------------------------*/
/* Return position offset relative to a specific CTM			*/
/*----------------------------------------------------------------------*/

void UGetCTMOffset(Matrix *ctm, int *offx, int *offy)
{
   if (offx) *offx = (int)ctm->c;
   if (offy) *offy = (int)ctm->f;
}

/*----------------------------------------------------------------------*/
/* Return position offset relative to top-level				*/
/*----------------------------------------------------------------------*/

void UTopOffset(int *offx, int *offy)
{
   UGetCTMOffset(DCTM, offx, offy);
}

/*----------------------------------------------------------------------*/
/* Return postion relative to the top-level schematic (not the window)	*/
/*----------------------------------------------------------------------*/

void UTopDrawingOffset(int *offx, int *offy)
{
   Matrix lctm, wctm;
   UCopyCTM(DCTM, &lctm);
   UResetCTM(&wctm);
   UMakeWCTM(&wctm);
   InvertCTM(&wctm);
   UPreMultCTMbyMat(&wctm, &lctm);
   UGetCTMOffset(&wctm, offx, offy);
}

/*----------------------------------------------------------------------*/
/* Get the cursor position						*/
/*----------------------------------------------------------------------*/

XPoint UGetCursor()
{
   Window nullwin;
   int    nullint, xpos, ypos;
   u_int   nullui;
   XPoint newpos;
 
#ifdef TCL_WRAPPER
   /* Don't use areawin->window;  if called from inside an object	*/
   /* (e.g., "here" in a Tcl expression), areawin->window will be	*/
   /* an off-screen pixmap, and cause a crash.				*/
#ifndef _MSC_VER
   XQueryPointer(dpy, Tk_WindowId(areawin->area), &nullwin, &nullwin,
	&nullint, &nullint, &xpos, &ypos, &nullui);
#else
   XQueryPointer_TkW32(dpy, Tk_WindowId(areawin->area), &nullwin, &nullwin,
	&nullint, &nullint, &xpos, &ypos, &nullui);
#endif
#else
   XQueryPointer(dpy, areawin->window, &nullwin, &nullwin, &nullint,
	&nullint, &xpos, &ypos, &nullui);
#endif

   newpos.x = xpos;
   newpos.y = ypos;

   return newpos;
}

/*----------------------------------------------------------------------*/
/* Get the cursor position and translate to user coordinates		*/
/*----------------------------------------------------------------------*/

XPoint UGetCursorPos()
{
   XPoint winpos, userpos;
 
   winpos = UGetCursor();

   window_to_user(winpos.x, winpos.y, &userpos);  

   return userpos;
}

/*----------------------------------------------------------------------*/
/* Translate a point to the nearest snap-to grid point			*/
/*----------------------------------------------------------------------*/
/* user coordinates to user coordinates version 			*/

void u2u_snap(XPoint *uvalue)
{
   float tmpx, tmpy;
   float tmpix, tmpiy;

   if (areawin->snapto) {
      tmpx = (float)uvalue->x / xobjs.pagelist[areawin->page]->snapspace;
      if (tmpx > 0)
	 tmpix = (float)((int)(tmpx + 0.5));
      else
         tmpix = (float)((int)(tmpx - 0.5));

      tmpy = (float)uvalue->y / xobjs.pagelist[areawin->page]->snapspace;
      if (tmpy > 0)
         tmpiy = (float)((int)(tmpy + 0.5));
      else
         tmpiy = (float)((int)(tmpy - 0.5));

      tmpix *= xobjs.pagelist[areawin->page]->snapspace;
      tmpix += (tmpix > 0) ? 0.5 : -0.5;
      tmpiy *= xobjs.pagelist[areawin->page]->snapspace;
      tmpiy += (tmpiy > 0) ? 0.5 : -0.5;

      uvalue->x = (int)tmpix;
      uvalue->y = (int)tmpiy;
   }
}

/*------------------------------------------------------------------------*/
/* window coordinates to user coordinates version 			  */
/*------------------------------------------------------------------------*/

void snap(short valuex, short valuey, XPoint *returnpt)
{
   window_to_user(valuex, valuey, returnpt);  
   u2u_snap(returnpt);
}

/*------------------------------------------------------------------------*/
/* Transform object coordinates through scale, translation, and rotation  */
/* This routine attempts to match the PostScript definition of trans-     */
/*    formation matrices.						  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Current transformation matrix manipulation routines			  */
/*------------------------------------------------------------------------*/

void UResetCTM(Matrix *ctm)
{
   ctm->a = ctm->e = 1;
   ctm->b = ctm->d = 0;
   ctm->c = ctm->f = 0;  /* 0.5 for nearest-int real->int conversion? */

#ifdef HAVE_CAIRO
   if (ctm == DCTM && areawin->redraw_ongoing)
      xc_cairo_set_matrix(ctm);
#endif /* HAVE_CAIRO */
}

/*------------------------------------------------------------------------*/

void InvertCTM(Matrix *ctm)
{
   float det = ctm->a * ctm->e - ctm->b * ctm->d;
   float tx = ctm->b * ctm->f - ctm->c * ctm->e;
   float ty = ctm->d * ctm->c - ctm->a * ctm->f;

   float tmpa = ctm->a;

   ctm->b = -ctm->b / det;
   ctm->d = -ctm->d / det;

   ctm->a = ctm->e / det;
   ctm->e = tmpa / det;
   ctm->c = tx / det;
   ctm->f = ty / det;

#ifdef HAVE_CAIRO
   if (ctm == DCTM && areawin->redraw_ongoing)
      xc_cairo_set_matrix(ctm);
#endif /* HAVE_CAIRO */
}

/*------------------------------------------------------------------------*/

void UCopyCTM(fctm, tctm)
   Matrix *fctm, *tctm;
{
   tctm->a = fctm->a;
   tctm->b = fctm->b;
   tctm->c = fctm->c;
   tctm->d = fctm->d;
   tctm->e = fctm->e;
   tctm->f = fctm->f;

#ifdef HAVE_CAIRO
   if (tctm == DCTM && areawin->redraw_ongoing)
      xc_cairo_set_matrix(tctm);
#endif /* HAVE_CAIRO */
}

/*-------------------------------------------------------------------------*/
/* Multiply CTM by current screen position and scale to get transformation */
/* matrix from a user point to the X11 window				   */
/*-------------------------------------------------------------------------*/

void UMakeWCTM(Matrix *ctm)
{
   ctm->a *= areawin->vscale;
   ctm->b *= areawin->vscale;
   ctm->c = (ctm->c - (float)areawin->pcorner.x) * areawin->vscale
	 + areawin->panx;
   
   ctm->d *= -areawin->vscale;
   ctm->e *= -areawin->vscale;
   ctm->f = (float)areawin->height + ((float)areawin->pcorner.y - ctm->f) *
	 areawin->vscale + areawin->pany;

#ifdef HAVE_CAIRO
   if (ctm == DCTM && areawin->redraw_ongoing)
      xc_cairo_set_matrix(ctm);
#endif /* HAVE_CAIRO */
}

/*------------------------------------------------------------------------*/

void UMultCTM(Matrix *ctm, XPoint position, float scale, float rotate)
{
   float tmpa, tmpb, tmpd, tmpe, yscale;
   float mata, matb, matc;
   double drot = (double)rotate * RADFAC;

   yscale = abs(scale);  /* -scale implies flip in x direction only */ 
   
   tmpa =  scale * cos(drot);
   tmpb = yscale * sin(drot);
   tmpd = -scale * sin(drot);
   tmpe = yscale * cos(drot);

   mata = ctm->a * tmpa + ctm->d * tmpb;
   matb = ctm->b * tmpa + ctm->e * tmpb;
   matc = ctm->c * tmpa + ctm->f * tmpb + position.x;

   ctm->d = ctm->d * tmpe + ctm->a * tmpd;
   ctm->e = ctm->e * tmpe + ctm->b * tmpd;
   ctm->f = ctm->f * tmpe + ctm->c * tmpd + position.y; 

   ctm->a = mata;
   ctm->b = matb;
   ctm->c = matc;

#ifdef HAVE_CAIRO
   if (ctm == DCTM && areawin->redraw_ongoing)
      xc_cairo_set_matrix(ctm);
#endif /* HAVE_CAIRO */
}

/*----------------------------------------------------------------------*/
/* Slanting function x' = x + beta * y, y' = y				*/
/*----------------------------------------------------------------------*/

void USlantCTM(Matrix *ctm, float beta)
{
   ctm->b += ctm->a * beta;
   ctm->e += ctm->d * beta;

#ifdef HAVE_CAIRO
   if (ctm == DCTM && areawin->redraw_ongoing)
      xc_cairo_set_matrix(ctm);
#endif /* HAVE_CAIRO */
}

#define EPS 1e-9
/*----------------------------------------------------------------------*/
/* Transform text to make it right-side up within 90 degrees of page	*/
/* NOTE:  This is not yet resolved, as xcircuit does not agree with	*/
/* PostScript in a few cases!						*/
/*----------------------------------------------------------------------*/

void UPreScaleCTM(Matrix *ctm)
{
   /* negative X scale (-1, +1) */
   if ((ctm->a < -EPS) || ((ctm->a < EPS) && (ctm->a > -EPS) &&
		((ctm->d * ctm->b) < 0))) {
      ctm->a = -ctm->a;
      ctm->d = -ctm->d;
   }

   /* negative Y scale (+1, -1) */
   if (ctm->e > EPS) {
      ctm->e = -ctm->e;
      ctm->b = -ctm->b;
   }

   /* At 90, 270 degrees need special attention to avoid discrepencies	*/
   /* with the PostScript output due to roundoff error.  This code	*/
   /* matches what PostScript produces.					*/

#ifdef HAVE_CAIRO
   if (ctm == DCTM && areawin->redraw_ongoing)
      xc_cairo_set_matrix(ctm);
#endif /* HAVE_CAIRO */
}

/*----------------------------------------------------------------------*/
/* Adjust anchoring and CTM as necessary for flip invariance		*/
/*----------------------------------------------------------------------*/

short flipadjust(short anchor)
{
   short tmpanchor = anchor & (~FLIPINV);

   if (anchor & FLIPINV) {
      if (((DCTM)->a < -EPS) || (((DCTM)->a < EPS) && ((DCTM)->a > -EPS) &&
		(((DCTM)->d * (DCTM)->b) < 0))) {
         if ((tmpanchor & (RIGHT | NOTLEFT)) != NOTLEFT)
            tmpanchor ^= (RIGHT | NOTLEFT);

	 /* NOTE:  Justification does not change under flip invariance.	*/
      }   
      if ((DCTM)->e > EPS) {
         if ((tmpanchor & (TOP | NOTBOTTOM)) != NOTBOTTOM) 
            tmpanchor ^= (TOP | NOTBOTTOM);
      }
      UPreScaleCTM(DCTM);
   }      
   return tmpanchor;
}

/*------------------------------------------------------------------------*/

void UPreMultCTM(Matrix *ctm, XPoint position, float scale, float rotate)
{
   float tmpa, tmpb, tmpd, tmpe, yscale;
   float mata, matd;
   double drot = (double)rotate * RADFAC;

   yscale = abs(scale);		/* negative scale value implies flip in x only */
   
   tmpa =  scale * cos(drot);
   tmpb = yscale * sin(drot);
   tmpd = -scale * sin(drot);
   tmpe = yscale * cos(drot);
   
   ctm->c += ctm->a * position.x + ctm->b * position.y;
   ctm->f += ctm->d * position.x + ctm->e * position.y;

   mata = ctm->a * tmpa + ctm->b * tmpd;
   ctm->b = ctm->a * tmpb + ctm->b * tmpe;

   matd = ctm->d * tmpa + ctm->e * tmpd;
   ctm->e = ctm->d * tmpb + ctm->e * tmpe;

   ctm->a = mata;
   ctm->d = matd;

#ifdef HAVE_CAIRO
   if (ctm == DCTM && areawin->redraw_ongoing)
      xc_cairo_set_matrix(ctm);
#endif /* HAVE_CAIRO */
}

/*----------------------------------------------------------------------*/
/* Direct Matrix-Matrix multiplication					*/
/*----------------------------------------------------------------------*/

void UPreMultCTMbyMat(Matrix *ctm, Matrix *pre)
{
   float mata, matd;

   mata = pre->a * ctm->a + pre->d * ctm->b;
   ctm->c += pre->c * ctm->a + pre->f * ctm->b;
   ctm->b = pre->b * ctm->a + pre->e * ctm->b;
   ctm->a = mata;

   matd = pre->a * ctm->d + pre->d * ctm->e;
   ctm->f += pre->c * ctm->d + pre->f * ctm->e;
   ctm->e = pre->b * ctm->d + pre->e * ctm->e;
   ctm->d = matd;

#ifdef HAVE_CAIRO
   if (ctm == DCTM && areawin->redraw_ongoing)
      xc_cairo_set_matrix(ctm);
#endif /* HAVE_CAIRO */
}

/*------------------------------------------------------------------------*/

void UTransformbyCTM(Matrix *ctm, XPoint *ipoints, XPoint *points, short number)
{
   pointlist current, ptptr = points;
   float fx, fy;
   /* short tmpx; (jdk) */

   for (current = ipoints; current < ipoints + number; current++, ptptr++) {
      fx = ctm->a * (float)current->x + ctm->b * (float)current->y + ctm->c;
      fy = ctm->d * (float)current->x + ctm->e * (float)current->y + ctm->f;
	
      ptptr->x = (fx >= 0) ? (short)(fx + 0.5) : (short)(fx - 0.5);
      ptptr->y = (fy >= 0) ? (short)(fy + 0.5) : (short)(fy - 0.5);
   }
}
  
/*------------------------------------------------------------------------*/
/* (same as above routine but using type (float) for point values;  this  */
/* is for calculation of Bezier curve internal points.			  */
/*------------------------------------------------------------------------*/

void UfTransformbyCTM(Matrix *ctm, XfPoint *fpoints, XPoint *points, short number)
{
   fpointlist current;
   pointlist new = points;
   float fx, fy;

   for (current = fpoints; current < fpoints + number; current++, new++) {
      fx = ctm->a * current->x + ctm->b * current->y + ctm->c;
      fy = ctm->d * current->x + ctm->e * current->y + ctm->f;
      new->x = (fx >= 0) ? (short)(fx + 0.5) : (short)(fx - 0.5);
      new->y = (fy >= 0) ? (short)(fy + 0.5) : (short)(fy - 0.5);
   }
}

/*------------------------------------------------------------------------*/

void UPopCTM()
{
   Matrixptr lastmatrix;

   if (areawin->MatStack == NULL) {
      Wprintf("Matrix stack pop error");
      return;
   }
   lastmatrix = areawin->MatStack->nextmatrix;
   free(areawin->MatStack);
   areawin->MatStack = lastmatrix;

#ifdef HAVE_CAIRO
   if (areawin->area) {
      xc_cairo_set_matrix(lastmatrix);
   }
#endif /* HAVE_CAIRO */
}

/*------------------------------------------------------------------------*/

void UPushCTM()
{
   Matrixptr nmatrix;

   nmatrix = (Matrixptr)malloc(sizeof(Matrix));
   if (areawin->MatStack == NULL)
      UResetCTM(nmatrix);
   else
      UCopyCTM(areawin->MatStack, nmatrix); 
   nmatrix->nextmatrix = areawin->MatStack;
   areawin->MatStack = nmatrix;
}

/*------------------------------------------------------------------------*/

void UTransformPoints(XPoint *points, XPoint *newpoints, short number,
	XPoint atpt, float scale, float rotate)
{
   Matrix LCTM;
 
   UResetCTM(&LCTM);
   UMultCTM(&LCTM, atpt, scale, rotate);
   UTransformbyCTM(&LCTM, points, newpoints, number);
}

/*----------------------------------------------------*/
/* Transform points inward to next hierarchical level */
/*----------------------------------------------------*/

void InvTransformPoints(XPoint *points, XPoint *newpoints, short number,
	XPoint atpt, float scale, float rotate)
{
   Matrix LCTM;
 
   UResetCTM(&LCTM);
   UPreMultCTM(&LCTM, atpt, scale, rotate);
   InvertCTM(&LCTM);
   UTransformbyCTM(&LCTM, points, newpoints, number);
}

/*----------------------------------------------------------------------*/
/* Adjust wire coords to force a wire to a horizontal or vertical	*/
/* position.								*/
/* "pospt" is the target position for the point of interest.		*/
/* "cycle" is the point number in the polygon of the point of interest.	*/
/* cycle == -1 is equivalent to the last point of the polygon.		*/
/* If "strict" is TRUE then single-segment wires are forced manhattan	*/
/* even if that means that the endpoint drifts from the target point.	*/
/* If "strict" is FALSE then single-segment wires will become non-	*/
/* manhattan so that the target point is reached.			*/
/* NOTE:  It might be preferable to add a segment to maintain a		*/
/* manhattan layout, except that we want to avoid merging nets		*/
/* together. . .							*/
/*----------------------------------------------------------------------*/

void manhattanize(XPoint *pospt, polyptr newpoly, short cycle, Boolean strict)
{
   XPoint *curpt, *bpt, *bbpt, *fpt, *ffpt;
   int deltax, deltay;

   if (newpoly->number == 1) return;	/* sanity check */

   if (cycle == -1 || cycle == newpoly->number - 1) {
      curpt = newpoly->points + newpoly->number - 1;
      bpt = newpoly->points + newpoly->number - 2;
      fpt = NULL;
      ffpt = NULL;
      if (newpoly->number > 2)
	 bbpt = newpoly->points + newpoly->number - 3;
      else
	 bbpt = NULL;
   } 
   else if (cycle == 0) {
      curpt = newpoly->points;
      fpt = newpoly->points + 1;
      bpt = NULL;
      bbpt = NULL;
      if (newpoly->number > 2)
	 ffpt = newpoly->points + 2;
      else
	 ffpt = NULL;
   }
   else {
      curpt = newpoly->points + cycle;
      fpt = newpoly->points + cycle + 1;
      bpt = newpoly->points + cycle - 1;
      if (cycle > 1)
	 bbpt = newpoly->points + cycle - 2;
      else
	 bbpt = NULL;

      if (cycle < newpoly->number - 2)
	 ffpt = newpoly->points + cycle + 2;
      else
	 ffpt = NULL;
   }

   /* enforce constraints on point behind cycle position */

   if (bpt != NULL) {
      if (bbpt != NULL) {
         if (bpt->x == bbpt->x) bpt->y = pospt->y;
         if (bpt->y == bbpt->y) bpt->x = pospt->x;
      }
      else if (strict) {
         deltax = abs(bpt->x - pospt->x);
         deltay = abs(bpt->y - pospt->y);

         /* Only one segment---just make sure it's horizontal or vertical */
         if (deltay > deltax) pospt->x = bpt->x;
         else pospt->y = bpt->y;
      }
   }

   /* enforce constraints on point forward of cycle position */

   if (fpt != NULL) {
      if (ffpt != NULL) {
         if (fpt->x == ffpt->x) fpt->y = pospt->y;
         if (fpt->y == ffpt->y) fpt->x = pospt->x;
      }
      else if (strict) {
         deltax = abs(fpt->x - pospt->x);
         deltay = abs(fpt->y - pospt->y);

         /* Only one segment---just make sure it's horizontal or vertical */
         if (deltay > deltax) pospt->x = fpt->x;
         else pospt->y = fpt->y;
      }
   }
}

/*----------------------------------------------------------------------*/
/* Bounding box calculation routines					*/
/*----------------------------------------------------------------------*/

void bboxcalc(short testval, short *lowerval, short *upperval)
{
   if (testval < *lowerval) *lowerval = testval;
   if (testval > *upperval) *upperval = testval;
}

/*----------------------------------------------------------------------*/
/* Bounding box calculation for elements which can be part of a path	*/
/*----------------------------------------------------------------------*/

void calcextents(genericptr *bboxgen, short *llx, short *lly, 
	short *urx, short *ury)
{
   switch (ELEMENTTYPE(*bboxgen)) {
      case(POLYGON): {
         pointlist bboxpts;
         for (bboxpts = TOPOLY(bboxgen)->points; bboxpts < TOPOLY(bboxgen)->points
		 + TOPOLY(bboxgen)->number; bboxpts++) {
	    bboxcalc(bboxpts->x, llx, urx);
	    bboxcalc(bboxpts->y, lly, ury);
         }
         } break;

      case(SPLINE): {
         fpointlist bboxpts;
         bboxcalc(TOSPLINE(bboxgen)->ctrl[0].x, llx, urx);
         bboxcalc(TOSPLINE(bboxgen)->ctrl[0].y, lly, ury);
         bboxcalc(TOSPLINE(bboxgen)->ctrl[3].x, llx, urx);
         bboxcalc(TOSPLINE(bboxgen)->ctrl[3].y, lly, ury);
         for (bboxpts = TOSPLINE(bboxgen)->points; bboxpts < 
		 TOSPLINE(bboxgen)->points + INTSEGS; bboxpts++) {
	    bboxcalc((short)(bboxpts->x), llx, urx);
	    bboxcalc((short)(bboxpts->y), lly, ury);
         }
         } break;

      case (ARC): {
         fpointlist bboxpts;
         for (bboxpts = TOARC(bboxgen)->points; bboxpts < TOARC(bboxgen)->points +
	         TOARC(bboxgen)->number; bboxpts++) {
            bboxcalc((short)(bboxpts->x), llx, urx);
	    bboxcalc((short)(bboxpts->y), lly, ury);
         }
         } break;
   }
}

/*----------------------------------------------------------------------*/
/* Calculate the bounding box of an object instance			*/
/*----------------------------------------------------------------------*/

void objinstbbox(objinstptr obbox, XPoint *npoints, int extend)
{
   XPoint points[4];

   points[0].x = points[1].x = obbox->bbox.lowerleft.x - extend;
   points[1].y = points[2].y = obbox->bbox.lowerleft.y + obbox->bbox.height
		+ extend;
   points[2].x = points[3].x = obbox->bbox.lowerleft.x + obbox->bbox.width
		+ extend;
   points[0].y = points[3].y = obbox->bbox.lowerleft.y - extend;

   UTransformPoints(points, npoints, 4, obbox->position,
		 obbox->scale, obbox->rotation); 
}

/*----------------------------------------------------------------------*/
/* Calculate the bounding box of a label				*/
/*----------------------------------------------------------------------*/

void labelbbox(labelptr labox, XPoint *npoints, objinstptr callinst)
{
   XPoint points[4];
   TextExtents tmpext;
   short j;

   tmpext = ULength(labox, callinst, NULL);
   points[0].x = points[1].x = (labox->anchor & NOTLEFT ? 
	       (labox->anchor & RIGHT ? -tmpext.maxwidth :
		-tmpext.maxwidth / 2) : 0);
   points[2].x = points[3].x = points[0].x + tmpext.maxwidth;
   points[0].y = points[3].y = (labox->anchor & NOTBOTTOM ?
	       (labox->anchor & TOP ? -tmpext.ascent :
		-(tmpext.ascent + tmpext.base) / 2) : -tmpext.base)
		+ tmpext.descent;
   points[1].y = points[2].y = points[0].y + tmpext.ascent - tmpext.descent;

   /* separate bounding box for pinlabels and infolabels */

   if (labox->pin)
      for (j = 0; j < 4; j++)
	 pinadjust(labox->anchor, &points[j].x, &points[j].y, 1);

   UTransformPoints(points, npoints, 4, labox->position,
		labox->scale, labox->rotation); 
}

/*----------------------------------------------------------------------*/
/* Calculate the bounding box of a graphic image			*/
/*----------------------------------------------------------------------*/

void graphicbbox(graphicptr gp, XPoint *npoints)
{
   XPoint points[4];
   int hw = xcImageGetWidth(gp->source) >> 1;
   int hh = xcImageGetHeight(gp->source) >> 1;

   points[1].x = points[2].x = hw;
   points[0].x = points[3].x = -hw;

   points[0].y = points[1].y = -hh;
   points[2].y = points[3].y = hh;

   UTransformPoints(points, npoints, 4, gp->position,
		gp->scale, gp->rotation); 
}

/*--------------------------------------------------------------*/
/* Wrapper for single call to calcbboxsingle() in the netlister */
/*--------------------------------------------------------------*/

void calcinstbbox(genericptr *bboxgen, short *llx, short *lly, short *urx,
                short *ury)
{
   *llx = *lly = 32767;
   *urx = *ury = -32768;

   calcbboxsingle(bboxgen, areawin->topinstance, llx, lly, urx, ury);
}

/*----------------------------------------------------------------------*/
/* Bounding box calculation for a single generic element		*/
/*----------------------------------------------------------------------*/

void calcbboxsingle(genericptr *bboxgen, objinstptr thisinst, 
		short *llx, short *lly, short *urx, short *ury)
{
   XPoint npoints[4];
   short j;

   /* For each screen element, compute the extents and revise bounding	*/
   /* box points, if necessary. 					*/

   switch(ELEMENTTYPE(*bboxgen)) {

      case(OBJINST):
	 objinstbbox(TOOBJINST(bboxgen), npoints, 0);

         for (j = 0; j < 4; j++) {
            bboxcalc(npoints[j].x, llx, urx);
            bboxcalc(npoints[j].y, lly, ury);
         }
	 break;

      case(LABEL):
	 /* because a pin is offset from its position point, include */
	 /* that point in the bounding box.				*/

	 if (TOLABEL(bboxgen)->pin) {
            bboxcalc(TOLABEL(bboxgen)->position.x, llx, urx);
            bboxcalc(TOLABEL(bboxgen)->position.y, lly, ury);
	 }
	 labelbbox(TOLABEL(bboxgen), npoints, thisinst);

         for (j = 0; j < 4; j++) {
            bboxcalc(npoints[j].x, llx, urx);
            bboxcalc(npoints[j].y, lly, ury);
         }
         break;

      case(GRAPHIC):
	 graphicbbox(TOGRAPHIC(bboxgen), npoints);
         for (j = 0; j < 4; j++) {
            bboxcalc(npoints[j].x, llx, urx);
            bboxcalc(npoints[j].y, lly, ury);
         }
	 break;

      case(PATH): {
	 genericptr *pathc;
	 for (pathc = TOPATH(bboxgen)->plist; pathc < TOPATH(bboxgen)->plist
		  + TOPATH(bboxgen)->parts; pathc++)
	       calcextents(pathc, llx, lly, urx, ury);
	 } break;

      default:
	 calcextents(bboxgen, llx, lly, urx, ury);
   }
}

/*------------------------------------------------------*/
/* Find if an object is in the specified library	*/
/*------------------------------------------------------*/

Boolean object_in_library(short libnum, objectptr thisobject)
{
   short i;

   for (i = 0; i < xobjs.userlibs[libnum].number; i++) {
      if (*(xobjs.userlibs[libnum].library + i) == thisobject)
	 return True;
   }
   return False;
}

/*-----------------------------------------------------------*/
/* Find if an object is in the hierarchy of the given object */
/* Returns the number (position in plist) or -1 if not found */
/*-----------------------------------------------------------*/

short find_object(objectptr pageobj, objectptr thisobject)
{
   short i, j;
   genericptr *pelem;

   for (i = 0; i < pageobj->parts; i++) {
      pelem = pageobj->plist + i;
      if (IS_OBJINST(*pelem)) {
	 if ((TOOBJINST(pelem))->thisobject == thisobject)
	    return i;
	 else if ((j = find_object((TOOBJINST(pelem))->thisobject, thisobject)) >= 0)
	    return i;  /* was j---is this the right fix? */
      }
   }
   return -1;
}

/*------------------------------------------------------*/
/* Find all pages and libraries containing this object	*/
/* and update accordingly.  If this object is a page,	*/
/* just update the page directory.			*/
/*------------------------------------------------------*/

void updatepagebounds(objectptr thisobject)
{
   short i, j;
   objectptr pageobj;

   if ((i = is_page(thisobject)) >= 0) {
      if (xobjs.pagelist[i]->background.name != (char *)NULL)
         backgroundbbox(i);
      updatepagelib(PAGELIB, i);
   }
   else {
      for (i = 0; i < xobjs.pages; i++) {
         if (xobjs.pagelist[i]->pageinst != NULL) {
	    pageobj = xobjs.pagelist[i]->pageinst->thisobject;
            if ((j = find_object(pageobj, thisobject)) >= 0) {
	       calcbboxvalues(xobjs.pagelist[i]->pageinst,
			(genericptr *)(pageobj->plist + j));
	       updatepagelib(PAGELIB, i);
	    }
         }
      }
      for (i = 0; i < xobjs.numlibs; i++)
         if (object_in_library(i, thisobject))
	    composelib(i + LIBRARY);
   }
}

/*--------------------------------------------------------------*/
/* Free memory for the schematic bounding box                   */
/*--------------------------------------------------------------*/

void invalidateschembbox(objinstptr thisinst)
{
   if (thisinst->schembbox != NULL) {
      free(thisinst->schembbox);
      thisinst->schembbox = NULL;
   }
}

/*--------------------------------------------------------------*/
/* Calculate the bounding box for an object instance.  Use the	*/
/* existing bbox and finish calculation on all the elements	*/
/* which have parameters not taking default values.		*/
/* This finishes the calculation partially done by		*/
/* calcbboxvalues().						*/
/*--------------------------------------------------------------*/

void calcbboxinst(objinstptr thisinst)
{
   objectptr thisobj;
   genericptr *gelem;
   short llx, lly, urx, ury;

   short pllx, plly, purx, pury;
   Boolean hasschembbox = FALSE;
   Boolean didparamsubs = FALSE;

   if (thisinst == NULL) return;

   thisobj = thisinst->thisobject;

   llx = thisobj->bbox.lowerleft.x;
   lly = thisobj->bbox.lowerleft.y;
   urx = llx + thisobj->bbox.width;
   ury = lly + thisobj->bbox.height;

   pllx = plly = 32767;
   purx = pury = -32768;

   for (gelem = thisobj->plist; gelem < thisobj->plist + thisobj->parts;
		gelem++) {
      /* pins which do not appear outside of the object	*/
      /* contribute to the objects "schembbox".		*/

      if (IS_LABEL(*gelem)) {
	 labelptr btext = TOLABEL(gelem);
	 if (btext->pin && !(btext->anchor & PINVISIBLE)) {
	    hasschembbox = TRUE;
	    calcbboxsingle(gelem, thisinst, &pllx, &plly, &purx, &pury);
	    continue;
	 }
      }

      if (has_param(*gelem)) {
	 if (didparamsubs == FALSE) {
	    psubstitute(thisinst);
	    didparamsubs = TRUE;
	 }
	 calcbboxsingle(gelem, thisinst, &llx, &lly, &urx, &ury);
      }

      /* If we have a clipmask, the clipmask is used to calculate the	*/
      /* bounding box, not the element it is masking.			*/

      switch(ELEMENTTYPE(*gelem)) {
	 case POLYGON: case SPLINE: case ARC: case PATH:
	    if (TOPOLY(gelem)->style & CLIPMASK) gelem++;
	    break;
      }
   }

   thisinst->bbox.lowerleft.x = llx;
   thisinst->bbox.lowerleft.y = lly;
   thisinst->bbox.width = urx - llx;
   thisinst->bbox.height = ury - lly;

   if (hasschembbox) {
      if (thisinst->schembbox == NULL)
         thisinst->schembbox = (BBox *)malloc(sizeof(BBox));

      thisinst->schembbox->lowerleft.x = pllx;
      thisinst->schembbox->lowerleft.y = plly;
      thisinst->schembbox->width = purx - pllx;
      thisinst->schembbox->height = pury - plly;
   }
   else
      invalidateschembbox(thisinst);
}

/*--------------------------------------------------------------*/
/* Update things based on a changed instance bounding box.	*/
/* If the parameter was a single-instance			*/
/* substitution, only the page should be updated.  If the	*/
/* parameter was a default value, the library should be updated */
/* and any pages containing the object where the parameter	*/
/* takes the default value.					*/
/*--------------------------------------------------------------*/

void updateinstparam(objectptr bobj)
{
   short i, j;
   objectptr pageobj;

   /* change bounds on pagelib and all pages			*/
   /* containing this *object* if and only if the object	*/
   /* instance takes the default value.  Also update the	*/
   /* library page.						*/

   for (i = 0; i < xobjs.pages; i++)
      if (xobjs.pagelist[i]->pageinst != NULL) {
	 pageobj = xobjs.pagelist[i]->pageinst->thisobject;
         if ((j = find_object(pageobj, topobject)) >= 0) {

	    /* Really, we'd like to recalculate the bounding box only if the */
	    /* parameter value is the default value which was just changed.	*/
	    /* However, then any non-default values may contain the wrong	*/
	    /* substitutions.						*/

	    objinstptr cinst = TOOBJINST(pageobj->plist + j);
	    if (cinst->thisobject->params == NULL) {
	       calcbboxvalues(xobjs.pagelist[i]->pageinst, pageobj->plist + j);
	       updatepagelib(PAGELIB, i);
	    }
	 }
      }

   for (i = 0; i < xobjs.numlibs; i++)
      if (object_in_library(i, topobject))
	 composelib(i + LIBRARY);
}

/*--------------------------------------------------------------*/
/* Calculate bbox on all elements of the given object		*/
/*--------------------------------------------------------------*/

void calcbbox(objinstptr binst)
{
   calcbboxvalues(binst, (genericptr *)NULL);
   if (binst == areawin->topinstance) {
      updatepagebounds(topobject);
   }
}

/*--------------------------------------------------------------*/
/* Calculate bbox on the given element of the specified object.	*/
/* This is a wrapper for calcbboxvalues() assuming that we're	*/
/* on the top-level, and that page bounds need to be updated.	*/
/*--------------------------------------------------------------*/

void singlebbox(genericptr *gelem)
{
   calcbboxvalues(areawin->topinstance, (genericptr *)gelem);
   updatepagebounds(topobject);
}

/*----------------------------------------------------------------------*/
/* Extend bounding box based on selected elements only			*/
/*----------------------------------------------------------------------*/

void calcbboxselect()
{
   short *bsel;
   for (bsel = areawin->selectlist; bsel < areawin->selectlist +
		areawin->selects; bsel++)
      calcbboxvalues(areawin->topinstance, topobject->plist + *bsel);
  
   updatepagebounds(topobject);
}

/*--------------------------------------------------------------*/
/* Update Bounding box for an object.				*/
/* If newelement == NULL, calculate bounding box from scratch.	*/
/* Otherwise, expand bounding box to enclose newelement.	*/
/*--------------------------------------------------------------*/

void calcbboxvalues(objinstptr thisinst, genericptr *newelement)
{
   genericptr *bboxgen;
   short llx, lly, urx, ury;
   objectptr thisobj = thisinst->thisobject;

   /* no action if there are no elements */
   if (thisobj->parts == 0) return;

   /* If this object has parameters, then we will do a separate		*/
   /* bounding box calculation on parameterized parts.  This		*/
   /* calculation ignores them, and the result is a base that the	*/
   /* instance bounding-box computation can use as a starting point.	*/

   /* set starting bounds as maximum bounds of screen */
   llx = lly = 32767;
   urx = ury = -32768;

   for (bboxgen = thisobj->plist; bboxgen < thisobj->plist +
		thisobj->parts; bboxgen++) {

      /* override the "for" loop if we're doing a single element */
      if (newelement != NULL) bboxgen = newelement;

      if ((thisobj->params == NULL) || (!has_param(*bboxgen))) {
	 /* pins which do not appear outside of the object 	*/
	 /* are ignored now---will be computed per instance.	*/

	 if (IS_LABEL(*bboxgen)) {
	    labelptr btext = TOLABEL(bboxgen);
	    if (btext->pin && !(btext->anchor & PINVISIBLE)) {
	       goto nextgen;
	    }
	 }
	 calcbboxsingle(bboxgen, thisinst, &llx, &lly, &urx, &ury);

	 if (newelement == NULL)
	    switch(ELEMENTTYPE(*bboxgen)) {
	       case POLYGON: case SPLINE: case ARC: case PATH:
	          if (TOPOLY(bboxgen)->style & CLIPMASK)
		     bboxgen++;
		  break;
	    }
      }
nextgen:
      if (newelement != NULL) break;
   }

   /* if this is a single-element calculation and its bounding box	*/
   /* turned out to be smaller than the object's, then we need to	*/
   /* recompute the entire object's bounding box in case it got		*/
   /* smaller.  This is not recursive, in spite of looks.		*/

   if (newelement != NULL) {
      if (llx > thisobj->bbox.lowerleft.x &&
		lly > thisobj->bbox.lowerleft.y &&
		urx < (thisobj->bbox.lowerleft.x + thisobj->bbox.width) &&
		ury < (thisobj->bbox.lowerleft.y + thisobj->bbox.height)) {
	 calcbboxvalues(thisinst, NULL);
	 return;
      }
      else {
	 bboxcalc(thisobj->bbox.lowerleft.x, &llx, &urx);
	 bboxcalc(thisobj->bbox.lowerleft.y, &lly, &ury);
	 bboxcalc(thisobj->bbox.lowerleft.x + thisobj->bbox.width, &llx, &urx);
	 bboxcalc(thisobj->bbox.lowerleft.y + thisobj->bbox.height, &lly, &ury);
      }
   }

   /* Set the new bounding box.  In pathological cases, such as a page	*/
   /* with only pin labels, the bounds may not have been changed from	*/
   /* their initial values.  If so, then don't touch the bounding box.	*/

   if ((llx <= urx) && (lly <= ury)) {
      thisobj->bbox.lowerleft.x = llx;
      thisobj->bbox.lowerleft.y = lly;
      thisobj->bbox.width = urx - llx;
      thisobj->bbox.height = ury - lly;
   }

   /* calculate instance-specific values */
   calcbboxinst(thisinst);
}

/*------------------------------------------------------*/
/* Center an object in the viewing window		*/
/*------------------------------------------------------*/

void centerview(objinstptr tinst)
{
   XPoint origin, corner;
   Dimension width, height;
   float fitwidth, fitheight;
   objectptr tobj = tinst->thisobject;

   origin = tinst->bbox.lowerleft;
   corner.x = origin.x + tinst->bbox.width;
   corner.y = origin.y + tinst->bbox.height;

   extendschembbox(tinst, &origin, &corner);

   width = corner.x - origin.x;
   height = corner.y - origin.y;

   fitwidth = (float)areawin->width / ((float)width + 2 * DEFAULTGRIDSPACE);
   fitheight = (float)areawin->height / ((float)height + 2 * DEFAULTGRIDSPACE);

   tobj->viewscale = (fitwidth < fitheight) ?
		 min(MINAUTOSCALE, fitwidth) : min(MINAUTOSCALE, fitheight);

   tobj->pcorner.x = origin.x - (areawin->width
	       / tobj->viewscale - width) / 2;
   tobj->pcorner.y = origin.y - (areawin->height
	       / tobj->viewscale - height) / 2;

   /* Copy new position values to the current window */

   if ((areawin->topinstance != NULL) && (tobj == topobject)) {
      areawin->pcorner = tobj->pcorner;
      areawin->vscale = tobj->viewscale;
   }
}

/*-----------------------------------------------------------*/
/* Refresh the window and scrollbars and write the page name */
/*-----------------------------------------------------------*/

void refresh(xcWidget bw, caddr_t clientdata, caddr_t calldata)
{
   areawin->redraw_needed = True;
   drawarea(NULL, NULL, NULL);
   if (areawin->scrollbarh)
      drawhbar(areawin->scrollbarh, NULL, NULL);
   if (areawin->scrollbarv)
      drawvbar(areawin->scrollbarv, NULL, NULL);
   printname(topobject);
}

/*------------------------------------------------------*/
/* Center the current page in the viewing window	*/
/*------------------------------------------------------*/

void zoomview(xcWidget w, caddr_t clientdata, caddr_t calldata)
{
   if (eventmode == NORMAL_MODE || eventmode == COPY_MODE ||
	 eventmode == MOVE_MODE || eventmode == CATALOG_MODE ||
	eventmode == FONTCAT_MODE || eventmode == EFONTCAT_MODE ||
	eventmode == CATMOVE_MODE) {

      if (areawin->topinstance)
	 centerview(areawin->topinstance);
      areawin->lastbackground = NULL;
      renderbackground();
      refresh(NULL, NULL, NULL);
   }
}

/*---------------------------------------------------------*/
/* Basic X Graphics Routines in the User coordinate system */
/*---------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawSimpleLine(XPoint *pt1, XPoint *pt2)
{
   XPoint newpt1, newpt2;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   UTransformbyCTM(DCTM, pt1, &newpt1, 1);
   UTransformbyCTM(DCTM, pt2, &newpt2, 1);

   DrawLine(dpy, areawin->window, areawin->gc,
		newpt1.x, newpt1.y, newpt2.x, newpt2.y); 
} 
#endif /* !HAVE_CAIRO */

/*-------------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawLine(XPoint *pt1, XPoint *pt2)
{
   float tmpwidth = UTopTransScale(xobjs.pagelist[areawin->page]->wirewidth);

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   SetLineAttributes(dpy, areawin->gc, tmpwidth, LineSolid, CapRound, JoinBevel);
   UDrawSimpleLine(pt1, pt2); 
} 
#endif /* !HAVE_CAIRO */

/*----------------------------------------------------------------------*/
/* Add circle at given point to indicate that the point is a parameter.	*/
/* The circle is divided into quarters.  For parameterized y-coordinate	*/
/* the top and bottom quarters are drawn.  For parameterized x-		*/
/* coordinate, the left and right quarters are drawn.  A full circle	*/
/* indicates either both x- and y-coordinates are parameterized, or	*/
/* else any other kind of parameterization (presently, not used).	*/
/*									*/
/* (note that the two angles in XDrawArc() are 1) the start angle,	*/
/* measured in absolute 64th degrees from 0 (3 o'clock), and 2) the	*/
/* path length, in relative 64th degrees (positive = counterclockwise,	*/
/* negative = clockwise)).						*/
/*----------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawCircle(XPoint *upt, u_char which)
{
   XPoint wpt;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   user_to_window(*upt, &wpt);
   SetThinLineAttributes(dpy, areawin->gc, 0, LineSolid, CapButt, JoinMiter);

   switch(which) {
      case P_POSITION_X:
         XDrawArc(dpy, areawin->window, areawin->gc, wpt.x - 4,
		wpt.y - 4, 8, 8, -(45 * 64), (90 * 64));
         XDrawArc(dpy, areawin->window, areawin->gc, wpt.x - 4,
		wpt.y - 4, 8, 8, (135 * 64), (90 * 64));
	 break;
      case P_POSITION_Y:
         XDrawArc(dpy, areawin->window, areawin->gc, wpt.x - 4,
		wpt.y - 4, 8, 8, (45 * 64), (90 * 64));
         XDrawArc(dpy, areawin->window, areawin->gc, wpt.x - 4,
		wpt.y - 4, 8, 8, (225 * 64), (90 * 64));
	 break;
      default:
         XDrawArc(dpy, areawin->window, areawin->gc, wpt.x - 4,
		wpt.y - 4, 8, 8, 0, (360 * 64));
	 break;
   }
}
#endif /* !HAVE_CAIRO */

/*----------------------------------------------------------------------*/
/* Add "X" at string origin						*/
/*----------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawXAt(XPoint *wpt)
{
   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   SetThinLineAttributes(dpy, areawin->gc, 0, LineSolid, CapButt, JoinMiter);
   DrawLine(dpy, areawin->window, areawin->gc, wpt->x - 3,
		wpt->y - 3, wpt->x + 3, wpt->y + 3);
   DrawLine(dpy, areawin->window, areawin->gc, wpt->x + 3,
		wpt->y - 3, wpt->x - 3, wpt->y + 3);
}
#endif /* !HAVE_CAIRO */

/*----------------------------------------------------------------------*/
/* Draw "X" on current level						*/
/*----------------------------------------------------------------------*/

void UDrawX(labelptr curlabel)
{
   XPoint wpt;

   user_to_window(curlabel->position, &wpt);
   UDrawXAt(&wpt);
}

/*----------------------------------------------------------------------*/
/* Draw "X" on top level (only for LOCAL and GLOBAL pin labels)		*/
/*----------------------------------------------------------------------*/

void UDrawXDown(labelptr curlabel)
{
   XPoint wpt;

   UTransformbyCTM(DCTM, &curlabel->position, &wpt, 1);
   UDrawXAt(&wpt);
}

/*----------------------------------------------------------------------*/
/* Find the "real" width, height, and origin of an object including pin	*/
/* labels and so forth that only show up on a schematic when it is the	*/
/* top-level object.							*/
/*----------------------------------------------------------------------*/

int toplevelwidth(objinstptr bbinst, short *rllx)
{
   short llx, urx;
   short origin, corner;

   if (bbinst->schembbox == NULL) {
      if (rllx) *rllx = bbinst->bbox.lowerleft.x;
      return bbinst->bbox.width;
   }

   origin = bbinst->bbox.lowerleft.x;
   corner = origin + bbinst->bbox.width;

   llx = bbinst->schembbox->lowerleft.x;
   urx = llx + bbinst->schembbox->width;

   bboxcalc(llx, &origin, &corner);
   bboxcalc(urx, &origin, &corner);

   if (rllx) *rllx = origin;
   return(corner - origin);
}

/*----------------------------------------------------------------------*/

int toplevelheight(objinstptr bbinst, short *rlly)
{
   short lly, ury;
   short origin, corner;

   if (bbinst->schembbox == NULL) {
      if (rlly) *rlly = bbinst->bbox.lowerleft.y;
      return bbinst->bbox.height;
   }

   origin = bbinst->bbox.lowerleft.y;
   corner = origin + bbinst->bbox.height;

   lly = bbinst->schembbox->lowerleft.y;
   ury = lly + bbinst->schembbox->height;

   bboxcalc(lly, &origin, &corner);
   bboxcalc(ury, &origin, &corner);

   if (rlly) *rlly = origin;
   return(corner - origin);
}

/*----------------------------------------------------------------------*/
/* Add dimensions of schematic pins to an object's bounding box		*/
/*----------------------------------------------------------------------*/

void extendschembbox(objinstptr bbinst, XPoint *origin, XPoint *corner)
{
   short llx, lly, urx, ury;

   if ((bbinst == NULL) || (bbinst->schembbox == NULL)) return;

   llx = bbinst->schembbox->lowerleft.x;
   lly = bbinst->schembbox->lowerleft.y;
   urx = llx + bbinst->schembbox->width;
   ury = lly + bbinst->schembbox->height;

   bboxcalc(llx, &(origin->x), &(corner->x));
   bboxcalc(lly, &(origin->y), &(corner->y));
   bboxcalc(urx, &(origin->x), &(corner->x));
   bboxcalc(ury, &(origin->y), &(corner->y));
}

/*----------------------------------------------------------------------*/
/* Adjust a pinlabel position to account for pad spacing		*/
/*----------------------------------------------------------------------*/

void pinadjust (short anchor, short *xpoint, short *ypoint, short dir)
{
   int delx, dely;

   dely = (anchor & NOTBOTTOM) ?
            ((anchor & TOP) ? -PADSPACE : 0) : PADSPACE;
   delx = (anchor & NOTLEFT) ?
            ((anchor & RIGHT) ? -PADSPACE : 0) : PADSPACE;

   if (xpoint != NULL) *xpoint += (dir > 0) ? delx : -delx;
   if (ypoint != NULL) *ypoint += (dir > 0) ? dely : -dely;
}

/*----------------------------------------------------------------------*/
/* Draw line for editing text (position of cursor in string is given by */
/*   tpos (2nd parameter)						*/
/*----------------------------------------------------------------------*/

void UDrawTextLine(labelptr curlabel, short tpos)
{
   XPoint  points[2]; /* top and bottom of text cursor line */
   short   tmpanchor, xbase;
   int     maxwidth;
   TextExtents tmpext;
   TextLinesInfo tlinfo;
   
   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   /* correct for position, rotation, scale, and flip invariance of text */

   UPushCTM();
   UPreMultCTM(DCTM, curlabel->position, curlabel->scale, curlabel->rotation);
   tmpanchor = flipadjust(curlabel->anchor);

   SetForeground(dpy, areawin->gc, AUXCOLOR);

   tlinfo.dostop = 0;
   tlinfo.tbreak = NULL;
   tlinfo.padding = NULL;

   tmpext = ULength(curlabel, areawin->topinstance, &tlinfo);
   maxwidth = tmpext.maxwidth;
   xbase = tmpext.base;
   tlinfo.dostop = tpos;
   tmpext = ULength(curlabel, areawin->topinstance, &tlinfo);

   points[0].x = (tmpanchor & NOTLEFT ?
        (tmpanchor & RIGHT ? -maxwidth : -maxwidth >> 1) : 0) + tmpext.width;
   if ((tmpanchor & JUSTIFYRIGHT) && tlinfo.padding)
      points[0].x += tlinfo.padding[tlinfo.line];
   else if ((tmpanchor & TEXTCENTERED) && tlinfo.padding)
      points[0].x += 0.5 * tlinfo.padding[tlinfo.line];
   points[0].y = (tmpanchor & NOTBOTTOM ?
        (tmpanchor & TOP ? -tmpext.ascent : -(tmpext.ascent + xbase) / 2)
	: -xbase) + tmpext.base - 3;
   points[1].x = points[0].x;
   points[1].y = points[0].y + TEXTHEIGHT + 6;

   if (curlabel->pin) {
      pinadjust(tmpanchor, &(points[0].x), &(points[0].y), 1);
      pinadjust(tmpanchor, &(points[1].x), &(points[1].y), 1);
   }
   if (tlinfo.padding != NULL) free(tlinfo.padding);

   /* draw the line */

   UDrawLine(&points[0], &points[1]);
   UPopCTM();

   UDrawX(curlabel);
}

/*-----------------------------------------------------------------*/
/* Draw lines for editing text when multiple characters are chosen */
/*-----------------------------------------------------------------*/

void UDrawTLine(labelptr curlabel)
{
   UDrawTextLine(curlabel, areawin->textpos);
   if ((areawin->textend > 0) && (areawin->textend < areawin->textpos)) {
      UDrawTextLine(curlabel, areawin->textend);
   }
}

/*----------------------*/
/* Draw an X		*/
/*----------------------*/

#ifndef HAVE_CAIRO
void UDrawXLine(XPoint opt, XPoint cpt)
{
   XPoint upt, vpt;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   SetForeground(dpy, areawin->gc, AUXCOLOR);

   user_to_window(cpt, &upt);
   user_to_window(opt, &vpt);

   SetThinLineAttributes(dpy, areawin->gc, 0, LineOnOffDash, CapButt, JoinMiter);
   DrawLine(dpy, areawin->window, areawin->gc, vpt.x, vpt.y, upt.x, upt.y);

   SetThinLineAttributes(dpy, areawin->gc, 0, LineSolid, CapButt, JoinMiter);
   DrawLine(dpy, areawin->window, areawin->gc, upt.x - 3, upt.y - 3,
		upt.x + 3, upt.y + 3);
   DrawLine(dpy, areawin->window, areawin->gc, upt.x + 3, upt.y - 3,
		upt.x - 3, upt.y + 3);

   SetForeground(dpy, areawin->gc, areawin->gccolor);
}
#endif /* HAVE_CAIRO */

/*-------------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawBox(XPoint origin, XPoint corner)
{
   XPoint	worig, wcorn;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   user_to_window(origin, &worig);
   user_to_window(corner, &wcorn);

   SetForeground(dpy, areawin->gc, AUXCOLOR);
   SetThinLineAttributes(dpy, areawin->gc, 0, LineSolid, CapRound, JoinBevel);
   DrawLine(dpy, areawin->window, areawin->gc, worig.x, worig.y,
		worig.x, wcorn.y);
   DrawLine(dpy, areawin->window, areawin->gc, worig.x, wcorn.y,
		wcorn.x, wcorn.y);
   DrawLine(dpy, areawin->window, areawin->gc, wcorn.x, wcorn.y,
		wcorn.x, worig.y);
   DrawLine(dpy, areawin->window, areawin->gc, wcorn.x, worig.y,
		worig.x, worig.y);
}
#endif /* HAVE_CAIRO */

/*----------------------------------------------------------------------*/
/* Get a box indicating the dimensions of the edit element that most	*/
/* closely reach the position "corner".					*/
/*----------------------------------------------------------------------*/

float UGetRescaleBox(XPoint *corner, XPoint *newpoints)
{
   genericptr	rgen;
   float	savescale, newscale;
   long		mindist, testdist, refdist;
   labelptr	rlab;
   graphicptr   rgraph;
   objinstptr	rinst;
   int		i;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      // return 0.0;
   }

   if (areawin->selects == 0) return 0.0;

   /* Use only the 1st selection as a reference to set the scale */

   rgen = SELTOGENERIC(areawin->selectlist);

   switch(ELEMENTTYPE(rgen)) {
      case LABEL:
	 rlab = (labelptr)rgen;
	 labelbbox(rlab, newpoints, areawin->topinstance);
	 newpoints[4] = newpoints[0];
	 mindist = LONG_MAX;
	 for (i = 0; i < 4; i++) {
	    testdist = finddist(&newpoints[i], &newpoints[i+1], corner);
	    if (testdist < mindist)
	       mindist = testdist;
	 }
	 refdist = wirelength(corner, &(rlab->position));
	 mindist = (int)sqrt(abs((double)mindist));
	 savescale = rlab->scale;
	 if (!test_insideness((int)corner->x, (int)corner->y, newpoints))
	    mindist = -mindist;
	 if (refdist == mindist) refdist = 1 - mindist;
	 if (rlab->scale < 0) rlab->scale = -rlab->scale;
	 newscale = fabs(rlab->scale * (float)refdist / (float)(refdist + mindist));
	 if (newscale > 10 * rlab->scale) newscale = 10 * rlab->scale;
	 if (areawin->snapto) {
	    float snapstep = 2 * (float)xobjs.pagelist[areawin->page]->gridspace
			/ (float)xobjs.pagelist[areawin->page]->snapspace;
	    newscale = (float)((int)(newscale * snapstep)) / snapstep;
	    if (newscale < (1.0 / snapstep)) newscale = (1.0 / snapstep);
	 }
	 else if (newscale < 0.1 * rlab->scale) newscale = 0.1 * rlab->scale;
	 rlab->scale = (savescale < 0) ? -newscale : newscale;
	 labelbbox(rlab, newpoints, areawin->topinstance);
	 rlab->scale = savescale;
	 if (savescale < 0) newscale = -newscale;
         break;

      case GRAPHIC:
	 rgraph = (graphicptr)rgen;
	 graphicbbox(rgraph, newpoints);
	 newpoints[4] = newpoints[0];
	 mindist = LONG_MAX;
	 for (i = 0; i < 4; i++) {
	    testdist = finddist(&newpoints[i], &newpoints[i+1], corner);
	    if (testdist < mindist)
	       mindist = testdist;
	 }
	 refdist = wirelength(corner, &(rgraph->position));
	 mindist = (int)sqrt(abs((double)mindist));
	 savescale = rgraph->scale;
	 if (!test_insideness((int)corner->x, (int)corner->y, newpoints))
	    mindist = -mindist;
	 if (refdist == mindist) refdist = 1 - mindist;  /* avoid inf result */
	 if (rgraph->scale < 0) rgraph->scale = -rgraph->scale;
	 newscale = fabs(rgraph->scale * (float)refdist / (float)(refdist + mindist));
	 if (newscale > 10 * rgraph->scale) newscale = 10 * rgraph->scale;
	 if (areawin->snapto) {
	    float snapstep = 2 * (float)xobjs.pagelist[areawin->page]->gridspace
			/ (float)xobjs.pagelist[areawin->page]->snapspace;
	    newscale = (float)((int)(newscale * snapstep)) / snapstep;
	    if (newscale < (1.0 / snapstep)) newscale = (1.0 / snapstep);
	 }
	 else if (newscale < 0.1 * rgraph->scale) newscale = 0.1 * rgraph->scale;
	 rgraph->scale = (savescale < 0) ? -newscale : newscale;
	 graphicbbox(rgraph, newpoints);
	 rgraph->scale = savescale;
	 if (savescale < 0) newscale = -newscale;
         break;

      case OBJINST:
	 rinst = (objinstptr)rgen;
	 objinstbbox(rinst, newpoints, 0);
	 newpoints[4] = newpoints[0];
	 mindist = LONG_MAX;
	 for (i = 0; i < 4; i++) {
	    testdist = finddist(&newpoints[i], &newpoints[i+1], corner);
	    if (testdist < mindist)
	       mindist = testdist;
	 }
	 refdist = wirelength(corner, &(rinst->position));
	 mindist = (int)sqrt(abs((double)mindist));
	 savescale = rinst->scale;
	 if (!test_insideness((int)corner->x, (int)corner->y, newpoints))
	    mindist = -mindist;
	 if (refdist == mindist) refdist = 1 - mindist;  /* avoid inf result */
	 if (rinst->scale < 0) rinst->scale = -rinst->scale;
	 newscale = fabs(rinst->scale * (float)refdist / (float)(refdist + mindist));
	 if (newscale > 10 * rinst->scale) newscale = 10 * rinst->scale;
	 if (areawin->snapto) {
	    float snapstep = 2 * (float)xobjs.pagelist[areawin->page]->gridspace
			/ (float)xobjs.pagelist[areawin->page]->snapspace;
	    newscale = (float)((int)(newscale * snapstep)) / snapstep;
	    if (newscale < (1.0 / snapstep)) newscale = (1.0 / snapstep);
	 }
	 else if (newscale < 0.1 * rinst->scale) newscale = 0.1 * rinst->scale;
	 rinst->scale = (savescale < 0) ? -newscale : newscale;
	 objinstbbox(rinst, newpoints, 0);
	 rinst->scale = savescale;
	 if (savescale < 0) newscale = -newscale;
         break;
   }

   return newscale;
}

/*----------------------------------------------------------------------*/
/* Draw a box indicating the dimensions of the edit element that most	*/
/* closely reach the position "corner".					*/
/*----------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawRescaleBox(XPoint *corner)
{
   XPoint origpoints[5], newpoints[5];

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   if (areawin->selects == 0)
      return;

   UGetRescaleBox(corner, newpoints);

   SetForeground(dpy, areawin->gc, AUXCOLOR);
   SetThinLineAttributes(dpy, areawin->gc, 0, LineSolid, CapRound, JoinBevel);
   
   UTransformbyCTM(DCTM, newpoints, origpoints, 4);
   strokepath(origpoints, 4, 0, 1);
}
#endif /* HAVE_CAIRO */

/*-------------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawBBox()
{
   XPoint	origin;
   XPoint	worig, wcorn, corner;
   objinstptr	bbinst = areawin->topinstance;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   if ((!areawin->bboxon) || (checkforbbox(topobject) != NULL)) return;

   origin = bbinst->bbox.lowerleft;
   corner.x = origin.x + bbinst->bbox.width;
   corner.y = origin.y + bbinst->bbox.height;

   /* Include any schematic labels in the bounding box.	*/
   extendschembbox(bbinst, &origin, &corner);

   user_to_window(origin, &worig);
   user_to_window(corner, &wcorn);

   SetForeground(dpy, areawin->gc, BBOXCOLOR);
   DrawLine(dpy, areawin->window, areawin->gc, worig.x, worig.y,
		worig.x, wcorn.y);
   DrawLine(dpy, areawin->window, areawin->gc, worig.x, wcorn.y,
		wcorn.x, wcorn.y);
   DrawLine(dpy, areawin->window, areawin->gc, wcorn.x, wcorn.y,
		wcorn.x, worig.y);
   DrawLine(dpy, areawin->window, areawin->gc, wcorn.x, worig.y,
		worig.x, worig.y);
}
#endif /* !HAVE_CAIRO */

/*----------------------------------------------------------------------*/
/* Fill and/or draw a border around the stroking path			*/
/*----------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void strokepath(XPoint *pathlist, short number, short style, float width)
{
   float        tmpwidth;

   tmpwidth = UTopTransScale(width);

   if (!(style & CLIPMASK) || (areawin->showclipmasks == TRUE) ||
		(areawin->clipped < 0)) {
      if (style & FILLED || (!(style & FILLED) && style & OPAQUE)) {
         if ((style & FILLSOLID) == FILLSOLID)
            SetFillStyle(dpy, areawin->gc, FillSolid);
         else if (!(style & FILLED)) {
            SetFillStyle(dpy, areawin->gc, FillOpaqueStippled); 
	    SetStipple(dpy, areawin->gc, 7);
         }
         else {
	    if (style & OPAQUE)
               SetFillStyle(dpy, areawin->gc, FillOpaqueStippled);
	    else
               SetFillStyle(dpy, areawin->gc, FillStippled);
            SetStipple(dpy, areawin->gc, ((style & FILLSOLID) >> 5));
         }
         FillPolygon(dpy, areawin->window, areawin->gc, pathlist, number, Nonconvex,
		CoordModeOrigin);
         /* return to original state */
         SetFillStyle(dpy, areawin->gc, FillSolid);
      }
      if (!(style & NOBORDER)) {
         if (style & (DASHED | DOTTED)) {
	    /* Set up dots or dashes */
	    char dashstring[2];
	    /* prevent values greater than 255 from folding back into	*/
	    /* type char.  Limit to 63 (=255/4) to keep at least the	*/
	    /* dot/gap ratio to scale when 'gap' is at its maximum	*/
	    /* value.							*/
	    unsigned char dotsize = min(63, max(1, (short)tmpwidth));
	    if (style & DASHED)
	       dashstring[0] = 4 * dotsize;
	    else if (style & DOTTED)
	       dashstring[0] = dotsize;
	    dashstring[1] = 4 * dotsize;
            SetDashes(dpy, areawin->gc, 0, dashstring, 2);
            SetLineAttributes(dpy, areawin->gc, tmpwidth, LineOnOffDash,
		CapButt, (style & SQUARECAP) ? JoinMiter : JoinBevel);
         }
         else
            SetLineAttributes(dpy, areawin->gc, tmpwidth, LineSolid,
		(style & SQUARECAP) ? CapProjecting : CapRound,
		(style & SQUARECAP) ? JoinMiter : JoinBevel);

         /* draw the spline and close off if so specified */
         DrawLines(dpy, areawin->window, areawin->gc, pathlist,
		number, CoordModeOrigin);
         if (!(style & UNCLOSED))
            DrawLine(dpy, areawin->window, areawin->gc, pathlist[0].x,
		pathlist[0].y, pathlist[number - 1].x, pathlist[number - 1].y);
      }
   }

   if (style & CLIPMASK) {
      if (areawin->clipped == 0) {
         XSetForeground(dpy, areawin->cmgc, 0);
         XFillRectangle(dpy, areawin->clipmask, areawin->cmgc, 0, 0,
		areawin->width, areawin->height);
         XSetForeground(dpy, areawin->cmgc, 1);
         FillPolygon(dpy, areawin->clipmask, areawin->cmgc, pathlist,
		number, Nonconvex, CoordModeOrigin);
         XSetClipMask(dpy, areawin->gc, areawin->clipmask);
	 // printf("level 0: Clip to clipmask\n");	// Diagnostic
         areawin->clipped++;
      }
      else if ((areawin->clipped > 0) && (areawin->clipped & 1) == 0) {
	 if (areawin->pbuf == (Pixmap)NULL) {
	    areawin->pbuf = XCreatePixmap (dpy, areawin->window,
			areawin->width, areawin->height, 1);
	 }
	 XCopyArea(dpy, areawin->clipmask, areawin->pbuf, areawin->cmgc,
		0, 0, areawin->width, areawin->height, 0, 0);
         XSetForeground(dpy, areawin->cmgc, 0);
         XFillRectangle(dpy, areawin->clipmask, areawin->cmgc, 0, 0,
		areawin->width, areawin->height);
         XSetForeground(dpy, areawin->cmgc, 1);
         FillPolygon(dpy, areawin->clipmask, areawin->cmgc, pathlist,
		number, Nonconvex, CoordModeOrigin);
         XSetFunction(dpy, areawin->cmgc, GXand);
	 XCopyArea(dpy, areawin->pbuf, areawin->clipmask, areawin->cmgc,
		0, 0, areawin->width, areawin->height, 0, 0);
         XSetFunction(dpy, areawin->cmgc, GXcopy);
         XSetClipMask(dpy, areawin->gc, areawin->clipmask);
	 // printf("level X: Clip to clipmask\n");	// Diagnostic
         areawin->clipped++;
      }
   }
}
#endif /* !HAVE_CAIRO */

/*-------------------------------------------------------------------------*/

void makesplinepath(splineptr thespline, XPoint *pathlist)
{
   XPoint *tmpptr = pathlist;

   UTransformbyCTM(DCTM, &(thespline->ctrl[0]), tmpptr, 1);
   UfTransformbyCTM(DCTM, thespline->points, ++tmpptr, INTSEGS);
   UTransformbyCTM(DCTM, &(thespline->ctrl[3]), tmpptr + INTSEGS, 1);
}

/*-------------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawSpline(splineptr thespline, float passwidth)
{
   XPoint       tmppoints[SPLINESEGS];
   float	scaledwidth;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   scaledwidth = thespline->width * passwidth;

   makesplinepath(thespline, tmppoints);
   strokepath(tmppoints, SPLINESEGS, thespline->style, scaledwidth);
}
#endif /* HAVE_CAIRO */

/*-------------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawPolygon(polyptr thepoly, float passwidth)
{
   XPoint *tmppoints = (pointlist) malloc(thepoly->number * sizeof(XPoint));
   float scaledwidth;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   scaledwidth = thepoly->width * passwidth;

   UTransformbyCTM(DCTM, thepoly->points, tmppoints, thepoly->number);
   strokepath(tmppoints, thepoly->number, thepoly->style, scaledwidth);
   free(tmppoints);
}
#endif /* HAVE_CAIRO */

/*-------------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawArc(arcptr thearc, float passwidth)
{
   XPoint  tmppoints[RSTEPS + 2];
   float scaledwidth;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   scaledwidth = thearc->width * passwidth;

   UfTransformbyCTM(DCTM, thearc->points, tmppoints, thearc->number);
   strokepath(tmppoints, thearc->number, thearc->style, scaledwidth);
}
#endif /* HAVE_CAIRO */

/*-------------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawPath(pathptr thepath, float passwidth)
{
   XPoint	*tmppoints = (pointlist) malloc(sizeof(XPoint));
   genericptr	*genpath;
   polyptr	thepoly;
   splineptr	thespline;
   int		pathsegs = 0, curseg = 0;
   float	scaledwidth;
   
   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   for (genpath = thepath->plist; genpath < thepath->plist + thepath->parts;
	  genpath++) {
      switch(ELEMENTTYPE(*genpath)) {
	 case POLYGON:
	    thepoly = TOPOLY(genpath);
	    pathsegs += thepoly->number;
	    tmppoints = (pointlist) realloc(tmppoints, pathsegs * sizeof(XPoint));
   	    UTransformbyCTM(DCTM, thepoly->points, tmppoints + curseg, thepoly->number);
	    curseg = pathsegs;
	    break;
	 case SPLINE:
	    thespline = TOSPLINE(genpath);
	    pathsegs += SPLINESEGS;
	    tmppoints = (pointlist) realloc(tmppoints, pathsegs * sizeof(XPoint));
   	    makesplinepath(thespline, tmppoints + curseg);
	    curseg = pathsegs;
	    break;
      }
   } 
   scaledwidth = thepath->width * passwidth;

   strokepath(tmppoints, pathsegs, thepath->style, scaledwidth);
   free(tmppoints);
}
#endif /* HAVE_CAIRO */

/*----------------------------------------------------------------------*/
/* Main recursive object instance drawing routine.			*/
/*    context is the instance information passed down from above	*/
/*    theinstance is the object instance to be drawn			*/
/*    level is the level of recursion 					*/
/*    passcolor is the inherited color value passed to object		*/
/*    passwidth is the inherited linewidth value passed to the object	*/
/*    stack contains graphics context information			*/
/*----------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawObject(objinstptr theinstance, short level, int passcolor,
		float passwidth, pushlistptr *stack)
{
   genericptr	*areagen;
   float	tmpwidth;
   int		defaultcolor = passcolor;
   int		curcolor = passcolor;
   int		thispart;
   short	savesel;
   XPoint 	bboxin[2], bboxout[2];
   u_char	xm, ym;
   objectptr	theobject = theinstance->thisobject;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   /* Save the number of selections and set it to zero while we do the	*/
   /* object drawing.							*/

   savesel = areawin->selects;
   areawin->selects = 0;

   /* All parts are given in the coordinate system of the object, unless */
   /* this is the top-level object, in which they will be interpreted as */
   /* relative to the screen.						 */

   UPushCTM();

   if (stack) {
      /* Save the current clipping mask and push it on the stack */
      if (areawin->clipped > 0) {
         push_stack((pushlistptr *)stack, theinstance, (char *)areawin->clipmask);
	 areawin->clipmask = XCreatePixmap(dpy, areawin->window, areawin->width,
		areawin->height, 1);
	 XCopyArea(dpy, (Pixmap)(*stack)->clientdata, areawin->clipmask, areawin->cmgc,
		0, 0, areawin->width, areawin->height, 0, 0);
      }
      else
         push_stack((pushlistptr *)stack, theinstance, (char *)NULL);
   }
   if (level != 0)
       UPreMultCTM(DCTM, theinstance->position, theinstance->scale,
			theinstance->rotation);

   if (theinstance->style & LINE_INVARIANT)
      passwidth /= fabs(theinstance->scale);

   /* do a quick test for intersection with the display window */

   bboxin[0].x = theobject->bbox.lowerleft.x;
   bboxin[0].y = theobject->bbox.lowerleft.y;
   bboxin[1].x = theobject->bbox.lowerleft.x + theobject->bbox.width;
   bboxin[1].y = theobject->bbox.lowerleft.y + theobject->bbox.height; 
   if (level == 0)
      extendschembbox(theinstance, &(bboxin[0]), &(bboxin[1]));
   UTransformbyCTM(DCTM, bboxin, bboxout, 2);

   xm = (bboxout[0].x < bboxout[1].x) ? 0 : 1;  
   ym = (bboxout[0].y < bboxout[1].y) ? 0 : 1;  

   if (bboxout[xm].x < areawin->width && bboxout[ym].y < areawin->height &&
       bboxout[1 - xm].x > 0 && bboxout[1 - ym].y > 0) {       

     /* make parameter substitutions */
     psubstitute(theinstance);

     /* draw all of the elements */
   
     tmpwidth = UTopTransScale(passwidth);
     SetLineAttributes(dpy, areawin->gc, tmpwidth, LineSolid, CapRound,
		JoinBevel);

     /* guard against plist being regenerated during a redraw by the	*/
     /* expression parameter mechanism (should that be prohibited?)	*/

     for (thispart = 0; thispart < theobject->parts; thispart++) {
       areagen = theobject->plist + thispart;
       if ((*areagen)->type & DRAW_HIDE) continue;

       if (defaultcolor != DOFORALL) {
	  Boolean clipcolor = FALSE;
	  switch(ELEMENTTYPE(*areagen)) {
 	     case(POLYGON): case(SPLINE): case(ARC): case(PATH):
	        if (TOPOLY(areagen)->style & CLIPMASK)
		   clipcolor = TRUE;
		break;
	  }
	  if (((*areagen)->color != curcolor) || (clipcolor == TRUE)) {
	     if (clipcolor)
		curcolor = CLIPMASKCOLOR;
	     else if ((*areagen)->color == DEFAULTCOLOR)
		curcolor = defaultcolor;
	     else
		curcolor = (*areagen)->color;

	     XcTopSetForeground(curcolor);
	  }
       }

       switch(ELEMENTTYPE(*areagen)) {
	  case(POLYGON):
	     if (level == 0 || !((TOPOLY(areagen))->style & BBOX))
                UDrawPolygon(TOPOLY(areagen), passwidth);
	     break;
   
	  case(SPLINE):
             UDrawSpline(TOSPLINE(areagen), passwidth);
	     break;
   
	  case(ARC):
             UDrawArc(TOARC(areagen), passwidth);
	     break;

	  case(PATH):
	     UDrawPath(TOPATH(areagen), passwidth);
	     break;

	  case(GRAPHIC):
	     UDrawGraphic(TOGRAPHIC(areagen));
	     break;
   
          case(OBJINST):
	     if (areawin->editinplace && stack && (TOOBJINST(areagen)
			== areawin->topinstance)) {
		/* If stack matches areawin->stack, then don't draw */
		/* because it would be redundant.		 */
		pushlistptr alist = *stack, blist = areawin->stack;
		while (alist && blist) {
		   if (alist->thisinst != blist->thisinst) break;
		   alist = alist->next;
		   blist = blist->next;
		}
		if ((!alist) || (!blist)) break;
	     }
	     if (areawin->clipped > 0) areawin->clipped += 2;
             UDrawObject(TOOBJINST(areagen), level + 1, curcolor, passwidth, stack);
	     if (areawin->clipped > 0) areawin->clipped -= 2;
	     break;
   
  	  case(LABEL): 
	     if (level == 0 || TOLABEL(areagen)->pin == False)
                UDrawString(TOLABEL(areagen), curcolor, theinstance);
	     else if ((TOLABEL(areagen)->anchor & PINVISIBLE) && areawin->pinpointon)
                UDrawString(TOLABEL(areagen), curcolor, theinstance);
	     else if (TOLABEL(areagen)->anchor & PINVISIBLE)
                UDrawStringNoX(TOLABEL(areagen), curcolor, theinstance);
	     else if (level == 1 && TOLABEL(areagen)->pin &&
			TOLABEL(areagen)->pin != INFO && areawin->pinpointon)
		UDrawXDown(TOLABEL(areagen));
	     break;
       }
       if (areawin->clipped > 0) {
	  if ((areawin->clipped & 3) == 1) {
	     areawin->clipped++;
	  }
	  else if ((areawin->clipped & 3) == 2) {
	     areawin->clipped -= 2;
	     if ((!stack) || ((*stack)->clientdata == (char *)NULL)) {
	        XSetClipMask(dpy, areawin->gc, None);
	 	// printf("1: Clear clipmask\n");	// Diagnostic
	     }
	     else {
	        XSetClipMask(dpy, areawin->gc, (Pixmap)((*stack)->clientdata));
	 	// printf("1: Set to pushed clipmask\n");	// Diagnostic
	     }
	  }
       }
     }

     /* restore the color passed to the object, if different from current color */

     if ((defaultcolor != DOFORALL) && (passcolor != curcolor)) {
	XTopSetForeground(passcolor);
     }
     if (areawin->clipped > 0) {
	if ((areawin->clipped & 3) != 3) {
	   if ((!stack) || ((*stack)->clientdata == (char *)NULL)) {
	      XSetClipMask(dpy, areawin->gc, None);
	      // printf("2: Clear clipmask\n");	// Diagnostic
	   }
	   else {
	      XSetClipMask(dpy, areawin->gc, (Pixmap)((*stack)->clientdata));
	      // printf("2: Set to pushed clipmask\n");	// Diagnostic
	   }
	}
	areawin->clipped &= ~3;
     }
   }

   /* restore the selection list (if any) */
   areawin->selects = savesel;
   UPopCTM();
   if (stack) {
      if ((*stack) != NULL) {
	 if ((*stack)->clientdata != (char *)NULL) {
	    XFreePixmap(dpy, areawin->clipmask);
            areawin->clipmask = (Pixmap)(*stack)->clientdata;
	    // printf("3: Restore clipmask\n");	// Diagnostic
	 }
      }
      pop_stack(stack);
   }
}
#endif /* HAVE_CAIRO */

/*----------------------------------------------------------------------*/
/* Recursively run through the current page and find any labels which	*/
/* are declared to be style LATEX.  If "checkonly" is present, we set	*/
/* it to TRUE or FALSE depending on whether or not LATEX labels have	*/
/* been encountered.  If NULL, then we write LATEX output appropriately	*/
/* to a file named with the page filename + suffix ".tex".		*/
/*----------------------------------------------------------------------*/

void UDoLatex(objinstptr theinstance, short level, FILE *f,
	float scale, float scale2, int tx, int ty, Boolean *checkonly)
{
   XPoint	lpos, xlpos;
   XfPoint	xfpos;
   labelptr	thislabel;
   genericptr	*areagen;
   objectptr	theobject = theinstance->thisobject;
   char		*ltext;
   int		lranchor, tbanchor;

   UPushCTM();
   if (level != 0)
       UPreMultCTM(DCTM, theinstance->position, theinstance->scale,
			theinstance->rotation);

   /* make parameter substitutions */
   psubstitute(theinstance);

   /* find all of the elements */
   
   for (areagen = theobject->plist; areagen < theobject->plist +
           theobject->parts; areagen++) {

      switch(ELEMENTTYPE(*areagen)) {
         case(OBJINST):
            UDoLatex(TOOBJINST(areagen), level + 1, f, scale, scale2, tx, ty, checkonly);
	    break;
   
	 case(LABEL): 
	    thislabel = TOLABEL(areagen);
	    if (level == 0 || thislabel->pin == False ||
			(thislabel->anchor & PINVISIBLE))
		if (thislabel->anchor & LATEXLABEL) {
		    if (checkonly) {
		       *checkonly = TRUE;
		       return;
		    }
		    else {
		       lpos.x = thislabel->position.x;
		       lpos.y = thislabel->position.y;
		       UTransformbyCTM(DCTM, &lpos, &xlpos, 1);
		       xlpos.x += tx;
		       xlpos.y += ty;
		       xfpos.x = (float)xlpos.x * scale;
		       xfpos.y = (float)xlpos.y * scale;
		       xfpos.x /= 72.0;
		       xfpos.y /= 72.0;
		       xfpos.x -= 1.0;
		       xfpos.y -= 1.0;
		       xfpos.x += 0.056;
		       xfpos.y += 0.056;
		       xfpos.x /= scale2;
		       xfpos.y /= scale2;
		       ltext = textprinttex(thislabel->string, theinstance);
		       tbanchor = thislabel->anchor & (NOTBOTTOM | TOP);
		       lranchor = thislabel->anchor & (NOTLEFT | RIGHT);

		       /* The 1.2 factor accounts for the difference between	 */
		       /* Xcircuit's label scale of "1" and LaTeX's "normalsize" */

		       fprintf(f, "   \\putbox{%3.2fin}{%3.2fin}{%3.2f}{",
				xfpos.x, xfpos.y, 1.2 * thislabel->scale);
		       if (thislabel->rotation != 0)
			  fprintf(f, "\\rotatebox{-%d}{", thislabel->rotation);
		       if (lranchor == (NOTLEFT | RIGHT)) fprintf(f, "\\rightbox{");
		       else if (lranchor == NOTLEFT) fprintf(f, "\\centbox{");
		       if (tbanchor == (NOTBOTTOM | TOP)) fprintf(f, "\\topbox{");
		       else if (tbanchor == NOTBOTTOM) fprintf(f, "\\midbox{");
		       fprintf(f, "%s", ltext);
		       if (lranchor != NORMAL) fprintf(f, "}");
		       if (tbanchor != NORMAL) fprintf(f, "}");
		       if (thislabel->rotation != 0) fprintf(f, "}");
		       fprintf(f, "}%%\n");
		       free(ltext);
		    }
		}
	    break;
       }
   }
   UPopCTM();
}

/*----------------------------------------------------------------------*/
/*  Top level routine for writing LATEX output.				*/
/*----------------------------------------------------------------------*/

void TopDoLatex()
{
   FILE *f;
   float psscale, outscale;
   int tx, ty, width, height;
   polyptr framebox;
   XPoint origin;
   Boolean checklatex = FALSE;
   char filename[100], extend[10], *dotptr;

   UDoLatex(areawin->topinstance, 0, NULL, 1.0, 1.0, 0, 0, &checklatex);

   if (checklatex == FALSE) return;	/* No LaTeX labels to write */

   /* Handle cases where the file might have a ".eps" extension.	*/
   /* Thanks to Graham Sheward for pointing this out.			*/

   if (xobjs.pagelist[areawin->page]->filename)
      sprintf(filename, "%s", xobjs.pagelist[areawin->page]->filename);
   else
      sprintf(filename, "%s",
		xobjs.pagelist[areawin->page]->pageinst->thisobject->name);

   if ((dotptr = strchr(filename + strlen(filename) - 4, '.')) == NULL) {
       dotptr = filename + strlen(filename);
       sprintf(dotptr, ".ps");
   }
   strcpy(extend, dotptr);
   strcpy(dotptr, ".tex");

   f = fopen(filename, "w");

   *dotptr = '\0';

   fprintf(f, "%% XCircuit output \"%s.tex\" for LaTeX input from %s%s\n",
		filename, filename, extend);
   fprintf(f, "\\def\\putbox#1#2#3#4{\\makebox[0in][l]{\\makebox[#1][l]{}"
		"\\raisebox{\\baselineskip}[0in][0in]"
		"{\\raisebox{#2}[0in][0in]{\\scalebox{#3}{#4}}}}}\n");
   fprintf(f, "\\def\\rightbox#1{\\makebox[0in][r]{#1}}\n");
   fprintf(f, "\\def\\centbox#1{\\makebox[0in]{#1}}\n");
   fprintf(f, "\\def\\topbox#1{\\raisebox{-0.60\\baselineskip}[0in][0in]{#1}}\n");
   fprintf(f, "\\def\\midbox#1{\\raisebox{-0.20\\baselineskip}[0in][0in]{#1}}\n");

   /* Modified to use \scalebox and \parbox by Alex Tercete, June 2008 */

   // fprintf(f, "\\begin{center}\n");

   outscale = xobjs.pagelist[areawin->page]->outscale;
   psscale = getpsscale(outscale, areawin->page);

   width = toplevelwidth(areawin->topinstance, &origin.x);
   height = toplevelheight(areawin->topinstance, &origin.y);

   /* Added 10/19/10:  If there is a specified bounding box, let it	*/
   /* determine the figure origin;  otherwise, the labels will be	*/
   /* mismatched to the bounding box.					*/

   if ((framebox = checkforbbox(topobject)) != NULL) {
      int i, maxx, maxy;

      origin.x = maxx = framebox->points[0].x;
      origin.y = maxy = framebox->points[0].y;
      for (i = 1; i < framebox->number; i++) {
	 if (framebox->points[i].x < origin.x) origin.x = framebox->points[i].x;
	 if (framebox->points[i].x > maxx) maxx = framebox->points[i].x;
	 if (framebox->points[i].y < origin.y) origin.y = framebox->points[i].y;
	 if (framebox->points[i].y > maxy) maxy = framebox->points[i].y;
      }
      origin.x -= ((width - maxx + origin.x) / 2);
      origin.y -= ((height - maxy + origin.y) / 2);
   }

   tx = (int)(72 / psscale) - origin.x,
   ty = (int)(72 / psscale) - origin.y;

   fprintf(f, "   \\scalebox{%g}{\n", outscale);
   fprintf(f, "   \\normalsize\n");
   fprintf(f, "   \\parbox{%gin}{\n", (((float)width * psscale) / 72.0) / outscale);
   fprintf(f, "   \\includegraphics[scale=%g]{%s}\\\\\n", 1.0 / outscale,
    			filename);
   fprintf(f, "   %% translate x=%d y=%d scale %3.2f\n", tx, ty, psscale);

   UPushCTM();		/* Save current state */
   UResetCTM(DCTM);	/* Set to identity matrix */
   UDoLatex(areawin->topinstance, 0, f, psscale, outscale, tx, ty, NULL);
   UPopCTM();		/* Restore state */

   fprintf(f, "   } %% close \'parbox\'\n");
   fprintf(f, "   } %% close \'scalebox\'\n");
   fprintf(f, "   \\vspace{-\\baselineskip} %% this is not"
		" necessary, but looks better\n");
   // fprintf(f, "\\end{center}\n");
   fclose(f);

   Wprintf("Wrote auxiliary file %s.tex", filename);
}

