/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        TextEdit.c
 **
 **   Project:     X Widgets
 **
 **   Description: Code for TextEdit widget
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

#include <stdio.h>
#ifdef SYSV
#include <fcntl.h>
#include <string.h>
#else
#include <strings.h>
#endif	/* SYSV */
#include <X11/Xos.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>

#include <Xw/TextEdit.h>
#include <Xw/TextEditP.h>
#include <X11/Xutil.h>  /* This include depends on something above it */

#define BufMax 1000
#define abs(x)	(((x) < 0) ? (-(x)) : (x))
#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))
#define GETLASTPOS(ctx)  ((*(ctx->text.source->getLastPos)) (ctx->text.source))
                    
#define BUTTONMASK 0x143d

#define zeroPosition ((XwTextPosition) 0)

/******************************************************************************
*
*  Resources for TextEdit
*
******************************************************************************/

static XtResource resources[] = {
/* Resources from other classes with new defaults */
  {XtNheight,
     XtCHeight,
     XtRDimension,
     sizeof(Dimension),
     XtOffset(XwTextEditWidget, core.height),
     XtRString,
     "200"},
  {XtNwidth,
     XtCWidth,
     XtRDimension,
     sizeof(Dimension),
     XtOffset(XwTextEditWidget, core.width),
     XtRString,
     "200"},
/* New resources for TextEdit */
  {XtNdisplayPosition,
     XtCTextPosition, 
     XtRInt,
     sizeof (XwTextPosition), 
     XtOffset(XwTextEditWidget, text.lt.top), 
     XtRString, "0"},
  {XtNinsertPosition,
     XtCTextPosition, 
     XtRInt, 
     sizeof(XwTextPosition), 
     XtOffset(XwTextEditWidget, text.insertPos), 
     XtRString,
     "0"},
  {XtNleftMargin,
     XtCMargin,
     XtRDimension,
     sizeof (Dimension), 
     XtOffset(XwTextEditWidget, text.leftmargin),
     XtRString, 
     "2"},
  {XtNrightMargin,
     XtCMargin,
     XtRDimension,
     sizeof (Dimension), 
     XtOffset(XwTextEditWidget, text.rightmargin),
     XtRString, 
     "2"},
  {XtNtopMargin,
     XtCMargin,
     XtRDimension,
     sizeof (Dimension), 
     XtOffset(XwTextEditWidget, text.topmargin),
     XtRString, 
     "2"},
  {XtNbottomMargin,
     XtCMargin,
     XtRDimension,
     sizeof (Dimension), 
     XtOffset(XwTextEditWidget, text.bottommargin),
     XtRString, 
     "2"},
  {XtNsourceType,
     XtCSourceType,
     XtRSourceType,	
     sizeof(XwSourceType),
     XtOffset(XwTextEditWidget, text.srctype),
     XtRString,
     "stringsrc"}, 
  {XtNtextSource, 
     XtCTextSource,
     XtRPointer,
     sizeof (caddr_t), 
     XtOffset(XwTextEditWidget, text.source), 
     XtRPointer, 
     NULL},
  {XtNselection,
     XtCSelection, 
     XtRPointer, 
     sizeof(caddr_t),
     XtOffset(XwTextEditWidget, text.s),
     XtRPointer, 
     NULL},
  {XtNmotionVerification,
     XtCCallback,
     XtRCallback,
     sizeof(XtCallbackProc),
     XtOffset(XwTextEditWidget, text.motion_verification),
     XtRCallback,
     (caddr_t) NULL},
  {XtNmodifyVerification,
     XtCCallback,
     XtRCallback,
     sizeof(XtCallbackProc),
     XtOffset(XwTextEditWidget, text.modify_verification),
     XtRCallback,
     (caddr_t) NULL},
  {XtNleaveVerification,
     XtCCallback,
     XtRCallback,
     sizeof(XtCallbackProc),
     XtOffset(XwTextEditWidget, text.leave_verification),
     XtRCallback,
     (caddr_t) NULL},
  {XtNexecute,
     XtCCallback,
     XtRCallback,
     sizeof(XtCallbackProc),
     XtOffset(XwTextEditWidget, text.execute),
     XtRCallback,
     (caddr_t) NULL},
  {XtNwrap,		
     XtCWrap,
     XtRWrap,
     sizeof (XwWrap),
     XtOffset (XwTextEditWidget, text.wrap_mode),
     XtRString,
     "softwrap"},
  {XtNwrapForm,		
     XtCWrapForm,
     XtRWrapForm,
     sizeof (XwWrapForm),
     XtOffset (XwTextEditWidget, text.wrap_form),
     XtRString,
     "sourceform"},
  {XtNwrapBreak,	
     XtCWrapBreak,
     XtRWrapBreak,
     sizeof (XwWrapBreak),
     XtOffset (XwTextEditWidget, text.wrap_break),
     XtRString,
     "wrapwhitespace"},
  {XtNscroll,		
     XtCScroll,
     XtRScroll,
     sizeof (XwScroll),
     XtOffset (XwTextEditWidget, text.scroll_mode),
     XtRString,
     "autoscrolloff"},
  {XtNgrow,		
     XtCGrow,
     XtRGrow,
     sizeof (XwGrow),
     XtOffset (XwTextEditWidget, text.grow_mode),
     XtRString,
     "growoff"},
};


/******************************************************************************
*
*  Forward declarations of functions to put into the class record
*
******************************************************************************/
static void Initialize();
static void Realize();
static void TextDestroy();
static void Resize();
static void ProcessExposeRegion();
static Boolean SetValues();
static unsigned char* _XwTextCopySubString();

/******************************************************************************
*
*  Forward declarations of action functions to put into the action table
*
******************************************************************************/
static void TraverseUp();
static void TraverseDown();
static void TraverseLeft();
static void TraverseRight();
static void TraverseNext();
static void TraversePrev();
static void TraverseHome();
static void TraverseNextTop();
static void Enter();
static void Leave();
static void Execute();
static void RedrawDisplay();
static void InsertChar();
static void TextFocusIn();
static void TextFocusOut();
static void MoveForwardChar();
static void MoveBackwardChar();
static void MoveForwardWord();
static void MoveBackwardWord();
static void MoveForwardParagraph();
static void MoveBackwardParagraph();
static void MoveToLineStart();
static void MoveToLineEnd();
static void MoveNextLine();
static void MovePreviousLine();
static void MoveNextPage();
static void MovePreviousPage();
static void MoveBeginningOfFile();
static void MoveEndOfFile();
static void ScrollOneLineUp();
static void ScrollOneLineDown();
static void DeleteForwardChar();
static void DeleteBackwardChar();
static void DeleteBackwardNormal();
static void DeleteForwardWord();
static void DeleteBackwardWord();
static void DeleteCurrentSelection();
static void KillForwardWord();
static void KillBackwardWord();
static void KillCurrentSelection();
static void KillToEndOfLine();
static void KillToEndOfParagraph();
static void UnKill();
static void Stuff();
static void InsertNewLineAndIndent();
static void InsertNewLineAndBackup();
static XwEditResult InsertNewLine();
static void SelectWord();
static void SelectAll();
static void SelectStart();
static void SelectAdjust();
static void SelectEnd();
static void ExtendStart();
static void ExtendAdjust();
static void ExtendEnd();
       
/******************************************************************************
*
* TextEdit Actions Table 
*
******************************************************************************/
XtActionsRec texteditActionsTable [] = {
/* motion bindings */
  {"forward-character", 	MoveForwardChar},
  {"backward-character", 	MoveBackwardChar},
  {"forward-word", 		MoveForwardWord},
  {"backward-word", 		MoveBackwardWord},
  {"forward-paragraph", 	MoveForwardParagraph},
  {"backward-paragraph", 	MoveBackwardParagraph},
  {"beginning-of-line", 	MoveToLineStart},
  {"end-of-line", 		MoveToLineEnd},
  {"next-line", 		MoveNextLine},
  {"previous-line", 		MovePreviousLine},
  {"next-page", 		MoveNextPage},
  {"previous-page", 		MovePreviousPage},
  {"beginning-of-file", 	MoveBeginningOfFile},
  {"end-of-file", 		MoveEndOfFile},
  {"scroll-one-line-up", 	ScrollOneLineUp},
  {"scroll-one-line-down", 	ScrollOneLineDown},
/* delete bindings */
  {"delete-next-character", 	DeleteForwardChar},
  {"delete-previous-character", DeleteBackwardNormal},
  {"delete-next-word", 		DeleteForwardWord},
  {"delete-previous-word", 	DeleteBackwardWord},
  {"delete-selection", 		DeleteCurrentSelection},
/* kill bindings */
  {"kill-word", 		KillForwardWord},
  {"backward-kill-word", 	KillBackwardWord},
  {"kill-selection", 		KillCurrentSelection},
  {"kill-to-end-of-line", 	KillToEndOfLine},
  {"kill-to-end-of-paragraph", 	KillToEndOfParagraph},
/* unkill bindings */
  {"unkill", 			UnKill},
  {"stuff", 			Stuff},
/* new line stuff */
  {"newline-and-indent", 	InsertNewLineAndIndent},
  {"newline-and-backup", 	InsertNewLineAndBackup},
  {"newline", 			(XtActionProc)InsertNewLine},
/* Selection stuff */
  {"select-word", 		SelectWord},
  {"select-all", 		SelectAll},
  {"select-start", 		SelectStart},
  {"select-adjust", 		SelectAdjust},
  {"select-end", 		SelectEnd},
  {"extend-start", 		ExtendStart},
  {"extend-adjust", 		ExtendAdjust},
  {"extend-end", 		ExtendEnd},
/* Miscellaneous */
  {"redraw-display", 		RedrawDisplay},
  {"insert-char", 		InsertChar},
  {"focus-in", 	 	        TextFocusIn},
  {"focus-out", 		TextFocusOut},
/* traversal direction functions */
  {"traverse-left", 		TraverseLeft},
  {"traverse-right", 		TraverseRight},
  {"traverse-next", 		TraverseNext},
  {"traverse-prev", 		TraversePrev},
  {"traverse-up", 		TraverseUp},
  {"traverse-down", 		TraverseDown},
  {"traverse-home", 		TraverseHome},
  {"traverse-next-top",		TraverseNextTop},  
/* highlighting */
  {"enter",			Enter},
  {"leave",			Leave},
  {"execute",                   Execute},
  {NULL,NULL}
};

