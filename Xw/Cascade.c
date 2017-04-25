/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        Cascade.c
 **
 **   Project:     X Widgets
 **
 **   Description: Code and class record for Cascade widget
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
#include <X11/Xutil.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Shell.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <Xw/CascadeP.h>
#include <Xw/Cascade.h>
#include <Xw/MenuBtnP.h>
#include <Xw/MenuBtn.h>
#include <ctype.h>

static void Initialize();
static void Realize();
static void Resize();
static void Redisplay();
static void InsertChild();
static void Select();
static void Leave();
static Boolean SetValues();
static XtGeometryResult GeometryManager();
static void ChangeManaged();
static void GetFontGC();
static void SetTitleAttributes();
static void Visibility();
static void Unmap();

/* The following are used for layout spacing */

#define PADDING_TO_LEFT       2
#define PADDING_TO_RIGHT      2
#define PADDING_FROM_ABOVE    2
#define PADDING_FROM_BELOW    PADDING_FROM_ABOVE
#define PADDING_FOR_UNDERLINE 0   /* No longer used */
#define DOUBLE_LINES          4


/****************************************************************
 *
 * Default translation table for class: MenuPane manager
 *
 ****************************************************************/

static char defaultTranslations[] =
    "<Btn1Down>:         select()\n\
     <Visible>:          visible()\n\
     <Unmap>:            unmap()\n\
     <LeaveWindow>:      leave()";


/****************************************************************
 *
 * Action list for class: MenuPane manager
 *
 ****************************************************************/

static XtActionsRec actionsList[] = {
   {"select", (XtActionProc) Select},
   {"visible", (XtActionProc) Visibility},
   {"unmap", (XtActionProc) Unmap},
   {"leave",  (XtActionProc) Leave},
};


/****************************************************************
 *
 * MenuPane Resources
 *
 ****************************************************************/

static XtResource resources[] = {
    {XtNtitlePosition, XtCTitlePosition, XtRTitlePositionType, sizeof(int),
       XtOffset(XwCascadeWidget, cascade.titlePosition),XtRString,"top"},
};


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

XwCascadeClassRec XwcascadeClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &XwmenupaneClassRec,
    /* class_name         */    "XwCascade",
    /* widget_size        */    sizeof(XwCascadeRec),
    /* class_initialize   */    NULL,
    /* class_part_init    */    NULL,
    /* class_inited       */	FALSE,
    /* initialize         */    Initialize,
    /* initialize_hook    */    NULL,
    /* realize            */    Realize,
    /* actions		  */	actionsList,
    /* num_actions	  */	XtNumber(actionsList),
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterlv   */	TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    NULL,
    /* resize             */    Resize,
    /* expose             */    Redisplay,
    /* set_values         */    SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook    */    NULL,
    /* accept_focus       */    NULL,
    /* version            */    XtVersion,
    /* PRIVATE cb list    */    NULL,
    /* tm_table           */    defaultTranslations,
    /* query_geometry     */    NULL,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },{
/* composite_class fields */
    /* geometry_manager   */    GeometryManager,
    /* change_managed     */    ChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,	
				NULL
  },{
/* constraint class fields */
    /* resources          */    NULL,
    /* num_resources      */    0,
    /* constraint_size    */    sizeof (Boolean),
    /* initialize         */    (XtInitProc)XtInheritMenuPaneConstraintInit,
    /* destroy            */    NULL,
    /* set_values         */    NULL,
				NULL
  },{
/* manager_class fields */
   /* traversal handler   */    (XwTraversalProc)XtInheritTraversalProc,
   /* translations        */    NULL
  },{
/* menu pane class - none */     
   /* setTraversalFlag    */    XtInheritSetTraversalFlagProc
  },{
/* cascade class - none */     
     /* mumble */               0
  }
};

WidgetClass XwcascadeWidgetClass = (WidgetClass)&XwcascadeClassRec;


/*************************************<->*************************************
 *
 *  Select ()
 *
 *   Description:
 *   -----------
 *     Called upon receipt of a 'select' action; simple invokes callbacks.
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

static void Select (w, event)

   Widget w;
   XEvent * event;

{
   Widget parent, grandparent;

   if ((parent = XtParent(w)) &&
       (grandparent = XtParent(parent)) &&
       (XtIsSubclass (grandparent, XwmenumgrWidgetClass)))
   {
      /* Ask the menu mgr if we should process or ignore the select */
      if ((*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.
          processSelect) (grandparent, w, event) == FALSE)
      {
         return;
      }
   }

   XtCallCallbacks (w, XtNselect, NULL);
}


