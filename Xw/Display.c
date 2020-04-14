/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        Display.c
 **
 **   Project:     X Widgets
 **
 **   Description: Code for TextEdit widget ascii sink
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1987, 1988 by Digital Equipment Corporation, Maynard,
 **             Massachusetts, and the Massachusetts Institute of Technology,
 **             Cambridge, Massachusetts
 **   
 **   Permission to use, copy, modify, and distribute this software 
 **   and its documentation for any purpose and without fee is hereby 
 **   granted, provided that the above copyright notice appear in all 
 **   copies and that both that copyright notice and this permission 
 **   notice appear in supporting documentation, and that the names of 
 **   Hewlett-Packard, Digital or  M.I.T.  not be used in advertising or 
 **   publicity pertaining to distribution of the software without 
 **   written prior permission.
 **   
 **   DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 **   ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 **   DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 **   ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 **   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 **   ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 **   SOFTWARE.
 **   
 *****************************************************************************
 *************************************<+>*************************************/


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>

#include <Xw/TextEditP.h>

#ifdef DEBUG
#include <stdio.h>
#endif


#define GETLASTPOS (*(source->scan))(source, 0, XwstLast, XwsdRight, 1, TRUE)
/* Private Ascii TextSink Definitions */

static unsigned bufferSize = 200;

typedef struct _AsciiSinkData {
    Pixel foreground;
    GC normgc, invgc, xorgc;
    XFontStruct *font;
    int tabwidth;
    Pixmap insertCursorOn;
    XwInsertState laststate;
} AsciiSinkData, *AsciiSinkPtr;

static unsigned char *buf;

/* XXX foreground default should be XtDefaultFGPixel. How do i do that?? */

static XtResource SinkResources[] = {
    {XtNfont, XtCFont, XtRFontStruct, sizeof (XFontStruct *),
        XtOffset(AsciiSinkPtr, font), XtRString, "Fixed"},
    {XtNforeground, XtCForeground, XtRPixel, sizeof (int),
        XtOffset(AsciiSinkPtr, foreground), XtRString, "XtDefaultForeground"},    
};

/* Utilities */

/*--------------------------------------------------------------------------+*/
static int CharWidth (data, x, margin, c)
/*--------------------------------------------------------------------------+*/
  AsciiSinkData *data;
  int x;
  int margin ;
  unsigned char c;
{
    int     width, nonPrinting;
    XFontStruct *font = data->font;

    if (c == '\t')
        /* This is totally bogus!! need to know tab settings etc.. */
	return data->tabwidth - ((x-margin) % data->tabwidth);
    if (c == LF)
	c = SP;
    nonPrinting = (c < SP);
    if (nonPrinting) c += '@';

    if (font->per_char &&
	    (c >= font->min_char_or_byte2 && c <= font->max_char_or_byte2))
	width = font->per_char[c - font->min_char_or_byte2].width;
    else
	width = font->min_bounds.width;

    if (nonPrinting)
	width += CharWidth(data, x, margin, '^');

    return width;
}

/* Sink Object Functions */

#define LBEARING(x) \
    ((font->per_char != NULL && \
      ((x) >= font->min_char_or_byte2 && (x) <= font->max_char_or_byte2)) \
	? font->per_char[(x) - font->min_char_or_byte2].lbearing \
	: font->min_bounds.lbearing)

