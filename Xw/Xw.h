/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        Xw.h
 **
 **   Project:     X Widgets
 **
 **   Description: This include file contains public defines and structures
 **                needed by all XToolkit applications using the X Widget Set.
 **                Included in the file are the definitions for all common
 **                resource types, defines for resource or arg list values,
 **                and the class and instance record definitions for all meta
 **                classes.
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **   
 **   Permission to use, copy, modify, and distribute this software 
 **   and its documentation for any purpose and without fee is hereby 
 **   granted, provided that the above copyright notice appear in all 
 **   copies and that both that copyright notice and this permission 
 **   notice appear in supporting documentation, and that the names of 
 **   Hewlett-Packard or  M.I.T.  not be used in advertising or publicity 
 **   pertaining to distribution of the software without specific, written 
 **   prior permission.
 **   
 *****************************************************************************
 *************************************<+>*************************************/


#ifndef _XtXw_h
#define _XtXw_h

/************************************************************************
 *
 *  Good 'ole Max and Min from the defunct <X11/Misc.h>
 *
 ************************************************************************/
#define Max(x, y)       (((x) > (y)) ? (x) : (y))
#define Min(x, y)       (((x) > (y)) ? (y) : (x))

/************************************************************************
 *
 *  Resource manager definitions
 *
 ************************************************************************/

#define XtRLong		       "Long"

#define XtNtraversalOn         "traversalOn"
#define XtCTraversalOn         "TraversalOn"

#define XtNtraversalType       "traversalType"
#define XtCTraversalType       "TraversalType"
#define XtRTraversalType       "TraversalType"

#define XtNhighlightStyle      "highlightStyle"
#define XtCHighlightStyle      "HighlightStyle"
#define XtNhighlightTile       "highlightTile"
#define XtCHighlightTile       "HighlightTile"
#define XtRHighlightStyle      "HighlightStyle"
#define XtNhighlightThickness  "highlightThickness"
#define XtCHighlightThickness  "HighlightThickness"
#define XtNhighlightColor      "highlightColor"

#define XtNbackgroundTile      "backgroundTile"
#define XtCBackgroundTile      "BackgroundTile"

#define XtNcursor		"cursor"

#define XtNrecomputeSize	"recomputeSize"
#define XtCRecomputeSize	"RecomputeSize"

#define XtNlayout              "layout"
#define XtCLayout              "Layout"
#define XtRLayout              "Layout"

#define XtRTileType		"tileType"

/* #define XtRTranslationTable     "TranslationTable" */
#define XtNlabelLocation        "labelLocation"
#define XtCLabelLocation        "LabelLocation"
#define XtRLabelLocation        "LabelLocation"
#define XtNsensitiveTile	"sensitiveTile"
#define XtCSensitiveTile	"SensitiveTile"

#define XtNcolumns              "columns"
#define XtCColumns              "Columns"
#define XtNmode                 "mode"
#define XtCMode                 "Mode"
#define XtRRCMode               "Mode"
#define XtNset                  "set"
#define XtCSet                  "Set"

#define XtNselect              "select"
#define XtNrelease             "release"
#define XtNnextTop             "nextTop"

#define XtNtitleShowing        "titleShowing"
#define XtCTitleShowing        "TitleShowing"
#define XtNmgrTitleOverride    "mgrTitleOverride"
#define XtCTitleOverride       "TitleOverride"
#define XtNtitleType           "titleType"
#define XtCTitleType           "TitleType"
#define XtRTitleType           "TitleType"
#define XtNtitleString         "titleString"
#define XtCTitleString         "TitleString"
#define XtNtitleImage          "titleImage"
#define XtCTitleImage          "TitleImage"
#define XtNfontColor           "fontColor"
#define XtNmnemonic            "mnemonic"
#define XtCMnemonic            "Mnemonic"
#define XtNunderlineTitle      "underlineTitle"
#define XtCUnderlineTitle      "UnderlineTitle"
#define XtNmgrUnderlineOverride "mgrUnderlineOverride"
#define XtCUnderlineOverride   "UnderlineOverride"
#define XtNunderlinePosition   "underlinePosition"
#define XtCUnderlinePosition   "UnderlinePosition"
#define XtNattachTo            "attachTo"
#define XtCAttachTo            "AttachTo"
#define XtNkbdAccelerator      "kbdAccelerator"
#define XtCKbdAccelerator      "KbdAccelerator"