/*************************************<->*************************************
 *
 *  Leave()
 *
 *   Description:
 *   -----------
 *     Called upon receipt of a LeaveNotify event; this is a NOOP, whose
 *     purpose is to simply prevent us from using the LeaveNotify handler
 *     supplied by the Manager class.  The Manager's action routine does
 *     an XSetInputFocus() to root, which is not what we want here.
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

static void Leave (w, event)

   Widget w;
   XEvent * event;

{
}


/*************************************<->*************************************
 *
 *  GetIdealSize
 *
 *   Description:
 *   -----------
 *    Returns the minimum height and width into which the menu pane will fit.
 *    replyHeight and replyWidth are set to the minimum acceptable size.
 *    Also, the maxCascadeWidth, maxLabel and largestButtonWidth fields are
 *    set in the widget structure.
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

static GetIdealSize(mw, replyWidth, replyHeight)

    XwCascadeWidget	mw;
    Dimension		*replyWidth, *replyHeight;	/* RETURN */

{
    Cardinal  i;
    Dimension minWidth, minHeight, largestButtonWidth;
    XwMenuButtonWidget menuButton;   /* Current button */
    Dimension idealWidth;

   
  /*
   * First, compute the minimum width (less padding) needed for
   * these items; include the title in these calculations.
   */

   minWidth = largestButtonWidth = 0;
   minHeight =  mw->manager.highlight_thickness << 1;

   if (mw->menu_pane.title_showing && !mw->menu_pane.title_override)
   {
      if (mw->menu_pane.title_type == XwSTRING)
      {
         minWidth = mw->cascade.title_width + 
                    PADDING_TO_LEFT +
                    PADDING_TO_RIGHT;
         largestButtonWidth = minWidth;
         if (mw->cascade.titlePosition & XwTOP)
         {
            minHeight += mw->cascade.title_height + 
                        PADDING_FROM_ABOVE +
                        PADDING_FOR_UNDERLINE +
                        DOUBLE_LINES;
         }
         if (mw->cascade.titlePosition & XwBOTTOM)
         {
            minHeight += mw->cascade.title_height + 
                        PADDING_FROM_BELOW +
                        PADDING_FOR_UNDERLINE +
                        DOUBLE_LINES;
         }
      }
      else
      {
         minWidth = mw->cascade.title_width + 
                    PADDING_TO_LEFT +
                    PADDING_TO_RIGHT;
         largestButtonWidth = minWidth;
         if (mw->cascade.titlePosition & XwTOP)
         {
            minHeight += mw->cascade.title_height + 
                        PADDING_FROM_ABOVE +
                        PADDING_FOR_UNDERLINE +
                        DOUBLE_LINES;
         }
         if (mw->cascade.titlePosition & XwBOTTOM)
         {
            minHeight += mw->cascade.title_height + 
                        PADDING_FROM_BELOW +
                        PADDING_FOR_UNDERLINE +
                        DOUBLE_LINES;
         }
      }
   }

   minWidth += mw->manager.highlight_thickness << 1;

 
   /* 
    * Determine the ideal size for each button; keep track of largest button
    */

   for (i=0; i < mw->manager.num_managed_children; i++)
   {
       menuButton = (XwMenuButtonWidget) mw->manager.managed_children[i];

       /*
        * If we are in the middle of a geometry request from one of
        * our children, then we need to use the ideal size of the
        * requesting widget, instead of the current widget.  Otherwise,
        * ask the widget for its ideal size.
        */
       if ((Widget) menuButton == mw->cascade.georeqWidget)
          idealWidth = mw->cascade.georeqWidth;
       else if (XtIsSubclass ((Widget)menuButton, XwmenubuttonWidgetClass))
       {
          (*(((XwMenuButtonWidgetClass)XtClass(menuButton))->
               menubutton_class.idealWidthProc))((Widget)menuButton, &idealWidth);
       }
       else
          idealWidth = menuButton->core.width;

       minWidth =  Max (minWidth, idealWidth + 
                              (mw->manager.highlight_thickness << 1) +
			      (menuButton->core.border_width << 1));
       largestButtonWidth =  Max (largestButtonWidth, idealWidth);
       minHeight += menuButton->core.height +
                              (menuButton->core.border_width << 1);
   }


   /* Set up return values */
   if (minWidth == 0) minWidth = 1;
   if (minHeight == 0) minHeight = 1;

   if (replyWidth != NULL)  *replyWidth = minWidth;
   if (replyHeight != NULL) *replyHeight = minHeight;

   mw->cascade.idealHeight = minHeight;
   mw->cascade.idealWidth = minWidth;
   mw->cascade.largestButtonWidth = largestButtonWidth;
}


