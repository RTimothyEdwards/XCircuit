/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        PopupMgr.h
 **
 **   Project:     X Widgets
 **
 **   Description: Public include file for popup menu manager class widgets
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


/***********************************************************************
 *
 * Popup Menu Manager Widget (subclass of MenuMgr)
 *
 ***********************************************************************/


/* Class record constants */

extern WidgetClass XwpopupmgrWidgetClass;
/* synonym added for consistent naming conventions */
extern WidgetClass XwpopupMgrWidgetClass;

typedef struct _XwPopupMgrClassRec *XwPopupMgrWidgetClass;
typedef struct _XwPopupMgrRec      *XwPopupMgrWidget;

extern void XwAppInitialize(XtAppContext);
extern void XwPostPopup(Widget, Widget, Widget, Position, Position);
