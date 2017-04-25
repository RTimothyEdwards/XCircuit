/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        MenuPane.c
 **
 **   Project:     X Widgets
 **
 **   Description: Menu Pane Meta Class Widget
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
#include <Xw/Xw.h>
#include <Xw/XwP.h>

static void Initialize();
static void Destroy();
static Boolean SetValues();
static void GetFontGC();
static void InsertChild();
static void ConstraintInitialize();
static void ClassPartInitialize();
static void SetTraversalFlag();


/****************************************************************
 *
 * MenuPane Resources
 *
 ****************************************************************/

static XtResource resources[] = {
    {XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
       XtOffset(XwMenuPaneWidget, menu_pane.title_font), XtRString, "fixed"},

    {XtNtitleString, XtCTitleString, XtRString, sizeof(caddr_t),
       XtOffset(XwMenuPaneWidget, menu_pane.title_string), XtRString, NULL},

    {XtNtitleImage, XtCTitleImage, XtRImage, sizeof(XImage *),
       XtOffset(XwMenuPaneWidget, menu_pane.titleImage), XtRImage, NULL},

    {XtNtitleShowing, XtCTitleShowing, XtRBoolean, sizeof(Boolean),
       XtOffset(XwMenuPaneWidget, menu_pane.title_showing), XtRString, "TRUE"},

    {XtNattachTo, XtCAttachTo, XtRString, sizeof(String),
       XtOffset(XwMenuPaneWidget, menu_pane.attach_to), XtRString, NULL},

    {XtNmgrTitleOverride, XtCTitleOverride, XtRBoolean, sizeof(Boolean),
       XtOffset(XwMenuPaneWidget, menu_pane.title_override), XtRString, 
       "FALSE"},

    {XtNtitleType, XtCTitleType, XtRTitleType, sizeof(int),
      XtOffset(XwMenuPaneWidget, menu_pane.title_type), XtRString, "string"},

    {XtNmnemonic, XtCMnemonic, XtRString, sizeof(String),
       XtOffset(XwMenuPaneWidget, menu_pane.mnemonic),XtRString,NULL},

    {XtNselect, XtCCallback, XtRCallback, sizeof(caddr_t),
       XtOffset(XwMenuPaneWidget, menu_pane.select), XtRPointer, NULL}
};


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

XwMenuPaneClassRec XwmenupaneClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &XwmanagerClassRec,
    /* class_name         */    "XwMenuPane",
    /* widget_size        */    sizeof(XwMenuPaneRec),
    /* class_initialize   */    NULL,
    /* class_part_init    */    ClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize         */    Initialize,
    /* initialize_hook    */    NULL,
    /* realize            */    NULL,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterlv   */	TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    NULL,
    /* expose             */    NULL,
    /* set_values         */    SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook    */    NULL,
    /* accept_focus       */    NULL,
    /* version            */    XtVersion,
    /* PRIVATE cb list    */    NULL,
    /* tm_table           */    NULL,
    /* query_geometry     */    NULL,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },{
/* composite_class fields */
    /* geometry_manager   */    NULL,
    /* change_managed     */    NULL,
    /* insert_child	  */	InsertChild,
    /* delete_child	  */	XtInheritDeleteChild,	
				NULL,
  },{
/* constraint class fields */
    /* resources          */    NULL,
    /* num_resources      */    0,
    /* Constraint_size    */    sizeof (Boolean),
    /* initialize         */    ConstraintInitialize,
    /* destroy            */    NULL,
    /* set_values         */    NULL,
				NULL,
  },{
/* manager_class fields */
   /* traversal handler   */    NULL,
   /* translations        */    NULL,
  },{
/* menu pane class - none */     
   /* setTraversalFlag    */    (XwWBoolProc) SetTraversalFlag
 }	
};

WidgetClass XwmenupaneWidgetClass = (WidgetClass)&XwmenupaneClassRec;