#define XtNassociateChildren   "associateChildren"
#define XtCAssociateChildren   "AssociateChildren"
#define XtNmenuPost            "menuPost"
#define XtCMenuPost            "MenuPost"
#define XtNmenuSelect          "menuSelect"
#define XtCMenuSelect          "MenuSelect"
#define XtNpostAccelerator     "postAccelerator"
#define XtCPostAccelerator     "PostAccelerator"
#define XtNmenuUnpost          "menuUnpost"
#define XtCMenuUnpost          "MenuUnpost"
#define XtNkbdSelect           "kbdSelect"
#define XtCKbdSelect           "KbdSelect"


/****************************************************************
 *
 * TextEdit widget
 *
 ****************************************************************/

#define XtNdisplayPosition      "displayPosition"
/* #define XtNinsertPosition	"insertPosition" */
#define XtNleftMargin		"leftMargin"
#define XtNrightMargin		"rightMargin"
#define XtNtopMargin		"topMargin"
#define XtNbottomMargin		"bottomMargin"
/* #define XtNselectionArray	"selectionArray"
#define XtNtextSource		"textSource"
#define XtNselection		"selection" */
#define XtNmaximumSize		"maximumSize"
#define XtCMaximumSize          "MaximumSize"

/* #define XtNeditType		"editType"
#define XtNfile			"file"
#define XtNstring		"string"
#define XtNlength		"length"
#define XtNfont			"font" */
#define XtNdiskSrc              "disksrc"
#define XtNstringSrc            "stringsrc"
#define XtNexecute              "execute"

#define XtNsourceType           "sourceType"
#define XtCSourceType           "SourceType"
#define XtRSourceType           "SourceType"

#define XtNmotionVerification	"motionVerification"
#define XtNmodifyVerification	"modifyVerification"
#define XtNleaveVerification	"leaveVerification"

#define XtNwrap			"wrap"
#define XtCWrap			"Wrap"
#define XtRWrap			"Wrap"

#define XtNwrapForm		"wrapForm"
#define XtCWrapForm		"WrapForm"
#define XtRWrapForm		XtCWrapForm

#define XtNwrapBreak		"wrapBreak"
#define XtCWrapBreak		"WrapBreak"
#define XtRWrapBreak		XtCWrapBreak

#define XtNscroll		"scroll"
#define XtCScroll		"Scroll"
#define XtRScroll		XtCScroll

#define XtNgrow			"grow"
#define XtCGrow			"Grow"
#define XtRGrow			XtCGrow

#define XwAutoScrollOff		0
#define XwAutoScrollHorizontal	1
#define XwAutoScrollVertical	2
#define XwAutoScrollBoth	3  /* must be bitwise OR of horiz. and vert. */

#define XwGrowOff		0
#define XwGrowHorizontal	1
#define XwGrowVertical		2
#define XwGrowBoth		3  /* must be bitwise OR of horiz. and vert. */

/* Valid Label Location Settings for Button */
   
#define XwRIGHT         0       
#define XwLEFT          1
#define XwCENTER        2

/* Valid Title Type Values For MenuPane */

#define XwSTRING  0
#define XwIMAGE   1
#define XwRECT	  2


/* New resource manager types */

#define XrmRImage	      	"Image"
#define XtRImage	      	XrmRImage
#define XtNalignment	"alignment"
#define XtCAlignment	"Alignment"
#define XtRAlignment	"Alignment"
#define XtNlineSpace 	"lineSpace"
#define XtCLineSpace 	"LineSpace"

