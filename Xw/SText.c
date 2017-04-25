/*************************************<+>*************************************
 *****************************************************************************
 **
 **		File:        SText.c
 **
 **		Project:     X Widgets
 **
 **		Description: Code/Definitions for StaticText widget class.
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
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/keysymdef.h>
#include <Xw/Xw.h>
#include <Xw/XwP.h>
#include <Xw/SText.h>
#include <Xw/STextP.h>

static void Initialize ();
static void Realize();
static void Destroy();
static void Resize();
static Boolean SetValues();
static void Redisplay();
static void Select();
static void Release();
static void Toggle();
static void ClassInitialize();


/*************************************<->*************************************
 *
 *
 *	Description:  default translation table for class: StaticText
 *	-----------
 *
 *	Matches events with string descriptors for internal routines.
 *
 *************************************<->***********************************/

static char defaultTranslations[] =
	"<EnterWindow>:    enter()\n\
	<LeaveWindow>:     leave()\n\
	<Btn1Down>:        select()\n\
	<Btn1Up>:          release()\n\
	<KeyDown>Select:   select() \n\
	<KeyUp>Select:     release()";


/*************************************<->*************************************
 *
 *
 *	Description:  action list for class: StaticText
 *	-----------
 *
 *	Matches string descriptors with internal routines.
 *      Note that Primitive will register additional event handlers
 *      for traversal.
 *
 *************************************<->***********************************/

static XtActionsRec actionsList[] =
{
	{ "enter",        (XtActionProc) _XwPrimitiveEnter         },
	{ "leave",        (XtActionProc) _XwPrimitiveLeave         },
	{ "select",       (XtActionProc) Select                    },
	{ "release",      (XtActionProc) Release                   },
	{"toggle",        (XtActionProc) Toggle           },
};


/*************************************<->*************************************
 *
 *
 *	Description:  resource list for class: StaticText
 *	-----------
 *
 *	Provides default resource settings for instances of this class.
 *	To get full set of default settings, examine resouce list of super
 *	classes of this class.
 *
 *************************************<->***********************************/

static XtResource resources[] = {
	{ XtNhSpace,
		XtCHSpace,
		XtRDimension,
		sizeof(Dimension),
		XtOffset(XwStaticTextWidget, static_text.internal_width),
		XtRString,
		"2"
	},
	{ XtNvSpace,
		XtCVSpace,
		XtRDimension,
		sizeof(Dimension),
		XtOffset(XwStaticTextWidget, static_text.internal_height),
		XtRString,
		"2"
	},
	{ XtNalignment,
		XtCAlignment,
		XtRAlignment,
		sizeof(XwAlignment),
		XtOffset(XwStaticTextWidget,static_text.alignment),
		XtRString,
		"Left"
	},
	{ XtNgravity,
		XtCGravity,
		XtRGravity,
		sizeof(int),
		XtOffset(XwStaticTextWidget,static_text.gravity),
		XtRString,
		"CenterGravity"
	},
	{ XtNwrap,
		XtCWrap,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(XwStaticTextWidget,static_text.wrap),
		XtRString,
		"TRUE"
	},
	{ XtNstrip,
		XtCStrip,
		XtRBoolean,
		sizeof(Boolean),
		XtOffset(XwStaticTextWidget,static_text.strip),
		XtRString,
		"TRUE"
	},
	{ XtNlineSpace,
		XtCLineSpace,
		XtRInt,
		sizeof(int),
		XtOffset(XwStaticTextWidget,static_text.line_space),
		XtRString,
		"0"
	},
	{ XtNfont,
		XtCFont,
		XtRFontStruct,
		sizeof(XFontStruct *),
		XtOffset(XwStaticTextWidget,static_text.font),
		XtRString,
		"Fixed"
	},
	{ XtNstring,
		XtCString,
		XtRString,
		sizeof(char *),
		XtOffset(XwStaticTextWidget, static_text.input_string),
		XtRString,
		NULL
	},
};



/*************************************<->*************************************
 *
 *
 *	Description:  global class record for instances of class: StaticText
 *	-----------
 *
 *	Defines default field settings for this class record.
 *
 *************************************<->***********************************/