/*************************************<->*************************************
 *
 *  ProcedureName (parameters)
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

static void ClassPartInitialize (wc)

   register XwMenuPaneWidgetClass wc;

{
  if (wc->constraint_class.initialize == XtInheritMenuPaneConstraintInit)
     wc->constraint_class.initialize = (XtInitProc) ClassPartInitialize;

  if (wc->menu_pane_class.setTraversalFlag == XtInheritSetTraversalFlagProc)
     wc->menu_pane_class.setTraversalFlag = (XwWBoolProc) SetTraversalFlag;
}


/*************************************<->*************************************
 *
 *  ProcedureName (parameters)
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

static void ConstraintInitialize (request, new)

   Widget request;
   Widget new;

{
   Boolean * managed_and_mapped;

   managed_and_mapped = (Boolean *) new->core.constraints;

   /* Widgets are never managed when they are first created */
   *managed_and_mapped = FALSE;
}


/*************************************<->*************************************
 *
 *  ProcedureName (parameters)
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

static void InsertChild (w)
   Widget     w;
{
   CompositeWidgetClass composite;
   Widget parent, grandparent, greatgrandparent;

   /* Invoke our superclass's insert_child procedure */
   composite = (CompositeWidgetClass)
         XwmenupaneWidgetClass->core_class.superclass;

   (*(composite->composite_class.insert_child)) (w);

   /* Call menu mgr's AddButton() procedure */
   if ((parent = (Widget) XtParent(w)) &&
       (grandparent = (Widget) XtParent(parent)) &&
       (greatgrandparent = (Widget) XtParent(grandparent)) &&
       (XtIsSubclass (greatgrandparent, XwmenumgrWidgetClass)))
   {
      (*((XwMenuMgrWidgetClass)XtClass(greatgrandparent))->menu_mgr_class.
              addButton) (greatgrandparent, w);
   }
}


