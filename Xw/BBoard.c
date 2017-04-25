/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        BBoard.c
 **
 **   Project:     X Widgets
 **
 **   Description: Code and class record for Bulletin Board Widget.
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
#include <Xw/BBoard.h>
#include <Xw/BBoardP.h>


static void Initialize();
static XtGeometryResult GeometryManager();
static void ChangeManaged();



/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

XwBulletinClassRec XwbulletinClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &XwmanagerClassRec,
    /* class_name         */    "XwBulletinBoard",
    /* widget_size        */    sizeof(XwBulletinRec),
    /* class_initialize   */    NULL,
    /* class_part_init    */    NULL,				
    /* class_inited       */	FALSE,
    /* initialize         */    (XtInitProc) Initialize,
    /* initialize_hook    */    NULL,
    /* realize            */    _XwRealize,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources          */    NULL,
    /* num_resources      */    0,
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterlv   */    TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    NULL,
    /* resize             */    NULL,
    /* expose             */    NULL,
    /* set_values         */    NULL,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook  */	NULL,
    /* accept_focus       */    NULL,
    /* version          */	XtVersion,
    /* callback_private */      NULL,
    /* tm_table         */      NULL,
    /* query_geometry   */	NULL, 
      XtInheritDisplayAccelerator, NULL,
  },{
/* composite_class fields */
    /* geometry_manager   */    (XtGeometryHandler) GeometryManager,
    /* change_managed     */    (XtWidgetProc) ChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,	/*  from superclass */
    /* delete_child	  */	XtInheritDeleteChild,	/*  from superclass */
				NULL,
  },{
/* constraint_class fields */
   /* resource list        */   NULL,
   /* num resources        */   0,
   /* constraint size      */   0,
   /* init proc            */   NULL,
   /* destroy proc         */   NULL,
   /* set values proc      */   NULL,
				NULL
  },{
/* manager_class fields */
   /* traversal handler   */    (XwTraversalProc) XtInheritTraversalProc,
   /* translations        */	NULL,
  },{
/* bulletin board class - none */     
    /* no new fields */         0
 }	
};

WidgetClass XwbulletinWidgetClass = (WidgetClass)&XwbulletinClassRec;
WidgetClass XwbulletinBoardWidgetClass = (WidgetClass)&XwbulletinClassRec;



/*************************************<->*************************************
 *
 *  CalcSize
 *
 *   Description:
 *   -----------
 *   Figure out how much size we need.
 *
 *************************************<->***********************************/

static Boolean CalcSize(bb, width, height, replyWidth, replyHeight)
    XwBulletinWidget	bb;
    Dimension		width, height;
    Dimension		*replyWidth, *replyHeight;  
{
    Cardinal  i, j, numItems;
    Dimension minWidth, minHeight;
    Widget box;   /* Current item */

    
  /*
   * If there are no items then any space will work 
   */

   if ((numItems=bb->manager.num_managed_children) <= 0)
      {
        if ((replyWidth != NULL) && (replyHeight != NULL))
	{
	   *replyWidth = 10;
	   *replyHeight = 10;
	}
	return(TRUE);
     }


   
  /*
   * Compute the minimum width & height for this box
   * by summing the rectangles of all children.
   */

   minWidth = minHeight = 0;

   for (i=0; i < numItems; i++)
      {
        box = (Widget) bb->manager.managed_children[i];
        minWidth = Max (minWidth, (box->core.x + 2*box->core.border_width +
			 box->core.width));
	minHeight = Max (minHeight, (box->core.y + 2*box->core.border_width +
			 box->core.height));
     }



   if (bb->manager.layout == XwMAXIMIZE)
    {
         minWidth = Max (minWidth, width);
         minHeight = Max (minHeight, height);
    }
 
  
   if (replyWidth != NULL)  *replyWidth = minWidth;
   if (replyHeight != NULL) *replyHeight = minHeight;


   if ((width < minWidth) || (height < minHeight)) return (FALSE);

   return (TRUE);
}



/*************************************<->*************************************
 *
 *  DoLayout
 *
 *   Description:
 *   -----------
 * Try to do a new layout within the current width and height;
 * if that fails try to do it within the box returned by CalcSize
 *
 *
 *************************************<->***********************************/