XwStaticTextClassRec XwstatictextClassRec = {
	{ /* core_class fields */
	/* superclass            */	(WidgetClass) &XwprimitiveClassRec,
	/* class_name            */	"XwStaticText",
	/* widget_size           */	sizeof(XwStaticTextRec),
	/* class_initialize      */	ClassInitialize,
	/* class_part_initialize */	NULL,
	/* class_inited          */	FALSE,
	/* initialize            */	(XtInitProc) Initialize,
	/* initialize_hook       */	NULL,
	/* realize               */	(XtRealizeProc) Realize,
	/* actions               */	actionsList,
	/* num_actions           */	XtNumber(actionsList),
	/* resources             */	resources,
	/* num_resources         */	XtNumber(resources),
	/* xrm_class             */	NULLQUARK,
	/* compress_motion       */	TRUE,
	/* compress_exposure     */	TRUE,
	/* compress_enterleave   */	TRUE,
	/* visible_interest      */	FALSE,
	/* destroy               */	(XtWidgetProc) Destroy,
	/* resize                */	(XtWidgetProc) Resize,
	/* expose                */	(XtExposeProc) Redisplay,
	/* set_values            */	(XtSetValuesFunc) SetValues,
	/* set_values_hook       */	NULL,
	/* set_values_almost     */	(XtAlmostProc) XtInheritSetValuesAlmost,
	/* get_values_hook       */	NULL,
	/* accept_focus          */	NULL,
	/* version               */	XtVersion,
	/* callback private      */	NULL,
	/* tm_table              */	defaultTranslations,
	/* query_geometry        */	NULL,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
	},
	{
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
	}
};

WidgetClass XwstatictextWidgetClass = (WidgetClass) &XwstatictextClassRec;
WidgetClass XwstaticTextWidgetClass = (WidgetClass) &XwstatictextClassRec;



/*************************************<->*************************************
 *
 *  ClassInitialize
 *
 *   Description:
 *   -----------
 *    Set fields in primitive class part of our class record so that
 *    the traversal code can invoke our select/release procedures (note
 *    that for this class toggle is a empty proc).
 *
 *************************************<->***********************************/
static void ClassInitialize()
{
   XwstatictextClassRec.primitive_class.select_proc = (XwEventProc) Select;
   XwstatictextClassRec.primitive_class.release_proc = (XwEventProc) Release;
   XwstatictextClassRec.primitive_class.toggle_proc = (XwEventProc) Toggle;
}


/*************************************<->*************************************
 * 
 *	Procedures and variables private to StaticText
 * 
 *************************************<->************************************/

static void ValidateInputs();
static void SetNormalGC();
static int RetCount();
static int Maxline();
static void FormatText();
static void GetTextRect();
static void SetSize();


/*************************************<->*************************************
 *
 *  static void ValidateInputs(stw)
 *		XwStaticTextWidget	stw;
 *
 *	Description:
 *	-----------
 *		Checks a StaticText widget for nonsensical values, and changes 
 *		nonsense values into meaninful values.
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		XtWarning
 *
 *************************************<->***********************************/
static void ValidateInputs(stw)
	XwStaticTextWidget	stw;
{
	XwStaticTextPart	*stp;

	stp = &(stw->static_text);

	/* 
	 *Check line_spacing.  We will allow text to over write,
	 * we will not allow it to move from bottom to top.
	 */
	if (stp->line_space < -100)
	{
		XtWarning("XwStaticTextWidget: line_space was less than -100, set to 0\n");
		stp->line_space = 0;
	}

	/*
	 * Check Alignment
	 */
	
	switch(stp->alignment)
	{
		case XwALIGN_LEFT:
		case XwALIGN_CENTER:
		case XwALIGN_RIGHT:
			/* Valid values. */
		break;
		default:
			XtWarning("XwStaticTextWidget: Unknown alignment, set to Left instead");
			stp->alignment = XwALIGN_LEFT;
		break;
	}

}


/*************************************<->*************************************
 *
 * static void SetNormalGC (stw)
 *	XwStaticTextWidget stw;
 *
 *	Description:
 *	-----------
 *		Sets up a GC for the static text widget to write to.
 *
 *	Inputs:
 *	------
 *		stw = Points to an instance structure which has primitive.foreground
 *	   	   core.background_pixel and static_text.font appropriately
 *	   	   filled in with values.
 *
 *	Outputs:
 *	-------
 *		Associates static_text.normal_GC with a GC.
 *
 *	Procedures Called
 *	-----------------
 *		XCreateGC
 *		XSetClipRectangles
 *
 *************************************<->***********************************/
