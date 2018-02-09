/*----------------------------------------------------------------------*/
/* svg.c 								*/
/* Copyright (c) 2009  Tim Edwards, Open Circuit Design			*/
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifndef XC_WIN32
#include <unistd.h>
#endif
#include <math.h>
#include <limits.h>
#include <sys/stat.h>
#ifndef XC_WIN32
#include <sys/wait.h>
#else
#include <process.h>
#endif

#ifndef _MSC_VER
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif

#ifndef HAVE_VFORK
#define vfork fork
#endif

/*----------------------------------------------------------------------*/
/* Local includes							*/
/*----------------------------------------------------------------------*/

#ifdef TCL_WRAPPER 
#include <tk.h>
#endif

#include "colordefs.h"
#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"

void SVGDrawString(labelptr, int, objinstptr);

/*----------------------------------------------------------------------*/
/* External Variable definitions					*/
/*----------------------------------------------------------------------*/

extern Display *dpy;
extern Pixmap STIPPLE[8];
extern XCWindowData *areawin;
extern Globaldata xobjs;
extern colorindex *colorlist;
extern int number_colors;
extern fontinfo *fonts;
extern short fontcount;

/*----------------------------------------------------------------------*/
/* The output file is a global variable used by all routines.		*/
/*----------------------------------------------------------------------*/

FILE *svgf;

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/

void svg_printcolor(int passcolor, char *prefix)
{
   if ((passcolor < number_colors) && (passcolor != DEFAULTCOLOR)) {
      fprintf(svgf, "%s\"#%02x%02x%02x\" ",
		prefix,
		(colorlist[passcolor].color.red >> 8),
		(colorlist[passcolor].color.green >> 8),
		(colorlist[passcolor].color.blue >> 8));
   }
}

/*----------------------------------------------------------------------*/
/* Since we can't do stipples in SVG, and since the whole stipple thing	*/
/* was only put in because we can't (easily) do transparency effects	*/
/* in X11, we convert stipples to transparency when the stipple is a	*/
/* mask, and do a color blend with white when the stipple is opaque.	*/
/*									*/
/* Blend amount is 1 (almost original color) to 7 (almost white)	*/
/*----------------------------------------------------------------------*/

void svg_blendcolor(int passcolor, char *prefix, int amount)
{
   int i, bred, bgreen, bblue;

   if (passcolor != DEFAULTCOLOR) {
      for (i = 0; i < number_colors; i++) {
         if (colorlist[i].color.pixel == passcolor) break;
      }
      if (i < number_colors) {
	 bred = colorlist[i].color.red >> 8;
	 bgreen = colorlist[i].color.green >> 8;
	 bblue = colorlist[i].color.blue >> 8;
      }
   }
   else {
	 bred = bgreen = bblue = 0;
   }
   bred = ((bred * amount) + (255 * (8 - amount))) >> 3;
   bgreen = ((bgreen * amount) + (255 * (8 - amount))) >> 3;
   bblue = ((bblue * amount) + (255 * (8 - amount))) >> 3;

   fprintf(svgf, "%s\"#%02x%02x%02x\" ", prefix, bred, bgreen, bblue);
}

/*----------------------------------------------------------------------*/
/* Fill and/or draw a border around an element				*/
/*----------------------------------------------------------------------*/

void svg_stroke(int passcolor, short style, float width)
{
   float        tmpwidth;
   short	minwidth, solidpart, shade;

   tmpwidth = UTopTransScale(xobjs.pagelist[areawin->page]->wirewidth * width);
   minwidth = max(1, (short)tmpwidth);

   if (style & FILLED || (!(style & FILLED) && style & OPAQUE)) {
      if ((style & FILLSOLID) == FILLSOLID) {
         svg_printcolor(passcolor, "fill=");
      }
      else if (!(style & FILLED)) {
	 fprintf(svgf, "fill=\"white\" ");
      }
      else {
	 shade = 1 + ((style & FILLSOLID) >> 5);
  	 if (style & OPAQUE) {
            svg_blendcolor(passcolor, "fill=", shade);
	 }
  	 else {
            svg_printcolor(passcolor, "fill=");
            fprintf(svgf, "fill-opacity=\"%g\" ", (float)shade / 8);
	 }
      }
   }
   else
      fprintf(svgf, "fill=\"none\" ");

   if (!(style & NOBORDER)) {
      /* set up dots or dashes */
      if (style & DASHED) solidpart = 4 * minwidth;
      else if (style & DOTTED) solidpart = minwidth;
      if (style & (DASHED | DOTTED)) {
	 fprintf(svgf, "style=\"stroke-dasharray:%d,%d\" ", solidpart, 4 * minwidth);

	 fprintf(svgf, "stroke-width=\"%g\" ", tmpwidth);
	 fprintf(svgf, "stroke-linecap=\"butt\" ");
	 if (style & SQUARECAP)
	    fprintf(svgf, "stroke-linejoin=\"miter\" ");
	 else
	    fprintf(svgf, "stroke-linejoin=\"bevel\" ");
      }
      else {
	 fprintf(svgf, "stroke-width=\"%g\" ", tmpwidth);
	 if (style & SQUARECAP) {
	    fprintf(svgf, "stroke-linejoin=\"miter\" ");
	    fprintf(svgf, "stroke-linecap=\"projecting\" ");
	 }
	 else {
	    fprintf(svgf, "stroke-linejoin=\"bevel\" ");
	    fprintf(svgf, "stroke-linecap=\"round\" ");
	 }
      }
      svg_printcolor(passcolor, "stroke=");
   }
   else
      fprintf(svgf, "stroke=\"none\" ");
   fprintf(svgf, "/>\n");
}

