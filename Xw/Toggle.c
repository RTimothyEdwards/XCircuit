/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        Toggle.c
 **
 **   Project:     X Widgets
 **
 **   Description: Contains code for primitive widget class: Toggle
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
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <Xw/ToggleP.h>
#include <Xw/Toggle.h>
#include <X11/StringDefs.h>
#include <X11/keysymdef.h>   
   
#define SPACE_FACTOR 3
#define T_BW 2

static void Redisplay();
static Boolean SetValues();
static void ClassInitialize();
static void Initialize();
static void Destroy();
static void Toggle();
static void Select();
static void Unselect();
static void DrawToggle();
static void Resize();
static void DrawDiamondButton();
static void GetSelectGC();



/*************************************<->*************************************
 *
 *
 *   Description:  default translation table for class: Toggle
 *   -----------
 *
 *   Matches events with string descriptors for internal routines.
 *
 *************************************<->***********************************/


static char defaultTranslations[] =
   "<EnterWindow>:          enter() \n\
    <LeaveWindow>:          leave() \n\
    <Btn1Down>:             toggle() \n\
    <Key>Select:            toggle()";



/*************************************<->*************************************
 *
 *
 *   Description:  action list for class: Toggle
 *   -----------
 *
 *   Matches string descriptors with internal routines.
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




/*************************************<->*************************************
 *
 *
 *   Description:  resource list for class: Toggle
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
      XtNsquare, XtCSquare, XtRBoolean, sizeof (Boolean),
      XtOffset (XwToggleWidget, toggle.square),
      XtRString, "True"
   },

   {
      XtNselectColor, XtCForeground, XtRPixel, sizeof (Pixel),
      XtOffset (XwToggleWidget, toggle.select_color),
      XtRString, "Black"
   }
};



/*************************************<->*************************************
 *
 *
 *   Description:  global class record for instances of class: Toggle
 *   -----------
 *
 *   Defines default field settings for this class record.
 *
 *************************************<->***********************************/

XwToggleClassRec XwtoggleClassRec = {
  {
/* core_class fields */	
    /* superclass	  */	(WidgetClass) &XwbuttonClassRec,
    /* class_name	  */	"Toggle",
    /* widget_size	  */	sizeof(XwToggleRec),
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
    /* destroy		  */	Destroy,
    /* resize		  */	Resize,
    /* expose		  */	Redisplay,
    /* set_values	  */	SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook  */	NULL,
    /* accept_focus     */      NULL,
    /* version          */	XtVersion,
    /* callback_private */      NULL,
    /* tm_table         */      defaultTranslations,
    /* query_geometry   */	NULL, 
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
   }
};
WidgetClass XwtoggleWidgetClass = (WidgetClass)&XwtoggleClassRec;

/*************************************<->*************************************
 *
 *  ComputeSpace (w, redisplay)
 *
 *   Description:
 *   -----------
 *     Compute allowable space to display label and/or toggle.
 *     If this function is used in redisplay (i.e. redisplay
 *     flag is true), compute space for label as well as toggle.
 *     Compute for toggle only if redisplay flag is FALSE.
 *
 *   Inputs:
 *   ------
 *     w           =   widget instance that was selected.
 *     redisplay   =   flag indicating routine is called from Redisplay()
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   XClearArea()
 *   _XwHighlightBorder()
 *   _XwUnhighlightBorder()
 *************************************<->***********************************/