/****************************************************************************
*
* TextEdit Default Event Translations
*
****************************************************************************/
char defaultTextEditTranslations[] =  
       "Ctrl<Key>F:	forward-character()\n\
	Ctrl<Key>Right:	traverse-right()\n\
	<Key>Right:	forward-character()\n\
	Ctrl<Key>B:	backward-character()\n\
	Ctrl<Key>Left:	traverse-left()\n\
	<Key>Left: 	backward-character()\n\
	Meta<Key>F:	forward-word()\n\
	Meta<Key>B:	backward-word()\n\
	Meta<Key>]:	forward-paragraph()\n\
	Ctrl<Key>[:	backward-paragraph()\n\
	Ctrl<Key>A:	beginning-of-line()\n\
	Ctrl<Key>E:	end-of-line()\n\
	Ctrl<Key>N:	next-line()\n\
	Ctrl<Key>Down:	traverse-down()\n\
	<Key>Down: 	next-line()\n\
	Ctrl<Key>P:	previous-line()\n\
	Ctrl<Key>Up:	traverse-up()\n\
	<Key>Up:	previous-line()\n\
	Ctrl<Key>V:	next-page()\n\
	Ctrl<Key>Next:	traverse-next()\n\
	<Key>Next:	next-page()\n\
	Meta<Key>V:	previous-page()\n\
	Ctrl<Key>Prior:	traverse-prev()\n\
	<Key>Prior:	previous-page()\n\
	Meta<Key>\\<:	beginning-of-file()\n\
	Meta<Key>\\>:	end-of-file()\n\
	Ctrl<Key>Home:	traverse-home()\n\
	Shift<Key>Home:	end-of-file()\n\
	<Key>Home:	beginning-of-file()\n\
	Ctrl<Key>Z:	scroll-one-line-up()\n\
	Meta<Key>Z:	scroll-one-line-down()\n\
	Ctrl<Key>D:	delete-next-character()\n\
	<Key>Delete:	delete-previous-character()\n\
	<Key>BackSpace:	delete-previous-character()\n\
	Ctrl<Key>H:	delete-previous-character()\n\
	Meta<Key>D:	delete-next-word()\n\
	Meta<Key>H:	delete-previous-word()\n\
	Shift Meta<Key>D:	kill-word()\n\
	Shift Meta<Key>H:	backward-kill-word()\n\
	Ctrl<Key>W:	kill-selection()\n\
	Ctrl<Key>K:	kill-to-end-of-line()\n\
	Meta<Key>K:	kill-to-end-of-paragraph()\n\
	Ctrl<Key>Y:	unkill()\n\
	Meta<Key>Y:	stuff()\n\
	Ctrl<Key>J:	newline-and-indent()\n\
	Ctrl<Key>O:	newline-and-backup()\n\
	Ctrl<Key>M:	newline()\n\
	<Key>Return:	newline()\n\
      <Key>Linefeed:  newline-and-indent()\n\
	Ctrl<Key>L:	redraw-display()\n\
	<FocusIn>:	focus-in()\n\
	<FocusOut>:	focus-out()\n\
	<Btn1Down>:	select-start()\n\
	Button1<PtrMoved>:	extend-adjust()\n\
	<Btn1Up>:	extend-end()\n\
	<Btn2Down>:	stuff()\n\
	<Btn3Down>:	extend-start()\n\
	Button3<PtrMoved>:	extend-adjust()\n\
	<Btn3Up>:	extend-end()\n\
	<Key>Execute:	execute()\n\
	<Key>:	insert-char()\n\
	Shift<Key>:	insert-char()\n\
	<EnterWindow>:	enter()\n\
	<LeaveWindow>:	leave()";


/* Utility routines for support of TextEdit */
#include "sub.c"

/******************************************************************************
*
* Functions to support selection
*
******************************************************************************/
/*--------------------------------------------------------------------------+*/
static void _XtTextSetNewSelection(ctx, left, right)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwTextPosition left, right;
{
    XwTextPosition pos;

    if (left < ctx->text.s.left) {
	pos = min(right, ctx->text.s.left);
	_XtTextNeedsUpdating(ctx, left, pos);
    }
    if (left > ctx->text.s.left) {
	pos = min(left, ctx->text.s.right);
	_XtTextNeedsUpdating(ctx, ctx->text.s.left, pos);
    }
    if (right < ctx->text.s.right) {
	pos = max(right, ctx->text.s.left);
	_XtTextNeedsUpdating(ctx, pos, ctx->text.s.right);
    }
    if (right > ctx->text.s.right) {
	pos = max(left, ctx->text.s.right);
	_XtTextNeedsUpdating(ctx, pos, right);
    }

    ctx->text.s.left = left;
    ctx->text.s.right = right;
}


/*
 * This routine implements multi-click selection in a hardwired manner.
 * It supports multi-click entity cycling (char, word, line, file) and mouse
 * motion adjustment of the selected entitie (i.e. select a word then, with
 * button still down, adjust wich word you really meant by moving the mouse).
 * [NOTE: This routine is to be replaced by a set of procedures that
 * will allows clients to implements a wide class of draw through and
 * multi-click selection user interfaces.]
*/
/*--------------------------------------------------------------------------+*/
static void DoSelection (ctx, position, time, motion)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwTextPosition position;
  Time time;
  Boolean motion;
{
    int     delta;
    XwTextPosition newLeft, newRight;
    XwSelectType newType;
    XwSelectType *sarray;

    delta = (time < ctx->text.lasttime) ?
	ctx->text.lasttime - time : time - ctx->text.lasttime;
    if (motion)
	newType = ctx->text.s.type;
    else {/* multi-click event */
	if ((delta < 500) && ((position >= ctx->text.s.left)
		    && (position <= ctx->text.s.right))) {
	  sarray = ctx->text.sarray;
	  for (sarray = ctx->text.sarray;
	       *sarray != XwselectNull && *sarray != ctx->text.s.type;
	       sarray++) ;
	  if (*sarray != XwselectNull) sarray++;
	  if (*sarray == XwselectNull) sarray = ctx->text.sarray;
	  newType = *sarray;
	} else {			/* single-click event */
	    newType = *(ctx->text.sarray);
	}
        ctx->text.lasttime = time;
    }
    switch (newType) {
	case XwselectPosition: 
            newLeft = newRight = position;
	    break;
	case XwselectChar: 
            newLeft = position;
            newRight = (*(ctx->text.source->scan))(
                    ctx->text.source, position, position, XwsdRight, 1, FALSE);
	    break;
	case XwselectWord: 
	    newLeft = (*(ctx->text.source->scan))
	      (ctx->text.source, position, XwstWhiteSpace, XwsdLeft, 1, FALSE);
	    newRight = (*(ctx->text.source->scan))
	      (ctx->text.source, position, XwstWhiteSpace, XwsdRight, 1, FALSE);
	    break;
	case XwselectLine: 
	case XwselectParagraph:  /* need "para" scan mode to implement pargraph */
 	    newLeft = (*(ctx->text.source->scan))(
		    ctx->text.source, position, XwstEOL, XwsdLeft, 1, FALSE);
	    newRight = (*(ctx->text.source->scan))(
		    ctx->text.source, position, XwstEOL, XwsdRight, 1, FALSE);
	    break;
	case XwselectAll: 
	    newLeft = (*(ctx->text.source->scan))(
		    ctx->text.source, position, XwstLast, XwsdLeft, 1, FALSE);
	    newRight = (*(ctx->text.source->scan))(
		    ctx->text.source, position, XwstLast, XwsdRight, 1, FALSE);
	    break;
	default:
	    break;
    }
    if ((newLeft != ctx->text.s.left) || (newRight != ctx->text.s.right)
	    || (newType != ctx->text.s.type)) {
	_XtTextSetNewSelection(ctx, newLeft, newRight);
	ctx->text.s.type = newType;
	if (position - ctx->text.s.left < ctx->text.s.right - position)
	    ctx->text.insertPos = newLeft;
	else 
	    ctx->text.insertPos = newRight;
    }
    if (!motion) { /* setup so we can freely mix select extend calls*/
	ctx->text.origSel.type = ctx->text.s.type;
	ctx->text.origSel.left = ctx->text.s.left;
	ctx->text.origSel.right = ctx->text.s.right;
	if (position >= ctx->text.s.left +
	    ((ctx->text.s.right - ctx->text.s.left) / 2))
	    ctx->text.extendDir = XwsdRight;
	else
	    ctx->text.extendDir = XwsdLeft;
    }
}