/*----------------------------------------------------------------------*/
/* Finish a path and fill and/or stroke					*/
/*----------------------------------------------------------------------*/

void svg_strokepath(int passcolor, short style, float width)
{
   /* Finish the path, closing if necessary */
   if (!(style & UNCLOSED))
      fprintf(svgf, "z\" ");
   else
      fprintf(svgf, "\" ");

   svg_stroke(passcolor, style, width);
}

/*-------------------------------------------------------------------------*/

void SVGCreateImages(int page)
{
    Imagedata *img;
    int i, x, y;
    short *glist;
    FILE *ppf;
    union {
       u_char b[4];
       u_long i;
    } pixel;
    char *fname, outname[128], *pptr;
#ifndef XC_WIN32
    pid_t pid;
#endif

    /* Check which images are used on this page */
    glist = (short *)malloc(xobjs.images * sizeof(short));
    for (i = 0; i < xobjs.images; i++) glist[i] = 0;
    count_graphics(xobjs.pagelist[page]->pageinst->thisobject, glist);

    for (i = 0; i < xobjs.images; i++) {
       if (glist[i] == 0) continue;
       img = xobjs.imagelist + i;

       /* Generate a PPM file, then convert it to PNG */

       fname = tmpnam(NULL);
       ppf = fopen(fname, "w");
       if (ppf != NULL) {
    	  int width, height;
	  width = xcImageGetWidth(img->image);
	  height = xcImageGetWidth(img->image);
	  fprintf(ppf, "P6 %d %d 255\n", width, height);
	  for (y = 0; y < height; y++) {
	     for (x = 0; x < width; x++) {
		u_char r, g, b;
		xcImageGetPixel(img->image, x, y, &r, &g, &b);
		fwrite(&r, 1, 1, ppf);
		fwrite(&g, 1, 1, ppf);
		fwrite(&b, 1, 1, ppf);
	     }
	  }
       }
       fclose(ppf);

       /* Run "convert" to make this into a png file */

       strcpy(outname, img->filename);
       if ((pptr = strrchr(outname, '.')) != NULL)
	  strcpy(pptr, ".png");
       else
	  strcat(outname, ".png");

#ifndef XC_WIN32
       if ((pid = vfork()) == 0) {
	  execlp("convert", "convert", fname, outname, NULL);
	  exit(0);  	/* not reached */
       }
       waitpid(pid, NULL, 0);
       unlink(fname);
#else
       _spawnl(_P_WAIT, GM_EXEC, GM_EXEC, "convert", fname, outname, NULL);
       _unlink(fname);
#endif
       Fprintf(stdout, "Generated standalone PNG image file %s\n", outname);
    }
    free(glist);
}

/*-------------------------------------------------------------------------*/

void SVGDrawGraphic(graphicptr gp)
{
    XPoint ppt, corner;
    Imagedata *img;
    int i;
    char outname[128], *pptr;
    float tscale;
    float rotation;
    int width = xcImageGetWidth(gp->source);
    int height = xcImageGetHeight(gp->source);

    for (i = 0; i < xobjs.images; i++) {
       img = xobjs.imagelist + i;
       if (img->image == gp->source)
	  break;
    }
    if (i == xobjs.images) return;

    strcpy(outname, img->filename);
    if ((pptr = strrchr(outname, '.')) != NULL)
       strcpy(pptr, ".png");
    else
       strcat(outname, ".png");

    UPushCTM();
    UPreMultCTM(DCTM, gp->position, gp->scale, gp->rotation);
    corner.x = -(width >> 1);
    corner.y = (height >> 1);
    UTransformbyCTM(DCTM, &corner, &ppt, 1);
    UPopCTM();

    tscale = gp->scale * UTopScale();
    rotation = gp->rotation + UTopRotation();
    if (rotation >= 360.0) rotation -= 360.0;
    else if (rotation < 0.0) rotation += 360.0;
    
    fprintf(svgf, "<image transform=\"translate(%d,%d) scale(%g) rotate(%f)\"\n",
			ppt.x, ppt.y, tscale, rotation);
    fprintf(svgf, "  width=\"%dpx\" height=\"%dpx\"", width, height);
    fprintf(svgf, " xlink:href=\"%s\">\n", outname);
    fprintf(svgf, "</image>\n");
}

/*-------------------------------------------------------------------------*/

void SVGDrawSpline(splineptr thespline, int passcolor)
{
   XPoint       tmppoints[4];

   UTransformbyCTM(DCTM, thespline->ctrl, tmppoints, 4);

   fprintf(svgf, "<path d=\"M%d,%d C%d,%d %d,%d %d,%d ",
		tmppoints[0].x, tmppoints[0].y,
		tmppoints[1].x, tmppoints[1].y,
		tmppoints[2].x, tmppoints[2].y,
		tmppoints[3].x, tmppoints[3].y);
   svg_strokepath(passcolor, thespline->style, thespline->width);
}

