/*-------------------------------------------------------------------------*/
/* cairo.c --- mainly cairo versions of the UDraw... stuff in functions.c  */
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	   */
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* originally written by Tim Edwards, 8/13/93    			   */
/* All cairo graphics library modifications by Erik van der Wal, May 2014  */
/*-------------------------------------------------------------------------*/

#ifdef HAVE_CAIRO

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <limits.h>

#ifndef XC_WIN32
#include <X11/Intrinsic.h>
#endif

#ifdef TCL_WRAPPER 
#include <tk.h>
#endif /* TCL_WRAPPER */

#include "xcircuit.h"
#include "colordefs.h"
#include "prototypes.h"

#ifdef CAIRO_HAS_FC_FONT
#include <cairo/cairo-ft.h>
#endif /* CAIRO_HAS_FC_FONT */

#ifdef HAVE_GS
#include <ghostscript/ierrors.h>
#include <ghostscript/iapi.h>
#include <ghostscript/gdevdsp.h>
#endif /* HAVE_GS */

extern XCWindowData *areawin;
extern Globaldata xobjs;
extern int number_colors;
extern colorindex *colorlist;
extern short fontcount;
extern fontinfo *fonts;
extern Cursor appcursors[NUM_CURSORS];
extern Display *dpy;
extern gs_state_t gs_state;
extern cairo_surface_t *bbuf;

static cairo_user_data_key_t fontinfo_key;

static void xc_cairo_strokepath(short style, float width);

/*----------------------------------------------------------------------------*/

void xc_cairo_set_matrix(const Matrix *xcm)
{
   cairo_matrix_t m;
   m.xx = xcm->a; m.xy = xcm->b; m.x0 = xcm->c;
   m.yx = xcm->d; m.yy = xcm->e; m.y0 = xcm->f;
   cairo_set_matrix(areawin->cr, &m);
}

/*----------------------------------------------------------------------*/
/* Set the color, based on the given color index                        */
/*----------------------------------------------------------------------*/

void xc_cairo_set_color(int coloridx)
{
   colorindex *xcc;

   if (coloridx >= number_colors) return;

   xcc = &colorlist[coloridx];
   cairo_set_source_rgb(areawin->cr,
		(double)xcc->color.red / 65535.,
		(double)xcc->color.green / 65535.,
		(double)xcc->color.blue / 65535.);
}

/*----------------------------------------------------------------------------*/

void UDrawLine(XPoint *pt1, XPoint *pt2)
{
   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   cairo_save(areawin->cr);
  
   cairo_set_line_width(areawin->cr, xobjs.pagelist[areawin->page]->wirewidth);
   cairo_set_dash(areawin->cr, NULL, 0, 0.);
   cairo_set_line_cap(areawin->cr, CAIRO_LINE_CAP_ROUND);
   cairo_set_line_join(areawin->cr, CAIRO_LINE_JOIN_BEVEL);

   cairo_move_to(areawin->cr, pt1->x, pt1->y);
   cairo_line_to(areawin->cr, pt2->x, pt2->y);
   cairo_stroke(areawin->cr);

   cairo_restore(areawin->cr);
} 

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

void UDrawCircle(XPoint *upt, u_char which)
{
   XPoint wpt;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   cairo_save(areawin->cr);
   cairo_identity_matrix(areawin->cr);

   user_to_window(*upt, &wpt);
   cairo_set_line_width(areawin->cr, .75);
   cairo_set_dash(areawin->cr, NULL, 0, 0.);
   cairo_set_line_cap(areawin->cr, CAIRO_LINE_CAP_BUTT);
   cairo_set_line_join(areawin->cr, CAIRO_LINE_JOIN_MITER);

   /* TODO: angles might be mirrored or turning the wrong way */
   switch(which) {
      case P_POSITION_X:
	 cairo_arc(areawin->cr, wpt.x, wpt.y, 4., M_PI * -.25, M_PI * .25);
	 cairo_arc(areawin->cr, wpt.x, wpt.y, 4., M_PI * .75, M_PI * 1.25);
	 break;
      case P_POSITION_Y:
	 cairo_arc(areawin->cr, wpt.x, wpt.y, 4., M_PI * .25, M_PI * .75);
	 cairo_arc(areawin->cr, wpt.x, wpt.y, 4., M_PI * 1.25, M_PI * 1.75);
	 break;
      default:
	 cairo_arc(areawin->cr, wpt.x, wpt.y, 4., 0., M_PI * 2.);
	 break;
   }

   cairo_restore(areawin->cr);
}

/*----------------------------------------------------------------------*/
/* Add "X" at string origin						*/
/*----------------------------------------------------------------------*/

void UDrawXAt(XPoint *wpt)
{
   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   cairo_save(areawin->cr);
   cairo_identity_matrix(areawin->cr);

   cairo_set_dash(areawin->cr, NULL, 0, 0.);
   cairo_set_line_width(areawin->cr, .75);

   cairo_move_to(areawin->cr, wpt->x - 3., wpt->y - 3.);
   cairo_line_to(areawin->cr, wpt->x + 3., wpt->y + 3.);
   cairo_move_to(areawin->cr, wpt->x + 3., wpt->y - 3.);
   cairo_line_to(areawin->cr, wpt->x - 3., wpt->y + 3.);
   cairo_stroke(areawin->cr);

   cairo_restore(areawin->cr);
}

void UDrawXLine(XPoint opt, XPoint cpt)
{
   XPoint upt, vpt;
   double dashes[] = {4., 4.};

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   cairo_save(areawin->cr);
   cairo_identity_matrix(areawin->cr);

   xc_cairo_set_color(AUXCOLOR);
   cairo_set_dash(areawin->cr, dashes, sizeof(dashes) / sizeof(double), 0.);
   cairo_set_line_width(areawin->cr, .75);

   user_to_window(cpt, &upt);
   user_to_window(opt, &vpt);
   cairo_move_to(areawin->cr, vpt.x, vpt.y);
   cairo_line_to(areawin->cr, upt.x, upt.y);
   cairo_stroke(areawin->cr);

   cairo_set_dash(areawin->cr, NULL, 0, 0.);
   cairo_move_to(areawin->cr, upt.x - 3., upt.y - 3.);
   cairo_line_to(areawin->cr, upt.x + 3., upt.y + 3.);
   cairo_move_to(areawin->cr, upt.x + 3., upt.y - 3.);
   cairo_line_to(areawin->cr, upt.x - 3., upt.y + 3.);
   cairo_stroke(areawin->cr);

   cairo_restore(areawin->cr);
}