static void SetNormalGC(stw)
	XwStaticTextWidget	stw;
{
	XGCValues	values;
	XtGCMask	valueMask;
	XRectangle	ClipRect;

	valueMask = GCForeground | GCBackground | GCFont;
    
	values.foreground = stw->primitive.foreground;
	values.background = stw->core.background_pixel;
	values.font	= stw->static_text.font->fid;

	stw->static_text.normal_GC = XCreateGC(XtDisplay(stw),XtWindow(stw),
		valueMask,&values);
	/*
	 * Set the Clip Region.
	 */
	ClipRect.width = stw->core.width - 
		(2 * (stw->static_text.internal_width + 
		stw->primitive.highlight_thickness));
	ClipRect.height = stw->core.height - 
		(2 * (stw->static_text.internal_height +
		stw->primitive.highlight_thickness));
	ClipRect.x = stw->primitive.highlight_thickness +
		stw->static_text.internal_width;
	ClipRect.y = stw->primitive.highlight_thickness +
		stw->static_text.internal_height;

	XSetClipRectangles(XtDisplay(stw),stw->static_text.normal_GC,
		0,0, &ClipRect,1,Unsorted);
}


/*************************************<->*************************************
 *
 *	static int RetCount(string)
 *		char	*string;
 *
 *	Description:
 *	-----------
 *		This routine returns the number of '\n' characters in "string"
 *
 *	Inputs:
 *	------
 *		string = The string to be counted.
 *
 *	Outputs:
 *	-------
 *		The number of '\n' characters in the string.
 *
 *	Procedures Called
 *	-----------------
 *
 *************************************<->***********************************/
static int RetCount(string)
	char	*string;
{
	int numlines;

	numlines = (*string ? 1 : 0);
	while (*string)
	{
		if (*string == '\n')
			numlines++;
		string++;
	}
	return(numlines);
}


/*************************************<->*************************************
 *
 *	static int Maxline(string, font)
 *		char	*string;
 *		XFontStruct	*font;
 *
 *	Description:
 *	-----------
 *		For a given string and a given font, returns the length of
 *		'\n' delimited series of characters.
 *
 *	Inputs:
 *	------
 *		string =  The string to be analyzed.
 *		font   = The font in which string is to be analyzed.
 *
 *	Outputs:
 *	-------
 *		The maximun length in pixels of the longest '\n' delimited series of 
 *		characters.
 *
 *	Procedures Called
 *	-----------------
 *		XTextWidth
 *
 *************************************<->***********************************/
static int Maxline(stw)
	XwStaticTextWidget	stw;
{
	int	i, cur;
	int	max = 0;
	char	*str1, *str2, *str3;
	XwAlignment	alignment = stw->static_text.alignment;
	XFontStruct	*font = stw->static_text.font;
	XwStaticTextPart	*stp;

	stp = &(stw->static_text);
	str1 = stp->output_string;
	while(*str1)
	{
		if ((stp->strip) && 
			((alignment == XwALIGN_LEFT) || 
			(alignment == XwALIGN_CENTER)))
			while (*str1 == ' ')
				str1++;
		str2 = str1;
		for(i=0; ((*str1 != '\n') && *str1); i++, str1++);
		if ((stp->strip) && 
			((alignment == XwALIGN_RIGHT) || 
			(alignment == XwALIGN_CENTER)))
		{
			str3 = str1;
			str3--;
			while (*str3 == ' ')
				i--, str3--;
		}
		if (i)
			cur = XTextWidth(font,str2,i);
		else
			cur = 0;
		if (cur > max)
			max = cur;
		if (*str1)
			str1++; /* Step past the \n */
	}
	return(max);
}


/*************************************<->*************************************
 *
 *	static void FormatText(stw)
 *		XwStaticTextWidget	stw;
 *
 *	Description:
 *	-----------
 *		Inserts newlines where necessary to fit stw->static_text.output_string
 *		into stw->core.width (if specified).
 *
 *	Inputs:
 *	------
 *		stw = The StaticText widget to be formatted.
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		XTextWidth
 *
 *************************************<->***********************************/
