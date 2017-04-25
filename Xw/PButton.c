/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        PButton.c
 **
 **   Project:     X Widgets
 **
 **   Description: Contains code for primitive widget class: PushButton
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
 * Include files & Static Routine Definitions
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <Xw/PButtonP.h>
#include <Xw/PButton.h>
#include <X11/keysymdef.h>   
   

#define PB_BW 2


static void Redisplay();
static void RedrawButtonFace();
static Boolean SetValues();
static void ClassInitialize();
static void Initialize();
static void Toggle();
static void ToggleRelease();
static void Select();
static void Unselect();
static void Resize();


/*************************************<->*************************************
 *
 *
 *   Description:  default translation table for class: PushButton
 *   -----------
 *
 *   Matches events with string descriptors for internal routines.
 *
 *************************************<->***********************************/

static char defaultTranslations[] =
   "<Btn1Down>:             select() \n\
    <Btn1Up>:             unselect() \n\
    <EnterWindow>:          enter() \n\
    <LeaveWindow>:          leave() \n\
    <KeyDown>Select:   select() \n\
    <KeyUp>Select:     unselect()";



/*************************************<->*************************************
 *
 *
 *   Description:  action list for class: PushButton
 *   -----------
 *
 *   Matches string descriptors with internal routines.
 *   Note that Primitive will register additional event handlers
 *   for traversal.
 *
 *************************************<->***********************************/

static XtActionsRec actionsList[] =
{
  {"toggle", (XtActionProc) Toggle},
  {"select", (XtActionProc) Select},
  {"unselect", (XtActionProc) Unselect},
  {"enter", (XtActionProc) _XwPrimitiveEnter},
  {"leave", (XtActionProc) _XwPrimitiveLeave},
};


/*  The resource list for Push Button  */

static XtResource resources[] = 
{     
   {
     XtNtoggle, XtCToggle, XtRBoolean, sizeof (Boolean),
     XtOffset (XwPushButtonWidget, pushbutton.toggle), XtRString, "False"
   }
};



/*************************************<->*************************************
 *
 *
 *   Description:  global class record for instances of class: PushButton
 *   -----------
 *
 *   Defines default field settings for this class record.
 *
 *************************************<->***********************************/