/*--------------------------------------------------------------------------+*/
static int AsciiDisplayText (w, x, y, pos1, pos2, highlight)
/*--------------------------------------------------------------------------+*/
  Widget w;
  Position x, y;
  int highlight;
  XwTextPosition pos1, pos2;
{
    XwTextSink *sink = ((XwTextEditWidget)w)->text.sink;
    XwTextSource *source = ((XwTextEditWidget)w)->text.source;
    AsciiSinkData *data = (AsciiSinkData *) sink->data ;
    int margin = ((XwTextEditWidget)w)->text.leftmargin ;

    XFontStruct *font = data->font;
    int     j, k;
    Dimension width;
    XwTextBlock blk;
    GC gc = highlight ? data->invgc : data->normgc;
    GC invgc = highlight ? data->normgc : data->invgc;

    y += font->ascent;
    j = 0;
    while (pos1 < pos2) {
	pos1 = (*(source->read))(source, pos1, &blk, pos2 - pos1);
	for (k = 0; k < blk.length; k++) {
	    if (j >= bufferSize - 5) {
		bufferSize *= 2;
		buf = (unsigned char *) XtRealloc(buf, bufferSize);
	    }
	    buf[j] = blk.ptr[k];
	    if (buf[j] == LF)
		buf[j] = ' ';
	    else if (buf[j] == '\t') {
	        XDrawImageString(XtDisplay(w), XtWindow(w),
			gc, x - LBEARING(*buf), y, buf, j);
		buf[j] = 0;
		x += XTextWidth(data->font, buf, j);
		width = CharWidth(data, x, margin, '\t');
		XFillRectangle(XtDisplay(w), XtWindow(w), invgc, x,
			       y - font->ascent, width,
			       (Dimension) (data->font->ascent +
					    data->font->descent));
		x += width;
		j = -1;
	    }
	    else
		if (buf[j] < ' ') {
		    buf[j + 1] = buf[j] + '@';
		    buf[j] = '^';
		    j++;
		}
	    j++;
	}
    }
    XDrawImageString(XtDisplay(w), XtWindow(w), gc, x - LBEARING(*buf), y,
		     buf, j);
}


#   define insertCursor_width 6
#   define insertCursor_height 3
static char insertCursor_bits[] = {0x0c, 0x1e, 0x33};

/*--------------------------------------------------------------------------+*/
static Pixmap CreateInsertCursor(s)
/*--------------------------------------------------------------------------+*/
Screen *s;
{

    return (XCreateBitmapFromData (DisplayOfScreen(s), RootWindowOfScreen(s),
        insertCursor_bits, insertCursor_width, insertCursor_height));
}

/*
 * The following procedure manages the "insert" cursor.
 */

/*--------------------------------------------------------------------------+*/
static AsciiInsertCursor (w, x, y, state)
/*--------------------------------------------------------------------------+*/
  Widget w;
  Position x, y;
  XwInsertState state;
{
    XwTextSink *sink = ((XwTextEditWidget)w)->text.sink;
    AsciiSinkData *data = (AsciiSinkData *) sink->data;

/*
    XCopyArea(sink->dpy,
	      (state == XwisOn) ? data->insertCursorOn : data->insertCursorOff, w,
	      data->normgc, 0, 0, insertCursor_width, insertCursor_height,
	      x - (insertCursor_width >> 1), y - (insertCursor_height));

    if (state != data->laststate && XtIsRealized(w))
	XCopyPlane(XtDisplay(w),
		  data->insertCursorOn, XtWindow(w),
		  data->xorgc, 0, 0, insertCursor_width, insertCursor_height,
		  x - (insertCursor_width >> 1), y - (insertCursor_height), 1);
*/
/* This change goes with the cursor hack for the broken server */
    XCopyArea (XtDisplay(w), data->insertCursorOn, XtWindow (w),
	       data->xorgc, 0, 0, insertCursor_width, insertCursor_height,
	       x - (insertCursor_width >> 1), y - (insertCursor_height));

    data->laststate = state;
}

/*
 * Clear the passed region to the background color.
 */

/*--------------------------------------------------------------------------+*/
static AsciiClearToBackground (w, x, y, width, height)
/*--------------------------------------------------------------------------+*/
  Widget w;
  Position x, y;
  Dimension width, height;
{
    XwTextSink *sink = ((XwTextEditWidget)w)->text.sink;
    AsciiSinkData *data = (AsciiSinkData *) sink->data;
    XFillRectangle(XtDisplay(w), XtWindow(w), data->invgc, x, y, width, height);
}

/*
 * Given two positions, find the distance between them.
 */