#define XtNgravity		"gravity"
#define XtCGravity		"Gravity"
#ifndef XtRGravity
#define XtRGravity		"Gravity"
#endif

typedef enum {
	XwALIGN_NONE,
	XwALIGN_LEFT,
	XwALIGN_CENTER,
	XwALIGN_RIGHT
} XwAlignment;

typedef enum {
	XwUNKNOWN,
	XwPULLDOWN,
	XwTITLE,
	XwWORK_SPACE
} XwWidgetType;

/*  Form resource definitions  */

#define XwHORIZONTAL    0
#define XwVERTICAL      1

#define XtNxRefName		"xRefName"
#define XtCXRefName		"XRefName"
#define XtNxRefWidget		"xRefWidget"
#define XtCXRefWidget		"XRefWidget"
#define XtNxOffset		"xOffset"
#define XtCXOffset		"XOffset"
#define XtNxAddWidth		"xAddWidth"
#define XtCXAddWidth		"XAddWidth"
#define XtNxVaryOffset		"xVaryOffset"
#define XtCXVaryOffset		"XVaryOffset"
#define XtNxResizable		"xResizable"
#define XtCXResizable		"XResizable"
#define XtNxAttachRight		"xAttachRight"
#define XtCXAttachRight		"XAttachRight"
#define XtNxAttachOffset	"xAttachOffset"
#define XtCXAttachOffset	"XAttachOffset"

#define XtNyRefName		"yRefName"
#define XtCYRefName		"YRefName"
#define XtNyRefWidget		"yRefWidget"
#define XtCYRefWidget		"YRefWidget"
#define XtNyOffset		"yOffset"
#define XtCYOffset		"YOffset"
#define XtNyAddHeight		"yAddHeight"
#define XtCYAddHeight		"YAddHeight"
#define XtNyVaryOffset		"yVaryOffset"
#define XtCYVaryOffset		"YVaryOffset"
#define XtNyResizable		"yResizable"
#define XtCYResizable		"YResizable"
#define XtNyAttachBottom	"yAttachBottom"
#define XtCYAttachBottom	"YAttachBottom"
#define XtNyAttachOffset	"yAttachOffset"
#define XtCYAttachOffset	"YAttachOffset"

/*  MenuBtn esource manager definitions  */

#define XtNlabelType	      "labelType"
#define XtCLabelType	      "LabelType"
#define XtRLabelType	      "LabelType"
#define XtNlabelImage         "labelImage"
#define XtCLabelImage         "LabelImage"
#define XtNrectColor	      "rectColor"
#define XtCRectColor	      "RectColor"
#define XtNrectStipple	      "rectStipple"
#define XtCRectStipple	      "RectStipple"
#define XtNcascadeImage       "cascadeImage"
#define XtCCascadeImage       "CascadeImage"
#define XtNmarkImage          "markImage"
#define XtCMarkImage          "MarkImage"
#define XtNsetMark            "setMark"
#define XtCSetMark            "SetMark"
#define XtNnoPad              "noPad"
#define XtCNoPad              "NoPad"
#define XtNcascadeOn          "cascadeOn"
#define XtCCascadeOn	      "CascadeOn"
#define XtNinvertOnEnter	"invertOnEnter"
#define XtCInvertOnEnter	"InvertOnEnter"
#define XtNmgrOverrideMnemonic  "mgrOverrideMnemonic"
#define XtCMgrOverrideMnemonic  "MgrOverrideMnemonic"
#define XtNcascadeSelect      "cascadeSelect"
#define XtNcascadeUnselect    "cascadeUnselect"
#define XtNmenuMgrId	      "menuMgrId"
#define XtCMenuMgrId	      "MenuMgrId"
#define XtNhint		      "hint"
#define XtCHint		      "Hint"
#define XtNhintProc	      "hintProc"
#define XtCHintProc	      "HintProc"

/*  Resources for PushButton  */