XwPushButtonClassRec XwpushButtonClassRec = {
  {
/* core_class fields */	
    /* superclass	  */	(WidgetClass) &XwbuttonClassRec,
    /* class_name	  */	"XwPushButton",
    /* widget_size	  */	sizeof(XwPushButtonRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_init    */    NULL,				
    /* class_inited       */	FALSE,
    /* initialize	  */	Initialize,
    /* initialize_hook    */    NULL,
    /* realize		  */	_XwRealize,
    /* actions		  */	actionsList,
    /* num_actions	  */	XtNumber(actionsList),
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterlv   */    TRUE,
    /* visible_interest	  */	FALSE,
    /* destroy		  */	NULL,
    /* resize		  */	Resize,
    /* expose		  */	Redisplay,
    /* set_values	  */	SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook  */	NULL,
    /* accept_focus	  */	NULL,
    /* version          */	XtVersion,
    /* callback_private */      NULL,
    /* tm_table         */      defaultTranslations,
    /* query_geometry   */	NULL, 
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  }
};
WidgetClass XwpushButtonWidgetClass = (WidgetClass)&XwpushButtonClassRec;


/*************************************<->*************************************
 *
 *  Toggle (w, event)                  PRIVATE
 *
 *   Description:
 *   -----------
 *     When this pushbutton is selected, toggle the activation state
 *     (i.e., draw it as active if it was not active and draw it as
 *     inactive if it was active).  Generate the correct callbacks
 *     in response.
 *
 *     NOTE: this code assumes that instances do not receive selection
 *           events when insensitive.
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
 *   XtCallCallbacks()  [libXtk]
 *   Redisplay  [PushButton.c]
 *************************************<->***********************************/

static void Toggle (w,event)
Widget w;
XEvent *event;

{
   XwPushButtonWidget pb = (XwPushButtonWidget)w;


   if (pb -> button.set) 
      pb -> button.set = False;
   else 
      pb -> button.set = True;

   RedrawButtonFace (w, event, FALSE);
   XFlush(XtDisplay(w));
   XtCallCallbacks (w, ((pb->button.set) ? XtNselect : XtNrelease), NULL);
}


/*************************************<->*************************************
 *
 *  Select (w, event)                  PRIVATE
 *
 *   Description:
 *   -----------
 *     Mark pushbutton as selected, (i.e., draw it as active)
 *     Generate the correct callbacks.  If "toggle" flag is
 *     TRUE then call the Toggle routine.
 *
 *     NOTE: this code assumes that instances do not receive selection
 *           events when insensitive.
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
 *   XtCallCallbacks()  [libXtk]
 *************************************<->***********************************/

static void Select (w,event)
Widget w;
XEvent *event;

{
   XwPushButtonWidget pb = (XwPushButtonWidget) w;

   if (pb->pushbutton.toggle == TRUE)
      Toggle(w,event);
   else
     {
       pb->button.set = TRUE;

       RedrawButtonFace (w, event, FALSE);
       XFlush (XtDisplay(w));
       XtCallCallbacks (w, XtNselect, NULL);
     }
}




/*************************************<->*************************************
 *
 *  Unselect (w, event)                  PRIVATE
 *
 *   Description:
 *   -----------
 *     When this pushbutton is unselected draw it as inactive.
 *     Generate the correct callbacks.
 *
 *     NOTE: this code respects the utility flag "toggle"  if this
 *           flag is set to true then this routine is a no op.
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
 *   XtCallCallbacks()   [libXtk]
 *************************************<->***********************************/

static void Unselect(w,event)
Widget w;
XEvent *event;

{
   XwPushButtonWidget pb = (XwPushButtonWidget) w;

 
   if (! pb->pushbutton.toggle)
     {
       pb->button.set = FALSE;

       RedrawButtonFace (w, event, FALSE);
       XFlush(XtDisplay(w));
       XtCallCallbacks (w, XtNrelease, NULL);
     }
}


static void ToggleRelease(w,event)
Widget w;
XEvent *event;

{
   XwPushButtonWidget pb = (XwPushButtonWidget) w;

   pb->button.set = FALSE;

   RedrawButtonFace (w, event, FALSE);
   XFlush(XtDisplay(w));
   XtCallCallbacks (w, XtNrelease, NULL);
}



/*************************************<->*************************************
 *
 *  Initialize 
 *
 *   Description:
 *   -----------
 *    If the core height and width fields are set to 0, treat that as a flag
 *    and compute the optimum size for this button.  Then using what ever
 *    the core fields are set to, compute the text placement fields.
 *    Make sure that the label location field is properly set for the
 *    Resize call.
 *
 *
 *   Inputs:
 *   ------
 *     request		=	request widget, old data.
 *
 *     new		=	new widget, new data; cumulative effect
 *				of initialize procedures.
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
   XwPushButtonWidget pb = (XwPushButtonWidget) new;

   /******************************************************************** 
       Needed width:
        2 * highlight thickness
        2 * button overhead: 2 lines for button outline
        2 * internal width (padding between label and button)
        width of label

      Needed height:
       2 * highlight thickness
       2 * button overhead: 2 lines for outline
       2 * internal height (padding)
       label height
    
    ************************************************************************/

   if (request->core.width == 0)  pb->core.width =  pb->button.label_width +
	      2 * ( pb->button.internal_width +    /* white space */
		     pb->primitive.highlight_thickness + PB_BW);
    
   if (request->core.height == 0) pb->core.height = pb->button.label_height + 
       2 * (pb->button.internal_height + pb->primitive.highlight_thickness + PB_BW);
    
   Resize(new);
}



/*************************************<->*************************************
 *
 *  ClassInitialize
 *
 *   Description:
 *   -----------
 *    Set fields in primitive class part of our class record so that
 *    the traversal code can invoke our button select procedures.
 *
 *************************************<->***********************************/

static void ClassInitialize()
{
   XwpushButtonClassRec.primitive_class.select_proc =  (XwEventProc) Select;
   XwpushButtonClassRec.primitive_class.release_proc = (XwEventProc) ToggleRelease;
   XwpushButtonClassRec.primitive_class.toggle_proc =  (XwEventProc) Toggle;
}



/*************************************<->*************************************
 *
 *  Redisplay (w, event, region)
 *
 *   Description:
 *   -----------
 *     Cause the widget, identified by w, to be redisplayed.
 *
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
 *   XDrawString()
 *************************************<->***********************************/

static void Redisplay (w, event, region)
Widget w;
XEvent *event;
Region region;

{
    RedrawButtonFace (w, event, TRUE);
}


static void RedrawButtonFace (w, event, all)
XwPushButtonWidget w;
XEvent *event;
Boolean all;

