/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        PopupMgr.c
 **
 **   Project:     X Widgets
 **
 **   Description: Popup Menu Manager Widget
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
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Shell.h>
#include <X11/ShellP.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <Xw/MenuBtnP.h>
#include <Xw/MenuBtn.h>
#include <Xw/PopupMgrP.h>
#include <Xw/PopupMgr.h>

   
/* Standard Toolkit Routines */
static void Initialize();
static void Destroy();
static Boolean SetValues();
static void ClassPartInitialize();

/* Global Routines */
static void AttachPane();
static void DetachPane();
static void AddPane();
static void SetSelectAccelerator();
static void ClearSelectAccelerator();
static void AddButton();
static void Unpost();
static void Post();
static Boolean ProcessSelect();
static Boolean ValidEvent();
static Boolean DoICascade();
static void ClassPost();
static void ClassSelect();
static Boolean DoYouWantThisAccelerator();
static Boolean DoYouWantThisPost();
static void SetPostMnemonic();
static void ClearPostMnemonic();
static void SetSelectMnemonic();
static void ClearSelectMnemonic();
static void SetTitleAttributes();
static void TraverseLeft();
static void TraverseRight();
static void TraverseNext();
static void TraversePrev();
static void TraverseHome();
static void TraverseUp();
static void TraverseDown();
static void TraverseNextTop();
static void BtnSensitivityChanged();
static void PaneSensitivityChanged();

/* Action Routines */
static void MMPost();
static void MMAccelPost();
static void MMAccelSelect();
static void MMUnpost();

/* Callback Routines */
static void _XwMenuPaneCleanup();
static void _XwMenuButtonCleanup();
static void _XwCascadeUnselect();
static void _XwCascadeSelect();

/* Internal Support Routines */
static void RegisterTranslation();
static void ExposeEventHandler();
static void RegisterPostTranslation();
static void PositionCascade();
static Boolean ActionIsUsurpedByChild();
static void AddToPendingList();
static void SetButtonAccelerators();
static void SetTreeAccelerators();
static void ClearTreeAccelerators();
static Boolean CompletePath();
static Widget PendingAttach();
static void SetUpTranslation();
static void ForceMenuPaneOnScreen();
static void PaneManagedChildren();
static Boolean Visible();
static void SetMenuTranslations();
static void ManualPost();
static void SendFakeLeaveNotify();
static void SetUpStickyList();
static void DoDetach();


/****************************************************************
 *
 * Popup Resources
 *
 ****************************************************************/

static XtResource resources[] = {
    {XtNstickyMenus, XtCStickyMenus, XtRBoolean, sizeof(Boolean),
       XtOffset(XwPopupMgrWidget, popup_mgr.stickyMode),XtRString,"FALSE"},

    {XtNpostAccelerator, XtCPostAccelerator, XtRString, sizeof(String), 
       XtOffset(XwPopupMgrWidget, popup_mgr.postAccelerator),XtRString,
       NULL}
};

/****************************************************************
 *
 * Miscellaneous Values
 *
 ****************************************************************/

/* Global Action Routines */
static XtActionsRec XwPopupActions[] = {
   {"PopupPostMenu",    MMPost},
   {"PopupUnpostMenu",  MMUnpost},
   {"PopupAccelPost",   MMAccelPost},
   {"PopupAccelSelect", MMAccelSelect}
};

/* Translation Template Strings */
static String postTemplate = ": PopupPostMenu(";
static String unpostTemplate = ": PopupUnpostMenu(";
static String accelPostTemplate = ": PopupAccelPost(";
static String accelSelectTemplate = ": PopupAccelSelect(";
static String selectTemplate = ": select()";

static char workArea[300];


/*
 * Used when positioning a cascade such that the cursor will be
 * in the first item of the cascade.
 */
#define FUDGE_FACTOR 8

#define SHELL_PARENT 0
#define ULTIMATE_PARENT 1

/*
 * This mask is used during the setting and clearing of button grabs.
 * If the event being grabbed is a buttonUp event, then these bits
 * cause the grabs to never activate; so they need to be cleared.
 */
#define btnMask (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

XwPopupMgrClassRec XwpopupmgrClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &XwmenumgrClassRec,
    /* class_name         */    "XwPopupMgr",
    /* widget_size        */    sizeof(XwPopupMgrRec),
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
   /* traversal handler   */    (XwTraversalProc)XtInheritTraversalProc,
   /* translations        */    NULL,
  },{
/* menu manager class */     
    /* attachPane         */    AttachPane,
    /* detachPane         */    DetachPane,
    /* addPane            */    AddPane,
    /* setSelectAccel     */    SetSelectAccelerator,
    /* clearSelectAccel   */    ClearSelectAccelerator,
    /* setPostMnemonic    */    SetPostMnemonic,
    /* clearPostMnemonic  */    ClearPostMnemonic,
    /* addButton          */    AddButton,
    /* processSelect      */    (XwBooleanProc)ProcessSelect,
    /* validEvent         */    (XwBooleanProc)ValidEvent,
    /* doIcascade         */    DoICascade,
    /* setSelectMnemonic  */    SetSelectMnemonic,
    /* clearSelectMnemon  */    ClearSelectMnemonic,
    /* setTitleAttributes */    SetTitleAttributes,
    /* paneManagedChildren*/    PaneManagedChildren,
    /* traverseLeft       */    TraverseLeft,
    /* traverseRight      */    TraverseRight,
    /* traverseNext       */    TraverseNext,
    /* traversePrev       */    TraversePrev,
    /* traverseHome       */    TraverseHome,
    /* traverseUp         */    TraverseUp,
    /* traverseDown       */    TraverseDown,
    /* traverseNextTop    */    TraverseNextTop,
    /* btnSensitivityChan */    BtnSensitivityChanged,
    /* paneSensitivityCha */    PaneSensitivityChanged,
  },{
/* popup menu manager class - none */     
    /* manualPost         */    (XwPostProc) ManualPost,
  }
};

WidgetClass XwpopupmgrWidgetClass = (WidgetClass)&XwpopupmgrClassRec;
WidgetClass XwpopupMgrWidgetClass = (WidgetClass)&XwpopupmgrClassRec;

/*----------------------------------------------------------------------*/
/* I don't really know why PopupMgr needs to register its actions       */
/* globally like this when all other widgets don't;  regardless,        */
/* running this in ClassInitialize() doesn't work because the app       */
/* context can be destroyed and remade, and then it loses the actions,  */
/* instead returning "Warning: Actions not found: PopupPostMenu".       */
/* Furthermore, the "correct" implementation needs a value for "app",   */
/* which must be declared global, which is clearly a hack.  So I'm      */
/* instead creating a world-accessible function which does what         */
/* ClassInitialize() did, but it takes an argument "app" and must be    */
/* called every time the application is (re)made.                       */
/*----------------------------------------------------------------------*/
 
void XwAppInitialize(XtAppContext app)
{
   /* Register the global action routines */
   XtAppAddActions (app, XwPopupActions, XtNumber(XwPopupActions));
}


/*************************************<->*************************************
 *
 *  Initialize 
 *
 *
 *   Description:
 *   -----------
 *   Initialize the popup_mgr fields within the widget's instance structure.
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

    XwPopupMgrWidget request;
    register XwPopupMgrWidget new;

{
   Widget grandparent;
   KeySym tempKeysym;

   /*
    * Clear the 'map when managed' flag, since this widget should
    * never be visible.
    */
   new->core.mapped_when_managed = FALSE;

   /* Fill in our instance fields */
   new->popup_mgr.topLevelPane = (Widget) NULL;
   new->popup_mgr.lastSelected = (Widget) NULL;
   new->popup_mgr.savedCascadeList = (Widget *) XtMalloc(25 * sizeof(Widget));
   new->popup_mgr.numSavedCascades = 0;
   new->popup_mgr.sizeSavedCascadeList = 25;
   new->popup_mgr.currentCascadeList = (Widget *) XtMalloc(25 * sizeof(Widget));
   new->popup_mgr.numCascades = 0;
   new->popup_mgr.sizeCascadeList = 25;
   new->popup_mgr.attachPane = (XwMenuPaneWidget) NULL;
   new->popup_mgr.detachPane = (XwMenuPaneWidget) NULL;
   new->popup_mgr.origMouseX = -1;
   new->popup_mgr.origMouseY = -1;

   /* Get the widget Id for the widget to which the menu is attached */
   grandparent = (Widget) XtParent(XtParent(new));

   /*
    * If a post accelerator is specified, then attempt to convert it to
    * a usable format (not a string); if the accelerator specification is
    * invalid, then use the default post accelerator (none).
    */
   if ((new->popup_mgr.postAccelerator) && 
       (strlen(new->popup_mgr.postAccelerator) > 0) &&
       (_XwMapKeyEvent (new->popup_mgr.postAccelerator,
                        &new->popup_mgr.accelEventType,
                        &tempKeysym,
                        &new->popup_mgr.accelModifiers)))
   {
      /* Valid accelerator string; save a copy */
      new->popup_mgr.accelKey = XKeysymToKeycode(XtDisplay(new),
                                     tempKeysym);
      new->popup_mgr.postAccelerator = (String) strcpy (XtMalloc (
         XwStrlen(new->popup_mgr. postAccelerator) + 1), 
         new->popup_mgr.postAccelerator);

      /* Set up a translation in the widget */
      RegisterTranslation (grandparent, new->popup_mgr.postAccelerator,
                           accelPostTemplate, new);
   }
   else
   {
      if ((new->popup_mgr.postAccelerator) &&
          (strlen(new->popup_mgr.postAccelerator) > 0))
      {
         XtWarning ("PopupMgr: Invalid post accelerator; disabling feature");
      }
      new->popup_mgr.postAccelerator = NULL;
      new->popup_mgr.accelEventType = 0;
      new->popup_mgr.accelKey = 0;
      new->popup_mgr.accelModifiers = 0;
   }

   /* Attach a posting translation to the widget */
   if (new->menu_mgr.postString)
   {
      RegisterTranslation (grandparent, new->menu_mgr.postString,
                           postTemplate, new);
   }

   /* Attach an unposting translation to the widget, if specified */
   if (new->menu_mgr.unpostString)
   {
      RegisterTranslation (grandparent, new->menu_mgr.unpostString,
                            unpostTemplate, new);
   }

   /*
    * If the children of the widget are to inherit this menu, then we
    * need to set up a button grab for the posting event; if the widget
    * is not yet realized, then we cannot set the grab, so we will
    * attach an exposure event handler to the widget, so that when it
    * does finally get mapped, we can set up any necessary grabs.
    */
   if (XtIsRealized(grandparent))
   {
      if (new->menu_mgr.associateChildren)
      {
         /* Set up a grab for the post event */
         if (new->menu_mgr.postString)
         {
            XGrabButton (XtDisplay(grandparent), new->menu_mgr.postButton,
                         (new->menu_mgr.postModifiers & ~btnMask),
                         XtWindow(grandparent),
                         False, ButtonPressMask|ButtonReleaseMask,
                         GrabModeAsync, GrabModeAsync, (Window)NULL, (Cursor)NULL);
         }

         /* Set up a grab for the post accelerator */
         if (new->popup_mgr.postAccelerator)
         {
            XGrabKey (XtDisplay(grandparent), 
                      new->popup_mgr.accelKey,
                      new->popup_mgr.accelModifiers, XtWindow(grandparent),
                      False, GrabModeAsync, GrabModeAsync);
         }
      }
   }
   else
   {
      XtAddEventHandler (grandparent, ExposureMask|StructureNotifyMask, False, 
                         ExposeEventHandler, new);
   }
} /* Initialize */


