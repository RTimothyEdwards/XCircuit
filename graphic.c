/*----------------------------------------------------------------------*/
/* graphic.c --- xcircuit routines handling rendered graphic elements	*/
/* Copyright (c) 2005  Tim Edwards, MultiGiG, Inc.			*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*	written by Tim Edwards, 7/11/05					*/
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef _MSC_VER
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif

#ifdef TCL_WRAPPER
#include <tk.h>
#endif

/*----------------------------------------------------------------------*/
/* Local includes							*/
/*----------------------------------------------------------------------*/

#include "xcircuit.h"
#include "colordefs.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations					*/
/*----------------------------------------------------------------------*/
#include "prototypes.h"

/*----------------------------------------------------------------------*/
/* Global Variable definitions						*/
/*----------------------------------------------------------------------*/

extern Display  *dpy;
extern Globaldata xobjs;
extern XCWindowData *areawin;
extern colorindex *colorlist;
extern int number_colors;

/*----------------------------------------------------------------------*/
/* Recursive search for graphic images in an object.			*/
/* Updates list "glist" when any image is found in an object or its	*/
/* descendents.								*/
/*----------------------------------------------------------------------*/

void count_graphics(objectptr thisobj, short *glist)
{
   genericptr *ge;
   graphicptr gp;
   Imagedata *iptr;
   int i;

   for (ge = thisobj->plist; ge < thisobj->plist + thisobj->parts; ge++) {
      if (IS_GRAPHIC(*ge)) {
	 gp = TOGRAPHIC(ge);
         for (i = 0; i < xobjs.images; i++) {
	    iptr = xobjs.imagelist + i;
            if (iptr->image == gp->source) {
	       glist[i]++;
	    }
	 }
      }
      else if (IS_OBJINST(*ge)) {
	 count_graphics(TOOBJINST(ge)->thisobject, glist);
      }
   }
}

/*----------------------------------------------------------------------*/
/* Given a list of pages, return a list of indices into the graphics	*/
/* buffer area of each graphic used on any of the indicated pages.	*/
/* The returned list is allocated and it is the responsibility of the	*/
/* calling routine to free it.						*/
/*----------------------------------------------------------------------*/

short *collect_graphics(short *pagelist)
{
   short *glist;
   int i;

   glist = (short *)malloc(xobjs.images * sizeof(short));

   for (i = 0; i < xobjs.images; i++) glist[i] = 0;

   for (i = 0; i < xobjs.pages; i++)
      if (pagelist[i] > 0)
	 count_graphics(xobjs.pagelist[i]->pageinst->thisobject, glist);
	
   return glist;
}