/*
 * This routine implements extension of the currently selected text in
 * the "current" mode (i.e. char word, line, etc.). It worries about
 * extending from either end of the selection and handles the case when you
 * cross through the "center" of the current selection (e.g. switch which
 * end you are extending!).
 * [NOTE: This routine will be replaced by a set of procedures that
 * will allows clients to implements a wide class of draw through and
 * multi-click selection user interfaces.]
*/
/*--------------------------------------------------------------------------+*/
static void ExtendSelection (ctx, position, motion)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwTextPosition position;
  Boolean motion;
{
    XwTextPosition newLeft, newRight;
	

    if (!motion) {		/* setup for extending selection */
	ctx->text.origSel.type = ctx->text.s.type;
	ctx->text.origSel.left = ctx->text.s.left;
	ctx->text.origSel.right = ctx->text.s.right;
	if (position >= ctx->text.s.left +
	    ((ctx->text.s.right - ctx->text.s.left) / 2))
	    ctx->text.extendDir = XwsdRight;
	else
	    ctx->text.extendDir = XwsdLeft;
    }
    else /* check for change in extend direction */
	if ((ctx->text.extendDir == XwsdRight &&
	     position < ctx->text.origSel.left) ||
	    (ctx->text.extendDir == XwsdLeft &&
	     position > ctx->text.origSel.right)) {
	  ctx->text.extendDir =
	    (ctx->text.extendDir == XwsdRight)? XwsdLeft : XwsdRight;
	  _XtTextSetNewSelection(ctx, ctx->text.origSel.left,
				 ctx->text.origSel.right);
	}
    newLeft = ctx->text.s.left;
    newRight = ctx->text.s.right;
    switch (ctx->text.s.type) {
	case XwselectPosition: 
	    if (ctx->text.extendDir == XwsdRight)
		newRight = position;
	    else
		newLeft = position;
	    break;
	case XwselectWord: 
	    if (ctx->text.extendDir == XwsdRight)
	      newRight = position = (*(ctx->text.source->scan))
	      (ctx->text.source, position, XwstWhiteSpace, XwsdRight, 1, FALSE);
	    else
	      newLeft = position = (*(ctx->text.source->scan))
	      (ctx->text.source, position, XwstWhiteSpace, XwsdLeft, 1, FALSE);
            break;
        case XwselectLine:
	case XwselectParagraph: /* need "para" scan mode to implement pargraph */
	    if (ctx->text.extendDir == XwsdRight)
		newRight = position = (*(ctx->text.source->scan))
		      (ctx->text.source, position, XwstEOL, XwsdRight, 1, TRUE);
	    else
		newLeft = position = (*(ctx->text.source->scan))
		       (ctx->text.source, position, XwstEOL, XwsdLeft, 1, FALSE);
	    break;
	case XwselectAll: 
	    position = ctx->text.insertPos;
	    break;
	default:
	    break;
    }
    _XtTextSetNewSelection(ctx, newLeft, newRight);
    ctx->text.insertPos = position;
}


/*
 * This routine is used to perform various selection functions. The goal is
 * to be able to specify all the more popular forms of draw-through and
 * multi-click selection user interfaces from the outside.
 */
/*--------------------------------------------------------------------------+*/
static void AlterSelection (ctx, mode, action)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget     ctx;
    XwSelectionMode      mode;   /* {XwsmTextSelect, XwsmTextExtend}	  */
    XwSelectionAction action; /* {XwactionStart, XwactionAdjust, XwactionEnd} */
{
    XwTextPosition position;
    unsigned char   *ptr;

    position = PositionForXY (ctx, (int) ctx->text.ev_x, (int) ctx->text.ev_y);
    if (action == XwactionStart) {
	switch (mode) {
	case XwsmTextSelect: 
	    DoSelection (ctx, position, ctx->text.time, FALSE);
	    break;
	case XwsmTextExtend: 
            ExtendSelection (ctx, position, FALSE);
            break;
        }
    }
    else {
        switch (mode) {
        case XwsmTextSelect: 
            DoSelection (ctx, position, ctx->text.time, TRUE);
            break;
        case XwsmTextExtend: 
            ExtendSelection (ctx, position, TRUE);
            break;
        }
    }
    if (action == XwactionEnd && ctx->text.s.left < ctx->text.s.right) {
        ptr = _XwTextCopySubString (ctx, ctx->text.s.left, ctx->text.s.right);
        XStoreBuffer (XtDisplay(ctx), ptr, min (XwStrlen (ptr), MAXCUT), 0);
        XtFree (ptr);
    }
}

/******************************************************************************
*
* TextEdit Class Methods
*
******************************************************************************/

/*****************************************************************************
* This method deletes the text from startPos to endPos in a source and
* then inserts, at startPos, the text that was passed. As a side effect it
* "invalidates" that portion of the displayed text (if any), so that things
* will be repainted properly.
******************************************************************************/

/*--------------------------------------------------------------------------+*/
static int _XwTextSubString(ctx, left, right, target, size, used)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwTextPosition left, right;
  unsigned char  *target;       /* Memory to copy into */
  int            size,          /* Size of memory */
                 *used;         /* Memory used by copy */
{
    unsigned char   *tempResult;
    int             length, n;
    XwTextBlock     text;
    XwTextPosition  end, nend;

    end = (*(ctx->text.source->read))(ctx->text.source, left, &text,
				      right - left);
    n = length = min(text.length,size);
    strncpy(target, text.ptr, n);
    while (n && (end < right) && (length < size)) {
        nend = (*(ctx->text.source->read))(ctx->text.source, end, &text,
					   right - end);
	n = text.length;
	if (length + n > size) n = size - length;
	tempResult = target + length;
        strncpy(tempResult, text.ptr, n);
	length += n;
        end = nend;
    }
    *used = length;
    return length; /* return the number of positions transfered to target */
}

/*--------------------------------------------------------------------------+*/
static unsigned char *_XwTextCopySubString(ctx, left, right)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwTextPosition left, right;
{
    unsigned char *result;
    int length, resultLength;
    int   dummy;
    
    resultLength = right - left + 10;	/* Bogus? %%% */
    result = (unsigned char *)XtMalloc((unsigned) resultLength);
    
    length = _XwTextSubString(ctx, left, right, result, resultLength, &dummy);

    result[length] = 0;
    return result;
}

/*--------------------------------------------------------------------------+*/
static unsigned char *_XwTextCopySelection(ctx)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
{
   return( _XwTextCopySubString(ctx, ctx->text.s.left, ctx->text.s.right));
}

/*--------------------------------------------------------------------------+*/
static void _XwTextSetSelection(w, left, right)
/*--------------------------------------------------------------------------+*/
  Widget w;
  Position left, right;
{
   XwTextEditWidget ctx = (XwTextEditWidget) w;

  _XtTextPrepareToUpdate(ctx);
  _XtTextSetNewSelection(ctx, (XwTextPosition)left, (XwTextPosition)right);
  _XtTextExecuteUpdate(ctx);
}

/*--------------------------------------------------------------------------+*/
static void _XwTextUnsetSelection(w)
/*--------------------------------------------------------------------------+*/
    Widget	    w;
{
    XwTextEditWidget ctx = (XwTextEditWidget) w;

    _XtTextPrepareToUpdate(ctx);
    _XtTextSetNewSelection(ctx, zeroPosition, zeroPosition);
    _XtTextExecuteUpdate(ctx);
}

/*--------------------------------------------------------------------------+*/
static XwEditResult _XwTextReplace(w, startPos, endPos, text, verify)
/*--------------------------------------------------------------------------+*/
    Widget	    w;
    XwTextPosition  startPos, endPos;
    XwTextBlock     *text;
    Boolean         verify;
{
    XwTextEditWidget ctx = (XwTextEditWidget) w;
    XwEditResult result;

    _XtTextPrepareToUpdate(ctx);
    result = ReplaceText(ctx, startPos, endPos, text, verify);
    _XtTextExecuteUpdate(ctx);
    return result;
}

/*--------------------------------------------------------------------------+*/
static void _XwTextRedraw (w)
/*--------------------------------------------------------------------------+*/
   Widget w;
{
   XwTextEditWidget ctx = (XwTextEditWidget) w;

   _XtTextPrepareToUpdate(ctx);
   ForceBuildLineTable(ctx);
   DisplayAllText(ctx);
   _XtTextExecuteUpdate(ctx);
   if (ctx->primitive.highlighted)
      _XwHighlightBorder(ctx);
}


/******************************************************************************
*
* Utilities
*
******************************************************************************/
/*--------------------------------------------------------------------------+*/
static XwEditResult _XwSetCursorPos(ctx, newposition)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget  ctx;
  XwTextPosition  newposition;
{
  XwTextVerifyCD  cbdata;

  cbdata.operation = motionVerify;
  cbdata.doit = TRUE;
  cbdata.currInsert = ctx->text.insertPos;
  cbdata.newInsert = newposition;

  XtCallCallbacks((Widget)ctx, XtNmotionVerification, &cbdata);

  if (!cbdata.doit) return XweditReject;

  ctx->text.insertPos = cbdata.newInsert;

  return XweditDone;
}