/*--------------------------------------------------------------------------+*/
static AsciiFindDistance (w, fromPos, fromx, toPos,
			  resWidth, resPos, resHeight)
/*--------------------------------------------------------------------------+*/
  Widget w;
  XwTextPosition fromPos;	/* First position. */
  int fromx;			/* Horizontal location of first position. */
  XwTextPosition toPos;		/* Second position. */
  int *resWidth;		/* Distance between fromPos and resPos. */
  int *resPos;			/* Actual second position used. */
  int *resHeight;		/* Height required. */
{
    XwTextSink *sink = ((XwTextEditWidget)w)->text.sink;
    XwTextSource *source = ((XwTextEditWidget)w)->text.source;
    int margin = ((XwTextEditWidget)w)->text.leftmargin ;

    AsciiSinkData *data;
    register    XwTextPosition index, lastPos;
    register unsigned char   c;
    XwTextBlock blk;

    data = (AsciiSinkData *) sink->data;
    /* we may not need this */
    lastPos = GETLASTPOS;
    (*(source->read))(source, fromPos, &blk, toPos - fromPos);
    *resWidth = 0;
    for (index = fromPos; index != toPos && index < lastPos; index++) {
	if (index - blk.firstPos >= blk.length)
	    (*(source->read))(source, index, &blk, toPos - fromPos);
	c = blk.ptr[index - blk.firstPos];
	if (c == LF) {
	    *resWidth += CharWidth(data, fromx + *resWidth, margin, SP);
	    index++;
	    break;
	}
	*resWidth += CharWidth(data, fromx + *resWidth, margin, c);
    }
    *resPos = index;
    *resHeight = data->font->ascent + data->font->descent;
}


/*--------------------------------------------------------------------------+*/
static TextFit AsciiTextFit (w, fromPos, fromx, width, wrap, wrapWhiteSpace,
			     fitPos, drawPos, nextPos, resWidth, resHeight)
/*--------------------------------------------------------------------------+*/
  Widget w;
  XwTextPosition fromPos; 	/* Starting position. */
  int fromx;			/* Horizontal location of starting position. */
  int width;			/* Desired width. */
  int wrap;			/* Whether line should wrap at all */
  int wrapWhiteSpace;		/* Whether line should wrap at white space */

  XwTextPosition *fitPos ;	/* pos of last char which fits in specified
				   width */
  XwTextPosition *drawPos ;	/* pos of last char to draw in specified
				   width based on wrap model */
  XwTextPosition *nextPos ;	/* pos of next char to draw outside specified
				   width based on wrap model */
  int *resWidth;		/* Actual width used. */
  int *resHeight;		/* Height required. */
{
    XwTextSink *sink = ((XwTextEditWidget)w)->text.sink;
    XwTextSource *source = ((XwTextEditWidget)w)->text.source;
    int margin = ((XwTextEditWidget)w)->text.leftmargin ;
    AsciiSinkData *data;
    XwTextPosition lastPos, pos, whiteSpacePosition;
    XwTextPosition fitL, drawL ;
    		/* local equivalents of fitPos, drawPos, nextPos */
    int     lastWidth, whiteSpaceWidth, whiteSpaceSeen ;
    int useAll ;
    TextFit fit ;
    unsigned char    c;
    XwTextBlock blk;
    
    data = (AsciiSinkData *) sink->data;
    lastPos = GETLASTPOS;

    *resWidth = 0;
    *fitPos = fromPos ;
    *drawPos = -1 ;
    c = 0;
    useAll = whiteSpaceSeen = FALSE ;

    pos = fromPos ;
    fitL = pos - 1 ;
    (*(source->read))(source, fromPos, &blk, bufferSize);

    while (*resWidth <= width)
      {	lastWidth = *resWidth;
	fitL = pos - 1 ;
	if (pos >= lastPos)
	  { pos = lastPos ;
	    fit = tfEndText ;
	    useAll = TRUE ;
	    break ;
	  } ;
	if (pos - blk.firstPos >= blk.length)
	  (*(source->read))(source, pos, &blk, bufferSize);
	c = blk.ptr[pos - blk.firstPos];

	if (isNewline(c))
	  { fit = tfNewline ;
	    useAll = TRUE ;
	    break ;
	  }

	if (wrapWhiteSpace && isWhiteSpace(c))
	  { whiteSpaceSeen = TRUE ;
	    drawL = pos - 1 ;
	    whiteSpaceWidth = *resWidth;
	  } ;
	  
	*resWidth += CharWidth(data, fromx + *resWidth, margin, c);
	pos++ ;
    } /* end while */

    *fitPos = fitL ;
    *drawPos = fitL ;
    if (useAll)
      {
	*nextPos = pos + 1 ;
	*resWidth = lastWidth ;
      }
    else if (wrapWhiteSpace && whiteSpaceSeen)
      { *drawPos = drawL ;
	*nextPos = drawL + 2 ;
	*resWidth = whiteSpaceWidth ;
	fit = tfWrapWhiteSpace ;
      }
    else if (wrap)
      {
	*nextPos = fitL + 1 ;
	*resWidth = lastWidth ;
	fit = tfWrapAny ;
      }
    else
      {
	/* scan source for newline or end */
	*nextPos =
	  (*(source->scan)) (source, pos, XwstEOL, XwsdRight, 1, TRUE) + 1 ;
	*resWidth = lastWidth ;
	fit = tfNoFit ;
      }
    *resHeight = data->font->ascent + data->font->descent;
    return (fit) ;
}

