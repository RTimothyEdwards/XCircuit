/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        Primitive.c
 **
 **   Project:     X Widgets
 **
 **   Description: This file contains source for the Primitive Meta Widget.
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
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <Xw/MenuBtn.h>
#include <X11/StringDefs.h>



/*************************************<->*************************************
 *
 *
 *   Description:  default translation table for class: Primitive.
 *   -----------
 *
 *   These translations will be compiled at class initialize.  When
 *   a subclass of primitive is created then these translations will
 *   be used to augment the translations of the subclass IFF
 *   traversal is on.  The SetValues routine will also augment
 *   a subclass's translations table IFF traversal goes from off to on.
 *   Since we are augmenting it should not be a problem when
 *   traversal goes from off to on to off and on again.
 *
 *************************************<->***********************************/

static char defaultTranslations[] =
   "<FocusIn>:              focusIn() \n\
    <FocusOut>:             focusOut() \n\
    <Visible>:         visibility() \n\
    <Unmap>:           unmap()";


/*************************************<->*************************************
 *
 *
 *   Description:  action list for class: Primtive
 *   -----------
 *   Used to provide actions for subclasses with traversal on.
 *
 *************************************<->***********************************/

static XtActionsRec actionsList[] =
{
  {"focusIn", (XtActionProc) _XwPrimitiveFocusIn},
  {"focusOut", (XtActionProc) _XwPrimitiveFocusOut},
  {"visibility", (XtActionProc) _XwPrimitiveVisibility},
  {"unmap", (XtActionProc) _XwPrimitiveUnmap},
};


/*  Resource definitions for Subclasses of Primitive */

static Cursor defaultCursor = None;

static XtResource resources[] =
{
   {
     XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
     XtOffset (XwPrimitiveWidget, primitive.foreground),
     XtRString, "XtDefaultForeground"
   },

   {
     XtNbackgroundTile, XtCBackgroundTile, XtRTileType, sizeof (int),
     XtOffset (XwPrimitiveWidget, primitive.background_tile),
     XtRString, "background"
   },

   {
     XtNcursor, XtCCursor, XtRCursor, sizeof (Cursor),
     XtOffset (XwPrimitiveWidget, primitive.cursor),
     XtRCursor, (caddr_t) &defaultCursor
   },

   {
     XtNtraversalType, XtCTraversalType, XtRTraversalType, sizeof (int),
     XtOffset (XwPrimitiveWidget, primitive.traversal_type),
     XtRString, "highlight_off"
   },

   {
     XtNhighlightThickness, XtCHighlightThickness, XtRInt, sizeof (int),
     XtOffset (XwPrimitiveWidget, primitive.highlight_thickness),
     XtRString, "0"
   },

   {
     XtNhighlightStyle, XtCHighlightStyle, XtRHighlightStyle, sizeof (int),
     XtOffset (XwPrimitiveWidget, primitive.highlight_style),
     XtRString, "pattern_border"
   },

   {
     XtNhighlightColor, XtCForeground, XtRPixel, sizeof (Pixel),
     XtOffset (XwPrimitiveWidget, primitive.highlight_color),
     XtRString, "Black"
   },

   {
     XtNhighlightTile, XtCHighlightTile, XtRTileType, sizeof (int),
     XtOffset (XwPrimitiveWidget, primitive.highlight_tile),
     XtRString, "foreground"
   },

   {
     XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
     XtOffset (XwPrimitiveWidget, primitive.recompute_size),
     XtRString, "True"
   },

   {
     XtNselect, XtCCallback, XtRCallback, sizeof(caddr_t),
     XtOffset (XwPrimitiveWidget, primitive.select),
     XtRPointer, (caddr_t) NULL
   },

   {
     XtNrelease, XtCCallback, XtRCallback, sizeof(caddr_t),
     XtOffset (XwPrimitiveWidget, primitive.release),
     XtRPointer, (caddr_t) NULL
   },
};



/*  Routine definiton used for initializing the class record.  */

static void              ClassInitialize();
static void   ClassPartInitialize();
static void          Initialize();
static void        Realize();
static void        Destroy();
static Boolean     SetValues();

static void GetHighlightGC();



/*  The primitive class record definition  */

