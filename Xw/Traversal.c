/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:       Traversal.c
 **
 **   Project:     X Widgets
 **
 **   Description: Code to support a generalized traversal handler for
 **                subclasses of XwManager.
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

#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/StringDefs.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <stdlib.h>   /* abs */


static int Overlap();
static void MoveNext();
static void MovePrev();
static void MoveTraversal();
static void Start();
static void Home();
static void TraverseToChild ();
static void ClearKbdFocus();
static Widget RightCheck();
static Widget LeftCheck();
static Widget UpCheck();
static Widget DownCheck();
static Widget ValidChoice();
static Boolean _XwManagerIsTraversable();


/* Structures used to maintain the traversal checklist */

typedef enum {
   XwVERIFY_ONLY,
   XwADD_TO_CHOSEN_LIST,
   XwADD_TO_ACTIVE_LIST,
   XwCLEAR_ACTIVE_LIST
} XwValidChoiceActions;

typedef enum {
   XwNONE,
   XwCHOSEN,
   XwACTIVE
} XwEntryType;

typedef struct {
   Widget widget;
   XwEntryType type;
} _XwCheckListEntry;


/*
 * Globals used to maintain a list of those widgets already checked
 * during a traversal request.
 */

_XwCheckListEntry * _XwCheckList = NULL;
int                 _XwCheckListSize = 0;
int                 _XwCheckListIndex = 0;

/*
 
A manager's traversal handler will be invoked with:

    Upper Left Point:  (x1, y1)
    Lower Right Point: (x2, y2)
    Direction:         XwTRAVERSE_CURRENT
                       XwTRAVERSE_LEFT
                       XwTRAVERSE_RIGHT
                       XwTRAVERSE_UP
                       XwTRAVERSE_DOWN
                       XwTRAVERSE_NEXT
                       XwTRAVERSE_PREV
                       XwTRAVERSE_HOME
                       XwTRAVERSE_NEXT_TOP




A manager has an active child which may be NULL.  Widgets under
consideration to receive the keyboard focus are called candidate widgets.
In order to be a candidate, a widget must be traversable.  This means
that it must be:

      1) a subclass of primitive, and
              sensitive (it and its ancestor)
              visible
              managed
              have its "mapped_when_managed" flag set to TRUE, and
              traversal_type=XwHIGHLIGHT_TRAVERSAL

      2) a composite with a traversable primitive




Step 1:  See if the manager contains any children which are traversable.
         If not, set the "active_child" to NULL and 
            if the parent of this manager is a subclass of manager AND
            that parent has traversal on, then invoke its traversal
            handling procedure directly, otherwise return focus to
            the root.

            <<<<NOTE TEST WHETHER THIS IS THE CORRECT THING TO DO, WE
                MAY NEED 2 DIFFERENT ACTIONS DEPENDING ON THE FOCUS MODEL
                IN EFFECT.>>>>

         If we do have someone who can accept the focus, then continue.


Step 2:  The caller of this procedure supplies POINT1, POINT2 and the
         DIRECTION.  Pass control to the specified direction handler.

*/



/**********************************/
/* Globals for Traversal Routines */
/**********************************/
static int widget_type;   
static   XwManagerWidget parent;
static   Boolean parent_is_manager;

void mgr_traversal (mgr, ul, lr, direction)

   XwManagerWidget mgr;
   XPoint ul, lr;
   int direction;

