/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        TextEdit.h
 **
 **   Project:     X Widgets
 **
 **   Description: TextEdit widget public include file
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

#ifndef _XwTextEdit_h
#define _XwTextEdit_h

#define XwSetArg(arg, n, v) \
     { Arg *_XwSetArgTmp = &(arg) ;\
       _XwSetArgTmp->name = (n) ;\
       _XwSetArgTmp->value = (XtArgVal) (v) ;}

#include <X11/StringDefs.h>



/*************************************************************************
*
*  Structures used in TextEdit function calls
*
*************************************************************************/

extern WidgetClass XwtexteditWidgetClass;
/* synonym added for consistent naming conventions */
extern WidgetClass XwtextEditWidgetClass;

typedef struct _XwTextEditClassRec *XwTextEditWidgetClass;
typedef struct _XwTextEditRec      *XwTextEditWidget;

typedef long XwTextPosition;

typedef enum {XwsdLeft, XwsdRight} XwScanDirection;
typedef enum 
  {XwstPositions, XwstWhiteSpace, XwstEOL, XwstLast} XwScanType;

typedef struct {
  int           firstPos;
  int           length;
  unsigned char *ptr;
  } XwTextBlock, *XwTextBlockPtr;

typedef enum {XwtextRead, XwtextAppend, XwtextEdit} XwEditType;
typedef enum
     {XweditDone, XweditError, XweditPosError, XweditReject} XwEditResult;

typedef struct {
    XtResource      *resources;
    Cardinal        resource_num;
    int		    (*read)();
    XwEditResult    (*replace)();
    XwTextPosition  (*getLastPos)();
    int		    (*setLastPos)();
    XwTextPosition  (*scan)();
    XwEditType      (*editType)();
    Boolean         (*check_data)();
    void            (*destroy)();
    int		    *data;       
    } XwTextSource, *XwTextSourcePtr;

/* this wouldn't be here if source and display (still called
   sink here) were properly separated, classed and subclassed
   */

typedef short TextFit ;
#define tfNoFit			0x01
#define tfIncludeTab		0x02
#define tfEndText		0x04
#define tfNewline		0x08
#define tfWrapWhiteSpace	0x10
#define tfWrapAny		0x20

typedef struct {
    XwTextEditWidget parent;
    XFontStruct *font;
    int foreground;
    XtResource *resources;
    Cardinal resource_num;
    int (*display)();
    int (*insertCursor)();
    int (*clearToBackground)();
    int (*findPosition)();
    TextFit (*textFitFn)();
    int (*findDistance)();
    int (*resolve)();
    int (*maxLines)();
    int (*maxHeight)();
    Boolean (*check_data)();
    void (*destroy)();
    int LineLastWidth ;
    XwTextPosition LineLastPosition ;
    int *data;
    } XwTextSink, *XwTextSinkPtr;

/*************************************************************************
*
* Support for Verification Callbacks
*
*************************************************************************/

typedef enum {motionVerify, modVerify, leaveVerify} XwVerifyOpType;

typedef struct {
  XEvent		*xevent;
  XwVerifyOpType	operation;
  Boolean		doit;
  XwTextPosition	currInsert, newInsert;
  XwTextPosition	startPos, endPos;
  XwTextBlock		*text;
} XwTextVerifyCD, *XwTextVerifyPtr;


/* Class record constants */

typedef enum {XwstringSrc, XwdiskSrc, XwprogDefinedSrc} XwSourceType;

/* other stuff */

#define wordBreak		0x01
#define scrollVertical		0x02
#define scrollHorizontal	0x04
#define scrollOnOverflow	0x08
#define resizeWidth		0x10
#define resizeHeight		0x20
#define editable		0x40

/****************************************************************************
*
*  Display control (grow, wrap, scroll, visible cursor)
*
****************************************************************************/

typedef enum {XwWrapOff, XwSoftWrap, XwHardWrap}  XwWrap ;
typedef enum {XwSourceForm, XwDisplayForm}        XwWrapForm ;
typedef enum {XwWrapAny, XwWrapWhiteSpace}        XwWrapBreak ;

/* Scroll options */
typedef int XwScroll ;

/* Grow options */
typedef int XwGrow ;

/*************************************************************************
*
*  External functions from TextEdit
*
*************************************************************************/

extern void XwTextClearBuffer();
   /* XwTextEditWidget w; */

extern unsigned char *XwTextCopyBuffer();
   /* XwTextEditWidget w */

extern unsigned char *XwTextCopySelection();
   /*   XwTextEditWidget w */

extern int XwTextReadSubString();
   /*   XwTextEditWidget w;
        XwTextPosition startpos,
	               endpos;
	unsigned char  *target;       Memory to copy into
	int            targetsize,    Memory size
	               targetused;    Memory used by copy      */

extern void XwTextUnsetSelection();
   /*   XwTextEditWidget w */

extern void XwTextSetSelection();
   /*   XwTextEditWidget           w;
        XwTextPosition left, right; */

extern XwEditResult XwTextReplace();
   /*    XwTextEditWidget    w;
         XwTextPosition      startPos, endPos;
         unsigned char       *string;  */

extern void XwTextRedraw();
   /*    XwTextEditWidget w  */

#define HAVE_XWTEXTRESIZE
extern void XwTextResize();
   /*    XwTextEditWidget w  */

extern void XwTextUpdate();
   /*  XwTextEditWidget w;
       Boolean status */
  
extern void XwTextInsert();
   /*   XwTextEditWidget  w       */
   /* unsigned char       *string */

extern XwTextPosition XwTextGetLastPos();
  /* XwTextEditWidget w; */

extern void XwTextGetSelectionPos();
   /* XwTextEditWidget w; */
   /* XwTextPosition *left, *right; */

extern void XwTextSetInsertPos();
   /* XwTextEditWidget w; */
   /* XwTextPosition position */

extern XwTextPosition XwTextGetInsertPos();
   /* XwTextEditWidget  widget */

extern void XwTextSetSource();
   /* XwTextEditWidget w; */
   /* XwTextSourcePtr source; */
   /* XwTextPosition startpos */

/*************************************************************************
*  
*  Extern Source and Sink Create/Destroy functions
*
*************************************************************************/

extern XwTextSink *XwAsciiSinkCreate();
    /* Widget    w       */
    /* ArgList   args    */
    /* Cardinal num_args */

extern XwTextSource *XwStringSourceCreate(); 
    /* Widget   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern void XwStringSourceDestroy();
    /* XwTextSource *src */

#endif
/* DON'T ADD STUFF AFTER THIS #endif */
