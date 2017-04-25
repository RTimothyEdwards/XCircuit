/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        MenuBtn.c
 **
 **   Project:     X Widgets
 **
 **   Description: Contains code for primitive widget class: MenuButton
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

/*
 *#define DEBUG TRUE 
 */
#undef NLS16

/*
 * Include files & Static Routine Definitions
 */
#include <string.h>
#include <X11/IntrinsicP.h>   
#include <X11/StringDefs.h>
#include <X11/keysymdef.h>   
#include <X11/Xatom.h>
#include <X11/Shell.h>   
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <Xw/MenuBtnP.h>
#include <Xw/MenuBtn.h>

#ifdef NLS16
#include <X11/XHPlib.h>
#endif
   
#ifndef XtRWidget
#define XtRWidget XtRPointer
#endif
   
static void Redisplay();
static Boolean SetValues();
static void ClassPartInitialize();
static void Inverted();
static void NonInverted();
static void Select();
static void Enter();
static void Leave();
static void Moved();
static void Initialize();
static void Destroy();
static void Realize();
static void Resize();
static void IdealWidth();
static void Unhighlight();
static void Highlight();
static void SetCascadeEnabled();
static void ClearCascadeEnabled();
static void EnterParentsWindow();
static void SetTraversalType();
static void TraverseLeft();
static void TraverseRight();
static void TraverseNext();
static void TraversePrev();
static void TraverseHome();
static void TraverseUp();
static void TraverseDown();
static void TraverseNextTop();
static void Unmap();
static void Visibility();
Boolean _XwUniqueEvent();

/*************************************<->*************************************
 *
 *
 *   Description:  default translation table for class: MenuButton
 *   -----------
 *
 *   Matches events with string descriptors for internal routines.
 *
 *************************************<->***********************************/


static char defaultTranslations[] = 
    "<Btn1Down>:             select()\n\
     <Visible>:              visibility()\n\
     <Unmap>:                unmap()\n\
     <EnterWindow>:          enter()\n\
     <LeaveWindow>:          leave()\n\
     <Motion>:	             moved()\n\
     <Key>Select:            select()\n\
     <Key>Left:              traverseLeft()\n\
     <Key>Up:                traverseUp()\n\
     <Key>Right:             traverseRight()\n\
     <Key>Down:              traverseDown()\n\
     <Key>Prior:             traversePrev()\n\
     <Key>Next:              traverseNext()\n\
     <Key>KP_Enter:          traverseNextTop()\n\
     <Key>Home:              traverseHome()";



/*************************************<->*************************************
 *
 *
 *   Description:  action list for class: MenuButton
 *   -----------
 *
 *   Matches string descriptors with internal routines.
 *
 *************************************<->***********************************/

static XtActionsRec actionsList[] =
{
  {"select", 	      (XtActionProc) Select},
  {"visibility",      (XtActionProc) Visibility},
  {"unmap", 	      (XtActionProc) Unmap},
  {"enter", 	      (XtActionProc) Enter},
  {"leave", 	      (XtActionProc) Leave},
  {"moved", 	      (XtActionProc) Moved},
  {"traverseLeft",    (XtActionProc) TraverseLeft },
  {"traverseRight",   (XtActionProc) TraverseRight },
  {"traverseNext",    (XtActionProc) TraverseNext },
  {"traversePrev",    (XtActionProc) TraversePrev },
  {"traverseHome",    (XtActionProc) TraverseHome },
  {"traverseUp",      (XtActionProc) TraverseUp },
  {"traverseDown",    (XtActionProc) TraverseDown },
  {"traverseNextTop", (XtActionProc) TraverseNextTop },
};

/*************************************<->*************************************
 *
 *
 *   Description:  resource list for class: MenuButton
 *   -----------
 *
 *   Provides default resource settings for instances of this class.
 *   To get full set of default settings, examine resouce list of super
 *   classes of this class.
 *
 *************************************<->***********************************/

static XtResource resources[] =
{
   {
      XtNborderWidth, XtCBorderWidth,XtRDimension, sizeof(Dimension),
      XtOffset(XwMenuButtonWidget, core.border_width),
      XtRString, "0"
   },

   {
      XtNlabelType, XtCLabelType, XtRLabelType, sizeof(int),
      XtOffset(XwMenuButtonWidget, menubutton.labelType),
      XtRString, "string"
   },

   {
      XtNlabelImage, XtCLabelImage, XtRImage, sizeof(XImage *),
      XtOffset(XwMenuButtonWidget, menubutton.labelImage),
      XtRImage, NULL
   },

   {
      XtNrectColor, XtCRectColor, XtRPixel, sizeof(Pixel),
      XtOffset(XwMenuButtonWidget, menubutton.rectColor),
      XtRString, "white"
   },

   {
      XtNrectStipple, XtCRectStipple, XtRPixmap, sizeof(Pixmap),
      XtOffset(XwMenuButtonWidget, menubutton.rectStipple),
      XtRPixmap, NULL
   },

   {
      XtNcascadeImage, XtCCascadeImage, XtRImage, sizeof(XImage *),
      XtOffset(XwMenuButtonWidget, menubutton.cascadeImage),
      XtRImage, NULL
   },

   {
      XtNmarkImage, XtCMarkImage, XtRImage, sizeof(XImage *),
      XtOffset(XwMenuButtonWidget, menubutton.markImage),
      XtRImage, NULL
   },
       
   {
      XtNsetMark, XtCSetMark, XtRBoolean, sizeof(Boolean),
      XtOffset(XwMenuButtonWidget, menubutton.setMark),
      XtRString, "FALSE"
   },

   {
      XtNnoPad, XtCNoPad, XtRBoolean, sizeof(Boolean),
      XtOffset(XwMenuButtonWidget, menubutton.noPad),
      XtRString, "FALSE"
   },

   {
      XtNcascadeOn, XtCCascadeOn, XtRWidget, sizeof(Widget),
      XtOffset(XwMenuButtonWidget, menubutton.cascadeOn),
      XtRWidget, NULL
   },

   {
      XtNkbdAccelerator, XtCKbdAccelerator, XtRString, sizeof(caddr_t),
      XtOffset(XwMenuButtonWidget, menubutton.accelerator),
      XtRString, NULL
   },

   {
      XtNhint, XtCHint, XtRString, sizeof(caddr_t),
      XtOffset(XwMenuButtonWidget, menubutton.hint),
      XtRString, NULL
   },

   {
      XtNhintProc, XtCHintProc, XtRFunction, sizeof(XwStrProc),
      XtOffset(XwMenuButtonWidget, menubutton.hintProc),
      XtRFunction, NULL
   },

   {
      XtNmgrOverrideMnemonic, XtCMgrOverrideMnemonic, XtRBoolean,
      sizeof(Boolean),
      XtOffset(XwMenuButtonWidget, menubutton.mgrOverrideMnemonic),
      XtRString, "FALSE"
   },

   {
      XtNmnemonic, XtCMnemonic, XtRString, sizeof(caddr_t),
      XtOffset(XwMenuButtonWidget, menubutton.mnemonic),
      XtRString, NULL
   },
	 
   {
      XtNhighlightStyle, XtCHighlightStyle, XtRHighlightStyle, sizeof (int),
      XtOffset (XwPrimitiveWidget, primitive.highlight_style),
      XtRString, "widget_defined"
   },
	 
   {
      XtNcascadeSelect, XtCCallback, XtRCallback, sizeof(caddr_t),
      XtOffset (XwMenuButtonWidget, menubutton.cascadeSelect),
      XtRPointer, (caddr_t) NULL
   },
   
   {
      XtNcascadeUnselect, XtCCallback, XtRCallback, sizeof(caddr_t),
      XtOffset (XwMenuButtonWidget, menubutton.cascadeUnselect),
      XtRPointer, (caddr_t) NULL
   },

   {
      XtNmenuMgrId, XtCMenuMgrId, XtRWidget, sizeof(Widget),
      XtOffset (XwMenuButtonWidget, menubutton.menuMgr),
      XtRWidget, NULL
   },
};

/*************************************<->*************************************
 *
 *
 *   Description:  global class record for instances of class: MenuButton
 *   -----------
 *
 *   Defines default field settings for this class record.
 *
 *************************************<->***********************************/

