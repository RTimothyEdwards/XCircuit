/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        CascadeP.h
 **
 **   Project:     X Widgets
 **
 **   Description: Private include file for Cascading menupane class widgets
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
 * Cascading MenuPane Widget Private Data
 *
 ***********************************************************************/

/* New fields for the Cascade widget class record */
typedef struct {
     int mumble;   /* No new procedures */
} XwCascadeClassPart;

/* Full class record declaration */
typedef struct _XwCascadeClassRec {
    CoreClassPart	core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XwManagerClassPart  manager_class;
    XwMenuPaneClassPart	menu_pane_class;
    XwCascadeClassPart	cascade_class;
} XwCascadeClassRec;

extern XwCascadeClassRec XwcascadeClassRec;

/* New fields for the Cascade widget record */
typedef struct {
    /* Internal fields */
    Position      title_x;
    Position      top_title_y;
    Position      bottom_title_y;
    Dimension     title_height;
    Dimension     title_width;
    Position      button_starting_loc;
    Dimension     idealWidth;
    Dimension     idealHeight;
    Dimension     largestButtonWidth;
    Widget        georeqWidget;
    Dimension     georeqWidth;

    /* User settable fields */
    int           titlePosition;
} XwCascadePart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XwCascadeRec {
    CorePart	    core;
    CompositePart   composite;
    ConstraintPart  constraint;
    XwManagerPart   manager;
    XwMenuPanePart  menu_pane;
    XwCascadePart   cascade;
} XwCascadeRec;

