/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        DisplayP.h
 **
 **   Project:     X Widgets
 **
 **   Description: Private include file for TextEdit widget ascii sink
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


#ifndef _XwTextEditDisplayPrivate_h
#define _XwTextEditDisplayPrivate_h

/*****************************************************************************
*
* Constants
*
*****************************************************************************/

#define INFINITE_WIDTH 32767

#define applyDisplay(method) (*(self->text.sink->method))

/*****************************************************************************
*
* Displayable text management data structures (LineTable)
*
*****************************************************************************/

#define RequiredCursorMargin 3

typedef struct {
    XwTextPosition	position, drawPos;
    Position		x, y, endX;
    TextFit		fit ;
    } XwLineTableEntry, *XwLineTableEntryPtr;

/* Line Tables are n+1 long - last position displayed is in last lt entry */
typedef struct {
    XwTextPosition  top;	/* Top of the displayed text.		*/
    XwTextPosition  lines;	/* How many lines in this table.	*/
    XwLineTableEntry  *info;	/* A dynamic array, one entry per line  */
    } XwLineTable, *XwLineTablePtr;

typedef enum {XwisOn, XwisOff} XwInsertState;

typedef enum {XwselectNull, XwselectPosition, XwselectChar, XwselectWord,
    XwselectLine, XwselectParagraph, XwselectAll} XwSelectType;

typedef enum {XwsmTextSelect, XwsmTextExtend} XwSelectionMode;

typedef enum {XwactionStart, XwactionAdjust, XwactionEnd} XwSelectionAction;

typedef struct {
    XwTextPosition left, right;
    XwSelectType  type;
} XwTextSelection;

#define IsPositionVisible(ctx, pos)\
  (pos >= ctx->text.lt.info[0].position && \
   pos <= ctx->text.lt.info[ctx->text.lt.lines].position)

  
#endif
/* DON'T ADD STUFF AFTER THIS #endif */