/*----------------------------------------------------------------------*/
/* Generate the target view of the indicated graphic image, combining	*/
/* the image's scale and rotation and the zoom factor of the current	*/
/* view.								*/
/*									*/
/* If the graphic is at the wrong scale or rotation but is not redrawn	*/
/* because it is outside the screen viewing area, return FALSE. 	*/
/* Otherwise, return TRUE.						*/
/*----------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
Boolean transform_graphic(graphicptr gp)
{
    int width, height, twidth, theight;
    float scale, tscale, rotation, crot;
    double cosr, sinr;
    int x, y, c, s, hw, hh, thw, thh, xorig, yorig, xc, yc;
    int screen = DefaultScreen(dpy);
    static GC cmgc = (GC)NULL;

    tscale = UTopScale();
    scale = gp->scale * tscale;
    rotation = gp->rotation + UTopRotation();

    if (rotation >= 360.0) rotation -= 360.0;
    else if (rotation < 0.0) rotation += 360.0;

    /* Check if the top-level rotation and scale match the	*/
    /* saved target image.  If so, then we're done.		*/
    if ((rotation == gp->trot) && (scale == gp->tscale)) return TRUE;

    cosr = cos(RADFAC * rotation);
    sinr = sin(RADFAC * rotation);
    c = (int)(8192 * cosr / scale);
    s = (int)(8192 * sinr / scale);

    /* Determine the necessary width and height of the pixmap	*/
    /* that fits the rotated and scaled image.			*/

    crot = rotation;
    if (crot > 90.0 && crot < 180.0) crot = 180.0 - crot;
    if (crot > 270.0 && crot < 360.0) crot = 360.0 - crot;
    cosr = cos(RADFAC * crot);
    sinr = sin(RADFAC * crot);
    width = gp->source->width * scale;
    height = gp->source->height * scale;

    twidth = (int)(fabs(width * cosr + height * sinr));
    theight = (int)(fabs(width * sinr + height * cosr));
    if (twidth & 1) twidth++;
    if (theight & 1) theight++;

    /* Check whether this instance is going to be off-screen,	*/
    /* to avoid excessive computation.				*/

    UTopOffset(&xc, &yc);
    xc += (int)((float)gp->position.x * tscale);
    yc = areawin->height - yc;
    yc += (int)((float)gp->position.y * tscale);

    if (xc - (twidth >> 1) > areawin->width) return FALSE;
    if (xc + (twidth >> 1) < 0) return FALSE;
    if (yc - (theight >> 1) > areawin->height) return FALSE;
    if (yc + (theight >> 1) < 0) return FALSE;

    /* Generate the new target image */
    if (gp->target != NULL) {
       if (gp->target->data != NULL) {
	  /* Free data first, because we used our own malloc() */
	  free(gp->target->data);
	  gp->target->data = NULL;
       }
       XDestroyImage(gp->target);
    }
    if (gp->clipmask != (Pixmap)NULL) XFreePixmap(dpy, gp->clipmask);

    gp->target = XCreateImage(dpy, DefaultVisual(dpy, screen),
			DefaultDepth(dpy, screen), ZPixmap,
			0, 0, twidth, theight, 8, 0);
    gp->target->data = (char *)malloc(theight * gp->target->bytes_per_line);

    if (gp->target->data == (char *)NULL) {
       XDestroyImage(gp->target);
       gp->target = (XImage *)NULL;
       gp->clipmask = (Pixmap)NULL;
       return FALSE;
    }

    if (rotation != 0.0) {
       gp->clipmask = XCreatePixmap(dpy, areawin->window, twidth, theight, 1);
       if (cmgc == (GC)NULL) {
          XGCValues values;
          values.foreground = 0;
          values.background = 0;
          cmgc = XCreateGC(dpy, gp->clipmask, GCForeground | GCBackground, &values);
       }
       XSetForeground(dpy, cmgc, 1);
       XFillRectangle(dpy, gp->clipmask, cmgc, 0, 0, twidth, theight);
       XSetForeground(dpy, cmgc, 0);
    }
    else
       gp->clipmask = (Pixmap)NULL;

    hh = gp->source->height >> 1;
    hw = gp->source->width >> 1;
    thh = theight >> 1;
    thw = twidth >> 1;
    for (y = -thh; y < thh; y++) {
	for (x = -thw; x < thw; x++) {
	    xorig = ((x * c + y * s) >> 13) + hw;
	    yorig = ((-x * s + y * c) >> 13) + hh;

	    if ((xorig >= 0) && (yorig >= 0) &&
			(xorig < gp->source->width) && (yorig < gp->source->height))
	       XPutPixel(gp->target, x + thw, y + thh,
			XGetPixel(gp->source, xorig, yorig));
	    else if (gp->clipmask)
	       XDrawPoint(dpy, gp->clipmask, cmgc, x + thw, y + thh);
	}
    }
    gp->tscale = scale;
    gp->trot = rotation;
    return TRUE;
}
#endif /* HAVE_CAIRO */

