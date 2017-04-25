/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        SourceStr.c
 **
 **   Project:     X Widgets
 **
 **   Description: Code for TextEdit widget string source
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

#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <Xw/TextEdit.h>
#include <Xw/TextEditP.h>
#include <Xw/SourceP.h>

/* Private StringSource Definitions */

#define MAGICVALUE		-1
#define DEFAULTBUFFERSIZE        512


#define Increment(data, position, direction)\
{\
    if (direction == XwsdLeft) {\
	if (position > 0) \
	    position -= 1;\
    }\
    else {\
	if (position < data->length)\
	    position += 1;\
    }\
}

static long magic_value = MAGICVALUE;

static XtResource stringResources[] = {
    {XtNstring, XtCString, XtRString, sizeof (char *),
        XtOffset(StringSourcePtr, initial_string), XtRString, NULL},
    {XtNmaximumSize, XtCMaximumSize, XtRLong, sizeof (long),
        XtOffset(StringSourcePtr, max_size), XtRLong, (caddr_t)&magic_value},
    {XtNeditType, XtCEditType, XtREditMode, sizeof(XwEditType), 
        XtOffset(StringSourcePtr, editMode), XtRString, "edit"},
};

/*--------------------------------------------------------------------------+*/
static unsigned char Look(data, position, direction)
/*--------------------------------------------------------------------------+*/
  StringSourcePtr data;
  XwTextPosition  position;
  XwScanDirection direction;
{
/* Looking left at pos 0 or right at position data->length returns newline */
    if (direction == XwsdLeft) {
	if (position == 0)
	    return(0);
	else
	    return(data->buffer[position-1]);
    }
    else {
	if (position == data->length)
	    return(0);
	else
	    return(data->buffer[position]);
    }
}

/*--------------------------------------------------------------------------+*/
static int StringReadText (src, pos, text, maxRead)
/*--------------------------------------------------------------------------+*/
  XwTextSource *src;
  int pos;
  XwTextBlock *text;
  int maxRead;
{
    int     charsLeft;
    StringSourcePtr data;

    data = (StringSourcePtr) src->data;
    text->firstPos = pos;
    text->ptr = data->buffer + pos;
    charsLeft = data->length - pos;
    text->length = (maxRead > charsLeft) ? charsLeft : maxRead;
    return pos + text->length;
}

/*--------------------------------------------------------------------------+*/
static XwEditResult StringReplaceText (src, startPos, endPos, text, delta)
/*--------------------------------------------------------------------------+*/
  XwTextSource *src;
  XwTextPosition startPos, endPos;
  XwTextBlock *text;
  int *delta;
{
    StringSourcePtr data;
    int     i, length;

    data = (StringSourcePtr) src->data;
    switch (data->editMode) {
        case XwtextAppend: 
	    if (startPos != endPos || endPos!= data->length)
		return (XweditPosError);
	    break;
	case XwtextRead:
	    return (XweditError);
	case XwtextEdit:
	    break;
	default:
	    return (XweditError);
    }
    length = endPos - startPos;
    *delta = text->length - length;
    if ((data->length + *delta) > data->buffer_size) {
      if (data->max_size_flag)
	return (XweditError);
      else {
	while ((data->length + *delta) > data->buffer_size)
	  data->buffer_size += DEFAULTBUFFERSIZE;
	data->buffer = (unsigned char *)
	  XtRealloc(data->buffer, data->buffer_size);
      }
    };
    

    if (*delta < 0)		/* insert shorter than delete, text getting
				   shorter */
	for (i = startPos; i < data->length + *delta; ++i)
	    data->buffer[i] = data->buffer[i - *delta];
    else
	if (*delta > 0)	{	/* insert longer than delete, text getting
				   longer */
	    for (i = data->length; i > startPos-1; --i)
		data->buffer[i + *delta] = data->buffer[i];
	}
    if (text->length != 0)	/* do insert */
	for (i = 0; i < text->length; ++i)
	    data->buffer[startPos + i] = text->ptr[i];
    data->length = data->length + *delta;
    data->buffer[data->length] = 0;
    return (XweditDone);
}

/*--------------------------------------------------------------------------+*/
static StringSetLastPos (src, lastPos)
/*--------------------------------------------------------------------------+*/
  XwTextSource *src;
  XwTextPosition lastPos;
{
    ((StringSourceData *) (src->data))->length = lastPos;
}

/*--------------------------------------------------------------------------+*/
static XwTextPosition StringGetLastPos (src)
/*--------------------------------------------------------------------------+*/
  XwTextSource *src;
{
    return( ((StringSourceData *) (src->data))->length );
}