/*************************************<->*************************************
 *
 *  ProcedureName (parameters)
 *
 *   Description:
 *   -----------
 *    Returns the minimum height and width into which the menu pane will fit.
 *    If the current menu pane size is small than this, then FALSE is
 *    returned; else, TRUE is returned.  In all cases, replyHeight and
 *    replyWidth are set to the minimum acceptable size.
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

static Boolean PreferredSize(mw, width, height, replyWidth, replyHeight)
    XwCascadeWidget	mw;
    Dimension		width, height;
    Dimension		*replyWidth, *replyHeight;
{
    GetIdealSize(mw, replyWidth, replyHeight);

    return ((*replyWidth <= width) && (*replyHeight <= height));
}


/*************************************<->*************************************
 *
 *  ProcedureName (parameters)
 *
 *   Description:
 *   -----------
 *   This routine assumes the menu pane size has already been updated in
 *   the core record.  It will simply relayout the menu buttons, to fit
 *   the new menu pane size.  This is called when XtResizeWidget() is invoked,
 *   and when our own geometry manager tries to change our size because one
 *   of our children is trying to change its size.
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

static void Resize(mw)

    XwCascadeWidget	mw;

{
    Cardinal  i;
    Cardinal xPos, yPos;
    Dimension oldHeight;
    XwMenuButtonWidget menuButton;   /* Current button */
    Dimension cascadeWidth, labelX;

   
    /* Recalculate where the title string should be displayed */
    mw->cascade.title_x = ((mw->core.width >> 1) +
                          mw->manager.highlight_thickness) - 
                            (mw->cascade.title_width >> 1);

    /* This is a two step process: first move the item, then resize it */
    xPos = mw->manager.highlight_thickness;
    yPos = mw->cascade.button_starting_loc;

    for (i = 0; i < mw->manager.num_managed_children; i++)
    {
       menuButton = (XwMenuButtonWidget) mw->manager.managed_children[i];
       XtMoveWidget ((Widget)menuButton, xPos, yPos);

       /*
        * The following kludge will force the widget's resize routine
        * to always be called.
        */
       oldHeight = menuButton->core.height;
       menuButton->core.height = 0;
       menuButton->core.width = 0;
      
       XtResizeWidget ((Widget)menuButton,
                       mw->cascade.largestButtonWidth,
                       oldHeight,
                       menuButton->core.border_width);

       yPos += menuButton->core.height +
               (menuButton->core.border_width << 1);
    }

    SetTitleAttributes (mw, FALSE);
} /* Resize */


/*************************************<->*************************************
 *
 *  TryNewLayout (parameters)
 *
 *   Description:
 *   -----------
 *   This determines if the menu pane needs to change size.  If it needs to
 *   get bigger, then it will ask its parent's geometry manager; if this
 *   succeeds, then the menu pane window will be resized (NOTE: the menu
 *   buttons are not touched yet).  Also, if our resize request succeeded,
 *   the our parent's geometry manager will have updated our size fields
 *   in our core structure.  This is called during a Change Manage request,
 *   and when one of our children issues a geometry request.
 *
 *   NOTE: menu panes ALWAYS use their 'preferred' size.
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

static Boolean TryNewLayout(mw)
    XwCascadeWidget	mw;
{
    XtWidgetGeometry request, reply;
    XtGeometryResult r;

    if ((mw->core.width == mw->cascade.idealWidth) && 
        (mw->core.height == mw->cascade.idealHeight)) 
    {
        /* Same size */
	return (TRUE);
    }


    switch (XtMakeResizeRequest ((Widget) mw, mw->cascade.idealWidth, 
            mw->cascade.idealHeight, NULL, NULL))
    {
	case XtGeometryYes:
	    return (TRUE);

	case XtGeometryNo:
	case XtGeometryAlmost:
	    return (FALSE);
    }

    return (FALSE);
}