XwMenuButtonClassRec XwmenubuttonClassRec =
{
  {
/* core_class fields */	
    /* superclass	  */	(WidgetClass) &XwbuttonClassRec,
    /* class_name	  */	"XwMenuButton",
    /* widget_size	  */	sizeof(XwMenuButtonRec),
    /* class_initialize   */    NULL,
    /* class_part_init    */    ClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize	  */	Initialize,
    /* initialize_hook    */	NULL,
    /* realize		  */	Realize,
    /* actions		  */	actionsList,
    /* num_actions	  */	XtNumber(actionsList),
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterlv   */	TRUE,    /* FIX ME LATER */
    /* visible_interest	  */	FALSE,
    /* destroy		  */	Destroy,
    /* resize		  */	Resize,
    /* expose		  */	Redisplay,
    /* set_values	  */	SetValues,
    /* set_values_hook    */	NULL,
    /* set_values_almost  */	XtInheritSetValuesAlmost,
    /* get_values_hook    */	NULL,
    /* accept_focus	  */	NULL,
    /* version     	  */	XtVersion,
    /* cb List     	  */	NULL,
    /* tm_table    	  */	defaultTranslations,
    /* query_geometry     */	NULL,     /* FIX ME LATER */
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },
  {
     /* primitive class fields */
     /* border_highlight   */	Inverted,
     /* border_unhighlight */	NonInverted,
     /* select_proc        */	Select,
     /* release_proc       */	NULL,
     /* toggle_proc        */	NULL,
     /* translations       */	NULL,
  },
  {
     /* button class fields */	0,
  },
  {
     /* menubutton class fields */
     /* ideal width proc  */	IdealWidth,
     /* unhighlight proc  */    Unhighlight,
     /* highlight proc    */    Highlight,
     /* cascade selected  */    SetCascadeEnabled,
     /* cascade unselected*/    ClearCascadeEnabled,
     /* enter parents win */    EnterParentsWindow,
     /* cascadeSelectProc */	NULL,
     /* cascadeUnselectProc */  NULL,
     /* setTraversalType    */  SetTraversalType
  }
};
WidgetClass XwmenubuttonWidgetClass = (WidgetClass)&XwmenubuttonClassRec;
WidgetClass XwmenuButtonWidgetClass = (WidgetClass)&XwmenubuttonClassRec;


/*************************************<->*************************************
 *
 *   SetCascadeEnabled (mbutton)
 *
 *   Description:
 *   -----------
 *   This routine is called by the Menu Manager to force the menubutton into
 *   thinking that the cascade select callbacks have been recently called.
 *
 *   Inputs:
 *   ------
 *   mbutton = this menubutton widget
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
static void SetCascadeEnabled (mbutton)
XwMenuButtonWidget mbutton;
{
   mbutton->menubutton.cascadeEnabled = TRUE;
}

/*************************************<->*************************************
 *
 *   ClearCascadeEnabled (mbutton)
 *
 *   Description:
 *   -----------
 *   This routine is called by the Menu Manager to force the menubutton into
 *   thinking that the cascade unselect callbacks have been recently called.
 *
 *   Inputs:
 *   ------
 *   mbutton = this menubutton widget
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
static void ClearCascadeEnabled (mbutton)
XwMenuButtonWidget mbutton;
{
   mbutton->menubutton.cascadeEnabled = FALSE;
}

/*************************************<->*************************************
 *
 *  ClassPartInitialize (parameters)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void ClassPartInitialize (wc)

   register XwMenuButtonWidgetClass wc;

{
   register XwMenuButtonWidgetClass super =
            (XwMenuButtonWidgetClass) wc->core_class.superclass;

   if (wc->menubutton_class.idealWidthProc == XtInheritIdealWidthProc)
      wc->menubutton_class.idealWidthProc = 
            super->menubutton_class.idealWidthProc;

   if (wc->menubutton_class.unhighlightProc == XtInheritUnhighlightProc)
      wc->menubutton_class.unhighlightProc = 
            super->menubutton_class.unhighlightProc;

   if (wc->menubutton_class.highlightProc == XtInheritHighlightProc)
      wc->menubutton_class.highlightProc = 
            super->menubutton_class.highlightProc;

   if (wc->menubutton_class.setCascadeProc == XtInheritSetCascadeProc)
      wc->menubutton_class.setCascadeProc = 
            super->menubutton_class.setCascadeProc;

   if (wc->menubutton_class.clearCascadeProc == XtInheritClearCascadeProc)
      wc->menubutton_class.clearCascadeProc = 
            super->menubutton_class.clearCascadeProc;

   if (wc->menubutton_class.enterParentProc == XtInheritEnterParentProc)
      wc->menubutton_class.enterParentProc = 
            super->menubutton_class.enterParentProc;

   if (wc->menubutton_class.cascadeSelectProc == XtInheritCascadeSelectProc)
      wc->menubutton_class.cascadeSelectProc = 
            super->menubutton_class.cascadeSelectProc;

   if (wc->menubutton_class.cascadeUnselectProc == XtInheritCascadeUnselectProc)
      wc->menubutton_class.cascadeUnselectProc = 
            super->menubutton_class.cascadeUnselectProc;

   if (wc->menubutton_class.setTraversalType == XtInheritSetTraversalTypeProc)
      wc->menubutton_class.setTraversalType = 
            super->menubutton_class.setTraversalType;
}


/*************************************<->*************************************
 *
 *   ComputeHeight (mbutton)
 *
 *   Description:
 *   -----------
 *   Returns the ideal height of the menubutton by considering whether the
 *   label is text or image.
 *
 *   Inputs:
 *   ------
 *   mbutton = widget to compute height of
 * 
 *   Outputs:
 *   -------
 *   returns the ideal height of mbutton
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Dimension ComputeHeight(mbutton)
XwMenuButtonWidget mbutton;
{
   Dimension max = 0;

   if (mbutton->menubutton.labelType == XwSTRING)
      max = mbutton->button.label_height;

   if ((mbutton->menubutton.labelType == XwIMAGE) &&
       (mbutton->menubutton.labelImage->height > max))
      max = mbutton->menubutton.labelImage->height;

   if (mbutton->menubutton.markImage->height > max)
      max = mbutton->menubutton.markImage->height;

   if (mbutton->menubutton.cascadeImage->height > max)
      max = mbutton->menubutton.cascadeImage->height;

   return (max + 2 * mbutton->button.internal_height +
	   2 * mbutton->primitive.highlight_thickness);

}

/*************************************<->*************************************
 *
 *   ComputeVertical
 *
 *   Description:
 *   -----------
 *   Computes the values for mark_y, label_y and cascade_y.
 *
 *   Inputs:
 *   ------
 *   mbutton = menubutton to compute and set values for
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void ComputeVertical (mbutton)
XwMenuButtonWidget mbutton;
{
   Dimension string_y;

   string_y = (mbutton->core.height - mbutton->button.label_height)/2 +
               mbutton->button.font->max_bounds.ascent;

   if (mbutton->menubutton.labelType == XwSTRING) 
      mbutton->button.label_y = string_y;

   else if (mbutton->menubutton.labelImage)
      mbutton->button.label_y = (mbutton->core.height -
		 		   mbutton->menubutton.labelImage->height) / 2;

   mbutton->menubutton.mark_y = (mbutton->core.height -
				 mbutton->menubutton.markImage->height)/2;

   mbutton->menubutton.cascade_y = (mbutton->core.height -
				   mbutton->menubutton.cascadeImage->height)/2;

}

/*************************************<->*************************************
 *
 *   SetUnderline (mbutton)
 *
 *   Description:
 *   -----------
 *   Set the underline parameters underline_width and underline_y.
 *
 *   Inputs:
 *   ------
 *   mbutton = menubutton
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   XTextWidth ()
 *   XGetFontProperty ()
 *
 *************************************<->***********************************/

