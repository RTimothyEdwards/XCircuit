/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        Button.c
 **
 **   Project:     X Widgets
 **
 **   Description: Code/Definitions for Button widget meta class.
 **
 ****************************************************************************
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
 *************************************<+>************************************/


/*
 * Include files & Static Routine Definitions
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>

static Boolean SetValues();
static void ButtonDestroy();


/*************************************<->*************************************
 *
 *
 *   Description:  resource list for class: Button
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
     XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
     XtOffset (XwButtonWidget, button.font), XtRString, "Fixed"
   },

   {
     XtNlabel, XtCLabel, XtRString, sizeof (caddr_t),
     XtOffset (XwButtonWidget, button.label), XtRString, NULL
   },

   {
     XtNlabelLocation, XtCLabelLocation, XtRLabelLocation, sizeof(int),
     XtOffset (XwButtonWidget, button.label_location), XtRString, "right"
   },

   {
     XtNvSpace, XtCVSpace, XtRDimension, sizeof(Dimension),
     XtOffset (XwButtonWidget, button.internal_height), XtRString, "2"
   },

   {
     XtNhSpace, XtCHSpace, XtRDimension, sizeof(Dimension),
     XtOffset (XwButtonWidget, button.internal_width), XtRString, "2"
   },

   {
     XtNset, XtCSet, XtRBoolean, sizeof (Boolean),
     XtOffset (XwButtonWidget, button.set), XtRString, "False"
   },

   {
      XtNsensitiveTile, XtCSensitiveTile, XtRTileType, sizeof (int),
      XtOffset(XwButtonWidget, button.sensitive_tile),
      XtRString, "foreground"
   },

   {
      XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
      XtOffset(XwButtonWidget,core.border_width), XtRString, "0"
   },
};




/*************************************<->*************************************
 *
 *
 *   Description:  global class record for instances of class: Button
 *   -----------
 *
 *   Defines default field settings for this class record.  Note that
 *   while Button needs a special type converter, it doesn't need to
 *   register that converter since Primitive takes care of registering
 *   all of the converters for the X Widgets library and Button is a
 *   subclass of Primitive.  
 *
 *************************************<->***********************************/

XwButtonClassRec XwbuttonClassRec =
{
   {
      (WidgetClass) &XwprimitiveClassRec,    /* superclass   */	
      "XwButton",                       /* class_name	     */	
      sizeof(XwButtonRec),              /* widget_size	     */	
      NULL,                             /* class_initialize  */    
      NULL,                             /* class part initialize */
      FALSE,                            /* class_inited      */	
      _XwInitializeButton,              /* initialize	     */	
      NULL,                             /* initialize hook   */
      NULL,                             /* realize	     */	
      NULL,                             /* actions           */	
      0,                                /* num_actions	     */	
      resources,                        /* resources	     */	
      XtNumber(resources),              /* num_resources     */	
      NULLQUARK,                        /* xrm_class	     */	
      TRUE,                             /* compress_motion   */	
      TRUE,                             /* compress_exposure */	
      TRUE,                             /* compress_enterleave */
      FALSE,                            /* visible_interest  */	
      ButtonDestroy,                    /* destroy           */	
      NULL,                             /* resize            */	
      NULL,                             /* expose            */	
      SetValues,                        /* set_values	     */	
      NULL,                             /* set_values_hook   */
      XtInheritSetValuesAlmost,         /* set_values_almost */
      NULL,                             /* get_values_hook   */
      NULL,                             /* accept_focus	     */
      XtVersion,                        /* toolkit version   */
      NULL,                             /* PRIVATE callback  */
      NULL,                             /* tm_table          */
      NULL,                             /* query_geometry    */
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
   },

 /* Inherit procedures for handling traversal and selection */
   {
	(XtWidgetProc) XtInheritBorderHighlight,
	(XtWidgetProc) XtInheritBorderUnhighlight,
	(XwEventProc)  XtInheritSelectProc,
	(XwEventProc)  XtInheritReleaseProc,
	(XwEventProc)  XtInheritToggleProc,
   },

   {
       0,         /* Button class part - unused */
   }
};

