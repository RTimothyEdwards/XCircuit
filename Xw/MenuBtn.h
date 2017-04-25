/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        MenuBtn.h
 **
 **   Project:     X Widgets
 **
 **   Description: Public include file for MenuButton class widgets
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
 * MenuButton Widget
 *
 ***********************************************************************/


extern WidgetClass XwmenubuttonWidgetClass;

/* synonym added for consistent naming conventions */
extern WidgetClass XwmenuButtonWidgetClass;

typedef struct _XwMenuButtonClassRec *XwMenuButtonWidgetClass;
typedef struct _XwMenuButtonRec      *XwMenuButtonWidget;



/********************
 *
 * CascadeUnselect callback data typedef
 *
 ********************/

typedef struct
{
   Dimension   rootX;
   Dimension   rootY;
   Boolean     remainHighlighted;
} XwunselectParams;