XwPrimitiveClassRec XwprimitiveClassRec =
{
   {
      (WidgetClass) &widgetClassRec,    /* superclass	         */	
      "XwPrimitive",                    /* class_name	         */	
      sizeof(XwPrimitiveRec),           /* widget_size	         */	
      (XtProc) ClassInitialize,         /* class_initialize      */    
      (XtWidgetClassProc) ClassPartInitialize, /* class_part_initialize */
      FALSE,                            /* class_inited          */	
      (XtInitProc) Initialize,          /* initialize	         */	
      NULL,                             /* initialize_hook       */
      (XtRealizeProc) Realize,          /* realize	         */	
      actionsList,                      /* actions               */	
      XtNumber(actionsList),            /* num_actions	         */	
      resources,                        /* resources	         */	
      XtNumber(resources),              /* num_resources         */	
      NULLQUARK,                        /* xrm_class	         */	
      TRUE,                             /* compress_motion       */
      TRUE,                             /* compress_exposure     */	
      TRUE,                             /* compress_enterleave   */
      FALSE,                            /* visible_interest      */
      (XtWidgetProc) Destroy,           /* destroy               */	
      NULL,                             /* resize                */	
      NULL,                             /* expose                */	
      (XtSetValuesFunc) SetValues,      /* set_values	         */	
      NULL,                             /* set_values_hook       */
      XtInheritSetValuesAlmost,         /* set_values_almost     */
      NULL,                             /* get_values_hook       */
      NULL,                             /* accept_focus	         */	
      XtVersion,                        /* version               */
      NULL,                             /* callback private      */
      NULL,                             /* tm_table              */
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
      NULL,         /* default translations         */
   }
};

WidgetClass XwprimitiveWidgetClass = (WidgetClass) &XwprimitiveClassRec;



/************************************************************************
 *
 *  ClassInitialize
 *    Initialize the primitive class structure.  This is called only
 *    the first time a primitive widget is created.  It registers the
 *    resource type converters unique to this class.
 *
 *
 * After class init, the "translations" variable will contain the compiled
 * translations to be used to augment a widget's translation
 * table if they wish to have keyboard traversal on.
 *
 ************************************************************************/

static void ClassInitialize()
{
   XwRegisterConverters();
   XwprimitiveClassRec.primitive_class.translations = 
     XtParseTranslationTable(defaultTranslations);
}




/************************************************************************
 *
 *  ClassPartInitialize
 *    Set up the inheritance mechanism for the routines exported by
 *    primitives class part.
 *
 ************************************************************************/

static void ClassPartInitialize (w)
WidgetClass w;

{
    register XwPrimitiveWidgetClass wc = (XwPrimitiveWidgetClass) w;
    register XwPrimitiveWidgetClass super =
	    (XwPrimitiveWidgetClass) wc->core_class.superclass;

    if (wc -> primitive_class.border_highlight == XtInheritBorderHighlight)
	wc -> primitive_class.border_highlight =
	   super -> primitive_class.border_highlight;

    if (wc->primitive_class.border_unhighlight == XtInheritBorderUnhighlight)
	wc -> primitive_class.border_unhighlight =
	   super -> primitive_class.border_unhighlight;

    if (wc -> primitive_class.select_proc == XtInheritSelectProc)
	wc -> primitive_class.select_proc =
	   super -> primitive_class.select_proc;

    if (wc -> primitive_class.release_proc == XtInheritReleaseProc)
	wc -> primitive_class.release_proc =
	   super -> primitive_class.release_proc;

    if (wc -> primitive_class.toggle_proc == XtInheritToggleProc)
	wc -> primitive_class.toggle_proc =
	   super -> primitive_class.toggle_proc;
}




   
/************************************************************************
 *
 *  GetHighlightGC
 *     Get the graphics context used for drawing the border.
 *
 ************************************************************************/

static void GetHighlightGC (pw)
XwPrimitiveWidget pw;