{
   XPoint newUl, newLr;   

   parent = (XwManagerWidget)mgr->core.parent;
   parent_is_manager = XtIsSubclass((Widget)parent,XwmanagerWidgetClass);

   if (direction == XwTRAVERSE_NEXT_TOP)
     {
       if (parent_is_manager)
       {
            newUl.x = newUl.y = newLr.x = newLr.y = 0;

            (*(((XwManagerWidgetClass)(parent->core.widget_class)) ->
		  manager_class.traversal_handler))
 		     ((Widget)parent, newUl, newLr, direction);
       }
       else
       {
	    XtCallCallbacks((Widget)mgr, XtNnextTop, NULL);
       }
       return;
     }


   if ((_XwFindTraversablePrim (mgr) == FALSE) ||
       (!_XwManagerIsTraversable(mgr)))
      {
       if (mgr->manager.active_child != NULL)
  	      mgr->manager.active_child = (Widget)NULL;


	 if (parent_is_manager && parent->manager.traversal_on)
	 {
               newUl.x = ul.x + mgr->core.x;
               newUl.y = ul.y + mgr->core.y;
               newLr.x = lr.x + mgr->core.x;
               newLr.y = lr.y + mgr->core.y;
	       (*(((XwManagerWidgetClass)(parent->core.widget_class)) ->
		  manager_class.traversal_handler))
 		     ((Widget)parent, newUl, newLr, direction);
	 }
         else
            ClearKbdFocus((Widget) mgr);
  	 return;
      }
  
   /************************************
    *
    * THERE IS A CHILD TO TRAVERSE TO,
    * DETERMINE DIRECTION & DO TRAVERSAL
    *
    ************************************/
    switch(direction)
       {
	case XwTRAVERSE_NEXT:
	  MoveNext((XwManagerWidget)mgr,ul, lr, direction); return;
	case XwTRAVERSE_PREV:
	  MovePrev((XwManagerWidget)mgr,ul, lr, direction); return;
	case XwTRAVERSE_DOWN:
	case XwTRAVERSE_UP:
	case XwTRAVERSE_RIGHT:
	case XwTRAVERSE_LEFT:
	  MoveTraversal((XwManagerWidget)mgr,ul, lr, direction); return;
	case XwTRAVERSE_CURRENT:
	  Start((XwManagerWidget)mgr, ul, lr ,direction);  return;
	case XwTRAVERSE_HOME:
	  Home((XwManagerWidget)mgr, ul, lr ,direction);  return;
       }
}

/*

DIRECTION = RIGHT  (All other directions are just variations of this)
=================

Step 1) Find the widget to the right of the active child. 

If widget A is the current active child and its upper left and lower right
points are defined by (x1,y1) and (x2,y2) respectively; and widget B is
a candidate widget whose upper left and lower right points are defined by
(x1',y1') and (x2',y2').  Then widget B is to the right of widget A

             IFF  ((x1 < x1') OR (if (x1 = x1') then (x2' > x2))
                  AND (y1 <= y2') AND (y2 >= y1')

If there is more than 1 candidate then choose the candidate with the 
smallest value for:

             (x1' - x1)

If 2 or more candidates have the same value for (x1'-x1) then choose the
candidate with the smallest value for:

             |(x2' - x2)|

If 2 or more candidates have the same value for |(x2'-x2)| then choose
the candidate with the largest overlap of widget A along its y dimension:


         if (y2' >= y2)
           then
              if (y1' <= y1)
                then 
                   overlap = height of active child
                else 
                   overlap = y2 - y1'
           else
              if (y1' <= y1)
               then
                  overlap = y2' - y1
               else
                  overlap = height of candidate

If 2 or more candidates have the same overlap then choose the widget with
the smallest value for y1'.

If 2 or more candidates have the same value for y1' then choose the widget with
the smallest value for y2'.

It should not be possible for 2 candidates to get this far since it would 
mean that one exactly overlaps the other, which should mean that the
visibility flag of the obscured widget should be FALSE (thus making it
un-traversable), however, if we do get this far, arbitrarily choose the
first candidate.


Step 2) If no widget is to the right of the active child then:

    if widget A's grandparent is a subclass of XwManager 
        then
           -set manager's active child field to NULL
           -invoke grandparent's traversal handler with:

              Upper Left Point: x1=x1+parent's x coordinate
                                y1=y1+parent's y coordinate


              Lower Right Point: x2=x2+parent's x coordinate
                                 y2=y2+parent's y coordinate

              Direction = XwTRAVERSE_RIGHT



     if widget A's grandparent is NOT a subclass of XwManager
        then
          -set manager's active child field to NULL
          -invoke parent's traversal handler with:

               Upper Left Point: x1=0, y1=y1;

               Lower Right Point: x2=0, y2=y2;

               Direction = XwTRAVERSE_RIGHT


Step 3) If a candidate has been chosen, then set it as the active child of 
        its parent and:

          if it is a subclass of XwPrimitive
            then
              -give it keyboard focus using XSetInputFocus function.


            else (its a manager subclass)
              -invoke its traversal handler with:
      
                   Upper Left Point: x1=x1 - x1';  y1=y1 - y1'

                   Lower Right Point: x2=x2 - x2'; y2=y2 - y2'

                   Direction = XwTRAVERSE_RIGHT
                        
*/