WidgetClass XwbuttonWidgetClass = (WidgetClass) &XwbuttonClassRec;



/*************************************<->*************************************
 *
 *  ButtonDestroy (bw)
 *
 *   Description:
 *   -----------
 *     Free up the space taken by the button's label and free up the
 *     button's graphic contexts.
 *
 *
 *   Inputs:
 *   ------
 *     bw = button (or subclass of button) to be destroyed.
 * 
 *************************************<->***********************************/
static void ButtonDestroy (bw)
  XwButtonWidget bw;
{
   if (bw->button.label)
      XtFree (bw->button.label);
   XtDestroyGC (bw->button.normal_GC);
   XtDestroyGC (bw->button.inverse_GC);
}   

/*************************************<->*************************************
 *
 *  SetValues(current, request, new, last)
 *
 *   Description:
 *   -----------
 *     This is the set values procedure for the button class.
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
    XwButtonWidget curcbox = (XwButtonWidget) current;
    XwButtonWidget newcbox = (XwButtonWidget) new;
    Boolean  flag = FALSE;    /* our return value */
    int  GCflag = 0;

#define NewNormal 1
#define NewInverse 2
    
    
   /************************************************************
    * If the button setting has been changed then set flag so that
    * button will be redisplayed.  NOTE THAT IF THE APPLICATION 
    * WANTS THE CALLBACKS ASSOCIATED WITH THE BUTTON TO BE INVOKED
    * IT WILL HAVE TO DIRECTLY CALL XTCALLCALLBACKS.
    ************************************************************/
    if (curcbox->button.set != newcbox->button.set)    flag = TRUE;


   /************************************************************
    * If the font has been changed then we need to
    * recompute the text width and height. We'll also have
    * to change other things, see below...
    ************************************************************/
    if (curcbox->button.font != newcbox->button.font)
       {
 	     _XwSetTextWidthAndHeight(newcbox);
	     flag = TRUE;
       }


   /**************************************************************
    * If a new label has been provided then we'll need to copy it
    * into our data space.
    * Free up any space previously allocated for the label name.
    * NOTE: We allow them to specify a NULL label.
    ***************************************************************/
    if (curcbox->button.label != newcbox->button.label)
     {
	/* Set label length, height and width */
	_XwSetTextWidthAndHeight(newcbox);

	if (newcbox->button.label)
	   newcbox->button.label = strcpy(
	        XtMalloc((unsigned) newcbox->button.label_len + 1),
		newcbox->button.label);

	if (curcbox->button.label)
	   XtFree ((char *) curcbox->button.label);
	flag = TRUE;
      }


    
    /**********************************************************************
     * If the label location flag has changed set the redisplay flag TRUE
     * and check that it has a valid setting.  If its invalid, then issue
     * a warning.
     ********************************************************************/
    if (curcbox->button.label_location != newcbox->button.label_location)
       {
	  flag = TRUE;
	  if ((newcbox->button.label_location != XwLEFT) &&
	      (newcbox->button.label_location != XwRIGHT) &&
	      (newcbox->button.label_location != XwCENTER))
	     {
		XtWarning("SetValues: Invalid Label Location Setting.");
		newcbox->button.label_location =
		   curcbox->button.label_location;
	     }
       }

   /*********************************************************************
    * If the sensitive tile has changed, validate it. Then, if the
    * foreground or background or font or sensitive tile has changed
    * then recompute/get a new (In)sensitive GC.
    ********************************************************************/
    if (newcbox->button.sensitive_tile != XwFOREGROUND)
      {
       newcbox->button.sensitive_tile = curcbox->button.sensitive_tile;
       XtWarning("SetValues: Invalid sensitive tile setting.");
       }

    if (curcbox->primitive.foreground != newcbox->primitive.foreground ||
	curcbox->core.background_pixel != newcbox->core.background_pixel)
      {
        /* GET ALL NEW GC's */
        GCflag = NewNormal | NewInverse;
      }

    if (curcbox->button.font->fid != newcbox->button.font->fid)
      {
          /* GET NEW GC's FOR  normal & inverse */
          GCflag = NewNormal | NewInverse;
      }

    if ((curcbox->button.sensitive_tile != newcbox->button.sensitive_tile) &&
           (! newcbox->core.sensitive || ! newcbox->core.ancestor_sensitive))
	 {
          /* GET NEW normal GC */
          GCflag |= NewNormal;
	 }

    if ((newcbox->core.sensitive != curcbox->core.sensitive) &&
        ((newcbox->core.sensitive && newcbox->core.ancestor_sensitive) ||
         (!newcbox->core.sensitive && curcbox->core.ancestor_sensitive)))
	 {
          /* GET NEW normal GC */
          GCflag |= NewNormal;
	 }
    else if (newcbox->core.sensitive &&
        (curcbox->core.ancestor_sensitive != newcbox->core.ancestor_sensitive))
	 {
          /* GET NEW normal GC */
          GCflag |= NewNormal;
	 }



    if (GCflag & NewNormal)
      {
	XtDestroyGC(curcbox->button.normal_GC);
	_XwGetNormalGC(newcbox);
        flag = TRUE;
      }
    if (GCflag & NewInverse)
      {
	XtDestroyGC(curcbox->button.inverse_GC);
        _XwGetInverseGC(newcbox);
	flag = TRUE;
        }


 /********************************************************************
  * NOTE THAT SUBCLASSES OF BUTTON WILL HAVE TO CHECK FOR CHANGES IN 
  * internal_width, internal_height AND DETERMINE WHAT SHOULD BE DONE.
  * WE'RE JUST BEING GOOD CITIZENS HERE AND FLAGING THAT IT WILL HAVE
  * TO BE REDISPLAYED BECAUSE OF THE CHANGE.
  *********************************************************************/
    if ((curcbox->button.internal_width
	  != newcbox->button.internal_width) ||
	(curcbox->button.internal_height
	  != newcbox->button.internal_height))  flag = TRUE;


    return( flag );
}