/*************************************<->*************************************
 *
 *  SetValues()
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

    register XwPopupMgrWidget current; 
    XwPopupMgrWidget request; 
    register XwPopupMgrWidget new;

{
   Widget  grandparent;
   Boolean postGrabSet = FALSE; 
   Boolean postGrabCleared = FALSE;
   Boolean postAccelGrabSet = FALSE;
   Boolean postAccelGrabCleared = FALSE;
   KeySym tempKeysym;
   XtTranslations translation;
   String workTemplate;
   register int i, j, k;

   grandparent = (Widget) XtParent(XtParent(current));

   /* Process the post accelerator */
   if (current->popup_mgr.postAccelerator != new->popup_mgr.postAccelerator)
   {
      if ((new->popup_mgr.postAccelerator) &&
          (strlen(new->popup_mgr.postAccelerator) > 0))
      {
         if (_XwMapKeyEvent (new->popup_mgr.postAccelerator,
                             &new->popup_mgr.accelEventType,
                             &tempKeysym,
                             &new->popup_mgr.accelModifiers))
         {
            /* Valid accelerator string; save a copy */
            new->popup_mgr.accelKey = XKeysymToKeycode (XtDisplay(new),
                                      tempKeysym);
            new->popup_mgr.postAccelerator = (String) strcpy (XtMalloc (
               XwStrlen(new->popup_mgr. postAccelerator) + 1), 
               new->popup_mgr.postAccelerator);
         }
         else
         {
            /* Invalid; revert to previous setting */
            XtWarning 
              ("PopupMgr: Invalid post accelerator; using previous setting");
            new->popup_mgr.postAccelerator = current->popup_mgr.postAccelerator;
            new->popup_mgr.accelEventType = current->popup_mgr.accelEventType;
            new->popup_mgr.accelKey = current->popup_mgr.accelKey;
            new->popup_mgr.accelModifiers = current->popup_mgr.accelModifiers;
         }
      }
      else
      {
         /* None specified, so disable the feature */
         new->popup_mgr.postAccelerator = NULL;
         new->popup_mgr.accelEventType = 0;
         new->popup_mgr.accelKey = 0;
         new->popup_mgr.accelModifiers = 0;
      }

      /*
       * This duplicate check prevents us from doing alot of unnecessary
       * work in the case where an invalid string was specified, and thus
       * we reverted to the previous value.
       */
      if (current->popup_mgr.postAccelerator != new->popup_mgr.postAccelerator)
      {
         /* Remove the old accelerator translation and grab */
         if (current->popup_mgr.postAccelerator)
         {
            RegisterTranslation (grandparent, 
                                 current->popup_mgr.postAccelerator,
                                 accelPostTemplate, NULL);

            if (XtIsRealized(grandparent) &&current->menu_mgr.associateChildren)
            {
               XUngrabKey (XtDisplay(grandparent), current->popup_mgr.accelKey,
                    current->popup_mgr.accelModifiers, XtWindow(grandparent));
               postAccelGrabCleared = TRUE;
            }
         }

         /* Set the new accelerator translation and grab */
         if (new->popup_mgr.postAccelerator)
         {
            RegisterTranslation (grandparent, new->popup_mgr.postAccelerator,
                               accelPostTemplate, new);

            if (XtIsRealized(grandparent) && new->menu_mgr.associateChildren)
            {
               XGrabKey (XtDisplay(grandparent), new->popup_mgr.accelKey,
                         new->popup_mgr.accelModifiers, XtWindow(grandparent),
                         False, GrabModeAsync, GrabModeAsync);
               postAccelGrabSet = TRUE;
            }
         }

         /* Free up the buffer holding the old string */
         XtFree (current->popup_mgr.postAccelerator);
      }
   }

   /* Process the post event  */
   if (current->menu_mgr.postString != new->menu_mgr.postString)
   {
      /* Remove the old post translation and grab */
      if (current->menu_mgr.postString)
      {
         RegisterTranslation (grandparent, current->menu_mgr.postString,
                              postTemplate, NULL);

         if (XtIsRealized(grandparent) && current->menu_mgr.associateChildren)
         {
            XUngrabButton (XtDisplay(grandparent), current->menu_mgr.postButton,
                           (current->menu_mgr.postModifiers & ~btnMask),
                           XtWindow(grandparent));
            postGrabCleared = TRUE;
         }

         /* Free up the old string */
         XtFree (current->menu_mgr.postString);
      }

      /* Set the new post translation and grab */
      if (new->menu_mgr.postString)
      {
         RegisterTranslation (grandparent, new->menu_mgr.postString,
                            postTemplate, new);

         if (XtIsRealized(grandparent) && new->menu_mgr.associateChildren)
         {
            XGrabButton (XtDisplay(grandparent), new->menu_mgr.postButton,
                         (new->menu_mgr.postModifiers & ~btnMask),
                         XtWindow(grandparent),
                         False, ButtonPressMask|ButtonReleaseMask,
                         GrabModeAsync, GrabModeAsync, (Window)NULL, (Cursor)NULL);
            postGrabSet = TRUE;
         }
      }
   }

   /* Process the select event.  */
   if (current->menu_mgr.selectString != new->menu_mgr.selectString)
   {

      /* 
       * Each menubutton and menupane must have a new 'select' translation
       * set up; no grabs are needed.  The old ones do not need to be
       * removed, since ProcessSelect() will reject them.
       */

      if (new->menu_mgr.selectString)
      {
         /* 
          * Create the translation string of format:
          *
          *    "!<event>: select()"
          */
         workTemplate = &workArea[0];
         strcpy (workTemplate, "!");
         strcat (workTemplate, new->menu_mgr.selectString);
         strcat (workTemplate, selectTemplate);
         translation = XtParseTranslationTable (workTemplate);
         SetMenuTranslations (new, translation);
         /* XtDestroyStateTable (XtClass(new), translation); */
      }

       if (current->menu_mgr.selectString)
         XtFree (current->menu_mgr.selectString);
   }

   /* Process the unpost event.  */
   if (current->menu_mgr.unpostString != new->menu_mgr.unpostString)
   {
      /* 
       * Each menubutton and menupane must have a new 'unpost' translation
       * set up; no grabs are needed.  The old ones need to first be
       * removed.
       */

      if (current->menu_mgr.unpostString)
      {
         /* 
          * Create the translation string of format:
          *
          *    "!<event>: MMUnpost(mgrId)"
          */

         workTemplate = &workArea[0];
         strcpy (workTemplate, "!");
         strcat (workTemplate, current->menu_mgr.unpostString);
         strcat (workTemplate, unpostTemplate);
         strcat (workTemplate, _XwMapToHex(NULL));
         strcat (workTemplate, ")");
         translation = XtParseTranslationTable (workTemplate);
         SetMenuTranslations (new, translation);
         /* XtDestroyStateTable (XtClass(new), translation); */
         RegisterTranslation (grandparent, current->menu_mgr.unpostString,
                              unpostTemplate, NULL);
         XtFree (current->menu_mgr.unpostString);
      }

      if (new->menu_mgr.unpostString)
      {
         /* 
          * Create the translation string of format:
          *
          *    "!<event>: MMUnpost(mgrId)"
          */

         workTemplate = &workArea[0];
         strcpy (workTemplate, "!");
         strcat (workTemplate, new->menu_mgr.unpostString);
         strcat (workTemplate, unpostTemplate);
         strcat (workTemplate, _XwMapToHex(new->core.self));
         strcat (workTemplate, ")");
         translation = XtParseTranslationTable (workTemplate);
         SetMenuTranslations (new, translation);
         /* XtDestroyStateTable (XtClass(new), translation); */
         RegisterTranslation (grandparent, new->menu_mgr.unpostString,
                              unpostTemplate, new);
      }
   }

   /* Process the keyboard select event.  */
   if (current->menu_mgr.kbdSelectString != new->menu_mgr.kbdSelectString)
   {
      /* 
       * Each menubutton and menupane must have a new 'kbd select' translation
       * set up; no grabs are needed.  The old ones do not need to be
       * removed, since ProcessSelect() will reject them.
       */

      if (new->menu_mgr.kbdSelectString)
      {
         /* 
          * Create the translation string of format:
          *
          *    "!<event>: select()"
          */
         workTemplate = &workArea[0];
         strcpy (workTemplate, "!");
         strcat (workTemplate, new->menu_mgr.kbdSelectString);
         strcat (workTemplate, selectTemplate);
         translation = XtParseTranslationTable (workTemplate);

         /*
          * Since the menupanes are our popup grand children, we
          * will process them simply by traversing our popup children list.
          */
         SetMenuTranslations (new, translation);
         /* XtDestroyStateTable (XtClass(new), translation); */
      }

       if (current->menu_mgr.kbdSelectString)
         XtFree (current->menu_mgr.kbdSelectString);
   }

   /* Process the associateChildren flag */
   if (current->menu_mgr.associateChildren != new->menu_mgr.associateChildren)
   {
      int i;

      /*
       * If the widget is realized, then we need to set/clear button
       * and key grabs for the post event, the post accelerator, and
       * all menubutton accelerators; the two post grabs will only be
       * set or cleared if they were not already done so earlier; this
       * would have happened if either of their event strings had
       * changed.
       */
      if (XtIsRealized (grandparent))
      {
         if (new->menu_mgr.associateChildren)
         {
            /* Set post grab */
            if ((!postGrabSet) && (new->menu_mgr.postString))
            {
               XGrabButton (XtDisplay(grandparent), new->menu_mgr.postButton,
                            (new->menu_mgr.postModifiers & ~btnMask),
                            XtWindow(grandparent),
                            False, ButtonPressMask|ButtonReleaseMask,
                            GrabModeAsync, GrabModeAsync, (Window)NULL, (Cursor)NULL);
            }
            
            /* Set post accelerator grab */
            if ((!postAccelGrabSet) && (new->popup_mgr.postAccelerator))
            {
               XGrabKey (XtDisplay(grandparent), new->popup_mgr.accelKey,
                         new->popup_mgr.accelModifiers, XtWindow(grandparent),
                         False, GrabModeAsync, GrabModeAsync);
            }

            /* Set menubutton accelerator grabs */
            for (i = 0; i < new->menu_mgr.numAccels; i++)
            {
               XGrabKey (XtDisplay(grandparent), 
                         new->menu_mgr.menuBtnAccelTable[i].accelKey,
                         new->menu_mgr.menuBtnAccelTable[i].accelModifiers, 
                         XtWindow(grandparent), False, GrabModeAsync, 
                         GrabModeAsync);
            }
         }
         else
         {
            /* Clear post grab */
            if ((!postGrabCleared) && (current->menu_mgr.postString))
            {
               XUngrabButton (XtDisplay(grandparent), 
                              current->menu_mgr.postButton,
                              (current->menu_mgr.postModifiers & ~btnMask),
                              XtWindow(grandparent));
            }
            
            /* Clear post accelerator grab */
            if ((!postAccelGrabCleared) && (current->popup_mgr.postAccelerator))
            {
               XUngrabKey (XtDisplay(grandparent), 
                         current->popup_mgr.accelKey,
                         current->popup_mgr.accelModifiers, 
                         XtWindow(grandparent));
            }
            
            /* Clear menubutton accelerator grabs */
            for (i = 0; i < current->menu_mgr.numAccels; i++)
            {
               XUngrabKey (XtDisplay(grandparent), 
                         current->menu_mgr.menuBtnAccelTable[i].accelKey,
                         current->menu_mgr.menuBtnAccelTable[i].accelModifiers, 
                         XtWindow(grandparent));
            }
         }
      }
   }

   /*
    * Since all of the menu components inherit their traversal setting
    * from their menu manager, anytime the menu manager's traversal
    * state changes, we need to propogate this down to all of the other
    * menu components (panes, buttons, separaters).
    */
   if (new->manager.traversal_on != current->manager.traversal_on)
   {
      int traversalType;

      if (new->manager.traversal_on)
         traversalType = XwHIGHLIGHT_TRAVERSAL;
      else
         traversalType = XwHIGHLIGHT_OFF;

      for (i = 0; i < current->core.num_popups; i++)
      {
         CompositeWidget shell;

         shell = (CompositeWidget) current->core.popup_list[i];

         for (j = 0; j < shell->composite.num_children; j++)
         {
            XwMenuPaneWidget menupane;

            /* Here we set the traversal flag for the menupanes */
            menupane = (XwMenuPaneWidget)shell->composite.children[j];
            if (XtIsSubclass ((Widget)menupane, XwmenupaneWidgetClass))
            {
	       (*(((XwMenuPaneWidgetClass)
		XtClass(menupane))-> menu_pane_class.setTraversalFlag))
	         ((Widget)menupane, new->manager.traversal_on);
            }

            for (k = 0; k < menupane->manager.num_managed_children; k++)
            {
               XwMenuButtonWidget mbutton;

               /* Here we set the traversal flag for the menubuttons */
               mbutton = (XwMenuButtonWidget)menupane->composite.children[k];
               if (XtIsSubclass ((Widget)mbutton, XwmenubuttonWidgetClass))
               {
	          (*(((XwMenuButtonWidgetClass)
		   XtClass(mbutton))-> menubutton_class.setTraversalType))
	            ((Widget)mbutton, traversalType);
               }
            }
         }
      }
   }

   /* Clear the sticky menu list, if sticky mode is disabled */
   if (new->popup_mgr.stickyMode == False)
      new->popup_mgr.numSavedCascades = 0;

   return (FALSE);
}


/*************************************<->*************************************
 *
 *  Destroy()
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

    register XwPopupMgrWidget  mw;

{
   Widget grandparent;
   register int i;

   /* Get the widget Id for the widget to which the menus are attached */
   grandparent = (Widget)XtParent(XtParent(mw));

   /* Free up any memory we allocated */
   XtFree ((char *)(mw->popup_mgr.savedCascadeList));
   XtFree ((char *)(mw->popup_mgr.currentCascadeList));

   /* Clean up the post and accelerated post translations */
   if (mw->menu_mgr.postString)
   {
      RegisterTranslation (grandparent, mw->menu_mgr.postString,
                           postTemplate, NULL);
   }
   if (mw->popup_mgr.postAccelerator)
   {
      RegisterTranslation (grandparent, mw->popup_mgr.postAccelerator,
                           accelPostTemplate, NULL);
   }

   if (mw->menu_mgr.unpostString)
   {
      RegisterTranslation (grandparent, mw->menu_mgr.unpostString,
                           unpostTemplate, NULL);
   }

   /***************
    * THE FOLLOWING IS NOT NEEDED, SINCE BY THE TIME WE GET HERE,
    * ALL OF OUR CHILDREN HAVE BEEN DESTROYED, AND THEIR ACCELERATORS
    * ALREADY CLEARED.
    *
    * Remove translations for each menubutton accelerator 
    *
    * for (i = 0; i < mw->menu_mgr.numAccels; i++)
    * {
    *  RegisterTranslation (grandparent, 
    *                       mw->menu_mgr.menuBtnAccelTable[i].accelString,
    *                       accelSelectTemplate, NULL);
    * }
    ****************/

   /* Remove any grabs we have set on the widget */
   if (XtIsRealized(grandparent))
   {
      if (mw->menu_mgr.associateChildren)
      {
         /* Remove post grab */
         if (mw->menu_mgr.postString)
         {
            XUngrabButton (XtDisplay(grandparent), mw->menu_mgr.postButton,
                           (mw->menu_mgr.postModifiers & ~btnMask),
                           XtWindow(grandparent));
         }

         /* Remove post accelerator grab */
         if (mw->popup_mgr.postAccelerator)
         {
            XUngrabKey (XtDisplay(grandparent), mw->popup_mgr.accelKey,
                      mw->popup_mgr.accelModifiers, XtWindow(grandparent));
         }

         /***************
          * THE FOLLOWING IS NOT NEEDED, SINCE BY THE TIME WE GET HERE,
          * ALL OF OUR CHILDREN HAVE BEEN DESTROYED, AND THEIR ACCELERATORS
          * ALREADY CLEARED.
          *
          * Remove grabs for each menubutton accelerator 
          * for (i = 0; i < mw->menu_mgr.numAccels; i++)
          * {
          *   XUngrabKey (XtDisplay(grandparent), 
          *                mw->menu_mgr.menuBtnAccelTable[i].accelKey,
          *                mw->menu_mgr.menuBtnAccelTable[i].accelModifiers,
          *                XtWindow(grandparent));
          * }
          ***************/
      }
   }
   else
   {
      /* Simply remove the exposure handler we added at initialize time */
      XtRemoveEventHandler (grandparent, ExposureMask|StructureNotifyMask, 
                            False, ExposeEventHandler, mw);
   }
}


