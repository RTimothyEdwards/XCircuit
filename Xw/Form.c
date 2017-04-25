/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        Form.c
 **
 **   Project:     X Widgets
 **
 **   Description: Contains code for the X Widget's Form manager.
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
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <Xw/Form.h>
#include <Xw/FormP.h>
#include <X11/StringDefs.h>
#include <X11/keysymdef.h>   


/*  Constraint resource list for Form  */

static XtResource constraintResources[] = 
{
   {
      XtNxRefName, XtCXRefName, XtRString, sizeof(caddr_t),
      XtOffset(XwFormConstraints, x_ref_name), XtRString, (caddr_t) NULL
   },

   {
      XtNxRefWidget, XtCXRefWidget, XtRPointer, sizeof(caddr_t),
      XtOffset(XwFormConstraints, x_ref_widget), XtRPointer, NULL
   },

   {
      XtNxOffset, XtCXOffset, XtRInt, sizeof(int),
      XtOffset(XwFormConstraints, x_offset), XtRString, "0"
   },

   {
      XtNxAddWidth, XtCXAddWidth, XtRBoolean, sizeof(Boolean),
      XtOffset(XwFormConstraints, x_add_width), XtRString, "False"
   },

   {
      XtNxVaryOffset, XtCXVaryOffset, XtRBoolean, sizeof(Boolean),
      XtOffset(XwFormConstraints, x_vary_offset), XtRString, "False"
   },

   {
      XtNxResizable, XtCXResizable, XtRBoolean, sizeof(Boolean),
      XtOffset(XwFormConstraints, x_resizable), XtRString, "False"
   },

   {
      XtNxAttachRight, XtCXAttachRight, XtRBoolean, sizeof(Boolean),
      XtOffset(XwFormConstraints, x_attach_right), XtRString, "False"
   },

   {
      XtNxAttachOffset, XtCXAttachOffset, XtRInt, sizeof(int),
      XtOffset(XwFormConstraints, x_attach_offset), XtRString, "0"
   },

   {
      XtNyRefName, XtCYRefName, XtRString, sizeof(caddr_t),
      XtOffset(XwFormConstraints, y_ref_name), XtRString, (caddr_t) NULL
   },

   {
      XtNyRefWidget, XtCYRefWidget, XtRPointer, sizeof(caddr_t),
      XtOffset(XwFormConstraints, y_ref_widget), XtRPointer, NULL
   },

   {
      XtNyOffset, XtCYOffset, XtRInt, sizeof(int),
      XtOffset(XwFormConstraints, y_offset), XtRString, "0"
   },

   {
      XtNyAddHeight, XtCYAddHeight, XtRBoolean, sizeof(Boolean),
      XtOffset(XwFormConstraints, y_add_height), XtRString, "False"
   },

   {
      XtNyVaryOffset, XtCYVaryOffset, XtRBoolean, sizeof(Boolean),
      XtOffset(XwFormConstraints, y_vary_offset), XtRString, "False"
   },

   {
      XtNyResizable, XtCYResizable, XtRBoolean, sizeof(Boolean),
      XtOffset(XwFormConstraints, y_resizable), XtRString, "False"
   },

   {
      XtNyAttachBottom, XtCYAttachBottom, XtRBoolean, sizeof(Boolean),
      XtOffset(XwFormConstraints, y_attach_bottom), XtRString, "False"
   },

   {
      XtNyAttachOffset, XtCYAttachOffset, XtRInt, sizeof(int),
      XtOffset(XwFormConstraints, y_attach_offset), XtRString, "0"
   }
};



/*  Static routine definitions  */

static void    Initialize();
static void    Realize();
static void    Resize();
static void    Destroy();
static Boolean SetValues();

static void      ChangeManaged();
static XtGeometryResult GeometryManager();

static void      ConstraintInitialize();
static void    ConstraintDestroy();
static Boolean ConstraintSetValues();

static void        GetRefWidget();
static Widget      XwFindWidget();
static XwFormRef * XwGetFormRef();
static Widget      XwFindValidRef();
static XwFormRef * XwRefTreeSearch();
static XwFormRef * XwParentRefTreeSearch();
static void        XwMakeRefs();
static void        XwDestroyRefs();
static void        XwProcessRefs();
static void        XwAddRef();
static void        XwRemoveRef();
static void        XwFindDepthAndCount();
static void        XwInitProcessList();
static void        XwConstrainList();
static void        XwFreeConstraintList();


/*  Static global variable definitions  */

static int depth, leaves, arrayIndex;


/*  The Form class record */

