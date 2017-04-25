/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        MenuMgr.c
 **
 **   Project:     X Widgets
 **
 **   Description: Menu Manager Meta Class Widget
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
static void ClassInitialize();


/****************************************************************
 *
 * MenuPane Resources
 *
 ****************************************************************/

static XtResource resources[] = {
    {XtNassociateChildren, XtCAssociateChildren, XtRBoolean, sizeof(Boolean),
       XtOffset(XwMenuMgrWidget, menu_mgr.associateChildren), XtRString, 
       "TRUE"},

    {XtNmenuPost, XtCMenuPost, XtRString, sizeof(String),
       XtOffset(XwMenuMgrWidget, menu_mgr.postString), XtRString, 
       "<Btn1Down>"},

    {XtNmenuSelect, XtCMenuSelect, XtRString, sizeof(String),
       XtOffset(XwMenuMgrWidget, menu_mgr.selectString), XtRString, 
       "<Btn1Up>"},

    {XtNmenuUnpost, XtCMenuUnpost, XtRString, sizeof(String),
       XtOffset(XwMenuMgrWidget, menu_mgr.unpostString), XtRString, 
       NULL},

    {XtNkbdSelect, XtCKbdSelect, XtRString, sizeof(String),
       XtOffset(XwMenuMgrWidget, menu_mgr.kbdSelectString), XtRString, 
       "<Key>Select"}
};


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

XwMenuMgrClassRec XwmenumgrClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &XwmanagerClassRec,
    /* class_name         */    "XwMenuMgr",
    /* widget_size        */    sizeof(XwMenuMgrRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_init    */    NULL,
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
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,	
				NULL,
  },{
/* constraint class fields */
    /* resources          */    NULL,
    /* num_resources      */    0,
    /* constraint_size    */    0,
    /* initialize         */    NULL,
    /* destroy            */    NULL,
    /* set_values         */    NULL,
				NULL,
  },{
/* manager_class fields */
    /* traversal handler  */    NULL,
    /* translations       */    NULL,
  },{
/* menu manager class - none */     
    /* attachPane         */    NULL,
    /* detachPane         */    NULL,
    /* addPane            */    NULL,
    /* setSelectAccelerar */    NULL,
    /* clearSelectAcceler */    NULL,
    /* setPostMnemonic    */    NULL,
    /* clearPostMnemonic  */    NULL,
    /* addButton          */    NULL,
    /* processSelect      */    NULL,
    /* validEvent         */    NULL,
    /* doICascade         */    NULL,
    /* setSelectMnemonic  */    NULL,
    /* clearSelectMnemon  */    NULL,
    /* setTitleAttributes */    NULL,
    /* paneManagedChildren*/    NULL,
    /* traverseLeft       */    NULL,
    /* traverseRight      */    NULL,
    /* traverseNext       */    NULL,
    /* traversePrev       */    NULL,
    /* traverseHome       */    NULL,
    /* traverseUp         */    NULL,
    /* traverseDown       */    NULL,
    /* traverseNextTop    */    NULL,
    /* btnSensitivityChan */    NULL,
    /* paneSensitivityCha */    NULL,
 }	
};

WidgetClass XwmenumgrWidgetClass = (WidgetClass)&XwmenumgrClassRec;