static void SetUnderline (mbutton)
XwMenuButtonWidget mbutton;
{
   int i, temp;

   if ((!mbutton->menubutton.mgrOverrideMnemonic) &&
       (mbutton->menubutton.mnemonic))
   {
      mbutton->menubutton.mnemonicMatch = FALSE;
      temp = XwStrlen (mbutton->button.label);

      for (i = 0; i < temp;)
      {
#ifdef NLS16
        if (XHPIs16bitCharacter (mbutton->button.font->fid,
                                 mbutton->button.label[i],
                                 mbutton->button.label[i+1]) == NULL)
        {
#endif
           if (*mbutton->menubutton.mnemonic == 
               (char) mbutton->button.label[i])
           {
	      unsigned long bval;

              mbutton->menubutton.mnemonicMatch = TRUE;
              mbutton->menubutton.underline_width = 
  	         XTextWidth(mbutton->button.font, mbutton->button.label+i, 1);

              mbutton->menubutton.underline_x =
  	         XTextWidth(mbutton->button.font, mbutton->button.label, i) +
	         mbutton->primitive.highlight_thickness + 
                 4 * mbutton->button.internal_width + XwMARKWIDTH;

	      if (XGetFontProperty(mbutton->button.font, XA_UNDERLINE_POSITION,
			           &bval) == 0)
	         mbutton->menubutton.underline_y = 
                                    mbutton->button.font->descent +1;
	      else
	         mbutton->menubutton.underline_y = (Dimension) bval;

              mbutton->menubutton.underline_y += mbutton->button.label_y;
              break;
            }
            else
               i += 1;
#ifdef NLS16
         }
         else
            i += 2;
#endif
      }
   }
   else
      mbutton->menubutton.mnemonicMatch = FALSE;
}

/*************************************<->*************************************
 *
 *   GetGC (mbutton)
 *
 *   Description:
 *   -----------
 *   Creates image_GC
 *
 *   Inputs:
 *   ------
 *   mbutton = menubutton
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   XtGetGC()
 *
 *************************************<->***********************************/

static void GetGC (mbutton)
XwMenuButtonWidget mbutton;
{
  
   XGCValues values;
   unsigned long dostipple = 0;

   values.function = GXcopy;
   values.plane_mask = AllPlanes;
   values.subwindow_mode = ClipByChildren;
   values.clip_x_origin = 0;
   values.clip_y_origin = 0;
   values.clip_mask = None;
   values.fill_style = FillSolid;
   values.graphics_exposures = True;

   values.foreground = mbutton->primitive.foreground;
   values.background = mbutton->core.background_pixel;

   mbutton->menubutton.defPixmap_GC =
     XtGetGC ((Widget) mbutton,
	      GCFunction | GCPlaneMask | GCSubwindowMode |
	      GCGraphicsExposures | GCClipXOrigin | GCClipYOrigin |
	      GCClipMask | GCForeground | GCBackground,
	      &values);

   values.foreground = mbutton->core.background_pixel;
   values.background = mbutton->primitive.foreground;

   mbutton->menubutton.invertPixmap_GC =
     XtGetGC ((Widget) mbutton,
	      GCFunction | GCPlaneMask | GCSubwindowMode |
	      GCGraphicsExposures | GCClipXOrigin | GCClipYOrigin |
	      GCClipMask | GCForeground | GCBackground,
	      &values);

   values.background = mbutton->core.background_pixel;
   values.stipple = mbutton->menubutton.rectStipple;
   if (values.stipple != (Pixmap)NULL) {
      dostipple = GCStipple;
      values.fill_style = FillOpaqueStippled;
      values.foreground = mbutton->primitive.foreground;
   }
   else
      values.foreground = mbutton->menubutton.rectColor;

   mbutton->menubutton.rect_GC =
     XtGetGC ((Widget) mbutton,
              GCFunction | GCPlaneMask | GCSubwindowMode |
              GCGraphicsExposures | GCClipXOrigin | GCClipYOrigin |
              GCClipMask | GCForeground | GCBackground | dostipple |
	      GCFillStyle, &values);
}

/*************************************<->*************************************
 *
 *   GetDefImages
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 *   mbutton = menubutton
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void GetDefImages (mbutton)
XwMenuButtonWidget mbutton;
{
  static unsigned char defMarkData[] =
  {
   0x00, 0x10, 0x00, 0x38, 0x00, 0x7c, 0x00, 0x3c,
   0x00, 0x1e, 0x00, 0x0e, 0x00, 0x07, 0x00, 0x07,
   0x8c, 0x03, 0x8e, 0x01, 0xde, 0x01, 0xdc, 0x00,
   0xd8, 0x00, 0x70, 0x00, 0x70, 0x00, 0x20, 0x00,
  };

  static unsigned char defCascadeData[] =
  {
   0x80, 0x00, 0x80, 0x01, 0x80, 0x03, 0x80, 0x07,
   0x80, 0x0f, 0xfe, 0x1f, 0xfe, 0x3f, 0xfe, 0x7f,
   0xfe, 0x3f, 0xfe, 0x1f, 0x80, 0x0f, 0x80, 0x07,
   0x80, 0x03, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00
  };

  mbutton->menubutton.defMarkImage =
     XCreateImage (XtDisplay(mbutton), CopyFromParent, 1, XYBitmap, 0,
		   defMarkData, 16, 16, 8, 2);

  mbutton->menubutton.defMarkImage->byte_order = MSBFirst;
  mbutton->menubutton.defMarkImage->bitmap_bit_order = LSBFirst;
  mbutton->menubutton.defMarkImage->bitmap_unit = 8;
     

  mbutton->menubutton.defCascadeImage =
     XCreateImage (XtDisplay(mbutton), CopyFromParent, 1, XYBitmap, 0,
		   defCascadeData, 16, 16, 8, 2);

  mbutton->menubutton.defCascadeImage->byte_order = MSBFirst;
  mbutton->menubutton.defCascadeImage->bitmap_bit_order = LSBFirst;
  mbutton->menubutton.defCascadeImage->bitmap_unit = 8;
}	    

/*************************************<->*************************************
 *
 *   CreatePixmap
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 *   mbutton = menubutton
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void CreatePixmap (mbutton, image, pix, invertedPix)
XwMenuButtonWidget  mbutton;
XImage            * image;
Pixmap            * pix;
Pixmap            * invertedPix;
{
   if (*pix != (Pixmap)NULL)
      XFreePixmap (XtDisplay(mbutton), *pix);

   if (*invertedPix != (Pixmap)NULL)
      XFreePixmap (XtDisplay(mbutton), *invertedPix);

   if (image)
   {
      *pix = XCreatePixmap (XtDisplay(mbutton),
			    RootWindowOfScreen(XtScreen(mbutton)),
			    image->width, image->height,
			    DefaultDepthOfScreen(XtScreen(mbutton)));

      XPutImage (XtDisplay(mbutton), *pix, mbutton->menubutton.defPixmap_GC,
		 image, 0, 0, 0, 0, image->width, image->height);

      if (image->format == XYBitmap)
      {
	 *invertedPix = XCreatePixmap (XtDisplay(mbutton),
				     RootWindowOfScreen(XtScreen(mbutton)),
				     image->width, image->height,
				     DefaultDepthOfScreen(XtScreen(mbutton)));

	 XPutImage (XtDisplay(mbutton), *invertedPix,
		    mbutton->menubutton.invertPixmap_GC,
		    image, 0, 0, 0, 0, image->width, image->height);
      }
	 
   }
   else
   {
      *pix = (Pixmap)NULL;
      *invertedPix = (Pixmap)NULL;
   }
}

/*************************************<->*************************************
 *
 *   IdealWidth
 *
 *   Description:
 *   -----------
 *   NOTE!!! This should be eventually replaced by a QueryProc.
 *
 *   Inputs:
 *   ------
 *   w = menubutton widget
 *
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void IdealWidth (w, width)
Widget w;
Dimension * width;
{
   XwMenuButtonWidget mbutton = (XwMenuButtonWidget) w;

   *width = 2 * mbutton->primitive.highlight_thickness +
           2 * (XwMENUBTNPAD + mbutton->button.internal_width) +
	   XwMARKWIDTH + XwCASCADEWIDTH;

   if (mbutton->menubutton.labelType == XwSTRING)
     *width  += mbutton->button.label_width;
   else if (mbutton->menubutton.labelType == XwIMAGE)
      *width += mbutton->menubutton.labelImage->width;
}

/*************************************<->*************************************
 *
 *  Initialize (request, new)
 *
 *   Description:
 *   -----------
 *     This is the menubutton instance initialize procedure.
 *
 *
 *   Inputs:
 *   ------
 *     request  = original instance record;
 *
 *     new      = instance record with modifications induced by
 *                other initialize routines, changes are made to this
 *                record;
 *
 *     args     = argument list specified in XtCreateWidget;
 *
 *     num_args = argument count;
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void Initialize (request, new)
 Widget request, new;
{
   Dimension	dim;
   KeySym tempKeysym;
   
   XwMenuButtonWidget mbutton = (XwMenuButtonWidget) new;

   /* Augment our translations to include the traversal actions */
   XtAugmentTranslations ((Widget)mbutton,
                     XwprimitiveClassRec.primitive_class.translations);

   /* 
    * Always disable traversal in a menubutton.  Since the traversal
    * state is inherited from the menu manager, we will let it control
    * our traversal state.
    */
   mbutton->primitive.traversal_type = XwHIGHLIGHT_OFF;

   mbutton->menubutton.cascadeEnabled = FALSE;
   mbutton->menubutton.inverted = FALSE;
   mbutton->menubutton.labelPixmap =
      mbutton->menubutton.markPixmap =
	 mbutton->menubutton.cascadePixmap =
	    mbutton->menubutton.invertLabelPixmap =
	       mbutton->menubutton.invertMarkPixmap =
		  mbutton->menubutton.invertCascadePixmap = (Pixmap)NULL;

   GetDefImages(mbutton);
   GetGC(mbutton);

   /*
    * If the menuMgr field has not been set up, check if in a menu system
    * (menu manager as ancestor).
    */
   if (mbutton->menubutton.menuMgr == NULL)
   {
      if ((XtIsSubclass (XtParent (mbutton), XwmenupaneWidgetClass)) &&
	  (XtIsSubclass (XtParent (XtParent (mbutton)), shellWidgetClass)) &&
	  (XtIsSubclass (XtParent (XtParent (XtParent (mbutton))),
			 XwmenumgrWidgetClass)))
      {
	 mbutton->menubutton.menuMgr = 
	    (Widget) XtParent (XtParent (XtParent(mbutton)));
      }
   }
   
   /*
    * We need to malloc space for the strings and copy them to our
    * space.  The toolkit simply copies the pointer to the string.
    */
   if ((mbutton->menubutton.accelerator) &&
       (_XwMapKeyEvent (mbutton->menubutton.accelerator,
			&mbutton->menubutton.accelEventType,
			&tempKeysym,
			&mbutton->menubutton.accelModifiers)))
   {
      mbutton->menubutton.accelDetail = XKeysymToKeycode (XtDisplay(mbutton),
                                        tempKeysym);
      mbutton->menubutton.accelerator =
       strcpy(XtMalloc((unsigned)(XwStrlen(mbutton->menubutton.accelerator)+1)),
		mbutton->menubutton.accelerator);
   }
   else
   {
      if (mbutton->menubutton.accelerator)
	 XtWarning ("MenuButton: Invalid accelerator; disabling feature");
      mbutton->menubutton.accelerator = NULL;
      mbutton->menubutton.accelEventType = 0;
      mbutton->menubutton.accelDetail = 0;
      mbutton->menubutton.accelModifiers = 0;
   }

   if (mbutton->menubutton.hint)
      mbutton->menubutton.hint =
       strcpy(XtMalloc((unsigned)(XwStrlen(mbutton->menubutton.hint)+1)),
		mbutton->menubutton.hint);

   /*
    * malloc space for mnemonic.  Only take 1st character & null
    */
   if ((mbutton->menubutton.mnemonic) &&
       (*(mbutton->menubutton.mnemonic) != '\0'))
   {
      char mne = mbutton->menubutton.mnemonic[0];

      mbutton->menubutton.mnemonic = (String) XtMalloc(2);
      mbutton->menubutton.mnemonic[0] = mne;
      mbutton->menubutton.mnemonic[1] = '\0';
   }
   else
      if (mbutton->menubutton.mnemonic)
	 XtWarning ("MenuButton: Invalid mnemonic; disabling feature");
   
   if (mbutton->menubutton.labelImage)
	 CreatePixmap(mbutton, mbutton->menubutton.labelImage,
		      &mbutton->menubutton.labelPixmap,
		      &mbutton->menubutton.invertLabelPixmap);

   if (!mbutton->menubutton.markImage)
     mbutton->menubutton.markImage = mbutton->menubutton.defMarkImage;

   CreatePixmap(mbutton, mbutton->menubutton.markImage,
		&mbutton->menubutton.markPixmap,
		&mbutton->menubutton.invertMarkPixmap);

   if (!mbutton->menubutton.cascadeImage)
     mbutton->menubutton.cascadeImage = mbutton->menubutton.defCascadeImage;

   CreatePixmap(mbutton, mbutton->menubutton.cascadeImage,
		&mbutton->menubutton.cascadePixmap,
		&mbutton->menubutton.invertCascadePixmap);
   
   if (request->core.height <= 0)
      mbutton->core.height = ComputeHeight(mbutton);

   ComputeVertical(mbutton);
   SetUnderline(mbutton);

   if (request->core.width <= 0)
      IdealWidth(mbutton, &mbutton->core.width);
}