XwFormClassRec XwformClassRec =
{
   {
      (WidgetClass) &XwmanagerClassRec, /* superclass	         */	
      "XwForm",                         /* class_name	         */	
      sizeof(XwFormRec),                /* widget_size	         */	
      NULL,                             /* class_initialize      */    
      NULL,                             /* class_part_initialize */    
      FALSE,                            /* class_inited          */	
      (XtInitProc) Initialize,          /* initialize	         */	
      NULL,                             /* initialize_hook       */
      (XtRealizeProc) Realize,          /* realize	         */	
      NULL,	                        /* actions               */	
      0,                                /* num_actions	         */	
      NULL,                             /* resources	         */	
      0,                                /* num_resources         */	
      NULLQUARK,                        /* xrm_class	         */	
      TRUE,                             /* compress_motion       */	
      TRUE,                             /* compress_exposure     */	
      TRUE,                             /* compress_enterleave   */
      FALSE,                            /* visible_interest      */	
      (XtWidgetProc) Destroy,           /* destroy               */	
      (XtWidgetProc) Resize,            /* resize                */
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

   {                                         /*  composite class           */
      (XtGeometryHandler) GeometryManager,   /*  geometry_manager          */
      (XtWidgetProc) ChangeManaged,          /*  change_managed            */
      XtInheritInsertChild,		     /* insert_child	           */
      XtInheritDeleteChild,                  /*  delete_child (inherited)  */
      NULL,
   },

   {                                      /*  constraint class            */
      constraintResources,		  /*  constraint resource set     */
      XtNumber(constraintResources),      /*  num_resources               */
      sizeof(XwFormConstraintRec),	  /*  size of the constraint data */
      (XtInitProc) ConstraintInitialize,  /*  contraint initilize proc    */
      (XtWidgetProc) ConstraintDestroy,   /*  contraint destroy proc      */
      (XtSetValuesFunc) ConstraintSetValues,  /*  contraint set values proc */
      NULL,
   },

   {                           /*  manager class      */
      (XwTraversalProc) XtInheritTraversalProc   /*  traversal handler  */
   },

   {              /*  form class  */
      0           /*  mumble      */               
   }	
};


WidgetClass XwformWidgetClass = (WidgetClass) &XwformClassRec;




/************************************************************************
 *
 *  Initialize
 *     The main widget instance initialization routine.
 *
 ************************************************************************/

static void Initialize (request, new)
XwFormWidget request, new;

{
   /*  Initialize the tree fields to NULL  */

   new -> form.width_tree = 
      XwGetFormRef (new, NULL, 0, False, False, True, False, 0, 0, 0);   
   new -> form.height_tree =
      XwGetFormRef (new, NULL, 0, False, False, True, False, 0, 0, 0);


   /*  Set up a geometry for the widget if it is currently 0.  */

   if (request -> core.width == 0)
      new -> core.width += 200;
   if (request -> core.height == 0)
      new -> core.height += 200;
}




/************************************************************************
 *
 *  ConstraintInitialize
 *     The main widget instance constraint initialization routine.
 *
 ************************************************************************/

static void ConstraintInitialize (request, new)
Widget request, new;

{
   XwFormConstraintRec * constraintRec;

   constraintRec = (XwFormConstraintRec *) new -> core.constraints;


   /*  Initialize the contraint widget sizes for later processing  */

   constraintRec -> set_x = 0;
   constraintRec -> set_y = 0;
   constraintRec -> set_width = 0;
   constraintRec -> set_height = 0;

   constraintRec -> x = new -> core.x;
   constraintRec -> y = new -> core.y;
   constraintRec -> width = new -> core.width;
   constraintRec -> height = new -> core.height;

   constraintRec -> managed = False;


   /*  Get and save copies of the names of the reference widgets  */

   GetRefWidget (&constraintRec -> x_ref_widget,
                 &constraintRec -> x_ref_name, new);
   GetRefWidget (&constraintRec -> y_ref_widget,
                 &constraintRec -> y_ref_name, new);

}




/************************************************************************
 *
 *  GetRefWidget
 *     Get and verify the reference widget given.
 *
 ************************************************************************/

static void GetRefWidget (widget, name, new)
Widget *  widget;
char   ** name;
Widget    new;
 
{   
   if (*widget != NULL)
   {
      if (*name != NULL)
      {
         if (strcmp (*name, (char *) ((*widget) -> core.name)) != 0)
         {
            XtWarning 
               ("Form: The reference widget and widget name do not match.");
            *name = (char *) (*widget) -> core.name;
         }
      }
      else
         *name = (char *) (*widget) -> core.name;

      if ((*widget) != new -> core.parent &&
          (*widget) -> core.parent != new -> core.parent)
      {
         XtWarning ("Form: The reference widget is not a child of the form");
         XtWarning ("      or the form widget.");
         *name = new -> core.parent -> core.name;
         *widget = new -> core.parent;
      }
   }

   else if (*name != NULL)
   {
      if ((*widget = XwFindWidget ((CompositeWidget)(new->core.parent), *name)) == NULL)
      {
	 XtWarning ("Form: The reference widget was not found.");
         *name = new -> core.parent -> core.name;
         *widget = new -> core.parent;
      }
   }
   else
   {
      *name = new -> core.parent -> core.name;
      *widget = new -> core.parent;
   }

   *name = strcpy (XtMalloc((unsigned) XwStrlen (*name) + 1), *name);
}




/************************************************************************
 *
 *  XwFindWidget
 *
 ************************************************************************/

static Widget XwFindWidget (w, name)
CompositeWidget w;
char   * name;

{
   register int i;
   register Widget * list;
   int count;

   if (strcmp (name, w -> core.name) == 0)
      return ((Widget) w);

   list = w -> composite.children;
   count = w -> composite.num_children;

   for (i = 0; i < count; i++)
   {
      if (strcmp (name, (*list) -> core.name) == 0)
         return (*list);
      list++;
   }
   return (NULL);
}




/************************************************************************
 *
 *  Realize
 *	Create the widget window and create the gc's.
 *
 ************************************************************************/

static void Realize (fw, valueMask, attributes)
XwFormWidget           fw;
XtValueMask          * valueMask;
XSetWindowAttributes * attributes;

{
   Mask newValueMask = *valueMask;
   XtCreateWindow ((Widget)fw, InputOutput, (Visual *) CopyFromParent,
		   newValueMask, attributes);
   _XwRegisterName(fw);
   XwProcessRefs (fw, False);
}




/************************************************************************
 *
 *  Resize
 *
 ************************************************************************/
		
static void Resize (fw)
XwFormWidget fw;

{
   if (XtIsRealized ((Widget)fw)) XwProcessRefs (fw, False);
}





/************************************************************************
 *
 *  Destroy
 *	Deallocate the head structures of the reference trees.
 *	The rest of the tree has already been deallocated.
 *
 ************************************************************************/

static void Destroy (fw)
XwFormWidget fw;

{
   XtFree ((char *)(fw -> form.width_tree));
   XtFree ((char *)(fw -> form.height_tree));
}




/************************************************************************
 *
 *  ConstraintDestroy
 *	Deallocate the allocated referenence names.
 *
 ************************************************************************/

static void ConstraintDestroy (w)
XwFormWidget w;

{
   XwFormConstraintRec * constraint;

   constraint = (XwFormConstraintRec *) w -> core.constraints;

   if (constraint -> x_ref_name != NULL) XtFree (constraint -> x_ref_name);
   if (constraint -> y_ref_name != NULL) XtFree (constraint -> y_ref_name);
}





/************************************************************************
 *
 *  SetValues
 *	Currently nothing needs to be done.  The XtSetValues call 
 *	handles geometry requests and form does not define any
 *	new resources.
 *
 ************************************************************************/

static Boolean SetValues (current, request, new)
XwFormWidget current, request, new;

{
   return (False);
}




/************************************************************************
 *
 *  ConstraintSetValues
 *	Process changes in the constraint set of a widget.
 *
 ************************************************************************/

static Boolean ConstraintSetValues (current, request, new)
Widget current, request, new;

{
   XwFormConstraintRec * curConstraint;
   XwFormConstraintRec * newConstraint;
   XwFormConstraintRec * tempConstraint;


   curConstraint = (XwFormConstraintRec *) current -> core.constraints;
   newConstraint = (XwFormConstraintRec *) new -> core.constraints;


   /*  Check the geometrys to see if new's contraint record  */
   /*  saved geometry data needs to be updated.              */

   if (XtIsRealized (current))
   {
      if (new -> core.x != current -> core.x)
         newConstraint -> set_x = new -> core.x;
      if (new -> core.y != current -> core.y)
         newConstraint -> set_y = new -> core.y;
      if (new -> core.width != current -> core.width)
         newConstraint -> set_width = new -> core.width;
      if (new -> core.height != current -> core.height)
         newConstraint -> set_height = new -> core.height;
   }


   /*  If the reference widget or name has changed, set the  */
   /*  opposing member to NULL in order to get the proper    */
   /*  referencing.  For names, the string space will be     */
   /*  deallocated out of current later.                     */

   if (newConstraint -> x_ref_widget != curConstraint -> x_ref_widget)
      newConstraint -> x_ref_name = NULL;
   else if (newConstraint -> x_ref_name != curConstraint -> x_ref_name)
      newConstraint -> x_ref_widget = NULL;

   if (newConstraint -> y_ref_widget != curConstraint -> y_ref_widget)
      newConstraint -> y_ref_name = NULL;
   else if (newConstraint -> y_ref_name != curConstraint -> y_ref_name)
      newConstraint -> y_ref_widget = NULL;


   /*  Get and save copies of the names of the reference widget names  */
   /*  and get the reference widgets.                                  */
   
   GetRefWidget (&newConstraint -> x_ref_widget,
                 &newConstraint -> x_ref_name, new);
   GetRefWidget (&newConstraint -> y_ref_widget,
                 &newConstraint -> y_ref_name, new);


   /*  See if the reference widgets have changed.  If so, free the  */
   /*  old allocated names and set them to the new names.           */

   if (newConstraint -> x_ref_widget != curConstraint -> x_ref_widget)
   {
      XtFree (curConstraint -> x_ref_name);
      curConstraint -> x_ref_name = newConstraint -> x_ref_name;
   }

   if (newConstraint -> y_ref_widget != curConstraint -> y_ref_widget)
   {
      XtFree (curConstraint -> y_ref_name);
      curConstraint -> y_ref_name = newConstraint -> y_ref_name;
   }


   /*  See if any constraint data for the widget has changed.  */
   /*  Is so, remove the old reference tree elements from the  */
   /*  forms constraint processing trees and build and insert  */
   /*  new reference tree elements.                            */
   /*                                                          */
   /*  Once this is finished, reprocess the constraint trees.  */

   if (newConstraint -> x_ref_widget != curConstraint -> x_ref_widget       ||
       newConstraint -> y_ref_widget != curConstraint -> y_ref_widget       ||

       newConstraint -> x_offset != curConstraint -> x_offset               ||
       newConstraint -> y_offset != curConstraint -> y_offset               ||

       newConstraint -> x_vary_offset != curConstraint -> x_vary_offset     ||
       newConstraint -> y_vary_offset != curConstraint -> y_vary_offset     ||

       newConstraint -> x_resizable != curConstraint -> x_resizable         ||
       newConstraint -> y_resizable != curConstraint -> y_resizable         ||

       newConstraint -> x_add_width != curConstraint -> x_add_width         ||
       newConstraint -> y_add_height != curConstraint -> y_add_height       ||

       newConstraint -> x_attach_right != curConstraint -> x_attach_right   ||
       newConstraint -> y_attach_bottom != curConstraint -> y_attach_bottom ||

       newConstraint -> x_attach_offset != curConstraint -> x_attach_offset ||
       newConstraint -> y_attach_offset != curConstraint -> y_attach_offset)
   {
      if (XtIsRealized (current) && current -> core.managed)
      {
         XwDestroyRefs (current);

         tempConstraint = (XwFormConstraintRec *) current -> core.constraints;
         current -> core.constraints = new -> core.constraints;
         XwMakeRefs (current);
         current -> core.constraints = (caddr_t) tempConstraint;
      }

      if (XtIsRealized (current)) XwProcessRefs (new -> core.parent, True);
   }

   return (False);
}




/************************************************************************
 *
 *  GeometryManager
 *      Always accept the childs new size, set the childs constraint
 *      record size to the new size and process the constraints.
 *
 ************************************************************************/

static XtGeometryResult GeometryManager (w, request, reply)
Widget w;
XtWidgetGeometry * request;
XtWidgetGeometry * reply;

{
   XwFormWidget fw = (XwFormWidget) w -> core.parent;
   XwFormConstraintRec * constraint;
   XwFormRef * xRef;
   XwFormRef * yRef;
   int newBorder = w -> core.border_width;
   Boolean moveFlag = False;
   Boolean resizeFlag = False;


   constraint = (XwFormConstraintRec *) w -> core.constraints;

   if (request -> request_mode & CWX)
      constraint -> set_x = request -> x;

   if (request -> request_mode & CWY)
      constraint -> set_y = request -> y;

   if (request -> request_mode & CWWidth)
      constraint -> set_width = request -> width;

   if (request -> request_mode & CWHeight)
      constraint -> set_height = request -> height;

   if (request -> request_mode & CWBorderWidth)
      newBorder = request -> border_width;


   /*  If the x or the width has changed, find the horizontal  */
   /*  reference tree structure for this widget and update it  */

   xRef = yRef = NULL;
   if  ((request->request_mode & CWWidth) || (request->request_mode & CWX))
   {
      if ((xRef = XwRefTreeSearch (w, fw -> form.width_tree)) != NULL)
      {
         if  (request->request_mode & CWX)
            xRef -> set_loc = request -> x;
         if  (request->request_mode & CWWidth)
            xRef -> set_size = request -> width;
      }
   }


   /*  If the y or the height has changed, find the vertical   */
   /*  reference tree structure for this widget and update it  */

   if  ((request->request_mode & CWHeight) || (request->request_mode & CWY))
   {
      if ((yRef = XwRefTreeSearch (w, fw -> form.height_tree)) != NULL)
      {
         if  (request->request_mode & CWY)
            yRef -> set_loc = request -> y;
         if  (request->request_mode & CWHeight)
            yRef -> set_size = request -> height;
      }
   }


   /*  Process the constraints if either of the ref structs have changed */

   if (xRef != NULL || yRef != NULL)
   {
      if ((request->request_mode & CWX) || (request->request_mode & CWY))
         moveFlag = True;
      if ((request->request_mode & CWWidth) ||
          (request->request_mode & CWHeight))
         resizeFlag = True;

      if (moveFlag && resizeFlag)
         XtConfigureWidget (w, constraint -> set_x, constraint -> set_y,
                            constraint -> set_width, constraint -> set_height,
                            newBorder);
      else if (resizeFlag)
         XtResizeWidget (w, constraint -> set_width, constraint -> set_height,
                            newBorder);
      else if (moveFlag)
         XtMoveWidget (w, constraint -> set_x, constraint -> set_y);


      XwProcessRefs (w -> core.parent, True);
   }


   /*  See if an almost condition should be returned  */

   if (((request->request_mode & CWX) && w->core.x != request->x) ||
       ((request->request_mode & CWY) && w->core.y != request->y) ||
       ((request->request_mode & CWWidth) && 
         w->core.width != request->width) ||
       ((request->request_mode & CWHeight) && 
         w->core.height != request->height))
   {
      reply->request_mode = request->request_mode;

      if (request->request_mode & CWX) reply->x = w->core.x;
      if (request->request_mode & CWY) reply->y = w->core.y;
      if (request->request_mode & CWWidth) reply->width = w->core.width;
      if (request->request_mode & CWHeight) reply->height = w->core.height;
      if (request->request_mode & CWBorderWidth)
         reply->border_width = request->border_width;
      if (request->request_mode & CWSibling)
         reply->sibling = request->sibling;
      if (request->request_mode & CWStackMode)
          reply->stack_mode = request->stack_mode;

      return (XtGeometryAlmost);
   }

   return (XtGeometryDone);
}




/************************************************************************
 *
 *  ChangeManaged
 *
 ************************************************************************/

static void ChangeManaged (fw)
XwFormWidget fw;

{
   Widget child;
   XwFormConstraintRec * constraint;
   register int i;


   /*  If the widget is being managed, build up the reference     */
   /*  structures for it, adjust any references, and process the  */
   /*  reference set.  If unmanaged, remove its reference.        */
   
   for (i = 0; i < fw -> composite.num_children; i++)
   {
      child = fw -> composite.children[i];
      constraint = (XwFormConstraintRec *) child -> core.constraints;

      if (constraint -> set_width == 0)
      {
         constraint -> set_x = child -> core.x;
         constraint -> set_y = child -> core.y;   
         constraint -> set_width = child -> core.width;
         constraint -> set_height = child -> core.height;
      }

      if (child -> core.managed != constraint -> managed)
      {
	 if (child -> core.managed)
         {
            if (constraint->width_when_unmanaged != child->core.width)
               constraint->set_width = child->core.width;
            if (constraint->height_when_unmanaged != child->core.height)
               constraint->set_height = child->core.height;
            XwMakeRefs (child);
         }
         else
         {
            constraint -> width_when_unmanaged = child->core.width;
            constraint -> height_when_unmanaged = child->core.height;
            XwDestroyRefs (child);
         }
         constraint -> managed = child -> core.managed;
      }
   }

   XwProcessRefs (fw, True);
}




/************************************************************************
 *
 *  XwMakeRefs
 *	Build up and insert into the forms reference trees the reference
 *      structures needed for the widget w.
 *
 ************************************************************************/

static void XwMakeRefs (w)
Widget w;

{      
   Widget xRefWidget;
   Widget yRefWidget;
   XwFormWidget formWidget;
   XwFormConstraintRec * constraint;
   XwFormRef * xRefParent;
   XwFormRef * yRefParent;
   XwFormRef * xRef;
   XwFormRef * yRef;
   XwFormRef * checkRef;
   register int i;


   formWidget = (XwFormWidget) w -> core.parent;
   constraint = (XwFormConstraintRec *) w -> core.constraints;


   /*  The "true" reference widget may be unmanaged, so  */
   /*  we need to back up through the reference set      */
   /*  perhaps all the way to Form.                      */
      
   xRefWidget = XwFindValidRef (constraint -> x_ref_widget, XwHORIZONTAL, 
                                formWidget -> form.width_tree);
   yRefWidget = XwFindValidRef (constraint -> y_ref_widget, XwVERTICAL,
                                formWidget -> form.height_tree);


   /*  Search the referencing trees for the referencing widgets  */
   /*  The constraint reference struct will be added as a child  */
   /*  of this struct.                                           */

   if (xRefWidget != NULL)
      xRefParent = XwRefTreeSearch (xRefWidget, formWidget -> form.width_tree);

   if (yRefWidget != NULL)
      yRefParent = XwRefTreeSearch (yRefWidget, formWidget->form.height_tree);
	    

   /*  Allocate, initialize, and insert the reference structures  */

   if (xRefWidget != NULL)
   {
      xRef = XwGetFormRef (w, xRefWidget, constraint->x_offset,
 		           constraint->x_add_width, constraint->x_vary_offset,
		           constraint->x_resizable, constraint->x_attach_right,
		           constraint->x_attach_offset,
  			   constraint->set_x, constraint->set_width);
      XwAddRef (xRefParent, xRef);
   }

   if (yRefWidget != NULL)
   {
      yRef = XwGetFormRef(w, yRefWidget, constraint->y_offset, 
		          constraint->y_add_height, constraint->y_vary_offset,
		          constraint->y_resizable, constraint->y_attach_bottom,
		          constraint->y_attach_offset,
			  constraint->set_y, constraint->set_height);
      XwAddRef (yRefParent, yRef);
   }


   /*  Search through the parents reference set to get any child  */
   /*  references which need to be made child references of the   */
   /*  widget just added.                                         */

   for (i = 0; i < xRefParent -> ref_to_count; i++)
   {
      checkRef = xRefParent -> ref_to[i];
      constraint = (XwFormConstraintRec *) checkRef->this->core.constraints;
	
      if (XwFindValidRef (constraint->x_ref_widget, XwHORIZONTAL,
                          formWidget -> form.width_tree) != xRefWidget)
      {
 	 XwRemoveRef (xRefParent, checkRef);
         checkRef -> ref = xRef -> this;
	 XwAddRef (xRef, checkRef);
      }
   }

   for (i = 0; i < yRefParent -> ref_to_count; i++)
   {
      checkRef = yRefParent -> ref_to[i];
      constraint = (XwFormConstraintRec *) checkRef->this->core.constraints;
	
      if (XwFindValidRef (constraint->y_ref_widget, XwVERTICAL,
                          formWidget -> form.height_tree) != yRefWidget)
      {
 	 XwRemoveRef (yRefParent, checkRef);
         checkRef -> ref = yRef -> this;
	 XwAddRef (yRef, checkRef);
      }
   }
}




/************************************************************************
 *
 *  XwDestroyRefs
 *	Remove and deallocate the reference structures for the widget w.
 *
 ************************************************************************/

static void XwDestroyRefs (w)
Widget w;

{      
   Widget xRefWidget;
   Widget yRefWidget;
   XwFormWidget formWidget;
   XwFormConstraintRec * constraint;
   XwFormRef * xRefParent;
   XwFormRef * yRefParent;
   XwFormRef * xRef;
   XwFormRef * yRef;
   XwFormRef * tempRef;
   register int i;


   formWidget = (XwFormWidget) w -> core.parent;
   constraint = (XwFormConstraintRec *) w -> core.constraints;


   /*  Search through the reference trees to see if the widget  */
   /*  is within the tree.                                      */

   xRefWidget = w;
   yRefWidget = w;

   xRefParent = 
      XwParentRefTreeSearch (xRefWidget, formWidget -> form.width_tree,
                                         formWidget -> form.width_tree);
   yRefParent = 
      XwParentRefTreeSearch (yRefWidget, formWidget -> form.height_tree,
                                          formWidget -> form.height_tree);


   /*  For both the width and height references, if the ref parent was  */
   /*  not null, find the reference to be removed within the parents    */
   /*  list, remove this reference.  Then, for any references attached  */
   /*  to the one just removed, reparent them to the parent reference.  */

   if (xRefParent != NULL)
   {
      for (i = 0; i < xRefParent -> ref_to_count; i++)
      {
         if (xRefParent -> ref_to[i] -> this == xRefWidget)
         {
             xRef = xRefParent -> ref_to[i];
	     break;
         }
      }

      XwRemoveRef (xRefParent, xRefParent -> ref_to[i]);

      while (xRef -> ref_to_count)
      {
         tempRef = xRef -> ref_to[0];
         tempRef -> ref = xRefParent -> this;
	 XwRemoveRef (xRef, tempRef);
	 XwAddRef (xRefParent, tempRef);
      }

      XtFree ((char *)xRef);   
   }

   if (yRefParent != NULL)
   {
      for (i = 0; i < yRefParent -> ref_to_count; i++)
      {
         if (yRefParent -> ref_to[i] -> this == yRefWidget)
         {
            yRef = yRefParent -> ref_to[i];
	    break;
	 }
      }

      XwRemoveRef (yRefParent, yRef);

      while (yRef -> ref_to_count)
      {
         tempRef = yRef -> ref_to[0];
         tempRef -> ref = yRefParent -> this;
	 XwRemoveRef (yRef, tempRef);
	 XwAddRef (yRefParent, tempRef);
      }

      XtFree ((char *)yRef);   
   }
}




/************************************************************************
 *
 *  XwGetFormRef
 *	Allocate and initialize a form constraint referencing structure.
 *
 ************************************************************************/

static XwFormRef *
XwGetFormRef (this, ref, offset, add, vary,
	      resizable, attach, attach_offset, loc, size)
Widget  this;
Widget  ref;
int     offset;
Boolean add;
Boolean vary;
Boolean resizable;
Boolean attach;
int     attach_offset;

{
   XwFormRef * formRef;

   formRef = (XwFormRef *) XtMalloc (sizeof (XwFormRef));
   formRef -> this = this;
   formRef -> ref = ref;
   formRef -> offset = offset;
   formRef -> add = add;
   formRef -> vary = vary;
   formRef -> resizable = resizable;
   formRef -> attach = attach;
   formRef -> attach_offset = attach_offset;

   formRef -> set_loc = loc;
   formRef -> set_size = size;

   formRef -> ref_to = NULL;
   formRef -> ref_to_count = 0;

   return (formRef);
}




/************************************************************************
 *
 *  XwFindValidRef
 *	Given an initial reference widget to be used as a constraint,
 *	find a valid (managed) reference widget.  This is done by
 *	backtracking through the widget references listed in the
 *	constraint records.  If no valid constraint is found, "form"
 *	is returned indicating that this reference should be stuck
 *	immediately under the form reference structure.
 *
 ************************************************************************/

static Widget XwFindValidRef (refWidget, refType, formRef)
Widget refWidget;
int    refType;
XwFormRef * formRef;

{
   XwFormConstraintRec * constraint;

   if (refWidget == NULL) return (NULL);
   
   while (1)
   {
      if (XwRefTreeSearch (refWidget, formRef) != NULL) return (refWidget);

      constraint = (XwFormConstraintRec *) refWidget -> core.constraints;

      if (refType == XwHORIZONTAL) refWidget = constraint -> x_ref_widget;
      else refWidget = constraint -> y_ref_widget;

      if (refWidget == NULL) return (refWidget -> core.parent);
   }
}




/************************************************************************
 *
 *  XwRefTreeSearch
 *	Search the reference tree until the widget listed is found.
 *
 ************************************************************************/

static XwFormRef * XwRefTreeSearch (w, formRef)
Widget w;
XwFormRef * formRef;

{
   register int i;
   XwFormRef * tempRef;

   if (formRef == NULL) return (NULL);
   if (formRef -> this == w) return (formRef);

   for (i = 0; i < formRef -> ref_to_count; i++)
   {
      tempRef = XwRefTreeSearch (w, formRef -> ref_to[i]);
      if (tempRef != NULL) return (tempRef);
   }

   return (NULL);
}




/************************************************************************
 *
 *  XwParentRefTreeSearch
 *	Search the reference tree until the parent reference of the 
 *      widget listed is found.
 *
 ************************************************************************/

static XwFormRef * XwParentRefTreeSearch (w, wFormRef, parentFormRef)
Widget w;
XwFormRef * wFormRef;
XwFormRef * parentFormRef;

{
   register int i;
   XwFormRef * tempRef;

   if (parentFormRef == NULL) return (NULL);
   if (wFormRef -> this == w) return (parentFormRef);

   for (i = 0; i < wFormRef -> ref_to_count; i++)
   {
      tempRef = 
         XwParentRefTreeSearch (w, wFormRef -> ref_to[i], wFormRef);
      if (tempRef != NULL) return (tempRef);
   }

   return (NULL);
}




/************************************************************************
 *
 *  XwAddRef
 *	Add a reference structure into a parent reference structure.
 *
 ************************************************************************/
      
static void XwAddRef (refParent, ref)
XwFormRef * refParent;
XwFormRef * ref;

{
   refParent -> ref_to =
      (XwFormRef **)
	 XtRealloc ((char *) refParent -> ref_to,
                    sizeof (XwFormRef *) * (refParent -> ref_to_count + 1));

   refParent -> ref_to[refParent -> ref_to_count] = ref;
   refParent -> ref_to_count += 1;
}




/************************************************************************
 *
 *  XwRemoveRef
 *	Remove a reference structure from a parent reference structure.
 *
 ************************************************************************/
      
static void XwRemoveRef (refParent, ref)
XwFormRef * refParent;
XwFormRef * ref;

{
   register int i, j;

   for (i = 0; i < refParent -> ref_to_count; i++)
   {
      if (refParent -> ref_to[i] == ref)
      {
	 for (j = i; j < refParent -> ref_to_count - 1; j++)
	    refParent -> ref_to[j] = refParent -> ref_to[j + 1];
         break;
      }
   }

   if (refParent -> ref_to_count > 1)
   {
      refParent -> ref_to =
         (XwFormRef **)	 XtRealloc ((char *) refParent -> ref_to,
         sizeof (XwFormRef *) * (refParent -> ref_to_count - 1));

   }
   else
   {
      XtFree ((char *)(refParent -> ref_to));
      refParent -> ref_to = NULL;
   }

   refParent -> ref_to_count -= 1;
}




/************************************************************************
 *
 *  XwProcessRefs
 *	Traverse throught the form's reference trees, calculate new
 *      child sizes and locations based on the constraints and adjust
 *      the children as is calculated.  The resizable flag indicates
 *      whether the form can be resized or not.
 *
 ************************************************************************/

static void XwProcessRefs (fw, formResizable)
XwFormWidget fw;
Boolean      formResizable;

{
   int formWidth, formHeight;
   register int i, j;

   int horDepth, horLeaves;
   int vertDepth, vertLeaves;
   XwFormProcess ** horProcessList;
   XwFormProcess ** vertProcessList;

   XtGeometryResult geometryReturn;
   Dimension replyW, replyH;

   XwFormConstraintRec * constraintRec;
   Widget child;
   Boolean moveFlag, resizeFlag;


   /*  Initialize the form width and height variables  */

   if (fw -> manager.layout == XwIGNORE) formResizable = False;

   if (formResizable) formWidth = formHeight = -1;
   else
   {
      formWidth = fw -> core.width;
      formHeight = fw -> core.height;
   }
      

   /*  Traverse the reference trees to find the depth and leaf node count  */

   leaves = 0;
   depth = 0;
   XwFindDepthAndCount (fw -> form.width_tree, 1);
   horDepth = depth;
   horLeaves = leaves;

   leaves = 0;
   depth = 0;
   XwFindDepthAndCount (fw -> form.height_tree, 1);
   vertDepth = depth;
   vertLeaves = leaves;

   if (horDepth == 0 && vertDepth == 0)
      return;


   /*  Allocate and initialize the constraint array processing structures  */

   horProcessList =
       (XwFormProcess **) XtMalloc (sizeof (XwFormProcess **) * horLeaves);
   for (i = 0; i < horLeaves; i++)
   {
      horProcessList[i] =
         (XwFormProcess *) XtMalloc (sizeof (XwFormProcess) * horDepth);

      for (j = 0; j < horDepth; j++)
         horProcessList[i][j].ref = NULL;
   }


   vertProcessList =
      (XwFormProcess **) XtMalloc (sizeof (XwFormProcess **) * vertLeaves);
   for (i = 0; i < vertLeaves; i++)
   {
      vertProcessList[i] =
         (XwFormProcess *) XtMalloc (sizeof (XwFormProcess) * vertDepth);
            
      for (j = 0; j < vertDepth; j++)
         vertProcessList[i][j].ref = NULL;
   }


   /*  Initialize the process array placing each node of the tree into    */
   /*  the array such that it is listed only once and its first children  */
   /*  listed directly next within the array.                             */

   arrayIndex = 0;
   XwInitProcessList (horProcessList, fw -> form.width_tree, 0);
   arrayIndex = 0;
   XwInitProcessList (vertProcessList, fw -> form.height_tree, 0);


   /*  Process each array such that each row of the arrays contain  */
   /*  their required sizes and locations to match the constraints  */

   XwConstrainList (horProcessList, horLeaves,
		    horDepth, &formWidth, formResizable, XwHORIZONTAL);
   XwConstrainList (vertProcessList, vertLeaves,
		    vertDepth, &formHeight, formResizable, XwVERTICAL);


   /*  If the form is resizable and the form width or height returned  */
   /*  is different from the current form width or height, then make   */
   /*  a geometry request to get the new form size.  If almost is      */
   /*  returned, use these sizes and reprocess the constrain lists     */
   
   if (formResizable &&
       (formWidth != fw -> core.width || formHeight != fw -> core.height))
   {
      geometryReturn =
	 XtMakeResizeRequest((Widget)fw, formWidth, formHeight, &replyW, &replyH);

      if (geometryReturn == XtGeometryAlmost)
      {
         formWidth = replyW;
         formHeight = replyH;

	 XtMakeResizeRequest((Widget)fw, formWidth, formHeight, NULL, NULL);

         XwConstrainList (horProcessList, horLeaves,
      		          horDepth, &formWidth, False, XwHORIZONTAL);
         XwConstrainList (vertProcessList, vertLeaves,
		          vertDepth, &formHeight, False, XwVERTICAL);
      }	 

      else if (geometryReturn == XtGeometryNo)
      {
         formWidth = fw -> core.width;
         formHeight = fw -> core.height;

         XwConstrainList (horProcessList, horLeaves,
      		          horDepth, &formWidth, False, XwHORIZONTAL);
         XwConstrainList (vertProcessList, vertLeaves,
		          vertDepth, &formHeight, False, XwVERTICAL);
      }
   }      
   

   /*  Process the forms child list to compare the widget sizes and  */
   /*  locations with the widgets current values and if changed,     */
   /*  reposition, resize, or reconfigure the child.                 */

   for (i = 0; i < fw -> composite.num_children; i++)
   {
      child = (Widget) fw -> composite.children[i];

      if (child -> core.managed)
      {
	 constraintRec = (XwFormConstraintRec *) child -> core.constraints;

         moveFlag = resizeFlag = False;

	 if (constraintRec -> x != child -> core.x ||
	     constraintRec -> y != child -> core.y)
	    moveFlag = True;

	 if (constraintRec -> width != child -> core.width ||
	     constraintRec -> height != child -> core.height)
	    resizeFlag = True;

         if (moveFlag && resizeFlag)
	    XtConfigureWidget (child, constraintRec->x, constraintRec->y,
                               constraintRec->width, constraintRec->height,
                               child -> core.border_width);
         else if (moveFlag)
	    XtMoveWidget (child, constraintRec->x, constraintRec->y);
         else if (resizeFlag)
	    XtResizeWidget (child, constraintRec->width,
                            constraintRec->height, child->core.border_width);
      }
   }

   XwFreeConstraintList (horProcessList, horLeaves);
   XwFreeConstraintList (vertProcessList, vertLeaves);
}




/************************************************************************
 *
 *  XwFreeConstraintList
 *	Free an allocated constraint list.
 *
 ************************************************************************/

static void XwFreeConstraintList (processList, leaves)
XwFormProcess ** processList;
int leaves;

{
   register int i;


   /*  Free each array attached to the list then free the list  */

   for (i = 0; i < leaves; i++)
      XtFree ((char *)(processList[i]));

   XtFree ((char *)processList);
}




/************************************************************************
 *
 *  XwFindDepthAndCount
 *	Search a constraint reference tree and find the maximum depth
 *      of the tree and the number of leaves in the tree.
 *
 ************************************************************************/

static void XwFindDepthAndCount (node, nodeLevel)
XwFormRef * node;
int         nodeLevel;

{
   register int i;

   if (node -> ref_to == NULL) leaves++;
   else
   {
      nodeLevel++;
      if (nodeLevel > depth) depth = nodeLevel;
      for (i = 0; i < node -> ref_to_count; i++)
	 XwFindDepthAndCount (node -> ref_to[i], nodeLevel);
   }
}




/************************************************************************
 *
 *  XwInitProcessList
 *	Search a constraint reference tree and find place the ref node
 *      pointers into the list.
 *
 ************************************************************************/

static void XwInitProcessList (processList, node, nodeLevel)
XwFormProcess ** processList;
XwFormRef      * node;
int              nodeLevel;

{
   register int i;

   processList[arrayIndex][nodeLevel].ref = node;

   if (node -> ref_to == NULL)
   {
      processList[arrayIndex][nodeLevel].leaf = True;
      arrayIndex++;
   }
   else
   {
      processList[arrayIndex][nodeLevel].leaf = False;
      nodeLevel++;
      for (i = 0; i < node -> ref_to_count; i++)
	 XwInitProcessList (processList, node -> ref_to[i], nodeLevel);
   }
}



/************************************************************************
 *
 *  XwConstrainList
 *	Process each array such that each row of the arrays contain
 *	their required sizes and locations to match the constraints
 *
 ************************************************************************/

static void XwConstrainList (processList, leaves, depth, 
			     formSize, varySize, orient)
XwFormProcess ** processList;
int              leaves;
int              depth;   
int            * formSize; 
Boolean          varySize;
int              orient;

{
   register int i, j;
   register XwFormRef  * ref;
   XwFormConstraintRec * constraint;
   XwFormConstraintRec * parentConstraint;
   int heldSize;
   int sizeDif;
   int vary, resize;
   int varyCount, varyAmount;
   int resizeCount, resizeAmount;
   int constantSubtract;
   int addAmount, subtractAmount;
   int size, separation;



   heldSize = 0;


   for (i = 0; i < leaves; i++)		/* Process all array lines  */
   {
      processList[i][0].size = 0;
      processList[i][0].loc = 0;


      for (j = 1; j < depth; j++)	/* Process array line */
      {
         ref = processList[i][j].ref;

	 if (ref != NULL)
	 {
            processList[i][j].size = ref -> set_size;

            if (ref -> ref == ref -> this -> core.parent)
            {
	       if (ref -> offset != 0)
                  processList[i][j].loc = ref -> offset;
	       else
		  processList[i][j].loc = ref -> set_loc;
            }
	    else
	    {
	       processList[i][j].loc = 
                  processList[i][j - 1].loc + ref->offset;
	       if (ref -> add)
		  processList[i][j].loc += processList[i][j - 1].size +
                  processList[i][j].ref -> this -> core.border_width * 2;
	    }
              
	 }
         else
	 {
	    processList[i][j].ref = processList[i - 1][j].ref;
	    processList[i][j].loc = processList[i - 1][j].loc;
	    processList[i][j].size = processList[i - 1][j].size;
	    processList[i][j].leaf = processList[i - 1][j].leaf;
	 }

         if (processList[i][j].leaf)
         {
            if (processList[i][0].size < processList[i][j].size +
                processList[i][j].ref -> this -> core.border_width * 2 +
                processList[i][j].loc + ref -> attach_offset)
            {
	       processList[i][0].size = processList[i][j].size + 
                  processList[i][j].ref -> this -> core.border_width * 2 +
                  processList[i][j].loc + ref -> attach_offset;
            }

            if (processList[i][j].leaf && processList[i][0].size > heldSize)
               heldSize = processList[i][0].size;

  	    break;
         }
      }
   }


   /*  Each array line has now been processed to optimal size.  Reprocess  */
   /*  each line to constrain it to formSize if not varySize or to         */
   /*  heldSize if varySize.                                               */

   if (varySize)
      *formSize = heldSize;


      
   for (i = 0; i < leaves; i++)
   {

      /*  For each array line if the 0th size (calculated form size needed  */
      /*  for this array line is less than the form size then increase the  */
      /*  seperations between widgets whose constaints allow it.            */
      /*  If can't do it by varying separation, but can do it by resizing,  */
      /*  then do that. 						    */

      if (processList[i][0].size < *formSize)
      {
	 sizeDif = *formSize - processList[i][0].size;
	 
         varyCount = 0;
	 resizeCount = 0;
	 for (j = 1; j < depth; j++)	
	 {
	     /*  Can't vary the first spacing  */
	     if (j > 1 && processList[i][j].ref -> vary) varyCount++;
	     if (processList[i][j].ref -> resizable) resizeCount++;
	     if (processList[i][j].leaf) break;
	 }
	       
         addAmount = 0;
	 resizeAmount = 0;
         if (varyCount == 0)
	 {
	     varyAmount = 0;
	     if (resizeCount != 0)
	        resizeAmount = sizeDif / resizeCount;
	 }
         else varyAmount = sizeDif / varyCount;

         j = 1;

         while (j < depth)
         {
            if (j > 1 && processList[i][j].ref -> vary)
	       addAmount += varyAmount;
            processList[i][j].loc += addAmount;
	    if (processList[i][j].ref -> resizable)
	    {
	       processList[i][j].size += resizeAmount;
	       addAmount += resizeAmount;
	    }

            if (processList[i][j].leaf) break;

            j++;
	 }

         if (j > 1)
         {
            if (processList[i][j].ref -> vary   &&
                processList[i][j].ref -> attach)
               processList[i][j].loc = *formSize - processList[i][j].size -
                                       processList[i][j].ref -> attach_offset;
            else if (processList[i][j].ref -> vary == False &&
                     processList[i][j].ref -> resizable     &&
                     processList[i][j].ref -> attach)
            {
               processList[i][j].size = 
                  *formSize - processList[i][j].loc -
                  processList[i][j].ref -> this -> core.border_width * 2 -
                  processList[i][j].ref -> attach_offset;
            }
         }
         else
         {
            if (processList[i][j].ref -> vary == False &&
                processList[i][j].ref -> resizable     &&
                processList[i][j].ref -> attach)
            {
               processList[i][j].loc = processList[i][j].ref -> offset;
               processList[i][j].size = 
                  *formSize - processList[i][j].loc -
                  processList[i][j].ref -> this -> core.border_width * 2 -
                  processList[i][j].ref -> attach_offset;

            }
            else if (processList[i][j].ref -> vary &&
                     processList[i][j].ref -> attach)
            {
               processList[i][j].loc = 
                  *formSize - processList[i][j].size -
                  processList[i][j].ref -> this -> core.border_width * 2 -
                  processList[i][j].ref -> attach_offset;
            }
            else if (processList[i][j].ref -> vary &&
                     processList[i][j].ref -> attach == False)
            {
               processList[i][j].loc = processList[i][j].ref -> offset;
            }
         }
      }


      /*  If the form size has gotton smaller, process the vary constraints */
      /*  until the needed size is correct or all seperations are 1 pixel.  */
      /*  If separations go to 1, then process the resizable widgets        */
      /*  until the needed size is correct or the sizes have gone to 1      */
      /*  pixel.  If the size is still not correct punt, cannot find a      */
      /*  usable size so clip it.                                           */

      if (processList[i][0].size > *formSize)
      {
	 sizeDif = processList[i][0].size - *formSize;
	 
         varyAmount = 0;
         varyCount = 0;

         j = 0;
         do
         {
            j++;

            if (j > 1 && processList[i][j].ref -> vary && 
                processList[i][j].ref -> offset)
            {
               varyAmount += processList[i][j].ref -> offset;
   	       varyCount++;
            }
  	 }
         while (processList[i][j].leaf == False);


	 resizeAmount = 0;
	 resizeCount = 0;
         for (j = 1; j < depth; j++)
         {
            if (processList[i][j].ref->resizable && processList[i][j].size > 1)
	    {
               if (processList[i][j].leaf || processList[i][j+1].ref->add)
               {
                  resizeCount++;
                  resizeAmount += processList[i][j].size - 1;
               }
            }
            if (processList[i][j].leaf) break;
	 }


         /*  Do we have enough varience to match the constraints?  */

         if (varyAmount + resizeAmount > sizeDif) 
         {

            /*  first process out the vary amount  */

            if (varyCount)
            {	    
   	       do
               {
                  subtractAmount = 0;

                  for (j = 1; j < depth; j++)
                  {
                     if (j > 1 && processList[i][j].ref -> vary)
		     {
                        vary = 
                           processList[i][j].loc - processList[i][j - 1].loc;

                        if (processList[i][j].ref -> add)
			   vary = vary - processList[i][j - 1].size - 1 -
                                  processList[i][j-1].ref->
                                  this->core.border_width * 2;
                     }
                     else vary = 0;

                     if (vary > 0) subtractAmount++;

                     if (subtractAmount)
                     {
                        processList[i][j].loc -= subtractAmount;
                        sizeDif--;
                     }

                     if (processList[i][j].leaf) break;
		  }
               }
               while (subtractAmount != 0 && sizeDif > 0);
	    }
		  

            /*  now process resize constraints if further constraint */
	    /*  processing is necessary.                             */

            if (sizeDif)
	    {
               if (resizeAmount > sizeDif) resizeAmount = sizeDif;
	       if (resizeCount) constantSubtract = resizeAmount / resizeCount;

   	       while (resizeAmount > 0)
               {
                  subtractAmount = 0;

                  for (j = 1; j < depth; j++)
                  {
                     if (processList[i][j].ref -> add)
                        processList[i][j].loc -= subtractAmount;

                     if (processList[i][j].ref -> resizable)
                     {
                        if (processList[i][j].leaf || 
                            processList[i][j + 1].ref -> add)
                           resize = processList[i][j].size - 1;
                        else
                           resize = 0;
		     
                        if (resize > 1)
                        {
                           if (constantSubtract < resize)
			      resize = constantSubtract;
                           subtractAmount += resize;
                           processList[i][j].size -= resize;
			}
		     }

                     if (processList[i][j].leaf) break;
		  }

                  resizeAmount -= subtractAmount;
                  constantSubtract = 1;
	       }
	    }
	 }
      }
   }



   /*  Now each array line is processed such that its line is properly  */
   /*  constrained to match the specified form size.  Since a single    */
   /*  widget reference structure can be referenced in multiple array   */
   /*  lines, the minumum constraint for each widget needs to be found. */
   /*  When found, the width and height will be placed into the widgets */
   /*  constraint structure.                                            */

   for (i = 1; i < depth; i++)
   {
      ref = NULL;

      for (j = 0; j < leaves + 1; j++)    /*  loop one to many - for exit  */
      {
	 if (j == leaves || ref != processList[j][i].ref)
	 {
            if (j == leaves || ref != NULL)
	    {
               if (ref != NULL)
               {
                  constraint = 
                     (XwFormConstraintRec *) ref -> this -> core.constraints;
                  parentConstraint =
                     (XwFormConstraintRec *) ref -> ref -> core.constraints;

                  if (orient == XwHORIZONTAL) constraint -> width = size;
                  else constraint -> height = size;

  	          if (i > 1)
                  {
		     if (orient== XwHORIZONTAL)
   		        constraint -> x = parentConstraint -> x + separation;
                     else
		        constraint -> y = parentConstraint -> y + separation;
	          }
	          else
	          {
		     if (orient == XwHORIZONTAL) constraint -> x = separation;
		     else constraint -> y = separation;
	          }
               }

               if (j == leaves) break;	/* exit out of the inner loop  */
	    }
	    
            ref = processList[j][i].ref;
            separation = 10000000;
            size = 10000000;
	 }

         if (ref != NULL)
         {
            if (size > processList[j][i].size) size = processList[j][i].size;
	 
            if (i > 1)
            {
               if (separation > processList[j][i].loc-processList[j][i-1].loc)
                  separation = processList[j][i].loc-processList[j][i-1].loc;
            }
            else
               if (separation > processList[j][i].loc)
	           separation = processList[j][i].loc;
         }
      }
   }
}
