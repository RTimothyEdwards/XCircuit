/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        XwP.h
 **
 **   Project:     X Widgets
 **
 **   Description: This include file contains the class and instance record
 **                definitions for all meta classes.  It also contains externs
 **                for internally shared functions and defines for internally 
 **                shared values.
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


/***********************************************************************
 *
 * Miscellaneous Private Defines
 *
 ***********************************************************************/

#define XwStrlen(s)      ((s) ? strlen(s) : 0)
#define XwBLOCK 10
#define XwMANAGER 1
#define XwPRIMITIVE 2


/************************************************************************
 *
 *  The Primitive Widget Class and instance records.
 *
 ************************************************************************/

typedef void (*XwEventProc)(
#if NeedFunctionPrototypes
    Widget,     /* widget */
    XEvent*	/* event */
#endif
);

typedef struct _XwPrimitiveClassPart
{
   XtWidgetProc   border_highlight;
   XtWidgetProc   border_unhighlight;
   XwEventProc    select_proc;
   XwEventProc    release_proc;
   XwEventProc    toggle_proc;
   XtTranslations translations;
} XwPrimitiveClassPart;

typedef struct _XwPrimitiveClassRec
{
    CoreClassPart        core_class;
    XwPrimitiveClassPart primitive_class;
} XwPrimitiveClassRec;

extern XwPrimitiveClassRec XwprimitiveClassRec;

#define XtInheritBorderHighlight   ((XtWidgetProc) _XtInherit)
#define XtInheritBorderUnhighlight ((XtWidgetProc) _XtInherit)
#define XtInheritSelectProc 	   ((XwEventProc)  _XtInherit)
#define XtInheritReleaseProc 	   ((XwEventProc)  _XtInherit)
#define XtInheritToggleProc 	   ((XwEventProc)  _XtInherit)

extern void _XwHighlightBorder();
extern void _XwUnhighlightBorder();

extern void _XwPrimitiveEnter();
extern void _XwPrimitiveLeave();
extern void _XwPrimitiveFocusIn();
extern void _XwPrimitiveFocusOut();
extern void _XwPrimitiveVisibility();
extern void _XwPrimitiveUnmap();

extern void _XwTraverseLeft();
extern void _XwTraverseRight();
extern void _XwTraverseUp();
extern void _XwTraverseDown();
extern void _XwTraverseNext();
extern void _XwTraversePrev();
extern void _XwTraverseHome();
extern void _XwTraverseNextTop();


typedef struct _XwPrimitivePart
{
   Pixel   foreground;
   int     background_tile;

   Cursor  cursor;

   int     traversal_type;
   Boolean I_have_traversal;

   int     highlight_thickness;
   int     highlight_style;
   Pixel   highlight_color;
   int     highlight_tile;


   Boolean recompute_size;

   GC      highlight_GC;

   Boolean display_sensitive;
   Boolean highlighted;
   Boolean display_highlighted;

   XtCallbackList select;
   XtCallbackList release;
   XtCallbackList toggle;
} XwPrimitivePart;

typedef struct _XwPrimitiveRec
{
   CorePart      core;
   XwPrimitivePart primitive;
} XwPrimitiveRec;



/************************************************************************
 *
 *  The Manager Widget Class and instance records.
 *
 ************************************************************************/

typedef void (*XwTraversalProc)(
#if NeedFunctionPrototypes
    Widget, /* widget */
    XPoint, /* ul     */
    XPoint, /* lr     */
    int     /* dir    */
#endif
);

typedef struct _XwManagerClassPart
{
   XwTraversalProc traversal_handler;
   XtTranslations translations;
} XwManagerClassPart;

typedef struct _XwManagerClassRec
{
    CoreClassPart       core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XwManagerClassPart  manager_class;
} XwManagerClassRec;

extern XwManagerClassRec XwmanagerClassRec;

typedef struct _XwManagerPart
{
   Pixel   foreground;
   int     background_tile;

   Boolean traversal_on;

   int     highlight_thickness;

   GC      highlight_GC;

   Widget     active_child;
   WidgetList managed_children;
   Cardinal   num_slots;
   Cardinal   num_managed_children;
   int        layout;
   XtCallbackList next_top;
} XwManagerPart;

