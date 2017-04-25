/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        PopupMgrP.h
 **
 **   Project:     X Widgets
 **
 **   Description: Private include file for Popup Menu Manager class widgets
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
 * Popup Menu Manager Widget Private Data
 *
 ***********************************************************************/

/* New fields for the PopupMgr widget class record */
typedef struct {
     XwPostProc manualPost;
} XwPopupMgrClassPart;

/* Full class record declaration */
typedef struct _XwPopupMgrClassRec {
    CoreClassPart	core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XwManagerClassPart  manager_class;
    XwMenuMgrClassPart	menu_mgr_class;
    XwPopupMgrClassPart	popup_mgr_class;
} XwPopupMgrClassRec;

extern XwPopupMgrClassRec XwpopupmgrClassRec;

/* New fields for the PopupMgr widget record */
typedef struct {
    /* Internal fields */
    unsigned int accelEventType;
    KeyCode      accelKey;
    unsigned int accelModifiers;
    Widget       topLevelPane;
    Widget       lastSelected;
    Widget     * savedCascadeList;
    int          numSavedCascades;
    int          sizeSavedCascadeList;
    Widget     * currentCascadeList;
    int          numCascades;
    int          sizeCascadeList;
    XwMenuPaneWidget attachPane;
    XwMenuPaneWidget detachPane;
    Position     origMouseX;
    Position     origMouseY;

    /* User settable fields */
    Boolean      stickyMode;
    String       postAccelerator;
} XwPopupMgrPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XwPopupMgrRec {
    CorePart	    core;
    CompositePart   composite;
    ConstraintPart  constraint;
    XwManagerPart   manager;
    XwMenuMgrPart   menu_mgr;
    XwPopupMgrPart  popup_mgr;
} XwPopupMgrRec;


#define XtInheritPopupTravLeft    ((XwEventProc) _XtInherit)
#define XtInheritPopupTravRight   ((XwEventProc) _XtInherit)
#define XtInheritPopupTravUp      ((XwEventProc) _XtInherit)
#define XtInheritPopupTravDown    ((XwEventProc) _XtInherit)
#define XtInheritPopupTravNext    ((XwEventProc) _XtInherit)
#define XtInheritPopupTravPrev    ((XwEventProc) _XtInherit)
#define XtInheritPopupTravHome    ((XwEventProc) _XtInherit)
#define XtInheritPopupTravNextTop ((XwEventProc) _XtInherit)