/*************************************<->*************************************
 *
 *   Destroy
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void Destroy (mbutton)
XwMenuButtonWidget mbutton;
{
   if (mbutton->menubutton.accelerator)
      XtFree (mbutton->menubutton.accelerator);

   XtDestroyGC (mbutton->menubutton.defPixmap_GC);
   XtDestroyGC (mbutton->menubutton.inverted_GC);
   XtDestroyGC (mbutton->menubutton.invertPixmap_GC);
   XtDestroyGC (mbutton->menubutton.rect_GC);
   
   mbutton->menubutton.defMarkImage->data = NULL;
   XDestroyImage (mbutton->menubutton.defMarkImage);
   mbutton->menubutton.defCascadeImage->data = NULL;
   XDestroyImage (mbutton->menubutton.defCascadeImage);

   XtRemoveAllCallbacks ((Widget)mbutton, XtNcascadeSelect);
   XtRemoveAllCallbacks ((Widget)mbutton, XtNcascadeUnselect);

   XFreePixmap (XtDisplay(mbutton), mbutton->menubutton.markPixmap);
   XFreePixmap (XtDisplay(mbutton), mbutton->menubutton.cascadePixmap);
   if (mbutton->menubutton.labelPixmap)
      XFreePixmap (XtDisplay(mbutton), mbutton->menubutton.labelPixmap);
}

/*************************************<->*************************************
 *
 *  Realize
 *
 *   Description:
 *   -----------
 *     Creates the window for this menubutton instance.  Sets bit gravity
 *     so that on resize the menubutton is repainted.
 *
 *
 *   Inputs:
 *   ------
 *     w  =  widget to be realized.
 *
 *     valueMask = contains event mask for this window/widget.
 *
 *     attributes = window attributes for this window/widget.
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   XtCreateWindow()
 *************************************<->***********************************/

static void Realize(w, p_valueMask, attributes)
Widget w;
XtValueMask * p_valueMask;   
XSetWindowAttributes * attributes;
{
   XwMenuButtonWidget mbutton = (XwMenuButtonWidget) w;
   
   XtValueMask valueMask = *p_valueMask;
   valueMask |= CWBitGravity;
   attributes->bit_gravity = ForgetGravity;


   XtCreateWindow ((Widget)mbutton, InputOutput, (Visual *) CopyFromParent,
		   valueMask, attributes);

   _XwRegisterName (mbutton);
} /* Realize */

/*************************************<->*************************************
 *
 *  Select (w, event)                  PRIVATE
 *
 *   Description:
 *   -----------
 *     Mark menubutton as selected, (i.e., draw it as active)
 *     Generate the correct callbacks.
 *
 *
 *   Inputs:
 *   ------
 *     w           =   widget instance that was selected.
 *     event       =   event record
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
  *   XtCallCallbacks()
 *************************************<->***********************************/

static void Select(w,event)
Widget w;
XEvent *event;

{
   XwMenuButtonWidget mbutton = (XwMenuButtonWidget) w;

   /*  Don't do anything if its not sensitive.  */

   if (XtIsSensitive((Widget)mbutton))
   {
     /*
      * if there is a menu manager, call the process select routine to
      * determine if the event is valid for the menu system.
      */
     if (mbutton->menubutton.menuMgr)
     {
	if ((*(((XwMenuMgrWidgetClass) XtClass(mbutton->menubutton.menuMgr))->
	       menu_mgr_class.processSelect))
	    (mbutton->menubutton.menuMgr, mbutton, event) == FALSE)
	{
	   return;
	}
     }

     XtCallCallbacks ((Widget)mbutton, XtNselect, NULL);
  }
}