static Boolean DoLayout(bb)
    XwBulletinWidget	bb;
{
    Dimension		width, height;
    XtWidgetGeometry request, reply;

   (void) CalcSize(bb, bb->core.width, bb->core.height, &width, &height);

   if ((bb->manager.layout == XwIGNORE) || 	/* don't care if it fits */
        ((bb->core.width == width) && (bb->core.height == height) &&
            bb->manager.layout != XwSWINDOW)) 
           	return (TRUE);   /* Same size & NOT special case */

    
    if (bb->manager.layout == XwMAXIMIZE)
       {
	  width = Max (bb->core.width, width);
	  height = Max (bb->core.height, height);
       }

    switch (XtMakeResizeRequest
	         ((Widget) bb, width, height, &width, &height))
    {
	case XtGeometryYes:
	    return (TRUE);

	case XtGeometryNo:
            return (FALSE);

	case XtGeometryAlmost:  /* TAKE WHAT YOU CAN GET! */
	    (void) XtMakeResizeRequest((Widget) bb, width, height, 
					NULL, NULL);
	    return (TRUE);
    }
}


/*************************************<->*************************************
 *
 *  Initialize
 *
 *   Description:
 *   -----------
 *     Make sure height and width are not set to zero at initialize time.
 *
 *************************************<->***********************************/
static void Initialize (request, new)
XwBulletinWidget request, new;

{
   /* DON'T ALLOW HEIGHT/WIDTH TO BE 0, BECAUSE X DOESN'T HANDLE THIS WELL */
      if (new->core.width == 0) new->core.width = 10;
      if (new->core.height == 0) new->core.height = 10;
}



/*************************************<->*************************************
 *
 *  GeometryManager(w, request, reply) 
 *
 *
 *************************************<->***********************************/

static XtGeometryResult GeometryManager(w, request, reply)
    Widget		w;
    XtWidgetGeometry	*request;
    XtWidgetGeometry	*reply;	/* RETURN */

{
    Dimension	width, height, borderWidth, junk;
    XwBulletinWidget bb;
    Boolean geoFlag;
    XtGeometryHandler manager;
    XtGeometryResult returnCode;

  /* 
   * IF LAYOUT MODE IS XwSWINDOW ( PARENT HAD BETTER BE A SUBCLASS OF 
   * SCROLLED WINDOW) THEN SIMPLY PASS THIS REQUEST TO MY PARENT AND
   * RETURN ITS RESPONSE TO THE REQUESTING WIDGET.
   */
   
    bb = (XwBulletinWidget) w->core.parent;
    if (bb->manager.layout == XwSWINDOW)
      {
        manager = ((CompositeWidgetClass) (bb->core.parent->core.widget_class))
                      ->composite_class.geometry_manager;
         returnCode = (*manager)(w, request, reply);
         return(returnCode);
	}

    /* Since this is a bulletin board, all position requests 
     * are accepted.
     */
    if (request->request_mode & (CWX | CWY))
       XtMoveWidget( w, ((request->request_mode & CWX) == 0)
		              ? w->core.x : request->x ,
		         ((request->request_mode & CWY) == 0)
		              ? w->core.y : request->y );


	/* Fill out entire request record */
	if ((request->request_mode & CWWidth) == 0)
	    request->width = w->core.width;
	if ((request->request_mode & CWHeight) == 0)
	    request->height = w->core.height;
        if ((request->request_mode & CWBorderWidth) == 0)
	    request->border_width = w->core.border_width;

	/* Save current size and set to new size */
	width = w->core.width;
	height = w->core.height;
	borderWidth = w->core.border_width;
	w->core.width = request->width;
	w->core.height = request->height;
	w->core.border_width = request->border_width;

        geoFlag = DoLayout(bb);

        w->core.width = width;
        w->core.height = height;
        w->core.border_width = borderWidth;

        if (geoFlag)
	  {
            XtResizeWidget(w, request->width, request->height,
                               request->border_width);    
            return(XtGeometryDone);
	  }
        return (XtGeometryNo);

 }

/*************************************<->*************************************
 *
 *  ChangeManaged
 *
 *   Description:
 *   -----------
 *     Restructure the manager's "managed children" list.  Then re-lay
 *     out children.
 *
 *************************************<->***********************************/
static void ChangeManaged(bb)
    XwBulletinWidget bb;
{
    
    /* Put "managed children" info together again */

    _XwReManageChildren ((XwManagerWidget)bb);
    
    /* Reconfigure the bulletinboard */
    
    (void) DoLayout(bb);
}