/*************************************<->*************************************
 *
 *  void _XwSetTextWidthAndHeight (aButton)
 *
 *   Description:
 *   -----------
 *   Calculate width and height of displayed text in pixels.
 *   Sets the following fields in the button portion of a button
 *   instance record:
 *
 *         label_len
 *         label_height
 *         label_width
 *
 *   Inputs:
 *   ------
 *     aButton = XwButtonWidget
 * 
 *   Procedures Called
 *   -----------------
 *   XwStrlen()  [a macro: #define XwStrlen(s)	((s) ? strlen(s) : 0) ]
 *   XTextWidth()
 *************************************<->***********************************/

void _XwSetTextWidthAndHeight(aButton)
    XwButtonWidget aButton;
{
    register XFontStruct	*fs = aButton->button.font;

    aButton->button.label_height =
       fs->max_bounds.ascent + fs->max_bounds.descent;

    if (aButton->button.label)
    {
       aButton->button.label_len = XwStrlen(aButton->button.label);
    
       aButton->button.label_width = XTextWidth(
			                      fs, aButton->button.label,
				              (int) aButton->button.label_len);
    }
    else
    {
       aButton->button.label_len = 0;
       aButton->button.label_width = 0;
    }
}

/*************************************<->*************************************
 *
 *  void _XwGetNormalGC(aButton)
 *          XwButtonWidget  aButton;
 *
 *   Description:
 *   -----------
 *      Uses the widget specific foreground to generate the "normal"
 *      graphic context.  Note that this is a XToolkit sharable GC.
 *      Creates the needed GC and sets ptr. in instance record.
 *
 *   Inputs:
 *   ------
 *     aButton = widget instance.
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   XtGetGC
 *************************************<->***********************************/