/*************************************<->*************************************
 *
 *   DrawLabelMarkCascade (mbutton)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void DrawLabelMarkCascade (mbutton)
XwMenuButtonWidget  mbutton;
{
   Dimension labelStarts;
   GC theGC;
   Pixmap thePixmap;
   
   if (mbutton->menubutton.inverted)
      theGC = mbutton->button.inverse_GC;
   else
      theGC = mbutton->button.normal_GC;
	 

   if (mbutton->menubutton.noPad == True)
      labelStarts = mbutton->primitive.highlight_thickness +
		 mbutton->button.internal_width;
   else
      labelStarts = mbutton->primitive.highlight_thickness +
                 mbutton->button.internal_width + XwMENUBTNPAD + XwMARKWIDTH;
   /*
    * Draw the label with its underline if needed.
    */
   if (mbutton->menubutton.labelType == XwSTRING)
   {
      XDrawString(
		  XtDisplay(mbutton), XtWindow(mbutton), theGC,
		  labelStarts, mbutton->button.label_y,
		  mbutton->button.label, (int) mbutton->button.label_len);

      if ((!mbutton->menubutton.mgrOverrideMnemonic) &&
          (mbutton->menubutton.mnemonicMatch))
	 XDrawLine(
		XtDisplay(mbutton), XtWindow(mbutton), theGC,
		mbutton->menubutton.underline_x, 
                mbutton->menubutton.underline_y,
		mbutton->menubutton.underline_x + 
                  mbutton->menubutton.underline_width,
		mbutton->menubutton.underline_y);
   }
   else if (mbutton->menubutton.labelType == XwRECT)
   {
      /* Draw a colored or stippled rectangle with a border around it */

      if (mbutton->menubutton.noPad == True) {
         XFillRectangle(
		XtDisplay(mbutton), XtWindow(mbutton),
		mbutton->menubutton.rect_GC,
		2, 2, mbutton->menubutton.labelImage->width - 1,
		mbutton->menubutton.labelImage->height - 1);
         XDrawRectangle(
		XtDisplay(mbutton), XtWindow(mbutton), theGC,
		2, 2, mbutton->menubutton.labelImage->width - 1,
		mbutton->menubutton.labelImage->height - 1);
      }
      else {
         XFillRectangle(
		XtDisplay(mbutton), XtWindow(mbutton),
		mbutton->menubutton.rect_GC,
		XwMARKWIDTH + 3, 3, mbutton->core.width - XwMARKWIDTH - 6,
		mbutton->core.height - 6);
         XDrawRectangle(
		XtDisplay(mbutton), XtWindow(mbutton), theGC,
		XwMARKWIDTH + 3, 3, mbutton->core.width - XwMARKWIDTH - 6,
		mbutton->core.height - 6);
      }
   }
   else 
   {
      if ((mbutton->menubutton.inverted) &&
	  (mbutton->menubutton.invertLabelPixmap))
	 thePixmap = mbutton->menubutton.invertLabelPixmap;
      else
	 thePixmap = mbutton->menubutton.labelPixmap;
      
      XCopyArea (XtDisplay(mbutton), thePixmap, XtWindow(mbutton),
		 mbutton->menubutton.defPixmap_GC, 0, 0,
		 mbutton->menubutton.labelImage->width,
		 mbutton->menubutton.labelImage->height,
		 labelStarts, mbutton->button.label_y);
   }

   /*
    * If the mark is set, display the mark.
    */
   if (mbutton->menubutton.setMark)
   {
      if ((mbutton->menubutton.inverted) &&
	  (mbutton->menubutton.invertMarkPixmap))
	 thePixmap = mbutton->menubutton.invertMarkPixmap;
      else
	 thePixmap = mbutton->menubutton.markPixmap;

      XCopyArea (XtDisplay(mbutton), thePixmap, XtWindow(mbutton),
		 mbutton->menubutton.defPixmap_GC, 0, 0,
		 mbutton->menubutton.markImage->width,
		 mbutton->menubutton.markImage->height,
		 mbutton->button.internal_width +
		         mbutton->primitive.highlight_thickness,
		 mbutton->menubutton.mark_y);
   }

   /*
    * If the cascade is set, display it.
    */
   if (mbutton->menubutton.cascadeOn)
   {
      if ((mbutton->menubutton.inverted) &&
	  (mbutton->menubutton.invertCascadePixmap))
	 thePixmap = mbutton->menubutton.invertCascadePixmap;
      else
	 thePixmap = mbutton->menubutton.cascadePixmap;

      XCopyArea (XtDisplay(mbutton), thePixmap, XtWindow(mbutton),
		 mbutton->menubutton.defPixmap_GC, 0, 0,
		 mbutton->menubutton.cascadeImage->width,
		 mbutton->menubutton.cascadeImage->height,
		 mbutton->core.width - XwCASCADEWIDTH -
		         mbutton->primitive.highlight_thickness -
		         mbutton->button.internal_width,
		 mbutton->menubutton.cascade_y);
   }
}

/*************************************<->*************************************
 *
 *   Inverted (mbutton)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   XFillRectangle
 *   DrawLabelMarkCascade
 *
 *************************************<->***********************************/

static void Inverted (mw)
XwMenuButtonWidget mw;

{
   mw -> menubutton.inverted = TRUE;

   XFillRectangle (XtDisplay (mw), XtWindow (mw), 
                   mw->button.normal_GC,
                   mw -> primitive.highlight_thickness + 1,
                   mw -> primitive.highlight_thickness + 1,
                   mw -> core.width - 2 * 
                      (mw -> primitive.highlight_thickness + 1),
                   mw -> core.height - 2 * 
                      (mw -> primitive.highlight_thickness + 1));
   DrawLabelMarkCascade (mw);
}



/*************************************<->*************************************
 *
 *   NonInverted (mbutton)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   XClearWindow
 *   DrawLabelMarkCascade
 *
 *************************************<->***********************************/

static void NonInverted (mbutton)
XwMenuButtonWidget mbutton;

{
   mbutton->menubutton.inverted = FALSE;

   XClearWindow (XtDisplay(mbutton), XtWindow(mbutton));
   DrawLabelMarkCascade (mbutton);
}



/*************************************<->*************************************
 *
 *   Highlight(mbutton)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   Moved
 *   Inverted
 *
 *************************************<->***********************************/

static void Highlight (mbutton)
XwMenuButtonWidget mbutton;
{
/*
   if (mbutton->primitive.traversal_type == XwHIGHLIGHT_TRAVERSAL)
      _XwHighlightBorder(mbutton);
   else
*/
      Inverted (mbutton);
}

/*************************************<->*************************************
 *
 *   Unhighlight (mbutton)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   Moved
 *   Inverted
 *
 *************************************<->***********************************/

static void Unhighlight (mbutton)
XwMenuButtonWidget mbutton;
{
/*
   if (mbutton->primitive.traversal_type == XwHIGHLIGHT_TRAVERSAL)
      _XwUnhighlightBorder(mbutton);
   else
*/
      NonInverted (mbutton);
   mbutton->menubutton.cascadeEnabled = FALSE;
}

/*************************************<->*************************************
 *
 *  Enter (w, event)                  PRIVATE
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 *     w           =   widget instance that was selected.
 *     event       =   event record
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   Moved
 *   Inverted
 *
 *************************************<->***********************************/

static void Enter(w,event)
Widget w;
XEvent *event;
{
   XwMenuButtonWidget mbutton = (XwMenuButtonWidget)w;

#ifdef DEBUG
   printf ("Enter %s\n", w->core.name);
#endif
   if ((mbutton->menubutton.menuMgr == NULL) ||
       ((*(((XwMenuMgrWidgetClass)
	  XtClass (mbutton->menubutton.menuMgr))->menu_mgr_class.validEvent))
	  (mbutton->menubutton.menuMgr, mbutton, event)))
   {
      /*
       * Check on cascade indicator
       */
      Moved (w, event);

/*
 *    if (mbutton->primitive.traversal_type != XwHIGHLIGHT_TRAVERSAL)
 */
	 Inverted(mbutton);
   }

   /* to-do:  if no hintProc is specified, should generate a hint tag */

   if ((mbutton->menubutton.hint != NULL) &&
	 (mbutton->menubutton.hintProc != NULL)) {
      mbutton->menubutton.hintProc(mbutton->menubutton.hint);
   }
}