/*-------------------------------------------------------------------------*/

void UDrawBox(XPoint origin, XPoint corner)
{
   XPoint worig, wcorn;
   double r, g, b, a;
   
   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   user_to_window(origin, &worig);
   user_to_window(corner, &wcorn);

   cairo_save(areawin->cr);
   cairo_identity_matrix(areawin->cr);

   cairo_set_line_width(areawin->cr, 1.0);
   cairo_set_dash(areawin->cr, NULL, 0, 0.);
   cairo_set_line_cap(areawin->cr, CAIRO_LINE_CAP_SQUARE);
   cairo_set_line_join(areawin->cr, CAIRO_LINE_JOIN_MITER);

   cairo_move_to(areawin->cr, worig.x + .5, worig.y + .5);
   cairo_line_to(areawin->cr, worig.x + .5, wcorn.y + .5);
   cairo_line_to(areawin->cr, wcorn.x + .5, wcorn.y + .5);
   cairo_line_to(areawin->cr, wcorn.x + .5, worig.y + .5);
   cairo_close_path(areawin->cr);

   xc_cairo_set_color(AUXCOLOR);
   cairo_pattern_get_rgba(cairo_get_source(areawin->cr), &r, &g, &b, &a);
   cairo_set_source_rgba(areawin->cr, r, g, b, .1 * a);
   cairo_fill_preserve(areawin->cr);
   cairo_set_source_rgba(areawin->cr, r, g, b, a);
   cairo_stroke(areawin->cr);

   cairo_restore(areawin->cr);
}

/*----------------------------------------------------------------------*/
/* Draw a box indicating the dimensions of the edit element that most	*/
/* closely reach the position "corner".					*/
/*----------------------------------------------------------------------*/

void UDrawRescaleBox(XPoint *corner)
{
   XPoint newpoints[5];

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      /* No return here, since the return value might be needed? */
   }

   if (areawin->selects == 0)
      return;

   if (areawin->redraw_ongoing) {
      int i;
      UGetRescaleBox(corner, newpoints);

      cairo_save(areawin->cr);
      xc_cairo_set_color(AUXCOLOR);
      cairo_set_dash(areawin->cr, NULL, 0, 0.);
      cairo_set_line_cap(areawin->cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join(areawin->cr, CAIRO_LINE_JOIN_BEVEL);
      cairo_move_to(areawin->cr, newpoints[0].x, newpoints[0].y);
      for (i = 1; i < 4; i++)
         cairo_line_to(areawin->cr, newpoints[i].x, newpoints[i].y);
      xc_cairo_strokepath(0, 1);
      cairo_restore(areawin->cr);
   }
}

/*-------------------------------------------------------------------------*/

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

   cairo_save(areawin->cr);
   cairo_identity_matrix(areawin->cr);

   xc_cairo_set_color(BBOXCOLOR);
   cairo_set_line_width(areawin->cr, 1.0);
   cairo_set_dash(areawin->cr, NULL, 0, 0.);
   cairo_set_line_cap(areawin->cr, CAIRO_LINE_CAP_SQUARE);
   cairo_set_line_join(areawin->cr, CAIRO_LINE_JOIN_MITER);

   cairo_move_to(areawin->cr, worig.x + .5, worig.y + .5);
   cairo_line_to(areawin->cr, worig.x + .5, wcorn.y + .5);
   cairo_line_to(areawin->cr, wcorn.x + .5, wcorn.y + .5);
   cairo_line_to(areawin->cr, wcorn.x + .5, worig.y + .5);
   cairo_close_path(areawin->cr);
   cairo_stroke(areawin->cr);

   cairo_restore(areawin->cr);
}

/*----------------------------------------------------------------------*/
/* Main recursive object instance drawing routine.			*/
/*    context is the instance information passed down from above	*/
/*    theinstance is the object instance to be drawn			*/
/*    level is the level of recursion 					*/
/*    passcolor is the inherited color value passed to object		*/
/*    passwidth is the inherited linewidth value passed to the object	*/
/*    stack contains graphics context information			*/
/*----------------------------------------------------------------------*/

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

   /* Stack is not used by cairo but *is* used by expression evaluators */
   if (stack) push_stack((pushlistptr *)stack, theinstance, (char *)NULL);
      
   if (level != 0)
       UPreMultCTM(DCTM, theinstance->position, theinstance->scale,
			theinstance->rotation);

   if (theinstance->style & LINE_INVARIANT)
      passwidth /= fabs((double)theinstance->scale);

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
     cairo_set_line_width(areawin->cr, tmpwidth);
     cairo_set_dash(areawin->cr, NULL, 0, 0.);
     cairo_set_line_cap(areawin->cr, CAIRO_LINE_CAP_ROUND);
     cairo_set_line_join(areawin->cr, CAIRO_LINE_JOIN_BEVEL);

     /* guard against plist being regenerated during a redraw by the	*/
     /* expression parameter mechanism (should that be prohibited?)	*/

     for (thispart = 0; thispart < theobject->parts; thispart++) {
       areagen = theobject->plist + thispart;
       if ((*areagen)->type & DRAW_HIDE) continue;

       if (defaultcolor != DOFORALL) {
	  if ((*areagen)->color != curcolor) {
	     if ((*areagen)->color == DEFAULTCOLOR)
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
             UDrawObject(TOOBJINST(areagen), level + 1, curcolor, passwidth, stack);
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
     }

     /* restore the color passed to the object, if different from current color */

     if ((defaultcolor != DOFORALL) && (passcolor != curcolor)) {
	XTopSetForeground(passcolor);
     }
   }

   /* restore the selection list (if any) */
   areawin->selects = savesel;
   UPopCTM();

   /* Stack is not used by cairo but *is* used by expression evaluators */
   if (stack) pop_stack(stack);
}

/*-------------------------------------------------------------------------*/