/*************************************<->*************************************
 *
 *  ProcedureName (parameters)
 *
 *   Description:
 *   -----------
 *   Invoked when one of our children wants to change its size.  This routine
 *   will automatically update the size fields within the child's core
 *   area.
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

/*ARGSUSED*/
static XtGeometryResult GeometryManager(w, request, reply)
    Widget		w;
    XtWidgetGeometry	*request;
    XtWidgetGeometry	*reply;	/* RETURN */

{
    Dimension	width, height, borderWidth, junkH, junkW, newHeight;
    XwCascadeWidget mw;
    XtGeometryResult result = XtGeometryYes;
    Boolean isPreferredSize;
    int i;

    /* 
     * Since the manager is solely responsible for controlling the position
     * of its children, any geometry request made which attempts to modify
     * the child's position will not be satified; instead, a compromise will
     * be returned, with the position bits cleared.
     */
    reply->request_mode = 0;

    if (request->request_mode & (CWX | CWY))
       result = XtGeometryAlmost;

    mw = (XwCascadeWidget) w->core.parent;


    /* Size changes must see if the new size can be accomodated */
    if (request->request_mode & (CWWidth | CWHeight | CWBorderWidth)) {

	/* Make all three fields in the request valid */
	if ((request->request_mode & CWWidth) == 0)
	    request->width = w->core.width;
	if ((request->request_mode & CWHeight) == 0)
	    request->height = w->core.height;
        if ((request->request_mode & CWBorderWidth) == 0)
	    request->border_width = w->core.border_width;

        /* Set the new size within the widget */
        width = w->core.width;
        height = w->core.height;
        borderWidth = w->core.border_width;
        w->core.width = request->width;
        w->core.height = request->height;
        w->core.border_width = request->border_width;

        /* Calculate the new height needed to hold the menu */
        newHeight = mw->core.height - (height + borderWidth) + 
                    (request->height + request->border_width);

        /* Let the layout routine know we are in the middle of a geo req */
        mw->cascade.georeqWidget = w;
        mw->cascade.georeqWidth = request->width;

        /* See if we can allow this size request */
        isPreferredSize = PreferredSize (mw, w->core.width + 
                                        (w->core.border_width << 1) +
                                        (mw->manager.highlight_thickness << 1),
                                         newHeight, &junkW, &junkH);

        /* Cleanup */
        mw->cascade.georeqWidget = (Widget) NULL;

        /* 
         * Only allow an item to shrink if it was the largest item.
         * However, always allow the item to get bigger.
         */

        if (!isPreferredSize)
        {
           /*
            * The widget is attempting to make itself smaller than is allowed;
            * therefore, we must return a compromise size, which we know will
            * succeed.
            */
           reply->width = junkW - (mw->manager.highlight_thickness << 1) -
                                  (w->core.border_width << 1);
           reply->height = request->height;
           reply->border_width = request->border_width;
           reply->request_mode = CWWidth | CWHeight | CWBorderWidth;
           result = XtGeometryAlmost;

           /* Restore the previous size values */
           w->core.width = width;
           w->core.height = height;
           w->core.border_width = borderWidth;
	} 
        else 
        {
            if (result == XtGeometryAlmost)
            {
               /*
                * Even though the size request succeeded, the request cannot
                * be satisfied, since someone before us has already set the
                * result to XtGeometryAlmost; most likely, it was because
                * an attempt was made to change the X or Y values.
                */
               reply->width = request->width;
               reply->height = request->height;
               reply->border_width = request->border_width;
               reply->request_mode = CWWidth | CWHeight | CWBorderWidth;

               /* Restore the previous size values */
               w->core.width = width;
               w->core.height = height;
               w->core.border_width = borderWidth;
            }
            else
            {
               /*
                * All components of the geometry request could be satisfied,
                * so make all necessary adjustments, and return a 'Yes'
                * response to the requesting widget.
                */
               TryNewLayout (mw);
               Resize (mw);
               /* SetTitleAttributes (mw, FALSE); */
	       width = w->core.width;
	       height = w->core.height;
	       borderWidth = w->core.border_width;
               result = XtGeometryDone;
            }
	}
    }; /* if any size changes requested */

    /*
     * Stacking changes are always allowed.  However, if the result has
     * already been set to XtGeometryAlmost, then some portion of the
     * request has already failed, so we need to copy the stacking request
     * back into the 'reply' structure, so that they are not lost.
     */

    if (result == XtGeometryAlmost)
    {
       if (request->request_mode & CWSibling)
       {
          reply->request_mode |= CWSibling;
          reply->sibling = request->sibling;
       }

       if (request->request_mode & CWStackMode)
       {
          reply->request_mode |= CWStackMode;
          reply->stack_mode = request->stack_mode;
       }
    }

    return (result);
 }