/*************************************<->*************************************
 *
 *   EnterParentsWindow (menupane, mbutton,event)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void EnterParentsWindow (menupane, mbutton, event)
Widget              menupane;
XwMenuButtonWidget  mbutton;
XEvent 		  * event;
{
   Boolean remainHighlighted;
   XwunselectParams params;

   XEnterWindowEvent * entEvent = (XEnterWindowEvent *) event;

#ifdef DEBUG
   printf ("EnterParents %s %s ", menupane->core.name, mbutton->core.name);
#endif
  
   /*
    * if outside of the menubutton, bring down submenu.  I am assuming that
    * the x parameters are okay.  This means that entering my parents borders
    * on the correct y parameters will not cause the unselects to be called.
    */
   if ((entEvent->y < mbutton->core.y) || 
       (entEvent->y > mbutton->core.y + mbutton->core.height + 
                       2 * mbutton->core.border_width))
   {
      params.rootX = entEvent->x_root; 
      params.rootY = entEvent->y_root;
      params.remainHighlighted = FALSE;

#ifdef DEBUG
      printf ("rootX %d rootY %d\n", params.rootX, params.rootY);
      printf ("disabled\n");
#endif

      XtCallCallbacks ((Widget)mbutton, XtNcascadeUnselect, &params);
      mbutton->menubutton.cascadeEnabled = params.remainHighlighted;
      if (mbutton->menubutton.cascadeEnabled == FALSE)
         Unhighlight (mbutton);
   }
/*   else
 *    Moved (mbutton, event);
 */

#ifdef DEBUG
   printf ("\n");
#endif

   XtRemoveEventHandler (menupane, EnterWindowMask, FALSE,
			 (XtEventHandler)EnterParentsWindow, mbutton);
}

/*************************************<->*************************************
 *
 *  Leave (w, event)                  PRIVATE
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   XtCallCallbacks
 *   NonInverted
 *
 *************************************<->***********************************/

static void Leave(w,event)
Widget w;
XEvent *event;

{
   XwunselectParams  params;
   XwMenuButtonWidget mbutton = (XwMenuButtonWidget)w;
   XLeaveWindowEvent * lEvent = (XLeaveWindowEvent *) event;

   params.rootX = lEvent->x_root;
   params.rootY = lEvent->y_root;


   if ((mbutton->menubutton.menuMgr == NULL) ||
       ((*(((XwMenuMgrWidgetClass)
	    XtClass (mbutton->menubutton.menuMgr))->menu_mgr_class.validEvent))
	    (mbutton->menubutton.menuMgr, mbutton, event)))
   {
      if (mbutton->menubutton.cascadeEnabled)
      {
         params.remainHighlighted = FALSE;
         XtCallCallbacks ((Widget)mbutton, XtNcascadeUnselect, &params);
         mbutton->menubutton.cascadeEnabled = params.remainHighlighted;

         if (mbutton->menubutton.cascadeEnabled)
         {
            XtAddEventHandler (XtParent(mbutton), EnterWindowMask, FALSE,
      			       (XtEventHandler)EnterParentsWindow, mbutton);
            return;
         }
      }

      if (mbutton->menubutton.inverted)
         NonInverted(mbutton);
   }

   if ((mbutton->menubutton.hint != NULL) &&
	 (mbutton->menubutton.hintProc != NULL)) {
      mbutton->menubutton.hintProc("");
   }
}

/*************************************<->*************************************
 *
 *   Moved
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   XQueryPointer
 *   XtCallCallbacks
 *
 *************************************<->***********************************/

static void Moved (w,event)
Widget   w;
XEvent * event;

{
   XwMenuButtonWidget mbutton = (XwMenuButtonWidget)w;

   int xPosition, yPosition;
   int xroot, yroot;
   Window root, child;
   unsigned int mask;
   XwunselectParams params;

   XButtonPressedEvent * buttonEvent = (XButtonPressedEvent *) event;

#ifdef DEBUG
   printf ("Moved %s\n", w->core.name);
#endif

   /*
    * only do this if I have a cascade showing
    */
   if ((mbutton->menubutton.cascadeOn) && 
       (mbutton->primitive.traversal_type == XwHIGHLIGHT_OFF))
   {
      /*
       * if there is a menu manager and the cascade has not been popped up,
       * then ask the menu manager if it should be popped up
       */
      if ((mbutton->menubutton.menuMgr == NULL) ||
          (mbutton->menubutton.cascadeEnabled == TRUE) ||
          (*(((XwMenuMgrWidgetClass)
            XtClass(mbutton->menubutton.menuMgr))->menu_mgr_class.doICascade))
	       (mbutton->menubutton.menuMgr, mbutton))
      {
         /*
          * check if the event appears to have occurred in the cascade area
          */
	 if ((mbutton->menubutton.cascadeEnabled) ||
	     ((buttonEvent->y > 0) &&
	      (buttonEvent->y < mbutton->core.height +  
                                2 * mbutton->core.border_width) &&
	      (buttonEvent->x < mbutton->core.width +
                                2 * mbutton->core.border_width) &&
	      (buttonEvent->x > mbutton->core.width +
                                2 * mbutton->core.border_width - 
                                XwCASCADEWIDTH -
	                        mbutton->primitive.highlight_thickness -
                                mbutton->button.internal_width)))
	 {
	    /*
	     * Verify that its really in the cascade area
	     */
	    XQueryPointer (XtDisplay(mbutton), mbutton->core.window,
			   &root, &child, &xroot, &yroot, &xPosition,
			   &yPosition, &mask);
   
	    if ((yPosition > 0) &&
		(yPosition < mbutton->core.height +
                             2 * mbutton->core.border_width) &&
		(xPosition < mbutton->core.width +
                             2 * mbutton->core.border_width) &&
		(xPosition > mbutton->core.width +
                             2 * mbutton->core.border_width - XwCASCADEWIDTH -
		             mbutton->primitive.highlight_thickness -
                             mbutton->button.internal_width))
	    {
	       if (!mbutton->menubutton.cascadeEnabled)
	       {
		  XtCallCallbacks ((Widget)mbutton, XtNcascadeSelect, NULL);
#ifdef DEBUG
                  printf ("enabled\n");
#endif
		  mbutton->menubutton.cascadeEnabled = TRUE;
	       }
	    }
	    else if (mbutton->menubutton.cascadeEnabled)
	    {	
               params.rootX = xroot;
 	       params.rootY = yroot;
	       params.remainHighlighted = FALSE;
#ifdef DEBUG
	       printf ("Moved rootX %d rootY %d\n", params.rootX, params.rootY);
#endif
	       XtCallCallbacks ((Widget)mbutton, XtNcascadeUnselect, &params);
	       mbutton->menubutton.cascadeEnabled = params.remainHighlighted;
	    }
	 }
      }
   }
}

/*************************************<->*************************************
 *
 *  Redisplay (w, event)
 *
 *   Description:
 *   -----------
 *     Cause the widget, identified by w, to be redisplayed.
 *
 *   Inputs:
 *   ------
 *     w = widget to be redisplayed;
 *     event = event structure identifying need for redisplay on this
 *             widget.
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   Inverted
 *   NonInverted
 *   _XwHighlightBorder
 *   _XwUnhighlightBorder
 *
 *************************************<->***********************************/

static void Redisplay(w, event)
    Widget w;
    XEvent *event;
{
   XwMenuButtonWidget mbutton = (XwMenuButtonWidget) w;
   
   /*
    * if the highlight state has changed since the last redisplay,
    * update the window and set the font GC.
    */
   if (mbutton->menubutton.inverted)
      Inverted(mbutton);

   else
      NonInverted(mbutton);

   if (mbutton->primitive.highlighted)
      _XwHighlightBorder(mbutton);

   else
      if (mbutton->primitive.display_highlighted)
	 _XwUnhighlightBorder(mbutton);
}