/*************************************<->*************************************
 *
 *  ClassPartInitialize(parameters)
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

   register XwPopupMgrWidgetClass wc;

{
   register XwPopupMgrWidgetClass super;

   super = (XwPopupMgrWidgetClass)wc->core_class.superclass;

   if (wc->menu_mgr_class.attachPane == XtInheritAttachPane)
      wc->menu_mgr_class.attachPane = super->menu_mgr_class.attachPane;

   if (wc->menu_mgr_class.detachPane == XtInheritDetachPane)
      wc->menu_mgr_class.detachPane = super->menu_mgr_class.detachPane;

   if (wc->menu_mgr_class.addPane == XtInheritAddPane)
      wc->menu_mgr_class.addPane = super->menu_mgr_class.addPane;

   if (wc->menu_mgr_class.setSelectAccelerator == XtInheritSetSelectAccelerator)
      wc->menu_mgr_class.setSelectAccelerator = 
                              super->menu_mgr_class.setSelectAccelerator;

   if (wc->menu_mgr_class.clearSelectAccelerator == 
           XtInheritClearSelectAccelerator)
      wc->menu_mgr_class.clearSelectAccelerator = 
            super->menu_mgr_class.clearSelectAccelerator;

   if (wc->menu_mgr_class.setPostMnemonic == XtInheritSetPostMnemonic)
      wc->menu_mgr_class.setPostMnemonic =super->menu_mgr_class.setPostMnemonic;

   if (wc->menu_mgr_class.clearPostMnemonic == XtInheritClearPostMnemonic)
      wc->menu_mgr_class.clearPostMnemonic = 
                                      super->menu_mgr_class.clearPostMnemonic;

   if (wc->menu_mgr_class.setPostMnemonic == XtInheritSetPostMnemonic)
      wc->menu_mgr_class.setPostMnemonic =super->menu_mgr_class.setPostMnemonic;

   if (wc->menu_mgr_class.clearPostMnemonic==XtInheritClearPostMnemonic)
      wc->menu_mgr_class.clearPostMnemonic = 
                                    super->menu_mgr_class.clearPostMnemonic;

   if (wc->menu_mgr_class.addButton == XtInheritAddButton)
      wc->menu_mgr_class.addButton = super->menu_mgr_class.addButton;

   if (wc->menu_mgr_class.setSelectMnemonic == XtInheritSetSelectMnemonic)
      wc->menu_mgr_class.setSelectMnemonic = 
                    super->menu_mgr_class.setSelectMnemonic;

   if (wc->menu_mgr_class.clearSelectMnemonic == XtInheritClearSelectMnemonic)
      wc->menu_mgr_class.clearSelectMnemonic = 
                    super->menu_mgr_class.clearSelectMnemonic;

   if (wc->menu_mgr_class.processSelect == XtInheritProcessSelect)
      wc->menu_mgr_class.processSelect = super->menu_mgr_class.processSelect;

   if (wc->menu_mgr_class.validEvent == XtInheritValidEvent)
      wc->menu_mgr_class.validEvent = super->menu_mgr_class.validEvent;

   if (wc->menu_mgr_class.doICascade == XtInheritDoICascade)
      wc->menu_mgr_class.doICascade = super->menu_mgr_class.doICascade;

   if (wc->menu_mgr_class.paneManagedChildren == XtInheritPaneManagedChildren)
      wc->menu_mgr_class.paneManagedChildren = 
                         super->menu_mgr_class.paneManagedChildren;

   if (wc->menu_mgr_class.setTitleAttributes == XtInheritSetTitleAttributes)
      wc->menu_mgr_class.setTitleAttributes = 
                                    super->menu_mgr_class.setTitleAttributes;

   if (wc->menu_mgr_class.traverseLeft == XtInheritPopupTravLeft)
      wc->menu_mgr_class.traverseLeft = super->menu_mgr_class.traverseLeft;

   if (wc->menu_mgr_class.traverseRight == XtInheritPopupTravRight)
      wc->menu_mgr_class.traverseRight = super->menu_mgr_class.traverseRight;

   if (wc->menu_mgr_class.traverseNext == XtInheritPopupTravNext)
      wc->menu_mgr_class.traverseNext = super->menu_mgr_class.traverseNext;

   if (wc->menu_mgr_class.traversePrev == XtInheritPopupTravPrev)
      wc->menu_mgr_class.traversePrev = super->menu_mgr_class.traversePrev;

   if (wc->menu_mgr_class.traverseHome == XtInheritPopupTravHome)
      wc->menu_mgr_class.traverseHome = super->menu_mgr_class.traverseHome;

   if (wc->menu_mgr_class.traverseUp == XtInheritPopupTravUp)
      wc->menu_mgr_class.traverseUp = super->menu_mgr_class.traverseUp;

   if (wc->menu_mgr_class.traverseDown == XtInheritPopupTravDown)
      wc->menu_mgr_class.traverseDown = super->menu_mgr_class.traverseDown;

   if (wc->menu_mgr_class.traverseNextTop == XtInheritPopupTravNextTop)
      wc->menu_mgr_class.traverseNextTop=super->menu_mgr_class.traverseNextTop;

   if (wc->menu_mgr_class.btnSensitivityChanged==XtInheritBtnSensitivityChanged)
      wc->menu_mgr_class.btnSensitivityChanged = 
                            super->menu_mgr_class.btnSensitivityChanged;

   if (wc->menu_mgr_class.paneSensitivityChanged ==
                                       XtInheritPaneSensitivityChanged)
      wc->menu_mgr_class.paneSensitivityChanged = 
                            super->menu_mgr_class.paneSensitivityChanged;
}

/*************************************<->*************************************
 *
 *  RegisterTranslation (parameters)
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

static void RegisterTranslation (widget, event, template, menuMgrId)

   Widget widget;
   String event;
   String template;
   Widget menuMgrId;


{
   register String workTemplate;
   XtTranslations translations;

   workTemplate = &workArea[0];

   /*
    * Construct the translation string, using the following format:
    *
    *  "!<event>: ActionProc(menuMgrId)"
    */

   strcpy (workTemplate, "!");
   strcat (workTemplate, event);
   strcat (workTemplate, template);
   if (menuMgrId)
      strcat (workTemplate, _XwMapToHex(menuMgrId->core.self));
   else
      strcat (workTemplate, _XwMapToHex(NULL));
   strcat (workTemplate, ")");

   /* Compile the translation and attach to the widget */
   translations = XtParseTranslationTable(workTemplate);
   XtOverrideTranslations (widget, translations);
   /* XtDestroyStateTable (XtClass(widget), translations); */
}


/*************************************<->*************************************
 *
 *  PositionCascade (parameters)
 *
 *   Description:
 *   -----------
 *     This routine positions a cacading submenu pane.  When the new
 *     menupane is posted, it will slightly overlap the previous menu
 *     pane (in the x direction).  In the y direction, the new menupane
 *     will be positioned such that its first menubutton is centered on
 *     the menubutton to which the menupane is attached.
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

static void PositionCascade (menubutton, menupane)

   register Widget          menubutton;
            XwManagerWidget menupane;

{
   Position x, y, yDelta;
   Widget menubuttonGrandparent;
   Widget menubuttonParent;
   Widget menupaneShell;

   menubuttonParent = (Widget)XtParent(menubutton);
   menubuttonGrandparent = (Widget)XtParent(menubuttonParent);
   menupaneShell = (Widget)XtParent(menupane);

   /* 
    * In order for this algorithm to work, we need to make sure the
    * menupane has been realized; this is because its size is not
    * yet valid because it is not notified that it has any children
    * until it is realized.
    */
   if (!XtIsRealized (menupaneShell))
      XtRealizeWidget (menupaneShell);

   /* 
    * Since the x and y need to be absolute positions, we need to
    * use the shell widget coordinates in portions of these calculations.
    */
   x = menubuttonGrandparent->core.x +
       menubuttonParent->core.border_width +
       menubutton->core.border_width +
       menubutton->core.width -
       (XwCASCADEWIDTH + FUDGE_FACTOR + menupane->core.border_width);

   y = menubuttonGrandparent->core.y +
       menubuttonParent->core.border_width +
       menubutton->core.border_width +
       menubutton->core.y +
       (menubutton->core.height >> 1);

   /* Attempt to center on the first button in the new menupane */
   if (menupane->manager.num_managed_children > 0)
   {
      Widget firstButton = menupane->manager.managed_children[0];
/*
      yDelta = firstButton->core.y +
               firstButton->core.border_width +
               (firstButton->core.height >> 1);
*/
      yDelta = 0;
   }
   else
      yDelta = menupane->core.border_width + (menupane->core.height >> 1);

   y -= yDelta;

   ForceMenuPaneOnScreen (menupane, &x, &y);

   XtMoveWidget (menupaneShell, x, y);
}


/*************************************<->*************************************
 *
 *  ExposeEventHandler (parameters)
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

static void ExposeEventHandler (w, menuMgr, event)

   Widget w;
   register XwPopupMgrWidget menuMgr;
   XEvent * event;

{
   register int i;

   /*
    * If the children inherit the menu tree, then set up grabs for
    * the post button, the post accelerator key, and each menubutton
    * accelerator.
    */
   if (menuMgr->menu_mgr.associateChildren)
   {
      if (menuMgr->menu_mgr.postString)
      {
         XGrabButton (XtDisplay(menuMgr), menuMgr->menu_mgr.postButton,
                      (menuMgr->menu_mgr.postModifiers & ~btnMask),
                      XtWindow(w), False, 
                      ButtonPressMask|ButtonReleaseMask,
                      GrabModeAsync, GrabModeAsync, (Window)NULL, (Cursor)NULL);
      }

      if (menuMgr->popup_mgr.postAccelerator)
      {
         XGrabKey (XtDisplay(menuMgr), menuMgr->popup_mgr.accelKey,
                   menuMgr->popup_mgr.accelModifiers, XtWindow(w),
                   False, GrabModeAsync, GrabModeAsync);
      }

      for (i = 0; i < menuMgr->menu_mgr.numAccels; i++)
      {
         XGrabKey (XtDisplay(menuMgr), 
                   menuMgr->menu_mgr.menuBtnAccelTable[i].accelKey,
                   menuMgr->menu_mgr.menuBtnAccelTable[i].accelModifiers, 
                   XtWindow(w), False, GrabModeAsync, GrabModeAsync);
      }
   }

   XtRemoveEventHandler (w, ExposureMask|StructureNotifyMask, False, 
                         (XtEventHandler)ExposeEventHandler, menuMgr);
}


/*************************************<->*************************************
 *
 *  _XwCascadeSelect(parameters)
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

static void _XwCascadeSelect (menubutton, menupane, data)

   Widget menubutton;
   XwMenuPaneWidget  menupane;
   caddr_t data;       /* not used */

{
   register int i;
   register XwPopupMgrWidget menuMgr;

   menuMgr = (XwPopupMgrWidget) XtParent(XtParent(menupane));

   /* If the pane is already posted, then do nothing but return */
   for (i = 0; i < menuMgr->popup_mgr.numCascades; i++)
   {
      if (menupane==(XwMenuPaneWidget)menuMgr->popup_mgr.currentCascadeList[i])
         return;
   }

   /* Position the cascading menupane */
   PositionCascade (menubutton, menupane);

   /* Post the menupane */
   Post (menuMgr, menupane, XtGrabNonexclusive);

   /* Set the traversal focus, if necessary */
   if (menuMgr->manager.traversal_on)
   {
      /* Force the highlight to the first item */
      menupane->manager.active_child = NULL;
      XtSetKeyboardFocus ((Widget)menupane, NULL);
      XwMoveFocus (menupane);
      SendFakeLeaveNotify(menubutton, SHELL_PARENT);
   }
   XFlush(XtDisplay(menuMgr));
}


/*************************************<->*************************************
 *
 *  _XwCascadeUnselect (parameters)
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

static void _XwCascadeUnselect (menubutton, menupane, params)

   Widget menubutton;
   Widget menupane;
   register XwunselectParams * params;

{
   XwPopupMgrWidget menuMgr;
   register Widget shell;

   menuMgr = (XwPopupMgrWidget) XtParent(XtParent(menupane));
   shell = (Widget)XtParent(menupane);

   /* 
    * Determine if the cursor left the cascade region and entered
    * the cascading menupane.  If this happened, then tell the menu
    * button to remain highlighted.  If this did not happen, then we
    * need to unpost the menupane, and tell the menubutton to unhighlight.
    *
    */

   /* See if the cursor is in the menupane */
   if ((params->rootX >= shell->core.x) && (params->rootX <
      (shell->core.x + shell->core.width + (shell->core.border_width << 1))) &&
      (params->rootY >= shell->core.y) && (params->rootY <
      (shell->core.y + shell->core.height + (shell->core.border_width << 1))))
   {
      /* Yes, we're in the cascading menupane */
      params->remainHighlighted = TRUE;
   }
   else
   {
      /* No, we've left the cascade region; unpost the menupane */
      Unpost (menuMgr, menupane);
      params->remainHighlighted = FALSE;
   }
}


/*************************************<->*************************************
 *
 *  _XwMenuPaneCleanup (parameters)
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

static void _XwMenuPaneCleanup (menupane, menuMgr, data)

   XwMenuPaneWidget menupane;
   Widget menuMgr;
   caddr_t data;   /* not used */

{
   /*
    * If this menupane is currently attached to a menubutton, or if this
    * is the top level menupane, then we need to detach it before we
    * allow it to be destroyed.
    */
   if (menupane->menu_pane.attach_to)
   {
      if (menupane->menu_pane.attachId == NULL)
         DetachPane (menuMgr, menupane, menupane->menu_pane.attach_to);
      else
         DoDetach (menuMgr, menupane->menu_pane.attachId, menupane);
   }
}


/*************************************<->*************************************
 *
 *  _XwMenuButtonCleanup(parameters)
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

static void _XwMenuButtonCleanup (menubutton, menuMgr, data)

   Widget menubutton;
   XwPopupMgrWidget menuMgr;
   caddr_t data;   /* not used */

{
   Arg arg[1];
   XwMenuPaneWidget cascadeOn;

   /*
    * Remove any accelerators associated with this menubutton, and if
    * there is a menupane cacading from us, then break that association,
    * and add the menupane back into the pending attach list.
    */
   ClearSelectAccelerator (menuMgr, menubutton);

   XtSetArg (arg[0], XtNcascadeOn, (XtArgVal) &cascadeOn);
   XtGetValues (menubutton, arg, 1);

   if (cascadeOn != NULL)
   {
      /* Detach, and add back into pending attach list */
      DoDetach (menuMgr, menubutton, cascadeOn);
      AddToPendingList (menuMgr, cascadeOn, cascadeOn->menu_pane.attach_to);
   }

   /*
    * If this menubutton had been the last item selected, and if sticky
    * menus is enabled, then we need to cleanup the saved cascade list,
    * so that we don't dump core the next time the menu is posted.
    */
   if ((menuMgr->popup_mgr.stickyMode) &&
       (menuMgr->popup_mgr.numSavedCascades > 0) &&
       (menuMgr->popup_mgr.lastSelected == menubutton->core.self))
   {
      menuMgr->popup_mgr.numSavedCascades = 0;
   }
}


/*************************************<->*************************************
 *
 *  MMPost(parameters)
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

static void MMPost (w, event, params, count)

   Widget w;
   XEvent * event;
   String * params;
   Cardinal count;

{
   XwPopupMgrWidget menuMgr;

   /* Extract the menu manager widget id */
   menuMgr = (XwPopupMgrWidget) _XwMapFromHex (params[0]);

   /* 
    * If we have made it down here, then we know the post event is
    * ours to handle.
    */
   if ((menuMgr) && (menuMgr->popup_mgr.topLevelPane))
      ClassPost (menuMgr, event, TRUE);
}