/*************************************<->*************************************
 *
 *  Initialize 
 *
 *
 *   Description:
 *   -----------
 *   Initialize the menu_pane fields within the widget's instance structure.
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
    XwMenuPaneWidget request, new;
{
    Widget parent, grandparent;
    KeySym tempKeysym;

    /*
     * Always force traversal off for a menupane.  We inherit our
     * traversal setting from our menu manager, and it will take
     * care of setting us to the correct state.
     */
    new->manager.traversal_on = FALSE;

    new->menu_pane.attachId = NULL;

    /* Call menu mgr's AddPane() procedure */
    if ((parent = (Widget) XtParent(new)) &&
        (grandparent = (Widget) XtParent(parent)) &&
        (XtIsSubclass (grandparent, XwmenumgrWidgetClass)))
    {
       (*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.addPane)
                 (grandparent, (Widget)new);
    }
    else
       grandparent = NULL;


    /* Get the GC used to display the title string/image */
    GetFontGC (new);

    /* Save a copy of the title string, if present */
    if (new->menu_pane.title_string)
    {
       new->menu_pane.title_string = (String)
               strcpy(XtMalloc(XwStrlen(new->menu_pane.title_string) + 1),
               new->menu_pane.title_string);
    }
    else
    {
       /* Set a default title string: the widget name */
       new->menu_pane.title_string = (String)
               strcpy(XtMalloc(XwStrlen(new->core.name) + 1),
               new->core.name);
    }

    /* Save a copy of the accelerator string, if present */
    if ((new->menu_pane.mnemonic) && (*(new->menu_pane.mnemonic) != '\0'))
    {
       /* Valid mnemonic specified */
       char mne = new->menu_pane.mnemonic[0];

       new->menu_pane.mnemonic = (String)XtMalloc(2);
       new->menu_pane.mnemonic[0] = mne;
       new->menu_pane.mnemonic[1] = '\0';

       /* Register it with the menu manager */
       if (grandparent)
       {
          (*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.
             setPostMnemonic) (grandparent, (Widget)new, new->menu_pane.mnemonic);
       }
    }
    else
    {
       if (new->menu_pane.mnemonic)
          XtWarning ("MenuPane: Invalid mnemonic; disabling feature");
       new->menu_pane.mnemonic = NULL;
    }

    /* Save a copy of the attach string, if present */
    if (new->menu_pane.attach_to)
    {
       new->menu_pane.attach_to = (String)
               strcpy(XtMalloc(XwStrlen(new->menu_pane.attach_to) + 1),
               new->menu_pane.attach_to);

       /* Call menu mgr's AttachPane() procedure */
       if (grandparent)
       {
          (*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.
                attachPane) (grandparent, (Widget)new, new->menu_pane.attach_to);
       }
    }

} /* Initialize */


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
    XwMenuPaneWidget current, request, new;
{
   Boolean redrawFlag = FALSE;
   Boolean titleAttributeChange = FALSE;
   Widget parent, grandparent;
   KeySym tempKeysym;

   /* We never allow our traversal flag to change using SetValues() */
   new->manager.traversal_on = current->manager.traversal_on;

   /* Determine if we are being controlled by a menu manager */
   if (((parent = (Widget) XtParent(new)) == NULL) ||
       ((grandparent = (Widget) XtParent(parent)) == NULL) ||
       (!XtIsSubclass (grandparent, XwmenumgrWidgetClass)))
   {
      grandparent = NULL;
   }

   if ((new->menu_pane.title_type != XwSTRING) &&
       (new->menu_pane.title_type != XwIMAGE))
   {
      XtWarning("MenuPane: Invalid title type; using previous setting");
      new->menu_pane.title_type = current->menu_pane.title_type;
   }

   if ((current->menu_pane.title_showing != new->menu_pane.title_showing) ||
      (current->menu_pane.title_override != new->menu_pane.title_override))
   {
      redrawFlag = TRUE;
   }

   /* Handle the case of a changed title string or image */

   if (current->menu_pane.title_string != new->menu_pane.title_string)
   {
      /* Allocate a new buffer to hold the string */
      if (new->menu_pane.title_string)
      {
         new->menu_pane.title_string = (String)
              strcpy (XtMalloc (XwStrlen (new->menu_pane.title_string) + 1),
              new->menu_pane.title_string);
      }
      else
      {
         /* Assign a default title: the widget name */
         new->menu_pane.title_string = (String)
              strcpy (XtMalloc (XwStrlen (new->core.name) + 1),
              new->core.name);
      }

      /*
       * Free up the buffer holding the old title string.
       * XtFree() is smart enough to catch a NULL pointer.
       */
      XtFree (current->menu_pane.title_string);

      if (new->menu_pane.title_type == XwSTRING)
      {
         titleAttributeChange = TRUE;

         if (new->menu_pane.title_showing)
            redrawFlag = TRUE;
      }
   }

   if (current->menu_pane.titleImage != new->menu_pane.titleImage)
   {
      /*
       * Depending upon whether the depth of the title image can change,
       * we may need to generate a new GC, which will be used by our call
       * to XPutImage().  However, I don't believe this is the case.
       */

      if (new->menu_pane.title_type == XwIMAGE)
      {
         titleAttributeChange = TRUE;

         if (new->menu_pane.title_showing)
            redrawFlag = TRUE;
      }
   }

   if (current->menu_pane.title_type != new->menu_pane.title_type)
   {
      redrawFlag = TRUE;
      titleAttributeChange = TRUE;
   }


   /* Determine if the menu manager needs to attach us elsewhere */

   if (current->menu_pane.attach_to != new->menu_pane.attach_to)
   {
      /* Allocate a new buffer to hold the string */
      if (new->menu_pane.attach_to)
      {
         new->menu_pane.attach_to = (String)
              strcpy (XtMalloc (XwStrlen (new->menu_pane.attach_to) + 1),
              new->menu_pane.attach_to);
      }

      /* Call menu mgr's DetachPane() procedure */
      if (grandparent)
      {
         (*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.
              detachPane) (grandparent, (Widget)new, current->menu_pane.attach_to);
      }

      /*
       * Free up the buffer holding the old attach string.
       * XtFree() is smart enough to catch a NULL pointer.
       */
      XtFree (current->menu_pane.attach_to);

      /* Call menu mgr's AttachPane() procedure */
      if (grandparent)
      {
         (*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.
              attachPane) (grandparent, (Widget)new, new->menu_pane.attach_to);
      }
   }

   /* Determine if the post mnemonic needs to be changed */

   if (current->menu_pane.mnemonic != new->menu_pane.mnemonic)
   {
      if (new->menu_pane.mnemonic)
      {
         if (*(new->menu_pane.mnemonic) == '\0')
         {
            /* Invalid string; revert to previous one */
            XtWarning 
              ("MenuPane: Invalid post mnemonic; using previous setting");
            new->menu_pane.mnemonic = current->menu_pane.mnemonic;
         }
         else
         {
            /* Valid string; remove old mnemonic, and add new one */
            char mne = new->menu_pane.mnemonic[0];

            new->menu_pane.mnemonic = (String) XtMalloc(2);
            new->menu_pane.mnemonic[0] = mne;
            new->menu_pane.mnemonic[1] = '\0';

            if (grandparent && current->menu_pane.mnemonic)
            {
               (*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.
                    clearPostMnemonic) (grandparent, (Widget)current);
            }

            if (grandparent) 
            {
               (*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.
                  setPostMnemonic) (grandparent, (Widget)new, 
                                    new->menu_pane.mnemonic);
            }
            XtFree (current->menu_pane.mnemonic);
         }
      }
      else
      {
         /* Remove old accelerator */
         if (grandparent && current->menu_pane.mnemonic)
         {
            (*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.
                 clearPostMnemonic) (grandparent, (Widget)current);
         }

         new->menu_pane.mnemonic = NULL;

         XtFree(current->menu_pane.mnemonic);
      }
   }


   /* Force a redraw and get a new GC if the font attributes change */
 
   if ((current->manager.foreground != new->manager.foreground) ||
       (current->menu_pane.title_font->fid != new->menu_pane.title_font->fid))
   {
      XtDestroyGC (current->menu_pane.titleGC);
      GetFontGC (new);
      redrawFlag = TRUE;
      titleAttributeChange = TRUE;
   }

   if ((current->core.background_pixel != new->core.background_pixel) ||
       (current->manager.background_tile != new->manager.background_tile))
   {
      titleAttributeChange = TRUE;
   }

   if (((new->core.sensitive != current->core.sensitive) ||
       (new->core.ancestor_sensitive != current->core.ancestor_sensitive)) && 
       grandparent)
   {
      /* Call menu mgr's paneSensitivityChanged() procedure */
      (*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.
            paneSensitivityChanged) (grandparent, (Widget)new);
   }

   if (titleAttributeChange && grandparent)
   {
      /* Call menu mgr's SetTitleAttributes() procedure */
      (*((XwMenuMgrWidgetClass)XtClass(grandparent))->menu_mgr_class.
            setTitleAttributes) (grandparent, (Widget)new);
   }


   return (redrawFlag);
}