static void FormatText(stw)
	XwStaticTextWidget	stw;
{
	int	i, width, win, wordindex;
	char	*str1, *str2;
	Boolean	gotone;
	XwStaticTextPart	*stp;

	stp = &(stw->static_text);
	str1 = stp->output_string;

	/* The available text window width is... */
	win = stw->core.width - (2 * stp->internal_width)
		- (2 * stw->primitive.highlight_thickness);

	while (*str1)
	{
		i = 0;
		width = 0;
		wordindex = -1;
		gotone = FALSE;
		str2 = str1;
		while (!gotone)
		{
			if ((stp->strip) && (wordindex == -1))
				while (*str1 == ' ')
					str1++;
			/*
			 *Step through until a character that we can
			 * break on.
			 */
			while ((*str1 != ' ') && (*str1 != '\n') && (*str1))
				i++, str1++;

			wordindex++;
			width = (i ?  XTextWidth(stp->font,str2,i) : 0);
			width;
			if (width < win)
                        {
				/*
				 * If the current string fragment is shorter than the 
				 * available window.  Check to see if we are at a 
				 * forced break or not, and process accordingly.
				 */
				switch (*str1)
				{
					case ' ':
						/*
						 * Add the space to the char count 
						 */
						i++;
						/*
						 * Check to see there's room to start another
						 * word.
						 */
						width = XTextWidth(stw->static_text.font,
							str2,i);
						if (width >= win)
						{
							/*
							 * Break the line if we can't start
							 * another word.
							 */
							*str1 = '\n';
							gotone = TRUE;
						}
						/*
						 * Step past the space
						 */
						str1++;
					break;
					case '\n':
						/*
						 * Forced break.  Step pase the \n.
						 * Note the fall through to default.
						 */
						str1++;
					default:
						/*
						 * End of string.
						 */
						gotone = TRUE;
					break;
				}
			}
			else if (width > win)
			{
				/*
				 * We know that we have something
				 */
				gotone = TRUE;

				/*
				 * See if there is at least one space to back up for.
				 */
				if (wordindex)
				{
					str1--;
					while (*str1 != ' ')
						str1--;
					*str1++ = '\n';
				}
				else
					/*
					 * We have a single word which is too long
					 * for the available window.  Let the text
					 * clip rectangle handle it.
					 */
					if (*str1)
						*str1++ = '\n';
			}
			else /* (width == win) */
                        {
				switch (*str1)
				{
					case ' ':
						/*
						 * If we stopped on a space, change it.
						 */
						*str1 = '\n';
					case '\n':
						/*
						 * Step past the \n.
						 */
						str1++;
					default:
						gotone = TRUE;
					break;
				}
                        }
		}
	}
}


/*************************************<->*************************************
 *
 *	static void GetTextRect(newstw)
 *		XwStaticTextWidget	newstw;
 *  
 *	Description:
 *	-----------
 *		Sets newstw->static_text.TextRect to appropriate values given 
 *		newstw->core.width and newstw->static_text.input_string.  The
 *		string is formatted if necessary.
 *		
 *		A newstw->core.width value of 0 tells this procedure to choose 
 *		its own size based soley on newstw->static_text.input_string.  
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		Maxline
 *		RetCount
 *
 *************************************<->***********************************/
static void GetTextRect(newstw)
	XwStaticTextWidget	newstw;
{
	int	maxlen, maxwin, fheight, numrets;
	XwStaticTextPart	*stp;

	stp = &(newstw->static_text);
	/*
	 * Set the line height from the font structure.
	 */
	fheight = stp->font->ascent + stp->font->descent;
	if (newstw->core.width)
	{
		/*
		 * We were requested with a specific width.  We must
		 * fit ourselves to it.
		 *
		 * The maximum available window width is...
		 */
		maxwin = newstw->core.width - (2 * stp->internal_width)
			- (2 * newstw->primitive.highlight_thickness);
		if (stp->wrap)
                {
			if ((maxlen = Maxline(newstw)) <= 
				maxwin)
				/*
				 * We fit without formatting.
				 */
				stp->TextRect.width = maxlen;
			else
			{
				stp->TextRect.width = maxwin; 
				/* 
				 * Make the string fit.
				 */
				FormatText(newstw);
			}
                }
		else
			stp->TextRect.width = Maxline(newstw);
	}
	else
		stp->TextRect.width = Maxline(newstw);
	/*
	 * See how tall the string wants to be.
	 */
	numrets = RetCount(stp->output_string);
	stp->TextRect.height = (fheight * numrets) 
		+ (((numrets - 1) * (stp->line_space / 100.0)) * fheight);

	if ((newstw->core.height) &&
		((newstw->core.height - (2 * stp->internal_height)
			- (2 * newstw->primitive.highlight_thickness)) < 
			stp->TextRect.height))
			/*
			 * Shorten the TextRect if the string wants
			 * to be too tall.
			 */
			stp->TextRect.height = newstw->core.height
				- (2 * stp->internal_height)
				- (2 * newstw->primitive.highlight_thickness);
}