/*************************************<->*************************************
 *
 *  ChangeManaged
 *
 *   Description:
 *   -----------
 *   Forces the managed_children list to be updated, and then attempts to
 *   relayout the menu buttons.  If the menu pane is not large enough, then
 *   it will ask its geometry manager to enlarge it.  This is invoked when
 *   a new child is managed, or an existing one is unmanaged. NOTE: only
 *   managed children which also have the mapped_when_managed flag set
 *   are included in the layout calculations.
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

static void ChangeManaged (mw)
    XwCascadeWidget mw;
{
   Widget parent, grandparent;
   Widget menubutton;
   int i;
   Boolean * managedConstraintFlag;

   /* Reconfigure the button box */
   _XwSetMappedManagedChildrenList(mw);
   SetTitleAttributes (mw, FALSE);
   GetIdealSize(mw, NULL, NULL);
   (void) TryNewLayout(mw);
   Resize(mw);

   /*
    * Tell the menu manager that the list of mapped & managed children
    * has changed,
    */
   if ((parent = XtParent(mw)) &&
       (grandparent = XtParent(parent)) &&
       (XtIsSubclass (grandparent, XwmenumgrWidgetClass)))
   {
      (*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.
        paneManagedChildren) (grandparent, (Widget)mw);
   }

   /*
    * The constraint record has been updated by the menu manager.
    */
}


/*************************************<->*************************************
 *
 *  Initialize 
 *
 *
 *   Description:
 *   -----------
 *   Initialize the menu_pane fields within the widget's instance structure.
 *   Calls for registering the pane with a menu manager, and for attaching
 *   the pane to a menu button are handled by our meta class.
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

static void Initialize (request, new)
    XwCascadeWidget request, new;
{
    Arg args2[2];

    new->cascade.georeqWidget = (Widget) NULL;

    /* Determine the title attributes, and the starting location for our kids */
    SetTitleAttributes (new, TRUE);

    /* Force our parent shell to be resizable */
    XtSetArg (args2[0], XtNallowShellResize, TRUE);
    XtSetArg (args2[1], XtNoverrideRedirect, TRUE);
    XtSetValues (new->core.parent, args2, XtNumber(args2));

    /* Augment our translations to include the traversal actions */
    XtAugmentTranslations ((Widget) new,
                       XwmanagerClassRec.manager_class.translations);

    /* KLUDGE!!!  This is necessary!!!   The shell must be realized so
     * that the cascades changed_managed routine is called and the
     * keyboard accelerators are set up for the menu manager.
     */
    if (new->core.height > 0)
       (new->core.parent)->core.height = new->core.height;
    else
       (new->core.parent)->core.height = 1;

    if (new->core.width > 0)
       (new->core.parent)->core.width = new->core.width;
    else
       (new->core.parent)->core.width = 1;

    if (XtIsSubclass(new->core.parent, shellWidgetClass))
       XtRealizeWidget (new->core.parent);
} /* Initialize */

/*************************************<->*************************************
 *
 *  Realize
 *
 *   Description:
 *   -----------
 *   Invoked only when the menu pane is realized.  It will create the window
 *   in which the menu pane will be displayed.
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

static void Realize(w, p_valueMask, attributes)
    register Widget w;
    Mask *p_valueMask;
    XSetWindowAttributes *attributes;
{
    XwCascadeWidget mw;
    Mask valueMask = *p_valueMask;

    attributes->bit_gravity = ForgetGravity;
    valueMask |= CWBitGravity;
    mw = (XwCascadeWidget) w;
    
    w->core.window =
	XCreateWindow(XtDisplay(w),  w->core.parent->core.window,
	w->core.x, w->core.y, w->core.width, w->core.height,
	w->core.border_width, w->core.depth,
	InputOutput, (Visual *)CopyFromParent,
	valueMask, attributes);

   _XwRegisterName (w);
} /* Realize */


