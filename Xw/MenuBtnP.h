/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        MenuBtnP.h    
 **
 **   Project:     X Widgets
 **
 **   Description: Private include file for MenuButton class
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

#define XwMARKWIDTH    16
#define XwCASCADEWIDTH 16
#define XwMENUBTNPAD    6

/********************************************
 *
 *   the MenuButton widget class record
 *
 ********************************************/

typedef struct _XwMenuButtonClassPart
{
   XwWidthProc  idealWidthProc;
   XtWidgetProc unhighlightProc;
   XtWidgetProc highlightProc;
   XtWidgetProc setCascadeProc;
   XtWidgetProc clearCascadeProc;
   XtWidgetProc enterParentProc;
   XtWidgetProc cascadeSelectProc;
   XtWidgetProc cascadeUnselectProc;
   XwWintProc   setTraversalType;
} XwMenuButtonClassPart;

#define XtInheritSetCascadeProc ((XtWidgetProc) _XtInherit)
#define XtInheritClearCascadeProc ((XtWidgetProc) _XtInherit)
#define XtInheritCascadeSelectProc ((XtWidgetProc) _XtInherit)
#define XtInheritCascadeUnselectProc ((XtWidgetProc) _XtInherit)
#define XtInheritSetTraversalTypeProc ((XwWintProc) _XtInherit)

/****************************************************
 *
 * Full class record declaration for MenuButton class
 *
 ****************************************************/
typedef struct _XwMenuButtonClassRec {
    CoreClassPart	  core_class;
    XwPrimitiveClassPart  primitive_class;
    XwButtonClassPart     button_class;
    XwMenuButtonClassPart menubutton_class;
} XwMenuButtonClassRec;


extern XwMenuButtonClassRec XwmenubuttonClassRec;

/********************************************
 *
 * New fields for the instance record
 *
 ********************************************/

typedef struct _XwMenuButtonPart
{
   /* can be set through resources */

    int         labelType;
    XImage    * labelImage;
    XImage    * cascadeImage;
    XImage    * markImage;
    Pixel	rectColor;
    Pixmap	rectStipple;
    Boolean	setMark;
    Boolean	noPad;
    Widget	cascadeOn;
    char      * accelerator;
    char      * hint;
    XwStrProc	hintProc;
    Boolean	mgrOverrideMnemonic;
    String      mnemonic;
    XtCallbackList cascadeSelect;
    XtCallbackList cascadeUnselect;

   /* can not be set through resources */

    Widget	menuMgr;
    Dimension	mark_y;
    Dimension	cascade_y;
    Dimension	underline_x;
    Dimension	underline_y;
    Dimension	underline_width;

    Boolean	mnemonicMatch;
    Boolean	cascadeEnabled;
    Boolean	inverted;

    unsigned int accelEventType;
    KeyCode      accelDetail;
    unsigned int accelModifiers;
       
    Pixmap	labelPixmap;
    Pixmap	markPixmap;
    Pixmap	cascadePixmap;
    Pixmap	invertLabelPixmap;
    Pixmap	invertMarkPixmap;
    Pixmap	invertCascadePixmap;
    XImage    * defMarkImage;
    XImage    * defCascadeImage;

    GC		inverted_GC;
    GC		defPixmap_GC;
    GC		invertPixmap_GC;
    GC		rect_GC;
    
} XwMenuButtonPart;



/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _XwMenuButtonRec {
    CorePart	       core;
    XwPrimitivePart    primitive;
    XwButtonPart       button;
    XwMenuButtonPart   menubutton;
} XwMenuButtonRec;



    