{
   XGCValues values;
   XtGCMask  valueMask;

   valueMask = GCForeground | GCBackground;
   values.foreground = pw -> primitive.highlight_color;
   values.background = pw -> core.background_pixel;

   if (pw -> primitive.highlight_tile == XwBACKGROUND)
   {
      values.foreground = pw -> core.background_pixel;
      values.background = pw -> primitive.highlight_color;
   }

   pw -> primitive.highlight_GC = XtGetGC ((Widget) pw, valueMask, &values);
}



   

/************************************************************************
 *
 *  Initialize
 *     The main widget instance initialization routine.
 *
 ************************************************************************/

static void Initialize (request, new)
XwPrimitiveWidget request, new;

{
   XwPrimitiveWidget pw = (XwPrimitiveWidget) new;
   Arg args[1];


   /* initialize traversal flag */
  
   pw->primitive.I_have_traversal = FALSE;


   /*  Check the pw widget for invalid data  */

   if (pw -> primitive.traversal_type != XwHIGHLIGHT_OFF      &&
       pw -> primitive.traversal_type != XwHIGHLIGHT_ENTER    &&
       pw -> primitive.traversal_type != XwHIGHLIGHT_TRAVERSAL)
   {
      XtWarning ("Primitive: Incorrect traversal type");
      pw -> primitive.traversal_type = XwHIGHLIGHT_OFF;
   }

   if (pw -> primitive.highlight_style != XwPATTERN_BORDER &&
       pw -> primitive.highlight_style != XwWIDGET_DEFINED)
   {
      XtWarning("Primitive: Incorrect highlight style.");
      pw -> primitive.highlight_style = XwPATTERN_BORDER;
   }

   if (pw -> primitive.highlight_tile != XwFOREGROUND)
   {
      XtWarning("Primitive: Incorrect highlight tile.");
      pw -> primitive.highlight_tile = XwFOREGROUND;
   }
   
   if (pw -> primitive.highlight_thickness < 0)
   {
      XtWarning("Primitive: Invalid highlight thickness.");
      pw -> primitive.highlight_thickness = 0;
   }
   
   if (pw -> primitive.background_tile != XwBACKGROUND)
   {
      XtWarning("Primitive: Incorrect background tile.");
      pw -> primitive.background_tile = XwBACKGROUND;
   }


   /*  Check the geometry information for the widget  */

   if (request -> core.width == 0)
      pw -> core.width += pw -> primitive.highlight_thickness * 2;
   else if (request -> core.width < pw -> primitive.highlight_thickness * 2)
   {
      pw -> primitive.highlight_thickness = request -> core.width / 2;
   }

   if (request -> core.height == 0)
      pw -> core.height += pw -> primitive.highlight_thickness * 2;
   else if (request -> core.height < pw -> primitive.highlight_thickness * 2)
   {
      pw -> primitive.highlight_thickness = request -> core.height / 2;
   }


   /*  Get the graphics contexts for the border drawing  */

   GetHighlightGC (pw);

   /*  Set some additional fields of the widget  */

   pw -> primitive.display_sensitive = FALSE;
   pw -> primitive.highlighted = FALSE;
   pw -> primitive.display_highlighted = FALSE;

   /* If this widget is requesting traversal then augment its
    * translation table with some additional events.
    */
   if (pw->primitive.traversal_type == XwHIGHLIGHT_TRAVERSAL)
     {
      XtAugmentTranslations((Widget) pw, 
			    XwprimitiveClassRec.primitive_class.translations);
      pw->core.widget_class->core_class.visible_interest = True;
    }

}



/************************************************************************
 *
 *  Realize
 *
 ************************************************************************/

static void Realize (widget, value_mask, attributes)
    Widget		 widget;
    Mask		 *value_mask;
    XSetWindowAttributes *attributes;
{
    XwPrimitiveWidget pw = (XwPrimitiveWidget) widget;

    if (pw -> primitive.cursor != None) *value_mask |= CWCursor;

    attributes->cursor = pw->primitive.cursor;

    XtCreateWindow(widget, (unsigned int) InputOutput,
		   (Visual *) CopyFromParent, *value_mask, attributes);
}


/************************************************************************
 *
 *  Destroy
 *	Clean up allocated resources when the widget is destroyed.
 *
 ************************************************************************/