/*************************************<->*************************************
 *
 *  MMUnpost(parameters)
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

static void MMUnpost (w, event, params, count)

   Widget w;
   XEvent * event;
   String * params;
   Cardinal count;

{
   register XwPopupMgrWidget menuMgr;
   Boolean traversalOn;

   /* Extract the menu manager widget id */
   menuMgr = (XwPopupMgrWidget) _XwMapFromHex (params[0]);

   /*
    * We need to use a temporary variable, because the application may
    * change this value from underneath us if it has attached any unpost
    * callbacks to any of the menupanes.
    */
   traversalOn = menuMgr->manager.traversal_on;

   if ((menuMgr) && (menuMgr->menu_mgr.menuActive))
   {
      /*
       * Unpost the menu.
       * To prevent undesirable side effects, the menuActive flag must
       * always be cleared before bring down the menu system.
       */
      menuMgr->menu_mgr.menuActive = FALSE;
      Unpost (menuMgr, menuMgr->popup_mgr.topLevelPane);
      XUngrabPointer (XtDisplay(menuMgr), CurrentTime);

      /*
       * If traversal is on, and there are no sticky menupanes currently
       * being remembered, then we need to reset the focus item to the
       * first item in the menupane.
       */
      if (traversalOn && (menuMgr->popup_mgr.numSavedCascades == 0))
      {
         ((XwPopupMgrWidget)menuMgr->popup_mgr.topLevelPane)->
                          manager.active_child = NULL;
         XtSetKeyboardFocus (menuMgr->popup_mgr.topLevelPane, NULL);
      }

      /*
       * If we warped the mouse because traversal was on, then we need
       * to move it back to where it was.
       */
      if (traversalOn &&
          (menuMgr->popup_mgr.origMouseX != -1) &&
          (menuMgr->popup_mgr.origMouseY != -1))
      {
	 XWarpPointer (XtDisplay (menuMgr), None,
		       RootWindowOfScreen(menuMgr->core.screen),
		       0, 0, 0, 0, 
                       menuMgr->popup_mgr.origMouseX, 
                       menuMgr->popup_mgr.origMouseY);
         menuMgr->popup_mgr.origMouseX = -1;
         menuMgr->popup_mgr.origMouseY = -1;
      }
   }
}


/*************************************<->*************************************
 *
 *  MMAccelPost (parameters)
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

static void MMAccelPost (w, event, params, count)

   Widget w;
   XEvent * event;
   String * params;
   Cardinal count;

{
   register XwPopupMgrWidget menuMgr;

   /* Extract the menu manager widget id */
   menuMgr = (XwPopupMgrWidget) _XwMapFromHex (params[0]);

   /* 
    * If we have made it down here, then we know the accelerator event is
    * ours to handle.
    */
   if ((menuMgr) && (menuMgr->popup_mgr.topLevelPane))
      ClassPost (menuMgr, event, FALSE);
}


/*************************************<->*************************************
 *
 *  MMAccelSelect (parameters)
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

static void MMAccelSelect (w, event, params, count)

   Widget w;
   XEvent * event;
   String * params;
   Cardinal count;

{
   register XwPopupMgrWidget menuMgr;

   /* Extract the menu manager widget id */
   menuMgr = (XwPopupMgrWidget) _XwMapFromHex (params[0]);

   if (menuMgr == NULL)
      return;

   /*
    * if menus are not inherited, throw out events that did occur
    * in the associated widget's children.  This takes care of the case
    * where the child does not select for key events, so they propagate
    * to us.
    */     
   if (menuMgr->menu_mgr.associateChildren == FALSE)
   {
      if ((event->xkey.window == XtWindow (XtParent (XtParent (menuMgr)))) &&
          (event->xkey.subwindow != 0) && 
          (XtWindowToWidget (XtDisplay (menuMgr), event->xkey.subwindow)))
      return;
   }

   /* 
    * If we have made it down here, then we know the accelerator event is
    * ours to handle.
    */
   if (menuMgr->popup_mgr.topLevelPane)
      ClassSelect ((XwMenuMgrWidget)menuMgr, event);
}


/*************************************<->*************************************
 *
 *   AttachPane(menuMgr, menupane, name)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void AttachPane (menuMgr, menupane, name)

   register XwPopupMgrWidget   menuMgr;
   register XwMenuPaneWidget   menupane;
   String             name;

{
   register XwMenuButtonWidget menubutton;
   Arg arg[1];
   register int i, j;
   CompositeWidget shell, mpane;
   XrmName nameQuark;

   if (name == NULL)
      return;

   /*
    * Save a copy of the menupane Id, since this has been called as
    * a result of a SetValues being done on the menupane, and all of
    * our lists only contain the old widget pointer.
    */
   menuMgr->popup_mgr.attachPane = menupane;

   /*
    * check if name is menu manager's name, then this is to be a top level
    */
   if ((strcmp (name, menuMgr->core.name) == 0) &&
       (XwStrlen(name) == XwStrlen(menuMgr->core.name)))
   {
      /* Remove the cascade list when the top level pane changes */
      menuMgr->popup_mgr.numSavedCascades = 0;

      if (menuMgr->popup_mgr.topLevelPane != NULL)
      {
         /* Detach the previous top level pane */
         XtSetArg (arg[0], XtNattachTo, (XtArgVal) NULL);
         XtSetValues (menuMgr->popup_mgr.topLevelPane, arg, 1);
      }

      /*
       * save menupane ID.  Since this may be called from menupane's
       * SetValues routine, the widget passed may be the new structure.
       * Grab the self field in core to insure the right ID is used.
       */
      menuMgr->popup_mgr.topLevelPane = menupane->core.self;
      menupane->menu_pane.attachId = NULL;
 
      SetTreeAccelerators (menuMgr, menupane);
   }
   else
   {
      /*
       * check if its a menubutton name in the menu system.
       */
      nameQuark = XrmStringToQuark(name);

      for (i = 0; i < menuMgr->core.num_popups; i++)
      {
	 /*
	  * for each shell, if its child is a menupane and the menupane
	  * has a menubutton of the passed in name,  then do the attach
	  */
	 shell = (CompositeWidget) menuMgr->core.popup_list[i];
	 if ((shell->composite.num_children == 1) &&
	     (XtIsSubclass(shell->composite.children[0],
			   XwmenupaneWidgetClass)))
	 {
            mpane = (CompositeWidget) shell->composite.children[0];
            for (j = 0; j < mpane->composite.num_children; j++)
	    {
               menubutton = (XwMenuButtonWidget)mpane->composite.children[j];
               if (menubutton->core.xrm_name == nameQuark)
               {
		  Widget dwidget = NULL;
                  /*
                   * If there is a menupane already attached to this
                   * button, then we need to first detach it.
                   */
	          XtSetArg (arg[0], XtNcascadeOn, (XtArgVal) &dwidget);
	          XtGetValues ((Widget)menubutton, arg, 1);
                  if (dwidget)
                  {
                     /* Detach the old pane */
                     Widget temppane = dwidget;
                     XtSetArg (arg[0], XtNattachTo, (XtArgVal) NULL);
                     XtSetValues (temppane, arg, 1);
                  }

	          XtSetArg (arg[0], XtNcascadeOn, (XtArgVal) menupane->core.self);
	          XtSetValues ((Widget)menubutton, arg, 1);
	 
   	          menupane->menu_pane.attachId = (Widget) menubutton;
   
	          XtAddCallback ((Widget) menubutton,
			      XtNcascadeSelect,	(XtCallbackProc)_XwCascadeSelect, 
                              menupane->core.self);

	          XtAddCallback ((Widget) menubutton,
			      XtNcascadeUnselect, (XtCallbackProc)_XwCascadeUnselect,
			      menupane->core.self);

	          if (CompletePath(menuMgr, menupane))
		      SetTreeAccelerators (menuMgr, menupane);

                  menuMgr->popup_mgr.attachPane = NULL;
	          return;
               }
	    }
	 }
      }
      /*
       * Couldn't find a menubutton with this name
       */
      AddToPendingList (menuMgr, menupane, name);
   }

   /* This always needs to be cleared out when we are done */
   menuMgr->popup_mgr.attachPane = NULL;
}

/*************************************<->*************************************
 *
 *   DetachPane (menuMgr, menupane, name)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void DetachPane (menuMgr, menupane, name)

   register XwPopupMgrWidget   menuMgr;
   XwMenuPaneWidget  menupane;
   String            name;

{
   register int i;
   Arg arg;
   XwMenuButtonWidget menubutton;
   CompositeWidget shell;
   
   if (name == NULL)
      return;

   /*
    * if menupane is on the pending attach list, remove it and return
    */
   for (i=0; i < menuMgr->menu_mgr.numAttachReqs; i++)
   {
      if (menuMgr->menu_mgr.pendingAttachList[i].menupaneId ==
	  menupane->core.self)
      {
	 menuMgr->menu_mgr.pendingAttachList[i] =
	    menuMgr->menu_mgr.pendingAttachList
	       [--menuMgr->menu_mgr.numAttachReqs];
	 return;
      }
   }

   /*
    * Save a copy of the menupane Id, since this has been called as
    * a result of a SetValues being done on the menupane, and all
    * of our lists only contain the old widget pointer.
    */
   menuMgr->popup_mgr.detachPane = menupane;

   /*
    * if name is the menu manager's name, removing top level
    */
   if ((strcmp (name, menuMgr->core.name) == 0) &&
       (XwStrlen(name) == XwStrlen(menuMgr->core.name)))
   {
      menupane->menu_pane.attachId = (Widget) NULL;

      if (menupane->core.self == menuMgr->popup_mgr.topLevelPane)
      {
	 ClearTreeAccelerators (menuMgr, menupane->core.self);
	 menuMgr->popup_mgr.topLevelPane = NULL;
      }
   }
   else
   {
      /*
       * detach pane from menubutton.  Look on each of the menu managers
       * popup children for a menupane which contains this named menubutton.
       */
      for (i = 0; i < menuMgr->core.num_popups; i++)
      {
	 shell = (CompositeWidget) menuMgr->core.popup_list[i];
	 if ((shell->composite.num_children == 1) &&
	     (XtIsSubclass(shell->composite.children[0],
			   XwmenupaneWidgetClass)))
	 {
	    if ((menubutton = (XwMenuButtonWidget)
		 XtNameToWidget(shell->composite.children[0], name))
		!= NULL)
	    {
	       /*
		* Found it!  Detach it
		*/
               DoDetach (menuMgr, menubutton, menupane);
               menuMgr->popup_mgr.detachPane = NULL;
	       return;
	    }
	 }
      }
   }
   
   /* This must always be cleared when we leave */
   menuMgr->popup_mgr.detachPane = NULL;
}


/*************************************<->*************************************
 *
 *  DoDetach(parameters)
 *
 *   Description:
 *   -----------
 *     This routine does the actual work of detaching a menupane from a
 *     menubutton.  It has been separated out from DetachPane() due to
 *     a BUG in the X toolkits destroy scenario.  When the toolkit is
 *     destroying popup children, it replaces the widget Id entry within
 *     the popup list with the window Id for that widget.  DetachPane()
 *     attempts to traverse this list, looking for the named menubutton.
 *     Unfortunately, if this list contains any window Ids, then we may
 *     get a core dump; this only can happen when the menu is being
 *     destroyed.
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

static void DoDetach (menuMgr, menubutton, menupane)

   register XwPopupMgrWidget   menuMgr;
   XwMenuButtonWidget menubutton;
   XwMenuPaneWidget   menupane;

{
   Arg arg[1];
   register int i;

   /* Detach a menupane from a menubutton */
   XtRemoveCallback ((Widget)menubutton, XtNcascadeSelect,
		     (XtCallbackProc)_XwCascadeSelect, menupane->core.self);
   XtRemoveCallback ((Widget)menubutton, XtNcascadeUnselect,
		     (XtCallbackProc)_XwCascadeUnselect, menupane->core.self);
   ClearTreeAccelerators (menuMgr, menupane->core.self);

   XtSetArg (arg[0], XtNcascadeOn, (XtArgVal) NULL);
   XtSetValues ((Widget)menubutton, arg, 1);
   menupane->menu_pane.attachId = NULL;

   /* 
    * If this menupane is on the saved cascade list (because sticky
    * menus are enabled), then we need to clean up the list, so that
    * we don't dump core the next time we post.
    */
   if ((menuMgr->popup_mgr.stickyMode) &&
       (menuMgr->popup_mgr.numSavedCascades > 0))
   {
      for (i = 0; i < menuMgr->popup_mgr.numSavedCascades; i++)
      {
         if (menuMgr->popup_mgr.savedCascadeList[i] == 
            (Widget)menupane->core.self)
         {
            menuMgr->popup_mgr.numSavedCascades = 0;
            break;
         }
      }
   }
}


/*************************************<->*************************************
 *
 *   AddPane(menuMgr, menupane)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void AddPane (menuMgr, menupane)

   register XwMenuMgrWidget  menuMgr;
   XwMenuPaneWidget menupane;

{
   register int k;
   register int traversalType;
   register XwMenuButtonWidget mbutton;

   XtAddCallback ((Widget)menupane, XtNdestroyCallback,
	(XtCallbackProc)_XwMenuPaneCleanup, menuMgr);

   if (menuMgr->menu_mgr.selectString)
   {
      SetUpTranslation (menupane, menuMgr->menu_mgr.selectString, 
                        selectTemplate);
   }

   if (menuMgr->menu_mgr.unpostString)
   {
      RegisterTranslation (menupane, menuMgr->menu_mgr.unpostString,
                           unpostTemplate, menuMgr);
   }

   if (menuMgr->menu_mgr.kbdSelectString)
   {
      SetUpTranslation (menupane, menuMgr->menu_mgr.kbdSelectString, 
                        selectTemplate);
   }

   /* Propogate the state of our traversal flag */
   if (XtIsSubclass ((Widget)menupane, XwmenupaneWidgetClass))
   {
       (*(((XwMenuPaneWidgetClass)
	XtClass(menupane))-> menu_pane_class.setTraversalFlag))
         ((Widget)menupane, menuMgr->manager.traversal_on);
   }

   /******************
    * THE FOLLOWING IS NOT NEEDED, SINCE A MENUPANE CAN NEVER HAVE
    * ANY CHILDREN AT THIS TIME.
    *
    * traversalType = (menuMgr->manager.traversal_on) ? XwHIGHLIGHT_TRAVERSAL:
    *                                                   XwHIGHLIGHT_OFF;
    *
    * for (k = 0; k < menupane->manager.num_managed_children; k++)
    * {
    *    /* Here we set the traversal flag for the menubuttons 
    *    mbutton = (XwMenuButtonWidget)menupane->composite.children[k];
    *    if (XtIsSubclass (mbutton, XwmenubuttonWidgetClass))
    *    {
    *       (*(((XwMenuButtonWidgetClass)
    *         XtClass(mbutton))-> menubutton_class.setTraversalType))
    *          (mbutton, traversalType);
    *    }
    * }
    *********************/
}

