/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        WorkSpace.c
 **
 **   Project:     X Widgets
 **
 **   Description: Contains code for workSpace widget class.
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


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <X11/StringDefs.h>
#include <X11/keysymdef.h>   
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <Xw/WorkSpace.h>
#include <Xw/WorkSpaceP.h>


/*  Event procedures referenced in the actions list  */

static void KeyDown();
static void KeyUp();
static void Select();
static void Release();


/*  Default translation table and action list  */

static char defaultTranslations[] =
    "<KeyDown>:         keydown()\n\
     <KeyUp>:           keyup()\n\
     <BtnDown>:         select()\n\
     <BtnUp>:           release()\n\
     <EnterWindow>:     enter()\n\
     <LeaveWindow>:     leave()";


static XtActionsRec actionsList[] =
{
  { "keydown",  (XtActionProc) KeyDown              },
  { "keyup",    (XtActionProc) KeyUp                },
  { "select",   (XtActionProc) Select               },
  { "release",  (XtActionProc) Release              },
  { "enter",    (XtActionProc) _XwPrimitiveEnter    },
  { "leave",    (XtActionProc) _XwPrimitiveLeave    },
 };


/*  Resource list for WorkSpace  */

static XtResource resources[] = 
{
   {
      XtNexpose, XtCCallback, XtRCallback, sizeof (caddr_t),
      XtOffset (XwWorkSpaceWidget, workSpace.expose),
      XtRPointer, (caddr_t) NULL
   },

   {
      XtNresize, XtCCallback, XtRCallback, sizeof (caddr_t),
      XtOffset (XwWorkSpaceWidget, workSpace.resize),
      XtRPointer, (caddr_t) NULL
   },

   {
      XtNkeyDown, XtCCallback, XtRCallback, sizeof (caddr_t),
      XtOffset (XwWorkSpaceWidget, workSpace.key_down),
      XtRPointer, (caddr_t) NULL
   },

   {
      XtNkeyUp, XtCCallback, XtRCallback, sizeof (caddr_t),
      XtOffset (XwWorkSpaceWidget, workSpace.key_up),
      XtRPointer, (caddr_t) NULL
   }
};


/*  Static routine definitions  */

static void      Initialize();
static void    Redisplay();
static void    Resize();
static void    Destroy();
static Boolean SetValues();


/*  The WorkSpace class record definition  */

XwWorkSpaceClassRec XwworkSpaceClassRec =
{
   {
      (WidgetClass) &XwprimitiveClassRec, /* superclass	         */	
      "XwWorkSpace",                    /* class_name	         */	
      sizeof(XwWorkSpaceRec),           /* widget_size           */	
      NULL,                             /* class_initialize      */    
      NULL,                             /* class_part_initialize */
      FALSE,                            /* class_inited          */	
      (XtInitProc) Initialize,          /* initialize	         */	
      NULL,                             /* initialize_hook       */
      _XwRealize,                       /* realize	         */	
      actionsList,                      /* actions               */	
      XtNumber(actionsList),            /* num_actions    	 */	
      resources,                        /* resources	         */	
      XtNumber(resources),              /* num_resources         */	
      NULLQUARK,                        /* xrm_class	         */	
      TRUE,                             /* compress_motion       */	
      TRUE,                             /* compress_exposure     */	
      TRUE,                             /* compress_enterleave   */
      FALSE,                            /* visible_interest      */	
      (XtWidgetProc) Destroy,           /* destroy               */	
      (XtWidgetProc) Resize,            /* resize                */	
      (XtExposeProc) Redisplay,         /* expose                */	
      (XtSetValuesFunc) SetValues,      /* set_values	         */	
      NULL,                             /* set_values_hook       */
      XtInheritSetValuesAlmost,         /* set_values_almost     */
      NULL,                             /* get_values_hook       */
      NULL,                             /* accept_focus	         */	
      XtVersion,                        /* version               */
      NULL,                             /* callback private      */
      defaultTranslations,              /* tm_table              */
      NULL,                             /* query_geometry        */
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
   },

   {
      NULL,         /* Primitive border_highlight   */
      NULL,         /* Primitive border_unhighlight */
      NULL,         /* Primitive select_proc        */
      NULL,         /* Primitive release_proc       */
      NULL,         /* Primitive toggle_proc        */
   }
};

WidgetClass XwworkSpaceWidgetClass = (WidgetClass) &XwworkSpaceClassRec;




/************************************************************************
 *
 *  Initialize
 *     The main widget instance initialization routine.
 *
 ************************************************************************/

static void Initialize (request, new)
XwWorkSpaceWidget request, new;

{
   if (request -> core.width == 0)
      new -> core.width += 20;
   if (request -> core.height == 0)
      new -> core.height += 20;
}




/************************************************************************
 *
 *  Redisplay
 *	Draw the traversal/enter border, and
 *	invoke the application exposure callbacks.
 *
 ************************************************************************/

static void Redisplay (hw, event, region)
XwWorkSpaceWidget hw;
XEvent * event;
Region region;

{
   if (hw -> primitive.highlighted)
      _XwHighlightBorder (hw);
   else if (hw -> primitive.display_highlighted)
      _XwUnhighlightBorder (hw);

   XtCallCallbacks ((Widget)hw, XtNexpose, region);
}




/************************************************************************
 *
 *  Resize
 *     Invoke the application resize callbacks.
 *
 ************************************************************************/

static void Resize (hw)
XwWorkSpaceWidget hw;

{
   XtCallCallbacks ((Widget)hw, XtNresize, NULL);
}




/************************************************************************
 *
 *  Destroy
 *	Remove the callback lists.
 *
 ************************************************************************/

static void Destroy (hw)
XwWorkSpaceWidget hw;

{
   XtRemoveAllCallbacks ((Widget)hw, XtNexpose);
   XtRemoveAllCallbacks ((Widget)hw, XtNresize);
   XtRemoveAllCallbacks ((Widget)hw, XtNkeyDown);
}




/************************************************************************
 *
 *  SetValues
 *
 ************************************************************************/

static Boolean SetValues (current, request, new)
XwWorkSpaceWidget current, request, new;

{
   return (False);
}




/************************************************************************
 *
 *  KeyDown
 *     This function processes key presses occuring on the workSpace.
 *
 ************************************************************************/

static void KeyDown (hw, event)
XwWorkSpaceWidget hw;
XButtonPressedEvent * event;

{
   XtCallCallbacks ((Widget)hw, XtNkeyDown, event);
}


/************************************************************************
 *
 *  KeyUp
 *     This function processes key releases occuring on the workSpace.
 *
 ************************************************************************/

static void KeyUp (hw, event)
XwWorkSpaceWidget hw;
XButtonPressedEvent * event;

{
   XtCallCallbacks ((Widget)hw, XtNkeyUp, event);
}




/************************************************************************
 *
 *  Select
 *     This function processes selections occuring on the workSpace.
 *
 ************************************************************************/

static void Select (hw, event)
XwWorkSpaceWidget hw;
XButtonPressedEvent * event;

{
   XtCallCallbacks ((Widget)hw, XtNselect, event);
}




/************************************************************************
 *
 *  Release
 *     This function processes releases occuring on the workSpace.
 *
 ************************************************************************/

static void Release (hw, event)
XwWorkSpaceWidget   hw;
XEvent * event;

{
   XtCallCallbacks ((Widget)hw, XtNrelease, event);
}