void _XwGetNormalGC(aButton)
    XwButtonWidget aButton;
{
    XGCValues	values;

    values.foreground	= aButton->primitive.foreground;
    values.background   = aButton->core.background_pixel;
    values.font		= aButton->button.font->fid;

    values.line_width   = 1;
    aButton->button.normal_GC = XtGetGC((Widget)aButton,
     	                                  (unsigned) GCForeground |
					  (unsigned) GCBackground |
					             GCLineWidth  |
					   GCFont,
       	                                    &values);
}

/*************************************<->*************************************
 *
 *  void _XwGetInverseGC(aButton)
 *          XwButtonWidget  aButton;
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 *     aButton = widget instance.
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *   XwGetGC
 *************************************<->***********************************/

void _XwGetInverseGC(aButton)
    XwButtonWidget aButton;
{
    XGCValues	values;
    values.foreground	= aButton->core.background_pixel;
    values.background   = aButton->primitive.foreground;
    values.font		= aButton->button.font->fid;
    values.line_width   = 1;
    
    aButton->button.inverse_GC = XtGetGC((Widget)aButton,
     	       (unsigned) GCForeground | (unsigned) GCBackground |
	       GCLineWidth  | GCFont, &values);
}

/*************************************<->*************************************
 *
 *  _XwInitializeButton (request, new)
 *
 *   Description:
 *   -----------
 *     This is the button instance initialize procedure.  Make sure
 *     that the button has a label, compute its height and width fields;
 *     set the appropriate graphic contexts.
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

void _XwInitializeButton (request, new)
Widget request, new;

{
    XwButtonWidget aButton = (XwButtonWidget) new;

   /*
    * NOTE: we don't allow a NULL string for our button label.
    * If we do find a NULL at initialize time then we will use the
    * button name.  The rationale for this is that this allows someone
    * to change the button label in the .Xdefaults file, whereas if
    * the application programmer hardwired the name into an arg list
    * it would be impossible to change.  However, if you really want
    * a NULL string, do a SetValues and change the label to NULL.
    * 
    * We need to malloc space for this string and copy it to our
    * space.  The toolkit simply copies the pointer to the string.
    */
    if (aButton->button.label == NULL) aButton->button.label = 
                                                    aButton->core.name;

    aButton->button.label = strcpy( XtMalloc((unsigned)
				     (XwStrlen(aButton->button.label)+1)),
		 		                    aButton->button.label);
	

    /*
     * make sure that sensitive_tile is valid
     */
    if (aButton->button.sensitive_tile != XwFOREGROUND)
      {
         aButton->button.sensitive_tile = XwFOREGROUND;
         XtWarning("Initialize: Invalid sensitive tile setting.");
       }

    
    _XwGetInverseGC(aButton);
    _XwGetNormalGC(aButton);

    _XwSetTextWidthAndHeight(aButton);

    aButton->primitive.display_sensitive = FALSE;
    aButton->primitive.highlighted = FALSE;


} /* _XwInitializeButton */


/*************************************<->*************************************
 *
 *  _XwRealize
 *
 *   Description:
 *   -----------
 *   This is a generalize routine used by both subclasses of Primitive
 *   and Manager.  It sets bit gravity to NW for managers and to Forget
 *   for others, but for Buttons is sets it according to the setting of
 *   label location.
 *
 *  Inputs
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
 *   XCreateWindow()
 *************************************<->***********************************/

void _XwRealize(w, p_valueMask, attributes)
    register Widget w;
    Mask *p_valueMask;
    XSetWindowAttributes *attributes;
{
   Mask valueMask = *p_valueMask;
    valueMask |= CWBitGravity;

    if (XtIsSubclass(w, XwprimitiveWidgetClass)) {
	register XwPrimitiveWidget pw = (XwPrimitiveWidget) w;
	if (pw->primitive.cursor != None) valueMask |= CWCursor;
	attributes->cursor = pw->primitive.cursor;
    }

    if (XtIsSubclass(w, XwmanagerWidgetClass))
      attributes->bit_gravity = NorthWestGravity;
    else
      if (w->core.widget_class != XwbuttonWidgetClass)
         attributes->bit_gravity = ForgetGravity;
      else
        switch (((XwButtonWidget)w)->button.label_location)
	  {
	  case XwRIGHT: 
               attributes->bit_gravity = EastGravity; break;
          case XwLEFT:
               attributes->bit_gravity = WestGravity; break;
          case XwCENTER:
               attributes->bit_gravity = CenterGravity; break;
          }

   XtCreateWindow(w, InputOutput, (Visual *)CopyFromParent,
	valueMask, attributes);

   _XwRegisterName(w);  /* hang widget name as property on window */

} /* _XwRealize */