/*************************************<->*************************************
 *
 *   SetSelectAccelerator (menuMgr, menubutton, accelString, accelEventType,
 *	  	           accelKey, accelModifiers)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void SetSelectAccelerator (menuMgr, menubutton, accelString,
				  accelEventType, accelKey, accelModifiers)
Widget       menuMgr;
Widget       menubutton;
String       accelString;
unsigned int accelEventType;
unsigned int accelKey;
unsigned int accelModifiers;
{
   Widget widget;
   XwKeyAccel * tempAccel;
  
   if (CompletePath(menuMgr, menubutton->core.parent))
   {
      ClearSelectAccelerator (menuMgr, menubutton->core.self);
      SetButtonAccelerators (menuMgr, menubutton->core.self, accelString, 
                             accelEventType, accelKey, accelModifiers);
   }
}


/*************************************<->*************************************
 *
 *   ClearSelectAccelerator (menuMgr, menubutton)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void ClearSelectAccelerator (menuMgr, menubutton)

   register XwPopupMgrWidget menuMgr;
   Widget menubutton;

{
   register int i;
   Widget widget;

   /*
    * search accelerator table for menubutton
    */
   for (i=0; i < menuMgr->menu_mgr.numAccels; i++)
   {
      if (menuMgr->menu_mgr.menuBtnAccelTable[i].menuBtn == 
            menubutton->core.self)
      {
	 /*
	  * remove translation in associated widget & toplevel pane
	  * and remove grab
	  */
	 widget = (Widget) XtParent (XtParent (menuMgr));
	 RegisterTranslation (widget,
		    menuMgr->menu_mgr.menuBtnAccelTable[i].accelString,
		    accelSelectTemplate, NULL);

	 if (menuMgr->popup_mgr.topLevelPane)
         {
	    RegisterTranslation (menuMgr->popup_mgr.topLevelPane,
			menuMgr->menu_mgr.menuBtnAccelTable[i].accelString,
			accelSelectTemplate, NULL);

            /*
             * Because of a short coming in the toolkit, we need to
             * potentially patch the top level widget's translations,
             * if it was in the middle of a setvalues.
             */
            if ((menuMgr->popup_mgr.detachPane) &&
               (menuMgr->popup_mgr.topLevelPane ==
                menuMgr->popup_mgr.detachPane->core.self))
            {
               menuMgr->popup_mgr.detachPane->core.tm = 
                   menuMgr->popup_mgr.topLevelPane->core.tm;
               menuMgr->popup_mgr.detachPane->core.event_table = 
                   menuMgr->popup_mgr.topLevelPane->core.event_table;
            }
         }

	 if ((menuMgr->menu_mgr.associateChildren) &&
	     (XtIsRealized (widget)))
         {
	    XUngrabKey (XtDisplay (widget),
		menuMgr->menu_mgr.menuBtnAccelTable[i].accelKey,
		menuMgr->menu_mgr.menuBtnAccelTable[i].accelModifiers,
		XtWindow (widget));
         }

	 /*
	  * remove menubutton accelerator table entry
	  */
	 menuMgr->menu_mgr.menuBtnAccelTable[i] =
	    menuMgr->menu_mgr.menuBtnAccelTable[--menuMgr->menu_mgr.numAccels];

	 return;
      }
   }
}

/*************************************<->*************************************
 *
 *   AddButton(menuMgr, menubutton)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void AddButton (menuMgr, menubutton)

   register XwMenuMgrWidget  menuMgr;
   register Widget  menubutton;

{
   Arg args[2];
   Widget pending;
   int traversalType;
   
   if (menuMgr->menu_mgr.selectString)
   {
      SetUpTranslation (menubutton, menuMgr->menu_mgr.selectString, 
                        selectTemplate);
   }
   if (menuMgr->menu_mgr.unpostString)
   {
      RegisterTranslation (menubutton, menuMgr->menu_mgr.unpostString,
                           unpostTemplate, menuMgr);
   }
   if (menuMgr->menu_mgr.kbdSelectString)
   {
      SetUpTranslation (menubutton, menuMgr->menu_mgr.kbdSelectString, 
                        selectTemplate);
   }

   XtSetArg (args[0], XtNcascadeOn, (XtArgVal) NULL);
   XtSetArg (args[1], XtNmgrOverrideMnemonic, (XtArgVal) TRUE);
   XtSetValues (menubutton, args, XtNumber(args));

   XtAddCallback (menubutton, XtNdestroyCallback,
		  (XtCallbackProc)_XwMenuButtonCleanup, menuMgr);

   /* 
    * Accelerators are now handled when the button is managed!
    */

   /*
    * if this menubutton is on the pending attach list, do attach
    */
   if (pending = PendingAttach (menuMgr, menubutton))
       AttachPane (menuMgr, pending, menubutton->core.name);

   /* Propogate the our traversal state */
   traversalType = (menuMgr->manager.traversal_on) ? XwHIGHLIGHT_TRAVERSAL:
                                                     XwHIGHLIGHT_OFF;

   if (XtIsSubclass (menubutton, XwmenubuttonWidgetClass))
   {
      (*(((XwMenuButtonWidgetClass)
        XtClass(menubutton))-> menubutton_class.setTraversalType))
        (menubutton, traversalType);
   }
}

/*************************************<->*************************************
 *
 *   Unpost (menuMgr, menupane)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void Unpost (menuMgr, menupane)

   register XwPopupMgrWidget  menuMgr;
   XwMenuPaneWidget  menupane;

{
   register int i,j;
   register XwMenuPaneWidget pane;
   register Widget * currentCascadeList;
   register WidgetList managed_children;

   /*
    * To keep all window managers happy, move the focus before we pop
    * down the menupane with the current focus.
    */
   if ((menupane != (XwMenuPaneWidget) menuMgr->popup_mgr.topLevelPane) &&
       (menuMgr->manager.traversal_on))
   {
      XwMoveFocus (XtParent(menupane->menu_pane.attachId));
   }

   currentCascadeList = menuMgr->popup_mgr.currentCascadeList;

   /*
    * popdown any cascading submenus including menupane
    */
   for (i = menuMgr->popup_mgr.numCascades - 1; i >= 0; i--)
   {
      /*
       * for each button in pane, unhighlight it and clear cascade flag
       */
      pane = ((XwMenuPaneWidget) currentCascadeList[i]);
      XtRemoveGrab ((Widget)pane);
      XtPopdown (XtParent (pane));
      managed_children = pane->manager.managed_children;

      for (j = 0; j < pane->manager.num_managed_children; j++)
      {
	 if (XtIsSubclass (managed_children[j], XwmenubuttonWidgetClass))
	 {
	    (*(((XwMenuButtonWidgetClass)
  	      XtClass(managed_children[j]))->menubutton_class.unhighlightProc))
	         (managed_children[j]);
	    (*(((XwMenuButtonWidgetClass)
	     XtClass(managed_children[j]))->menubutton_class.clearCascadeProc))
	         (managed_children[j]);
	 }
      }

      --menuMgr->popup_mgr.numCascades;

      /*
       * if this is not the target pane then the next one must have had an enter
       * window event handler added when cascaded out of it.
       */
      if ((pane != menupane) && (i > 0))
      {
         if (menuMgr->manager.traversal_on == FALSE)
         {
            XtRemoveEventHandler (currentCascadeList[i-1],
                                  EnterWindowMask, FALSE,
				  (XtEventHandler)
                                  ((XwMenuButtonWidgetClass) 
                                   XtClass(pane->menu_pane.attachId))->
                                    menubutton_class.enterParentProc,
                                  pane->menu_pane.attachId);
         }
         else
         {
            /* Kludge to force the grablist to get cleaned up */
            XtSetKeyboardFocus (XtParent(pane), None);
         }
      }
      else   
      {
         if (menuMgr->manager.traversal_on)
         {
            /* Kludge to force the grablist to get cleaned up */
            XtSetKeyboardFocus (XtParent(pane), None);
         }
         XFlush (XtDisplay(menuMgr));
         return;
      }
   }
}

/*************************************<->*************************************
 *
 *   Post(menuMgr, menupane, grabtype)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *   _XtPopup
 *
 *************************************<->***********************************/

static void Post (menuMgr, menupane, grabtype)

   register XwPopupMgrWidget  menuMgr;
   Widget  menupane;
   XtGrabKind grabtype;

{
   register int i;

   /*
    * if already posted, do nothing
    */
   for (i=0; i < menuMgr->popup_mgr.numCascades; i++)
      if (menuMgr->popup_mgr.currentCascadeList[i] == menupane)
	 return;

   /*
    * set up grabs
    */
   if (grabtype == XtGrabNonexclusive)
   {
      _XtPopup (XtParent (menupane), grabtype, FALSE);
      XtAddGrab (menupane, FALSE, FALSE);
   }
   else
   {
      _XtPopup (XtParent (menupane), grabtype, TRUE);
      XtAddGrab (menupane, TRUE, TRUE);
   }

   /*
    * add menupane to current cascade list
    */
   if (menuMgr->popup_mgr.numCascades == menuMgr->popup_mgr.sizeCascadeList)
   {
      menuMgr->popup_mgr.sizeCascadeList =
	 2 * menuMgr->popup_mgr.sizeCascadeList;
      menuMgr->popup_mgr.currentCascadeList =
	 (Widget *) XtRealloc((char *)(menuMgr->popup_mgr.currentCascadeList),
                              menuMgr->popup_mgr.sizeCascadeList *
			      sizeof (Widget));
   }

   menuMgr->popup_mgr.currentCascadeList[menuMgr->popup_mgr.numCascades++] =
      menupane;
}

/*************************************<->*************************************
 *
 *   ProcessSelect(menuMgr, widget, event)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Boolean ProcessSelect (menuMgr, widget, event)

   register XwPopupMgrWidget  menuMgr;
   Widget            widget;
   XEvent          * event;

{
   register XwKeyAccel * accelerator;
   register int i;
   Boolean found = FALSE;
   Widget assocWidget;
   Boolean traversalOn;

   /*
    * If the menu manager or the associated widget is insensitive, then
    * ignore the select request.
    */
   assocWidget = XtParent(XtParent(menuMgr));
   if (!XtIsSensitive((Widget)menuMgr) || !XtIsSensitive(assocWidget))
      return (FALSE);
   
   /*
    * is this a valid button event?
    */
   if ((event->xany.type==ButtonPress) || (event->xany.type==ButtonRelease))
   {

      if (_XwMatchBtnEvent (event, menuMgr->menu_mgr.selectEventType,
			    menuMgr->menu_mgr.selectButton,
			    menuMgr->menu_mgr.selectModifiers))
      {
         if (menuMgr->menu_mgr.menuActive == FALSE)
	    return (FALSE);

         /*
          * During traversal, since menupanes are not unposted when
          * the mouse enters a different menupane, we need to ignore
          * selects which occur in a menubtn in one of these panes.
          */
         if ((menuMgr->manager.traversal_on) &&
             (XtIsSubclass (widget, XwmenubuttonWidgetClass)) &&
             (XtParent(widget) != menuMgr->popup_mgr.currentCascadeList[
                               menuMgr->popup_mgr.numCascades - 1]))
         {
            return (FALSE);
         }

         SetUpStickyList (menuMgr, widget);
      }
      else
         return (FALSE);
   }
   /*
    * if its not key accelerator, return false
    */
   else if ((event->xany.type == KeyPress) || (event->xany.type == KeyRelease))
   {
      /* Check for the kbd select event */
      if (_XwMatchKeyEvent (event,
             menuMgr->menu_mgr.kbdSelectEventType,
             menuMgr->menu_mgr.kbdSelectKey,
             menuMgr->menu_mgr.kbdSelectModifiers))
      {
         SetUpStickyList (menuMgr, widget);
      }
      else
      {
         for (i=0; (i < menuMgr->menu_mgr.numAccels) && (found == FALSE); i++)
         {
            accelerator = menuMgr->menu_mgr.menuBtnAccelTable + i;
	    found = _XwMatchKeyEvent (event,
		       accelerator->accelEventType,
		       accelerator->accelKey,
		       accelerator->accelModifiers);
         }

         if (found == FALSE)
	    return (FALSE);
   
         menuMgr->popup_mgr.numSavedCascades = 0;
         if (menuMgr->manager.traversal_on)
         {
            /* Force the first item to be highlighted next time */
            ((XwPopupMgrWidget)menuMgr->popup_mgr.topLevelPane)->
                             manager.active_child = NULL;
            XtSetKeyboardFocus (menuMgr->popup_mgr.topLevelPane, NULL);
         }
         /* SetUpStickyList (menuMgr, widget); */
      }
   }
   else
      return (FALSE);

   /*
    * if the menu system is active, bring it down
    */
   if (menuMgr->menu_mgr.menuActive)
   {
      /*
       * We need to use a temporary variable because the application has
       * the chance to change this from underneath us if they have set up
       * an unpost callback.
       */
      traversalOn = menuMgr->manager.traversal_on;

      menuMgr->menu_mgr.menuActive = FALSE;

      Unpost (menuMgr, menuMgr->popup_mgr.topLevelPane);

      /* We need to remove the grab we set in ClassPost() */
      XUngrabPointer (XtDisplay(menuMgr), CurrentTime);

      /*
       * If we warped the mouse because traversal was on, then we need
       * to move it back to where it was.
       */
      if (traversalOn &&
          (menuMgr->popup_mgr.origMouseX != -1) &&
          (menuMgr->popup_mgr.origMouseY != -1))
      {
	 XWarpPointer (XtDisplay (menuMgr), None,
		       RootWindowOfScreen(menuMgr->core.screen),
		       0, 0, 0, 0, 
                       menuMgr->popup_mgr.origMouseX, 
                       menuMgr->popup_mgr.origMouseY);
         menuMgr->popup_mgr.origMouseX = -1;
         menuMgr->popup_mgr.origMouseY = -1;
      }
   }
   return (TRUE);
}

/*************************************<->*************************************
 *
 *   ValidEvent(menuMgr, menubutton, event)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Boolean ValidEvent (menuMgr, menubutton, event)

   XwPopupMgrWidget  menuMgr;
   Widget   menubutton;
   XEvent * event;

{
   /* Ignore enter and leave events if traversal is active */
   if (menuMgr->manager.traversal_on)
      return (FALSE);
   else
      return (TRUE);
}

/*************************************<->*************************************
 *
 *   DoICascade (menuMgr)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Boolean DoICascade (menuMgr, menuBtn)

   register XwPopupMgrWidget menuMgr;
   Widget menuBtn;

{
   /* Ignore cascade events if traversal is currently active */
   if ((menuMgr->menu_mgr.menuActive) &&
      (menuMgr->manager.traversal_on == FALSE))
   {
      Widget lastCascade = menuMgr->popup_mgr.currentCascadeList[
          menuMgr->popup_mgr.numCascades -1];

      if ((XtParent(menuBtn) == lastCascade) ||
          (menuBtn == ((XwMenuPaneWidget)(lastCascade))->menu_pane.attachId))
      {
         return (TRUE);
      }
   }

   return (FALSE);
}