/*-------------------------------------------------------------------------*/

void SVGDrawPolygon(polyptr thepoly, int passcolor)
{
   int i;
   XPoint *tmppoints = (pointlist) malloc(thepoly->number * sizeof(XPoint));

   UTransformbyCTM(DCTM, thepoly->points, tmppoints, thepoly->number);
   
   fprintf(svgf, "<path ");
   if (thepoly->style & BBOX) fprintf(svgf, "visibility=\"hidden\" ");
   fprintf(svgf, "d=\"M%d,%d L", tmppoints[0].x, tmppoints[0].y);
   for (i = 1; i < thepoly->number; i++) {
      fprintf(svgf, "%d,%d ", tmppoints[i].x, tmppoints[i].y);
   }

   svg_strokepath(passcolor, thepoly->style, thepoly->width);
   free(tmppoints);
}

/*-------------------------------------------------------------------------*/

void SVGDrawArc(arcptr thearc, int passcolor)
{
   XPoint  endpoints[2];
   int	   radius[2];
   int	   tarc;

   radius[0] = UTopTransScale(thearc->radius);
   radius[1] = UTopTransScale(thearc->yaxis);

   tarc = (thearc->angle2 - thearc->angle1);
   if (tarc == 360) {
      UTransformbyCTM(DCTM, &(thearc->position), endpoints, 1);
      fprintf(svgf, "<ellipse cx=\"%d\" cy=\"%d\" rx=\"%d\" ry=\"%d\" ",
		endpoints[0].x, endpoints[0].y, radius[0], radius[1]);
      svg_stroke(passcolor, thearc->style, thearc->width);
   }
   else {
      UfTransformbyCTM(DCTM, thearc->points, endpoints, 1);
      UfTransformbyCTM(DCTM, thearc->points + thearc->number - 1, endpoints + 1, 1);

      /* When any arc is flipped, the direction of travel reverses. */
      fprintf(svgf, "<path d=\"M%d,%d A%d,%d 0 %d,%d %d,%d ",
		endpoints[0].x, endpoints[0].y,
		radius[0], radius[1],
		((tarc > 180) ? 1 : 0),
		(((DCTM->a * DCTM->e) >= 0) ? 1 : 0),
		endpoints[1].x, endpoints[1].y);
      svg_strokepath(passcolor, thearc->style, thearc->width);
   }
}

/*-------------------------------------------------------------------------*/

void SVGDrawPath(pathptr thepath, int passcolor)
{
   XPoint	*tmppoints = (pointlist) malloc(sizeof(XPoint));
   genericptr	*genpath;
   polyptr	thepoly;
   splineptr	thespline;
   int		i, firstpt = 1;
   
   fprintf(svgf, "<path d=\"");

   for (genpath = thepath->plist; genpath < thepath->plist + thepath->parts;
	  genpath++) {
      switch(ELEMENTTYPE(*genpath)) {
	 case POLYGON:
	    thepoly = TOPOLY(genpath);
	    tmppoints = (pointlist) realloc(tmppoints, thepoly->number * sizeof(XPoint));
   	    UTransformbyCTM(DCTM, thepoly->points, tmppoints, thepoly->number);
	    if (firstpt) {
	       fprintf(svgf, "M%d,%d ", tmppoints[0].x, tmppoints[0].y);
	       firstpt = 0;
	    }
	    fprintf(svgf, "L");
   	    for (i = 1; i < thepoly->number; i++) {
	       fprintf(svgf, "%d,%d ", tmppoints[i].x, tmppoints[i].y);
	    }
	    break;
	 case SPLINE:
	    thespline = TOSPLINE(genpath);
	    tmppoints = (pointlist) realloc(tmppoints, 4 * sizeof(XPoint));
	    UTransformbyCTM(DCTM, thespline->ctrl, tmppoints, 4);
	    if (firstpt) {
	       fprintf(svgf, "M%d,%d ", tmppoints[0].x, tmppoints[0].y);
	       firstpt = 0;
	    }
	    fprintf(svgf, "C%d,%d %d,%d %d,%d ",
		tmppoints[1].x, tmppoints[1].y,
		tmppoints[2].x, tmppoints[2].y,
		tmppoints[3].x, tmppoints[3].y);
	    break;
      }
   } 
   svg_strokepath(passcolor, thepath->style, thepath->width);
   free(tmppoints);
}

/*----------------------------------------------------------------------*/
/* Main recursive object instance drawing routine.			*/
/*    context is the instance information passed down from above	*/
/*    theinstance is the object instance to be drawn			*/
/*    level is the level of recursion 					*/
/*    passcolor is the inherited color value passed to object		*/
/*----------------------------------------------------------------------*/