static void MoveTraversal (mw, ul, lr, direction)

  XwManagerWidget mw;
  XPoint ul, lr;
  int direction;
{
  Widget candidate, chosen;
  int chosen_widget_type;
  int i=(-1);
  XPoint cand_ul, cand_lr;

  chosen = NULL;

  while (++i < mw->composite.num_children)
  {
      candidate = mw->composite.children[i];
      if ((candidate != mw->manager.active_child) && 
              XwTestTraversability(candidate, &widget_type))
      {
         cand_ul.x = candidate->core.x;
         cand_ul.y = candidate->core.y;
         cand_lr.x = candidate->core.x + candidate->core.width +
                     (candidate->core.border_width << 1) - 1;
         cand_lr.y = candidate->core.y + candidate->core.height +
                     (candidate->core.border_width << 1) - 1;
          
         switch (direction)
         {
            case XwTRAVERSE_RIGHT:
               chosen=RightCheck (chosen, ul, lr, candidate, cand_ul, cand_lr);
               break;

            case XwTRAVERSE_LEFT:
               chosen = LeftCheck (chosen, ul, lr, candidate, cand_ul, cand_lr);
               break;

            case XwTRAVERSE_UP:
               chosen = UpCheck (chosen, ul, lr, candidate, cand_ul, cand_lr);
               break;

            case XwTRAVERSE_DOWN:
               chosen = DownCheck (chosen, ul, lr, candidate, cand_ul, cand_lr);
               break;

         }
      }

      if (chosen == candidate) chosen_widget_type = widget_type;
   }

   if (chosen == NULL)  /* no widget in specified direction! */
   { 
      /*
       * Since our parent is a manager, see if there is another sibling
       * of ours which can be traversed to.
       */
      mw->manager.active_child = NULL;
      if (parent_is_manager)  /* really grandparent of active child */
      {
         /* Add ourselves to the active list */
         ValidChoice (mw, mw, XwADD_TO_ACTIVE_LIST);

         ul.x +=  mw->core.x;
         ul.y +=  mw->core.y;
         lr.x +=  mw->core.x;
         lr.y +=  mw->core.y;
	 (*(((XwManagerWidgetClass)(parent->core.widget_class)) ->
	   manager_class.traversal_handler))
 	    ((Widget)parent, ul, lr, direction);
      }
      else
      {
         /* Wrap to opposite edge */

         /* Clear the active traversal list */
         ValidChoice (NULL, NULL, XwCLEAR_ACTIVE_LIST);

         switch (direction)
         {
            case XwTRAVERSE_RIGHT:
            {
               ul.x = -10000;
               lr.x = -10000;
               break;
            }

            case XwTRAVERSE_LEFT:
            {
               ul.x = mw->core.width + (mw->core.border_width << 1);
               lr.x =  mw->core.width + (mw->core.border_width << 1);
               break;
            }

            case XwTRAVERSE_UP:
            {
               ul.y = mw->core.height + (mw->core.border_width << 1);
               lr.y =  mw->core.height + (mw->core.border_width << 1);
               break;
            }

            case XwTRAVERSE_DOWN:
            {
               ul.y = -10000;
               lr.y = -10000;
               break;
            }
         }

	 (*(((XwManagerWidgetClass)(mw->core.widget_class)) ->
	    manager_class.traversal_handler))
 	      ((Widget)mw, ul, lr, direction);
      }
   }
   else
   {
      ValidChoice(chosen, chosen, XwADD_TO_CHOSEN_LIST);
      TraverseToChild(mw, chosen, ul, lr, direction);      
   }
}
          


static void Start (mw, ul, lr, direction)
  XwManagerWidget mw;
  XPoint ul, lr;
  int direction;
{
  Widget candidate, chosen;
  int i=0;
  
   chosen = NULL;

  /* To prevent endless recursion, see if the child has already been checked */
  if ((mw->manager.active_child != NULL) &&
      (ValidChoice(NULL, mw->manager.active_child, XwVERIFY_ONLY)))
    {
      if (XwTestTraversability(mw->manager.active_child, &widget_type))
      {
         ValidChoice(chosen, chosen, XwADD_TO_CHOSEN_LIST);
         TraverseToChild (mw, mw->manager.active_child, ul, lr, direction);
      }
    }
   else
     {
        while (i < mw->composite.num_children)
	  {
            candidate = mw->composite.children[i];
            if (XwTestTraversability(candidate, &widget_type))
	      {
                if (chosen==NULL) 
                   chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
                else

		  {
                    if ((candidate->core.x+candidate->core.y) <
                              (chosen->core.x+chosen->core.y))
                            chosen = ValidChoice(chosen, candidate, 
                                                 XwVERIFY_ONLY);
                    
		  }
	      }
          i++;
	  }

      ValidChoice(chosen, chosen, XwADD_TO_CHOSEN_LIST);
      TraverseToChild(mw, chosen, ul, lr, direction);      

     }
}



/* We are guaranteed that there is a child to traverse to! */

