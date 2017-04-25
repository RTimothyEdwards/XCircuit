/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        Manager.c
 **
 **   Project:     X Widgets
 **
 **   Description: This file contains source for the Manager Meta Widget.
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
#include <X11/Shell.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <X11/StringDefs.h>

static Boolean XwFocusIsHere();
static void    MgrDestroy();
static void    MgrClassPartInit();
static void    ClassInitialize();
static void    Initialize();
static Boolean SetValues();



/*************************************<->*************************************
 *
 *   Default translations for all subclasses of Manager
 *   -----------
 *   Note, if a manager subclass has additional translations it
 *   will have to duplicate this set as well.
 *************************************<->***********************************/

static char defaultTranslations[] = 
    "<EnterWindow>:          enter() \n\
     <LeaveWindow>:          leave() \n\
     <Visible>:              visible() \n\
     <FocusIn>:              focusIn()";



/*************************************<->*************************************
 *
 *   Default actions for all subclasses of Manager
 *   -----------
 *
 *************************************<->***********************************/

static XtActionsRec actionsList[] =
{
  {"enter", (XtActionProc) _XwManagerEnter},
  {"leave", (XtActionProc) _XwManagerLeave},
  {"focusIn", (XtActionProc) _XwManagerFocusIn},  
  {"visible", (XtActionProc) _XwManagerVisibility},
};
 
/*
 *  Resource definitions for Subclasses of Manager
 */

static XtResource resources[] =
{
   {
     XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
     XtOffset (XwManagerWidget, manager.foreground),
     XtRString, "XtDefaultForeground"
   },

   {
     XtNbackgroundTile, XtCBackgroundTile, XtRTileType, sizeof (int),
     XtOffset (XwManagerWidget, manager.background_tile),
     XtRString, "background"
   },

   {
     XtNhighlightThickness, XtCHighlightThickness, XtRInt, sizeof (int),
     XtOffset (XwManagerWidget, manager.highlight_thickness),
     XtRString, "0"
   },

   {
     XtNtraversalOn, XtCTraversalOn, XtRBoolean, sizeof (Boolean),
     XtOffset (XwManagerWidget, manager.traversal_on),
     XtRString, "False"
   },

   {
     XtNlayout, XtCLayout, XtRLayout, sizeof(int),
     XtOffset(XwManagerWidget, manager.layout),
     XtRString, "minimize"
   },

   {
     XtNnextTop, XtCCallback, XtRCallback, sizeof(caddr_t),
     XtOffset (XwManagerWidget, manager.next_top),
     XtRPointer, (caddr_t) NULL
   },

};


/******************************************************************
 *  The Manager class record definition
 ******************************************************************/

XwManagerClassRec XwmanagerClassRec =
{
   {
      (WidgetClass) &constraintClassRec,    /* superclass     */	
      "XwManager",                      /* class_name	     */	
      sizeof(XwManagerRec),             /* widget_size	     */	
      ClassInitialize,                  /* class_initialize  */    
      MgrClassPartInit,                 /* class part initialize */
      FALSE,                            /* class_inited      */	
      Initialize,                       /* initialize	     */	
      NULL,                             /* initialize hook   */
      NULL,                             /* realize	     */	
      actionsList,                      /* actions           */	
      XtNumber(actionsList),            /* num_actions	     */	
      resources,                        /* resources	     */	
      XtNumber(resources),              /* num_resources     */	
      NULLQUARK,                        /* xrm_class	     */	
      TRUE,                             /* compress_motion   */	
      TRUE,                             /* compress_exposure */	
      TRUE,                             /* compress enterleave */
      FALSE,                            /* visible_interest  */	
      MgrDestroy,                       /* destroy           */	
      NULL,                             /* resize            */	
      NULL,                             /* expose            */	
      SetValues,                        /* set_values	     */	
      NULL,                             /* set_values_hook   */
      XtInheritSetValuesAlmost,         /* set_values_almost */
      NULL,                             /* get_values_hook   */
      NULL,                             /* accept_focus	     */
      XtVersion,                        /* version           */	
      NULL,                             /* callback private  */
      NULL,                             /* tm_table          */
      NULL,                             /* query geometry    */
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
   },
   {                /* Composite class */
      NULL,                             /* Geometry Manager  */
      NULL,                             /* Change Managed    */
      _XwManagerInsertChild,            /* Insert Child      */
      XtInheritDeleteChild,             /* Delete Child      */
      NULL,
   },

   {  /* constraint class fields */
      NULL,
      0,
      0,
      NULL,
      NULL,
      NULL,
      NULL,
   },

   {
      (XwTraversalProc) mgr_traversal,    /* Mgr class traversal handler */
      NULL,                               /* default translations */
   }
};