#define XtNinvertOnSelect	"invertOnSelect"
#define XtCInvertOnSelect	"InvertOnSelect"
#define XtNtoggle               "toggle"
#define XtCToggle               "Toggle"

/*  Resources for Toggle  */
 
#define XtNsquare       "square"  
#define XtCSquare       "Square" 
#define XtNselectColor  "selectColor"

/*  WorkSpace resources  */

#define XtNexpose	"expose"
#ifndef XtNresize
#define XtNresize	"resize"
#endif
#define XtNkeyDown	"keyDown"
#define XtNkeyUp	"keyUp"


/***********************************************************************
 *
 * Cascading MenuPane Widget Resources
 *
 ***********************************************************************/

#define XtRTitlePositionType    "TitlePositionType"
#define XtNtitlePosition        "titlePosition"
#define XtCTitlePosition        "TitlePosition"

/***********************************************************************
 *
 * Popup Menu Manager Widget Resources
 *
 ***********************************************************************/

#define XtNstickyMenus        "stickyMenus"
#define XtCStickyMenus        "StickyMenus"

/***********************************************************************
 *
 * Pulldown Menu Manager Widget Resources
 *
 ***********************************************************************/


#define XtNallowCascades        "allowCascades"
#define XtCAllowCascades        "AllowCascades"
#define XtNpulldownBarId        "pulldownBarId"
#define XtCPulldownBarId        "PulldownBarId"


/***********************************************************************
 *
 * Cascading MenuPane Widget Attribute Definitions
 *
 ***********************************************************************/

#define XwTOP    0x01
#define XwBOTTOM 0x02
#define XwBOTH   XwTOP|XwBOTTOM

/***********************************************************************
 *
 * Static Text resource string definitions
 *
 ***********************************************************************/
#define XtNstrip	"strip"
#define XtCStrip	"Strip"

/************************************************************************
 *
 *  Class record constants for Primitive Widget Meta Class
 *
 ************************************************************************/

extern WidgetClass XwprimitiveWidgetClass;

typedef struct _XwPrimitiveClassRec * XwPrimitiveWidgetClass;
typedef struct _XwPrimitiveRec      * XwPrimitiveWidget;


/*  Tile types used for area filling and patterned text placement  */

#define XwFOREGROUND		0
#define XwBACKGROUND		1

/*  Traversal type definitions  */

#define XwHIGHLIGHT_OFF		0
#define XwHIGHLIGHT_ENTER	1
#define XwHIGHLIGHT_TRAVERSAL	2


/*  Border highlighting type defines  */

#define XwPATTERN_BORDER   1
#define XwWIDGET_DEFINED   2


/* Manager Layout Info */

#define XwIGNORE           0
#define XwMINIMIZE         1
#define XwMAXIMIZE         2
#define XwSWINDOW          3   /* Special Setting for Scrolled Window */

#define XtNwidgetType            "widgetType"
#define XtCWidgetType            "WidgetType"
#define XtRWidgetType            "WidgetType"

/************************************************************************
 *
 *  Class record constants for Meta Class Widgets
 *
 ************************************************************************/

extern WidgetClass XwmanagerWidgetClass;

typedef struct _XwManagerClassRec * XwManagerWidgetClass;
typedef struct _XwManagerRec      * XwManagerWidget;



extern WidgetClass XwbuttonWidgetClass;

typedef struct _XwButtonClassRec * XwButtonWidgetClass;
typedef struct _XwButtonRec      * XwButtonWidget;


extern WidgetClass XwmenupaneWidgetClass;

typedef struct _XwMenuPaneClassRec *XwMenuPaneWidgetClass;
typedef struct _XwMenuPaneRec      *XwMenuPaneWidget;


extern WidgetClass XwmenumgrWidgetClass;

typedef struct _XwMenuMgrClassRec *XwMenuMgrWidgetClass;
typedef struct _XwMenuMgrRec      *XwMenuMgrWidget;



#endif
/* DON'T ADD STUFF AFTER THIS #endif */