/*************************************<->*************************************
 *
 *   ClassPost(menuMgr, event, warpOn)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void ClassPost (menuMgr, event, warpOn)

   register XwPopupMgrWidget  menuMgr;
   XEvent          * event;
   Boolean           warpOn;

{
   register int i;
   Position posx, posy;
   int x, y, relativeX, relativeY;
   int yDelta;
   XButtonEvent * buttonEvent = (XButtonEvent *) event;
   Widget assocWidget, w;
   Widget menuBtn;
   XwMenuPaneWidget tempPane;
   XwManagerWidget topLevelPane;
   XWindowChanges windowChanges;
   ShellWidget shell;
   Window root, child;
   int rootx, rooty, childx, childy;
   unsigned int returnMask;

   /*
    * If either the menu manager or the associated widget is insensitive,
    * then ignore the post request.
    */
   assocWidget = XtParent (XtParent (menuMgr));
   if (!XtIsSensitive((Widget)menuMgr) || !XtIsSensitive(assocWidget))
      return;

   /*
    * if menus are not inherited, throw out events that did occur
    * in the associated widget's children.  
    */     
   if (menuMgr->menu_mgr.associateChildren == FALSE)
   {
      if ((event->xkey.window == XtWindow (XtParent (XtParent (menuMgr)))) &&
          (event->xkey.subwindow != 0) && 
          (XtWindowToWidget (XtDisplay (menuMgr), event->xkey.subwindow)))
      return;
   }

   /*
    * The following is a kludge fix to bypass a shortcoming within Xtk.
    * We need to manually inform the previous focus widget that it has
    * lost the cursor; this is because once we add our popups to the
    * grablist, the LeaveNotify will be ignored for the focus widget,
    * because it is not on the grablist.
    */
   /* if (menuMgr->manager.traversal_on) */
      SendFakeLeaveNotify(menuMgr, ULTIMATE_PARENT);

   /* Mark the menu system as 'active' */
   menuMgr->menu_mgr.menuActive = TRUE;

   /*
    * Position the menupane's parent shell.  If its a post, position to
    * the pointer position.  If its an accelerator, position to the center
    * of the associated widget
    */
   topLevelPane =(XwManagerWidget)menuMgr->popup_mgr.topLevelPane;

   /* 
    * In order for this algorithm to work, we need to make sure the
    * menupane has been realized; this is because its size is not
    * yet valid because it is not notified that it has any children
    * until it is realized.
    */
   if (!XtIsRealized (XtParent(topLevelPane )))
      XtRealizeWidget (XtParent (topLevelPane ));

   if ((event->type == ButtonPress) ||
       (event->type == ButtonRelease))
   {

      /*
       * The server does an implicit grab, but doesn't do it in the
       * manner we need for menupanes and menubuttons to continue
       * to highlight and function.
       */
      if (event->type == ButtonPress)
         XUngrabPointer (XtDisplay(menuMgr), CurrentTime);

      /*
       * If traversal is on, then save the current mouse position,
       * so that we can restore the mouse to its original position
       * when the menu is unposted.
       */
      if (menuMgr->manager.traversal_on)
      {
         menuMgr->popup_mgr.origMouseX = buttonEvent->x_root;
         menuMgr->popup_mgr.origMouseY = buttonEvent->y_root;
      }

      x = buttonEvent->x_root - (XwCASCADEWIDTH + FUDGE_FACTOR + 
                                 topLevelPane ->core.border_width);
      y = buttonEvent->y_root;

      /* Attempt to center on the first button in the new menupane */
      {
 
         if (topLevelPane->manager.num_managed_children > 0)
         {
            Widget firstButton = topLevelPane->manager.managed_children[0];

/*
            yDelta = firstButton->core.y +
                     firstButton->core.border_width +
                     (firstButton->core.height >> 1);
*/
            yDelta = 0;
         }
         else
         {
            yDelta = topLevelPane->core.border_width + 
                     (topLevelPane->core.height >> 1);
         }
      }

      y -= yDelta;
   }
   else
   {
      /*
       * center on the associated widget
       */      
      relativeX = (assocWidget->core.width>>1);
      relativeY = (assocWidget->core.height>>1);
  
      /* Get our coordinates, relative to the root window */
      XTranslateCoordinates (XtDisplay(menuMgr), 
                          assocWidget->core.window,
                          RootWindowOfScreen(XtScreen(menuMgr)),
                          relativeX,
                          relativeY,
                          &x, &y, &child);

      /*
       * If traversal is on, then save the current mouse position,
       * so that we can restore the mouse to its original position
       * when the menu is unposted.
       */
      if (menuMgr->manager.traversal_on)
      {
         if (XQueryPointer(XtDisplay(menuMgr), 
                           RootWindowOfScreen(menuMgr->core.screen),
                           &root, &child, &rootx, &rooty, &childx,
                           &childy, &returnMask))
         {
            menuMgr->popup_mgr.origMouseX = rootx;
            menuMgr->popup_mgr.origMouseY = rooty;
         }
      }
   }

   posx = x; posy = y;
   ForceMenuPaneOnScreen (topLevelPane, &posx, &posy);
   x = posx; y = posy;
   XtMoveWidget (XtParent(topLevelPane), x, y);

   /*
    * This allows us to catch all selects, and unpost the menus
    * regardless where the select event occurs.
    */
   XGrabPointer (XtDisplay(menuMgr), XtWindow(assocWidget), TRUE,
        ButtonPressMask|ButtonReleaseMask|EnterWindowMask|LeaveWindowMask|
        PointerMotionMask, GrabModeAsync, GrabModeAsync, None, (Cursor)NULL,
        CurrentTime);

   if ((menuMgr->popup_mgr.stickyMode) &&
       (menuMgr->popup_mgr.numSavedCascades))
   {
      /*
       * Attempt to gracefully handle the case where one of the menupanes
       * or menubuttons in the current cascade list has become insensitive
       * since the last posting.
       */
      for (i=0; i < menuMgr->popup_mgr.numSavedCascades; i++)
      {
         tempPane = (XwMenuPaneWidget)menuMgr->popup_mgr.savedCascadeList[i];

         if ((tempPane->menu_pane.attachId) &&
             (!XtIsSensitive(tempPane->menu_pane.attachId)))
         {
            /*
             * We are cascading from an insensitive menubutton.
             * Stop at the preceeding menupane.
             */
            menuMgr->popup_mgr.numSavedCascades = i;
            menuMgr->popup_mgr.lastSelected = tempPane->menu_pane.attachId;
            break;
         }
         else if (!XtIsSensitive((Widget)tempPane))
         {
            /*
             * If this menupane is insensitive, then stop here.
             */
            menuMgr->popup_mgr.numSavedCascades = i + 1;
            if (tempPane->manager.num_managed_children > 0)
            {
               menuMgr->popup_mgr.lastSelected = tempPane->manager.
                                                 managed_children[0];
            }
            else
               menuMgr->popup_mgr.lastSelected = (Widget)tempPane;
            break;
         }
      }

      /* Set up grab for the top level menupane */
      XtAddGrab (XtParent(topLevelPane), TRUE, TRUE);
      XtAddGrab ((Widget)topLevelPane, TRUE, TRUE);
      menuMgr->popup_mgr.currentCascadeList[menuMgr->popup_mgr.numCascades++] =
         (Widget)topLevelPane;

      /*
       * position the menupanes on the sticky cascade list
       */
      for (i=1; i < menuMgr->popup_mgr.numSavedCascades; i++)
      {
	 tempPane = (XwMenuPaneWidget) menuMgr->popup_mgr.savedCascadeList[i];
	 PositionCascade (tempPane->menu_pane.attachId, tempPane);
         XtAddGrab (XtParent(tempPane), FALSE, FALSE);
         XtAddGrab ((Widget)tempPane, FALSE, FALSE);
         menuMgr->popup_mgr.currentCascadeList
                 [menuMgr->popup_mgr.numCascades++] = (Widget)tempPane;
      }

      /* 
       * warp the pointer to its final destination.  This must be done
       * AFTER the panes are positioned, but BEFORE they are posted.
       */
      if (warpOn)
      {
	 XWarpPointer (XtDisplay (menuMgr), None,
		       XtWindow(menuMgr->popup_mgr.lastSelected),
		       0, 0, 0, 0, 5, 5);
      }

      /* Post the last menupane */
      shell = (ShellWidget)XtParent (menuMgr->popup_mgr.savedCascadeList
                [menuMgr->popup_mgr.numSavedCascades - 1]);
      shell->shell.popped_up = TRUE;
      shell->shell.grab_kind = XtGrabNonexclusive;
      shell->shell.spring_loaded = FALSE;
      if (!XtIsRealized((Widget)shell))
         XtRealizeWidget ((Widget)shell);
      XMapRaised (XtDisplay(shell), XtWindow(shell));

      /*
       * Post and then configure the menupanes and menubuttons.
       * THIS MUST BE DONE AFTER THE WARP POINTER!!!
       */
      for (i= menuMgr->popup_mgr.numSavedCascades - 2; i >= 0; i--)
      {
         /* 
          * Popup the pane and add to the grablist.
          * This cannot use _XtPopup() because it used XMapRaised().
          */
	 tempPane = (XwMenuPaneWidget) menuMgr->popup_mgr.savedCascadeList[i];
         shell = (ShellWidget) XtParent(tempPane);
         windowChanges.sibling = XtWindow(XtParent(menuMgr->popup_mgr.
                                          savedCascadeList[i+1]));
         windowChanges.stack_mode = Below;
         XConfigureWindow (XtDisplay(menuMgr), XtWindow(shell),
                           CWSibling | CWStackMode, &windowChanges);
         shell->shell.popped_up = TRUE;
         shell->shell.grab_kind = XtGrabNonexclusive;
         shell->shell.spring_loaded = FALSE;
         if (!XtIsRealized ((Widget)shell))
            XtRealizeWidget ((Widget)shell);
         XMapWindow (XtDisplay(shell), XtWindow(shell));

	 /*
	  * highlight menubutton, set its cascade flag and set up event
          * handler on its parent
	  */
         menuBtn = (((XwMenuPaneWidget)(menuMgr->popup_mgr.
                    savedCascadeList[i+1]))->menu_pane.attachId);

         if (menuMgr->manager.traversal_on == FALSE)
         {
	    (*(((XwMenuButtonWidgetClass)
	        XtClass(menuBtn))->menubutton_class.highlightProc)) (menuBtn);
         }

	 (*(((XwMenuButtonWidgetClass) XtClass(menuBtn))->
	                menubutton_class.setCascadeProc)) (menuBtn);

         if (menuMgr->manager.traversal_on == FALSE)
         {
	    XtAddEventHandler ((Widget)tempPane, EnterWindowMask, False,
				(XtEventHandler)
			        ((XwMenuButtonWidgetClass) XtClass(menuBtn))->
			        menubutton_class.enterParentProc,
			        menuBtn);
         }
      }

      /*
       * DON'T DO THE FOLLOWING; HAVE THE ITEM HILIGHT WHEN THE CURSOR
       * ENTERS IT.
       */
      /********
       * for keyboard posts, highlight the last selected menubutton.
       *
       *if ((warpOn == FALSE) && (menuMgr->manager.traversal_on == FALSE))
       *{
       * (*(((XwMenuButtonWidgetClass)
       *     XtClass(menuMgr->popup_mgr.lastSelected))->
       *             menubutton_class.highlightProc))
       *    (menuMgr->popup_mgr.lastSelected);
       *}
       *******/
   }
   else
   {
      /*
       * post only the toplevel.
       * post the toplevel menupane with exclusive grabs.  All other panes
       * have nonexclusive grabs.  
       */
      Post (menuMgr, topLevelPane, XtGrabExclusive);
   }

   /* Set the traversal focus to the last pane, if necessary */
   if (menuMgr->manager.traversal_on)
   {
      XwMoveFocus (menuMgr->popup_mgr.currentCascadeList [
                     menuMgr->popup_mgr.numCascades - 1]);
   }

   XFlush(XtDisplay(menuMgr));
}

/*************************************<->*************************************
 *
 *   ClassSelect(menuMgr, event)
 *
 *   Description:
 *   -----------
 *        Called only when a menubutton select accelerator is received.
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void ClassSelect (menuMgr, event)

   XwMenuMgrWidget  menuMgr;
   XEvent         * event;

{
   register int i;
   Widget assocWidget;
   register XwKeyAccel * accelerator;

   /*
    * If either the menu manager or the associated widget is insensitive,
    * then ignore the select accelerator.
    */
   assocWidget = XtParent(XtParent(menuMgr));
   if (!XtIsSensitive((Widget)menuMgr) || !XtIsSensitive(assocWidget))
      return;

   /*
    * map the event into an accelerator and call the menubuttons select rtn.
    */
   for (i=0; i < menuMgr->menu_mgr.numAccels; i++)
   {
      accelerator = menuMgr->menu_mgr.menuBtnAccelTable + i;

      if (_XwMatchKeyEvent (event,
		    accelerator->accelEventType,
		    accelerator->accelKey,
		    accelerator->accelModifiers))
      {
	 if (XtIsSensitive(accelerator->menuBtn) &&
             _XwAllAttachesAreSensitive(accelerator->menuBtn))
         {
	    (*(((XwMenuButtonWidgetClass)
	        XtClass (accelerator->menuBtn))-> primitive_class.select_proc))
	          (accelerator->menuBtn, event);
         }

	 return;
      }
   }
}


/*************************************<->*************************************
 *
 *   AddToPendingList(menuMgr, menupane, name)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void AddToPendingList (menuMgr, menupane, name)

   register XwMenuMgrWidget menuMgr;
   Widget menupane;
   String name;

{
   register int nameLen;
   register int i;
   Arg arg[1];
   register String button;

   /*
    * If there is already a request in the pending list for an attach
    * to the same menu button, then we need to remove the older one.
    */
   nameLen = XwStrlen(name);
   for (i = 0; i < menuMgr->menu_mgr.numAttachReqs; i++)
   {
      button = menuMgr->menu_mgr.pendingAttachList[i].menuBtnName;
      if ((strcmp(name, button) == 0) && (nameLen == XwStrlen(button)))
      {
         /* Detach the older request */
         XtSetArg (arg[0], XtNattachTo, (XtArgVal) NULL);
         XtSetValues (menuMgr->menu_mgr.pendingAttachList[i].menupaneId,
                      arg, 1);
         break;
      }
   }

   if (menuMgr->menu_mgr.numAttachReqs == menuMgr->menu_mgr.sizeAttachList)
   {
      /*
       * resize the list
       */
      menuMgr->menu_mgr.sizeAttachList = 2 * menuMgr->menu_mgr.sizeAttachList;
      menuMgr->menu_mgr.pendingAttachList = 
	 (XwAttachList *) XtRealloc((char *)(menuMgr->menu_mgr.pendingAttachList),
                                    menuMgr->menu_mgr.sizeAttachList *
				    sizeof(XwAttachList));
   }
   menuMgr->menu_mgr.pendingAttachList
      [menuMgr->menu_mgr.numAttachReqs].menuBtnName = name;
   menuMgr->menu_mgr.pendingAttachList
      [menuMgr->menu_mgr.numAttachReqs++].menupaneId = menupane->core.self;
}