WidgetClass XwmanagerWidgetClass = (WidgetClass) &XwmanagerClassRec;



/************************************************************************
 *
 *  ClassInitialize
 *    Initialize the manager class structure.  This is called only the
 *    first time a manager widget is created.
 *
 *
 * After class init, the "translations" variable will contain the compiled
 * translations to be used to augment a widget's translation
 * table if they wish to have keyboard traversal on.
 *
 ************************************************************************/

static void ClassInitialize()
{
    XwRegisterConverters();   /* Register Library Conversion Rtnes */

    XwmanagerClassRec.manager_class.translations = 
              XtParseTranslationTable(defaultTranslations);
}


/************************************************************************
 *
 *  MgrClassPartInit
 *
 *  Used by subclasses of manager to inherit class record procedures.
 *
 ************************************************************************/

static void MgrClassPartInit(w)
WidgetClass  w;

{
   XwManagerWidgetClass mw= (XwManagerWidgetClass) w;

    if (mw->manager_class.traversal_handler == XtInheritTraversalProc)
	mw->manager_class.traversal_handler = (XwTraversalProc) mgr_traversal;
}




   

/************************************************************************
 *
 *  Initialize
 *     The main widget instance initialization routine.
 *
 ************************************************************************/

static void Initialize (request, new)
  Widget request, new;
{
   XwManagerWidget mw = (XwManagerWidget) new;
   Arg args[1];


   if ((mw->manager.layout < XwIGNORE) || (mw->manager.layout > XwSWINDOW))
   {
      mw->manager.layout = XwMINIMIZE;
      XtWarning("Manager: Invalid layout setting.");
   }


   mw -> manager.managed_children = (WidgetList) XtMalloc
      (XwBLOCK * sizeof(Widget));
   mw -> manager.num_slots = XwBLOCK;
   mw -> manager.num_managed_children = 0;
   mw -> manager.active_child = NULL;
   
   mw -> composite.children = (WidgetList) XtMalloc
      (XwBLOCK * sizeof(Widget));
   mw -> composite.num_slots = XwBLOCK;
   mw -> composite.num_children = 0;
/*   mw -> composite.num_mapped_children = 0; for R3 */

   mw -> composite.insert_position = (XtOrderProc) _XwInsertOrder;


   /*  Verify the highlight and tiling resources  */
   
   if (mw -> manager.background_tile != XwBACKGROUND)
   {
      XtWarning("Manager: Incorrect background tile.");
      mw -> manager.background_tile = XwBACKGROUND;
   }


   /* If this widget is requesting traversal then augment its
    * translation table with some additional events.
    */
   if (mw->manager.traversal_on == True)
     {
      XtAugmentTranslations((Widget) mw, 
			    XwmanagerClassRec.manager_class.translations);
      mw->core.widget_class->core_class.visible_interest = True;
    }

}





/*********************************************************************
 *
 * _XwManagerInsertChild
 *
 ********************************************************************/

 /* ARGSUSED */