static void MoveNext (mw, ul, lr, direction)
  XwManagerWidget mw;
  XPoint ul, lr;
  int direction;
{
  int i=0;

   parent = (XwManagerWidget)mw->core.parent;
   parent_is_manager = XtIsSubclass((Widget)parent,XwmanagerWidgetClass);

  if (mw->manager.active_child != NULL)
    {
      ul.x = (mw->manager.active_child)->core.x;
      ul.y = (mw->manager.active_child)->core.y;
      lr.x = ul.x + (mw->manager.active_child)->core.width +
             ((mw->manager.active_child)->core.border_width << 1) - 1;
      lr.y = ul.y + (mw->manager.active_child)->core.height +
             ((mw->manager.active_child)->core.border_width << 1) - 1;

      /* FIND INDEX OF CURRENTLY ACTIVE CHILD */
      while ((mw->composite.children[i] != mw->manager.active_child) &&
               (i < mw->composite.num_children))i++;
    }
   else
     /* START LOOKING AT START OF LIST + 1 (TO ALLOW FOR 1ST DECREMENT)
      * NOTE: THIS ALSO TAKES CARE OF THE DEGENERATE CASE WHEN THERE
      * ARE NO CHILDREN.  IN THIS CASE THE PARENT "MUST" BE A SUBCLASS
      * OF XWMANAGER OR WE WOULD NEVER HAVE GOTTEN HERE.
      */
     i = -1;

   /* 
    * IF WE'VE COME TO THE END OF THE WIDGET LIST AND THE PARENT
    * IS ALSO A SUBCLASS OF XWMANAGER, THEN INVOKE THE PARENT'S
    * TRAVERSAL HANDLER, OTHERWISE, WRAP THE COUNTER TO START LOOKING
    * AT THE LIST AGAIN.
    */
   if (++i >= mw->composite.num_children) 
     {
      if (parent_is_manager)
	{ 
          mw->manager.active_child = NULL;
          (*(((XwManagerWidgetClass)(parent->core.widget_class)) ->
	      manager_class.traversal_handler))
 		     ((Widget)parent, ul, lr, direction);
          return;
         }
       else
         i=0; 
       }

   /* FIND NEXT TRAVERSABLE WIDGET --THERE MUST BE ONE! */
   while (! XwTestTraversability(mw->composite.children[i], &widget_type))
            if (++i >= mw->composite.num_children) i=0; 

   TraverseToChild(mw, mw->composite.children[i], ul, lr, direction);
}




/* We are guaranteed that there is a child to traverse to! */

static void MovePrev (mw, ul, lr, direction)
  XwManagerWidget mw;
  XPoint ul, lr;
  int direction;
{
  int i;

   parent = (XwManagerWidget)mw->core.parent;
   parent_is_manager = XtIsSubclass((Widget)parent,XwmanagerWidgetClass);

   if (mw->manager.active_child != NULL)
    {
      ul.x = (mw->manager.active_child)->core.x;
      ul.y = (mw->manager.active_child)->core.y;
      lr.x = ul.x + (mw->manager.active_child)->core.width +
             ((mw->manager.active_child)->core.border_width << 1) - 1;
      lr.y = ul.y + (mw->manager.active_child)->core.height +
             ((mw->manager.active_child)->core.border_width << 1) - 1;

      /* FIND INDEX OF CURRENTLY ACTIVE CHILD */
      i=mw->composite.num_children -1;
      while ((mw->composite.children[i] != mw->manager.active_child) &&
               (i > 0))i--;
    }
   else
     /* START LOOKING AT END OF LIST + 1 (TO ALLOW FOR 1ST DECREMENT)
      * NOTE: THIS ALSO TAKES CARE OF THE DEGENERATE CASE WHEN THERE
      * ARE NO CHILDREN.  IN THIS CASE THE PARENT "MUST" BE A SUBCLASS
      * OF XWMANAGER OR WE WOULD NEVER HAVE GOTTEN HERE.
      */
     i = mw->composite.num_children;   


   /* 
    * IF WE'VE COME TO THE END OF THE WIDGET LIST AND THE PARENT
    * IS ALSO A SUBCLASS OF XWMANAGER, THEN INVOKE THE PARENT'S
    * TRAVERSAL HANDLER, OTHERWISE, WRAP THE COUNTER TO START LOOKING
    * AT THE LIST AGAIN.
    */
   if (--i < 0)
     {
      if (parent_is_manager)
	{ 
          mw->manager.active_child = NULL;
          (*(((XwManagerWidgetClass)(parent->core.widget_class)) ->
	      manager_class.traversal_handler))
 		     ((Widget)parent, ul, lr, direction);
          return;
         }
       else
         i = (mw->composite.num_children)-1;
       }


   /* FIND NEXT TRAVERSABLE WIDGET --THERE MUST BE ONE! */
   while (! XwTestTraversability(mw->composite.children[i], &widget_type))
        if (--i < 0) i = (mw->composite.num_children)-1;

   TraverseToChild (mw, mw->composite.children[i], ul, lr, direction);
}