static void Destroy (pw)
XwPrimitiveWidget pw;
{
   XwManagerWidget parent;

   XtRemoveCallbacks ((Widget)pw, XtNselect, pw -> primitive.select);
   XtRemoveCallbacks ((Widget)pw, XtNrelease, pw -> primitive.release);
   if (pw->primitive.I_have_traversal)
      XwProcessTraversal (pw, XwTRAVERSE_HOME, TRUE);
   else if ((XtIsSubclass((Widget)(parent = (XwManagerWidget)XtParent(pw)), 
            XwmanagerWidgetClass)) && 
           (parent->manager.traversal_on) &&
           (parent->manager.active_child == (Widget)pw))
   {
      parent->manager.active_child = NULL;

      /* Find the top most manager */
      while ((parent->core.parent != NULL) &&
             (XtIsSubclass(parent->core.parent, XwmanagerWidgetClass)))
         parent = (XwManagerWidget)parent->core.parent;

      /* Clear the toolkit kbd focus from us */
      XtSetKeyboardFocus((Widget)parent, NULL);
   }
}




/************************************************************************
 *
 *  SetValues
 *     Perform and updating necessary for a set values call.
 *
 ************************************************************************/

static Boolean SetValues (current, request, new)
Widget current, request, new;

{
   Boolean tempSensitive, tempAnSensitive;
   int tempTrav;

   XwPrimitiveWidget curpw = (XwPrimitiveWidget) current;
   XwPrimitiveWidget reqpw = (XwPrimitiveWidget) request;
   XwPrimitiveWidget newpw = (XwPrimitiveWidget) new;
   Boolean returnFlag = FALSE;
   int     difference;


   /*  Verify correct new values.  */

   if (newpw->primitive.traversal_type != XwHIGHLIGHT_OFF      &&
       newpw->primitive.traversal_type != XwHIGHLIGHT_ENTER    &&
       newpw->primitive.traversal_type != XwHIGHLIGHT_TRAVERSAL)
   {
      XtWarning("Primitive: Incorrect traversal type.");
      newpw->primitive.traversal_type = curpw->primitive.traversal_type;
   }


   /*  If traversal has been turned on, then augment the translations  */
   /*  of the new widget.                                              */

   if ((newpw->primitive.traversal_type != curpw->primitive.traversal_type)
        && (newpw->primitive.traversal_type == XwHIGHLIGHT_TRAVERSAL))
     {
         /* XXX this was a hard bug!  where were the XXX's?
          * 
          * In R2 we could not pass newpw to this toolkit call!  But I
          * have found the bug!!!!  In R2 it was a temporary structure (stack)
          * but now curmw is the temporary one and newpw is real!
          * I can see why they were worried before.  Now there is no worry.
          * Go ahead and pass newpw!
          */
         XtAugmentTranslations((Widget) newpw, 
		 XwprimitiveClassRec.primitive_class.translations); 
	 newpw->core.widget_class->core_class.visible_interest = True;
     }

    
   /* IF we have the traversal and something is happening to
    * make that not possible then move it somewhere else!
    */
   
     if (((curpw->core.sensitive != newpw->core.sensitive) ||   
        (curpw->core.ancestor_sensitive != newpw->core.ancestor_sensitive) ||
        (newpw->primitive.traversal_type != curpw->primitive.traversal_type))
        && (curpw->primitive.I_have_traversal))
     {
          newpw->primitive.highlighted = FALSE;

          tempTrav= curpw->primitive.traversal_type;
          tempSensitive = curpw->core.sensitive;
          tempAnSensitive = curpw->core.ancestor_sensitive;

          curpw->primitive.traversal_type = newpw->primitive.traversal_type;
          curpw->core.ancestor_sensitive = newpw->core.ancestor_sensitive;
          curpw->core.sensitive = newpw->core.sensitive;

          XwProcessTraversal (curpw, XwTRAVERSE_HOME, FALSE);

          curpw->primitive.traversal_type = tempTrav;
          curpw->core.sensitive = tempSensitive;
          curpw->core.ancestor_sensitive = tempAnSensitive;

        returnFlag = TRUE;
     }


   if (newpw->primitive.highlight_style != XwPATTERN_BORDER &&
       newpw->primitive.highlight_style != XwWIDGET_DEFINED)
   {
      XtWarning ("Primitive: Incorrect highlight style.");
      newpw->primitive.highlight_style = curpw->primitive.highlight_style;
   }
   
   if (newpw -> primitive.highlight_tile != XwFOREGROUND)
   {
      XtWarning ("Primitive: Incorrect highlight tile.");
      newpw->primitive.highlight_tile = curpw->primitive.highlight_tile;
   }

   if (newpw->primitive.highlight_thickness < 0)
   {
      XtWarning ("Primtive: Invalid highlight thickness.");
      newpw -> primitive.highlight_thickness =
	 curpw -> primitive.highlight_thickness;
   }


   if (newpw -> primitive.background_tile != XwBACKGROUND)
   {
      XtWarning("Primitive: Incorrect background tile.");
      newpw -> primitive.background_tile =
         curpw -> primitive.background_tile;
   }


   /*  Set the widgets background tile  */

   if (newpw -> primitive.background_tile !=
       curpw -> primitive.background_tile     ||
       newpw -> primitive.foreground !=
       curpw -> primitive.foreground          ||
       newpw -> core.background_pixel !=
       curpw -> core.background_pixel)
   {
      Mask valueMask;
      XSetWindowAttributes attributes;

      if (newpw -> primitive.background_tile == XwFOREGROUND)
      {
         valueMask = CWBackPixel;
         attributes.background_pixel = newpw -> primitive.foreground;
      }
      else
      {
         valueMask = CWBackPixel;
         attributes.background_pixel = newpw -> core.background_pixel;
      }

      if (XtIsRealized ((Widget)newpw))
         XChangeWindowAttributes (XtDisplay(newpw), newpw -> core.window,
                                  valueMask, &attributes);
	
      returnFlag = TRUE;
   }

   if (newpw -> primitive.cursor != curpw -> primitive.cursor) {
       Mask valueMask = 0;
       XSetWindowAttributes attributes;

       if (newpw->primitive.cursor != None) valueMask = CWCursor;
       attributes.cursor = newpw->primitive.cursor;

       if (XtIsRealized ((Widget)newpw))
	   XChangeWindowAttributes (XtDisplay(newpw), newpw -> core.window,
				    valueMask, &attributes);

       returnFlag = TRUE;
   }

   /*  Check the geometry in relationship to the highlight thickness  */

   if (reqpw -> core.width == 0) 
      if (curpw -> core.width == 0)
         newpw -> core.width += newpw -> primitive.highlight_thickness * 2;
   else if (reqpw -> core.width < newpw -> primitive.highlight_thickness * 2)
   {
      newpw -> primitive.highlight_thickness = reqpw -> core.width / 2;
   }

   if (reqpw -> core.height == 0) 
      if (curpw -> core.height == 0)
         newpw -> core.height += newpw -> primitive.highlight_thickness * 2;
   else if (reqpw -> core.height < newpw -> primitive.highlight_thickness * 2)
   {
      XtWarning("Primitive: The highlight thickness is too large.");
      newpw -> primitive.highlight_thickness = reqpw -> core.height / 2;
   }


   /*  If the highlight color, traversal type, highlight tile or highlight */
   /*  thickness has changed, regenerate the GC's.                         */
    
   if (curpw->primitive.highlight_color != newpw->primitive.highlight_color ||
       curpw->primitive.highlight_thickness !=
       newpw->primitive.highlight_thickness                                 ||
       curpw->primitive.highlight_style != newpw->primitive.highlight_style ||
       curpw->primitive.highlight_tile != newpw->primitive.highlight_tile)
   {
      XtDestroyGC (newpw -> primitive.highlight_GC);

      GetHighlightGC (newpw);

      returnFlag = TRUE;
   }


   /*  Return a flag which may indicate that a redraw needs to occur.  */
   
   return (returnFlag);
}