/*************************************<->*************************************
 *
 *  _XwResizeButton(w)
 *
 *   Description:
 *   -----------
 *     A resize event has been generated. Recompute location of button
 *     elements.
 *
 *     USED BY CHECKBOX AND PUSHBUTTON CLASSES.
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


void _XwResizeButton(w)
    Widget w;
{
    register XwButtonWidget aButton = (XwButtonWidget) w;
    int direction = aButton->button.label_location;

       switch (direction)
	 {
          case XwCENTER:
            aButton->button.label_x = ((int)aButton->core.width + 1 - 
				  (int)aButton->button.label_width) / 2;
            break;

            case XwRIGHT:
             aButton->button.label_x = (int)aButton->core.width - 
	                         (int)aButton->button.internal_width -
                                  aButton->primitive.highlight_thickness -
	 		         (int)aButton->button.label_width;
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
 *  ProcedureName (parameters)
 *
 *   Description:
 *   -----------
 *    This procedure is used to store the name of the widget as a
 *    property on the widget's window.  The property name is the
 *    string "XW_NAME". Note that we do not malloc space for the
 *    widget's name, we merely use the pointer to this string which
 *    is already in core.name.  This shouldn't be a problem since
 *    the name will remain as long as the widget exists.
 *
 *    We only want to do this one time, thus the hokey method of
 *    initializing a static boolean which prevents us from registering
 *    this atom more than once on the server.
 *
 *    If possible, we should put this together with other functions done
 *    once.  Also, WILL THIS BE A PROBLEM WHEN THE TOOLKIT ALLOWS MORE
 *    THAN ONE DISPLAY TO BE OPENED?????
 *
 *   Inputs:
 *   ------
 *     w = widget being created.
 * 
 *   Outputs:
 *   -------
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Atom widget_name;
static Boolean first=True;

void _XwRegisterName(w)
 Widget w;
{
  if (first)
   {
      widget_name = XInternAtom (XtDisplay(w), "XW_NAME", False);
      first = False;
   }
   XChangeProperty (XtDisplay(w), XtWindow(w), widget_name, XA_STRING, 8,
		     PropModeReplace, w->core.name,
		       XwStrlen(w->core.name)+1);
}


/*************************************<->*************************************
 *  Boolean
 *  _XwRecomputeSize(current, new)
 *
 *   Description:
 *   -----------
 *     Used during SetValues for PButton and Toggle.
 *     If the font has changed OR the label has changed OR
 *     the internal spacing has changed OR the highlight
 *     thickness has changed AND the recompute flag is TRUE
 *     (in the new widget) return TRUE, else return FALSE.
 *
 *
 *   Inputs:
 *   ------
 *    current = current version of widget
 *    new = new version of widget
 * 
 *   Outputs:
 *   -------
 *     TRUE if resize is needed and okay, FALSE otherwise.
 *
 *************************************<->***********************************/
 Boolean _XwRecomputeSize(current, new)
  XwButtonWidget current, new;
{
  if (((new->button.font != current->button.font) ||
     (new->button.label != current->button.label) ||
     (new->primitive.highlight_thickness != 
         current->primitive.highlight_thickness) ||
     (new->button.internal_height != current->button.internal_height) ||
     (new->button.internal_width != current->button.internal_width)) &&
     (new->primitive.recompute_size == TRUE))
         return(TRUE);
  else
         return(FALSE);
}