static int Overlap (pt1, pt2, w)
  XPoint pt1, pt2;
  Widget w;
{
  XPoint pt1_prime, pt2_prime;

  pt1_prime.x = w->core.x;
  pt1_prime.y = w->core.y;
  pt2_prime.x = w->core.x + w->core.width + (w->core.border_width << 1) - 1;
  pt2_prime.y = w->core.y + w->core.height + (w->core.border_width << 1) - 1;

  if (pt2_prime.y >= pt2.y)
  {
   if (pt1_prime.y <= pt1.y) 
    return(pt2.y - pt1.y);
   else 
    return(pt2.y - pt1_prime.y);
  }
  else
  {
   if (pt1_prime.y <= pt1.y)
    return(pt2_prime.y - pt1.y);
   else
    return(w->core.height);
  }
}


/* CALCULATE OVERLAP IN X DIRECTION */
static int xOverlap (pt1, pt2, w)
  XPoint pt1, pt2;
  Widget w;
{
  XPoint pt1_prime, pt2_prime;

  pt1_prime.x = w->core.x;
  pt1_prime.y = w->core.y;
  pt2_prime.x = w->core.x + w->core.width + (w->core.border_width << 1) - 1;
  pt2_prime.y = w->core.y + w->core.height + (w->core.border_width << 1) - 1;

  if (pt2_prime.x >= pt2.x)
  {
   if (pt1_prime.x <= pt1.x) 
    return(pt2.x - pt1.x);
   else 
    return(pt2.x - pt1_prime.x);
  }
  else
  {
   if (pt1_prime.x <= pt1.x)
    return(pt2_prime.x - pt1.x);
   else
    return(w->core.width);
  }
}


/*    
 * HOME:  If I get called there is NO guarantee that there is something
 *        which is traversable.  This is due to the way that X generates
 *        its visiblity events; the manager is notified before its children.
 *        Thus, the manager may still think it has traversable children,
 *        when in fact it doesn't.  That is why we must have a special
 *        check to see that a child really was chosen.
 */

static void Home (mw, ul, lr, direction)
  XwManagerWidget mw;
  XPoint ul, lr;
  int direction;
{
  Widget candidate, chosen;
  int chosen_widget_type;
  int i=(-1);
  
   chosen = NULL;

 /* 
    search thru list of children to find child which is closest to
    origin and which is traversable.
 */
  while (++i < mw->composite.num_children)
   {
     candidate = mw->composite.children[i];
     if (XwTestTraversability(candidate, &widget_type))
      {
        if (chosen==NULL) 
           chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
        else
         {
           if ((candidate->core.x+candidate->core.y) <
                (chosen->core.x+chosen->core.y))
                   chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
          }
       }
      if (chosen == candidate) chosen_widget_type = widget_type;
     }

   /* 
    * If the closest to home widget is NOT the current active child then
    * make it the active child.  Otherwise, try and go up a level.
    */

   if (mw->manager.active_child != chosen)
       {
          mw->manager.active_child=chosen;
          ValidChoice(chosen, chosen, XwADD_TO_CHOSEN_LIST);

          if (chosen_widget_type == XwPRIMITIVE)

            TraverseToChild(mw, chosen, ul, lr, direction);
     
          else
	       (*(((XwManagerWidgetClass)(chosen->core.widget_class)) ->
		  manager_class.traversal_handler))
 		     (chosen, ul, lr, direction);

       }
       else   /* WE ARE ALREADY HOMED IN THIS MANAGER, GO UP */
	 {
           if (parent_is_manager)  /* really grandparent of active child */
             {
               mw->manager.active_child = NULL;
	       (*(((XwManagerWidgetClass)(parent->core.widget_class)) ->
		  manager_class.traversal_handler))
 		     ((Widget)parent, ul, lr, direction);
             }

           /*  If our parent is not a manager, then we are already
               as HOME as we are going to get, so do nothing. */
         }
}



static void TraverseToChild (mw, chosen, ul, lr, direction)
  XwManagerWidget mw;
  Widget chosen;
  XPoint ul, lr;
  int direction;
  