/*--------------------------------------------------------------------------+*/
static XwTextPosition StringScan (src, pos, sType, dir, count, include)
/*--------------------------------------------------------------------------+*/
  XwTextSource	  *src;
  XwTextPosition  pos;
  XwScanType	  sType;
  XwScanDirection dir;
  int		  count;
  Boolean	  include;
{
    StringSourcePtr data;
    XwTextPosition position;
    int     i, whiteSpace;
    unsigned char c;
    int ddir = (dir == XwsdRight) ? 1 : -1;

    data = (StringSourcePtr) src->data;
    position = pos;
    switch (sType) {
	case XwstPositions: 
	    if (!include && count > 0)
		count -= 1;
	    for (i = 0; i < count; i++) {
		Increment(data, position, dir);
	    }
	    break;
	case XwstWhiteSpace: 

	    for (i = 0; i < count; i++) {
		whiteSpace = -1;
		while (position >= 0 && position <= data->length) {
		    c = Look(data, position, dir);
		    if ((c == ' ') || (c == '\t') || (c == '\n')){
		        if (whiteSpace < 0) whiteSpace = position;
		    } else if (whiteSpace >= 0)
			break;
		    position += ddir;
		}
	    }
	    if (!include) {
		if(whiteSpace < 0 && dir == XwsdRight) whiteSpace = data->length;
		position = whiteSpace;
	    }
	    break;
	case XwstEOL: 
	    for (i = 0; i < count; i++) {
		while (position >= 0 && position <= data->length) {
		    if (Look(data, position, dir) == '\n')
			break;
		    if(((dir == XwsdRight) && (position == data->length)) || 
			(dir == XwsdLeft) && ((position == 0)))
			break;
		    Increment(data, position, dir);
		}
		if (i + 1 != count)
		    Increment(data, position, dir);
	    }
	    if (include) {
	    /* later!!!check for last char in file # eol */
		Increment(data, position, dir);
	    }
	    break;
	case XwstLast: 
	    if (dir == XwsdLeft)
		position = 0;
	    else
		position = data->length;
    }
    if (position < 0) position = 0;
    if (position > data->length) position = data->length;
    return(position);
}

/*--------------------------------------------------------------------------+*/
static XwEditType StringGetEditType(src)
/*--------------------------------------------------------------------------+*/
  XwTextSource *src;
{
    StringSourcePtr data;
    data = (StringSourcePtr) src->data;
    return(data->editMode);
} 

/*--------------------------------------------------------------------------+*/
static Boolean XwStringSourceCheckData(src)
/*--------------------------------------------------------------------------+*/
    XwTextSource *src;
{   int  initial_size = 0;
    StringSourcePtr data = (StringSourcePtr)src->data;

    data->max_size_flag = (data->max_size != MAGICVALUE);
    
    if (data->initial_string == NULL) {
	if (data->max_size_flag) 
	  data->buffer_size = data->max_size;
	else 
	  data->buffer_size = DEFAULTBUFFERSIZE;
	if (!data->buffer) data->length = 0;
      }
    else {
      initial_size = XwStrlen(data->initial_string);
      if (data->max_size_flag) {
	if (data->max_size < initial_size) {
	  XtWarning("Initial string size larger than XtNmaximumSize");
	  data->max_size = initial_size;
	}
	data->buffer_size = data->max_size;
      }
      else {
	data->buffer_size =
	  (initial_size < DEFAULTBUFFERSIZE) ? DEFAULTBUFFERSIZE :
	    ((initial_size / DEFAULTBUFFERSIZE) + 1) * DEFAULTBUFFERSIZE;
      };
      data->length = initial_size;
    };

    if (data->buffer && initial_size) 
      data->buffer =
	(unsigned char *) XtRealloc(data->buffer, data->buffer_size);
    else if (!data->buffer)
      data->buffer = (unsigned char *) XtMalloc(data->buffer_size);
    
    if (initial_size) {
      strcpy(data->buffer, data->initial_string);
      data->initial_string = NULL;
    }
}  

/***** Public routines *****/

/*--------------------------------------------------------------------------+*/
void XwStringSourceDestroy (src)
/*--------------------------------------------------------------------------+*/
  XwTextSource *src;
{
    XtFree((char *) src->data);
    XtFree((char *) src);
}

/*--------------------------------------------------------------------------+*/
XwTextSource *XwStringSourceCreate (w, args, argCount)
/*--------------------------------------------------------------------------+*/
    Widget w;
    ArgList args;
    int     argCount;
{
    XwTextSource *src;
    StringSourcePtr data;


    src = XtNew(XwTextSource);
    src->read = StringReadText;
    src->replace = StringReplaceText;
    src->setLastPos = StringSetLastPos;
    src->getLastPos = StringGetLastPos;    
    src->scan = StringScan;
    src->editType = StringGetEditType;
    src->resources = stringResources;
    src->resource_num = XtNumber(stringResources);
    src->check_data = XwStringSourceCheckData;
    src->destroy = XwStringSourceDestroy;
    data = XtNew(StringSourceData);
    data->editMode = XwtextRead;
    data->buffer = NULL;
    data->initial_string = NULL;
    data->length = 0;
    data->buffer_size = 0;
    data->max_size = 0;
    data->max_size_flag = 0;
    src->data = (int *)data;

    /* Use the name given to the TextEdit widget this source will go with.
       This could be a problem if we allow multiple views on one source */

    XtGetSubresources (w, data, XtNstringSrc, "StringSrc",
        stringResources, XtNumber(stringResources), args, argCount);

    src->data = (int *) (data);

    XwStringSourceCheckData(src);

    return src;
}