void _XwManagerInsertChild(w)
    Widget	w;
{
    Cardinal	    position;
    Cardinal	    i;
    CompositeWidget cw;

    cw = (CompositeWidget) w->core.parent;
    if (cw -> composite.insert_position != NULL)
        position = (*cw->composite.insert_position)((Widget)cw);
    else
        position = cw->composite.num_children;

    
    /*************************************************
     * Allocate another block of space if needed
     *************************************************/
    if ((cw -> composite.num_children + 1) >
		 cw -> composite.num_slots)
	     {
	       cw -> composite.num_slots += XwBLOCK;
	       cw -> composite.children = (WidgetList)
		  XtRealloc ((caddr_t) cw -> composite.children,
			     (cw -> composite.num_slots * sizeof(Widget)));
	     }

    
    /********************************************************
     * Ripple children up one space from "position"
     *********************************************************/
    
    for (i = cw->composite.num_children; i > position; i--) {
        cw->composite.children[i] = cw->composite.children[i-1];
    }
    cw->composite.children[position] = w;
    cw->composite.num_children++;
}


/***************************************************************************
 *
 * Cardinal
 * _XwInsertOrder (cw)
 *
 ************************************************************************/

Cardinal _XwInsertOrder (cw)

   CompositeWidget cw;         /* Composite widget being inserted into */
{
   Cardinal position = cw->composite.num_children;

   return (position);
}

/**********************************************************************
 *
 * _XwReManageChildren
 *
 * This procedure will be called by the ChangeManged procedure of
 * each manager.  It will reassemble the currently managed children
 * into the "manager.managed_children" list.
 *
 ********************************************************************/
void _XwReManageChildren(mw)
 XwManagerWidget mw;
{
   int i;
    
    /* Put "managed children" info together again */

    mw -> manager.num_managed_children = 0;
    for (i=0; i < mw->composite.num_children; i++)
     {
       if (mw -> composite.children[i]->core.managed)
	  {
	    if ((mw -> manager.num_managed_children + 1) >
		 mw -> manager.num_slots)
	     {
	       mw -> manager.num_slots += XwBLOCK;
	       mw -> manager.managed_children = (WidgetList)
		  XtRealloc ((caddr_t) mw -> manager.managed_children,
			     (mw -> manager.num_slots * sizeof(Widget)));
	     }
	    mw -> manager.managed_children
	            [mw -> manager.num_managed_children++] =
		       mw -> composite.children[i];
	  }
     }
}


/************************************************************************
 *
 *  SetValues
 *     Perform and updating necessary for a set values call.
 *
 ************************************************************************/

static Boolean SetValues (current, request, new, last)
Widget current, request, new;
Boolean last;