{ 
   Widget topmost_manager= (Widget) mw;

   mw->manager.active_child = chosen;
   if (XtIsSubclass(chosen, XwprimitiveWidgetClass))
     {
        while ((topmost_manager->core.parent != NULL) &&
	       XtIsSubclass((Widget)(topmost_manager->core.parent),
		 XwmanagerWidgetClass))
	   topmost_manager = topmost_manager->core.parent;

        XtSetKeyboardFocus(topmost_manager, chosen);
      }
   else
	{
               ul.x -= chosen->core.x;
               ul.y -= chosen->core.y;
               lr.x -= chosen->core.x;
               lr.y -= chosen->core.y;
	       (*(((XwManagerWidgetClass)(chosen->core.widget_class)) ->
		  manager_class.traversal_handler))
 		     (chosen, ul, lr, direction);
	}

}



/*************************************<->*************************************
 *
 *  XwMoveFocus(w)
 *
 *   Description:
 *   -----------
 *    Useful when an application has created multiple top level
 *    hierarchies of widgets, is using keyboard traversal and
 *    wishes to move between them.
 *
 *
 *   Inputs:  w  = widget which is the new top level widget.
 *   ------
 *
 *************************************<->***********************************/


void XwMoveFocus(w)

 Widget w;

{
 /* MOVE MOUSE INTO NEW WINDOW HIERARCHY */
  XWarpPointer(XtDisplay(w), None, XtWindow(w),0,0,0,0,1,1);

 /* SET FOCUS HERE, SHOULD WORK FOR ALL FOCUS MODES */
 XSetInputFocus (XtDisplay(w), XtWindow(w), RevertToParent,
		 CurrentTime);


}

static Widget RightCheck (chosen, ul, lr, candidate, cand_ul, cand_lr)

   Widget chosen, candidate;
   XPoint ul, lr, cand_ul, cand_lr;

{
   if (((ul.x < cand_ul.x) || ((ul.x == cand_ul.x) && (lr.x < cand_lr.x))) &&
        (ul.y <= cand_lr.y) && (lr.y >= cand_ul.y))
   {
      if (chosen == NULL) 
         chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
      else
      {
         if (chosen->core.x > cand_ul.x)
            chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
         else
            if (cand_ul.x == chosen->core.x)
            {
               if (abs(cand_lr.x - lr.x) < 
                   abs((chosen->core.x+chosen->core.width +
                         (chosen->core.border_width << 1))-lr.x))
                     chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
               else
                  if (abs(cand_lr.x - lr.x) ==
                      abs((chosen->core.x+chosen->core.width+
                           (chosen->core.border_width << 1))-lr.x))
                            
                     if (Overlap(ul, lr, candidate) > Overlap(ul, lr, chosen))
                        chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
                     else
		     {
                        if (Overlap(ul,lr,candidate) == Overlap(ul,lr,chosen))
                           if (cand_ul.y < chosen->core.y)
                              chosen = ValidChoice(chosen, candidate,
                                                   XwVERIFY_ONLY);
                            else
			    {
                                if (cand_ul.y == chosen->core.y)
                                   if (cand_lr.y < chosen->core.y +
                                       chosen->core.height +
                                       (chosen->core.border_width << 1))
                                           chosen = ValidChoice(chosen,  
                                                                candidate,
                                                              XwVERIFY_ONLY);
                             }
                     }
            }
      }
   }
   return (chosen);
}


static Widget LeftCheck (chosen, ul, lr, candidate, cand_ul, cand_lr)

   Widget chosen, candidate;
   XPoint ul, lr, cand_ul, cand_lr;

{
   if (((cand_ul.x < ul.x) || ((cand_ul.x == ul.x) && (cand_lr.x < lr.x))) &&
       (ul.y <= cand_lr.y) && (lr.y >= cand_ul.y))
   {
      if (chosen == NULL) 
         chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
      else
      {
         if (cand_ul.x > chosen->core.x)
              chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
         else
            if (cand_ul.x == chosen->core.x)
	    {
               if (abs(cand_lr.x - lr.x) < abs(chosen->core.x+
                   chosen->core.width+(chosen->core.border_width<<1)-lr.x))
                  chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
               else
                  if (abs(cand_lr.x - lr.x) == abs(chosen->core.x+
                      chosen->core.width+(chosen->core.border_width<<1)-lr.x))
                     if (Overlap(ul, lr, candidate) > Overlap(ul, lr, chosen))
                        chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
                     else
		     {
                        if (Overlap(ul,lr,candidate) == Overlap(ul,lr,chosen))
                           if (cand_ul.y > chosen->core.y)
                              chosen = ValidChoice(chosen, candidate, 
                                                   XwVERIFY_ONLY);
                           else
			   {
                              if (cand_ul.y == chosen->core.y)
                                 if (cand_lr.y > 
                                     chosen->core.y + chosen->core.height +
                                     (chosen->core.border_width << 1))
                                       chosen = ValidChoice(chosen, candidate,
                                                            XwVERIFY_ONLY);
                           }
                      }
            }
      }
   }
   return (chosen);
}