/*************************************<->*************************************
 *
 *   SetButtonAccelerators (menuMgr, menubutton, accelString, accelEventType,
 * 		       	    accelKey, accelModifiers)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void SetButtonAccelerators (menuMgr, menubutton, accelString,
				   accelEventType, accelKey, accelModifiers)

   register XwPopupMgrWidget menuMgr;
   Widget           menubutton;
   String           accelString;
   unsigned int     accelEventType;
   unsigned int     accelKey;
   unsigned int     accelModifiers;

{
   Widget widget;
   register XwKeyAccel * accelerator;
   
   /*
    * add entry to menubutton accelerator table
    */
   if (menuMgr->menu_mgr.numAccels == menuMgr->menu_mgr.sizeAccelTable)
   {
      menuMgr->menu_mgr.sizeAccelTable =
	 2 * menuMgr->menu_mgr.sizeAccelTable;
      menuMgr->menu_mgr.menuBtnAccelTable = 
	 (XwKeyAccel *) XtRealloc((char *)(menuMgr->menu_mgr.menuBtnAccelTable),
                                  menuMgr->menu_mgr.sizeAccelTable *
				  sizeof(XwKeyAccel));
   }

   accelerator = menuMgr->menu_mgr.menuBtnAccelTable +
                 menuMgr->menu_mgr.numAccels;
   accelerator->accelString = accelString;
   accelerator->accelEventType = accelEventType;
   accelerator->accelKey = accelKey;
   accelerator->accelModifiers = accelModifiers;
   accelerator->menuBtn = menubutton->core.self;
   menuMgr->menu_mgr.numAccels++;
      
   /*
    * set translation in associated widget & toplevel pane for accelerator
    */
   widget =  (Widget) XtParent (XtParent (menuMgr));
   RegisterTranslation (widget, accelString, accelSelectTemplate, menuMgr);

   if (menuMgr->popup_mgr.topLevelPane)
   {
      RegisterTranslation (menuMgr->popup_mgr.topLevelPane, accelString, 
                           accelSelectTemplate, menuMgr);

      /*
       * Because of a short coming in the toolkit, we need to
       * potentially patch the top level widget's translations,
       * if it was in the middle of a setvalues.
       */
      if ((menuMgr->popup_mgr.attachPane) &&
         (menuMgr->popup_mgr.topLevelPane ==
         menuMgr->popup_mgr.attachPane->core.self))
      {
         menuMgr->popup_mgr.attachPane->core.tm = 
              menuMgr->popup_mgr.topLevelPane->core.tm;
         menuMgr->popup_mgr.attachPane->core.event_table = 
              menuMgr->popup_mgr.topLevelPane->core.event_table;
      }
   }

   /*
    * set up key grabs if possible
    */
   if ((menuMgr->menu_mgr.associateChildren) && (XtIsRealized (widget)))
      XGrabKey (XtDisplay (widget), accelKey, accelModifiers,
		XtWindow (widget), False, GrabModeAsync, GrabModeAsync);
}

/*************************************<->*************************************
 *
 *   SetTreeAccelerators (menuMgr, menupane)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void SetTreeAccelerators (menuMgr, menupane)

   XwMenuMgrWidget  menuMgr;
   XwMenuPaneWidget menupane;

{
   register int i;
   Arg args[2];
   unsigned int eventType;
   KeyCode key;
   unsigned int modifiers;
   KeySym tempKeysym;
   register WidgetList managed_children;

   managed_children = menupane->manager.managed_children;

   for (i=0; i < menupane->manager.num_managed_children; i++)
   {
      if (XtIsSubclass (managed_children[i], XwmenubuttonWidgetClass))
      {
	 String dkey = NULL;
	 Widget dtree = NULL;

	 XtSetArg (args[0], XtNkbdAccelerator, (XtArgVal)(&dkey));
	 XtSetArg (args[1], XtNcascadeOn, (XtArgVal)(&dtree));
	 XtGetValues (managed_children[i], args, XtNumber(args));

	 /*
	  * set up keyboard accelerators
	  */
	 if (dkey)
	 {
	    _XwMapKeyEvent (dkey, &eventType, &tempKeysym, &modifiers);
            key = XKeysymToKeycode (XtDisplay (menupane), tempKeysym);
	    SetButtonAccelerators (menuMgr, managed_children[i],
                                   dkey, eventType, key, modifiers);
	 }
	 /*
	  * traverse any submenus
	  */
	 if (dtree)
	    SetTreeAccelerators (menuMgr, dtree);
      }
   }
}

/*************************************<->*************************************
 *
 *   ClearTreeAccelerators (menuMgr, menupane)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void ClearTreeAccelerators (menuMgr, menupane)

   Widget           menuMgr;
   XwMenuPaneWidget menupane;

{
   register int i;
   Arg arg[1];
   register WidgetList managed_children;

   managed_children = menupane->manager.managed_children;
   
   for (i=0; i < menupane->manager.num_managed_children; i++)
   {
      if (XtIsSubclass (managed_children[i], XwmenubuttonWidgetClass))
      {
	 Widget twidg = NULL;
	 ClearSelectAccelerator(menuMgr, managed_children[i]);

	 /*
	  * clear accelerators from any submenu
	  */
	 XtSetArg (arg[0], XtNcascadeOn, (XtArgVal) &twidg);
	 XtGetValues (managed_children[i], arg, 1);
	 if (twidg)
	    ClearTreeAccelerators (menuMgr, twidg);
      }
   }
}

/*************************************<->*************************************
 *
 *   CompletePath(menuMgr, menupane)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Boolean CompletePath (menuMgr, menupane)

   XwPopupMgrWidget menuMgr;
   XwMenuPaneWidget menupane;

{
   register XwMenuPaneWidget pane;
   
   if (menuMgr->popup_mgr.topLevelPane == FALSE)
      return (FALSE);

   for (pane = menupane; pane != NULL;
	pane = (XwMenuPaneWidget) XtParent (pane->menu_pane.attachId))
   {
      if (pane == (XwMenuPaneWidget) menuMgr->popup_mgr.topLevelPane)
	 return (TRUE);

      if (pane->menu_pane.attachId == NULL)
	 return (FALSE);

      if (pane->core.managed == False)
	 return (FALSE);

      if ((pane->menu_pane.attachId)->core.mapped_when_managed == FALSE)
	 return (FALSE);

      if ((pane->menu_pane.attachId)->core.managed == FALSE)
	 return (FALSE);
   }
   return (FALSE);
}

/*************************************<->*************************************
 *
 *   PendingAttach (menuMgr, menubutton)
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static Widget PendingAttach (menuMgr, menubutton)

   XwMenuMgrWidget menuMgr;
   register Widget          menubutton;

{
   register int i;
   Widget id;
   register String buttonName = menubutton->core.name;
   register int buttonNameLen = XwStrlen(buttonName);
   register XwAttachList * pendingAttachList;
   
   pendingAttachList = menuMgr->menu_mgr.pendingAttachList;

   for (i=0; i < menuMgr->menu_mgr.numAttachReqs; i++)
   {
      if ((strcmp (pendingAttachList[i].menuBtnName, buttonName) == 0) &&
          (XwStrlen(pendingAttachList[i].menuBtnName) == buttonNameLen))
      {
         id = pendingAttachList[i].menupaneId;

         /* Remove from pending attach list */
         pendingAttachList[i] = pendingAttachList[--menuMgr->menu_mgr.
                                                  numAttachReqs];
           
	 return (id);
      }
   }

   return (NULL);
}

/*************************************<->*************************************
 *
 *   SetUpTranslation (widget, event, action)
 *
 *   Description:
 *   -----------
 *
 *   Inputs:
 *   ------
 *
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void SetUpTranslation (widget, event, action)

   Widget widget;
   String event;
   String action;

{
   register String workSpace;
   XtTranslations translations;
   
   workSpace = &workArea[0];

   strcpy (workSpace, "!");
   strcat (workSpace, event);
   strcat (workSpace, action);

   /*
    * compile the translation and attach to the menupane
    */
   translations = XtParseTranslationTable(workSpace);
   XtOverrideTranslations (widget, translations);
   /* XtDestroyStateTable (XtClass(widget), translations); */
}


/*************************************<->*************************************
 *
 *  SetPostMnemonic (parameters)
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

static void SetPostMnemonic (menuMgr, menupane, mnemonic)

   Widget menuMgr;
   Widget menupane;
   String mnemonic;
   
{
}


/*************************************<->*************************************
 *
 *  ClearPostMnemonic (parameters)
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

static void ClearPostMnemonic (menuMgr, menupane)

   Widget menuMgr;
   Widget menupane;
   
{
}


/*************************************<->*************************************
 *
 *  SetTitleAttributes (parameters)
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

static void SetTitleAttributes(w, x)
  Widget w, x;
{
}


/*************************************<->*************************************
 *
 *  ForceMenuPaneOnScreen (parameters)
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

static void ForceMenuPaneOnScreen (menupane, x, y)

   register Widget menupane;
   register Position * x;
   register Position * y;

{
   int rightEdgeOfMenu, dispWidth;
   int bottomEdgeOfMenu, dispHeight;

   /* 
    * In order for this algorithm to work, we need to make sure the
    * menupane has been realized; this is because its size is not
    * yet valid because it is not notified that it has any children
    * until it is realized.
    */
   if (!XtIsRealized (XtParent(menupane)))
      XtRealizeWidget (XtParent (menupane));

   /* Force the menupane to be completely visible */

   rightEdgeOfMenu = *x + (menupane->core.border_width << 1) +
                     menupane->core.width;
   bottomEdgeOfMenu = *y + (menupane->core.border_width << 1) +
                      menupane->core.height;
   dispWidth = WidthOfScreen (XtScreen(menupane));
   dispHeight = HeightOfScreen (XtScreen(menupane));

   if (*x < 0)
      *x = 0;

   if (*y < 0)
      *y = 0;

   if (rightEdgeOfMenu >= dispWidth)
      *x -= (rightEdgeOfMenu - dispWidth + 1);

   if (bottomEdgeOfMenu >= dispHeight)
      *y -= (bottomEdgeOfMenu - dispHeight + 1);
}


/*************************************<->*************************************
 *
 *  SetSelectMnemonic (parameters)
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

static void SetSelectMnemonic (menuMgr, menubutton, mnemonic)

   Widget menuMgr;
   Widget menubutton;
   String mnemonic;
   
{
}


/*************************************<->*************************************
 *
 *  ClearSelectMnemonic (parameters)
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

static void ClearSelectMnemonic (menuMgr, menupane)

   Widget menuMgr;
   Widget menupane;
   
{
}


/*************************************<->*************************************
 *
 *  OnCascadeList(parameters)
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

static Boolean OnCascadeList (menuMgr, menupane)

   register XwPopupMgrWidget menuMgr;
   register XwMenuPaneWidget menupane;
{
   register int i;

   if ((menuMgr->popup_mgr.stickyMode) &&
       (menuMgr->popup_mgr.numSavedCascades > 0))
   {
      for (i=0; i < menuMgr->popup_mgr.numSavedCascades; i++)
      {
         if ((Widget)menupane->core.self == 
              menuMgr->popup_mgr.savedCascadeList[i])
         {
            return (True);
         }
      }
   }

   return (False);
}

/*************************************<->*************************************
 *
 *   PaneManagedChildren()
 *
 *   Description:
 *   -----------
 *
 *
 *   Inputs:
 *   ------
 *
 * 
 *   Outputs:
 *   -------
 *
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/

static void PaneManagedChildren (menuMgr, menupane)

   register XwPopupMgrWidget  menuMgr;
   register XwMenuPaneWidget  menupane;

{
   register Widget child;
   Boolean * wasManaged;
   Arg args[2];
   unsigned int event;
   unsigned int tempKeySym;
   unsigned int modifiers;
   KeyCode key;
   register int i;
   Boolean parentOnCascadeList;
   String dkey;
   Widget dtree;

   
   if (CompletePath (menuMgr, menupane))
   {
      /*
       * If the parent menupane is in the sticky menu list, then we
       * need to remember this, so that we can check if any of its
       * children which are now being unmanaged are cascading to
       * another entry in the sticky menu list.  We will then clean
       * up the list, if necessary.
       */
      parentOnCascadeList = OnCascadeList (menuMgr, menupane);

      for (i=0; i < menupane->composite.num_children; i++)
      {
	 child = menupane->composite.children[i];
	 wasManaged = (Boolean *) child->core.constraints;

	 if ((*wasManaged == FALSE) && (child->core.managed == TRUE))
	 {
	    dkey = NULL;
	    dtree = NULL;

	    /*
	     * child has gone from unmanaged to managed
	     */
	    *wasManaged = TRUE;
	    XtSetArg (args[0], XtNkbdAccelerator, (XtArgVal) &dkey);
	    XtSetArg (args[1], XtNcascadeOn, (XtArgVal) &dtree);
	    XtGetValues (child, args, XtNumber(args));
	       
	    /*
	     * keyboard accelerator?
	     */
	    if (dkey)
	    {
	       _XwMapKeyEvent (dkey, &event, &tempKeySym, &modifiers);
	       key = XKeysymToKeycode (XtDisplay(menuMgr), tempKeySym);
	       SetSelectAccelerator (menuMgr, child, dkey,
				     event, key, modifiers);
	    }

	    /*
	     * Does this menubutton cascade?
	     */
	    if (dtree)
	       SetTreeAccelerators (menuMgr, dtree);
	 }
	 else if ((*wasManaged == TRUE) && (child->core.managed == FALSE))
	 {
	    dkey = NULL;
	    dtree = NULL;

	    /*
	     * child went from managed to unmanaged
	     */
            *wasManaged = FALSE;
	    XtSetArg (args[0], XtNkbdAccelerator, (XtArgVal) &dkey);
	    XtSetArg (args[1], XtNcascadeOn, (XtArgVal) &dtree);
	    XtGetValues (child, args, XtNumber(args));

	    /*
	     * accelerator to clear out?
	     */
	    if (dkey)
	       ClearSelectAccelerator (menuMgr, child);

	    /*
	     * Does this menubutton cascade?
	     */
	    if (dtree)
	    {
	       ClearTreeAccelerators (menuMgr, dtree);
               if (Visible (menuMgr, dtree))
	          Unpost (menuMgr, dtree);
               
               /*
                * If this button cascaded to a menupane which was on
                * the saved cascade list, then we need to clean up the
                * saved cascade list.
                */
               if (parentOnCascadeList && OnCascadeList(menuMgr, dtree))
               {
                  parentOnCascadeList = False;
                  menuMgr->popup_mgr.numSavedCascades = 0;
               }
	    }	

            /*
             * If this button was the last selected item on the saved
             * cascade list, then we need to clean up the list.
             */
            if (parentOnCascadeList && 
               (child == menuMgr->popup_mgr.lastSelected))
            {
               parentOnCascadeList = False;
               menuMgr->popup_mgr.numSavedCascades = 0;
            }
	 }
      }
   }
}