/*----------------------------------------------------------------------*/
/* Draw a graphic image by copying from the image to the window.	*/
/* Image is centered on the center point of the graphic image.		*/
/*----------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
void UDrawGraphic(graphicptr gp)
{
    XPoint ppt;

    /* transform to current scale and rotation, if necessary */
    if (transform_graphic(gp) == FALSE) return;  /* Graphic off-screen */

    /* transform to current position */
    UTransformbyCTM(DCTM, &(gp->position), &ppt, 1);

    /* user_to_window(gp->position, &ppt); */

    ppt.x -= (gp->target->width >> 1);
    ppt.y -= (gp->target->height >> 1);

    if (gp->clipmask != (Pixmap)NULL) {
       if (areawin->clipped > 0) {
	  /* Clipmask areawin->clipmask already applied to window. */
	  /* Modify existing clipmask with the graphic's.	   */
	  XSetFunction(dpy, areawin->cmgc, GXand);
	  XCopyArea(dpy, gp->clipmask, areawin->clipmask, areawin->cmgc,
		0, 0, gp->target->width, gp->target->height, ppt.x, ppt.y);
          XSetClipMask(dpy, areawin->gc, areawin->clipmask);
	  XSetFunction(dpy, areawin->cmgc, GXcopy);
       }
       else {
          XSetClipOrigin(dpy, areawin->gc, ppt.x, ppt.y);
          XSetClipMask(dpy, areawin->gc, gp->clipmask);
       }
    }

    XPutImage(dpy, areawin->window, areawin->gc,
		gp->target, 0, 0, ppt.x, ppt.y, gp->target->width,
		gp->target->height);

    if (gp->clipmask != (Pixmap)NULL) {
       if (areawin->clipped <= 0) {
	  XSetClipMask(dpy, areawin->gc, None);
          XSetClipOrigin(dpy, areawin->gc, 0, 0);
       }
    }
}
#endif /* HAVE_CAIRO */

/*----------------------------------------------------------------------*/
/* Allocate space for a new graphic source image of size width x height	*/
/*----------------------------------------------------------------------*/

Imagedata *addnewimage(char *name, int width, int height)
{
   Imagedata *iptr;
   int screen = DefaultScreen(dpy);

   /* Create the image and store in the global list of images */

   xobjs.images++;
   if (xobjs.imagelist)
      xobjs.imagelist = (Imagedata *)realloc(xobjs.imagelist,
		xobjs.images * sizeof(Imagedata));
   else
      xobjs.imagelist = (Imagedata *)malloc(sizeof(Imagedata));
    
   /* Save the image source in a file */
   iptr = xobjs.imagelist + xobjs.images - 1;
   if (name)
      iptr->filename = strdup(name);
   else
      iptr->filename = NULL;	/* must be filled in later! */
   iptr->refcount = 0;		/* no calls yet */
   iptr->image = xcImageCreate(width, height);

   return iptr;
}

/*----------------------------------------------------------------------*/
/* Create a new graphic image from a PPM file, and position it at the	*/
/* indicated (px, py) coordinate in user space.				*/
/*									*/
/* This should be expanded to incorporate more PPM formats.  Also, it	*/
/* needs to be combined with the render.c routines to transform		*/
/* PostScript graphics into an internal XImage for faster rendering.	*/
/*----------------------------------------------------------------------*/