static Widget UpCheck (chosen, ul, lr, candidate, cand_ul, cand_lr)

   Widget chosen, candidate;
   XPoint ul, lr, cand_ul, cand_lr;

{
   if (((cand_ul.y < ul.y) || ((cand_ul.y == ul.y) && (cand_lr.y < lr.y))) &&
       (ul.x <= cand_lr.x) && (lr.x >= cand_ul.x))
   {
      if (chosen == NULL) 
         chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
      else
      {
         if (cand_ul.y > chosen->core.y)
            chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
         else
            if (cand_ul.y == chosen->core.y)
	    {
               if (abs(cand_lr.y - lr.y) < abs(chosen->core.y+
                   chosen->core.height+(chosen->core.border_width<<1)-lr.y))
                  chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
               else
                  if (abs(cand_lr.y - lr.y) == abs(chosen->core.y+
                      chosen->core.height+(chosen->core.border_width<<1)-lr.y))
                     if (xOverlap(ul, lr, candidate) > xOverlap(ul, lr, chosen))
                        chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
                     else
		     {
                        if (xOverlap(ul, lr, candidate) ==
                            xOverlap(ul, lr, chosen))
                           if (cand_ul.x > chosen->core.x)
                              chosen = ValidChoice(chosen, candidate, 
                                                   XwVERIFY_ONLY);
                           else
			   {
                              if (cand_ul.x == chosen->core.x)
                                 if (cand_lr.x > 
                                     chosen->core.x + chosen->core.width +
                                     (chosen->core.border_width << 1))
                                     chosen = ValidChoice(chosen,candidate,
                                                          XwVERIFY_ONLY);
                           }
                      }
            }
      }
   }
   return (chosen);
}


static Widget DownCheck (chosen, ul, lr, candidate, cand_ul, cand_lr)

   Widget chosen, candidate;
   XPoint ul, lr, cand_ul, cand_lr;

{
   if (((ul.y < cand_ul.y) || ((ul.y == cand_ul.y) && (lr.y < cand_lr.y))) &&
       (ul.x <= cand_lr.x) && (lr.x >= cand_ul.x))
   {
      if (chosen == NULL) 
         chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
      else
      {
         if (chosen->core.y > cand_ul.y)
            chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
         else
            if (cand_ul.y == chosen->core.y)
	    {
               if (abs(cand_lr.y - lr.y) < 
                   abs((chosen->core.y+chosen->core.height+
                      (chosen->core.border_width << 1))-lr.y))
                      chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
               else
                  if (abs(cand_lr.y - lr.y) ==
                      abs((chosen->core.y+chosen->core.height+
                      (chosen->core.border_width << 1))-lr.y))
                            
                     if (xOverlap(ul, lr, candidate) > xOverlap(ul, lr, chosen))
                        chosen = ValidChoice(chosen, candidate, XwVERIFY_ONLY);
                     else
		     {
                        if (xOverlap(ul, lr, candidate) ==
                            xOverlap(ul, lr, chosen))
                           if (cand_ul.x < chosen->core.x)
                              chosen = ValidChoice(chosen, candidate, 
                                                   XwVERIFY_ONLY);
                           else
			   {
                              if (cand_ul.x == chosen->core.x)
                                 if (cand_lr.x < 
                                     chosen->core.x + chosen->core.width +
                                    (chosen->core.border_width << 1))
                                     chosen = ValidChoice(chosen, candidate,
                                                          XwVERIFY_ONLY);
                           }
                     }
            }
      }
                     
   }
   return (chosen);
}

