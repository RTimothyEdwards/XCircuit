/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        FormP.h    
 **
 **   Project:     X Widgets
 **
 **   Description: Private include file for the Form class
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


/*  Form constraint rec  */

typedef struct _XwFormConstraintRec
{
   String  x_ref_name;		/*  the name of the widget to reference     */
   Widget  x_ref_widget;	/*  the widget to reference                 */
   int     x_offset;		/*  the offset (pixels) from the reference  */
   Boolean x_add_width;		/*  add width of the reference to x coord   */
   Boolean x_vary_offset;	/*  able to vary the seperation             */
   Boolean x_resizable;		/*  able to resize in x direction           */
   Boolean x_attach_right;	/*  attached to the right edge of the form  */
   int     x_attach_offset;	/*  offset (pixels) from attached edge      */

   String  y_ref_name;		/*  y constraints are the same as x  */
   Widget  y_ref_widget;
   int     y_offset;
   Boolean y_add_height;
   Boolean y_vary_offset;
   Boolean y_resizable;
   Boolean y_attach_bottom;
   int     y_attach_offset;


   Boolean   managed;		/*  whether the widget is managed or not     */
   int       x, y;		/*  location after constraint processing     */
   Dimension width, height;	/*  size after constraint processing         */

   int       set_x;		/*  original y                               */
   int       set_y;		/*  original x                               */
   Dimension set_width;		/*  original or set values width of widget   */
   Dimension set_height;	/*  original or set values height of widget  */

   Dimension width_when_unmanaged;
   Dimension height_when_unmanaged;
} XwFormConstraintRec;
      

/*  Form class structure  */

typedef struct _XwFormClassPart
{
   int foo;	/*  No new fields needed  */
} XwFormClassPart;


/*  Full class record declaration for Form class  */

typedef struct _XwFormClassRec
{
   CoreClassPart        core_class;
   CompositeClassPart   composite_class;
   ConstraintClassPart  constraint_class;
   XwManagerClassPart   manager_class;
   XwFormClassPart      form_class;
} XwFormClassRec;

extern XwFormClassRec XwformClassRec;


/*  The Form tree structure used for containing the constraint tree  */
  
struct _XwFormRef
{
   Widget  this;		/*  the widget these constaints are for     */
   Widget  ref;			/*  the widget this constraint is for       */
   int     offset;		/*  offset (pixels) from parent ref         */
   Boolean add;			/*  add (width or height) of parent ref     */
   Boolean vary;		/*  able to vary the seperation             */
   Boolean resizable;		/*  able to resize                          */
   Boolean attach;		/*  attached to the edge (right or bottom)  */
   int     attach_offset;	/*  offset (pixels) from attached edge      */

   int set_loc;		   /*  the initial or set value location  */
   int set_size;	   /*  the initial or set value size      */
   
   struct _XwFormRef ** ref_to;		/*  child references            */
   int                  ref_to_count;	/*  number of child references  */
};

typedef struct _XwFormRef XwFormRef;


typedef struct _XwFormProcess
{
   XwFormRef * ref;
   int loc;
   int size;
   Boolean leaf;
} XwFormProcess;

/*  The Form instance record  */

typedef struct _XwFormPart
{
   XwFormRef * width_tree;
   XwFormRef * height_tree;
} XwFormPart;


/*  Full instance record declaration  */

typedef struct _XwFormRec
{
   CorePart	    core;
   CompositePart    composite;
   ConstraintPart   constraint;
   XwManagerPart    manager;
   XwFormPart       form;
} XwFormRec;