/*************************************<->*************************************
 *
 *  ClassInitialize(parameters)
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

static void ClassInitialize ()
{
}


/*************************************<->*************************************
 *
 *  Initialize 
 *
 *
 *   Description:
 *   -----------
 *   Initialize the menu_mgr fields within the widget's instance structure.
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

    XwMenuMgrWidget request, new;

{
   KeySym tempKeysym;

   /* FOR NOW, MENUS DO NOT UNDERSTAND TRAVERSAL */
   /* new->manager.traversal_on = FALSE; */

   /*
    * Save a copy of the posting event string, and also convert it
    * into a form which can be compared against an X button event.
    */
   if ((new->menu_mgr.postString) &&
       (strlen(new->menu_mgr.postString) > 0))
   {
      if (_XwMapBtnEvent (new->menu_mgr.postString, 
                      &new->menu_mgr.postEventType,
                      &new->menu_mgr.postButton,
                      &new->menu_mgr.postModifiers) == FALSE)
      {
         /* String was not valid; use the default one */
         XtWarning("MenuMgr: Invalid post event; using <Btn1Down>");
         new->menu_mgr.postString = (String) strcpy (XtMalloc (XwStrlen
              ("<Btn1Down>") + 1), "<Btn1Down>");

         _XwMapBtnEvent ("<Btn1Down>", 
                         &new->menu_mgr.postEventType,
                         &new->menu_mgr.postButton,
                         &new->menu_mgr.postModifiers);
      }
      else
      {
         /* String was valid */
         new->menu_mgr.postString = (String) strcpy (XtMalloc (XwStrlen
              (new->menu_mgr.postString) + 1), new->menu_mgr.postString);
      }
   }
   else
   {
      new->menu_mgr.postString = NULL;
      new->menu_mgr.postModifiers = 0;
      new->menu_mgr.postButton = 0;
   }

   /*
    * Save a copy of the selecting event string, and also convert it
    * into a form which can be compared against an X button event.
    */
   if ((new->menu_mgr.selectString) &&
       (strlen(new->menu_mgr.selectString) > 0))
   {
      if (_XwMapBtnEvent (new->menu_mgr.selectString, 
                      &new->menu_mgr.selectEventType,
                      &new->menu_mgr.selectButton,
                      &new->menu_mgr.selectModifiers) == FALSE)
      {
         /* String was not valid; use the default one */
         XtWarning("MenuMgr: Invalid select event; using <Btn1Up>");
         new->menu_mgr.selectString = (String) strcpy (XtMalloc (XwStrlen
              ("<Btn1Up>") + 1), "<Btn1Up>");

         _XwMapBtnEvent ("<Btn1Up>", 
                         &new->menu_mgr.selectEventType,
                         &new->menu_mgr.selectButton,
                         &new->menu_mgr.selectModifiers);
      }
      else
      {
         /* String was valid */
         new->menu_mgr.selectString = (String) strcpy (XtMalloc (XwStrlen
              (new->menu_mgr.selectString) + 1), new->menu_mgr.selectString);
      }
   }
   else
   {
      new->menu_mgr.selectString = NULL;
      new->menu_mgr.selectModifiers = 0;
      new->menu_mgr.selectButton = 0;
   }

   /*
    * Save a copy of the unposting event string, and also convert it
    * into a form which can be compared against an X button event.
    */
   if ((new->menu_mgr.unpostString) &&
       (strlen(new->menu_mgr.unpostString) > 0))
   {
      if (_XwMapKeyEvent (new->menu_mgr.unpostString, 
                      &new->menu_mgr.unpostEventType,
                      &tempKeysym,
                      &new->menu_mgr.unpostModifiers) == FALSE)
      {
         /* String was not valid; use the default one */
         XtWarning("MenuMgr: Invalid unpost event; disabling option");
         new->menu_mgr.unpostString = NULL;
      }
      else
      {
         /* String was valid */
         new->menu_mgr.unpostKey = XKeysymToKeycode(XtDisplay(new),tempKeysym);
         new->menu_mgr.unpostString = (String) strcpy (XtMalloc (XwStrlen
              (new->menu_mgr.unpostString) + 1), new->menu_mgr.unpostString);
      }
   }
   else
   {
      new->menu_mgr.unpostString = NULL;
      new->menu_mgr.unpostModifiers = 0;
      new->menu_mgr.unpostKey = 0;
   }

   /*
    * Save a copy of the kbd select event string, and also convert it
    * into a form which can be compared against an X key event.
    */
   if ((new->menu_mgr.kbdSelectString) &&
       (strlen(new->menu_mgr.kbdSelectString) > 0))
   {
      if (_XwMapKeyEvent (new->menu_mgr.kbdSelectString, 
                      &new->menu_mgr.kbdSelectEventType,
                      &tempKeysym,
                      &new->menu_mgr.kbdSelectModifiers) == FALSE)
      {
         /* String was not valid; use the default one */
         XtWarning("MenuMgr: Invalid keyboard select event; using default");
         new->menu_mgr.kbdSelectString = (String) strcpy (XtMalloc (XwStrlen
              ("<Key>Select") + 1), "<Key>Select");

         _XwMapKeyEvent ("<Key>Select", 
                         &new->menu_mgr.kbdSelectEventType,
                         &new->menu_mgr.kbdSelectKey,
                         &new->menu_mgr.kbdSelectModifiers);
      }
      else
      {
         /* String was valid */
         new->menu_mgr.kbdSelectKey = 
                        XKeysymToKeycode(XtDisplay(new),tempKeysym);
         new->menu_mgr.kbdSelectString = (String) strcpy (XtMalloc (XwStrlen
              (new->menu_mgr.kbdSelectString) + 1), 
              new->menu_mgr.kbdSelectString);
      }
   }
   else
   {
      new->menu_mgr.kbdSelectString = NULL;
      new->menu_mgr.kbdSelectModifiers = 0;
      new->menu_mgr.kbdSelectKey = 0;
   }


   /* Allocate space for each of the tables used by a menu manager */
   new->menu_mgr.menuBtnAccelTable = 
       (XwKeyAccel *) XtMalloc(100 * sizeof(XwKeyAccel));
   new->menu_mgr.numAccels = 0;
   new->menu_mgr.sizeAccelTable = 100;

   new->menu_mgr.pendingAttachList = 
       (XwAttachList *) XtMalloc(25 * sizeof(XwAttachList));
   new->menu_mgr.numAttachReqs = 0;
   new->menu_mgr.sizeAttachList = 25;


   /* Set menu flags */
   new->menu_mgr.menuActive = FALSE;
} /* Initialize */