typedef struct _XwManagerRec
{
   CorePart       core;
   CompositePart  composite;
   ConstraintPart constraint;
   XwManagerPart  manager;
} XwManagerRec;


#define XtInheritTraversalProc ((XwTraversalProc) _XtInherit)

/**************************************
 *  
 * Exported Routines from Manager.c
 *
 **************************************/
extern void _XwManagerEnter();             /* start traversal               */
extern void _XwManagerLeave();             /* end traversal                 */
extern void _XwManagerFocusIn();           /* focus management              */
extern void _XwManagerVisibility(); 	   /* more focus management         */
extern void _XwManagerUnmap();     	   /* even more focus management    */
extern Boolean _XwFindTraversablePrim();   /* traversal checking routine    */
extern void _XwReManageChildren();         /* rebuild Managed Children List */
extern void _XwManagerInsertChild();       /* special insert child routine  */
extern Cardinal _XwInsertOrder();	   /* compute position from arglist */


/************************************************************************
 *
 *  Externs and defines for traversal handling functions incorporated 
 *  into all primitive widgets.
 *
 ************************************************************************/

extern void mgr_traversal();
extern void _XwDrawBox();
extern void XwSetFocusPath();
extern void XwMoveFocus();
extern void XwProcessTraversal();
extern void XwSetInputFocus();
extern void _XwInitCheckList();


/* Traversal direction defines */

#define XwTRAVERSE_CURRENT      0
#define XwTRAVERSE_LEFT         1
#define XwTRAVERSE_RIGHT        2
#define XwTRAVERSE_UP           3
#define XwTRAVERSE_DOWN         4
#define XwTRAVERSE_NEXT         5
#define XwTRAVERSE_PREV         6
#define XwTRAVERSE_HOME         7
#define XwTRAVERSE_NEXT_TOP     8


/**************************************
 *  
 * Exported Routines from Button.c
 *
 **************************************/
extern void _XwSetTextWidthAndHeight();
extern void _XwGetNormalGC();
extern void _XwGetSensitiveGC();
extern void _XwGetInverseGC();
extern void _XwInitializeButton();
extern void _XwRealize();
extern void _XwResizeButton();
extern void _XwRegisterName();
extern Boolean _XwRecomputeSize();
extern Boolean XwTestTraversability();

/************************************************************************
 *
 *  New fields for the Button widget class record
 *  and the full class record.
 *
 ************************************************************************/

typedef struct _XwButtonClassPart
{
   int mumble;
} XwButtonClassPart;

typedef struct _XwButtonClassRec
{
   CoreClassPart        core_class;
   XwPrimitiveClassPart primitive_class;
   XwButtonClassPart    button_class;
} XwButtonClassRec;

extern XwButtonClassRec XwbuttonClassRec;


/************************************************************************
 *
 *  New fields for the Button instance record and
 *  the full instance record.
 *
 ************************************************************************/

typedef struct _XwButtonPart
{
   XFontStruct * font;
   char        * label;
   int           label_location;
   Dimension     internal_height;
   Dimension     internal_width;
   int		 sensitive_tile;
   GC            normal_GC;
   GC            inverse_GC;
   Position      label_x;
   Position      label_y;
   Dimension     label_width;
   Dimension     label_height;
   unsigned int  label_len;
   Boolean       set;
   Boolean       display_set;
} XwButtonPart;

typedef struct _XwButtonRec
{
   CorePart        core;
   XwPrimitivePart primitive;
   XwButtonPart    button;
} XwButtonRec;


/***********************************************************************
 *
 *  Inherited functions exported by MenuBtn
 *
 ***********************************************************************/

#define XtInheritIdealWidthProc ((XwWidthProc) _XtInherit)
#define XtInheritUnhighlightProc ((XtWidgetProc) _XtInherit)
#define XtInheritHighlightProc ((XtWidgetProc) _XtInherit)
#define XtInheritEnterParentProc ((XtWidgetProc) _XtInherit)

/* Corrected by Steve Langasek (was incompatible with the revised Wprintf()) */

typedef void (*XwStrProc)(
#if NeedFunctionPrototypes
    char *format, ...  /* hint */
#endif
);