/*--------------------------------------------------------------------------+*/
static void StartAction(ctx, event)
/*--------------------------------------------------------------------------+*/
   XwTextEditWidget ctx;
   XEvent *event;
{
    _XtTextPrepareToUpdate(ctx);
    if (event) {
      ctx->text.time = event->xbutton.time;
      ctx->text.ev_x = event->xbutton.x;
      ctx->text.ev_y = event->xbutton.y;
    }
}

/*--------------------------------------------------------------------------+*/
static void EndAction(ctx)
/*--------------------------------------------------------------------------+*/
   XwTextEditWidget ctx;
{
    CheckResizeOrOverflow(ctx);
    _XtTextExecuteUpdate(ctx);
}

/*--------------------------------------------------------------------------+*/
static void DeleteOrKill(ctx, from, to, kill)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget	   ctx;
    XwTextPosition from, to;
    Boolean	   kill;
{
    XwTextBlock text;
    unsigned char *ptr;
    XwEditResult result;
    
    if (kill && from < to) 
      ptr = _XwTextCopySubString(ctx, from, to);

    text.length = 0;

    if (result = ReplaceText(ctx, from, to, &text, TRUE)) {
      if (result != XweditReject)
	XBell(XtDisplay(ctx), 50);
      if (kill && from < to) 
	XtFree(ptr);
      return;
    }

    if (kill && from < to) {
      XStoreBuffer(XtDisplay(ctx), ptr, XwStrlen(ptr), 1);
      XtFree(ptr);
    }
    _XwSetCursorPos(ctx, from);
    ctx->text.showposition = TRUE;
    
    from = ctx->text.insertPos;
    _XtTextSetNewSelection(ctx, from, from);
}


/*--------------------------------------------------------------------------+*/
static void StuffFromBuffer(ctx, buffer)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  int buffer;
{
    extern char *XFetchBuffer();
    XwTextBlock text;
    XwEditResult result;
    XwTextPosition nextcursorpos;

    text.ptr =
      (unsigned char*)XFetchBuffer(XtDisplay(ctx), &(text.length), buffer);
    if (result =
	ReplaceText(ctx, ctx->text.insertPos, ctx->text.insertPos,
		    &text, TRUE)) {
	if (result != XweditReject)
	  XBell(XtDisplay(ctx), 50);
	XtFree(text.ptr);
	return;
    }
    nextcursorpos = (*(ctx->text.source->scan))
      (ctx->text.source, ctx->text.insertPos, XwstPositions, XwsdRight,
       text.length, TRUE);
    _XwSetCursorPos(ctx, nextcursorpos);
    nextcursorpos = ctx->text.insertPos;
    _XtTextSetNewSelection(ctx, nextcursorpos, nextcursorpos);
    XtFree(text.ptr);
}

/*--------------------------------------------------------------------------+*/
static XwTextPosition NextPosition(ctx, position, kind, direction)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget ctx;
    XwTextPosition   position;
    XwScanType       kind;
    XwScanDirection  direction;
{
    XwTextPosition pos;

     pos = (*(ctx->text.source->scan))(
	    ctx->text.source, position, kind, direction, 1, FALSE);
     if (pos == ctx->text.insertPos) 
         pos = (*(ctx->text.source->scan))(
            ctx->text.source, position, kind, direction, 2, FALSE);
     return pos;
}

/*--------------------------------------------------------------------------+*/
static XwEditResult InsertNewLineAndBackupInternal(ctx)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
{
    XwTextBlock text;
    XwEditResult result;
    
    text.length = 1;
    text.ptr = (unsigned char*)"\n";
    text.firstPos = 0;
    if (result =
	ReplaceText(ctx, ctx->text.insertPos, ctx->text.insertPos,
		    &text, TRUE)) {
	if (result != XweditReject)
	  XBell( XtDisplay(ctx), 50);
	return(result);
    }
    _XtTextSetNewSelection(ctx, (XwTextPosition) 0, (XwTextPosition) 0);
    ctx->text.showposition = TRUE;
    return(result);
}
/* Insert a file of the given name into the text.  Returns 0 if file found, 
   -1 if not. */

/* Unadvertized function */
/*--------------------------------------------------------------------------+*/
int XwTextInsertFile(ctx, str)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  unsigned char *str;
{
    int fid;
    XwTextBlock text;
    unsigned char    buf[1000];
    XwTextPosition position;

    if (str == NULL || XwStrlen(str) == 0) return -1;
    fid = open(str, O_RDONLY);
    if (fid <= 0) return -1;
    _XtTextPrepareToUpdate(ctx);
    position = ctx->text.insertPos;
    while ((text.length = read(fid, buf, 512)) > 0) {
	text.ptr = buf;
	ReplaceText(ctx, position, position, &text, TRUE);
	position = (*(ctx->text.source->scan))(ctx->text.source, position, 
		XwstPositions, XwsdRight, text.length, TRUE);
    }
    close(fid);
    ctx->text.insertPos = position;
    _XtTextExecuteUpdate(ctx);
    return 0;
}