void SVGDrawObject(objinstptr theinstance, short level, int passcolor, pushlistptr *stack)
{
   genericptr	*areagen;
   float	tmpwidth;
   int		defaultcolor = passcolor;
   int		curcolor = passcolor;
   int		thispart;
   objectptr	theobject = theinstance->thisobject;

   /* All parts are given in the coordinate system of the object, unless */
   /* this is the top-level object, in which they will be interpreted as */
   /* relative to the screen.						 */

   UPushCTM();

   if (stack) push_stack(stack, theinstance, NULL);
   if (level != 0)
       UPreMultCTM(DCTM, theinstance->position, theinstance->scale,
			theinstance->rotation);

   /* make parameter substitutions */
   psubstitute(theinstance);

   /* draw all of the elements */
   
   tmpwidth = UTopTransScale(xobjs.pagelist[areawin->page]->wirewidth);

   /* Here---set a default style using "g" like PostScript "gsave"	*/
   /* stroke-width = tmpwidth, stroke = passcolor 			*/

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
	 }
      }

      switch(ELEMENTTYPE(*areagen)) {
	 case(POLYGON):
	    if (level == 0 || !((TOPOLY(areagen))->style & BBOX))
               SVGDrawPolygon(TOPOLY(areagen), curcolor);
	    break;
   
	 case(SPLINE):
            SVGDrawSpline(TOSPLINE(areagen), curcolor);
	    break;
   
	 case(ARC):
            SVGDrawArc(TOARC(areagen), curcolor);
	    break;

	 case(PATH):
	    SVGDrawPath(TOPATH(areagen), curcolor);
	    break;

	 case(GRAPHIC):
	    SVGDrawGraphic(TOGRAPHIC(areagen));
	    break;
   
         case(OBJINST):
	    if (areawin->editinplace && stack && (TOOBJINST(areagen)
			== areawin->topinstance)) {
	       /* If stack matches areawin->stack, then don't	*/
	       /* draw because it would be redundant.		 */
	       pushlistptr alist = *stack, blist = areawin->stack;
	       while (alist && blist) {
		  if (alist->thisinst != blist->thisinst) break;
		  alist = alist->next;
		  blist = blist->next;
	       }
	       if ((!alist) || (!blist)) break;
	    }
            SVGDrawObject(TOOBJINST(areagen), level + 1, curcolor, stack);
	    break;
   
  	  case(LABEL): 
	    if (level == 0 || TOLABEL(areagen)->pin == False ||
			(TOLABEL(areagen)->anchor & PINVISIBLE))
            SVGDrawString(TOLABEL(areagen), curcolor, theinstance);
	    break;
      }
   }

   UPopCTM();
   if (stack) pop_stack(stack);
}

/*----------------------------------------------------------------------*/

#define addlinepoint(pointlist, numvals, xval, yval) \
{  if (!numvals) pointlist = (XPoint *)malloc(sizeof(XPoint)); \
   else pointlist = (XPoint *)realloc(pointlist, (numvals + 1) * sizeof(XPoint)); \
   pointlist[numvals].x = xval; \
   pointlist[numvals++].y = -yval; \
}

/*----------------------------------------------------------------------*/
/* Draw an entire string, including parameter substitutions		*/
/*----------------------------------------------------------------------*/