/*************************************<->*************************************
 *
 *  GetFontGC
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

static void GetFontGC (mw)

    XwMenuPaneWidget  mw;

{
   XGCValues values;

   values.foreground = mw->manager.foreground;
   values.background = mw->core.background_pixel;
   values.font = mw->menu_pane.title_font->fid;
   values.line_width = 1;
   values.line_style = LineSolid;
   values.function = GXcopy;
   values.plane_mask = AllPlanes;
   values.subwindow_mode = ClipByChildren;
   values.clip_x_origin = 0;
   values.clip_y_origin = 0;
   values.clip_mask = None;

   mw->menu_pane.titleGC = XtGetGC ((Widget)mw, GCForeground | GCFont |
                           GCFunction | GCLineWidth | GCLineStyle |
                           GCBackground | GCPlaneMask | GCSubwindowMode |
                           GCClipXOrigin | GCClipYOrigin | GCClipMask, 
                           &values);
}


/*************************************<->*************************************
 *
 *  Destroy
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

static void Destroy (mw)

    XwMenuPaneWidget  mw;

{
   /* Free up our resources */

   if (mw->menu_pane.title_string)
      XtFree (mw->menu_pane.title_string);

   if (mw->menu_pane.attach_to)
      XtFree (mw->menu_pane.attach_to);

   if (mw->menu_pane.mnemonic)
      XtFree (mw->menu_pane.mnemonic);

   XtDestroyGC (mw->menu_pane.titleGC);

   XtRemoveCallbacks ((Widget)mw, XtNselect, mw->menu_pane.select);
}