static void xc_cairo_strokepath(short style, float width)
{
   if (!(style & CLIPMASK) || (areawin->showclipmasks == TRUE)) {
      if (style & FILLED || (!(style & FILLED) && style & OPAQUE)) {
         if ((style & FILLSOLID) == FILLSOLID)
	    cairo_fill_preserve(areawin->cr);
	 else {
	    double red, green, blue, alpha;
	    cairo_pattern_get_rgba(cairo_get_source(areawin->cr),
		  &red, &green, &blue, &alpha);
            if (!(style & FILLED))
	       cairo_set_source_rgba(areawin->cr, 1., 1., 1., alpha);
	    else {
	       double m = (1 + ((style & FILLSOLID) >> 5)) / 8.;
	       if (style & OPAQUE) {
		  double n = (1. - m); 
		  cairo_set_source_rgba(areawin->cr, m * red + n, 
			m * green + n, m * blue + n, alpha);
	       }
	       else
		  cairo_set_source_rgba(areawin->cr, red, green, blue,
			m * alpha);
	    }
	    cairo_fill_preserve(areawin->cr);
	    cairo_set_source_rgba(areawin->cr, red, green, blue, alpha);
         }
      }
      if (!(style & NOBORDER)) {
	 cairo_set_line_width(areawin->cr, width);
	 cairo_set_line_join(areawin->cr, (style & SQUARECAP) ? 
	       CAIRO_LINE_JOIN_MITER : CAIRO_LINE_JOIN_BEVEL);
         if (style & (DASHED | DOTTED)) {
            double dashes[2];
	    dashes[0] = dashes[1] = 4.0 * width;
	    if (style & DOTTED)
	       dashes[0] = width;
	    cairo_set_dash(areawin->cr, dashes, 2, 0.0);
	    cairo_set_line_width(areawin->cr, width);
	    cairo_set_line_cap(areawin->cr, CAIRO_LINE_CAP_BUTT);
         }
         else {
	    cairo_set_dash(areawin->cr, NULL, 0, 0.0);
	    cairo_set_line_cap(areawin->cr, (style & SQUARECAP) ? 
	          CAIRO_LINE_CAP_SQUARE : CAIRO_LINE_CAP_ROUND);
	 }

         /* draw the spline and close off if so specified */
         if (!(style & UNCLOSED))
	    cairo_close_path(areawin->cr);
	 cairo_stroke_preserve(areawin->cr);
      }
   }
   if (style & CLIPMASK)
      cairo_clip_preserve(areawin->cr);
   cairo_new_path(areawin->cr); /* clear preserved paths */
}

/*-------------------------------------------------------------------------*/

void UDrawPolygon(polyptr thepoly, float passwidth)
{
   int i;
   
   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   if (thepoly->number) {
      cairo_move_to(areawin->cr, thepoly->points[0].x, thepoly->points[0].y);
      for (i = 1; i < thepoly->number; i++)
         cairo_line_to(areawin->cr, thepoly->points[i].x, thepoly->points[i].y);
      xc_cairo_strokepath(thepoly->style, thepoly->width * passwidth);
   }
}

/*-------------------------------------------------------------------------*/

void UDrawArc(arcptr thearc, float passwidth)
{
   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   if (abs(thearc->radius) == thearc->yaxis)
      cairo_arc(areawin->cr, thearc->position.x, thearc->position.y,
	    abs(thearc->radius), thearc->angle1 * M_PI / 180.0,
	    thearc->angle2 * M_PI / 180.0);
   else if (thearc->yaxis) {
      /* perform elliptical arc, as described in cairo manual */
      cairo_save(areawin->cr);
      cairo_translate(areawin->cr, thearc->position.x, thearc->position.y);
      cairo_scale(areawin->cr, abs(thearc->radius), thearc->yaxis);
      cairo_arc(areawin->cr, 0.0, 0.0, 1.0, thearc->angle1 * M_PI / 180.0,
	    thearc->angle2 * M_PI / 180.0);
      cairo_restore(areawin->cr);
   }
   else { /* no y-axis dimension: draw a line */
      /* can we do this in a more elegant manner? */
      double theta;
      double theta_start = thearc->angle1 * RADFAC;
      double theta_stop = thearc->angle2 * RADFAC;
      cairo_move_to(areawin->cr, thearc->position.x + fabs((double)thearc->radius)
	    * cos(theta_start), thearc->position.y);
      for (theta = -M_PI; theta < theta_stop; theta += M_PI) {
         if (theta <= theta_start) continue;
         cairo_line_to(areawin->cr, thearc->position.x 
	    + fabs((double)thearc->radius) * cos(theta), thearc->position.y);
      }
      cairo_line_to(areawin->cr, thearc->position.x + fabs((double)thearc->radius)
	    * cos(theta_stop), thearc->position.y);
   }
   xc_cairo_strokepath(thearc->style, thearc->width * passwidth);
}

/*-------------------------------------------------------------------------*/

void UDrawPath(pathptr thepath, float passwidth)
{
   genericptr	*genpath;
   polyptr	thepoly;
   splineptr	thespline;
   XPoint *xp;
   
   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   /* Draw first point */
   if (thepath->parts) {
      genpath = thepath->plist;
      switch(ELEMENTTYPE(*genpath)) {
	 case POLYGON:
	    thepoly = TOPOLY(genpath);
	    cairo_move_to(areawin->cr, thepoly->points[0].x,
		  thepoly->points[0].y);
	    xp = &thepoly->points[0];
	    break;
	 case SPLINE:
	    thespline = TOSPLINE(genpath);
	    cairo_move_to(areawin->cr, thespline->ctrl[0].x,
		  thespline->ctrl[0].y);
	    xp = &thespline->ctrl[0];
	    break;
      }
   }
   /* Draw all other points */         
   for (genpath = thepath->plist; genpath < thepath->plist + thepath->parts;
	  genpath++) {
      int i;

      switch(ELEMENTTYPE(*genpath)) {
	 case POLYGON:
	    thepoly = TOPOLY(genpath);
	    for (i = 1; i < thepoly->number; i++)
	       cairo_line_to(areawin->cr, thepoly->points[i].x,
		     thepoly->points[i].y);
	    xp = &thepoly->points[thepoly->number - 1];
	    break;
	 case SPLINE:
	    thespline = TOSPLINE(genpath);

	    /* If the curve 1st point is not the same as the last point
	     * drawn, then first draw a line to the first curve point.
	     */

	    if ((thespline->ctrl[0].x != xp->x) || (thespline->ctrl[0].y != xp->y))
	       cairo_line_to(areawin->cr, thespline->ctrl[0].x,
			thespline->ctrl[0].y);

	    cairo_curve_to(areawin->cr, thespline->ctrl[1].x,
		  thespline->ctrl[1].y, thespline->ctrl[2].x,
		  thespline->ctrl[2].y, thespline->ctrl[3].x,
		  thespline->ctrl[3].y);
	    xp = &thespline->ctrl[3];
	    break;
      }
   }
   xc_cairo_strokepath(thepath->style, thepath->width * passwidth);
}