/*************************************<->*************************************
 *
 *  SetValues
 *
 *   Description:
 *   -----------
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

static Boolean SetValues (current, request, new)
    XwCascadeWidget current, request, new;
{
   Boolean redrawFlag = FALSE;

   /*
    * We never allow our traversal flags to be changed during SetValues();
    * this is enforced by our superclass.
    */

   /* Validate the title position, and see if it has changed */

   if ((new->cascade.titlePosition & (XwTOP | XwBOTTOM)) == 0)
   {
      new->cascade.titlePosition = current->cascade.titlePosition;
      XtWarning (
        "Cascade: Invalid title position; resetting to previous setting");
   }

   if ((current->cascade.titlePosition & (XwTOP | XwBOTTOM)) !=
       (new->cascade.titlePosition & (XwTOP | XwBOTTOM)))
   {
      redrawFlag = TRUE;
   }


   SetTitleAttributes (new, FALSE);
   GetIdealSize(new, NULL, NULL);
   new->core.height = new->cascade.idealHeight;
   new->core.width = new->cascade.idealWidth;

   /*
    * If the title position changes, but the size of the menupane
    * does not (because no other attributes have changed which could
    * cause the size to change), then we need to force a call to our
    * resize() procedure, so that the menu buttons get repositioned
    * relative to the new title position.
    */
   if (redrawFlag && (new->core.height == current->core.height) &&
       (new->core.width == current->core.width))
   {
      Resize(new);
   }

   return (redrawFlag);
}


/*************************************<->*************************************
 *
 *  Redisplay
 *
 *   Description:
 *   -----------
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

static void Redisplay (w, event)
    Widget   w;
    XEvent * event;
{
   XwCascadeWidget mw = (XwCascadeWidget) w;

   /* If a title is defined, then redisplay it, if 'showing' flag set */
   if (mw->menu_pane.title_showing && ! mw->menu_pane.title_override) 
   {

      /* Recalculate the title position */
      mw->cascade.title_x = (mw->core.width >> 1) +
                             mw->manager.highlight_thickness -
                             (mw->cascade.title_width >> 1);

      if (mw->menu_pane.title_type == XwSTRING)
      {
         if (mw->cascade.titlePosition & XwTOP)
         {
            XDrawString (XtDisplay(w), XtWindow(w), mw->menu_pane.titleGC,
                    mw->cascade.title_x, mw->cascade.top_title_y,
                    mw->menu_pane.title_string,
                    XwStrlen(mw->menu_pane.title_string));

            XDrawLine (XtDisplay(w), XtWindow(w), mw->menu_pane.titleGC, 0, 
                    mw->cascade.top_title_y + 
                          mw->menu_pane.title_font->descent + 
                          PADDING_FOR_UNDERLINE + 1,
                    mw->core.width, 
                    mw->cascade.top_title_y + 
                          mw->menu_pane.title_font->descent + 
                          PADDING_FOR_UNDERLINE + 1);

            XDrawLine (XtDisplay(w), XtWindow(w), mw->menu_pane.titleGC, 0, 
                    mw->cascade.top_title_y + 
                          mw->menu_pane.title_font->descent + 
                          PADDING_FOR_UNDERLINE + 3,
                    mw->core.width, 
                    mw->cascade.top_title_y + 
                          mw->menu_pane.title_font->descent + 
                          PADDING_FOR_UNDERLINE + 3);
         }

         if (mw->cascade.titlePosition & XwBOTTOM)
         {
            XDrawString (XtDisplay(w), XtWindow(w), mw->menu_pane.titleGC,
                    mw->cascade.title_x, mw->cascade.bottom_title_y,
                    mw->menu_pane.title_string,
                    XwStrlen(mw->menu_pane.title_string));

            XDrawLine (XtDisplay(w), XtWindow(w), mw->menu_pane.titleGC, 0, 
                    mw->cascade.bottom_title_y - 
                          mw->menu_pane.title_font->ascent - 
                          DOUBLE_LINES + 2,
                    mw->core.width, 
                    mw->cascade.bottom_title_y - 
                          mw->menu_pane.title_font->ascent - 
                          DOUBLE_LINES + 2);

            XDrawLine (XtDisplay(w), XtWindow(w), mw->menu_pane.titleGC, 0, 
                    mw->cascade.bottom_title_y - 
                          mw->menu_pane.title_font->ascent - 
                          DOUBLE_LINES,
                    mw->core.width, 
                    mw->cascade.bottom_title_y - 
                          mw->menu_pane.title_font->ascent - 
                          DOUBLE_LINES);
         }
      }
      else if (mw->menu_pane.titleImage)
      {
         if (mw->cascade.titlePosition & XwTOP)
         {
            XPutImage (XtDisplay(mw), XtWindow(mw), mw->menu_pane.titleGC,
                       mw->menu_pane.titleImage, 0, 0,
                       mw->cascade.title_x, mw->cascade.top_title_y,
                       mw->cascade.title_width, mw->cascade.title_height);

            XDrawLine (XtDisplay(w), XtWindow(w), mw->menu_pane.titleGC, 0, 
                    mw->cascade.top_title_y + 
                          mw->cascade.title_height + 
                          PADDING_FOR_UNDERLINE + 1,
                    mw->core.width, 
                    mw->cascade.top_title_y + 
                          mw->cascade.title_height + 
                          PADDING_FOR_UNDERLINE + 1);

            XDrawLine (XtDisplay(w), XtWindow(w), mw->menu_pane.titleGC, 0, 
                    mw->cascade.top_title_y + 
                          mw->cascade.title_height + 
                          PADDING_FOR_UNDERLINE + 3,
                    mw->core.width, 
                    mw->cascade.top_title_y + 
                          mw->cascade.title_height + 
                          PADDING_FOR_UNDERLINE + 3);
         }

         if (mw->cascade.titlePosition & XwBOTTOM)
         {
            XPutImage (XtDisplay(mw), XtWindow(mw), mw->menu_pane.titleGC,
                       mw->menu_pane.titleImage, 0, 0,
                       mw->cascade.title_x, mw->cascade.bottom_title_y,
                       mw->cascade.title_width, mw->cascade.title_height);

            XDrawLine (XtDisplay(w), XtWindow(w), mw->menu_pane.titleGC, 0, 
                    mw->cascade.bottom_title_y - 
                          DOUBLE_LINES + 2,
                    mw->core.width, 
                    mw->cascade.bottom_title_y - 
                          DOUBLE_LINES + 2);

            XDrawLine (XtDisplay(w), XtWindow(w), mw->menu_pane.titleGC, 0, 
                    mw->cascade.bottom_title_y - 
                          DOUBLE_LINES,
                    mw->core.width, 
                    mw->cascade.bottom_title_y - 
                          DOUBLE_LINES);
         }
      }
   }

}


