/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        BBoardP.h
 **
 **   Project:     X Widgets 
 **
 **   Description: Private include file for widgets which are
 **                subclasses of bulletin board or which 
 **                need to access directly the instance/class
 **                fields of the bulletin board widget.
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
 * BulletinBoard Widget Private Data
 *
 ***********************************************************************/

/* New fields for the BulletinBoard widget class record */
typedef struct {
     int mumble;   /* No new procedures */
} XwBulletinClassPart;

/* Full class record declaration */
typedef struct _XwBulletinClassRec {
    CoreClassPart	core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XwManagerClassPart  manager_class;
    XwBulletinClassPart	bulletin_board_class;
} XwBulletinClassRec;

extern XwBulletinClassRec XwbulletinClassRec;

/* New fields for the Bulletin widget record */
typedef struct {
   int mumble;  /* No new fields */
} XwBulletinPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XwBulletinRec {
    CorePart	    core;
    CompositePart   composite;
    ConstraintPart  constraint;
    XwManagerPart   manager;
    XwBulletinPart   bulletin_board;
} XwBulletinRec;