/*-------------------------------------------------------------------------*/

void UDrawSpline(splineptr thespline, float passwidth)
{
   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   cairo_move_to(areawin->cr, thespline->ctrl[0].x, thespline->ctrl[0].y);
   cairo_curve_to(areawin->cr, thespline->ctrl[1].x, thespline->ctrl[1].y,
         thespline->ctrl[2].x, thespline->ctrl[2].y,
	 thespline->ctrl[3].x, thespline->ctrl[3].y);
   xc_cairo_strokepath(thespline->style, thespline->width * passwidth);
}

/*-------------------------------------------------------------------------*/

void UDrawGraphic(graphicptr gp)
{
   double w, h;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   cairo_save(areawin->cr);
   cairo_translate(areawin->cr,
	 gp->position.x, 
	 gp->position.y);
   cairo_rotate(areawin->cr, -gp->rotation * RADFAC);
   cairo_scale(areawin->cr, gp->scale, -gp->scale);
   w = cairo_image_surface_get_width(gp->source);
   h = cairo_image_surface_get_height(gp->source);
   cairo_set_source_surface(areawin->cr, gp->source,
	 -w / 2., -h / 2.);
   cairo_rectangle(areawin->cr, -w / 2, -h / 2, w, h);
   cairo_clip(areawin->cr);
   cairo_paint(areawin->cr);
   cairo_restore(areawin->cr);
}

/*----------------------------*/
/* Draw the grids, axis, etc. */
/*----------------------------*/

void draw_grids(void)
{
   double spc, spc2, spc3;
   cairo_matrix_t m = {1., 0., 0., -1., 0., 0.};
   m.x0 = -areawin->pcorner.x * areawin->vscale;
   m.y0 = areawin->height + areawin->pcorner.y * areawin->vscale;

   if (!areawin->redraw_ongoing) {
      areawin->redraw_needed = True;
      return;
   }

   cairo_save(areawin->cr);

   /* draw lines for grid */
   spc = xobjs.pagelist[areawin->page]->gridspace * areawin->vscale;
   if (areawin->gridon && spc > 8) {
      double x, y;
      int ix, iy;
      /* find bottom-right point on the grid */
      double xbegin = areawin->width;
      double ybegin = areawin->height;
      cairo_set_matrix(areawin->cr, &m);
      cairo_scale(areawin->cr, spc, spc);
      cairo_device_to_user(areawin->cr, &xbegin, &ybegin);
      xbegin = floor(xbegin);
      ybegin = ceil(ybegin);
      ix = xbegin;
      iy = ybegin;
      cairo_user_to_device(areawin->cr, &xbegin, &ybegin);
      cairo_identity_matrix(areawin->cr);
      /* draw the grid */
      xc_cairo_set_color(GRIDCOLOR);
      cairo_set_line_width(areawin->cr, 1.);
      for (x = xbegin; x >= 0.; x -= spc, ix--) {
	 if (!ix && areawin->axeson) continue; /* do not draw main axis */
         cairo_move_to(areawin->cr, floor(x) + .5, .5);
         cairo_line_to(areawin->cr, floor(x) + .5, areawin->height + .5);
      }
      for (y = ybegin; y >= 0.; y -= spc, iy++) {
	 if (!iy && areawin->axeson) continue; /* do not draw main axis */
         cairo_move_to(areawin->cr, .5, floor(y) + .5);
         cairo_line_to(areawin->cr, areawin->width + .5, floor(y) + .5);
      }
      cairo_stroke(areawin->cr);
   }


   if (areawin->axeson) {
      /* find main axis */
      double x = 0, y = 0;
      cairo_set_matrix(areawin->cr, &m);
      cairo_user_to_device(areawin->cr, &x, &y);
      cairo_identity_matrix(areawin->cr);
      /* draw the grid */
      xc_cairo_set_color(AXESCOLOR);
      cairo_set_line_width(areawin->cr, 1.);
      cairo_move_to(areawin->cr, floor(x) + .5, .5);
      cairo_line_to(areawin->cr, floor(x) + .5, areawin->height + .5);
      cairo_move_to(areawin->cr, .5, floor(y) + .5);
      cairo_line_to(areawin->cr, areawin->width + .5, floor(y) + .5);
      cairo_stroke(areawin->cr);
   }

   /* bounding box goes beneath everything except grid/axis lines */
   UDrawBBox();

   /* draw a little red dot at each snap-to point */
   spc2 = xobjs.pagelist[areawin->page]->snapspace * areawin->vscale;
   if (areawin->snapto && spc2 > 8) {
      double x, y;
      /* find bottom-right point on the grid */
      double xbegin = areawin->width;
      double ybegin = areawin->height;
      cairo_set_matrix(areawin->cr, &m);
      cairo_scale(areawin->cr, spc2, spc2);
      cairo_device_to_user(areawin->cr, &xbegin, &ybegin);
      xbegin = floor(xbegin);
      ybegin = ceil(ybegin);
      cairo_user_to_device(areawin->cr, &xbegin, &ybegin);
      cairo_identity_matrix(areawin->cr);
      /* draw the grid */
      xc_cairo_set_color(SNAPCOLOR);
      cairo_set_line_width(areawin->cr, 1.);
      cairo_set_line_cap(areawin->cr, CAIRO_LINE_CAP_ROUND);
      for (x = xbegin; x >= 0.; x -= spc2) {
      	 for (y = ybegin; y >= 0.; y -= spc2) {
            cairo_move_to(areawin->cr, floor(x) + .5, floor(y) + .5);
            cairo_close_path(areawin->cr);
         }
      }
      cairo_stroke(areawin->cr);
   }

   /* Draw major snap points */
   spc3 = spc * 20.;
   if (spc > 4.) {
      double x, y;
      /* find bottom-right point on the grid */
      double xbegin = areawin->width;
      double ybegin = areawin->height;
      cairo_set_matrix(areawin->cr, &m);
      cairo_scale(areawin->cr, spc3, spc3);
      cairo_device_to_user(areawin->cr, &xbegin, &ybegin);
      xbegin = floor(xbegin);
      ybegin = ceil(ybegin);
      cairo_user_to_device(areawin->cr, &xbegin, &ybegin);
      cairo_identity_matrix(areawin->cr);
      /* draw the grid */
      xc_cairo_set_color(GRIDCOLOR);
      cairo_set_line_width(areawin->cr, 3.);
      cairo_set_line_cap(areawin->cr, CAIRO_LINE_CAP_ROUND);
      for (x = xbegin; x >= 0.; x -= spc3) {
      	 for (y = ybegin; y >= 0.; y -= spc3) {
            cairo_move_to(areawin->cr, floor(x) + .5, floor(y) + .5);
            cairo_close_path(areawin->cr);
         }
      }
      cairo_stroke(areawin->cr);
   }
   cairo_restore(areawin->cr);
}