/*************************************<->*************************************
 *
 *  _XwSetMappedManagedChildrenList (parameters)
 *
 *   Description:
 *   -----------
 *     MenuPane widgets are only interested in using their children
 *     which are both managed and mapped_when_managed, during layout
 *     calculations.  This global routine traverses the list of 
 *     children for the specified manager widget, and constructs a
 *     parallel list of only those children meeting the above criteria.
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

void _XwSetMappedManagedChildrenList (mw)

   XwManagerWidget mw;

{
   int i;

   mw->manager.num_managed_children = 0;

   for (i = 0; i < mw->composite.num_children; i++)
   {
      if ((mw->composite.children[i]->core.managed) &&
          (mw->composite.children[i]->core.mapped_when_managed))
      {
         /* Add to list */
         if ((mw->manager.num_managed_children + 1) >
             mw->manager.num_slots)
         {
            /* Grow the list */
            mw->manager.num_slots += XwBLOCK;
            mw->manager.managed_children = (WidgetList)
              XtRealloc ((caddr_t)mw->manager.managed_children,
               (mw->manager.num_slots * sizeof(Widget)));
         }

         mw->manager.managed_children [mw->manager.num_managed_children++] =
               mw->composite.children[i];
      }
   }

   /* 
    * In case this has been called during a SetValues on the pane,
    * we want to make sure that both the new and old widgets get
    * properly updated.
    */
   ((XwManagerWidget)mw->core.self)->manager.num_managed_children = 
                       mw->manager.num_managed_children;
   ((XwManagerWidget)mw->core.self)->manager.managed_children = 
                       mw->manager.managed_children;
}


/*************************************<->*************************************
 *
 *  SetTraversalFlag(parameters)
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

static void SetTraversalFlag (w, enableFlag)

   XwMenuPaneWidget w;
   Boolean enableFlag;

{
   w->manager.traversal_on = enableFlag;

   if (enableFlag == True)
   {
      XtAugmentTranslations ((Widget)w, XwmanagerClassRec.manager_class.
                                translations);
      w->core.widget_class->core_class.visible_interest = True;
   }
}


/*************************************<->*************************************
 *
 *  _XwAllAttachesAreSensitive(parameters)
 *
 *   Description:
 *   -----------
 *     This functions verifies that the path from a menubutton back upto
 *     the top level menupane has no insensitive widgets (panes or buttons)
 *     in it.
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

Boolean _XwAllAttachesAreSensitive (menuBtn)

   Widget menuBtn;

{
   XwMenuPaneWidget parent;

   while (menuBtn != NULL)
   {
      parent = (XwMenuPaneWidget)XtParent(menuBtn);

      if (!XtIsSensitive((Widget)parent) || !XtIsSensitive(menuBtn))
         return (False);

      menuBtn = (Widget) parent->menu_pane.attachId;
   }

   return (True);
}