/*************************************<->*************************************
 *
 *	static void SetSize(newstw)
 *		XwStaticTextWidget	newstw;
 *
 *	Description:
 *	-----------
 *		Copies newstw->static_text.input_string into output_string
 *
 *		Sets newstw->core.width, newstw->core.height, and
 *		newstw->static_text.TextRect appropriately, formatting the 
 *		string if necessary.
 *
 *		The Clip Rectangle is placed in the window.
 *
 *	Inputs:
 *	------
 *		stw = A meaningful StaticTextWidget.
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		strcpy
 *		GetTextRect
 *
 *************************************<->***********************************/
static void SetSize(newstw)
	XwStaticTextWidget	newstw;
{
	XwStaticTextPart	*stp;

	stp = &(newstw->static_text);
	/*
	 * Copy the input string into the output string.
	 */
	if (stp->output_string != NULL)
		XtFree(stp->output_string);
	stp->output_string = XtMalloc(XwStrlen(stp->input_string)+1);
	if(stp->input_string)
	  strcpy(stp->output_string,stp->input_string);
	if (*(stp->output_string))
		/*
		 * If we have a string then size it.
		 */
		GetTextRect(newstw);
	else
	{
		stp->TextRect.width = 0;
		stp->TextRect.height = 0;
	}
	/*
	 * Has a size been specified?
	 */
	if (newstw->core.width)
        {
		if ((newstw->core.width
			- (2 * newstw->primitive.highlight_thickness)
			- (2 * stp->internal_width)) > stp->TextRect.width)
                {
			/*
			 * Use the extra space according to the gravity
			 * resource setting.
			 */
			switch (stp->gravity)
			{
				case EastGravity:
				case NorthEastGravity:
				case SouthEastGravity:
					stp->TextRect.x = newstw->core.width -
						(newstw->primitive.highlight_thickness + 
						stp->TextRect.width + stp->internal_width);
				break;
				case WestGravity:
				case NorthWestGravity:
				case SouthWestGravity:
					stp->TextRect.x = stp->internal_width + 
						newstw->primitive.highlight_thickness;
				break;
				default:
					stp->TextRect.x = (newstw->core.width
						- stp->TextRect.width) / 2;
				break;
			}
                }
		else
			/*
		 	* We go to the left.
		 	*/
			stp->TextRect.x = newstw->primitive.highlight_thickness
				+ stp->internal_width;
        }
	else
	{
		/*
		 * We go to the left.
		 */
		stp->TextRect.x = newstw->primitive.highlight_thickness
			+ stp->internal_width;
		newstw->core.width = stp->TextRect.width
			+ (2 * stp->internal_width)
			+ (2 * newstw->primitive.highlight_thickness);
	}
	/*
	 * Has a height been specified?
	 */
	if (newstw->core.height)
        {
		if ((newstw->core.height - (2 * stp->internal_height)
			- (2 * newstw->primitive.highlight_thickness)) > 
			stp->TextRect.height)
                {
			/*
			 * Use the extra space according to the gravity
			 * resource setting.
			 */
			switch (stp->gravity)
			{
				case NorthGravity:
				case NorthEastGravity:
				case NorthWestGravity:
					stp->TextRect.y = stp->internal_height +
						newstw->primitive.highlight_thickness;
				break;
				case SouthGravity:
				case SouthEastGravity:
				case SouthWestGravity:
					stp->TextRect.y = newstw->core.height -
						(newstw->primitive.highlight_thickness + 
						stp->TextRect.height + stp->internal_width);
				break;
				default:
					stp->TextRect.y = (newstw->core.height 
						- stp->TextRect.height)/ 2;
				break;
			}
                }
		else
                {
			/*
			 * We go to the top.
			 */
			stp->TextRect.y = newstw->primitive.highlight_thickness
				+ stp->internal_height;
                }
        }
	else
	{
		/*
		 * We go to the top.
		 */
		stp->TextRect.y = newstw->primitive.highlight_thickness
			+ stp->internal_height;
		/*
		 * We add our size to the current size.
		 * (Primitive has already added highlight_thicknesses.)
		 */
		newstw->core.height = stp->TextRect.height
			+ (2 * stp->internal_height)
			+ (2 * newstw->primitive.highlight_thickness);
	}
}