/************************************************************************
 *
 *  The traversal event processing routines.
 *    The following set of routines are the entry points invoked from
 *    each primitive widget when one of the traversal event conditions
 *    occur.  These routines are externed in Xw.h.
 *
 ************************************************************************/

void _XwTraverseLeft (w, event)
Widget w;
XEvent * event;

{
   XwProcessTraversal (w, XwTRAVERSE_LEFT, TRUE);
}


void _XwTraverseRight (w, event)
Widget w;
XEvent * event;

{
   XwProcessTraversal (w, XwTRAVERSE_RIGHT, TRUE);
}


void _XwTraverseUp (w, event)
Widget w;
XEvent * event;

{
   XwProcessTraversal (w, XwTRAVERSE_UP, TRUE);
}


void _XwTraverseDown (w, event)
Widget w;
XEvent * event;

{
   XwProcessTraversal (w, XwTRAVERSE_DOWN, TRUE);
}


void _XwTraverseNext (w, event)
Widget w;
XEvent * event;

{
   XwProcessTraversal (w, XwTRAVERSE_NEXT, TRUE);
}


void _XwTraversePrev (w, event)
Widget w;
XEvent * event;

{
   XwProcessTraversal (w, XwTRAVERSE_PREV, TRUE);
}


void _XwTraverseHome (w, event)
Widget w;
XEvent * event;