static void ComputeSpace(w, redisplay)
     Widget w;
     Boolean redisplay;
{
   XwToggleWidget cbox = (XwToggleWidget)w;
   Boolean clipHeight, clipWidth;
   int x, available_width, available_height;
     
  /* COMPUTE SPACE FOR DRAWING TOGGLE */

   if (redisplay)
      {
         available_width = Max(0, (int)cbox->core.width - 2*
                                 ((int)cbox->button.internal_width +
	  		               cbox->primitive.highlight_thickness));

        /* SEE IF WE NEED TO CLIP THIS LABEL ON RIGHT */

         if (((int)cbox->button.label_width + 
              (int)cbox->button.label_height +
              (int)cbox->button.label_height/SPACE_FACTOR)  > available_width)
                clipWidth = True;
         else 
                clipWidth = False;

      }
   else 
      {
         available_width = Max(0,((int)cbox->core.width - 2*
                                 ((int)cbox->button.internal_width +
	           		       cbox->primitive.highlight_thickness) -
                                  (int)cbox->button.label_width -
                                  (int)cbox->button.label_height/SPACE_FACTOR));

        /* SEE IF WE NEED TO CLIP THIS LABEL ON RIGHT */

         if ((int)cbox->button.label_height > available_width)
             clipWidth = True;
         else
             clipWidth = False;
      }
 
   available_height = Max(0, (int)cbox->core.height - 2*
                            ((int)cbox->button.internal_height +
				  cbox->primitive.highlight_thickness));

  
  /* SEE IF WE NEED TO CLIP THIS LABEL ON TOP AND/OR BOTTOM */

    if ((int)cbox->button.label_height > available_height)
        clipHeight = True;
    else 
        clipHeight = False;

    if (clipWidth)
       {
          x = (int)cbox->core.width - cbox->primitive.highlight_thickness -
              (int)cbox->button.internal_width;
  
          if ((cbox->button.label_location != XwRIGHT) && redisplay)
             x -= Min((int)cbox->button.label_height, 
                      Max(0, (int)cbox->core.height - 
		      2*(cbox->primitive.highlight_thickness + 
                      (int)cbox->button.internal_height)));


          XClearArea (XtDisplay(w), XtWindow(w), x, 0,
 		      ((int)cbox->core.width - x), cbox->core.height, FALSE);
       }
   
    if (clipHeight)
       {
           XClearArea (XtDisplay(w), XtWindow(w), 0,0, cbox->core.width, 
		       (cbox->primitive.highlight_thickness +
		         cbox->button.internal_height), FALSE);
           XClearArea (XtDisplay(w), XtWindow(w), 0,
		       ((int)cbox->core.height - 
                        cbox->primitive.highlight_thickness -
		         (int)cbox->button.internal_height), cbox->core.width,
		       (cbox->primitive.highlight_thickness +
		         cbox->button.internal_height), FALSE);
       }

    if (cbox->primitive.highlighted)
         _XwHighlightBorder(w);
    else if (cbox->primitive.display_highlighted)
         _XwUnhighlightBorder(w);

    /* REDRAW THE TOGGLE ON LABEL LOCATION LEFT CONDITIONS AND RECURSIVELY
       CALL THIS FUNCTION.  THIS ENSURES VISIBILITY OF THE TOGGLE */
    if ((cbox->button.label_location != XwRIGHT) && redisplay)
      {
  	DrawToggle(w, FALSE);
  	ComputeSpace(w, FALSE);
      }

} /* ComputeSpace */


/*************************************<->*************************************
 *
 *  Toggle (w, event)                  PRIVATE
 *
 *   Description:
 *   -----------
 *     When this Toggle is selected, toggle the activation state
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
 *   DrawToggle()    [Toggle.c]
 *   ComputeSpace()    [Toggle.c]
 *   XtCallCallbacks()
 *************************************<->***********************************/

static void Toggle(w,event)
     Widget w;
     XEvent *event;
{
  XwToggleWidget cbox = (XwToggleWidget)w;

  cbox->button.set = (cbox->button.set == TRUE) ? FALSE : TRUE;
  DrawToggle(w, FALSE);
  ComputeSpace(w, FALSE);
  XFlush(XtDisplay(w));
  if (cbox->button.set == TRUE)
     XtCallCallbacks (w, XtNselect, NULL);
  else
     XtCallCallbacks (w, XtNrelease, NULL);
}


/*************************************<->*************************************
 *
 *  Select (w, event)                  PRIVATE
 *
 *   Description:
 *   -----------
 *     Mark Toggle as selected, (i.e., draw it as active)
 *     Generate the correct callbacks.
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
 *   DrawToggle()    [Toggle.c]
 *   ComputeSpace()    [Toggle.c]
 *   XtCallCallbacks()
 *************************************<->***********************************/