/******************************************************************************
*
*  Action Table Functions
*
******************************************************************************/
/*--------------------------------------------------------------------------+*/
static Boolean VerifyLeave(ctx,event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget  ctx;
  XEvent            *event;
{
  XwTextVerifyCD  cbdata;

  StartAction(ctx, event);
  cbdata.operation = leaveVerify;
  cbdata.doit = TRUE;
  cbdata.currInsert = ctx->text.insertPos;
  cbdata.newInsert = ctx->text.insertPos;
  cbdata.startPos = ctx->text.insertPos;
  cbdata.endPos = ctx->text.insertPos;
  cbdata.text = NULL;
  cbdata.xevent = event;
  XtCallCallbacks((Widget)ctx, XtNleaveVerification, &cbdata);
  ctx->text.insertPos = cbdata.newInsert;
  EndAction(ctx);
  return( cbdata.doit );
}
  
/*--------------------------------------------------------------------------+*/
static void TraverseUp(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent           *event;
{
  /* Allow the verification routine to control the traversal */
  if (VerifyLeave(ctx, event))
    _XwTraverseUp((Widget)ctx, event);
}

/*--------------------------------------------------------------------------+*/
static void TraverseDown(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent           *event;
{
  /* Allow the verification routine to control the traversal */
  if (VerifyLeave(ctx, event))
    _XwTraverseDown((Widget)ctx, event);
}

/*--------------------------------------------------------------------------+*/
static void TraversePrev(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent           *event;
{
  /* Allow the verification routine to control the traversal */
  if (VerifyLeave(ctx, event))
    _XwTraversePrev((Widget)ctx, event);
}

/*--------------------------------------------------------------------------+*/
static void TraverseNext(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent           *event;
{
  /* Allow the verification routine to control the traversal */
  if (VerifyLeave(ctx, event))
    _XwTraverseNext((Widget)ctx, event);
}

/*--------------------------------------------------------------------------+*/
static void TraverseLeft(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent           *event;
{
  /* Allow the verification routine to control the traversal */
  if (VerifyLeave(ctx, event))
    _XwTraverseLeft((Widget)ctx, event);
}

/*--------------------------------------------------------------------------+*/
static void TraverseRight(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent           *event;
{
  /* Allow the verification routine to control the traversal */
  if (VerifyLeave(ctx, event)) 
    _XwTraverseRight((Widget)ctx, event);
}

/*--------------------------------------------------------------------------+*/
static void TraverseHome(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent           *event;
{
  /* Allow the verification routine to control the traversal */
  if (VerifyLeave(ctx, event)) 
    _XwTraverseHome((Widget)ctx, event);
}

/*--------------------------------------------------------------------------+*/
static void TraverseNextTop(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent           *event;
{
  /* Allow the verification routine to control the traversal */
  if (VerifyLeave(ctx, event)) 
    _XwTraverseNextTop((Widget)ctx, event);
}

/*--------------------------------------------------------------------------+*/
static void Enter(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent           *event;
{
  _XwPrimitiveEnter((Widget)ctx, event);
}

/*--------------------------------------------------------------------------+*/
static void Leave(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent           *event;
{
  /* If traversal is on, then the leave verification callback is called
     in the focus out event handler */
  if (ctx->primitive.traversal_type != XwHIGHLIGHT_TRAVERSAL) 
    VerifyLeave(ctx, event);

  _XwPrimitiveLeave((Widget)ctx, event);
}

/*--------------------------------------------------------------------------+*/
static void TextFocusIn (ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
   XEvent *event;
{
  _XwPrimitiveFocusIn((XwPrimitiveWidget)ctx, event);
  ctx->text.hasfocus = TRUE;
}

/*--------------------------------------------------------------------------+*/
static void TextFocusOut(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{ 
  /* If traversal is on, then the leave verification callback is called in
     the traversal event handler */
  if (ctx->primitive.traversal_type != XwHIGHLIGHT_TRAVERSAL)
      VerifyLeave(ctx, event);

  _XwPrimitiveFocusOut((XwPrimitiveWidget)ctx, event);
  ctx->text.hasfocus = FALSE;
}

/* Kill and Stuff */
/*--------------------------------------------------------------------------+*/
static void UnKill(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    StuffFromBuffer(ctx, 1);
   EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void Stuff(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    StuffFromBuffer(ctx, 0);
   EndAction(ctx);
}

/* routines for moving around */

/*--------------------------------------------------------------------------+*/
static void MoveForwardChar(ctx, event)
/*--------------------------------------------------------------------------+*/
   XwTextEditWidget ctx;
   XEvent *event;
{
  StartAction(ctx, event);
  _XwSetCursorPos(ctx, (*(ctx->text.source->scan))
       (ctx->text.source, ctx->text.insertPos, XwstPositions, XwsdRight, 1,
	TRUE)
		  );
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MoveBackwardChar(ctx, event)
/*--------------------------------------------------------------------------+*/
   XwTextEditWidget ctx;
   XEvent *event;
{
  StartAction(ctx, event);
  _XwSetCursorPos(ctx, (*(ctx->text.source->scan))
       (ctx->text.source, ctx->text.insertPos, XwstPositions, XwsdLeft, 1, TRUE)
		  );
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MoveForwardWord(ctx, event)
/*--------------------------------------------------------------------------+*/
   XwTextEditWidget ctx;
   XEvent *event;
{
  StartAction(ctx, event);
  _XwSetCursorPos(ctx,
	  NextPosition(ctx, ctx->text.insertPos, XwstWhiteSpace, XwsdRight)
		  );
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MoveBackwardWord(ctx, event)
/*--------------------------------------------------------------------------+*/
   XwTextEditWidget ctx;
   XEvent *event;
{
  StartAction(ctx, event);
  _XwSetCursorPos(ctx,
         NextPosition(ctx, ctx->text.insertPos, XwstWhiteSpace, XwsdLeft)
		  );
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MoveBackwardParagraph(ctx, event)
/*--------------------------------------------------------------------------+*/
   XwTextEditWidget ctx;
   XEvent *event;
{
  StartAction(ctx, event);
  _XwSetCursorPos(ctx,
         NextPosition(ctx, ctx->text.insertPos, XwstEOL, XwsdLeft)
		  );
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MoveForwardParagraph(ctx, event)
/*--------------------------------------------------------------------------+*/
   XwTextEditWidget ctx;
   XEvent *event;
{
  StartAction(ctx, event);
  _XwSetCursorPos(ctx,
         NextPosition(ctx, ctx->text.insertPos, XwstEOL, XwsdRight)
		  );
   EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MoveToLineStart(ctx, event)
/*--------------------------------------------------------------------------+*/
     XwTextEditWidget ctx;
     XEvent *event;
{
  int line;
  StartAction(ctx, event);
  _XtTextShowPosition(ctx);
  line = LineForPosition(ctx, ctx->text.insertPos);
  _XwSetCursorPos(ctx, ctx->text.lt.info[line].position);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MoveToLineEnd(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  int line;
  XwTextPosition next, lastpos = GETLASTPOS(ctx);
  StartAction(ctx, event);
  _XtTextShowPosition(ctx);
  line = LineForPosition(ctx, ctx->text.insertPos);
  next = ctx->text.lt.info[line+1].position;
  if (next > lastpos)
    next = lastpos;
  else
    next = (*(ctx->text.source->scan))
      (ctx->text.source, next, XwstPositions, XwsdLeft, 1, TRUE);
  if (next <= ctx->text.lt.info[line].drawPos)
    next = ctx->text.lt.info[line].drawPos + 1 ;
  _XwSetCursorPos(ctx, next);
  EndAction(ctx);
}


/*--------------------------------------------------------------------------+*/
static void MoveNextLine(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  int     width, width2, height, line;
  XwTextPosition position, maxp, lastpos = GETLASTPOS(ctx);
  XwTextSink *sink = ctx->text.sink ;

  StartAction(ctx, event);
  _XtTextShowPosition(ctx); /* Needed if cursor goes off screen ??? */
  line = LineForPosition(ctx, ctx->text.insertPos);

  if (ctx->text.lt.info[line+1].position != (lastpos + 1)) {
    if ((line == ctx->text.lt.lines - 1) && (line > 0)) {
      _XtTextScroll(ctx, 1);
      line = LineForPosition(ctx, ctx->text.insertPos);
    }
    if (sink->LineLastPosition == ctx->text.insertPos)
      width = sink->LineLastWidth;
    else
      (*(ctx->text.sink->findDistance))
	(ctx, ctx->text.lt.info[line].position, ctx->text.lt.info[line].x,
	  ctx->text.insertPos, &width, &position, &height);
    line++;
    if (ctx->text.lt.info[line].position > lastpos) {
      position = lastpos;
    }
    else {
      (*(ctx->text.sink->findPosition))(ctx,
					ctx->text.lt.info[line].position, ctx->text.lt.info[line].x,
					width, FALSE, &position, &width2, &height);
      maxp = (*(ctx->text.source->scan))(ctx->text.source,
					 ctx->text.lt.info[line+1].position,
					 XwstPositions, XwsdLeft, 1, TRUE);
      if (position > maxp)
	position = maxp;
    }
    if (_XwSetCursorPos(ctx, position) == XweditDone) {
      sink->LineLastWidth = width;
      sink->LineLastPosition = position;
    }
  }
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MovePreviousLine(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  int     width, width2, height, line;
  XwTextPosition position, maxp;
  XwTextSink *sink = ctx->text.sink ;

  StartAction(ctx, event);
  _XtTextShowPosition(ctx);
  line = LineForPosition(ctx, ctx->text.insertPos);
  if (line == 0) {
    _XtTextScroll(ctx, -1);
    line = LineForPosition(ctx, ctx->text.insertPos);
  }
  if (line > 0) {
    if (sink->LineLastPosition == ctx->text.insertPos)
      width = sink->LineLastWidth;
    else
      (*(ctx->text.sink->findDistance))(ctx,
		    ctx->text.lt.info[line].position, 
		    ctx->text.lt.info[line].x,
		    ctx->text.insertPos, &width, &position, &height);
    line--;
    (*(ctx->text.sink->findPosition))(ctx,
		ctx->text.lt.info[line].position, ctx->text.lt.info[line].x,
		width, FALSE, &position, &width2, &height);
    maxp = (*(ctx->text.source->scan))(ctx->text.source, 
		ctx->text.lt.info[line+1].position,
		XwstPositions, XwsdLeft, 1, TRUE);
    if (position > maxp)
	    position = maxp;
    if (_XwSetCursorPos(ctx, position) == XweditDone) {
      sink->LineLastWidth = width;
      sink->LineLastPosition = position;
    }
  }
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MoveBeginningOfFile(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  _XwSetCursorPos(ctx, (*(ctx->text.source->scan))
	   (ctx->text.source, ctx->text.insertPos, XwstLast, XwsdLeft, 1, TRUE)
		  );
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MoveEndOfFile(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  _XwSetCursorPos(ctx, (*(ctx->text.source->scan))
	 (ctx->text.source, ctx->text.insertPos, XwstLast,  XwsdRight, 1, TRUE)
		  );
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void ScrollOneLineUp(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  _XtTextScroll(ctx, 1);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void ScrollOneLineDown(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  _XtTextScroll(ctx, -1);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MoveNextPage(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  XwLineTablePtr  lt = &(ctx->text.lt) ;
  int cur_line;
  XwTextPosition line_offset, new_pos, lastpos = GETLASTPOS(ctx);

  StartAction(ctx, event);
  if (lt->info[(lt->lines)].position != (lastpos + 1)) {
    cur_line = LineForPosition(ctx, ctx->text.insertPos);
    line_offset = (ctx->text.insertPos - lt->info[cur_line].position);
    _XtTextScroll(ctx, max(1, lt->lines - 2));
    if (lt->info[cur_line].position > lastpos) 
      cur_line = LineForPosition(ctx, lastpos);
    new_pos = (lt->info[cur_line].position + line_offset);
    new_pos = min(new_pos, lt->info[cur_line].drawPos);
    _XwSetCursorPos(ctx, new_pos);
  }
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void MovePreviousPage(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{ XwLineTablePtr  lt = &(ctx->text.lt) ;
  int cur_line = LineForPosition(ctx, ctx->text.insertPos);
  XwTextPosition line_offset =
                  (ctx->text.insertPos - lt->info[cur_line].position);
  XwTextPosition new_pos;
  StartAction(ctx, event);
  _XtTextScroll(ctx, -max(1, lt->lines - 2));
  new_pos = (lt->info[cur_line].position + line_offset);
  _XwSetCursorPos(ctx, min(new_pos, lt->info[cur_line].drawPos));
  EndAction(ctx);
}

/* delete routines */

/*--------------------------------------------------------------------------+*/
static void DeleteForwardChar(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  XwTextPosition next;

  StartAction(ctx, event);
  next = (*(ctx->text.source->scan))(
            ctx->text.source, ctx->text.insertPos, XwstPositions, 
	    XwsdRight, 1, TRUE);
  DeleteOrKill(ctx, ctx->text.insertPos, next, FALSE);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void DeleteBackwardNormal(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  /* If there's a selection, delete it, otherwise, delete the character */

  if (ctx->text.s.left != ctx->text.s.right)
     DeleteCurrentSelection(ctx, event);
  else
     DeleteBackwardChar(ctx, event);
}

/*--------------------------------------------------------------------------+*/
static void DeleteBackwardChar(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  XwTextPosition next;

  StartAction(ctx, event);
  next = (*(ctx->text.source->scan))(
         ctx->text.source, ctx->text.insertPos, XwstPositions, 
	 XwsdLeft, 1, TRUE);
  DeleteOrKill(ctx, next, ctx->text.insertPos, FALSE);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void DeleteForwardWord(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  XwTextPosition next;

  StartAction(ctx, event);
  next = NextPosition(ctx, ctx->text.insertPos, XwstWhiteSpace, XwsdRight);
  DeleteOrKill(ctx, ctx->text.insertPos, next, FALSE);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void DeleteBackwardWord(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  XwTextPosition next;

  StartAction(ctx, event);
  next = NextPosition(ctx, ctx->text.insertPos, XwstWhiteSpace, XwsdLeft);
  DeleteOrKill(ctx, next, ctx->text.insertPos, FALSE);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void KillForwardWord(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  XwTextPosition next;

  StartAction(ctx, event);
  next = NextPosition(ctx, ctx->text.insertPos, XwstWhiteSpace, XwsdRight);
  DeleteOrKill(ctx, ctx->text.insertPos, next, TRUE);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void KillBackwardWord(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  XwTextPosition next;

  StartAction(ctx, event);
  next = NextPosition(ctx, ctx->text.insertPos, XwstWhiteSpace, XwsdLeft);
  DeleteOrKill(ctx, next, ctx->text.insertPos, TRUE);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void KillCurrentSelection(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  DeleteOrKill(ctx, ctx->text.s.left, ctx->text.s.right, TRUE);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void DeleteCurrentSelection(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  DeleteOrKill(ctx, ctx->text.s.left, ctx->text.s.right, FALSE);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void KillToEndOfLine(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  int     line;
  XwTextPosition last, next, lastpos = GETLASTPOS(ctx);
  StartAction(ctx, event);
  _XtTextShowPosition(ctx);
  line = LineForPosition(ctx, ctx->text.insertPos);
  last = ctx->text.lt.info[line + 1].position;
  next = (*(ctx->text.source->scan))(ctx->text.source, ctx->text.insertPos,
       XwstEOL, XwsdRight, 1, FALSE);
  if (last > lastpos)
    last = lastpos;
  if (last > next && ctx->text.insertPos < next)
    last = next;
  DeleteOrKill(ctx, ctx->text.insertPos, last, TRUE);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void KillToEndOfParagraph(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  XwTextPosition next;

  StartAction(ctx, event);
  next = (*(ctx->text.source->scan))(ctx->text.source, ctx->text.insertPos,
				       XwstEOL, XwsdRight, 1, FALSE);
  if (next == ctx->text.insertPos)
    next = (*(ctx->text.source->scan))(ctx->text.source, next, XwstEOL,
					   XwsdRight, 1, TRUE);
  DeleteOrKill(ctx, ctx->text.insertPos, next, TRUE);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void InsertNewLineAndBackup(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  InsertNewLineAndBackupInternal(ctx);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static XwEditResult InsertNewLine(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  XwTextPosition next;
  XwEditResult result;

  StartAction(ctx, event);
  if (InsertNewLineAndBackupInternal(ctx)) {
    EndAction(ctx);
    return(XweditError);
  }
  next = (*(ctx->text.source->scan))(ctx->text.source, ctx->text.insertPos,
	    XwstPositions, XwsdRight, 1, TRUE);
  result = _XwSetCursorPos(ctx, next);
  EndAction(ctx);
  return(result);
}

/*--------------------------------------------------------------------------+*/
static void InsertNewLineAndIndent(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  XwTextBlock text;
  XwTextPosition pos1, pos2;
  XwEditResult result;

  StartAction(ctx, event);
  pos1 = (*(ctx->text.source->scan))(ctx->text.source, ctx->text.insertPos, 
				     XwstEOL, XwsdLeft, 1, FALSE);
  pos2 = (*(ctx->text.source->scan))(ctx->text.source, pos1, XwstEOL, 
				     XwsdLeft, 1, TRUE);
  pos2 = (*(ctx->text.source->scan))(ctx->text.source, pos2, XwstWhiteSpace, 
				     XwsdRight, 1, TRUE);
  text.ptr = _XwTextCopySubString(ctx, pos1, pos2);
  text.length = XwStrlen(text.ptr);
  if (InsertNewLine(ctx, event)) return;
  if (result =
	ReplaceText(ctx, ctx->text.insertPos, ctx->text.insertPos,
		    &text, TRUE)) {
    if (result != XweditReject)
      XBell(XtDisplay(ctx), 50);
    XtFree(text.ptr);
    EndAction(ctx);
    return;
  }
  _XwSetCursorPos(ctx, (*(ctx->text.source->scan))
                        (ctx->text.source, ctx->text.insertPos, XwstPositions,
			 XwsdRight, text.length, TRUE));
  XtFree(text.ptr);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void NewSelection(ctx, l, r)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XwTextPosition l, r;
{
  unsigned char   *ptr;
  _XtTextSetNewSelection(ctx, l, r);
  if (l < r) {
    ptr = _XwTextCopySubString(ctx, l, r);
    XStoreBuffer(XtDisplay(ctx), ptr, min(XwStrlen(ptr), MAXCUT), 0);
    XtFree(ptr);
  }
}

/*--------------------------------------------------------------------------+*/
static void SelectWord(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  XwTextPosition l, r;
  StartAction(ctx, event);
  l = (*(ctx->text.source->scan))(ctx->text.source, ctx->text.insertPos, 
				  XwstWhiteSpace, XwsdLeft, 1, FALSE);
  r = (*(ctx->text.source->scan))(ctx->text.source, l, XwstWhiteSpace, 
				  XwsdRight, 1, FALSE);
  NewSelection(ctx, l, r);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void SelectAll(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  NewSelection(ctx, (XwTextPosition) 0, GETLASTPOS(ctx));
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void SelectStart(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  AlterSelection(ctx, XwsmTextSelect, XwactionStart);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void SelectAdjust(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  AlterSelection(ctx, XwsmTextSelect, XwactionAdjust);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void SelectEnd(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  AlterSelection(ctx, XwsmTextSelect, XwactionEnd);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void ExtendStart(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  AlterSelection(ctx, XwsmTextExtend, XwactionStart);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void ExtendAdjust(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  AlterSelection(ctx, XwsmTextExtend, XwactionAdjust);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void ExtendEnd(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  AlterSelection(ctx, XwsmTextExtend, XwactionEnd);
  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void RedrawDisplay(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  StartAction(ctx, event);
  ForceBuildLineTable(ctx);
  DisplayAllText(ctx);
  ClearWindow(ctx);
  EndAction(ctx);
/* This seems like the wrong place for this */
  if (ctx->primitive.highlighted)
     _XwHighlightBorder(ctx);
}

#define STRBUFSIZE 100
static XComposeStatus compose_status = {NULL, 0};

/*--------------------------------------------------------------------------+*/
static void InsertChar(ctx, event)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget ctx;
  XEvent *event;
{
  unsigned char strbuf[STRBUFSIZE];
  KeySym keysym;
  XwEditResult result;
  XwTextBlock text;

  text.length = XLookupString ((XKeyEvent *)event, strbuf, STRBUFSIZE,
			       &keysym, &compose_status);
  if (text.length==0) return;
  StartAction(ctx, event);
  text.ptr = &strbuf[0];;
  text.firstPos = 0;
  if (result =
      ReplaceText(ctx, ctx->text.insertPos, ctx->text.insertPos,
		  &text, TRUE)) {
    if (result != XweditReject)
      XBell(XtDisplay(ctx), 50);
    EndAction(ctx);
    return;
  }
  _XwSetCursorPos(ctx,
	(*(ctx->text.source->scan))(ctx->text.source, ctx->text.insertPos,
			           XwstPositions, XwsdRight, text.length, TRUE));
  _XtTextSetNewSelection(ctx, ctx->text.insertPos, ctx->text.insertPos);

  EndAction(ctx);
}

/*--------------------------------------------------------------------------+*/
static void Execute(ctx, event)
/*--------------------------------------------------------------------------+*/
     XwTextEditWidget ctx;
     XEvent           *event;
{

  XtCallCallbacks((Widget)ctx, XtNexecute, NULL);

}

/*************************************************************************
*
* Class Record Support Functions
*
*************************************************************************/
     
/******************************************************************************
* 
*  Class Record Functions  
*
******************************************************************************/

/*--------------------------------------------------------------------------+*/
static void ClassInitialize()
/*--------------------------------------------------------------------------+*/
{


} /* ClassInitialize */



/******************************************************************************
*
*  External functions (Methods???) on the class
*
******************************************************************************/

/*--------------------------------------------------------------------------+*/
void XwTextClearBuffer(w)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget w;
{
  _XtTextPrepareToUpdate(w);
  (*(w->text.source->setLastPos))(w->text.source, (XwTextPosition)0);
  w->text.insertPos = 0;
  ForceBuildLineTable(w);
  DisplayAllText(w);
  _XtTextExecuteUpdate(w);
}

/*--------------------------------------------------------------------------+*/
unsigned char *XwTextCopyBuffer(w)
/*--------------------------------------------------------------------------+*/
   XwTextEditWidget w;
{
  
  return (*((XwTextEditClassRec *)(w->core.widget_class))->textedit_class.copy_substring)
           ((XwTextEditWidget)w, 0, GETLASTPOS(w));

}
  
/*--------------------------------------------------------------------------+*/
unsigned char *XwTextCopySelection(w)
/*--------------------------------------------------------------------------+*/
   XwTextEditWidget w;
{
  return (*((XwTextEditClassRec *)(w->core.widget_class))->textedit_class.copy_selection)
    ((XwTextEditWidget)w);
}

/*--------------------------------------------------------------------------+*/
int XwTextReadSubString( w, startpos, endpos, target, targetsize, targetused )
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget  w;
  XwTextPosition    startpos, endpos;
  unsigned char     *target;
  int               targetsize,
                    *targetused;
{
  return( _XwTextSubString(w, startpos, endpos, target, targetsize, targetused) );

}    

/*--------------------------------------------------------------------------+*/
void XwTextUnsetSelection(w)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget w;
{
 (*((XwTextEditClassRec *)(w->core.widget_class))->textedit_class.unset_selection)
             ((Widget)w);
}

/*--------------------------------------------------------------------------+*/
void XwTextSetSelection(w, left, right)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget  w;
  XwTextPosition    left, right;
{
  (*((XwTextEditClassRec *)(w->core.widget_class))->textedit_class.set_selection)
    ((Widget)w, left, right);
}

/*--------------------------------------------------------------------------+*/
XwEditResult XwTextReplace(w, startPos, endPos, string)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget w;
    XwTextPosition   startPos, endPos;
    unsigned char    *string;
{
  XwTextBlock blk;

  blk.ptr = string;
  blk.length = XwStrlen(string);
  blk.firstPos = (XwTextPosition)0;

  return (*((XwTextEditClassRec *)(w->core.widget_class))->textedit_class.replace_text)
    (w, startPos, endPos, &blk, FALSE);
}

/*--------------------------------------------------------------------------+*/
void XwTextRedraw(w)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget w;
{
  (*((XwTextEditClassRec *)(w->core.widget_class))->textedit_class.redraw_text)
     ((Widget)w);
}

/*--------------------------------------------------------------------------+*/
void XwTextResize(w)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget w;
{
    _XtTextPrepareToUpdate(w);
    CheckResizeOrOverflow(w);
    _XtTextExecuteUpdate(w);
}

/*--------------------------------------------------------------------------+*/
void XwTextUpdate( w, status )
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget w;
    Boolean status;
{
   w->text.update_flag = status;
   if (status) {
     _XtTextExecuteUpdate(w);
   }
   else {
     w->text.numranges = 0;
     w->text.showposition = FALSE;
     w->text.oldinsert = w->text.insertPos;
     if ( XtIsRealized((Widget)w) ) InsertCursor(w, XwisOff);
   }
}


/*--------------------------------------------------------------------------+*/
void XwTextInsert(w, string)
/*--------------------------------------------------------------------------+*/
     XwTextEditWidget w;
     unsigned char *string;
{
     XwTextBlock blk;

     blk.ptr = string;
     blk.length = XwStrlen(string);
     blk.firstPos = (XwTextPosition) 0;

    _XtTextPrepareToUpdate(w);
    if (ReplaceText(w, w->text.insertPos,
		    w->text.insertPos, &blk, FALSE) == XweditDone) {
      w->text.showposition = TRUE;
      w->text.insertPos = w->text.insertPos + blk.length;
    }
    _XtTextExecuteUpdate(w);

}

/*--------------------------------------------------------------------------+*/
XwTextPosition XwTextGetLastPos (w)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget w;
{
    return( GETLASTPOS(w) );
}

/*--------------------------------------------------------------------------+*/
void XwTextGetSelectionPos(w, left, right)
/*--------------------------------------------------------------------------+*/
     XwTextEditWidget w;
     XwTextPosition *left, *right;
{
  
  *left = w->text.s.left;
  *right = w->text.s.right;
}

/*--------------------------------------------------------------------------+*/
void XwTextSetInsertPos(w, position)
/*--------------------------------------------------------------------------+*/     
     XwTextEditWidget w;
     XwTextPosition position;
{

  _XtTextPrepareToUpdate(w);
  w->text.insertPos = position;
  w->text.showposition = TRUE;
  _XtTextExecuteUpdate(w);
}

/*--------------------------------------------------------------------------+*/
XwTextPosition XwTextGetInsertPos(w)
/*--------------------------------------------------------------------------+*/
     XwTextEditWidget w;
{

  return( w->text.insertPos );

}

/*--------------------------------------------------------------------------+*/
void XwTextSetSource(w, source, startpos)
/*--------------------------------------------------------------------------+*/
     XwTextEditWidget w;
     XwTextSourcePtr source;
     XwTextPosition startpos;
{

  w->text.source = source;
  w->text.lt.top = startpos;
  w->text.s.left = w->text.s.right = 0;
  w->text.insertPos = startpos;

  ForceBuildLineTable(w);
  if (XtIsRealized((Widget)w)) {
    _XtTextPrepareToUpdate(w);
    DisplayAllText(w);
    _XtTextExecuteUpdate(w);
  }
}
/* Not advertised. Don't think it is necessary */
/*--------------------------------------------------------------------------+*/
unsigned char *XwTextCopySubString(w, left, right)
/*--------------------------------------------------------------------------+*/
  XwTextEditWidget w;
  XwTextPosition left, right;
{
  return (*((XwTextEditClassRec *)(w->core.widget_class))->textedit_class.copy_substring)
    ((XwTextEditWidget)w, left, right);
}

/*--------------------------------------------------------------------------+*/
void XwTextInvalidate(w, from, to)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget     w;
    XwTextPosition       from,to;
{
        _XtTextPrepareToUpdate(w);
        _XtTextNeedsUpdating(w, from, to);
        ForceBuildLineTable(w);
        _XtTextExecuteUpdate(w);
}

/*--------------------------------------------------------------------------+*/
XwTextPosition XwTextTopPosition(w)
/*--------------------------------------------------------------------------+*/
    XwTextEditWidget  w;
{
     return w->text.lt.top;
}

/*****************************************************************************
*  Class Functions
*****************************************************************************/

/*--------------------------------------------------------------------------+*/
static void Initialize(request, new)
/*--------------------------------------------------------------------------+*/
 Widget request, new;
{
    XwTextEditWidget ctx = (XwTextEditWidget)new;
    XwTextEditPart *text = &(ctx->text);

    text->lt.lines = 0;
    text->lt.info = NULL;
    text->s.left = text->s.right = 0;
    text->s.type = XwselectPosition;
    text->sarray[0] = XwselectPosition;
    text->sarray[1] = XwselectWord;
    text->sarray[2] = XwselectLine;
    text->sarray[3] = XwselectParagraph;
    text->sarray[4] = XwselectAll;
    text->sarray[5] = XwselectNull;
    text->lasttime = 0; /* ||| correct? */
    text->time = 0; /* ||| correct? */

    text->oldinsert = -1 ;
    text->update_flag = FALSE;
    text->showposition = TRUE;
    text->updateFrom = (XwTextPosition *) XtMalloc(sizeof(XwTextPosition));
    text->updateTo = (XwTextPosition *) XtMalloc(sizeof(XwTextPosition));
    text->numranges = text->maxranges = 0;
    text->gc = DefaultGCOfScreen(XtScreen(ctx));
    text->hasfocus = FALSE;
    text->scroll_state = text->scroll_mode ;
    text->grow_state = text->grow_mode ;
    text->prevW = ctx->core.width ;
    text->prevH = ctx->core.height ;
    if (text->grow_mode & XwGrowHorizontal)
      {	text->wrap_mode = XwWrapOff ;
      } ;
    if (text->wrap_mode == XwWrapOff)
      { text->wrap_form = XwSourceForm ;
	text->wrap_break = XwWrapAny ;
      } ;

#ifdef GLSDEBUG
    fprintf (stderr, "%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %x\n"
	     , "  wrap mode", text->wrap_mode
	     , "  wrap form", text->wrap_form
	     , " wrap break", text->wrap_break
	     , "scroll mode", text->scroll_mode
	     , "  grow mode", text->grow_mode
	     , "    options", text->options
	     ) ;
#endif
}

/*--------------------------------------------------------------------------+*/
static void InitializeHook(widget, args, num_args)
/*--------------------------------------------------------------------------+*/
   Widget widget;
   ArgList args;
   Cardinal *num_args;
{ XwTextEditWidget ctx = (XwTextEditWidget)widget;
  register XwTextEditPart *text = &(ctx->text);

  text->sink = XwAsciiSinkCreate(widget, args, *num_args);

  if (text->srctype == XwstringSrc)
    text->source = XwStringSourceCreate(widget, args, *num_args);
  else if (text->srctype != XwprogDefinedSrc)
    {XtWarning("XwSourceType not recognized. Using XwstringSrc.");
     text->source = XwStringSourceCreate(widget, args, *num_args);
   };
  
  ForceBuildLineTable((XwTextEditWidget)widget);
  if (text->lt.lines > 1)
    { text->scroll_state &= ~XwAutoScrollHorizontal ;
    }
}

/*--------------------------------------------------------------------------+*/
static void TextDestroy(ctx)
/*--------------------------------------------------------------------------+*/
     XwTextEditWidget ctx;
{
  (*(ctx->text.source->destroy))(ctx->text.source);
  (*(ctx->text.sink->destroy))(ctx->text.sink);
  XtFree((char *)ctx->text.updateFrom);
  XtFree((char *)ctx->text.updateTo);
  XtRemoveAllCallbacks((Widget)ctx, XtNmotionVerification);
  XtRemoveAllCallbacks((Widget)ctx, XtNmodifyVerification);
  XtRemoveAllCallbacks((Widget)ctx, XtNleaveVerification);
}


/*--------------------------------------------------------------------------+*/
static void Resize(w)
/*--------------------------------------------------------------------------+*/
    Widget          w;
{
    XwTextEditWidget ctx = (XwTextEditWidget) w;
    XwTextEditPart   *tp ; 
    Dimension width, height ;
    Boolean          realized = XtIsRealized(w);

    tp = &(ctx->text) ;
    width = ctx->core.width ;
    if ((width < tp->prevW)  && (tp->grow_mode & XwGrowHorizontal))
      tp->grow_state |= XwGrowHorizontal ;
    height = ctx->core.height ;
    if ((height < tp->prevH)  && (tp->grow_mode & XwGrowVertical))
      tp->grow_state |= XwGrowVertical ;
    
    if (realized) _XtTextPrepareToUpdate(ctx);
    ForceBuildLineTable(ctx);

    if (tp->lt.lines > 1)
      { tp->scroll_state &= ~XwAutoScrollHorizontal ;
      }
    else
      { tp->scroll_state |=  tp->scroll_mode & XwAutoScrollHorizontal ;
      }
 
    if (realized)
    {
	DisplayAllText(ctx);
        ClearWindow(ctx);
    }

    if (realized) _XtTextExecuteUpdate(ctx);
}

/*--------------------------------------------------------------------------+*/
static Boolean SetValues(current, request, new)
/*--------------------------------------------------------------------------+*/
Widget current, request, new;
{
    XwTextEditWidget oldtw = (XwTextEditWidget) current;
    XwTextEditWidget newtw = (XwTextEditWidget) new;
    Boolean    redisplay = FALSE;
    XwTextEditPart *oldtext = &(oldtw->text);
    XwTextEditPart *newtext = &(newtw->text);
    Boolean realized = XtIsRealized(current);
    
    if (realized) _XtTextPrepareToUpdate(oldtw);
    
    if (oldtext->source != newtext->source) {
	ForceBuildLineTable(oldtw);
	if ((oldtext->s.left == newtext->s.left) &&
	    (oldtext->s.right == newtext->s.right)) {
	  newtext->s.left = (XwTextPosition) 0;
	  newtext->s.right = (XwTextPosition) 0;
	}
	redisplay = TRUE;
    }

    if (oldtext->sink != newtext->sink)
	redisplay = TRUE;

    if (oldtext->insertPos != newtext->insertPos)
	oldtext->showposition = TRUE;

    if (oldtext->lt.top != newtext->lt.top)
	redisplay = TRUE;

    if ((oldtext->leftmargin != newtext->leftmargin) ||
	(oldtext->topmargin != newtext->topmargin) ||
	(oldtext->rightmargin != newtext->rightmargin) ||
	(oldtext->bottommargin != newtext->bottommargin))
	redisplay = TRUE;

    if ((oldtext->s.left != newtext->s.left) ||
	(oldtext->s.right != newtext->s.right)) {
      if (newtext->s.left > newtext->s.right) {
	XwTextPosition temp = newtext->s.right;
	newtext->s.right = newtext->s.left;
	newtext->s.left = temp;
	redisplay = TRUE;
      }
    }
	

    /* ||| This may be the best way to do this, as some optimizations
     *     can occur here that may be harder if we let XtSetValues
     *     call our expose proc.
     */

    if (redisplay && realized) 
	DisplayAllText(newtw);

    if (realized) _XtTextExecuteUpdate(newtw);

    return ( FALSE );
}

/*--------------------------------------------------------------------------+*/
static Boolean SetValuesHook(widget, args, num_args)
/*--------------------------------------------------------------------------+*/
  Widget widget;
  ArgList args;
  Cardinal *num_args;
{
  XwTextEditWidget ctx = (XwTextEditWidget)widget;
  XwTextSource *source = ctx->text.source;
  XwTextSink   *sink   = ctx->text.sink;
  Boolean realized = XtIsRealized(widget);

  if (realized) _XtTextPrepareToUpdate(ctx);

  /* This is ugly, but need to know if user set initial_string */
  ((StringSourcePtr)(source->data))->initial_string = NULL;

  XtSetSubvalues(source->data, source->resources,
		 source->resource_num, args, *num_args);
  XtSetSubvalues(sink->data, sink->resources,
		 sink->resource_num, args, *num_args);

  (*(source->check_data))(source);

  ForceBuildLineTable(ctx);
  if (realized) {
    DisplayAllText(ctx);
    _XtTextExecuteUpdate(ctx);
  }
  
  return( FALSE );
}

/*--------------------------------------------------------------------------+*/
static void GetValuesHook(widget, args, num_args)
/*--------------------------------------------------------------------------+*/
  Widget widget;
  ArgList args;
  Cardinal *num_args;
{
  XwTextSource *source = ((XwTextEditWidget)widget)->text.source;
  XwTextSink *sink = ((XwTextEditWidget)widget)->text.sink;
  int i = 0;
  
  /* This is ugly, but have to get internal buffer to initial_string storage */
  while (i < *num_args) {
    if (strcmp(args[i].name, XtNstring) == 0) {
      ((StringSourcePtr)(source->data))->initial_string =
	XwTextCopyBuffer((XwTextEditWidget)widget);
      break;
    };
    i++;
  };

  XtGetSubvalues(source->data, source->resources, source->resource_num,
			 args, *num_args);
  XtGetSubvalues(sink->data, sink->resources, sink->resource_num,
			 args, *num_args);
}

/*--------------------------------------------------------------------------+*/
static void Realize( w, valueMask, attributes )
/*--------------------------------------------------------------------------+*/
   Widget w;
   Mask *valueMask;
   XSetWindowAttributes *attributes;
{
   XwTextEditWidget ctx = (XwTextEditWidget)w;

   *valueMask |= CWBitGravity;
   attributes->bit_gravity = NorthWestGravity;

   XtCreateWindow( w, InputOutput, (Visual *)CopyFromParent,
		   *valueMask, attributes);
   if (XtIsRealized(w)) {
     XDefineCursor( w->core.screen->display, w->core.window,
		   XCreateFontCursor( w->core.screen->display, XC_left_ptr));
     ctx->text.update_flag = TRUE;
   }
   else {
     XtWarning("Unable to realize TextEdit");
   }
   _XwRegisterName(w);
}


/*****************************************************************************
*
*  Text Edit Class Record
*
*****************************************************************************/
XwTextEditClassRec XwtexteditClassRec = {
  {
    /* core fields */
    /* superclass       */      (WidgetClass) &XwprimitiveClassRec,
    /* class_name       */      "XwTextEdit",
    /* widget_size      */      sizeof(XwTextEditRec),
    /* class_initialize */      ClassInitialize,
    /* class_part_init  */      NULL,
    /* class_inited     */      FALSE,
    /* initialize       */      (XtInitProc)Initialize,
    /* initialize_hook  */      InitializeHook,
    /* realize          */      Realize,
    /* actions          */      texteditActionsTable,
    /* num_actions      */      XtNumber(texteditActionsTable),
    /* resources        */      resources,
    /* num_ resource    */      XtNumber(resources),
    /* xrm_class        */      NULLQUARK,
    /* compress_motion  */      TRUE,
    /* compress_exposure*/      FALSE,
    /* compress_enterleave*/    TRUE,
    /* visible_interest */      FALSE,
    /* destroy          */      (XtWidgetProc)TextDestroy,
    /* resize           */      Resize,
    /* expose           */      (XtExposeProc)ProcessExposeRegion,
    /* set_values       */      (XtSetValuesFunc)SetValues,
    /* set_values_hook  */      SetValuesHook,
    /* set_values_almost*/      XtInheritSetValuesAlmost,
    /* get_values_hook  */      GetValuesHook,
    /* accept_focus     */      NULL,
    /* version          */      XtVersion,
    /* callback_private */      NULL,
    /* tm_table         */      defaultTextEditTranslations,
    /* query_geometry   */      NULL,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },
  {
    /* XwPrimitive fields */

    (XtWidgetProc)   NULL,
    (XtWidgetProc)   NULL,
    (XwEventProc)    NULL,
    (XwEventProc)    NULL,
    (XwEventProc)    NULL,
    (XtTranslations) NULL
  },
  {
    /* XwTextEdit fields */
    /* copy_substring    */     _XwTextCopySubString,
    /* copy_selection    */	_XwTextCopySelection,
    /* unset_selection   */     _XwTextUnsetSelection,
    /* set_selection     */     (XwSetSProc)_XwTextSetSelection,
    /* replace_text      */	_XwTextReplace,
    /* redraw_text       */	_XwTextRedraw,
  }
};

WidgetClass XwtexteditWidgetClass = (WidgetClass)&XwtexteditClassRec;
WidgetClass XwtextEditWidgetClass = (WidgetClass)&XwtexteditClassRec;