{
   XwProcessTraversal (w, XwTRAVERSE_HOME, TRUE);
}


void _XwTraverseNextTop (w, event)
Widget w;
XEvent * event;

{
   XwProcessTraversal (w, XwTRAVERSE_NEXT_TOP, TRUE);
}



/************************************************************************
 *
 *  XwProcessTraversal
 *     This function handles all of the directional traversal conditions.
 *     It first verifies that traversal is active, and then searches up
 *     the widget hierarchy until a composite widget class is found.
 *     If this widget also contains a ManagerWidgetClass, it invokes
 *     the class's traversal handler giving the widget and direction.
 *     If not a ManagerWidgetClass, it invokes the composits 
 *     move_focus_to_next or move_focus_to_prev functions for directions
 *     of TRAVERSE_NEXT and TRAVERSE_PREV respectively.
 *
 ************************************************************************/

void XwProcessTraversal (w, dir, check)
Widget w;
int    dir;
Boolean check;    

{
   Widget tw;
   XPoint ul, lr;
   XwPrimitiveWidget pw = (XwPrimitiveWidget) w;    

   if (check && (pw -> primitive.traversal_type != XwHIGHLIGHT_TRAVERSAL))
          return;

   /* Initialize the traversal checklist */
   _XwInitCheckList();

   tw = w -> core.parent;

   ul.x = w -> core.x;
   ul.y = w -> core.y;
   lr.x = ul.x + w->core.width - 1;
   lr.y = ul.y + w->core.height - 1;

   if (XtIsSubclass (tw, XwmanagerWidgetClass))
   {
       (*(((XwManagerWidgetClass) (tw -> core.widget_class)) ->
                                   manager_class.traversal_handler))
                                      (tw, ul, lr, dir);
   }
/* *** R3
   else if (dir == XwTRAVERSE_NEXT &&
           ((CompositeWidgetClass)(tw -> core.widget_class)) ->
                                   composite_class.move_focus_to_next != NULL)
   {
      (*(((CompositeWidgetClass) tw -> core.widget_class) ->
                                 composite_class.move_focus_to_next))(w);
   }

   else if (dir == XwTRAVERSE_PREV &&
           ((CompositeWidgetClass)(tw -> core.widget_class)) ->
                                   composite_class.move_focus_to_prev != NULL)
   {
      (*(((CompositeWidgetClass) tw -> core.widget_class) ->
                                 composite_class.move_focus_to_prev))(w);
   }
   *** R3 */
}



/************************************************************************
 *
 *  The border highlighting and unhighlighting routines.
 *
 *  These routines are called through primitive widget translations and
 *  by primitive widget redisplay routines.  They provide for the border
 *  drawing for active and inactive cases.
 *
 ************************************************************************/

void _XwHighlightBorder (pw)
XwPrimitiveWidget pw;