/*************************************<->*************************************
 *
 *  SetValues(urrent, request, new)
 *
 *   Description:
 *   -----------
 *     This is the set values procedure for the menubutton class.  It is
 *     called last (the set values rtnes for its superclasses are called
 *     first).
 *
 *
 *   Inputs:
 *   ------
 *    current = original widget;
 *    request = copy of current (?);
 *    new = copy of request which reflects changes made to it by
 *          set values procedures of its superclasses;
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Boolean SetValues (current, request, new)
Widget current, request, new;
{
    XtWidgetGeometry	reqGeo;
    XtWidgetGeometry	replyGeo;
    XwMenuButtonWidget 	curmbutton = (XwMenuButtonWidget) current;
    XwMenuButtonWidget 	newmbutton  = (XwMenuButtonWidget) new;
    Boolean  		flag = FALSE;    /* our return value */
    Dimension	        dim; 
    KeySym              tempKeysym;

    /* We never allow our traversal state to change using SetValues() */
    newmbutton->primitive.traversal_type = curmbutton->primitive.traversal_type;
    
    /*
     * If the accelerator string changed, malloc space for the string
     * and copy it to our space.  The old string must be freed.
     */

    if (curmbutton->menubutton.accelerator !=
	newmbutton->menubutton.accelerator)
    {
       if (newmbutton->menubutton.accelerator)
       {
	  if (_XwMapKeyEvent (newmbutton->menubutton.accelerator,
			      &newmbutton->menubutton.accelEventType,
			      &tempKeysym,
			      &newmbutton->menubutton.accelModifiers)
	      == FALSE)
	  {
	     /* Invalid string; revert to previous one */
	     XtWarning 
		("MenuButton: Invalid accelerator; using previous setting");
	     newmbutton->menubutton.accelerator =
		curmbutton->menubutton.accelerator;
	     newmbutton->menubutton.accelEventType =
		curmbutton->menubutton.accelEventType;
	     newmbutton->menubutton.accelDetail =
		curmbutton->menubutton.accelDetail;
	     newmbutton->menubutton.accelModifiers =
		curmbutton->menubutton.accelModifiers;
	  }
	  else
	  {
	     /* valid string */
             newmbutton->menubutton.accelDetail = XKeysymToKeycode (
                    XtDisplay(newmbutton), tempKeysym);
	     newmbutton->menubutton.accelerator =
		strcpy(XtMalloc((unsigned)
			   (XwStrlen(newmbutton->menubutton.accelerator)+1)),
		       newmbutton->menubutton.accelerator);

	     if (newmbutton->menubutton.menuMgr)
		(*(((XwMenuMgrWidgetClass)
		    XtClass (newmbutton->menubutton.menuMgr))->
		       menu_mgr_class.setSelectAccelerator))
		   (newmbutton->menubutton.menuMgr, (Widget)newmbutton,
		    newmbutton->menubutton.accelerator,
		    newmbutton->menubutton.accelEventType,
		    newmbutton->menubutton.accelDetail,
		    newmbutton->menubutton.accelModifiers);

	     if (curmbutton->menubutton.accelerator)
		XtFree ((char *) curmbutton->menubutton.accelerator);
	  }
       }
       else if (curmbutton->menubutton.accelerator)
       {
	  if (curmbutton->menubutton.menuMgr)
	     (*(((XwMenuMgrWidgetClass)
		 XtClass(curmbutton->menubutton.menuMgr))->
		          menu_mgr_class.clearSelectAccelerator))
		    (curmbutton->menubutton.menuMgr, (Widget)curmbutton);

	  XtFree ((char *) curmbutton->menubutton.accelerator);
       }
    }

    /*
     * Determine if the mnemonic changed, verify and malloc space. 
     * Notify menuMgr of the change.
     */

    if (curmbutton->menubutton.mnemonic != newmbutton->menubutton.mnemonic)
    {
      if (newmbutton->menubutton.mnemonic)
      {
         if (*(newmbutton->menubutton.mnemonic) == '\0')
         {
            XtWarning 
              ("MenuButton: Invalid mnemonic; using previous setting");
            newmbutton->menubutton.mnemonic = curmbutton->menubutton.mnemonic;
         }
         else
         {
            char mne = newmbutton->menubutton.mnemonic[0];

            newmbutton->menubutton.mnemonic = (String)XtMalloc(2);
            newmbutton->menubutton.mnemonic[0] = mne;
            newmbutton->menubutton.mnemonic[1] = '\0';

 	    if (newmbutton->menubutton.menuMgr)
	       (*(((XwMenuMgrWidgetClass)
		    XtClass (newmbutton->menubutton.menuMgr))->
		       menu_mgr_class.setSelectMnemonic))
		   (newmbutton->menubutton.menuMgr, (Widget)newmbutton, 
                    newmbutton->menubutton.mnemonic);

            XtFree (curmbutton->menubutton.mnemonic);
         }
      }
      else
      {
	 if (newmbutton->menubutton.menuMgr)
	    (*(((XwMenuMgrWidgetClass)
		 XtClass (curmbutton->menubutton.menuMgr))->
		    menu_mgr_class.clearSelectMnemonic))
		(curmbutton->menubutton.menuMgr, (Widget)curmbutton);

         XtFree(curmbutton->menubutton.mnemonic);
      }
    }

    /*
     * recalculate the underline parameters if mnemonic or font changes
     */
    if ((newmbutton->menubutton.mnemonic != curmbutton->menubutton.mnemonic) ||
	(newmbutton->button.font != curmbutton->button.font))
    {
       SetUnderline (newmbutton);
       flag = TRUE;
    }
		    

    /*
     * If the foreground or background changed, or the color or stipple
     * declaration was changed, recreate the GC's
     */
    if ((newmbutton->primitive.foreground !=
            curmbutton->primitive.foreground) ||
        (newmbutton->core.background_pixel !=
            curmbutton->core.background_pixel) ||
	(newmbutton->menubutton.rectColor !=
	    curmbutton->menubutton.rectColor) ||
	(newmbutton->menubutton.rectStipple !=
	    curmbutton->menubutton.rectStipple))
    {
       GetGC (newmbutton);
       XtDestroyGC (curmbutton->menubutton.defPixmap_GC);
       XtDestroyGC (curmbutton->menubutton.invertPixmap_GC);
    }

    /*
     * If the GCs are new, or the images are new, create new pixmaps
     */
    if ((newmbutton->menubutton.markImage != 
            curmbutton->menubutton.markImage) ||
        (newmbutton->primitive.foreground !=
            curmbutton->primitive.foreground) ||
        (newmbutton->core.background_pixel !=
            curmbutton->core.background_pixel))
    {
       if (!newmbutton->menubutton.markImage)
	  newmbutton->menubutton.markImage =
	     newmbutton->menubutton.defMarkImage;

       CreatePixmap(newmbutton, newmbutton->menubutton.markImage,
		    &newmbutton->menubutton.markPixmap,
		    &newmbutton->menubutton.invertMarkPixmap);
       flag = TRUE;
    }

    if ((newmbutton->menubutton.cascadeImage !=
	    curmbutton->menubutton.cascadeImage) ||
        (newmbutton->primitive.foreground !=
            curmbutton->primitive.foreground) ||
        (newmbutton->core.background_pixel !=
            curmbutton->core.background_pixel))
    {
       if (!newmbutton->menubutton.cascadeImage)
	  newmbutton->menubutton.cascadeImage =
	     newmbutton->menubutton.defCascadeImage;

       CreatePixmap(newmbutton, newmbutton->menubutton.cascadeImage,
		    &newmbutton->menubutton.cascadePixmap,
		    &newmbutton->menubutton.invertCascadePixmap);
       flag = TRUE;
    }

    if ((newmbutton->menubutton.labelImage !=
	    curmbutton->menubutton.labelImage) ||
        (newmbutton->primitive.foreground !=
            curmbutton->primitive.foreground) ||
        (newmbutton->core.background_pixel !=
            curmbutton->core.background_pixel))
    {
       CreatePixmap(newmbutton, newmbutton->menubutton.labelImage,
		    &newmbutton->menubutton.labelPixmap,
		    &newmbutton->menubutton.invertLabelPixmap);

       if (newmbutton->menubutton.labelType == XwIMAGE)
	  flag = TRUE;
    }

    if ((newmbutton->core.sensitive != curmbutton->core.sensitive) ||
       (newmbutton->core.ancestor_sensitive != 
                        curmbutton->core.ancestor_sensitive))
    {
	if (curmbutton->menubutton.menuMgr)
        {
	   (*(((XwMenuMgrWidgetClass)
	     XtClass(curmbutton->menubutton.menuMgr))->
	             menu_mgr_class.btnSensitivityChanged))
	      (curmbutton->menubutton.menuMgr, (Widget)newmbutton);
        }
    }

    /*
     * fields that change that cause a redraw
     */
    if 	((newmbutton->menubutton.labelType !=
	     curmbutton->menubutton.labelType) ||
	 (newmbutton->menubutton.setMark !=
	     curmbutton->menubutton.setMark) ||
	 (newmbutton->menubutton.cascadeOn !=
	     curmbutton->menubutton.cascadeOn) ||
	 (newmbutton->menubutton.mgrOverrideMnemonic !=
	     curmbutton->menubutton.mgrOverrideMnemonic) ||
         (newmbutton->menubutton.rectColor !=
             curmbutton->menubutton.rectColor) ||
         (newmbutton->menubutton.rectStipple != 
             curmbutton->menubutton.rectStipple)) 


       flag = TRUE;

	
    /**********************************************************************
     * Calculate the window size:  The assumption here is that if
     * the width and height are the same in the new and current instance
     * record that those fields were not changed with set values.  Therefore
     * its okay to recompute the necessary width and height.  However, if
     * the new and current do have different width/heights then leave them
     * alone because that's what the user wants.
     *********************************************************************/

    /* "noPad" option prevents button from resizing itself  2/24/00 --Tim */

    if (curmbutton->core.width == request->core.width &&
	  curmbutton->menubutton.noPad == False)
    {
       IdealWidth (newmbutton, &newmbutton->core.width);
       flag = TRUE;
    }
    else if (request->core.width <= 0)
    {
       XtWarning ("MenuButton:  Invalid width; using previous setting");
       newmbutton->core.width = curmbutton->core.width;
    }

    if (curmbutton->core.height == request->core.height &&
	  curmbutton->menubutton.noPad == False)
    {
       newmbutton->core.height = ComputeHeight(newmbutton);
       flag = TRUE;
    }
    else if (request->core.height <= 0)
    {
       XtWarning ("MenuButton:  Invalid height; using previous setting");
       newmbutton->core.height = curmbutton->core.height;
    }

    return (flag);
}

