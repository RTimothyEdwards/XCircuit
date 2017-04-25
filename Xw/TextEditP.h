/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        TextEditP.h
 **
 **   Project:     X Widgets
 **
 **   Description: TextEdit widget private include file
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

#ifndef _XtTextEditPrivate_h
#define _XtTextEditPrivate_h


/****************************************************************
 *
 * TextEdit widget private
 *
 ****************************************************************/

#define MAXCUT	30000	/* Maximum number of characters that can be cut. */

#define LF	0x0a
#define CR	0x0d
#define TAB	0x09
#define BS	0x08
#define SP	0x20
#define DEL	0x7f
#define BSLASH	'\\'

#define isNewline(c) (c=='\n')
#define isWhiteSpace(c) ( c==' ' || c=='\t' || c=='\n' || c=='\r' )

#include <Xw/TextEdit.h>

typedef int XwTextLineRange ;

#include "SourceP.h"
#include "DisplayP.h"

/* NOTE:  more of the following information will eventually be moved
   to the files TESourceP.h and TEDisplayP.h.
   */
  
/* Private TextEdit Definitions */

typedef int (*ActionProc)();

typedef XwSelectType SelectionArray[20];

typedef struct {
  unsigned char *string;
  int value;
} XwSetValuePair;
  

/*************************************************************************
*
*  New fields for the TextEdit widget class record
*
************************************************************************/
typedef struct {
  unsigned char*  (*copy_substring)();
  unsigned char*  (*copy_selection)();
  XtWidgetProc    unset_selection;
  XwSetSProc      set_selection;
  XwEditResult    (*replace_text)();
  XtWidgetProc    redraw_text;
} XwTextEditClassPart;

/*************************************************************************
*
* Full class record declaration for TextEdit class
*
************************************************************************/
typedef struct _XwTextEditClassRec {
    CoreClassPart	 core_class;
    XwPrimitiveClassPart primitive_class;
    XwTextEditClassPart	 textedit_class;
} XwTextEditClassRec;

extern XwTextEditClassRec XwtexteditClassRec; 

/*************************************************************************
*
* New fields for the TextEdit widget instance record 
*
************************************************************************/
typedef struct _XwTextEditPart {
    XwSourceType    srctype;        /* used by args & rm to set source */
    XwTextSource    *source;
    XwTextSink	    *sink;
    XwLineTable     lt;
    XwTextPosition  insertPos;
    XwTextPosition  oldinsert;
    XwTextSelection s;
    XwScanDirection extendDir;
    XwTextSelection origSel;        /* the selection being modified */
    SelectionArray  sarray;         /* Array to cycle for selections. */
    Dimension       leftmargin;     /* Width of left margin. */
    Dimension       rightmargin;    /* Width of right margin. */
    Dimension       topmargin;      /* Width of top margin. */
    Dimension       bottommargin;   /* Width of bottom margin. */
    int             options;        /* wordbreak, scroll, etc. */
    Time            lasttime;       /* timestamp of last processed action */
    Time            time;           /* time of last key or button action */ 
    Position        ev_x, ev_y;     /* x, y coords for key or button action */
    XwTextPosition  *updateFrom;    /* Array of start positions for update. */
    XwTextPosition  *updateTo;      /* Array of end positions for update. */
    int             numranges;      /* How many update ranges there are. */
    int             maxranges;      /* How many ranges we have space for */
    Boolean         showposition;   /* True if we need to show the position. */
    GC              gc;
    Boolean         hasfocus;       /* TRUE if we currently have input focus.*/
    XtCallbackList  motion_verification;
    XtCallbackList  modify_verification;
    XtCallbackList  leave_verification;
    XwWrap	    wrap_mode;
    XwWrapForm      wrap_form;
    XwWrapBreak     wrap_break;
    XwScroll        scroll_mode;
    XwScroll        scroll_state;
    XwGrow          grow_mode;
    XwGrow          grow_state;     /* tells whether further growth should
				       be attempted */
    Dimension	    prevW, prevH;   /* prev values of window width, height */
    Boolean         visible_insertion ;
    Boolean         update_flag ;   /* turn updates on and off */
    XtCallbackList execute ;        /* Execute callback list */
} XwTextEditPart;

/****************************************************************
 *
 * Full instance record declaration
 *
****************************************************************/

typedef struct _XwTextEditRec {
  CorePart	   core;
  XwPrimitivePart  primitive;
  XwTextEditPart   text;
} XwTextEditRec;


#endif
/* DON'T ADD STUFF AFTER THIS #endif */