/***********************************************************************
 *
 *  New Fields for the MenuPane widget class record, and
 *  the full class record.
 *
 ***********************************************************************/

typedef void (*XwWBoolProc)(
#if NeedFunctionPrototypes
    Widget,     /* widget */
    Boolean	/* flag  */
#endif
);

typedef struct {
     XwWBoolProc setTraversalFlag;   
} XwMenuPaneClassPart;

/* Full class record declaration */
typedef struct _XwMenuPaneClassRec {
    CoreClassPart	core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XwManagerClassPart  manager_class;
    XwMenuPaneClassPart	menu_pane_class;
} XwMenuPaneClassRec;

extern XwMenuPaneClassRec XwmenupaneClassRec;
extern Boolean _XwAllAttachesAreSensitive();

#define XtInheritMenuPaneConstraintInit ((XtInitProc) _XtInherit)
#define XtInheritSetTraversalFlagProc   ((XwWBoolProc) _XtInherit)

/***********************************************************************
 *
 *  New Fields for the MenuPane widget instance record, and
 *  the full class record.
 *
 ***********************************************************************/

typedef struct {
    /* Internal fields */
    GC            titleGC;
    Widget        attachId;

    /* User settable fields */
    XFontStruct * title_font;
    String        title_string;
    Boolean       title_showing;
    String        attach_to;
    XImage      * titleImage;
    Boolean       title_override;
    int           title_type;
    String        mnemonic;
    XtCallbackList select;
} XwMenuPanePart;


typedef struct _XwMenuPaneRec {
    CorePart	    core;
    CompositePart   composite;
    ConstraintPart  constraint;
    XwManagerPart   manager;
    XwMenuPanePart  menu_pane;
} XwMenuPaneRec;


/* New Procedures for _XtInherit, with proper function prototypes */

typedef void (*XwWidthProc)(
#if NeedFunctionPrototypes
    Widget,     /* widget */
    Dimension*  /* width  */
#endif
);

typedef void (*XwWintProc)(
#if NeedFunctionPrototypes
    Widget,     /* widget */
    int		/* type  */
#endif
);

typedef void (*XwWWProc)(
#if NeedFunctionPrototypes
    Widget,     /* manager */
    Widget	/* pane */
#endif
);

typedef void (*XwSelectProc)(
#if NeedFunctionPrototypes
    Widget,     /* manager */
    Widget,	/* button */
    String,	/* accelerator */
    unsigned int, /* accelEventType */
    unsigned int, /* accelDetail */
    unsigned int  /* accelModifiers */
#endif
);

typedef void (*XwWWSProc)(
#if NeedFunctionPrototypes
    Widget,     /* manager */
    Widget,	/* button */
    String	/* accelerator */
#endif
);

typedef void (*XwPostProc)(
#if NeedFunctionPrototypes
    Widget,     /* widget */
    Widget,     /* widget */
    Widget,     /* widget */
    Position,	/* x */
    Position	/* y */
#endif
);

typedef void (*XwSetSProc)(
#if NeedFunctionPrototypes
    Widget,  	 /* widget */                     
    Position,    /* left   */
    Position     /* right  */
#endif
);

/***********************************************************************
 *
 *  Global functions exported by MapEvent.c
 *
 ***********************************************************************/

typedef Boolean (*XwBooleanProc)();

extern Boolean _XwMatchBtnEvent();
extern Boolean _XwMapBtnEvent();
extern Boolean _XwMapKeyEvent();
extern Boolean _XwMatchKeyEvent();
extern Boolean _XwValidModifier();
extern String  _XwMapToHex();
extern unsigned long _XwMapFromHex();

extern void _XwSetMappedManagedChildrenList();