static void Select(w,event)
     Widget w;
     XEvent *event;
{
  XwToggleWidget cbox = (XwToggleWidget)w;

  cbox->button.set = TRUE;
  DrawToggle(w, FALSE);
  ComputeSpace(w,FALSE);
  XFlush(XtDisplay(w));
  XtCallCallbacks (w, XtNselect, NULL);
}


/*************************************<->*************************************
 *
 *  Unselect (w, event)                  PRIVATE
 *
 *   Description:
 *   -----------
 *     When this Toggle is unselected draw it as inactive.
 *     Generate the correct callbacks.
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
 *   DrawToggle()    [Toggle.c]
 *   ComputeSpace()    [Toggle.c]
 *   XtCallCallbacks()
 *************************************<->***********************************/

static void Unselect(w,event)
     Widget w;
     XEvent *event;
{
  XwToggleWidget cbox = (XwToggleWidget)w;

  cbox->button.set = FALSE;
  DrawToggle(w, FALSE);
  ComputeSpace(w,FALSE);
  XFlush(XtDisplay(w));
  XtCallCallbacks (w, XtNrelease, NULL);
}




/************************************************************************
 *
 *  GetSelectGC
 *	Get the graphics context to be used to fill the interior of
 *	a square or diamond when selected.
 *
 ************************************************************************/

static void GetSelectGC (tw)
XwToggleWidget tw;

{
   XGCValues values;

   values.foreground = tw -> toggle.select_color;
   values.background = tw -> core.background_pixel;
   values.fill_style = FillSolid;

   tw -> toggle.select_GC = 
      XtGetGC ((Widget)tw, GCForeground | GCBackground | GCFillStyle, &values);
               
}




/*************************************<->*************************************
 *
 *  Resize
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
 *************************************<->***********************************/