/*---------------------------------------------------------------------*/
/* Draw a string at position offset, starting at the start index and   */
/* ending before the end index. The offset is updated on return.       */
/*---------------------------------------------------------------------*/

void UDrawCharString(u_char *text, int start, int end, XfPoint *offset,
      short styles, short ffont, int groupheight, int passcolor, float tmpscale)
{
   int idx, nglyphs = 0;
   cairo_matrix_t oldm;
   double x, y;
   cairo_glyph_t *glyphs;
   UNUSED(passcolor);

   cairo_get_matrix(areawin->cr, &oldm);
   cairo_scale(areawin->cr, tmpscale, fabs((double)tmpscale));

   /* under- and overlines */
   if (styles & 8)
      y = offset->y / tmpscale - 6.;
   else if (styles & 16)
      y = offset->y / tmpscale + groupheight + 4;
   if (styles & 24) {
      /* under/overline thickness scales if bold */
      float tmpthick = ((fonts[ffont].flags & 0x21) == 0x21) ?  4.0 : 2.0;
      cairo_set_line_width(areawin->cr, tmpthick);

      x = offset->x / tmpscale;
   }

   glyphs = cairo_glyph_allocate(end - start);
   for (idx = start; idx < end; idx++)
   {
      /* Add glyphs */
      glyphs[nglyphs].index = fonts[ffont].glyph_index[text[idx]];
      glyphs[nglyphs].x = offset->x / tmpscale;
      glyphs[nglyphs].y = offset->y / fabs((double)tmpscale);
      nglyphs++;
      /* Determine character width */
      offset->x += fonts[ffont].glyph_advance[text[idx]] * tmpscale;
   }
   if (nglyphs) {
      cairo_show_glyphs(areawin->cr, glyphs, nglyphs);
      cairo_new_path(areawin->cr);
   }
   cairo_glyph_free(glyphs);
   
   if (styles & 24) {
      cairo_move_to(areawin->cr, x, y);
      cairo_line_to(areawin->cr, offset->x / tmpscale, y);
      cairo_stroke(areawin->cr);
   }
   cairo_set_matrix(areawin->cr, &oldm);
}

/*----------------------------------------------------------------------*/
/* A completely stripped down version of UDrawObject, used for stroke	*/
/* font drawing in xc_user_font_render					*/
/*----------------------------------------------------------------------*/

static void xc_draw_glyph_object(objinstptr inst, float passwidth)
{
   int thispart;
   genericptr *areagen;
   objectptr obj = inst->thisobject;
   objinstptr ptr;

   for (thispart = 0; thispart < obj->parts; thispart++) {
      areagen = obj->plist + thispart;
      switch(ELEMENTTYPE(*areagen)) {
	 case(POLYGON): 
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
         case(OBJINST):
	    ptr = TOOBJINST(areagen);
	    cairo_save(areawin->cr);
	    cairo_translate(areawin->cr, ptr->position.x, ptr->position.y);
	    cairo_rotate(areawin->cr, -ptr->rotation * RADFAC);
	    cairo_scale(areawin->cr, ptr->scale, fabs((double)ptr->scale));
	    xc_draw_glyph_object(ptr, passwidth);
	    cairo_restore(areawin->cr);
	    break;
      }
   }
}

/*----------------------------------------------------------------------*/
/* Rendering function for a cairo_user_font_face, to draw the stroke	*/
/* fonts of xcircuit							*/
/* TODO: always this factor 40 to match with cairo. Is it BASELINE?	*/
/*----------------------------------------------------------------------*/