static void ProcessBackslashes(output,input)
	char	*output, *input;
{
	char *out, *in;

	out = output;
	in = input;
	while(*in)
		if (*in == '\\')
		{
			in++;
			switch (*in)
			{
				case 'n':
					*out++ = '\n';
				break;
				case 't':
					*out++ = '\t';
				break;
				case 'b':
					*out++ = '\b';
				break;
				case 'r':
					*out++ = '\r';
				break;
				case 'f':
					*out++ = '\f';
				break;
				default:
					*out++ = '\\';
					*out++ = *in;
				break;
			}
			in++;
		}
		else
			*out++ = *in++;
	*out = '\0';
}

/*************************************<->*************************************
 *
 *  static void Initialize (request, new)
 *  	Widget request, new;
 *  
 *	Description:
 *	-----------
 *		See XToolKit Documentation
 *
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		Xmalloc
 *		ProcessBackslashes
 *		XwStrlen
 *		SetSize
 *
 *************************************<->***********************************/
static void Initialize (req, new)
	XwStaticTextWidget req, new;
{
	char *s;

	new->static_text.output_string = NULL;

	if (new->static_text.input_string != NULL) 
    {

		/*
		 * Copy the input string into local space.
		 */
		s = XtMalloc(XwStrlen(new->static_text.input_string)+1);
		ProcessBackslashes(s,new->static_text.input_string);
		new->static_text.input_string = s;
	}

	ValidateInputs(new);
	new->core.width = req->core.width;
	new->core.height = req->core.height;
	SetSize(new);
}


/*************************************<->*************************************
 *
 *	static Boolean SetValues(current, request, new, last)
 *		XwStaticTextWidget current, request, new;
 *		Boolean last;
 *
 *	Description:
 *	-----------
 *		See XToolKit Documentation
 *
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		ValidateInputs
 *		strcmp
 *		SetSize
 *		SetNormalGC
 *
 *************************************<->***********************************/
static Boolean SetValues(current, request, new, last)
	XwStaticTextWidget current, request, new;
	Boolean last;
{
	Boolean flag = FALSE;
	Boolean	newstring = FALSE;
	Boolean	layoutchanges = FALSE;
	Boolean	otherchanges = FALSE;
	char	*newstr, *curstr;
	char *s;
	XwStaticTextPart	*newstp, *curstp;
	int	new_w, new_h;

	newstp = &(new->static_text);
	curstp = &(current->static_text);
	if (newstp->input_string != curstp->input_string) 
    {
		newstring = TRUE;
		/*
		 * Copy the input string into local space.
		 */
		s = XtMalloc(XwStrlen(newstp->input_string)+1);
		ProcessBackslashes(s,new->static_text.input_string);
		/*
		 * Deallocate the old string.
		 */
		XtFree(curstp->input_string);
		/*
		 * Have everybody point to the new string.
		 */
		newstp->input_string = s;
		curstp->input_string = s;
		request->static_text.input_string = s;
	}

	ValidateInputs(new);

	if ((newstring) ||
		(newstp->font != curstp->font) ||
		(newstp->internal_height != curstp->internal_height) ||
		(newstp->internal_width != curstp->internal_width) ||
		((new->core.width <= 0) || (new->core.height <= 0)) ||
		(request->core.width != current->core.width) ||
		(request->core.height != current->core.height))
	{
		if (request->primitive.recompute_size)
		{
			if (request->core.width == current->core.width)
				new->core.width = 0;
			if (request->core.height == current->core.height)
				new->core.height = 0;
		}

		/*
		 * Call SetSize to get the new size.
		 */
		SetSize(new);

		/*
		 * Save changes that SetSize does to the layout.
		 */
		new_w = new->core.width;
		new_h = new->core.height;
		/*
		 * In case our parent won't let us change size we must
		 * now restore the widget to the current size.
		 */
		new->core.width = current->core.width;
		new->core.height = current->core.height;
		SetSize(new);
		/*
		 * Reload new with the new sizes in order cause XtSetValues
		 * to invoke our parent's geometry management procedure.
		 */
		new->core.width = new_w;
		new->core.height = new_h;

		flag = TRUE;
	}

	if ((new->primitive.foreground != 
			current->primitive.foreground) ||
		(new->core.background_pixel != 
			current->core.background_pixel) ||
		(newstp->font != curstp->font))
	{
		if (XtIsRealized((Widget)new))
		{
			XFreeGC(XtDisplay(new),new->static_text.normal_GC);
			SetNormalGC(new);
		}
		flag = TRUE;
	}
	
	return(flag);
}