{
   register XwPushButtonWidget pb = (XwPushButtonWidget) w;
   int available_height, available_width;
   Boolean clipHeight, clipWidth;

   /* COMPUTE SPACE AVAILABLE FOR DRAWING LABEL */

   available_width = Max(0,(int)pb->core.width - 2*((int)pb->button.internal_width + pb->primitive.highlight_thickness + PB_BW));

   available_height = Max(0, (int)pb->core.height - 2*((int)pb->button.internal_height + pb->primitive.highlight_thickness + PB_BW));


   
   /* SEE IF WE NEED TO CLIP THIS LABEL ON TOP AND/OR BOTTOM */

   if (pb->button.label_width > available_width)
       clipWidth = True;
   else 
       clipWidth = False;


   if (pb->button.label_height > available_height)
       clipHeight = True;
   else 
       clipHeight = False;



   /* COMPUTE & DRAW PUSHBUTTON */



   /* COMPUTE x LOCATION FOR STRING & DRAW STRING                    */
    XFillRectangle (XtDisplay(w), XtWindow(w),
		         ((pb->button.set
                          )
                                     ? pb->button.normal_GC
		                     : pb->button.inverse_GC),
                      w -> primitive.highlight_thickness + 1 + PB_BW,
                      w -> primitive.highlight_thickness + 1 + PB_BW,
                      w->core.width- 2 * (w->primitive.highlight_thickness
                                          + 1 + PB_BW),
                      w->core.height- 2 * (w->primitive.highlight_thickness
                                          + 1 + PB_BW));

   if (pb->button.label_len > 0)
   {
      XDrawString(XtDisplay(w), XtWindow(w),
		   ((pb->button.set
                    )
                                     ? pb->button.inverse_GC
		                     : pb->button.normal_GC),
                   (((int)pb->core.width + 1 - pb->button.label_width) / 2),
	            pb->button.label_y,  pb->button.label, 
		    pb->button.label_len);


     if (clipWidth)
       {
         XClearArea (XtDisplay(w), XtWindow(w), 0,0,
		     (pb->primitive.highlight_thickness + PB_BW +
		       pb->button.internal_width), pb->core.height, FALSE);

         XClearArea (XtDisplay(w), XtWindow(w), 
		     ((int)pb->core.width - pb->primitive.highlight_thickness -
		       PB_BW - (int)pb->button.internal_width),0,
		     (pb->primitive.highlight_thickness + PB_BW +
		       pb->button.internal_width), pb->core.height, FALSE);
       }
   
     if (clipHeight)
       {
         XClearArea (XtDisplay(w), XtWindow(w), 0,0, pb->core.width, 
		     (pb->primitive.highlight_thickness + PB_BW +
		       pb->button.internal_height), FALSE);
         XClearArea (XtDisplay(w), XtWindow(w), 0,
		     ((int)pb->core.height - pb->primitive.highlight_thickness -
		       PB_BW - (int)pb->button.internal_height), pb->core.width,
		     (pb->primitive.highlight_thickness + PB_BW +
		       pb->button.internal_height), FALSE);
       }
   }


   _XwDrawBox (XtDisplay (w), XtWindow (w), 
                  w -> button.normal_GC, PB_BW,
                  w -> primitive.highlight_thickness,
                  w -> primitive.highlight_thickness,
                  w->core.width - 2 * w->primitive.highlight_thickness,
                  w->core.height - 2 * w->primitive.highlight_thickness);



   /*  
    * Draw traversal/enter highlight if actual exposure or
    * if we had to clip text area
    */

   if (all || clipWidth || clipHeight) 
   {
      if (pb->primitive.highlighted)
         _XwHighlightBorder(w);
      else if (pb->primitive.display_highlighted) 
         _XwUnhighlightBorder(w);
   }
}



/*************************************<->*************************************
 *
 *  SetValues(current, request, new)
 *
 *   Description:
 *   -----------
 *     This is the set values procedure for the pushbutton class.  It is
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
 *    last = TRUE if this is the last set values procedure to be called.
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Boolean SetValues(current, request, new)
Widget current, request, new;

{
    XtWidgetGeometry	reqGeo;
    XwPushButtonWidget curpb = (XwPushButtonWidget) current;
    XwPushButtonWidget newpb = (XwPushButtonWidget) new;
    Boolean  flag = FALSE;    /* our return value */
    
    
    /**********************************************************************
     * Calculate the window size:  The assumption here is that if
     * the width and height are the same in the new and current instance
     * record that those fields were not changed with set values.  Therefore
     * its okay to recompute the necessary width and height.  However, if
     * the new and current do have different width/heights then leave them
     * alone because that's what the user wants.
     *********************************************************************/
    if ((curpb->core.width == newpb->core.width) &&
         (_XwRecomputeSize(current, new)))
     {
	newpb->core.width =
	    newpb->button.label_width + 2*(newpb->button.internal_width +
     		    newpb->primitive.highlight_thickness + PB_BW);
	flag = TRUE;
     }

    if ((curpb->core.height == newpb->core.height) &&
         (_XwRecomputeSize(current, new)))
     {
	newpb->core.height =
	    newpb->button.label_height + 2*(newpb->button.internal_height +
     		    newpb->primitive.highlight_thickness + PB_BW);
	flag = TRUE;
     }

   return(flag);
}




/*************************************<->*************************************
 *
 *  Resize(w)
 *
 *   Description:
 *   -----------
 *     Recompute location of button text
 *
 *   Inputs:
 *   ------
 *     w  = widget to be resized.
 * 
 *
 *************************************<->***********************************/


static void Resize(w)
    Widget w;
{
    XwPushButtonWidget pb = (XwPushButtonWidget) w;

    pb->button.label_x = ((int)pb->core.width + 1 - pb->button.label_width) / 2;

    pb->button.label_y =
       ((int)pb->core.height - pb->button.label_height) / 2
	+ pb->button.font->max_bounds.ascent;
}