{
   Boolean returnFlag = False;
   Boolean tempTrav, tempVisible, tempSensitive;
   Boolean tempAnSensitive, tempMapped;
   XwManagerWidget curmw = (XwManagerWidget) current;
   XwManagerWidget reqmw = (XwManagerWidget) request;
   XwManagerWidget newmw = (XwManagerWidget) new;
   Widget tempoldActive, old_active;
   XwManagerWidget parent;
   Boolean traversalIsHere = False;
   Boolean parentIsMgr = False;


   /*  Verify correct new values.  */

   if (newmw -> manager.traversal_on != True &&
       newmw -> manager.traversal_on != False)
      newmw -> manager.traversal_on = curmw -> manager.traversal_on;


   /*  Process the change in values */
   
    if ((curmw->core.mapped_when_managed != newmw->core.mapped_when_managed) ||
        (curmw->core.sensitive != newmw->core.sensitive) ||
	(curmw->core.ancestor_sensitive != newmw->core.ancestor_sensitive) ||
        (curmw->core.visible != newmw-> core.visible)    ||
        (curmw->manager.traversal_on != newmw->manager.traversal_on))
       {
         returnFlag = TRUE;
         if (XwFocusIsHere(curmw))
         {
            tempTrav= curmw->manager.traversal_on;
            tempVisible = curmw->core.visible;
            tempSensitive = curmw->core.sensitive;
            tempAnSensitive = curmw->core.ancestor_sensitive;
            tempMapped = curmw->core.mapped_when_managed;

            curmw->manager.traversal_on = newmw->manager.traversal_on;
            curmw->core.ancestor_sensitive = newmw->core.ancestor_sensitive;
            curmw->core.sensitive = newmw->core.sensitive;
            curmw->core.visible = newmw->core.visible;
            curmw->core.mapped_when_managed = newmw->core.mapped_when_managed;

            XwProcessTraversal (curmw, XwTRAVERSE_HOME, FALSE);

            curmw->manager.traversal_on = tempTrav;
            curmw->core.visible = tempVisible;
            curmw->core.sensitive = tempSensitive;
            curmw->core.ancestor_sensitive = tempAnSensitive;
            curmw->core.mapped_when_managed = tempMapped;
	 }
         else 
         {
            traversalIsHere = False;
            parentIsMgr = False;

            /* There are special cases if we are the highest Xw mgr widget */
            if (XtIsSubclass((Widget)(parent = (XwManagerWidget)XtParent(newmw)),
                   XwmanagerWidgetClass))
            {
               parentIsMgr = True;
               if (parent->manager.active_child == (Widget)curmw)
                  traversalIsHere = True;
            }
            else if (newmw->manager.active_child)
               traversalIsHere = True;

            if (traversalIsHere)
            {
               /* Clear the active child path, from us down */
               old_active = newmw->manager.active_child;
               newmw->manager.active_child = NULL;
               if (parentIsMgr)
                  parent->manager.active_child = NULL;
               while (old_active != NULL)
               {
                  if (!XtIsSubclass(old_active, XwmanagerWidgetClass))
                     old_active = NULL;
                  else
                  {
                     tempoldActive = ((XwManagerWidget)old_active)->manager.
                                                                   active_child;
                     ((XwManagerWidget)old_active)->manager.active_child = NULL;
                     old_active = tempoldActive;
                  }
               }

               /* Find the top most manager */
               if (parentIsMgr)
               {
                  while ((parent->core.parent != NULL) &&
                     (XtIsSubclass(parent->core.parent, XwmanagerWidgetClass)))
                     parent = (XwManagerWidget)parent->core.parent;
               }
               else
                  parent = curmw;

               /* Clear the toolkit kbd focus */
               XtSetKeyboardFocus((Widget)parent, NULL);
            }
         }
        }


   if (newmw->manager.layout != curmw->manager.layout) 
     {
       if ((newmw->manager.layout < XwIGNORE) ||
           (newmw->manager.layout > XwSWINDOW))
	 {
             newmw->manager.layout = curmw->manager.layout;
             XtWarning("Manager: Invalid layout setting.");
	  }
        else returnFlag = True;
     }


   if (newmw -> manager.highlight_thickness < 0)
   {
      XtWarning ("Manager: Invalid highlight thickness.");
      newmw -> manager.highlight_thickness =
	 curmw -> manager.highlight_thickness;
   }


   if (newmw -> manager.background_tile != XwBACKGROUND)
   {
      XtWarning("Manager: Incorrect background tile.");
      newmw -> manager.background_tile =
         curmw -> manager.background_tile;
   }

   /*  Set the widget's background tile  */

   if (newmw -> manager.background_tile !=
       curmw -> manager.background_tile     ||
       newmw -> manager.foreground !=
       curmw -> manager.foreground          ||
       newmw -> core.background_pixel !=
       curmw -> core.background_pixel)
   {
      Mask valueMask;
      XSetWindowAttributes attributes;

      if (newmw -> manager.background_tile == XwFOREGROUND)
      {
         valueMask = CWBackPixel;
         attributes.background_pixel = newmw -> manager.foreground;
      }
      else
      {
         valueMask = CWBackPixel;
         attributes.background_pixel = newmw -> core.background_pixel;
      }

      if (XtIsRealized ((Widget)newmw))
         XChangeWindowAttributes (XtDisplay(newmw), newmw -> core.window,
                                  valueMask, &attributes);
	
      returnFlag = TRUE;
   }


   /*  Check the geometry in relationship to the highlight thickness  */

   if ((request -> core.width == 0) && (current -> core.width == 0))
      newmw -> core.width += newmw -> manager.highlight_thickness * 2;

   if ((request -> core.height == 0) && (current -> core.height == 0))
      newmw -> core.height += newmw -> manager.highlight_thickness * 2;


   /* If this widget is requesting traversal then augment its
    * translation table with some additional events.
    */
   if ((newmw->manager.traversal_on != curmw->manager.traversal_on) &&
       (newmw->manager.traversal_on == True))
     {
         /* XXX this was a hard bug!  where were the XXX's?
          * 
          * In R2 we could not pass newmw to this toolkit call!  But I
          * have found the bug!!!!  In R2 it was a temporary structure (stack)
          * but now curmw is the temporary one and newmw is real!
          * I can see why they were worried before.  Now there is no worry.
          * Go ahead and pass newmw!
          */
         XtAugmentTranslations((Widget) newmw, 
			       XwmanagerClassRec.manager_class.translations);
	 newmw->core.widget_class->core_class.visible_interest = True;

     }


   /*  Return flag to indicate if redraw is needed.  */

   return (returnFlag);
}



