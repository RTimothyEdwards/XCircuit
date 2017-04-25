/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        PButtonP.h
 **
 **   Project:     X Widgets 
 **
 **   Description: Private include file for widgets which are
 **                subclasses of push button or which need to
 **                access directly the instance and class fields
 **                of the pushbutton widget.
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

/********************************************
 *
 *   No new fields need to be defined
 *   for the PushButton widget class record
 *
 ********************************************/

typedef struct {int foo;} XwPushButtonClassPart;


/****************************************************
 *
 * Full class record declaration for PushButton class
 *
 ****************************************************/
typedef struct _XwPushButtonClassRec {
    CoreClassPart	  core_class;
    XwPrimitiveClassPart  primitive_class;
    XwButtonClassPart     button_class;
    XwPushButtonClassPart pushbutton_class;
} XwPushButtonClassRec;


extern XwPushButtonClassRec XwpushButtonClassRec;


/********************************************
 *
 * No new fields needed for instance record
 *
 ********************************************/

typedef struct _XwPushButtonPart{
     Boolean toggle;
} XwPushButtonPart;



/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XwPushButtonRec {
    CorePart	     core;
    XwPrimitivePart  primitive;
    XwButtonPart     button;
    XwPushButtonPart pushbutton;
} XwPushButtonRec;