/*--------------------------------------------------------------------------+*/
static AsciiFindPosition(w, fromPos, fromx, width, stopAtWordBreak, 
			 resPos, resWidth, resHeight)
/*--------------------------------------------------------------------------+*/
  Widget w;
  XwTextPosition fromPos; 	/* Starting position. */
  int fromx;			/* Horizontal location of starting position. */
  int width;			/* Desired width. */
  int stopAtWordBreak;		/* Whether the resulting position should be at
				   a word break. */
  XwTextPosition *resPos;	/* Resulting position. */
  int *resWidth;		/* Actual width used. */
  int *resHeight;		/* Height required. */
{
    XwTextSink *sink = ((XwTextEditWidget)w)->text.sink;
    XwTextSource *source = ((XwTextEditWidget)w)->text.source;
    int margin = ((XwTextEditWidget)w)->text.leftmargin ;
    AsciiSinkData *data;
    XwTextPosition lastPos, index, whiteSpacePosition;
    int     lastWidth, whiteSpaceWidth;
    Boolean whiteSpaceSeen;
    unsigned char c;
    XwTextBlock blk;
    data = (AsciiSinkData *) sink->data;
    lastPos = GETLASTPOS;

    (*(source->read))(source, fromPos, &blk, bufferSize);
    *resWidth = 0;
    whiteSpaceSeen = FALSE;
    c = 0;
    for (index = fromPos; *resWidth <= width && index < lastPos; index++) {
	lastWidth = *resWidth;
	if (index - blk.firstPos >= blk.length)
	    (*(source->read))(source, index, &blk, bufferSize);
	c = blk.ptr[index - blk.firstPos];
	if (c == LF) {
	    *resWidth += CharWidth(data, fromx + *resWidth, margin, SP);
	    index++;
	    break;
	}
	*resWidth += CharWidth(data, fromx + *resWidth, margin, c);
	if ((c == SP || c == TAB) && *resWidth <= width) {
	    whiteSpaceSeen = TRUE;
	    whiteSpacePosition = index;
	    whiteSpaceWidth = *resWidth;
	}
    }
    if (*resWidth > width && index > fromPos) {
	*resWidth = lastWidth;
	index--;
	if (stopAtWordBreak && whiteSpaceSeen) {
	    index = whiteSpacePosition + 1;
	    *resWidth = whiteSpaceWidth;
	}
    }
    if (index == lastPos && c != LF) index = lastPos + 1;
    *resPos = index;
    *resHeight = data->font->ascent + data->font->descent;
}