/************************************************************************
 *
 *  Enter, FocusIn and Leave Window procs
 *
 *     These two procedures handle traversal activation and deactivation
 *     for manager widgets. They are invoked directly throught the
 *     the action table of a widget.
 *
 ************************************************************************/

void _XwManagerEnter (mw, event)
XwManagerWidget   mw;
XEvent          * event;

{
   XPoint ul,lr;

   ul.x = ul.y = lr.x = lr.y = 0;

   /* Only the top level manager should invoke a traversal handler*/
   if (!XtIsSubclass (mw -> core.parent, XwmanagerWidgetClass))
   {
      /*
       * All external entry points into the traversal code must take
       * care of initializing the traversal list.
       */
      _XwInitCheckList();

      (*(((XwManagerWidgetClass) mw -> core.widget_class) ->
           manager_class.traversal_handler))
	           ((Widget)mw, ul, lr, XwTRAVERSE_CURRENT);
   }

   return;
}



void _XwManagerFocusIn (mw, event)
XwManagerWidget   mw;
XEvent          * event;

{
   _XwManagerEnter (mw, event);
   return;
}


void _XwManagerLeave (mw, event)
XwManagerWidget   mw;
XEvent          * event;

{
   XwManagerWidget   tw;

  /* if my parent is NOT a subclass of XwManager then I will assume
     that I have left the traversal hierarchy.  For the time being
     I will set the focus back to pointer root--I should not have to
     do this if the window manager people can come to an agreement
     on conventions for focus change.
  */
   if (!XtIsSubclass (mw -> core.parent, XwmanagerWidgetClass))
	     XSetInputFocus (XtDisplay (mw->core.parent), 
                              PointerRoot,  RevertToNone, CurrentTime);
}




/************************************************************************
 *
 *  Find a Traversable Primitive
 *     This functions is used by manager widget's traversal handlers
 *     to determine if there is a primitive in their hierarchy which
 *     can be traversed to.
 *
 ************************************************************************/

Boolean _XwFindTraversablePrim (cw)
CompositeWidget cw;

{
   register int i;
   WidgetList   wList;
   Widget       w;

   wList = cw -> composite.children;

   for (i = 0; i < cw -> composite.num_children; i++)
   {
      w = *wList++;

      if (XtIsSubclass(w, XwmanagerWidgetClass) && 
         ((((XwManagerWidget)w)->manager.traversal_on == False) ||
         (w->core.visible == FALSE)))
      {
         continue;
      }

      if (XtIsSubclass (w, compositeWidgetClass) && w -> core.managed == True)
      {
	 if (_XwFindTraversablePrim ((CompositeWidget) w))
         {
	    return (True);
	 }
      }

      else if (XtIsSubclass (w, XwprimitiveWidgetClass))
      {
	 if (w -> core.sensitive == True &&
	     w -> core.ancestor_sensitive == True &&
             w -> core.visible == True   &&
             w -> core.managed == True   &&
             w -> core.mapped_when_managed == True   &&
             w -> core.being_destroyed == False   &&
	     ((XwPrimitiveWidget) w)->primitive.traversal_type ==
	     XwHIGHLIGHT_TRAVERSAL)
      
         {
	    return (True);
	 }
      }

      else       /*  Check for non - primitive class primitives??  */
      {
      }
   }

   return (False);
}