/*
 * To prevent the traversal mechanism from getting into an infinite
 * recursion, we need to keep a list of those widgets already checked
 * during a given traversal request.  If a candidate has already been
 * checked once, we don't want to give it a second chance.  This is
 * normally a problem only when a traversal request causes the search
 * to wrap to the opposite edge.  
 *
 * There are two types of entries maintained on the list: active entries
 * and chosen entries.  An entry is a chosen entry if its parent has
 * determined that the entry can possible accept the traversal request,
 * and has thus chosen it to in turn check its children to see if they
 * can accept the traversal.  An entry is an active entry if it is part
 * of the current traversal path, but it does not have any children which
 * can currently accept the traversal request; a widget is added to this
 * list only if it is not already on the chosen list, AND it is passing
 * the traversal request up to its parent.
 *
 * The active list prevents any of the widgets in the current traversal
 * path from being rechecked by their parent, once they have already
 * decided they can't accept the traversal request, until the upper most
 * manager decides to wrap at an edge and start looking again; at this
 * point, all active entries are cleared from the list, so that they can
 * have a chance to regain the traversal if no other widget can.
 *
 * If an entry is marked as being on the
 * active list, and then a request comes to place it on the chosen list,
 * then the move will occur.  If, however, a widget is on the chosen list,
 * and it asks to move to the active list, this will be denied, since we
 * don't want this widget to have the chance of being chosen again.  An
 * entry on the active list has not yet been chosen, it mearly was part of
 * the original active child list.
 */

static Widget ValidChoice (chosen, candidate, action)

   Widget chosen, candidate;
   XwValidChoiceActions action;

{
   int i;

   /* If candidate is already on the list, then don't reprocess it */
   if (action == XwVERIFY_ONLY)
   {
      /* Don't consider it unless it has some traversable primitives */
      if (XtIsSubclass(candidate, XwmanagerWidgetClass) &&
         _XwFindTraversablePrim(candidate) == False)
           return (chosen);

      for (i = 0; i < _XwCheckListIndex; i++)
      {
         if ((_XwCheckList[i].widget == candidate) && 
             (_XwCheckList[i].type != XwNONE))
            return (chosen);
      }
   }

   if ((action == XwADD_TO_CHOSEN_LIST) || (action == XwADD_TO_ACTIVE_LIST))
   {
      /* See if already on the list; then just update type field */
      for (i = 0; i < _XwCheckListIndex; i++)
      {
         if (_XwCheckList[i].widget == candidate)
         {
            if (_XwCheckList[i].type != XwCHOSEN)
            {
               _XwCheckList[_XwCheckListIndex].type = 
                       (action == XwADD_TO_CHOSEN_LIST) ? XwCHOSEN : XwACTIVE;
            }
            return (candidate);
         }
      }

      /* See if the list needs to grow */
      if (_XwCheckListIndex >= _XwCheckListSize)
      {
         _XwCheckListSize += 20;
         _XwCheckList = (_XwCheckListEntry *)XtRealloc((char *)_XwCheckList, 
                              (sizeof(_XwCheckListEntry) * _XwCheckListSize));
      }

      /* Add to the checklist */
      _XwCheckList[_XwCheckListIndex].widget = candidate;
      _XwCheckList[_XwCheckListIndex].type = (action == XwADD_TO_CHOSEN_LIST) ?
                                             XwCHOSEN : XwACTIVE;
      _XwCheckListIndex++;
   }

   /* Clear all entries which were not on the chosen list */
   if (action == XwCLEAR_ACTIVE_LIST)
   {
      for (i = 0; i < _XwCheckListIndex; i++)
      {
         if (_XwCheckList[i].type == XwACTIVE)
            _XwCheckList[i].type = XwNONE;
      }
   }

   return (candidate);
}

void _XwInitCheckList()

{
   /* Initialize the traversal checklist */
   if (_XwCheckList == NULL)
   {
      _XwCheckList=(_XwCheckListEntry *)XtMalloc(sizeof(_XwCheckListEntry)*20);
      _XwCheckListSize = 20;
   }

   /* Always reset the checklist to the beginning */
   _XwCheckListIndex = 0;
}


/*
 * This function verifies that all of the ancestors of a manager
 * have traversal enabled.
 */

static Boolean _XwManagerIsTraversable (w)

   XwManagerWidget w;

{
   while (w && XtIsSubclass((Widget)w, XwmanagerWidgetClass))
   {
      if ((w->manager.traversal_on == FALSE) || (w->core.visible == False))
         return (False);

      w = (XwManagerWidget)w->core.parent;
   }

   return (True);
}


static void ClearKbdFocus (topmost_manager)

   Widget topmost_manager;

{
   while ((topmost_manager->core.parent != NULL) &&
           XtIsSubclass((Widget)(topmost_manager->core.parent),
 	   XwmanagerWidgetClass))
      topmost_manager = topmost_manager->core.parent;

   XtSetKeyboardFocus(topmost_manager, NULL);
}
