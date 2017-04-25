/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        WorkSpaceP.h    
 **
 **   Project:     X Widgets
 **
 **   Description: Private include file for the WorkSpace class
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


/*  WorkSpace class structure  */

typedef struct _XwWorkSpaceClassPart
{
   int foo;	/*  No new fields needed  */
} XwWorkSpaceClassPart;


/*  Full class record declaration for WorkSpace class  */

typedef struct _XwWorkSpaceClassRec
{
   CoreClassPart         core_class;
   XwPrimitiveClassPart  primitive_class;
   XwWorkSpaceClassPart    workSpace_class;
} XwWorkSpaceClassRec;

extern XwWorkSpaceClassRec XwworkSpaceClassRec;


/*  The WorkSpace instance record  */

typedef struct _XwWorkSpacePart
{
   XtCallbackList resize;
   XtCallbackList expose;
   XtCallbackList key_down;
   XtCallbackList key_up;
} XwWorkSpacePart;


/*  Full instance record declaration  */

typedef struct _XwWorkSpaceRec
{
   CorePart	   core;
   XwPrimitivePart primitive;
   XwWorkSpacePart workSpace;
} XwWorkSpaceRec;