{
   if ((pw -> primitive.highlight_thickness == 0
        ) && (!XtIsSubclass ((Widget)pw, XwmenubuttonWidgetClass)))
      return;

   pw -> primitive.highlighted = TRUE;
   pw -> primitive.display_highlighted = TRUE;

   switch (pw -> primitive.highlight_style)
   {

      case XwWIDGET_DEFINED:
      {
         if (((XwPrimitiveWidgetClass) (pw -> core.widget_class)) ->
 	      primitive_class.border_highlight != NULL)
         (*(((XwPrimitiveWidgetClass) (pw -> core.widget_class)) ->
	    primitive_class.border_highlight))((Widget)pw);
      }
      break;

      case XwPATTERN_BORDER:
      {
         XRectangle rect[4];
         int windowWidth = pw -> core.width;
         int windowHeight = pw -> core.height;
         int highlightWidth = pw -> primitive.highlight_thickness;

         rect[0].x = 0;
         rect[0].y = 0;
         rect[0].width = windowWidth;
         rect[0].height = highlightWidth;

         rect[1].x = 0;
         rect[1].y = 0;
         rect[1].width = highlightWidth;
         rect[1].height = windowHeight;

         rect[2].x = windowWidth - highlightWidth;
         rect[2].y = 0;
         rect[2].width = highlightWidth;
         rect[2].height = windowHeight;

         rect[3].x = 0;
         rect[3].y = windowHeight - highlightWidth;
         rect[3].width = windowWidth;
         rect[3].height = highlightWidth;

         XFillRectangles (XtDisplay (pw), XtWindow (pw),
                          pw -> primitive.highlight_GC, &rect[0], 4);
      }
      break;
   }     
}




void _XwUnhighlightBorder (pw)
XwPrimitiveWidget pw;

{
   if ((pw -> primitive.highlight_thickness == 0
        ) && (!XtIsSubclass ((Widget)pw, XwmenubuttonWidgetClass)))
      return;


   pw -> primitive.highlighted = FALSE;
   pw -> primitive.display_highlighted = FALSE;

   switch (pw -> primitive.highlight_style)
   {
      case XwWIDGET_DEFINED:
      {
         if (((XwPrimitiveWidgetClass) (pw -> core.widget_class)) ->
 	      primitive_class.border_unhighlight != NULL)
            (*(((XwPrimitiveWidgetClass) (pw -> core.widget_class)) ->
               primitive_class.border_unhighlight))((Widget)pw);
      }
      break;


      case XwPATTERN_BORDER:
      {
         int windowWidth = pw -> core.width;
         int windowHeight = pw -> core.height;
         int highlightWidth = pw -> primitive.highlight_thickness;


         XClearArea (XtDisplay (pw), XtWindow (pw),
                     0, 0, windowWidth, highlightWidth, False);

         XClearArea (XtDisplay (pw), XtWindow (pw),
                     0, 0, highlightWidth, windowHeight, False);

         XClearArea (XtDisplay (pw), XtWindow (pw),
                     windowWidth - highlightWidth, 0, 
		     highlightWidth, windowHeight, False);

         XClearArea (XtDisplay (pw), XtWindow (pw),
                     0, windowHeight - highlightWidth,
                     windowWidth, highlightWidth, False);
      }
      break;
   }     
}




/************************************************************************
 *
 *  Enter & Leave
 *      Enter and leave event processing routines.  Handle border
 *      highlighting and dehighlighting.
 *
 ************************************************************************/

void _XwPrimitiveEnter (pw, event)
XwPrimitiveWidget pw;
XEvent * event;

{
  if (pw -> primitive.traversal_type == XwHIGHLIGHT_ENTER)
     _XwHighlightBorder (pw);
}


void _XwPrimitiveLeave (pw, event)
XwPrimitiveWidget pw;
XEvent * event;

{
  if (pw -> primitive.traversal_type == XwHIGHLIGHT_ENTER)
     _XwUnhighlightBorder (pw);
}


/************************************************************************
 *
 * Visibility
 *      Track whether a widget is visible.
 *
 ***********************************************************************/
void _XwPrimitiveVisibility (pw, event)
  XwPrimitiveWidget pw;
  XEvent * event;
{
  XVisibilityEvent * vEvent = (XVisibilityEvent *)event;

  if (vEvent->state == VisibilityFullyObscured)
    {
      pw->core.visible = False;
      if (pw->primitive.I_have_traversal)
       XwProcessTraversal (pw, XwTRAVERSE_HOME, FALSE);
     }
  else 
    pw->core.visible = True;


}