/*--------------------------------------------------------------------------+*/
static int AsciiResolveToPosition (w, pos, fromx, width,
				   leftPos, rightPos)
/*--------------------------------------------------------------------------+*/
  Widget w;
  XwTextPosition pos;
  int fromx,width;
  XwTextPosition *leftPos, *rightPos;
{
    int     resWidth, resHeight;
    XwTextSink *sink = ((XwTextEditWidget)w)->text.sink;
    XwTextSource *source = ((XwTextEditWidget)w)->text.source;

    AsciiFindPosition(w, pos, fromx, width, FALSE,
	    leftPos, &resWidth, &resHeight);
    if (*leftPos > GETLASTPOS)
	*leftPos = GETLASTPOS;
    *rightPos = *leftPos;
}


/*--------------------------------------------------------------------------+*/
static int AsciiMaxLinesForHeight (w)
/*--------------------------------------------------------------------------+*/
  Widget w;
{
    AsciiSinkData *data;
    XwTextSink *sink = ((XwTextEditWidget)w)->text.sink;

    data = (AsciiSinkData *) sink->data;
    return (int) ((w->core.height
		   - ((XwTextEditWidget) w)->text.topmargin
		   - ((XwTextEditWidget) w)->text.bottommargin
		   )
		  / (data->font->ascent + data->font->descent)
		  );
}


/*--------------------------------------------------------------------------+*/
static int AsciiMaxHeightForLines (w, lines)
/*--------------------------------------------------------------------------+*/
  Widget w;
  int lines;
{
    AsciiSinkData *data;
    XwTextSink *sink = ((XwTextEditWidget)w)->text.sink;

    data = (AsciiSinkData *) sink->data;
    return(lines * (data->font->ascent + data->font->descent));
}


/***** Public routines *****/

static Boolean initialized = FALSE;
static XContext asciiSinkContext;

/*--------------------------------------------------------------------------+*/
void AsciiSinkInitialize()
/*--------------------------------------------------------------------------+*/
{
    if (initialized)
    	return;
    initialized = TRUE;

    asciiSinkContext = XUniqueContext();

    buf = (unsigned char *) XtMalloc(bufferSize);
}

/*--------------------------------------------------------------------------+*/
static Boolean XwAsciiSinkCheckData(self)
/*--------------------------------------------------------------------------+*/
  XwTextSink *self ;
{
  XwTextEditWidget tew = self->parent ;
  
  /* make sure margins are big enough to keep traversal highlight
     from obscuring text or cursor.
     */
  { int minBorder ;
    minBorder = (tew->primitive.traversal_type == XwHIGHLIGHT_OFF)
                  ? RequiredCursorMargin
		  : RequiredCursorMargin + tew->primitive.highlight_thickness ;

    if (tew->text.topmargin < minBorder)
      tew->text.topmargin = minBorder ;
    if (tew->text.bottommargin < minBorder)
      tew->text.bottommargin = minBorder ;
    if (tew->text.rightmargin < minBorder)
      tew->text.rightmargin = minBorder ;
    if (tew->text.leftmargin < minBorder)
      tew->text.leftmargin = minBorder ;
  }

  if ((*(self->maxLines))(tew) < 1)
    XtWarning("TextEdit window too small to display a single line of text.");

}

/*--------------------------------------------------------------------------+*/
void XwAsciiSinkDestroy (sink)
/*--------------------------------------------------------------------------+*/
    XwTextSink *sink;
{
    AsciiSinkData *data;
    data = (AsciiSinkData *) sink->data;
    XtFree((char *) data);
    XtFree((char *) sink);
}