/*************************************<->*************************************
 *
 *  Boolean
 *  XwTestTraversability(widget, widgetType)
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

Boolean XwTestTraversability(widget, widgetType)
   Widget widget;
   int * widgetType;
{
   Boolean returnFlag=FALSE;
   
      if (XtIsSubclass (widget, XwprimitiveWidgetClass))
	 {
           *widgetType = XwPRIMITIVE;
           if (((XwPrimitiveWidget) widget)->primitive.traversal_type ==
	       XwHIGHLIGHT_TRAVERSAL)
                     returnFlag = TRUE;
          }
       else
	 if (XtIsSubclass (widget, XwmanagerWidgetClass))
	  {
	    *widgetType = XwMANAGER;
	    if (((XwManagerWidget)widget) -> manager.traversal_on)
	            returnFlag = TRUE;
	  }
         else
              *widgetType = 0;


      if (widget-> core.sensitive != True ||
	  widget-> core.ancestor_sensitive != True ||
          widget-> core.visible != True   ||
          widget-> core.managed != True   ||
          widget-> core.mapped_when_managed != True ||
          widget-> core.being_destroyed == True)

               returnFlag = FALSE;


   return(returnFlag);
}


static Boolean XwFocusIsHere(mw)

  XwManagerWidget mw;
{
  while (1)
  {
   if (mw->manager.active_child == NULL) return(FALSE);
   if (XtIsSubclass(mw->manager.active_child, XwmanagerWidgetClass))
       mw= (XwManagerWidget)mw->manager.active_child;
   else
      if (XtIsSubclass(mw->manager.active_child, XwprimitiveWidgetClass))
	{
          if (((XwPrimitiveWidget)mw->manager.active_child)
              ->primitive.I_have_traversal) return(TRUE);
          else  return(FALSE);
	}
    }
}


/*************************************<->*************************************
 *
 *  MgrDestroy (w)
 *
 *   Description:
 *   -----------
 *     Free up space allocated to track managed children.
 *
 *   Inputs:
 *   ------
 *   w = manager subclass to be destroyed.
 * 
 *
 *************************************<->***********************************/

static void MgrDestroy(w)
    XwManagerWidget	w;
{
   XwManagerWidget parent;

   /*
    * If we are in the traversal focus path, then we need to clean
    * things up.
    */
   if (XwFocusIsHere(w))
      XwProcessTraversal(w, XwTRAVERSE_HOME, FALSE);
   else if ((XtIsSubclass((Widget)(parent = (XwManagerWidget)XtParent(w)),
            XwmanagerWidgetClass)) &&
           (parent->manager.active_child == (Widget)w))
   {
      parent->manager.active_child = NULL;
   }
   XtRemoveCallbacks ((Widget)w, XtNnextTop, w -> manager.next_top);
   XtFree((char *) w->manager.managed_children);
}


/************************************************************************
 *
 * Visibility
 *      Track whether a widget is visible.
 *
 ***********************************************************************/
void _XwManagerVisibility (mw, event)
  XwManagerWidget mw;
  XEvent * event;
{
  XVisibilityEvent * vEvent = (XVisibilityEvent *)event;

  if (vEvent->state == VisibilityFullyObscured)
    {
      mw->core.visible = False;
      if (XwFocusIsHere(mw))
         XwProcessTraversal (mw, XwTRAVERSE_HOME, FALSE);
     }
  else 
    mw->core.visible = True;


}


/************************************************************************
 *
 * Unmap
 *      Track whether a widget is visible.
 *
 ***********************************************************************/
void _XwManagerUnmap (mw, event)
  XwManagerWidget mw;
  XEvent * event;
{
   mw->core.visible = False;
   if (XwFocusIsHere(mw))
         XwProcessTraversal (mw, XwTRAVERSE_HOME, FALSE);

}