/*************************************<->*************************************
 *
 *   Visible
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

static Boolean Visible (menuMgr, menupane)

   XwPopupMgrWidget menuMgr;
   Widget menupane;

{
   register int i;
   register Widget * currentCascadeList =menuMgr->popup_mgr.currentCascadeList;

   for (i=0; i < menuMgr->popup_mgr.numCascades; i++)
      if (currentCascadeList[i] == menupane)
	 return (TRUE);

   return (FALSE);
}


/*************************************<->*************************************
 *
 *   SetMenuTranslations
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

static void SetMenuTranslations (menuMgr, translation)

   XwPopupMgrWidget menuMgr;
   XtTranslations translation;

{
   register int i, j, k;
   register CompositeWidget shell;
   register XwMenuPaneWidget menupane;

   /*
    * Since the menupanes are our popup grand children, we
    * will process them simply by traversing our popup children list.
    */
   for (i = 0; i < menuMgr->core.num_popups; i++)
   {
      shell = (CompositeWidget) menuMgr->core.popup_list[i];

      for (j = 0; j < shell->composite.num_children; j++)
      {
         /* Here we set the translation for the menupanes */
         menupane = (XwMenuPaneWidget)shell->composite.children[j];
         XtOverrideTranslations ((Widget)menupane, translation);

         for (k = 0; k < menupane->manager.num_managed_children; k++)
         {
            /* Here we set the translation for the menubuttons */
            XtOverrideTranslations(menupane->manager.managed_children[k],
                                    translation);
         }
      }
   }
}


/*************************************<->*************************************
 *
 *  TraverseRight(parameters)
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

static void TraverseRight (menuMgr, w)

   register XwPopupMgrWidget menuMgr;
   XwMenuButtonWidget w;

{
   XwMenuPaneWidget menupane;
   Arg arg[1];

   if ((menuMgr->manager.traversal_on) && (menuMgr->menu_mgr.menuActive) &&
      (XtParent(w) == menuMgr->popup_mgr.currentCascadeList[
       menuMgr->popup_mgr.numCascades - 1]))
   {
      /* Cascade to the menupane attached to the specified menubutton */
      XtSetArg (arg[0], XtNcascadeOn, (XtArgVal) &menupane);
      XtGetValues ((Widget)w, arg, XtNumber(arg));

      /*
       * Only cascade if there is a traversable primitive widget in 
       * the pane we would be cascading to.
       */
      if (_XwFindTraversablePrim (menupane) == FALSE)
         return;

      if (menupane)
         _XwCascadeSelect (w, menupane, NULL);
   }
}


/*************************************<->*************************************
 *
 *  TraverseLeft(parameters)
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

static void TraverseLeft (menuMgr, w)

   register XwPopupMgrWidget menuMgr;
   XwMenuButtonWidget w;

{
   /* Traverse to the previous menupane, if there is one */
   if ((menuMgr->menu_mgr.menuActive) &&
      (menuMgr->manager.traversal_on) &&
      (menuMgr->popup_mgr.numCascades > 1) &&
      (XtParent(w) == menuMgr->popup_mgr.currentCascadeList[
       menuMgr->popup_mgr.numCascades - 1]))
   {
      /* Unpost() will set the traversal focus, if needed */
      Unpost (menuMgr, menuMgr->popup_mgr.currentCascadeList[
                       menuMgr->popup_mgr.numCascades - 1]);
   }
}


/*************************************<->*************************************
 *
 *  TraverseUp(parameters)
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

static void TraverseUp (menuMgr, w)

   register XwPopupMgrWidget menuMgr;
   XwMenuButtonWidget w;

{
   if ((menuMgr->manager.traversal_on) && (menuMgr->menu_mgr.menuActive) &&
      (XtParent(w) == menuMgr->popup_mgr.currentCascadeList[
       menuMgr->popup_mgr.numCascades - 1]))
   {
      XwProcessTraversal (w, XwTRAVERSE_UP, TRUE);
   }
}


/*************************************<->*************************************
 *
 *  TraverseDown(parameters)
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

static void TraverseDown (menuMgr, w)

   XwPopupMgrWidget menuMgr;
   XwMenuButtonWidget w;

{
   if ((menuMgr->manager.traversal_on) && (menuMgr->menu_mgr.menuActive) &&
      (XtParent(w) == menuMgr->popup_mgr.currentCascadeList[
       menuMgr->popup_mgr.numCascades - 1]))
   {
      XwProcessTraversal (w, XwTRAVERSE_DOWN, TRUE);
   }
}


/*************************************<->*************************************
 *
 *  TraverseNextTop(parameters)
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

static void TraverseNextTop (menuMgr, w)

   XwPopupMgrWidget menuMgr;
   XwMenuButtonWidget w;

{
}


/*************************************<->*************************************
 *
 *  TraverseNext(parameters)
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

static void TraverseNext (menuMgr, w)

   XwPopupMgrWidget menuMgr;
   XwMenuButtonWidget w;

{
}


/*************************************<->*************************************
 *
 *  TraversePrev(parameters)
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

static void TraversePrev (menuMgr, w)

   XwPopupMgrWidget menuMgr;
   XwMenuButtonWidget w;

{
}


/*************************************<->*************************************
 *
 *  TraverseHome(parameters)
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

static void TraverseHome (menuMgr, w)

   XwPopupMgrWidget menuMgr;
   XwMenuButtonWidget w;

{
}


/*************************************<->*************************************
 *
 *  ManualPost(parameters)
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

static void ManualPost (menuMgr, pane, relativeTo, x, y)

   register XwPopupMgrWidget menuMgr;
   register Widget pane;
   Widget relativeTo;
   Position x;
   Position y;

{
   int introotx, introoty, intchildx, intchildy;
   Position rootx, rooty;
   Position childx, childy;
   Widget assocWidget;
   Window root, child;
   unsigned int returnMask;

   if (pane == (Widget)NULL)
      pane = (Widget) menuMgr->popup_mgr.topLevelPane;

   /* Get the widget Id for the associated widget */
   assocWidget = XtParent (XtParent (menuMgr));

   /*
    * Do nothing if the menu is already active, there is no top level
    * menupane, the menu manager is insensitive, or the associated
    * widget is insensitive.
    */
   if ((menuMgr->menu_mgr.menuActive == TRUE) || (pane == NULL) ||
      !XtIsSensitive((Widget)menuMgr) || !XtIsSensitive(assocWidget)) {
      return;
   }

   /* 
    * Determine where to post the menu and translate coordinates, 
    * if necessary.
    */
   if (relativeTo)
   {
      /*
       * If the coordinates are relative to a widget, then we need
       * to map them relative to the root window.
       */
      /* XtTranslateCoords (relativeTo, x, y, &rootx, &rooty); */
      XTranslateCoordinates (XtDisplay(relativeTo), XtWindow(relativeTo), 
                             RootWindowOfScreen(XtScreen(menuMgr)),
                             x, y, &introotx, &introoty, &child);
      rootx = introotx;
      rooty = introoty;
   }
   else
   {
      /*
       * If no widget is specified, then the coordinates are assumed
       * to already be relative to the root window.
       */
      rootx = x;
      rooty = y;
   }

   /*
    * The following is a kludge fix to bypass a shortcoming within Xtk.
    * We need to manually inform the previous focus widget that it has
    * lost the cursor; this is because once we add our popups to the
    * grablist, the LeaveNotify will be ignored for the focus widget,
    * because it is not on the grablist.
    */
   /* if (menuMgr->manager.traversal_on) */
      SendFakeLeaveNotify(menuMgr, ULTIMATE_PARENT);

   /* Mark the menu system as active */
   menuMgr->menu_mgr.menuActive = TRUE;

   /* Position the menupane */
   if (!XtIsRealized (XtParent(pane)))
      XtRealizeWidget (XtParent(pane));
   ForceMenuPaneOnScreen (pane, &rootx, &rooty);
   XtMoveWidget (XtParent(pane), rootx, rooty);

   /*
    * Set up a pointer grab; this allows us to catch all selects and
    * to then unpost the menus, regardless as to where the select
    * occurred.
    */
   XGrabPointer (XtDisplay(menuMgr), XtWindow(assocWidget), TRUE,
         ButtonPressMask|ButtonReleaseMask|EnterWindowMask|LeaveWindowMask|
         PointerMotionMask, GrabModeAsync, GrabModeAsync, None, (Cursor)NULL,
         CurrentTime);

   /* Post the menupane */
   Post (menuMgr, pane, XtGrabExclusive);

   /*
    * If traversal is on, then save the current mouse position,
    * so that we can restore the mouse to its original position
    * when the menu is unposted.
    */
   if (menuMgr->manager.traversal_on)
   {
      if (XQueryPointer(XtDisplay(menuMgr), 
                        RootWindowOfScreen(menuMgr->core.screen),
                        &root, &child, &introotx, &introoty, &intchildx,
                        &intchildy, &returnMask))
      {
         menuMgr->popup_mgr.origMouseX = introotx;
         menuMgr->popup_mgr.origMouseY = introoty;
      }

      /* Set the traversal focus to the pane */
      XwMoveFocus (pane);
   }
}


/*************************************<->*************************************
 *
 *  XwPostPopup(parameters)
 *
 *   Description:
 *   -----------
 *     This is a utility routine which may be used by an application to
 *     manually post a popup menu pane.
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

void XwPostPopup (menuMgr, pane, relativeTo, x, y)

   Widget menuMgr, pane, relativeTo;
   Position x, y;

{
   /* Only popup a menu if it is a subclass of the popup menu manager */
   /* Do nothing if invalid menu manager Id */
   if (menuMgr == NULL) 
      return;

   if (XtIsSubclass (menuMgr, XwpopupmgrWidgetClass))
   {
       (*(((XwPopupMgrWidgetClass)
	XtClass(menuMgr))-> popup_mgr_class.manualPost))
         (menuMgr, pane, relativeTo, x, y);
   }
   else
   {
      XtWarning ("PopupMgr: Widget is not a subclass of menuMgr");
   }
}


/*************************************<->*************************************
 *
 *  SendFakeLeaveNotify(parameters)
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

static void SendFakeLeaveNotify (focus_widget, mode)

   Widget focus_widget;
   int mode;

{
   XEvent event;

   if (mode == ULTIMATE_PARENT)
   {
      /* Search up to top level shell */
      while (XtParent(focus_widget) != NULL)
         focus_widget = XtParent(focus_widget);
   }
   else
   {
      /* Search up to the shell in which this menubtn resides */
      while ((!XtIsSubclass (focus_widget, shellWidgetClass)) &&
             (XtParent(focus_widget) != NULL))
         focus_widget = XtParent(focus_widget);
   }

   event.type = FocusOut;
   event.xfocus.serial = LastKnownRequestProcessed(XtDisplay(focus_widget));
   event.xfocus.send_event = True;
   event.xfocus.display = XtDisplay(focus_widget);
   event.xfocus.window = XtWindow(focus_widget);
   event.xfocus.mode = NotifyNormal;
   event.xfocus.detail = NotifyAncestor;

   XtDispatchEvent (&event);
}


/*************************************<->*************************************
 *
 *  SetUpStickyList(parameters)
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

static void SetUpStickyList (menuMgr, widget)

   XwPopupMgrWidget menuMgr;
   Widget widget;

{
   int i;

   menuMgr->popup_mgr.numSavedCascades = 0;

   if ((XtIsSubclass (widget, XwmenubuttonWidgetClass)) &&
       (menuMgr->popup_mgr.stickyMode) &&
       (XtParent(widget) == menuMgr->popup_mgr.currentCascadeList
          [menuMgr->popup_mgr.numCascades -1]))
   {
      if (menuMgr->popup_mgr.numCascades >
          menuMgr->popup_mgr.sizeSavedCascadeList)
      {
          menuMgr->popup_mgr.sizeSavedCascadeList =
             menuMgr->popup_mgr.sizeCascadeList;
	  menuMgr->popup_mgr.savedCascadeList =
	     (Widget *) XtRealloc((char *)(menuMgr->popup_mgr.savedCascadeList),
                                  menuMgr->popup_mgr.sizeSavedCascadeList *
			          sizeof (Widget));
       }

       menuMgr->popup_mgr.numSavedCascades = menuMgr->popup_mgr.numCascades;
       menuMgr->popup_mgr.lastSelected = widget;

       /* Set this widget as the traversal item */
       if (menuMgr->manager.traversal_on)
       {
          XwMenuPaneWidget pane;

          pane = ((XwMenuPaneWidget)(menuMgr->popup_mgr.currentCascadeList 
          [menuMgr->popup_mgr.numCascades -1]));
          pane->manager.active_child = widget;
          XtSetKeyboardFocus ((Widget)pane, widget);
       }

       for (i=0; i < menuMgr->popup_mgr.numSavedCascades; i++)
          menuMgr->popup_mgr.savedCascadeList[i] =
	          menuMgr->popup_mgr.currentCascadeList[i];
   }
   else if (menuMgr->manager.traversal_on)
   {
      /* Force the first item to be highlighted next time */
      ((XwPopupMgrWidget)menuMgr->popup_mgr.topLevelPane)->
                       manager.active_child = NULL;
      XtSetKeyboardFocus (menuMgr->popup_mgr.topLevelPane, NULL);
   }
}


/*************************************<->*************************************
 *
 *  BtnSensitivityChanged(parameters)
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

static void BtnSensitivityChanged (menuMgr, btn)

   XwPopupMgrWidget menuMgr;
   Widget btn;

{
   /* Noop */
}


/*************************************<->*************************************
 *
 *  PaneSensitivityChanged(parameters)
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

static void PaneSensitivityChanged (menuMgr, pane)

   XwPopupMgrWidget menuMgr;
   Widget pane;

{
   /* Noop */
}