/*--------------------------------------------------------------------------+*/
XwTextSink *XwAsciiSinkCreate (w, args, num_args)
/*--------------------------------------------------------------------------+*/
    Widget w;
    ArgList 	args;
    Cardinal 	num_args;
{
    XwTextSink *sink;
    AsciiSinkData *data;
    unsigned long valuemask = (GCFont | GCGraphicsExposures |
			       GCForeground | GCBackground | GCFunction);
    XGCValues values;
    unsigned long wid;
    XFontStruct *font;

    if (!initialized)
    	AsciiSinkInitialize();

    sink                    = XtNew(XwTextSink);
    sink->parent            = (XwTextEditWidget) w ; 
    sink->parent->text.sink = sink ;		/* disgusting */
    sink->display           = AsciiDisplayText;
    sink->insertCursor      = AsciiInsertCursor;
    sink->clearToBackground = AsciiClearToBackground;
    sink->findPosition      = AsciiFindPosition;
    sink->textFitFn         = AsciiTextFit;
    sink->findDistance      = AsciiFindDistance;
    sink->resolve           = AsciiResolveToPosition;
    sink->maxLines          = AsciiMaxLinesForHeight;
    sink->maxHeight         = AsciiMaxHeightForLines;
    sink->resources         = SinkResources;
    sink->resource_num      = XtNumber(SinkResources);
    sink->check_data        = XwAsciiSinkCheckData;
    sink->destroy           = XwAsciiSinkDestroy;
    sink->LineLastWidth	    = 0 ;
    sink->LineLastPosition  = 0 ;
    data                    = XtNew(AsciiSinkData);
    sink->data              = (int *)data;

    XtGetSubresources (w, (caddr_t)data, "display", "Display", 
		       SinkResources, XtNumber(SinkResources),
		       args, num_args);

/* XXX do i have to XLoadQueryFont or does the resource guy do it for me */

    font = data->font;
    values.function = GXcopy;
    values.font = font->fid;
    values.graphics_exposures = (Bool) FALSE;
    values.foreground = data->foreground;
    values.background = w->core.background_pixel;
    data->normgc = XtGetGC(w, valuemask, &values);
    values.foreground = w->core.background_pixel;
    values.background = data->foreground;
    data->invgc = XtGetGC(w, valuemask, &values);
    values.function = GXxor;
    values.foreground = data->foreground ^ w->core.background_pixel;
    values.background = 0;
    data->xorgc = XtGetGC(w, valuemask, &values);

    wid = -1;
    if ((!XGetFontProperty(font, XA_QUAD_WIDTH, &wid)) || wid <= 0) {
	if (font->per_char && font->min_char_or_byte2 <= '0' &&
	    		      font->max_char_or_byte2 >= '0')
	    wid = font->per_char['0' - font->min_char_or_byte2].width;
	else
	    wid = font->max_bounds.width;
    }
    if (wid <= 0) wid = 1;
    data->tabwidth = 8 * wid;
    data->font = font;

/*    data->insertCursorOn = CreateInsertCursor(XtScreen(w)); */
    
    {/* Correction from AsciiSink.c on R2 tape */
      Screen *screen = XtScreen(w);
      Display *dpy = XtDisplay(w);
      Window root = RootWindowOfScreen(screen);
      Pixmap bitmap = XCreateBitmapFromData(dpy, root, insertCursor_bits,
					    insertCursor_width,
					    insertCursor_height);
      Pixmap pixmap = XCreatePixmap(dpy, root,insertCursor_width,
				    insertCursor_height,
				    DefaultDepthOfScreen(screen));
      XGCValues gcv;
      GC gc;
      
      gcv.function = GXcopy;
      gcv.foreground = data->foreground ^ w->core.background_pixel;
      gcv.background = 0;
      gcv.graphics_exposures = False;
      gc = XtGetGC(w, (GCFunction | GCForeground | GCBackground |
		       GCGraphicsExposures), &gcv);
      XCopyPlane(dpy, bitmap, pixmap, gc, 0, 0, insertCursor_width,
		 insertCursor_height, 0, 0, 1);
      XtDestroyGC(gc);
      data->insertCursorOn = pixmap;
    }

    data->laststate = XwisOff;
    (*(sink->check_data))(sink);

    return sink;
}