void SVGDrawString(labelptr drawlabel, int passcolor, objinstptr localinst)
{
   stringpart *strptr;
   char *textptr;
   short  fstyle, ffont, tmpanchor, baseline, deltay;
   int    pos, defaultcolor, curcolor, scolor;
   short  oldx, oldfont, oldstyle;
   int olinerise = 4;
   float  tmpscale = 1.0, natscale = 1.0;
   XPoint newpoint;
   TextExtents tmpext;
   TextLinesInfo tlinfo;
   short *tabstops = NULL;
   short tabno, numtabs = 0, group = 0;
   short linenum = 0;
   int open_text, open_span, open_decor;
   XPoint *decorations = NULL;
   short nvals = 0;

   char *symbol_html_encoding[] = {
	" ", "!", "&#8704;", "#", "&#8707;", "%", "&", "?", "(", ")",
	"*", "+", ",", "&#8722;", ".", "/", "0", "1", "2", "3", "4",
	"5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "&#8773;",
	"&#913;", "&#914;", "&#935;", "&#916;", "&#917;", "&#934;",
	"&#915;", "&#919;", "&#921;", "&#977;", "&#922;", "&#923;",
	"&#924;", "&#925;", "&#927;", "&#928;", "&#920;", "&#929;",
	"&#931;", "&#932;", "&#933;", "&#963;", "&#937;", "&#926;",
	"&#936;", "&#918;", "[", "&#8756;", "]", "&#8869;", "_",
	"&#8254;", "&#945;", "&#946;", "&#967;", "&#948;", "&#949;",
	"&#966;", "&#947;", "&#951;", "&#953;", "&#966;", "&#954;",
	"&#955;", "&#956;", "&#957;", "&#959;", "&#960;", "&#952;",
	"&#961;", "&#963;", "&#964;", "&#965;", "&#969;", "&#969;",
	"&#958;", "&#968;", "&#950;", "{", "|", "}", "~", "", "", "",
	"", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	"&#978;", "&#8242;", "&#8804;", "&#8260;", "&#8734;", "&#402;",
	"&#9827;", "&#9830;", "&#9829;", "&#9824;", "&#8596;",
	"&#8592;", "&#8593;", "&#8594;", "&#8595;", "&#176;", "&#177;",
	"&#8243;", "&#8805;", "&#215;", "&#8733;", "&#8706;", "&#8226;",
	"&#247;", "&#8800;", "&#8801;", "&#8773;", "&#8230;"
   };

   /* Standard encoding vector, in HTML, from character 161 to 255 */
   u_int standard_html_encoding[] = {
	161, 162, 163, 8725, 165, 131, 167, 164, 146, 147, 171, 8249,
	8250, 64256, 64258, 0, 8211, 8224, 8225, 183, 0, 182, 8226,
	8218, 8222, 8221, 187, 8230, 8240, 0, 191, 0, 96, 180, 710,
	126, 713, 728, 729, 168, 0, 730, 184, 0, 733, 731, 711, 8212,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 508, 0,
	170, 0, 0, 0, 0, 321, 216, 338, 186, 0, 0, 0, 0, 0, 230, 0,
	0, 0, 185, 0, 0, 322, 248, 339, 223};

   if (fontcount == 0) return;

   /* Don't draw temporary labels from schematic capture system */
   if (drawlabel->string->type != FONT_NAME) return;

   if (passcolor == DOSUBSTRING)
      defaultcolor = curcolor = drawlabel->color;
   else
      defaultcolor = curcolor = passcolor;

   if (defaultcolor != DOFORALL) {
      if (drawlabel->color != DEFAULTCOLOR)
	 curcolor = drawlabel->color;
      else
	 curcolor = defaultcolor;
   }

   /* calculate the transformation matrix for this object */
   /* in natural units of the alphabet vectors		  */
   /* (conversion to window units)			  */

   /* Labels don't rotate in Firefox, so use <g> record for transform */

   UPushCTM();
   UPreMultCTM(DCTM, drawlabel->position, drawlabel->scale, drawlabel->rotation);

   /* check for flip invariance; recompute CTM and anchoring if necessary */
   tmpanchor = flipadjust(drawlabel->anchor);

   /* Note that the Y-scale is inverted or text comes out upside-down.  But we	*/
   /* need to adjust to the Y baseline.						*/

   fprintf(svgf, "<g transform=\"matrix(%4g %4g %4g %4g %3g %3g)\" ",
	DCTM->a, DCTM->d, -(DCTM->b), -(DCTM->e), DCTM->c, DCTM->f);

   svg_printcolor(passcolor, "fill=");
   fprintf(svgf, ">\n");

   /* "natural" (unscaled) length */
   tlinfo.dostop = 0;
   tlinfo.tbreak = NULL;
   tlinfo.padding = NULL;
   tmpext = ULength(drawlabel, localinst, &tlinfo);

   newpoint.x = (tmpanchor & NOTLEFT ?
       (tmpanchor & RIGHT ? -tmpext.maxwidth : -tmpext.maxwidth >> 1) : 0);
   newpoint.y = (tmpanchor & NOTBOTTOM ?
       (tmpanchor & TOP ? -tmpext.ascent : -(tmpext.ascent + tmpext.base) >> 1)
		: -tmpext.base);

   /* Pinlabels have an additional offset spacing to pad */
   /* them from the circuit point to which they attach.  */

   if (drawlabel->pin) {
      pinadjust(tmpanchor, &(newpoint.x), &(newpoint.y), 1);
   }

   oldx = newpoint.x;
   baseline = newpoint.y;

   open_text = -1;
   open_span = 0;
   open_decor = 0;
   pos = 0;

   /* Adjust for center or right justification */
   if (tlinfo.padding != NULL) {
      if (tmpanchor & JUSTIFYRIGHT)
	 newpoint.x += tlinfo.padding[linenum];
      else if (tmpanchor & TEXTCENTERED)
	 newpoint.x += 0.5 * tlinfo.padding[linenum];
      linenum++;
   }

   for (strptr = drawlabel->string; strptr != NULL;
		strptr = nextstringpart(strptr, localinst)) {

      /* All segments other than text cancel any	*/
      /* existing overline/underline in effect.		*/

      if (strptr->type != TEXT_STRING)
	 fstyle &= 0xfc7;

      switch(strptr->type) {
	 case RETURN:
	    while (open_span > 0) {
	       fprintf(svgf, "</tspan>");
	       open_span--;
	    }
	    while (open_text > 0) {
	       fprintf(svgf, "</text>");
	       open_text--;
	    }
	    if (open_decor) {
	       addlinepoint(decorations, nvals, newpoint.x, group);
	       open_decor--;
	    }
	    break;

	 case FONT_SCALE:
	 case FONT_NAME:
	    while (open_span > 0) {
	       fprintf(svgf, "</tspan>");
	       open_span--;
	    }
	    while (open_text > 0) {
	       fprintf(svgf, "</text>");
	       open_text--;
	    }
	    if (open_decor) {
	       addlinepoint(decorations, nvals, newpoint.x, group);
	       open_decor--;
	    }
	    break;

	 case KERN:
	 case TABFORWARD:
	 case TABBACKWARD:
	 case TABSTOP:
	 case HALFSPACE:
	 case QTRSPACE:
	 case NOLINE:
	 case UNDERLINE:
	 case OVERLINE:
	 case SUBSCRIPT:
	 case SUPERSCRIPT:
	 case NORMALSCRIPT:
	    while (open_span > 1) {
	       fprintf(svgf, "</tspan>");
	       open_span--;
	    }
	    if (open_decor) {
	       addlinepoint(decorations, nvals, newpoint.x, group);
	       open_decor--;
	    }
	    break;

	 /* These do not need to be handled */
	 case TEXT_STRING:
	 case PARAM_START:
	 case PARAM_END:
	    break;

	 /* These are not handled yet, but should be */
	 case FONT_COLOR:
	    break;

	 default:
	    break;
      }

      /* deal with each text segment type */

      switch(strptr->type) {
	 case FONT_SCALE:
	 case FONT_NAME:
	    if (strptr->data.font < fontcount) {
	       ffont = strptr->data.font;
	       fstyle = 0;		   /* style reset by font change */
	       if (baseline == newpoint.y) {  /* set top-level font and style */
	          oldfont = ffont;
	          oldstyle = fstyle;
	       }
	    }
	    fprintf(svgf, "<text stroke=\"none\" ");
	    fprintf(svgf, "font-family=");
	    if (issymbolfont(ffont))
		fprintf(svgf, "\"Times\" ");
	    else if (!strncmp(fonts[ffont].family, "Times", 5))
		fprintf(svgf, "\"Times\" ");
	    else
	       fprintf(svgf, "\"%s\" ", fonts[ffont].family);

	    if (fonts[ffont].flags & 0x1)
	       fprintf(svgf, " font-weight=\"bold\" ");
	    if (fonts[ffont].flags & 0x2) {
	       if (issansfont(ffont))
	          fprintf(svgf, " font-style=\"oblique\" ");
	       else
		  fprintf(svgf, " font-style=\"italic\" ");
	    }
	    olinerise = (issansfont(ffont)) ? 7 : 4;
		      
	    if (strptr->type == FONT_SCALE) {
	       tmpscale = natscale * strptr->data.scale;
	       if (baseline == newpoint.y) /* reset top-level scale */
		  natscale = tmpscale;
	    }
	    else
	       tmpscale = 1;

	    /* Actual scale taken care of by transformation matrix */
	    fprintf(svgf, "font-size=\"%g\" >", tmpscale * 40);
	    fprintf(svgf, "<tspan x=\"%d\" y=\"%d\">", newpoint.x, -newpoint.y);
	    open_text++;
	    open_span++;
	    break;

	 case KERN:
	    newpoint.x += strptr->data.kern[0];
	    newpoint.y += strptr->data.kern[1];
	    fprintf(svgf, "<text dx=\"%d\" dy=\"%d\">",
			strptr->data.kern[0], strptr->data.kern[1]);
	    open_text++;
	    break;
		
	 case FONT_COLOR:
	    if (defaultcolor != DOFORALL) {
	       if (strptr->data.color != DEFAULTCOLOR)
	          curcolor = colorlist[strptr->data.color].color.pixel;
	       else {
	          curcolor = DEFAULTCOLOR;
	       }
	    }
	    break;

	 case TABBACKWARD:	/* find first tab value with x < xtotal */
	    for (tabno = numtabs - 1; tabno >= 0; tabno--) {
	       if (tabstops[tabno] < newpoint.x) {
	          newpoint.x = tabstops[tabno];
	          break;
	       }
	    }
	    fprintf(svgf, "<tspan x=\"%d\">", newpoint.x);
	    open_span++;
	    break;

	 case TABFORWARD:	/* find first tab value with x > xtotal */
	    for (tabno = 0; tabno < numtabs; tabno++) {
	       if (tabstops[tabno] > newpoint.x) {
		  newpoint.x = tabstops[tabno];
		  break;
	       }
	    }
	    fprintf(svgf, "<tspan x=\"%d\">", newpoint.x);
	    open_span++;
	    break;

	 case TABSTOP:
	    numtabs++;
	    if (tabstops == NULL) tabstops = (short *)malloc(sizeof(short));
	    else tabstops = (short *)realloc(tabstops, numtabs * sizeof(short));
	    tabstops[numtabs - 1] = newpoint.x;
	    /* Force a tab at this point so that the output aligns	*/
	    /* to our computation of the position, not its own.	*/
	    fprintf(svgf, "<tspan x=\"%d\">", newpoint.x);
	    open_span++;
	    break;

	 case RETURN:
	    tmpscale = natscale = 1.0;
	    baseline -= BASELINE;
	    newpoint.y = baseline;
	    newpoint.x = oldx;

	    if (tlinfo.padding != NULL) {
	       if (tmpanchor & JUSTIFYRIGHT)
		  newpoint.x += tlinfo.padding[linenum];
	       else if (tmpanchor & TEXTCENTERED)
		  newpoint.x += 0.5 * tlinfo.padding[linenum];
	       linenum++;
	    }

	    fprintf(svgf, "<tspan x=\"%d\" y=\"%d\">", newpoint.x, -newpoint.y);
	    open_span++;
	    break;
	
	 case SUBSCRIPT:
	    natscale *= SUBSCALE; 
	    tmpscale = natscale;
	    deltay = (short)((TEXTHEIGHT >> 1) * natscale);
	    newpoint.y -= deltay;
	    fprintf(svgf, "<tspan dy=\"%d\" font-size=\"%g\">", deltay,
			40 * natscale);
	    open_span++;
	    break;

	 case SUPERSCRIPT:
	    natscale *= SUBSCALE;
	    tmpscale = natscale;
	    deltay = (short)(TEXTHEIGHT * natscale);
	    newpoint.y += deltay;
	    fprintf(svgf, "<tspan dy=\"%d\" font-size=\"%g\">", -deltay,
			40 * natscale);
	    open_span++;
	    break;

	 case NORMALSCRIPT:
	    tmpscale = natscale = 1.0;
	    ffont = oldfont;	/* revert to top-level font and style */
	    fstyle = oldstyle;
	    newpoint.y = baseline;
	    fprintf(svgf, "<tspan y=\"%d\">", baseline); 
	    open_span++;
	    break;

	 case UNDERLINE:
	    fstyle |= 8;
	    group = newpoint.y - 6;
	    addlinepoint(decorations, nvals, newpoint.x, group);
	    open_decor++;
	    break;

	 case OVERLINE:
	    if (strptr->nextpart != NULL && strptr->nextpart->type == TEXT_STRING) {
	       objectptr charptr;
	       int tmpheight;

	       group = 0;
	       for (textptr = strptr->nextpart->data.string;
				textptr && *textptr != '\0'; textptr++) {
		  charptr = fonts[ffont].encoding[*(u_char *)textptr];
		  tmpheight = (int)((float)charptr->bbox.height
				* fonts[ffont].scale);
		  if (group < tmpheight) group = (short)tmpheight;
	       }
	       fstyle |= 16;
	       group += olinerise + newpoint.y;
	       addlinepoint(decorations, nvals, newpoint.x, group);
	    }
	    open_decor++;
	    break;

	 case NOLINE:
	    break;

	 case HALFSPACE: case QTRSPACE: {
	    short addx;
	    objectptr drawchar = fonts[ffont].encoding[(u_char)32];
	    addx = (drawchar->bbox.lowerleft.x + drawchar->bbox.width) *
			fonts[ffont].scale;
	    addx >>= ((strptr->type == HALFSPACE) ? 1 : 2);
	    newpoint.x += addx;
	    fprintf(svgf, "<tspan dx=\"%d\">", addx);
	    open_span++;

	    } break;
	    
	 case TEXT_STRING:
	    textptr = strptr->data.string;

	    if (issymbolfont(ffont)) {
	       for (; *textptr != '\0'; textptr++)
		  if (((u_char)(*textptr) >= 32) && ((u_char)(*textptr) < 158))
		     fprintf(svgf, "%s", symbol_html_encoding[(*textptr) - 32]);
	    }
	    else {
	       /* Handle "&" and non-ASCII characters in the text */
	       if (isisolatin1(ffont)) {
		  for (; *textptr != '\0'; textptr++) {
		     if (*textptr == '&')
		        fprintf(svgf, "&amp;");
		     else if ((u_char)(*textptr) >= 128)
		        fprintf(svgf, "&#%d;", (int)((u_char)*textptr));
		     else if ((u_char)(*textptr) >= 32)
			fprintf(svgf, "%c", *textptr);
		  }
	       }
	       else {
		  for (; *textptr != '\0'; textptr++) {
		     if (*textptr == '&')
		        fprintf(svgf, "&amp;");
		     else if ((u_char)(*textptr) >= 161)
		        fprintf(svgf, "&#%d;",
				standard_html_encoding[(u_char)(*textptr)
				- 161]);
		     else if ((u_char)(*textptr) >= 32 && (u_char)(*textptr) < 161)
			fprintf(svgf, "%c", *textptr);
		  }
	       }
	    }
	    pos--;

	    /* Compute the new X position */

	    for (textptr = strptr->data.string; *textptr != '\0'; textptr++) {
	       objectptr drawchar = fonts[ffont].encoding[(u_char)(*textptr)];
	       short addx = (drawchar->bbox.lowerleft.x + drawchar->bbox.width) *
			fonts[ffont].scale;
	       newpoint.x += addx;
	    }
	    break;
      }
      pos++;
   }
   while (open_span > 0) {
      fprintf(svgf, "</tspan>");
      open_span--;
   }
   while (open_text > 0) {
      fprintf(svgf, "</text>");
      open_text--;
   }
   fprintf(svgf, "\n</text>");

   UPopCTM();

   if (tabstops != NULL) free(tabstops);
   if (tlinfo.padding != NULL) free(tlinfo.padding);

   /* If there were decorations (underlines, overlines), generate them */

   if (decorations != NULL) {
      int i;
      if (open_decor) {
	 addlinepoint(decorations, nvals, newpoint.x, group);
      }
      for (i = 0; i < nvals; i += 2) {
	 fprintf(svgf, "\n<line stroke-width=\"2\" stroke-linecap=\"square\" "
		"x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" />",
		decorations[i].x, decorations[i].y, decorations[i + 1].x,
		decorations[i + 1].y);
      }
      free(decorations);
   }
   fprintf(svgf, "</g>\n");
}