static cairo_status_t xc_user_font_render(cairo_scaled_font_t *scaled_font,
      unsigned long glyph, cairo_t *cr, cairo_text_extents_t *extents)
{
   objectptr chr;
   objinst charinst;
   objinstptr theinstance;
   cairo_t *old_cr;
   float passwidth;
   double llx, lly, trx, try;

   cairo_font_face_t *ff = cairo_scaled_font_get_font_face(scaled_font);
   size_t fontidx = (size_t) cairo_font_face_get_user_data(ff, &fontinfo_key);
   fontinfo *fi = &fonts[fontidx];
	
   chr = fi->encoding[glyph];
   theinstance = &charinst;
   charinst.thisobject = chr;

   llx = chr->bbox.lowerleft.x / 40.;
   lly = chr->bbox.lowerleft.y / 40.;
   trx = (chr->bbox.lowerleft.x + chr->bbox.width) / 40.;
   try = (chr->bbox.lowerleft.y + chr->bbox.height) / 40.;

   /* temporary override areawin->cr with the font context */
   old_cr = areawin->cr;
   areawin->cr = cr;

   cairo_scale(cr, 1. / 40., -1. / 40.);
   cairo_set_line_width(cr, 1);

   /* if font is derived and italic, premultiply by slanting matrix */
   if ((fi->flags & 0x22) == 0x22) {
      cairo_matrix_t m = {1., 0., .25, 1., 0., 0.};
      cairo_transform(areawin->cr, &m);
      llx += .25 * lly;
      trx += .25 * try;
   }
   /* simple boldface technique for derived fonts */
   passwidth = ((fi->flags & 0x21) == 0x21) ?  4. : 2.;

   /* Correct extentswith line width */
   llx -= passwidth / 40.;
   lly -= passwidth / 40.;
   trx += passwidth / 40.;
   try += passwidth / 40.;

   xc_draw_glyph_object(theinstance, passwidth);

   extents->x_bearing = llx;
   extents->y_bearing = -try;
   extents->width = trx - llx;
   extents->height = try - lly;
   extents->x_advance = (chr->bbox.lowerleft.x + chr->bbox.width) / 40.;
   extents->y_advance = 0.;
   
   areawin->cr = old_cr;
   return CAIRO_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------*/
/* Function to translate unicode into a glyph index for the stroke font */
/*----------------------------------------------------------------------*/

static cairo_status_t xc_user_font_glyph(cairo_scaled_font_t *scaled_font,
      unsigned long unicode, unsigned long *glyph_index)
{
   cairo_font_face_t *ff = cairo_scaled_font_get_font_face(scaled_font);
   size_t fontidx = (size_t) cairo_font_face_get_user_data(ff, &fontinfo_key);
   fontinfo *fi = &fonts[fontidx];
   unsigned long idx; 

   /* search all glyphs in the utf8encoding. This is allowed to be slow, */
   /* as the glyph list will be buffered anyway */
   for (idx = 1; idx < 255; idx++) {
      const char *s = fi->utf8encoding[idx];
      int cidx = 0;

      /* Convert utf-8 to unicode */
      unsigned long uc = s[0];
      if (uc & 0x80) {
	 unsigned long mask = 0x3f;
	 while ((s[++cidx] & 0xc0) == 0x80) {
	    uc = (uc << 6) | (s[cidx] & 0x3f);
	    mask = (mask << 5) | 0x1f;
	 }
	 uc &= mask;
      }

      if (unicode == uc) {
	 *glyph_index = idx;
	 return CAIRO_STATUS_SUCCESS;
      }
   }
   /* This should not happen: replace unknown glyph with question mark */
   *glyph_index = '?';
   return CAIRO_STATUS_SUCCESS;
}

/*---------------------------------------------------------------------*/
/* find the corresponing cairo fontface for a given fontinfo structure */
/*---------------------------------------------------------------------*/

typedef struct {
   const char* postscript_name;
   const char* replacement_name;
   const char* foundry_name;
} xc_font_replacement;

static const xc_font_replacement replacement_fonts[] =
{
   /* First try to see if 'real' postscript fonts have been installed */
   {"ITC Avant Garde Gothic",	"ITC Avant Garde Gothic",   "adobe"},
   {"ITC Bookman",		"ITC Bookman",		    "adobe"},
   {"Courier",			"Courier",		    "adobe"},
   {"Helvetica",		"Helvetica",		    "adobe"},
   {"Helvetica Narrow",		"Helvetica Narrow",	    "adobe"},
   {"New Century Schoolbook",	"New Century Schoolbook",   "adobe"},
   {"Palatino",			"Palatino",		    "adobe"},
   {"Symbol",			"Symbol",		    "adobe"},
   {"Times",			"Times",		    "adobe"},
   {"Times-Roman",		"Times-Roman",		    "adobe"},
   {"ITC ZapfChangery",		"ITC ZapfChangery",	    "adobe"},
   {"ITC ZapfDingbats",		"ITC ZapfDingbats",	    "adobe"},
   /* Next try the URW postscript fonts (guaranteed to have same extents) */
   {"ITC Avant Garde Gothic",	"URW Gothic L",		    "urw"},
   {"ITC Bookman",		"URW Bookman L",	    "urw"},
   {"Courier",			"Nimbus Mono L",	    "urw"},
   {"Helvetica",		"Nimbus Sans L",	    "urw"},
   {"Helvetica Narrow",		"Nimbus Sans L Condensed",  "urw"},
   {"New Century Schoolbook",	"Century Schoolbook L",	    "urw"},
   {"Palatino",			"URW Palladio L",	    "urw"},
   {"Symbol",			"Standard Symbols L",	    "urw"},
   {"Times",			"Nimbus Roman No9 L",	    "urw"},
   {"Times-Roman",		"Nimbus Roman No9 L",	    "urw"},
   {"ITC ZapfChangery",		"URW Changery L",	    "urw"},
   {"ITC ZapfDingbats",		"Dingbats",		    "urw"},
   /* No success, use the 'old' stroke fonts */
   {NULL, NULL, NULL}
};

void xc_cairo_set_fontinfo(size_t fontidx)
{
   /* TODO: memory leak. font_face is created here. It should also be */
   /* destroyed again somewhere */
   fontinfo *fi = &fonts[fontidx];
   const char *family = fi->family;
   const xc_font_replacement *replace;
   int c;
   
#ifdef CAIRO_HAS_FC_FONT
   int weight = FC_WEIGHT_NORMAL;
   int slant = FC_SLANT_ROMAN;

   fi->font_face = NULL;
   
   if (fi->flags & 1)
      weight = FC_WEIGHT_BOLD;

   if (fi->flags & 2) {
      if (!strcmp(family, "Helvetica"))
         slant = FC_SLANT_OBLIQUE;
      else
         slant = FC_SLANT_ITALIC;
   }
   /* Try to find a proper postscript font */
   for (replace = replacement_fonts; replace->postscript_name; replace++) {
      if (!strcmp(replace->postscript_name, fi->family)) {
	 FcPattern *matched;
	 FcChar8 *matched_family, *matched_foundry;
	 FcResult result;
	 FcPattern *pattern = FcPatternBuild(NULL,
	       FC_FAMILY, FcTypeString, replace->replacement_name,
	       FC_WEIGHT, FcTypeInteger, weight,
	       FC_SLANT, FcTypeInteger, slant,
	       FC_FOUNDRY, FcTypeString, replace->foundry_name,
	       NULL);
	 FcConfigSubstitute(0, pattern, FcMatchPattern);
	 FcDefaultSubstitute(pattern);
	 matched = FcFontMatch(0, pattern, &result);
	 /* Check if the matched font is actually the replacement font */
	 FcPatternGetString(matched, FC_FAMILY, 0, &matched_family);
	 FcPatternGetString(matched, FC_FOUNDRY, 0, &matched_foundry);
	 if (!strcmp(matched_family, replace->replacement_name)
	       && !strcmp(matched_foundry, replace->foundry_name))
	    fi->font_face = cairo_ft_font_face_create_for_pattern(matched);
	 FcPatternDestroy(matched);
	 FcPatternDestroy(pattern);
	 if (fi->font_face)
	    break;
      }
   }
#else /* CAIRO_HAS_FC_FONT */
   fi->font_face = NULL;
#endif /* CAIRO_HAS_FC_FONT */

   /* Cache the dimensions and all glyphs of the font */
   if (fi->font_face) {
      int num_glyphs;
      cairo_glyph_t *glyphs = NULL;
      cairo_scaled_font_t *scaled_font;
      cairo_text_extents_t extents;

      cairo_save(areawin->cr);
      cairo_identity_matrix(areawin->cr);
      cairo_set_font_face(areawin->cr, fi->font_face);
      cairo_set_font_size(areawin->cr, 100.);
      scaled_font = cairo_get_scaled_font(areawin->cr);
      for (c = 1; c < 256; c++) { /* skip encoding at index 0 */
	 cairo_scaled_font_text_to_glyphs(scaled_font, 0., 0.,
	       fi->utf8encoding[c], -1, &glyphs, &num_glyphs,
	       NULL, NULL, NULL);
	 fi->glyph_index[c] = glyphs[0].index;
	 cairo_scaled_font_glyph_extents(scaled_font, glyphs, 1, &extents);
	 fi->glyph_top[c] = -extents.y_bearing * 40. / 100.;
	 /* fi->glyph_bottom[c] = extents.height * 40. / 100. + fi->glyph_top[c]; */
	 fi->glyph_bottom[c] = fi->glyph_top[c] - extents.height * 40. / 100.;
	 fi->glyph_advance[c] = extents.x_advance * 40. / 100.;
      }
      cairo_glyph_free(glyphs);
      cairo_restore(areawin->cr);
   }
   else {
      /* No decent postscript font found. Backup using stroke fonts */
      fi->font_face = cairo_user_font_face_create();
      cairo_font_face_set_user_data(fi->font_face, &fontinfo_key,
	    (void*) fontidx, (cairo_destroy_func_t) cairo_font_face_destroy);
      cairo_user_font_face_set_render_glyph_func(fi->font_face,
	    xc_user_font_render);
      cairo_user_font_face_set_unicode_to_glyph_func(fi->font_face,
	    xc_user_font_glyph);
      for (c = 0; c < 256; c++) {
	 objectptr chr = fi->encoding[c];
	 fi->glyph_index[c] = c;
	 fi->glyph_top[c] = chr->bbox.lowerleft.y + chr->bbox.height;
	 fi->glyph_bottom[c] = chr->bbox.lowerleft.y;
	 fi->glyph_advance[c] = chr->bbox.lowerleft.x + chr->bbox.width;
      }
   }
}

/*----------------------------------------------------------------------*/
/* A light wrapper around cairo_surface_t, to a generalized xcImage     */
/*----------------------------------------------------------------------*/

/* caching for cairo_surface_t */
static xcImage *xcImagePixel_oldimg = NULL;
static uint32_t *xcImagePixel_data;
static int xcImagePixel_width;
static int xcImagePixel_height;

static inline void xcImageCheckCache(xcImage *img)
{
   if (img != xcImagePixel_oldimg) {
      xcImagePixel_oldimg = img;
      xcImagePixel_data = (uint32_t*) cairo_image_surface_get_data(img);
      xcImagePixel_width = cairo_image_surface_get_width(img);
      xcImagePixel_height = cairo_image_surface_get_height(img);
   }
}

xcImage *xcImageCreate(int width, int height)
{
   return cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
}

void xcImageDestroy(xcImage *img)
{
   cairo_surface_destroy(img);
}

int xcImageGetWidth(xcImage *img)
{
   xcImageCheckCache(img);
   return xcImagePixel_width;
}

int xcImageGetHeight(xcImage *img)
{
   xcImageCheckCache(img);
   return xcImagePixel_height;
}

void xcImagePutPixel(xcImage *img, int x, int y, u_char r, u_char g, u_char b)
{
   xcImageCheckCache(img);
   xcImagePixel_data[y * xcImagePixel_width + x] = (r << 16) | (g << 8) | b;
}

void xcImageGetPixel(xcImage *img, int x, int y, u_char *r, u_char *g,
      u_char *b)
{
   uint32_t argb;
   xcImageCheckCache(img);
   argb = xcImagePixel_data[y * xcImagePixel_width + x];
   *r = argb >> 16;
   *g = argb >> 8;
   *b = argb;
}


/*------------------------------------------------------------------------*/
/* Ghostscript rendering function. In contrast to X11, direct calls to    */
/* ghostscriptapi are made						  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* gsapi I/O redirection functions                                        */
/*------------------------------------------------------------------------*/

#ifdef HAVE_GS
static int GSDLLCALL gs_stdin_fn(void *caller_handle, char *buf, int len)
{
   UNUSED(caller_handle); UNUSED(buf); UNUSED(len);

   return 0; /* EOF */
}

static int GSDLLCALL gs_stdout_fn(void *caller_handle, const char *str, int len)
{
#ifndef GS_DEBUG
   UNUSED(caller_handle); UNUSED(str);
#else
   fwrite(str, 1, len, stdout);
   fflush(stdout);
#endif
   return len;
}

static int GSDLLCALL gs_stderr_fn(void *caller_handle, const char *str, int len)
{
#ifndef GS_DEBUG
   UNUSED(caller_handle); UNUSED(str);
#else
   fwrite(str, 1, len, stderr);
   fflush(stderr);
#endif
   return len;
}
#endif /* HAVE_GS */

/*------------------------------------------------------------------------*/
/* gsapi displaycallback functions                                        */
/*------------------------------------------------------------------------*/

#ifdef HAVE_GS
static unsigned char *gs_pimage;
static int gs_width, gs_height, gs_raster;

static int gs_display_dummy(void)
{
   return 0;
}
#endif /* HAVE_GS */

#ifdef HAVE_GS
static int gs_display_size(void *handle, void *device, int width, int height, 
      int raster, unsigned int format, unsigned char *pimage)
{
   UNUSED(handle); UNUSED(device); UNUSED(format);

   gs_pimage = pimage;
   gs_width = width;
   gs_height = height;
   gs_raster = raster;
   return 0;
}
#endif /* HAVE_GS */

#ifdef HAVE_GS
int gs_display_page(void *handle, void *device, int copies, int flush)
{
   cairo_surface_t *tbuf;
   cairo_t *bbuf_cr;
   UNUSED(handle); UNUSED(device); UNUSED(copies); UNUSED(flush);

   if (bbuf)
      cairo_surface_destroy(bbuf);
   /* Since cairo_image_surface_create_for_data assumes the buffer to be */
   /* valid until destruction, and ghostscript frees it immediately,     */
   /* first use a temporary buffer. Then immediately copy the buffer	 */
   /* To the final location						 */
   tbuf = cairo_image_surface_create_for_data(gs_pimage, CAIRO_FORMAT_RGB24,
         gs_width, gs_height, gs_raster);
   bbuf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, gs_width, gs_height);
   bbuf_cr = cairo_create(bbuf);
   cairo_set_source_surface(bbuf_cr, tbuf, 0., 0.);
   cairo_paint(bbuf_cr);
   cairo_destroy(bbuf_cr);
   cairo_surface_destroy(tbuf);
   return 0;
}
#endif /* HAVE_GS */