/*************************************<->*************************************
 *
 *	static void Realize(w , valueMask, attributes)
 *		Widget              	w;
 *		XtValueMask         	*valueMask;
 *		XSetWindowAttributes	*attributes;
 *
 *	Description:
 *	-----------
 *		See XToolKit Documentation
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		XtCreateWindow
 *		SetNormalGC
 *
 *************************************<->***********************************/
static void Realize(stw , valueMask, attributes)
	XwStaticTextWidget		stw;
	XtValueMask         	*valueMask;
	XSetWindowAttributes	*attributes;
{

	XtCreateWindow((Widget)stw,InputOutput,(Visual *) CopyFromParent,
			*valueMask,attributes);
	SetNormalGC(stw);
	_XwRegisterName(stw);
}


/*************************************<->*************************************
 *
 *	static void Destroy(stw)
 *		XwStaticTextWidget	stw;
 *
 *	Description:
 *	-----------
 *		See XToolKit Documentation
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		XtFree
 *		XFreeGC
 *
 *************************************<->***********************************/
static void Destroy(stw)
	XwStaticTextWidget	stw;
{
	XtFree(stw->static_text.input_string);
	XtFree(stw->static_text.output_string);
	if (XtIsRealized((Widget)stw))
	{
		XFreeGC(XtDisplay(stw),stw->static_text.normal_GC);
	}
}


/*************************************<->*************************************
 *
 *	static void Redisplay(stw)
 *		XwStaticTextWidget	stw;
 *
 *	Description:
 *	-----------
 *		See XToolKit Documentation
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		XTextExtents
 *		XDrawString
 *		XtWarning
 *
 *************************************<->***********************************/