graphicptr new_graphic(objinstptr destinst, char *filename, int px, int py)
{
    graphicptr *gp;
    objectptr destobject;
    objinstptr locdestinst;
    Imagedata *iptr;
    FILE *fg;
    int nr, width, height, imax, x, y, i; /* nf, (jdk) */
    char id[5], c, buf[128];

    locdestinst = (destinst == NULL) ? areawin->topinstance : destinst;
    destobject = locdestinst->thisobject;

    /* Check the existing list of images.  If there is a match,	*/
    /* re-use the source; don't load the file again.		*/

    for (i = 0; i < xobjs.images; i++) {
       iptr = xobjs.imagelist + i;
       if (!strcmp(iptr->filename, filename)) {
	  break;
       }
    }
    if (i == xobjs.images) {

       fg = fopen(filename, "r");
       if (fg == NULL) return NULL;

       /* This ONLY handles binary ppm files with max data = 255 */

       while (1) {
          nr = fscanf(fg, " %s", buf);
	  if (nr <= 0) return NULL;
	  if (buf[0] != '#') {
	     if (sscanf(buf, "%s", id) <= 0)
		return NULL;
	     break;
	  }
	  else fgets(buf, 127, fg);
       }
       if ((nr <= 0) || strncmp(id, "P6", 2)) return NULL;

       while (1) {
          nr = fscanf(fg, " %s", buf);
	  if (nr <= 0) return NULL;
	  if (buf[0] != '#') {
	     if (sscanf(buf, "%d", &width) <= 0)
		return NULL;
	     break;
	  }
	  else fgets(buf, 127, fg);
       }
       if (width <= 0) return NULL;

       while (1) {
          nr = fscanf(fg, " %s", buf);
	  if (nr <= 0) return NULL;
	  if (buf[0] != '#') {
	     if (sscanf(buf, "%d", &height) <= 0)
		return NULL;
	     break;
	  }
	  else fgets(buf, 127, fg);
       }
       if (height <= 0) return NULL;

       while (1) {
          nr = fscanf(fg, " %s", buf);
	  if (nr <= 0) return NULL;
	  if (buf[0] != '#') {
	     if (sscanf(buf, "%d", &imax) <= 0)
		return NULL;
	     break;
	  }
	  else fgets(buf, 127, fg);
       }
       if (imax != 255) return NULL;

       while (1) {
          fread(&c, 1, 1, fg);
          if (c == '\n') break;
          else if (c == '\0') return NULL;
       }

       iptr = addnewimage(filename, width, height);

       /* Read the image data from the PPM file */
       for (y = 0; y < height; y++)
          for (x = 0; x < width; x++) {
	     u_char r, g, b;
	     fread(&r, 1, 1, fg);
	     fread(&g, 1, 1, fg);
	     fread(&b, 1, 1, fg);
	     xcImagePutPixel(iptr->image, x, y, r, g, b);
          }
    }

    iptr->refcount++;
    NEW_GRAPHIC(gp, destobject);

    (*gp)->scale = 1.0;
    (*gp)->position.x = px;
    (*gp)->position.y = py;
    (*gp)->rotation = 0.0;
    (*gp)->color = DEFAULTCOLOR;
    (*gp)->passed = NULL;
    (*gp)->source = iptr->image;
#ifndef HAVE_CAIRO
    (*gp)->target = NULL;
    (*gp)->trot = 0.0;
    (*gp)->tscale = 0;
    (*gp)->clipmask = (Pixmap)NULL;
#endif /* HAVE_CAIRO */

    calcbboxvalues(locdestinst, (genericptr *)gp);
    updatepagebounds(destobject);
    incr_changes(destobject);

    register_for_undo(XCF_Graphic, UNDO_DONE, areawin->topinstance, *gp);

    return *gp;
}

/*----------------------------------------------------------------------*/
/* Create a gradient field graphic					*/
/* For now, gradient field is linear white-to-black size 100x100.	*/
/*----------------------------------------------------------------------*/

graphicptr gradient_field(objinstptr destinst, int px, int py, int c1, int c2)
{
    graphicptr *gp;
    objectptr destobject;
    objinstptr locdestinst;
    Imagedata *iptr;
    int width, height, imax, x, y, i;
    int r, g, b, rd, gd, bd;
    char id[11];

    locdestinst = (destinst == NULL) ? areawin->topinstance : destinst;
    destobject = locdestinst->thisobject;

    if (c1 < 0) c1 = 0;
    if (c1 >= number_colors) c1 = 1;
    if (c2 < 0) c2 = 0;
    if (c2 >= number_colors) c2 = 1;

    /* Create name "gradientXX"	*/

    y = 0;
    for (i = 0; i < xobjs.images; i++) {
       iptr = xobjs.imagelist + i;
       if (!strncmp(iptr->filename, "gradient", 8)) {
	  if (sscanf(iptr->filename + 8, "%2d", &x) == 1)
	     if (x >= y)
		y = x + 1;
       }
    }
    sprintf(id, "gradient%02d", y);

    width = height = 100;	/* Fixed size, at least for now */

    iptr = addnewimage(id, width, height);

    r = (colorlist[c1].color.red >> 8);
    g = (colorlist[c1].color.green >> 8);
    b = (colorlist[c1].color.blue >> 8);

    rd = (colorlist[c2].color.red >> 8) - r;
    gd = (colorlist[c2].color.green >> 8) - g;
    bd = (colorlist[c2].color.blue >> 8) - b;

    for (y = 0; y < height; y++)
      for (x = 0; x < width; x++) {
	 xcImagePutPixel(iptr->image, x, y,
	       r + ((y * rd) / (height - 1)),
	       g + ((y * gd) / (height - 1)),
	       b + ((y * bd) / (height - 1)));
      }

    iptr->refcount++;
    NEW_GRAPHIC(gp, destobject);

    (*gp)->scale = 1.0;
    (*gp)->position.x = px;
    (*gp)->position.y = py;
    (*gp)->rotation = 0.0;
    (*gp)->color = DEFAULTCOLOR;
    (*gp)->passed = NULL;
    (*gp)->source = iptr->image;
#ifndef HAVE_CAIRO
    (*gp)->target = NULL;
    (*gp)->trot = 0.0;
    (*gp)->tscale = 0;
    (*gp)->clipmask = (Pixmap)NULL;
#endif /* HAVE_CAIRO */

    calcbboxvalues(locdestinst, (genericptr *)gp);
    updatepagebounds(destobject);
    incr_changes(destobject);

    register_for_undo(XCF_Graphic, UNDO_DONE, areawin->topinstance, *gp);

    return *gp;
}