/*************************************<->*************************************
 *
 *  SetTitleAttributes
 *
 *   Description:
 *   -----------
 *   This procedure calculates the height, width, and starting location
 *   for the title string/image.  In addition, it uses
 *   this information to determine where the menu buttons should be laid
 *   out within the pane.
 *
 *
 *   Inputs:
 *   ------
 *     mw = The menu pane widget whose title attributes are being calculated.
 *
 *     setCoreFields = A boolean flag, indicating whether the core height
 *                     and width fields should be set to their default value.
 *                     This flag should only be set to TRUE when this routine
 *                     is invoked at initialization time; all other calls must
 *                     set this to FALSE.
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void SetTitleAttributes (mw, setCoreFields)

   XwCascadeWidget mw;
   Boolean         setCoreFields;

{

   if (mw->menu_pane.title_showing && !mw->menu_pane.title_override)
   {
      if (mw->menu_pane.title_type == XwSTRING)
      {
         /* Determine size of title string */
         mw->cascade.title_width = XTextWidth (mw->menu_pane.title_font,
                                          mw->menu_pane.title_string,
                                          XwStrlen(mw->menu_pane.title_string));
         mw->cascade.title_height = mw->menu_pane.title_font->ascent +
                                    mw->menu_pane.title_font->descent;
         mw->cascade.title_x = PADDING_TO_LEFT + 
                               mw->manager.highlight_thickness;

         /* Default size should contain just the title string */
         if (setCoreFields)
         {
            mw->core.width = mw->cascade.title_width + 
                             (mw->manager.highlight_thickness << 1) +
                             PADDING_TO_LEFT +
                             PADDING_TO_RIGHT;
            mw->core.height = mw->manager.highlight_thickness << 1;
            if (mw->cascade.titlePosition & XwTOP)
               mw->core.height += mw->cascade.title_height + 
                                 PADDING_FROM_ABOVE +
                                 PADDING_FOR_UNDERLINE + 
                                 DOUBLE_LINES;
            if (mw->cascade.titlePosition & XwBOTTOM)
               mw->core.height += mw->cascade.title_height + 
                                 PADDING_FROM_BELOW +
                                 PADDING_FOR_UNDERLINE + 
                                 DOUBLE_LINES;
         }

         if (mw->cascade.titlePosition & XwTOP)
            mw->cascade.top_title_y = PADDING_FROM_ABOVE + 
                                      mw->manager.highlight_thickness +
                                      mw->menu_pane.title_font->ascent;
         else
            mw->cascade.top_title_y = 0;

         if (mw->cascade.titlePosition & XwBOTTOM)
            mw->cascade.bottom_title_y = mw->core.height - 
                                         mw->manager.highlight_thickness -
                                         mw->menu_pane.title_font->descent -
                                         PADDING_FROM_ABOVE -
                                         PADDING_FOR_UNDERLINE;
         else
            mw->cascade.bottom_title_y = mw->core.height -
                                         mw->manager.highlight_thickness;

         if (mw->cascade.titlePosition & XwTOP)
            mw->cascade.button_starting_loc = mw->cascade.top_title_y + 
                                             mw->menu_pane.title_font->descent +
                                             PADDING_FOR_UNDERLINE + 
                                             DOUBLE_LINES;
         else
            mw->cascade.button_starting_loc = mw->manager.highlight_thickness;
      }
      else if (mw->menu_pane.titleImage) 
      {
         /* Determine size of title image */
         mw->cascade.title_width = mw->menu_pane.titleImage->width;
         mw->cascade.title_height = mw->menu_pane.titleImage->height;
         mw->cascade.title_x = PADDING_TO_LEFT +
                               mw->manager.highlight_thickness;

         /* Default size should contain just the title image */
         if (setCoreFields)
         {
            mw->core.width = mw->cascade.title_width + 
                             (mw->manager.highlight_thickness << 1) +
                             PADDING_TO_LEFT +
                             PADDING_TO_RIGHT;
            mw->core.height = (mw->manager.highlight_thickness << 1);;
            if (mw->cascade.titlePosition & XwTOP)
               mw->core.height += mw->cascade.title_height + 
                                 PADDING_FROM_ABOVE +
                                 PADDING_FOR_UNDERLINE +
                                 DOUBLE_LINES;
            if (mw->cascade.titlePosition & XwBOTTOM)
               mw->core.height += mw->cascade.title_height + 
                                 PADDING_FROM_ABOVE +
                                 PADDING_FOR_UNDERLINE +
                                 DOUBLE_LINES;
         }

         if (mw->cascade.titlePosition & XwTOP)
            mw->cascade.top_title_y = PADDING_FROM_ABOVE +
                                      mw->manager.highlight_thickness;
         else
            mw->cascade.top_title_y = 0;
         if (mw->cascade.titlePosition & XwBOTTOM)
            mw->cascade.bottom_title_y = mw->core.height -
                                         mw->manager.highlight_thickness -
                                         mw->cascade.title_height -
                                         PADDING_FROM_BELOW;
         else
            mw->cascade.bottom_title_y = mw->core.height -
                                         mw->manager.highlight_thickness;

         if (mw->cascade.titlePosition & XwTOP)
            mw->cascade.button_starting_loc = mw->cascade.top_title_y + 
                                             mw->cascade.title_height +
                                             PADDING_FOR_UNDERLINE +
                                             DOUBLE_LINES;
         else
            mw->cascade.button_starting_loc = mw->manager.highlight_thickness;
      }
      else
      {
         /* No image is present */
   
         if (setCoreFields)
         {
            mw->core.width = (mw->manager.highlight_thickness > 0) ?
                             (mw->manager.highlight_thickness << 1) : 2;
            mw->core.height = mw->core.width;
         }
         mw->cascade.title_width = 0;
         mw->cascade.title_height = 0;
         mw->cascade.title_x = 0;
         mw->cascade.top_title_y = 0;
         mw->cascade.bottom_title_y = mw->core.height;
         mw->cascade.button_starting_loc = mw->manager.highlight_thickness;
      }
   }
   else
   {
      /* No image or string is present */
  
      if (setCoreFields)
      {
         mw->core.width = (mw->manager.highlight_thickness > 0) ?
                          (mw->manager.highlight_thickness << 1) : 2;
         mw->core.height = mw->core.width;
      }
      mw->cascade.title_width = 0;
      mw->cascade.title_height = 0;
      mw->cascade.title_x = 0;
      mw->cascade.top_title_y = 0;
      mw->cascade.bottom_title_y = mw->core.height;
      mw->cascade.button_starting_loc = mw->manager.highlight_thickness;
   }
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
    * Noop; purpose is to prevent Manager's translation from
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
    * Noop; purpose is to prevent Manager's translation from
    * taking effect.
    */
}