#ifdef HAVE_GS
display_callback gs_display = {
    sizeof(display_callback),
    DISPLAY_VERSION_MAJOR,
    DISPLAY_VERSION_MINOR,
    (int (*)(void*, void*)) gs_display_dummy, /* display_open */
    (int (*)(void*, void*)) gs_display_dummy, /* display_preclose */
    (int (*)(void*, void*)) gs_display_dummy, /* display_close */
    (int (*)(void*, void*, int, int, int, unsigned int)) gs_display_dummy,
					      /* display_presize */
    gs_display_size,
    (int (*)(void*, void*)) gs_display_dummy, /* display_sync */
    gs_display_page,
    NULL, /* display_update */
    NULL, /* display_memalloc */
    NULL, /* display_memfree */
    NULL /* display_separation */
};
#endif /* HAVE_GS */

/*------------------------------------------------------*/
/* write scale and position to ghostscript              */
/* and tell ghostscript to run the requested file       */
/*------------------------------------------------------*/

#ifdef HAVE_GS
const char *gs_argv[] = {
      "-dQUIET",			/* Suppress startup messages	*/
      "-dNOPAUSE",			/* Disable pause at end of page	*/
      "-dBATCH",			/* Exit when all files done	*/ 
      "-sDEVICE=display",
      "-sDisplayHandle=0",
      "-r75",				/* Display resolution 		*/
      "-dGraphicsAlphaBits=4",		/* Graphics anti-aliasing	*/
      "-dTextAlphaBits=4"		/* Text anti-aliasing		*/
};
#endif /* HAVE_GS */