#define XtInheritAttachPane ((XwWWSProc) _XtInherit)
#define XtInheritDetachPane ((XwWWSProc) _XtInherit)
#define XtInheritAddPane ((XwWWProc) _XtInherit)
#define XtInheritSetSelectAccelerator ((XwSelectProc) _XtInherit)
#define XtInheritClearSelectAccelerator ((XwWWProc) _XtInherit)
#define XtInheritSetPostMnemonic ((XwWWSProc) _XtInherit)
#define XtInheritClearPostMnemonic ((XwWWProc) _XtInherit)
#define XtInheritAddButton ((XwWWProc) _XtInherit)
#define XtInheritSetSelectMnemonic ((XwWWSProc) _XtInherit)
#define XtInheritClearSelectMnemonic ((XwWWProc) _XtInherit)
#define XtInheritProcessSelect ((XwBooleanProc) _XtInherit)
#define XtInheritValidEvent ((XwBooleanProc) _XtInherit)
#define XtInheritDoICascade ((XwBooleanProc) _XtInherit)
#define XtInheritSetTitleAttributes ((XwWWProc) _XtInherit)
#define XtInheritPaneManagedChildren ((XwWWProc) _XtInherit)
#define XtInheritBtnSensitivityChanged ((XwWWProc) _XtInherit)
#define XtInheritPaneSensitivityChanged ((XwWWProc) _XtInherit)

/***********************************************************************
 *
 *  New Fields for the MenuMgr widget class record, and
 *  the full class record.
 *
 ***********************************************************************/

typedef struct {
    XwWWSProc     attachPane;
    XwWWSProc     detachPane;
    XwWWProc      addPane;
    XwSelectProc  setSelectAccelerator;
    XwWWProc      clearSelectAccelerator;
    XwWWSProc     setPostMnemonic;
    XwWWProc      clearPostMnemonic;
    XwWWProc      addButton;
    XwBooleanProc processSelect;
    XwBooleanProc validEvent;
    XwBooleanProc doICascade;
    XwWWSProc     setSelectMnemonic;
    XwWWProc      clearSelectMnemonic;
    XwWWProc      setTitleAttributes;
    XwWWProc      paneManagedChildren;
    XwEventProc   traverseLeft;
    XwEventProc   traverseRight;
    XwEventProc   traverseNext;
    XwEventProc   traversePrev;
    XwEventProc   traverseHome;
    XwEventProc   traverseUp;
    XwEventProc   traverseDown;
    XwEventProc   traverseNextTop;
    XwWWProc      btnSensitivityChanged;
    XwWWProc      paneSensitivityChanged;
} XwMenuMgrClassPart;

/* Full class record declaration */
typedef struct _XwMenuMgrClassRec {
    CoreClassPart	core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XwManagerClassPart  manager_class;
    XwMenuMgrClassPart	menu_mgr_class;
} XwMenuMgrClassRec;

extern XwMenuMgrClassRec XwmenumgrClassRec;

/***********************************************************************
 *
 *  New Fields for the MenuMgr widget instance record, and
 *  the full class record.
 *
 ***********************************************************************/

/* Structure used by menu mgr to store keyboard accelerators */
typedef struct {
    String       accelString;
    unsigned int accelEventType;
    KeyCode      accelKey;
    unsigned int accelModifiers;
    Widget       menuBtn;
} XwKeyAccel;

/* Structure used by menu mgr to store pending attach requests */
typedef struct {
    String  menuBtnName;
    Widget  menupaneId;
} XwAttachList;

typedef struct {
    /* Internal fields */
    unsigned int   postEventType;
    unsigned int   postButton;
    unsigned int   postModifiers;
    unsigned int   selectEventType;
    unsigned int   selectButton;
    unsigned int   selectModifiers;
    XwKeyAccel   * menuBtnAccelTable;
    int            numAccels;
    int            sizeAccelTable;
    XwAttachList * pendingAttachList;
    int            numAttachReqs;
    int            sizeAttachList;
    Boolean        menuActive;
    unsigned int   unpostEventType;
    KeyCode        unpostKey;
    unsigned int   unpostModifiers;
    unsigned int   kbdSelectEventType;
    KeyCode        kbdSelectKey;
    unsigned int   kbdSelectModifiers;

    /* User settable fields */
    Boolean       associateChildren;
    String        postString;
    String        selectString;
    String        unpostString;
    String        kbdSelectString;
} XwMenuMgrPart;


typedef struct _XwMenuMgrRec {
    CorePart	    core;
    CompositePart   composite;
    ConstraintPart  constraint;
    XwManagerPart   manager;
    XwMenuMgrPart   menu_mgr;
} XwMenuMgrRec;