/*----------------------------------------------------------------------*/
/* Free memory associated with the XImage structure for a graphic.	*/
/*----------------------------------------------------------------------*/

void freeimage(xcImage *source)
{
   int i, j;
   Imagedata *iptr;

   for (i = 0; i < xobjs.images; i++) {
      iptr = xobjs.imagelist + i;
      if (iptr->image == source) {
	 iptr->refcount--;
	 if (iptr->refcount <= 0) {
	    xcImageDestroy(iptr->image);
	    free(iptr->filename);

	    /* Remove this from the list of images */

	    for (j = i; j < xobjs.images - 1; j++)
	       *(xobjs.imagelist + j) = *(xobjs.imagelist + j + 1);
	    xobjs.images--;
	 }
	 break;
      }
   }
}

/*----------------------------------------------------------------------*/
/* Free memory allocated by a graphicptr structure.			*/
/*----------------------------------------------------------------------*/

void freegraphic(graphicptr gp)
{
#ifndef HAVE_CAIRO
   if (gp->target != NULL) {
      if (gp->target->data != NULL) {
	 /* Free data first, because we used our own malloc() */
	 free(gp->target->data);
	 gp->target->data = NULL;
      }
      XDestroyImage(gp->target);
   }
   if (gp->clipmask != (Pixmap)NULL) XFreePixmap(dpy, gp->clipmask);
#endif /* HAVE_CAIRO */
   freeimage(gp->source);
}

/*----------------------------------------------------------------------*/
/* A light wrapper around XImage, to a generalized xcImage              */
/*----------------------------------------------------------------------*/

#ifndef HAVE_CAIRO
xcImage *xcImageCreate(int width, int height)
{
   int screen = DefaultScreen(dpy);
   xcImage *img = XCreateImage(dpy, DefaultVisual(dpy, screen),
	 DefaultDepth(dpy, screen), ZPixmap, 0, NULL, width, height, 8, 0);
   img->data = (char *) malloc(height * img->bytes_per_line);
   return img;
}

void xcImageDestroy(xcImage *img)
{
   free(img->data);
   img->data = NULL;
   XDestroyImage(img);
}

int xcImageGetWidth(xcImage *img)
{
   return img->width;
}

int xcImageGetHeight(xcImage *img)
{
   return img->height;
}

void xcImagePutPixel(xcImage *img, int x, int y, u_char r, u_char g, u_char b)
{
   union {
      u_char b[4];
      u_long i;
   } pixel;
   pixel.b[3] = 0;
   pixel.b[2] = r;
   pixel.b[1] = g;
   pixel.b[0] = b;
   XPutPixel(img, x, y, pixel.i);
}

void xcImageGetPixel(xcImage *img, int x, int y, u_char *r, u_char *g,
      u_char *b)
{
   union {
      u_char b[4];
      u_long i;
   } pixel;
   pixel.i = XGetPixel(img, x, y);
   *r = pixel.b[2];
   *g = pixel.b[1];
   *b = pixel.b[0];
}
#endif /* HAVE_CAIRO */