/************************************************************************
 *
 * Unmap
 *      Track whether a widget is visible.
 *
 ***********************************************************************/
void _XwPrimitiveUnmap (pw, event)
  XwPrimitiveWidget pw;
  XEvent * event;
{
   pw->core.visible = False;
   if (pw->primitive.I_have_traversal)
       XwProcessTraversal (pw, XwTRAVERSE_HOME, FALSE);

}

/************************************************************************
 *
 *  Focus In & Out
 *      Handle border highlighting and dehighlighting.
 *
 ************************************************************************/

void _XwPrimitiveFocusIn (pw, event)
XwPrimitiveWidget pw;
XEvent * event;

{
   int widgetType;


   /*  SET THE FOCUS PATH DOWN TO THIS WIDGET.  */

   XwSetFocusPath((Widget)pw);


   /*  IF THE SPECIFIED WIDGET CANNOT BE TRAVERSED TO, THEN INVOKE  */
   /*  MANAGER'S TRAVERSAL HANDLER WITH DIRECTION = HOME.           */

   if (! XwTestTraversability((Widget)pw, &widgetType)) 
     XwProcessTraversal (pw, XwTRAVERSE_HOME, FALSE);
   else
   {
      _XwHighlightBorder (pw);
      pw->primitive.I_have_traversal = TRUE;
   }
}


void _XwPrimitiveFocusOut (pw, event)
XwPrimitiveWidget pw;
XEvent * event;

{
   if (pw -> primitive.traversal_type == XwHIGHLIGHT_TRAVERSAL)
   {
      _XwUnhighlightBorder (pw);
      pw->primitive.I_have_traversal = FALSE;
   }
}


void XwSetFocusPath(w)
Widget w;

{
   XwManagerWidget mw;
   Widget tempoldActive, oldActive;


   /*  IF THE PARENT OF THIS WIDGET IS NOT A MANAGER THEN THERE'S NO  */
   /*  PATH TO SET, SO RETURN.                                        */

   if (! XtIsSubclass(w->core.parent, XwmanagerWidgetClass)) return;

   mw = (XwManagerWidget)w->core.parent;


   /*  IF THIS WIDGET IS ALREADY THE ACTIVE CHID THEN THERE'S NOTHING  */
   /*  TO DO.                                                          */

   if (mw->manager.active_child == w) return;

   oldActive = mw->manager.active_child;
   mw->manager.active_child = w;
   XwProcessTraversal (w, XwTRAVERSE_CURRENT, FALSE);


   /*  TRAVERSE DOWN THE OTHER BRANCH OF TREE TO CLEAN UP ACTIVE CHILD  */
   /*  PATH.                                                            */

   while (oldActive != NULL) 
   {
      if (! XtIsSubclass(oldActive, XwmanagerWidgetClass)) 
         oldActive = NULL;
      else
      {
         tempoldActive = ((XwManagerWidget)oldActive)->manager.active_child;
         ((XwManagerWidget)oldActive)->manager.active_child = NULL;          
         oldActive=tempoldActive;
      }
   }
  
   XwSetFocusPath((Widget)mw);
}






/************************************************************************
 *
 *  _XwDrawBox
 *	Draw the bordering on the drawable d, thick pixels wide,
 *      using the provided GC and rectangle.
 *
 ************************************************************************/

void _XwDrawBox (display, d, normal_GC, thick, x, y, width, height)
Display * display;
Drawable d;
GC normal_GC;
register int thick;
register int x;
register int y;
register int width;
register int height;

{
   XRectangle rect[4];

   rect[0].x = x;		/*  top   */
   rect[0].y = y;
   rect[0].width = width;
   rect[0].height = 2;

   rect[1].x = x;		/*  left  */
   rect[1].y = y + 2;
   rect[1].width = 2;
   rect[1].height = height - 4;

   rect[2].x = x;		/*  bottom  */
   rect[2].y = y + height - 2;
   rect[2].width = width;
   rect[2].height = 2;

   rect[3].x = x + width - 2;	/*  Right two segments  */
   rect[3].y = y + 2;
   rect[3].width = 2;
   rect[3].height = height - 4;

   XFillRectangles (display, d, normal_GC, &rect[0], 4);
}
