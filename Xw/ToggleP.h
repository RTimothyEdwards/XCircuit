/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        ToggleP.h    
 **
 **   Project:     X Widgets
 **
 **   Description: Private include file for Toggle class, used by 
 **                widgets which are subclasses of the toggle widget
 **                or which need to get direct access to the instance
 **                and class fields of the toggle widget.
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
 *   for the Toggle widget class record
 *
 ********************************************/

typedef struct {int foo;} XwToggleClassPart;


/****************************************************
 *
 * Full class record declaration for Toggle class
 *
 ****************************************************/
typedef struct _XwToggleClassRec {
    CoreClassPart	  core_class;
    XwPrimitiveClassPart  primitive_class;
    XwButtonClassPart     button_class;
    XwToggleClassPart	  toggle_class;
} XwToggleClassRec;


extern XwToggleClassRec XwtoggleClassRec;


/********************************************
 *
 * No new fields needed for instance record
 *
 ********************************************/

typedef struct _XwTogglePart
{ 
   Boolean	square;
   Pixel	select_color;
   GC		select_GC;
} XwTogglePart;



/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XwToggleRec {
    CorePart	       core;
    XwPrimitivePart    primitive;
    XwButtonPart       button;
    XwTogglePart       toggle;
} XwToggleRec;