/*************************************<->*************************************
 *
 *  SetValues()
 *
 *   Description:
 *   -----------
 *
 *   This procedure simply checks to see if the post or select specification
 *   has changed, and allocates space for the new string and frees up the
 *   old string.  It will also map the new specification into a button/
 *   modifier pair.
 *
 *   NOTE: When a new post or select string is specified, this routine will
 *         allocate space for the new string, but will NOT free up the old 
 *         string; thus, our subclasses can check to see if these strings
 *         differ, and can remove any translations.  It is then upto the
 *         subclass to free the old string and to modify any translations or
 *         button grabs which are associated with the post or select actions.
 *
 *         It is also the responsibility of the subclass to check the state
 *         of the 'associate children' flag, and to modify any corresponding
 *         translations or grabs.
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

    XwMenuMgrWidget current, request, new;

{
   KeySym tempKeysym;

   /* FOR NOW, MENUS DO NOT UNDERSTAND TRAVERSAL */
   /* new->manager.traversal_on = FALSE; */

   /* Handle the case of a changed posting string */

   if (current->menu_mgr.postString != new->menu_mgr.postString)
   {
      /* Allocate a new buffer to hold the string */
      if ((new->menu_mgr.postString) &&
          (strlen(new->menu_mgr.postString) > 0))
      {
         if (_XwMapBtnEvent (new->menu_mgr.postString, 
                             &new->menu_mgr.postEventType,
                             &new->menu_mgr.postButton,
                             &new->menu_mgr.postModifiers) == FALSE)
         {
            /* Invalid post string; use the previous one */
            XtWarning("MenuMgr: Invalid post event; using previous setting");
            new->menu_mgr.postString = current->menu_mgr.postString;
            new->menu_mgr.postEventType = current->menu_mgr.postEventType;
            new->menu_mgr.postButton = current->menu_mgr.postButton;
            new->menu_mgr.postModifiers = current->menu_mgr.postModifiers;
         }
         else
         {
            /* Valid post string */
            new->menu_mgr.postString = (String)
                 strcpy (XtMalloc (XwStrlen (new->menu_mgr.postString) + 1),
                 new->menu_mgr.postString);
         }
      }
      else
      {
         new->menu_mgr.postString = NULL;
         new->menu_mgr.postEventType = 0;
         new->menu_mgr.postButton = 0;
         new->menu_mgr.postModifiers = 0;
      }

      /*
       * Don't free up the buffer holding the old post string.
       * This is to be done by our subclass.
       */
   }

   /* Handle the case of a changed select string */

   if (current->menu_mgr.selectString != new->menu_mgr.selectString)
   {
      /* Allocate a new buffer to hold the string */
      if ((new->menu_mgr.selectString) &&
          (strlen(new->menu_mgr.selectString) > 0))
      {
         if (_XwMapBtnEvent (new->menu_mgr.selectString, 
                             &new->menu_mgr.selectEventType,
                             &new->menu_mgr.selectButton,
                             &new->menu_mgr.selectModifiers) == FALSE)
         {
            /* Invalid select string; use the previous one */
            XtWarning("MenuMgr: Invalid select event; using previous setting");
            new->menu_mgr.selectString = current->menu_mgr.selectString;
            new->menu_mgr.selectEventType = current->menu_mgr.selectEventType;
            new->menu_mgr.selectButton = current->menu_mgr.selectButton;
            new->menu_mgr.selectModifiers = current->menu_mgr.selectModifiers;
         }
         else
         {
            /* Valid select string */
            new->menu_mgr.selectString = (String)
                 strcpy (XtMalloc (XwStrlen (new->menu_mgr.selectString) + 1),
                 new->menu_mgr.selectString);
         }
      }
      else
      {
         new->menu_mgr.selectString = NULL;
         new->menu_mgr.selectEventType = 0;
         new->menu_mgr.selectButton = 0;
         new->menu_mgr.selectModifiers = 0;
      }

      /*
       * Don't free up the buffer holding the old select string.
       * This is to be done by our subclass.
       */
   }

   /* Handle the case of a changed unposting string */

   if (current->menu_mgr.unpostString != new->menu_mgr.unpostString)
   {
      /* Allocate a new buffer to hold the string */
      if ((new->menu_mgr.unpostString) &&
          (strlen(new->menu_mgr.unpostString) > 0))
      {
         if (_XwMapKeyEvent (new->menu_mgr.unpostString, 
                             &new->menu_mgr.unpostEventType,
                             &tempKeysym,
                             &new->menu_mgr.unpostModifiers) == FALSE)
         {
            /* Invalid unpost string; use the previous one */
            XtWarning("MenuMgr: Invalid unpost event; using previous setting");
            new->menu_mgr.unpostString = current->menu_mgr.unpostString;
            new->menu_mgr.unpostEventType = current->menu_mgr.unpostEventType;
            new->menu_mgr.unpostKey = current->menu_mgr.unpostKey;
            new->menu_mgr.unpostModifiers = current->menu_mgr.unpostModifiers;
         }
         else
         {
            /* Valid unpost string */
            new->menu_mgr.unpostKey = XKeysymToKeycode(XtDisplay(new),
                                      tempKeysym);
            new->menu_mgr.unpostString = (String)
                 strcpy (XtMalloc (XwStrlen (new->menu_mgr.unpostString) + 1),
                 new->menu_mgr.unpostString);
         }
      }
      else
      {
         new->menu_mgr.unpostString = NULL;
         new->menu_mgr.unpostEventType = 0;
         new->menu_mgr.unpostKey = 0;
         new->menu_mgr.unpostModifiers = 0;
      }

      /*
       * Don't free up the buffer holding the old unpost string.
       * This is to be done by our subclass.
       */
   }

   /* Handle the case of a changed kbd select string */

   if (current->menu_mgr.kbdSelectString != new->menu_mgr.kbdSelectString)
   {
      /* Allocate a new buffer to hold the string */
      if ((new->menu_mgr.kbdSelectString) &&
          (strlen(new->menu_mgr.kbdSelectString) > 0))
      {
         if (_XwMapKeyEvent (new->menu_mgr.kbdSelectString, 
                             &new->menu_mgr.kbdSelectEventType,
                             &tempKeysym,
                             &new->menu_mgr.kbdSelectModifiers) == FALSE)
         {
            /* Invalid kbd select string; use the previous one */
            XtWarning
            ("MenuMgr: Invalid keyboard select event; using previous setting");
            new->menu_mgr.kbdSelectString = current->menu_mgr.kbdSelectString;
            new->menu_mgr.kbdSelectEventType = 
                              current->menu_mgr.kbdSelectEventType;
            new->menu_mgr.kbdSelectKey = current->menu_mgr.kbdSelectKey;
            new->menu_mgr.kbdSelectModifiers = 
                              current->menu_mgr.kbdSelectModifiers;
         }
         else
         {
            /* Valid kbd select string */
            new->menu_mgr.kbdSelectKey = XKeysymToKeycode(XtDisplay(new),
                                      tempKeysym);
            new->menu_mgr.kbdSelectString = (String)
               strcpy (XtMalloc (XwStrlen (new->menu_mgr.kbdSelectString) + 1),
               new->menu_mgr.kbdSelectString);
         }
      }
      else
      {
         new->menu_mgr.kbdSelectString = NULL;
         new->menu_mgr.kbdSelectEventType = 0;
         new->menu_mgr.kbdSelectKey = 0;
         new->menu_mgr.kbdSelectModifiers = 0;
      }

      /*
       * Don't free up the buffer holding the old kbd select string.
       * This is to be done by our subclass.
       */
   }

   return (FALSE);
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

    XwMenuMgrWidget  mw;

{
   /* Free up our resources */

   if (mw->menu_mgr.postString)
      XtFree (mw->menu_mgr.postString);

   if (mw->menu_mgr.selectString)
      XtFree (mw->menu_mgr.selectString);

   if (mw->menu_mgr.unpostString)
      XtFree (mw->menu_mgr.unpostString);

   if (mw->menu_mgr.kbdSelectString)
      XtFree (mw->menu_mgr.kbdSelectString);

   XtFree ((char *)(mw->menu_mgr.menuBtnAccelTable));
   XtFree ((char *)(mw->menu_mgr.pendingAttachList));
}