/*----------------------------------------------------------------------*/
/* Write the SVG file output						*/
/*----------------------------------------------------------------------*/

#define PMARGIN	6		/* Pixel margin around drawing */

void
OutputSVG(char *filename, Boolean fullscale)
{
   short	savesel;
   objinstptr	pinst;
   int cstyle;
   float outwidth, outheight, cscale;

   svgf = fopen(filename, "w");
   if (svgf == NULL) {
      Fprintf(stderr, "Cannot open file %s for writing.\n", filename);
      return;
   }

   /* Generate external image files, if necessary */
   SVGCreateImages(areawin->page);

   /* Save the number of selections and set it to zero while we do the	*/
   /* object drawing.							*/

   savesel = areawin->selects;
   areawin->selects = 0;
   pinst = xobjs.pagelist[areawin->page]->pageinst;

   UPushCTM();	/* Save the top-level graphics state */

   /* This is like UMakeWCTM()---it inverts the whole picture so that	*/
   /* The origin is at the top left, and all data points fit in a box	*/
   /* at (0, 0) to the object (width, height)				*/

   DCTM->a = 1.0;
   DCTM->b = 0.0;
   DCTM->c = -pinst->bbox.lowerleft.x;
   DCTM->d = 0.0;
   DCTM->e = -1.0;
   DCTM->f = pinst->bbox.lowerleft.y + pinst->bbox.height;

   fprintf(svgf, "<svg xmlns=\"http://www.w3.org/2000/svg\"\n");
   fprintf(svgf, "   xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
   fprintf(svgf, "   version=\"1.1\"\n");
   fprintf(svgf, "   id=\"%s\" ", pinst->thisobject->name);

   if (fullscale) {
      fprintf(svgf, "width=\"100%%\" height=\"100%%\" ");
   }
   else {
      cscale = getpsscale(xobjs.pagelist[areawin->page]->outscale, areawin->page);
      cstyle = xobjs.pagelist[areawin->page]->coordstyle;

      outwidth = toplevelwidth(pinst, NULL) * cscale;
      outwidth /= (cstyle == CM) ?  IN_CM_CONVERT : 72.0;
      outheight = toplevelheight(pinst, NULL) * cscale;
      outheight /= (cstyle == CM) ?  IN_CM_CONVERT : 72.0;

      /* Set display height to that specified in the output properties (in inches) */
      fprintf(svgf, "width=\"%.3g%s\" height=\"%.3g%s\" ",
		outwidth, (cstyle == CM) ? "cm" : "in",
		outheight, (cstyle == CM) ? "cm" : "in");
   }
   fprintf(svgf, " viewBox=\"%d %d %d %d\">\n",
		-PMARGIN, -PMARGIN, pinst->bbox.width + PMARGIN,
		pinst->bbox.height + PMARGIN);

   fprintf(svgf, "<desc>\n");
   fprintf(svgf, "XCircuit Version %s\n", PROG_VERSION);
   fprintf(svgf, "File \"%s\" Page %d\n", xobjs.pagelist[areawin->page]->filename,
		areawin->page + 1); 
   fprintf(svgf, "</desc>\n");

   /* Set default color to black */
   fprintf(svgf, "<g stroke=\"black\">\n");

   if (areawin->hierstack) free_stack(&areawin->hierstack);
   SVGDrawObject(areawin->topinstance, TOPLEVEL, FOREGROUND, &areawin->hierstack);
   if (areawin->hierstack) free_stack(&areawin->hierstack);

   /* restore the selection list (if any) */
   areawin->selects = savesel;

   fprintf(svgf, "</g>\n</svg>\n");
   fclose(svgf);

   UPopCTM();	/* Restore the top-level graphics state */
}

#ifdef TCL_WRAPPER 

/*----------------------------------------------------------------------*/
/* The TCL command-line for the SVG file write routine.			*/
/*----------------------------------------------------------------------*/

int xctcl_svg(ClientData clientData, Tcl_Interp *interp,
        int objc, Tcl_Obj *CONST objv[])
{
   char filename[128], *pptr;
   Boolean fullscale = 0;
   int locobjc = objc;
   char *lastarg;

   /* Argument "-full" forces full scale (not scaled per page output settings) */
   if (objc > 1) {
      lastarg = Tcl_GetString(objv[objc - 1]);
      if (lastarg[0] == '-') {
         if (!strncmp(lastarg + 1, "full", 4))
	    fullscale = 1;
	 else {
	    Tcl_SetResult(interp, "Unknown option.\n", NULL);
	    return TCL_ERROR;
	 }
	 locobjc--;
      }
   }


   if (locobjc >= 2) {
      /* If there is a non-option argument, use it for the output filename */
      sprintf(filename, Tcl_GetString(objv[1]));
   }
   else if (xobjs.pagelist[areawin->page]->pageinst->thisobject->name == NULL)
      sprintf(filename, xobjs.pagelist[areawin->page]->filename);
   else
      sprintf(filename, xobjs.pagelist[areawin->page]->pageinst->thisobject->name);

   pptr = strrchr(filename, '.');
   if (pptr != NULL)
      sprintf(pptr + 1, "svg");
   else if (strcmp(filename + strlen(filename) - 3, "svg"))
      strcat(filename, ".svg");

   OutputSVG(filename, fullscale);
   Fprintf(stdout, "Saved page as SVG format file \"%s\"\n", filename);
   return XcTagCallback(interp, objc, objv);
}

#endif /* TCL_WRAPPER */

/*-------------------------------------------------------------------------*/