static void Redisplay(stw)
	XwStaticTextWidget	stw;
{
	GC	normal_GC;
	int	i;
	int	cur_x, cur_y;	/* Current start of baseline */
	int	y_delta;	/* Absolute space between baselines */
	int	x_delta;	/* For left bearing of the first char in a line */
	int dir, asc, desc;	/* Junk parameters for XTextExtents */
	char	*str1, *str2, *str3;
	XFontStruct	*font;
	XCharStruct	overall;
	XRectangle	*ClipRect;
	XwStaticTextPart	*stp;

	/*
	 * This is to save typing for me and
	 * pointer arithmetic for the machine.
	 */
	stp = &(stw->static_text);
	str1 = str2 = stp->output_string;
	font = stp->font;
	normal_GC = stp->normal_GC;
	ClipRect = &(stp->TextRect);

	if (XtIsRealized((Widget)stw))
		XClearArea(XtDisplay(stw),XtWindow(stw),
			0,0,stw->core.width,stw->core.height,FALSE);

	/*
	 * We don't get to start drawing at the top of the text box.
	 * See X11 font manual pages.
	 */
	cur_y = ClipRect->y + font->ascent;

	/*
	 * Set up the total line spacing.
	 */
	y_delta = ((stp->line_space / 100.0) + 1) * (font->descent + font->ascent);

	switch (stp->alignment)
	{
		case XwALIGN_LEFT:
			cur_x = ClipRect->x;
			while(*str1)
			{
				/*
				 * Left alignment strips leading blanks.
				 */
				if (stp->strip)
					while(*str1 == ' ')
						str1++;
				str2 = str1;
				/*
				 * Look for a newline and count characters.
				 */
				for(i=0; ((*str1 != '\n') && *str1); i++, str1++);
				/*
				 * Did we get anything?
				 */
				if (i)
				{
					/*
					 * Handle the left bearing of the first char
					 * in this substring.
					 */
					XTextExtents(font,str2,1,&dir,&asc,&desc,
						&overall);
					x_delta = overall.lbearing;
					/*
					 * Write to the window
					 */
					XDrawString(XtDisplay(stw),XtWindow(stw),
						normal_GC,(cur_x + x_delta),cur_y,str2,i);
					x_delta = overall.lbearing = 0;/* I'm paranoid...*/
				}
				/*
				 * Move to the next line.
				 */
				cur_y += y_delta;
				if (*str1)
					str1++; /* Step past the \n */
			}
		break;
		case XwALIGN_CENTER:
			cur_x = ClipRect->x + (ClipRect->width / 2);
			while(*str1)
			{
				/*
				 * Strip leading blanks.
				 */
				if (stp->strip)
					while(*str1 == ' ')
						str1++;
				str2 = str1;
				for(i=0; ((*str1 != '\n') && *str1); i++, str1++);
				/*
				 * Strip trailing blanks.
				 */
				if (stp->strip)
				{
					str3 = str1;
					str3--;
					while (*str3 == ' ')
						i--, str3--;
				}
				if (i)
				{
					/*
					 * Center the substring.
					 */
					x_delta = XTextWidth(font,str2,i) / 2;
					XDrawString(XtDisplay(stw),XtWindow(stw),
						normal_GC,(cur_x - x_delta),cur_y,str2,i);
					x_delta =  0; /* ... still paranoid...*/
				}
				cur_y += y_delta;
				if (*str1)
					str1++; /* Step past the \n */
			}
		break;
		case XwALIGN_RIGHT:
			cur_x = ClipRect->x + ClipRect->width;
			while(*str1)
			{
				str2 = str1;
				for(i=0; ((*str1 != '\n') && *str1); i++, str1++);
				/*
				 * Strip trailing blanks.
				 */
				if (stp->strip)
				{
					str3 = str1;
					str3--;
					while (*str3 == ' ')
						i--, str3--;
				}
				if (i)
				{
					XDrawString(XtDisplay(stw),XtWindow(stw),
						normal_GC,
						(cur_x - XTextWidth(font,str2,i)),
						cur_y,str2,i);
				}
				cur_y += y_delta;
				if (*str1)
					str1++; /* Step past the \n */
			}
		break;
		default:  
			/*
			 * How did we get here? 
			 */
			XtWarning("XwStaticTextWidget: An Unknown Alignment has crept into the code\n");
		break;
	}

  /* 
   * We don't want to lose the highlight on redisplay
   * do we?
   */
    if (stw->primitive.highlighted)
      {
	 _XwHighlightBorder(stw);
	 stw->primitive.display_highlighted = TRUE;
      }
    else
      if (stw->primitive.display_highlighted)
	 {
	    _XwUnhighlightBorder(stw);
	    stw->primitive.display_highlighted = FALSE;
	 }

}

/*************************************<->*************************************
 *
 *	static void Resize(stw)
 *		XwStaticTextWidget	stw;
 *
 *	Description:
 *	-----------
 *		See XToolKit Documentation
 *
 *	Inputs:
 *	------
 *
 *	Outputs:
 *	-------
 *
 *	Procedures Called
 *	-----------------
 *		SetSize
 *		XSetClipRectangles
 *
 *************************************<->***********************************/
static void Resize(stw)
	XwStaticTextWidget	stw;
{
	XRectangle	ClipRect;
	
	/*
	 * Same as at initialization except just look at the new widget.
	 */
	SetSize(stw);

	if (XtIsRealized((Widget)stw))
	{
		/*
		 * Set the Clip Region.
		 */
		ClipRect.width = stw->core.width - 
			(2 * (stw->static_text.internal_width + 
			stw->primitive.highlight_thickness));
		ClipRect.height = stw->core.height - 
			(2 * (stw->static_text.internal_height +
			stw->primitive.highlight_thickness));
		ClipRect.x = stw->primitive.highlight_thickness +
			stw->static_text.internal_width;
		ClipRect.y = stw->primitive.highlight_thickness +
			stw->static_text.internal_height;

		XSetClipRectangles(XtDisplay(stw),stw->static_text.normal_GC,
			0,0, &ClipRect,1,Unsorted);
	}
}




/****************************************************************
 *
 *  Event Routines.
 *
 *       Select - Call the callback when the left button goes down.
 *
 *       Release - Call the callback when the left button goes up.
 *
 ****************************************************************/

static void Select(w,event)
	XwStaticTextWidget w;
	XEvent *event;
{
	XtCallCallbacks((Widget)w,XtNselect,event);
}

static void Release(w,event)
	XwStaticTextWidget w;
	XEvent *event;
{
    XtCallCallbacks((Widget)w,XtNrelease,event);
}

static void Toggle(w,event)
	XwStaticTextWidget w;
	XEvent *event;
{
}