static void Resize(w)
    Widget w;
{
    register XwButtonWidget aButton = (XwButtonWidget) w;
    int direction = aButton->button.label_location;

       switch (direction)
	 {
            case XwRIGHT:
             aButton->button.label_x =  aButton->button.internal_width +
                                  aButton->primitive.highlight_thickness +
				  aButton->button.label_height +
				    aButton->button.label_height/SPACE_FACTOR;
             break;

           default:
            aButton->button.label_x = aButton->button.internal_width +
                                 aButton->primitive.highlight_thickness;
          }

    aButton->button.label_y =
       (((int)aButton->core.height - (int)aButton->button.label_height) >> 1)
 	 + aButton->button.font->max_bounds.ascent;
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
 *************************************<->***********************************/
static void Initialize (request, new)
 Widget request, new;
{
     XwToggleWidget cbox = (XwToggleWidget) new;

    if ((cbox->button.label_location != XwLEFT) &&
          (cbox->button.label_location != XwRIGHT))
      {
        cbox->button.label_location = XwLEFT;
	XtWarning ("XwToggle Initialize: invalid label location setting.");
      }

    if (request->core.width == 0)  cbox->core.width +=
           cbox->button.label_width +
	      2 * cbox->button.internal_width +    /* white space */
		   cbox->button.label_height +    /* size of box */
		       cbox->button.label_height/SPACE_FACTOR  +    /* space */
		    2 * cbox->primitive.highlight_thickness;
    
    if (request->core.height == 0) cbox->core.height +=
           cbox->button.label_height + 2 * cbox->button.internal_height
	      + 2 * cbox->primitive.highlight_thickness;  

/* dana change */
    if (!cbox->toggle.square) cbox->core.height += 2;

    GetSelectGC (new);
    Resize (new);
}   





/************************************************************************
 *
 *  Destroy
 *	Free toggle's graphic context.
 *
 ************************************************************************/

static void Destroy (tw)
XwToggleWidget tw;

{
   XtDestroyGC (tw -> toggle.select_GC);
}





/*************************************<->*************************************
 *
 *  DrawToggle(w)
 *
 *   Description:
 *   -----------
 *     Depending on the state of this widget, draw the Toggle.
 *
 *
 *   Inputs:
 *   ------
 *     w = widget to be (re)displayed.
 * 
 *************************************<->***********************************/

static void DrawToggle(w, all)
   XwToggleWidget w;
   Boolean all;         /* redo all, or just center? */
{
   int x, y, edge;
   
   /***************************************************************
    * Determine whether label should appear to the left or
    * the right of the box and set label X coordinate accordingly.
    ****************************************************************/

   edge = Min((int)w->button.label_height, 
	      Max(0, (int)w->core.height - 2*(w->primitive.highlight_thickness
		   + (int)w->button.internal_height)));

   if (w->button.label_location == XwRIGHT)
        x= w->button.internal_width + w->primitive.highlight_thickness;
   else
	x= Max(0, (int)w->core.width - (int)w->button.internal_width
	   - w->primitive.highlight_thickness - edge);      /* size of box */
   
   y= Max(0, ((int)w->core.height - edge) / 2); 

   if (w->toggle.square)
   {
      _XwDrawBox (XtDisplay (w), XtWindow (w), 
                     w -> button.normal_GC, T_BW,
                     x, y, edge, edge);
      if (edge > 6) XFillRectangle (XtDisplay ((Widget) w), 
                                    XtWindow ((Widget) w),
                                    ((w->button.set) ? w -> toggle.select_GC :
                                                       w -> button.inverse_GC),
                                    x+3, y+3, edge-6, edge-6);
   }
   else
      DrawDiamondButton ((XwButtonWidget) w, x-1, y-1, edge+2,
                         w -> button.normal_GC,
                         ((w->button.set) ? 
                           w -> toggle.select_GC :
                           w -> button.inverse_GC));
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
   XwtoggleClassRec.primitive_class.select_proc = (XwEventProc) Select;
   XwtoggleClassRec.primitive_class.release_proc = (XwEventProc) Unselect;
   XwtoggleClassRec.primitive_class.toggle_proc = (XwEventProc) Toggle;
}


/*************************************<->*************************************
 *
 *  Redisplay (w, event)
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
 *   DrawToggle()     [Toggle.c]
 *   ComputeSpace()   [Toggle.c]
 *************************************<->***********************************/

static void Redisplay(w, event)
    Widget w;
    XEvent *event;
{
   register XwToggleWidget cbox = (XwToggleWidget) w;

   if (cbox->button.label_len > 0)
          XDrawString( XtDisplay(w), XtWindow(w),  cbox->button.normal_GC,
	               cbox->button.label_x, cbox->button.label_y,
    	               cbox->button.label, cbox->button.label_len);
   DrawToggle(cbox, TRUE);
   ComputeSpace(w, TRUE);
}



/*************************************<->*************************************
 *
 *  SetValues(current, request, new)
 *
 *   Description:
 *   -----------
 *     This is the set values procedure for the Toggle class.  It is
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
 *
 *************************************<->***********************************/

static Boolean SetValues(current, request, new)
    Widget current, request, new;
{
    XwToggleWidget curcbox = (XwToggleWidget) current;
    XwToggleWidget newcbox = (XwToggleWidget) new;
    Boolean  flag = FALSE;    /* our return value */
    
    
    /**********************************************************************
     * Calculate the window size:  The assumption here is that if
     * the width and height are the same in the new and current instance
     * record that those fields were not changed with set values.  Therefore
     * its okay to recompute the necessary width and height.  However, if
     * the new and current do have different width/heights then leave them
     * alone because that's what the user wants.
     *********************************************************************/

    if ((curcbox->core.width == newcbox->core.width) &&
        (_XwRecomputeSize(current, new)))
     {
	newcbox->core.width =
	    newcbox->button.label_width +2*newcbox->button.internal_width
	       + newcbox->button.label_height +
		  newcbox->button.label_height/SPACE_FACTOR +
     		    2 * newcbox->primitive.highlight_thickness;
	flag = TRUE;
     }

    if ((curcbox->core.height == newcbox->core.height) &&
        (_XwRecomputeSize(current, new)))
     {
	newcbox->core.height =
	    newcbox->button.label_height + 2*newcbox->button.internal_height +
     		    2 * newcbox->primitive.highlight_thickness;
	flag = TRUE;
     }


    if (curcbox -> toggle.select_color != newcbox -> toggle.select_color)
    {
       XtDestroyGC (newcbox -> toggle.select_GC);
       GetSelectGC (newcbox);
       flag = True;
    }


    if (curcbox -> toggle.square != newcbox -> toggle.square)
    {
       flag = True;
    }


    Resize(new);  /* fix label x and label y positioning */
    return(flag);
}





/************************************************************************
 *
 *  DrawDiamondButton()
 *	The dimond drawing routine.  Used in place of primitives
 *	draw routine when toggle's square flag is False.
 *
 ************************************************************************/


static void DrawDiamondButton (bw, x, y, size, borderGC, centerGC)
XwButtonWidget bw;
int x, y, size;
GC borderGC, centerGC;


{
   XSegment seg[8];
   XPoint   pt[5];
   int midX, midY;

   if (size % 2 == 0)
      size--;

   midX = x + (size + 1) / 2;
   midY = y + (size + 1) / 2;


   /*  The top segments  */

   seg[0].x1 = x;			/*  1  */
   seg[0].y1 = midY - 1;
   seg[0].x2 = midX - 1;		/*  2  */
   seg[0].y2 = y;

   seg[1].x1 = x + 1;			/*  3  */
   seg[1].y1 = midY - 1;
   seg[1].x2 = midX - 1;		/*  4  */
   seg[1].y2 = y + 1;

   seg[2].x1 = midX - 1;		/*  5  */
   seg[2].y1 = y;
   seg[2].x2 = x + size - 1;		/*  6  */
   seg[2].y2 = midY - 1;

   seg[3].x1 = midX - 1;		/*  7  */
   seg[3].y1 = y + 1;
   seg[3].x2 = x + size - 2;		/*  8  */
   seg[3].y2 = midY - 1;


   /*  The bottom segments  */

   seg[4].x1 = x;			/*  9  */
   seg[4].y1 = midY - 1;
   seg[4].x2 = midX - 1;		/*  10  */
   seg[4].y2 = y + size - 1;

   seg[5].x1 = x + 1;			/*  11  */
   seg[5].y1 = midY - 1;
   seg[5].x2 = midX - 1;		/*  12  */
   seg[5].y2 = y + size - 2;

   seg[6].x1 = midX - 1;		/*  13  */
   seg[6].y1 = y + size - 1;
   seg[6].x2 = x + size - 1;		/*  14  */
   seg[6].y2 = midY - 1;

   seg[7].x1 = midX - 1;		/*  15  */
   seg[7].y1 = y + size - 2;
   seg[7].x2 = x + size - 2;		/*  16  */
   seg[7].y2 = midY - 1;

   XDrawSegments (XtDisplay ((Widget) bw), XtWindow ((Widget) bw),
                  borderGC, &seg[2], 2);

   XDrawSegments (XtDisplay ((Widget) bw), XtWindow ((Widget) bw),
                  borderGC, &seg[4], 4);

   XDrawSegments (XtDisplay ((Widget) bw), XtWindow ((Widget) bw),
                  borderGC, &seg[0], 2);


   pt[0].x = x + 3;
   pt[0].y = midY - 1;
   pt[1].x = midX - 1;
   pt[1].y = y + 3;
   pt[2].x = x + size - 4;
   pt[2].y = midY - 1;
   pt[3].x = midX - 2;
   pt[3].y = y + size - 4;

   XFillPolygon (XtDisplay ((Widget) bw), XtWindow ((Widget) bw),
                 centerGC, pt, 4, Convex, CoordModeOrigin);
}
