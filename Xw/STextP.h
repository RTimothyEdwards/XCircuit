/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        STextP.h    
 **
 **   Project:     X Widgets
 **
 **   Description: Private include file for StaticText class
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



/********************************************
 *
 *   No new fields need to be defined
 *   for the StaticText widget class record
 *
 ********************************************/
typedef struct {int foo;} XwStaticTextClassPart;

/****************************************************
 *
 * Full class record declaration for StaticText class
 *
 ****************************************************/
typedef struct _XwStaticTextClassRec {
	CoreClassPart      	core_class;
	XwPrimitiveClassPart 	primitive_class;
	XwStaticTextClassPart	statictext_class;
} XwStaticTextClassRec;

/********************************************
 *
 * New fields needed for instance record
 *
 ********************************************/
typedef struct _XwStaticTextPart {
	/*
	 * "Public" members (Can be set by resource manager).
	 */
	char       	*input_string; /* String sent to this widget. */
    XwAlignment	alignment; /* Alignment within the box */
	int       	gravity; /* Controls use of extra space in window */
	Boolean    	wrap;  /* Controls wrapping on spaces */
	Boolean		strip; /* Controls stripping of blanks */
	int     	line_space; /* Ratio of font height use as dead space
							   between lines.  Can be less than zero 
							   but not less than -1.0  */
	XFontStruct	*font;  /* Font to write in. */
	Dimension  	internal_height; /* Space from text to top and 
									bottom highlights */
	Dimension  	internal_width; /* Space from left and right side highlights */
	/*
	 * "Private" members -- values computed by 
	 *  XwStaticTextWidgetClass methods.
	 */
	GC         	normal_GC; /* GC for text */
	XRectangle 	TextRect; /* The bounding box of the text, or clip rectangle
							 of the window; whichever is smaller. */
	char       	*output_string; /* input_string after formatting */
} XwStaticTextPart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/
typedef struct _XwStaticTextRec {
	CorePart      	core;
	XwPrimitivePart 	primitive;
	XwStaticTextPart	static_text;
} XwStaticTextRec;