/*************************************<->*************************************
 *
 *  Resize(w)
 *
 *   Description:
 *   -----------
 *     A resize event has been generated. Recompute location of button
 *     elements.
 *
 *   Inputs:
 *   ------
 *     w  = widget to be resized.
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void Resize(w)
   Widget w;
{

   XwMenuButtonWidget mbutton = (XwMenuButtonWidget) w;

   ComputeVertical(mbutton);
   SetUnderline(mbutton);
}


/*************************************<->*************************************
 *
 *  SetTraversalType(w)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void SetTraversalType (w, highlight_mode)

   Widget w;
   int highlight_mode;

{

   XwMenuButtonWidget mbutton = (XwMenuButtonWidget) w;

   mbutton->primitive.traversal_type = highlight_mode;

   if (highlight_mode == XwHIGHLIGHT_TRAVERSAL)
   {
      XtAugmentTranslations (w, XwprimitiveClassRec.primitive_class.
                                translations);
      w->core.widget_class->core_class.visible_interest = True;
   }
}


/*************************************<->*************************************
 *
 *  TraverseRight(w, event)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void TraverseRight (w, event)

   XwMenuButtonWidget w;
   XEvent * event;

{
   /*
    * Ask the menu manager to traverse to the next menupane, if we
    * have a cascade.
    */

   if ((w->menubutton.cascadeOn) && (w->menubutton.menuMgr) &&
       (w->primitive.I_have_traversal) && (_XwUniqueEvent (event)))
   {
      (*(((XwMenuMgrWidgetClass)
        XtClass(w->menubutton.menuMgr))->menu_mgr_class.traverseRight))
        (w->menubutton.menuMgr, event);
   }
}


/*************************************<->*************************************
 *
 *  TraverseLeft(w, event)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void TraverseLeft (w, event)

   XwMenuButtonWidget w;
   XEvent * event;

{
   /*
    * Ask the menu manager to traverse to the previous menupane.
    */

   if ((w->menubutton.menuMgr) && (_XwUniqueEvent (event)) &&
       (w->primitive.I_have_traversal))
   {
      (*(((XwMenuMgrWidgetClass)
        XtClass(w->menubutton.menuMgr))->menu_mgr_class.traverseLeft))
        (w->menubutton.menuMgr, event);
   }
}


/*************************************<->*************************************
 *
 *  TraverseNext(w, event)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void TraverseNext (w, event)

   XwMenuButtonWidget w;
   XEvent * event;

{
   /*
    * Ask the menu manager to traverse to the previous menupane.
    */

   if ((w->menubutton.menuMgr) && (_XwUniqueEvent (event)) &&
       (w->primitive.I_have_traversal))
   {
      (*(((XwMenuMgrWidgetClass)
        XtClass(w->menubutton.menuMgr))->menu_mgr_class.traverseNext))
        (w->menubutton.menuMgr, event);
   }
}


/*************************************<->*************************************
 *
 *  TraversePrev(w, event)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void TraversePrev (w, event)

   XwMenuButtonWidget w;
   XEvent * event;

{
   /*
    * Ask the menu manager to traverse to the previous menupane.
    */

   if ((w->menubutton.menuMgr) && (_XwUniqueEvent (event)) &&
       (w->primitive.I_have_traversal))
   {
      (*(((XwMenuMgrWidgetClass)
        XtClass(w->menubutton.menuMgr))->menu_mgr_class.traversePrev))
        (w->menubutton.menuMgr, event);
   }
}


/*************************************<->*************************************
 *
 *  TraverseHome(w)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void TraverseHome (w, event)

   XwMenuButtonWidget w;
   XEvent * event;

{
   /*
    * Ask the menu manager to traverse to the first menupane.
    */

   if ((w->menubutton.menuMgr) && (_XwUniqueEvent (event)) &&
       (w->primitive.I_have_traversal))
   {
      (*(((XwMenuMgrWidgetClass)
        XtClass(w->menubutton.menuMgr))->menu_mgr_class.traverseHome))
        (w->menubutton.menuMgr, event);
   }
}


/*************************************<->*************************************
 *
 *  TraverseUp(w)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void TraverseUp (w, event)

   XwMenuButtonWidget w;
   XEvent * event;

{
   /*
    * Ask the menu manager to traverse up one menu button
    */

   if ((w->menubutton.menuMgr) && (_XwUniqueEvent (event)) &&
       (w->primitive.I_have_traversal))
   {
      (*(((XwMenuMgrWidgetClass)
        XtClass(w->menubutton.menuMgr))->menu_mgr_class.traverseUp))
        (w->menubutton.menuMgr, event);
   }
}


/*************************************<->*************************************
 *
 *  TraverseDown(w)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void TraverseDown (w, event)

   XwMenuButtonWidget w;
   XEvent * event;

{
   /*
    * Ask the menu manager to traverse down one menu button
    */

   if ((w->menubutton.menuMgr) && (_XwUniqueEvent (event)) &&
       (w->primitive.I_have_traversal))
   {
      (*(((XwMenuMgrWidgetClass)
        XtClass(w->menubutton.menuMgr))->menu_mgr_class.traverseDown))
        (w->menubutton.menuMgr, event);
   }
}


/*************************************<->*************************************
 *
 *  TraverseNextTop(w)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void TraverseNextTop (w, event)

   XwMenuButtonWidget w;
   XEvent * event;

{
   /*
    * Ask the menu manager to traverse to the next top level menupane
    */

   if ((w->menubutton.menuMgr) && (_XwUniqueEvent (event)) &&
       (w->primitive.I_have_traversal))
   {
      (*(((XwMenuMgrWidgetClass)
        XtClass(w->menubutton.menuMgr))->menu_mgr_class.traverseNextTop))
        (w->menubutton.menuMgr, event);
   }
}


/************************************************************************
 *
 *  _XwExtractTime
 *     Extract the time field from the event structure.
 *
 ************************************************************************/

static Time _XwExtractTime (event)

   XEvent * event;

{
   if ((event->type == ButtonPress) || (event->type == ButtonRelease))
      return (event->xbutton.time);

   if ((event->type == KeyPress) || (event->type == KeyRelease))
      return (event->xkey.time);

   return ((Time) 0);
}


Boolean _XwUniqueEvent (event)

   XEvent * event;

{
   static unsigned long serial = 0;
   static Time time = 0;
   static int type = 0;
   Time newTime;

   /*
    * Ignore duplicate events, caused by an event being dispatched
    * to both the focus widget and the spring-loaded widget, where
    * these map to the same widget (menus).
    */
   if ((time != (newTime = _XwExtractTime (event))) ||
       (type != event->type) ||
       (serial != event->xany.serial))
   {
      /* Save the fingerprints for the new event */
      type = event->type;
      serial = event->xany.serial;
      time = newTime;

      return (TRUE);
   }

   return (FALSE);
}


/*************************************<->*************************************
 *
 *  Visibility(parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void Visibility (widget, event)

   Widget widget;
   XEvent * event;

{
   /*
    * Noop; purpose is to prevent Primitive's translation from
    * taking effect.
    */
}


/*************************************<->*************************************
 *
 *  Unmap(parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void Unmap (widget, event)

   Widget widget;
   XEvent * event;

{
   /*
    * Noop; purpose is to prevent Primitive's translation from
    * taking effect.
    */
}