void write_scale_position_and_run_gs(float norm, float xpos, float ypos,
      const char *bgfile)
{
#ifndef HAVE_GS
   UNUSED(norm); UNUSED(xpos); UNUSED(ypos); UNUSED(bgfile);
#endif
#ifdef HAVE_GS
   int i, code, exit_code;
   void *instance;
   char gs_cmd[256];
   char display_format[] = "-dDisplayFormat=........";
   char pixmap_size[] = "-g........x........";
   int argc = sizeof(gs_argv) / sizeof(gs_argv[0]);
   const char **argv = (const char**) malloc((argc + 2) * sizeof(const char*));

   for (i = 0; i < argc; i++)
      argv[i] = gs_argv[i];
   argv[argc++] = display_format;
   argv[argc++] = pixmap_size;

   sprintf(gs_cmd,
	 " /GSobj save def"
   	 " /setpagedevice {pop} def"
   	 " gsave"
   	 " %3.2f %3.2f translate"
   	 " %3.2f %3.2f scale"
   	 " (%s) run"
   	 " GSobj restore"
	 " grestore",
	 xpos, ypos, norm, norm, bgfile);

   sprintf(display_format, "-dDisplayFormat=%d", DISPLAY_COLORS_RGB
          | DISPLAY_UNUSED_LAST | DISPLAY_DEPTH_8 | DISPLAY_LITTLEENDIAN
          | DISPLAY_TOPFIRST | DISPLAY_ROW_ALIGN_DEFAULT);
   sprintf(pixmap_size, "-g%dx%d", areawin->width, areawin->height);

   XDefineCursor(dpy, areawin->window, WAITFOR);

   if ((code = gsapi_new_instance(&instance, NULL)) == 0) {
      gsapi_set_stdio(instance, gs_stdin_fn, gs_stdout_fn, gs_stderr_fn);
      gsapi_set_display_callback(instance, &gs_display);
      if (!(code = gsapi_init_with_args(instance, argc, (char**) argv)))
         gsapi_run_string(instance, gs_cmd, 0, &exit_code);
      gsapi_exit(instance);
      gsapi_delete_instance(instance);
   }
   free(argv);
#ifdef GS_DEBUG
   fprintf(stdout, "Xcircuit: ghostscript done\n");
#endif

   XDefineCursor(dpy, areawin->window, DEFAULTCURSOR);
   areawin->lastbackground = xobjs.pagelist[areawin->page]->background.name;
   drawarea(areawin->area, NULL, NULL);
#endif /* HAVE_GS */
   gs_state = GS_READY;
}

#else /* HAVE_CAIRO */
typedef int no_empty_translation_unit_warning;
#endif /* HAVE_CAIRO */

