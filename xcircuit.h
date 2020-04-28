/*----------------------------------------------------------------------*/
/* xcircuit.h 								*/
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University        	*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*      written by Tim Edwards, 8/20/93    				*/
/*----------------------------------------------------------------------*/

#ifndef HAVE_U_CHAR
typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned int		u_int;
typedef unsigned long		u_long;
typedef unsigned long long	u_long_long;
#endif

#ifdef XC_WIN32
#ifdef TCL_WRAPPER
#include "tkwin32.h"
#else
#include "xcwin32.h"
#endif
#endif

#ifdef HAVE_CAIRO
#include <cairo/cairo.h>
#endif

/*----------------------------------------------------------------------*/
/* portable definition to prevent unused variables warning		*/
/*----------------------------------------------------------------------*/

#define UNUSED(x) (void) x

/*----------------------------------------------------------------------*/
/* Graphics functions defined for X11					*/
/*----------------------------------------------------------------------*/

#ifdef HAVE_CAIRO

#define SetForeground(display, gc, fg) xc_cairo_set_color(fg)
typedef cairo_surface_t xcImage;

#else /* !HAVE_CAIRO */

typedef XImage xcImage;
#define DrawLine	XDrawLine
#define DrawLines	XDrawLines
#define DrawPoint	XDrawPoint
#define FillPolygon	XFillPolygon

#define SetForeground(dpy, gc, fg)  XSetForeground(dpy, gc, colorlist[fg].color.pixel)
#define SetBackground(dpy, gc, bg)  XSetBackground(dpy, gc, colorlist[bg].color.pixel)
#define SetThinLineAttributes	XSetLineAttributes
#define SetLineAttributes(a, b, c, d, e, f) \
	 XSetLineAttributes(a, b, ((c) >= 1.55 ? (int)(c + 0.45) : 0), d, e, f)
#define SetDashes	XSetDashes
#define SetFillStyle	XSetFillStyle
#define SetStipple(a,b,c) XSetStipple(a,b,STIPPLE[c])

#endif /* !HAVE_CAIRO */

#define flusharea()	

/*----------------------------------------------------------------------*/
/* Redefinition of fprintf() allows redirection of output to a console	*/
/* in the Tcl interpreter-based version.				*/
/*----------------------------------------------------------------------*/

#ifdef TCL_WRAPPER
  #define Fprintf tcl_printf
  #define Flush tcl_stdflush
#else
  #define Fprintf fprintf
  #define Flush fflush
#endif

#ifdef TCL_WRAPPER
  #define malloc Tcl_Alloc
  /* (see definition of my_calloc in the asg subdirectory) */
  /* #define calloc(a,b) Tcl_Alloc(a * b) */
  #define free(a) Tcl_Free((char *)(a))
  #define realloc(a,b) Tcl_Realloc((char *)(a), b)
  #undef strdup
  #define strdup Tcl_Strdup
  extern char *Tcl_Strdup(const char *);
#endif

/*----------------------------------------------------------------------*/
/* Deal with 32/64 bit processors based on autoconf results.		*/
/*----------------------------------------------------------------------*/

#ifndef SIZEOF_VOID_P
#error "SIZEOF_VOID_P undefined!"
#endif
#ifndef SIZEOF_UNSIGNED_INT
#error "SIZEOF_UNSIGNED_INT undefined!"
#endif
#ifndef SIZEOF_UNSIGNED_LONG
#error "SIZEOF_UNSIGNED_LONG undefined!"
#endif
#ifndef SIZEOF_UNSIGNED_LONG_LONG
#error "SIZEOF_UNSIGNED_LONG_LONG undefined!"
#endif

#if SIZEOF_VOID_P == SIZEOF_UNSIGNED_INT
#define Number(a)		(void *)((u_int) a)
typedef u_int pointertype;
#elif SIZEOF_VOID_P == SIZEOF_UNSIGNED_LONG
#define Number(a)		(void *)((u_long) a)
typedef u_long pointertype;
#elif SIZEOF_VOID_P == SIZEOF_UNSIGNED_LONG_LONG
#define Number(a)		(void *)((u_long_long) a)
typedef u_long_long pointertype;
#else
ERROR: Cannot compile without knowing the size of a pointer.  See xcircuit.h.
#endif

/*----------------------------------------------------------------------*/
/* Basic element types 							*/
/*									*/
/* These values determine how a genericptr should be cast into one of	*/
/* labelptr, polyptr, etc.  To query the element type, use the macros	*/
/* ELEMENTTYPE(), IS_LABEL(), IS_POLYGON(), etc.  These macros mask the	*/
/* bit field.  Note that an element marked with REMOVE_TAG will not be	*/
/* recognized as any valid element, but otherwise, querying the type is	*/
/* transparent to other bit settings (such as NETLIST_INVALID).		*/
/*----------------------------------------------------------------------*/

/* Single-bit flags */
#define	OBJINST		0x01
#define LABEL		0x02
#define POLYGON		0x04
#define ARC		0x08
#define SPLINE  	0x10
#define PATH		0x20
#define GRAPHIC		0x40
#define ARRAY		0x80	/* reserved; unused, for now.		*/

#define REMOVE_TAG	0x100	/* element to be removed from netlist	*/
#define NETLIST_INVALID 0x200	/* this element invalidates the netlist	*/
#define SELECT_HIDE	0x400	/* this element cannot be selected	*/
#define DRAW_HIDE	0x800	/* this element is not drawn.		*/

/* Bit fields */
#define ALL_TYPES 	(OBJINST | LABEL | POLYGON | ARC | SPLINE | PATH \
				| GRAPHIC | ARRAY)
#define VALID_TYPES 	(REMOVE_TAG | ALL_TYPES)

/*----------------------------------------------------------------------*/
/* Definition shortcuts 						*/
/*----------------------------------------------------------------------*/

#define XtnSetArg(a,b)  	XtSetArg(wargs[n], a, b); n++
#define abs(a)			((a) < 0 ? -(a) : (a))
#define sign(a)			((a) <= 0 ? -1 : 1)
#ifndef min
#define max(a,b)		((a) < (b) ? (b) : (a))
#define min(a,b)		((a) < (b) ? (a) : (b))
#endif

/*----------------------------------------------------------------------*/
/* Lengthier define constructs						*/
/*----------------------------------------------------------------------*/

/* Names used for convenience throughout the source code */

#define eventmode	areawin->event_mode
#define topobject	areawin->topinstance->thisobject
#define hierobject	areawin->hierstack->thisinst->thisobject

#define PLIST_INCR(a) \
	(a)->plist = (genericptr *) realloc ((a)->plist, \
	((a)->parts + 1) * sizeof(genericptr))

#define ENDPART		topobject->plist + topobject->parts - 1
#define EDITPART	topobject->plist + *areawin->selectlist

/* Type checks (argument "a" is type genericptr) */

#define ELEMENTTYPE(a)	((a)->type & VALID_TYPES)

#define IS_POLYGON(a)   (ELEMENTTYPE(a) == POLYGON)
#define IS_LABEL(a)     (ELEMENTTYPE(a) == LABEL)
#define IS_OBJINST(a)   (ELEMENTTYPE(a) == OBJINST)
#define IS_ARC(a)       (ELEMENTTYPE(a) == ARC)
#define IS_SPLINE(a)    (ELEMENTTYPE(a) == SPLINE)
#define IS_PATH(a)      (ELEMENTTYPE(a) == PATH)
#define IS_GRAPHIC(a)   (ELEMENTTYPE(a) == GRAPHIC)
#define IS_ARRAY(a)     (ELEMENTTYPE(a) == ARRAY)

/* Conversions from generic to specific types */
/* specifically, from type (genericptr *) to type (polyptr), etc. */

#define TOPOLY(a)	(*((polyptr *)(a)))
#define TOLABEL(a)	(*((labelptr *)(a)))
#define TOOBJINST(a)	(*((objinstptr *)(a)))
#define TOARC(a)	(*((arcptr *)(a)))
#define TOSPLINE(a)	(*((splineptr *)(a))) 
#define TOPATH(a)	(*((pathptr *)(a)))
#define TOGRAPHIC(a)	(*((graphicptr *)(a)))
#define TOGENERIC(a)	(*((genericptr *)(a)))

/* conversions from a selection to a specific type */

#define SELTOGENERICPTR(a) ((areawin->hierstack == NULL) ? \
			(topobject->plist + *(a)) : \
			(hierobject->plist + *(a)))

#define SELTOPOLY(a)	TOPOLY(SELTOGENERICPTR(a))
#define SELTOLABEL(a)	TOLABEL(SELTOGENERICPTR(a))
#define SELTOOBJINST(a)	TOOBJINST(SELTOGENERICPTR(a))
#define SELTOARC(a)	TOARC(SELTOGENERICPTR(a))
#define SELTOSPLINE(a)	TOSPLINE(SELTOGENERICPTR(a))
#define SELTOPATH(a)	TOPATH(SELTOGENERICPTR(a))
#define SELTOGRAPHIC(a)	TOGRAPHIC(SELTOGENERICPTR(a))
#define SELTOGENERIC(a)	TOGENERIC(SELTOGENERICPTR(a))

#define SELECTTYPE(a)   ((SELTOGENERIC(a))->type & ALL_TYPES)
#define SELTOCOLOR(a)	((SELTOGENERIC(a))->color)


/* creation of new elements */

#define NEW_POLY(a,b) \
	PLIST_INCR(b); \
	a = (polyptr *)b->plist + b->parts; \
	*a = (polyptr) malloc(sizeof(polygon)); \
	b->parts++; \
	(*a)->type = POLYGON
#define NEW_LABEL(a,b) \
	PLIST_INCR(b); \
	a = (labelptr *)b->plist + b->parts; \
	*a = (labelptr) malloc(sizeof(label)); \
	b->parts++; \
	(*a)->type = LABEL
#define NEW_OBJINST(a,b) \
	PLIST_INCR(b); \
	a = (objinstptr *)b->plist + b->parts; \
	*a = (objinstptr) malloc(sizeof(objinst)); \
	b->parts++; \
	(*a)->type = OBJINST
#define NEW_ARC(a,b) \
	PLIST_INCR(b); \
	a = (arcptr *)b->plist + b->parts; \
	*a = (arcptr) malloc(sizeof(arc)); \
	b->parts++; \
	(*a)->type = ARC
#define NEW_SPLINE(a,b) \
	PLIST_INCR(b); \
	a = (splineptr *)b->plist + b->parts; \
	*a = (splineptr) malloc(sizeof(spline)); \
	b->parts++; \
	(*a)->type = SPLINE
#define NEW_PATH(a,b) \
	PLIST_INCR(b); \
	a = (pathptr *)b->plist + b->parts; \
	*a = (pathptr) malloc(sizeof(path)); \
	b->parts++; \
	(*a)->type = PATH
#define NEW_GRAPHIC(a,b) \
	PLIST_INCR(b); \
	a = (graphicptr *)b->plist + b->parts; \
	*a = (graphicptr) malloc(sizeof(graphic)); \
	b->parts++; \
	(*a)->type = GRAPHIC

/*----------------------------------------------------------------------*/

typedef struct {
   float x, y;
} XfPoint;

typedef struct {
   long x, y;
} XlPoint;

typedef struct {
   short width, ascent, descent, base;
   int maxwidth;
} TextExtents;

typedef struct {
   float  *padding;	/* Allocated array of padding info per line	*/
   XPoint *tbreak;	/* Position in text to stop			*/
   short  dostop;	/* Location (index) in text to stop 		*/
   short  line;		/* Stop line number, if using dostop or tbreak. */
} TextLinesInfo;

/*----------------------------------------------------------------------*/
/* Implementation-specific definitions 					*/
/*----------------------------------------------------------------------*/

#define LIBS	    5 /* initial number of library pages 		*/
#define PAGES      10 /* default number of top-level pages 		*/
#define SCALEFAC  1.5 /* zoom in/out scaling multiplier 		*/
#define SUBSCALE 0.67 /* ratio of subscript size to normal script size 	*/
#define SBARSIZE   13 /* Pixel size of the scrollbar 			*/
#define RSTEPS     72 /* Number of points defining a circle approx.	*/
#define SPLINESEGS 20 /* Number of points per spline approximation 	*/
#define DELBUFSIZE 10 /* Number of delete events to save for undeleting */
#define MINAUTOSCALE 0.75 /* Won't automatically scale closer than this */
#define MAXCHANGES 20 /* Number of changes to induce a temp file save	*/
#define STIPPLES    8 /* Number of predefined stipple patterns		*/
#define PADSPACE   10 /* Spacing of pinlabels from their origins	*/

#ifdef HAVE_XPM
#define TBBORDER   1  /* border around toolbar buttons */
#endif

#define TOPLEVEL	0
#define SINGLE		1

#define INTSEGS		(SPLINESEGS - 2)
#define LASTSEG		(SPLINESEGS - 3)

#define NOFILENAME	(char *)(-1)

#define FONTHEIGHT(y)	 (y->ascent + y->descent + 6) 
#define ROWHEIGHT	 FONTHEIGHT(appdata.xcfont)
#define FILECHARASCENT   (appdata.filefont->ascent)
#define FILECHARHEIGHT   (FILECHARASCENT + appdata.filefont->descent)
#define LISTHEIGHT	 200
#define TEXTHEIGHT	 28 /* Height of xcircuit vectored font at nominal size */
#define BASELINE	 40 /* Height of baseline */
#define DEFAULTGRIDSPACE 32
#define DEFAULTSNAPSPACE 16

/*----------------------------------------------------------------------*/

#define RADFAC 0.0174532925199  /* (pi / 180) */
#define INVRFAC 57.295779 /* (180 / pi) */

/*----------------------------------------------------------------------*/

#define INCHSCALE 0.375     /* Scale of .25 inches to PostScript units */
#define CMSCALE 0.35433071  /* Scale of .5 cm to PostScript units */
#define IN_CM_CONVERT 28.3464567	/* 72 (in) / 2.54 (cm/in) */

/*----------------------------------------------------------------------*/
/* Event mode definitions (state of drawing area)			*/
/*----------------------------------------------------------------------*/

typedef enum editmode {
  NORMAL_MODE = 0,	/* On the drawing page, none of the situations below */
  UNDO_MODE,		/* In the process of an undo/redo operation */
  MOVE_MODE,		/* In the process of moving elements */
  COPY_MODE,		/* In the process of copying elements */
  PAN_MODE,		/* In the process of panning to follow the cursor */
  SELAREA_MODE,		/* Area selection box */
  RESCALE_MODE,		/* Interactive element rescaling box */
  CATALOG_MODE,		/* On a library page, library directory, or page directory */
  CATTEXT_MODE,		/* Editing an existing object name in the library */
  FONTCAT_MODE,		/* Accessing the font character page from TEXT_MODE */
  EFONTCAT_MODE,	/* Accessing the font character page from ETEXT_MODE */
  TEXT_MODE,		/* Creating a new label */
  WIRE_MODE,		/* Creating a new polygon (wire) */
  BOX_MODE,		/* Creating a new box */
  ARC_MODE,		/* Creating a new arc */
  SPLINE_MODE,		/* Creating a new spline */
  ETEXT_MODE,		/* Editing an exiting label */
  EPOLY_MODE,		/* Editing an existing polygon */
  EARC_MODE,		/* Editing an existing arc */
  ESPLINE_MODE,		/* Editing an existing spline */
  EPATH_MODE,		/* Editing an existing path */
  EINST_MODE,		/* Editing an instance (from the level above) */
  ASSOC_MODE,		/* Choosing an associated schematic or symbol */
  CATMOVE_MODE		/* Moving objects in or between libraries */
} event_mode_t;

/*----------------------------------------------------------------------*/
/* File loading modes							*/
/*----------------------------------------------------------------------*/

enum loadmodes {IMPORT = 1, PSBKGROUND, SCRIPT, RECOVER,
#ifdef ASG
	IMPORTSPICE,
#endif
#ifdef HAVE_CAIRO
	IMPORTGRAPHIC,
#endif
	LOAD_MODES
};

/*----------------------------------------------------------------------*/
/* Text anchoring styles and other parameters (bitmask)			*/
/*----------------------------------------------------------------------*/

#define NOTLEFT		1	/* Center or right anchoring		*/
#define RIGHT		2	/* Right anchoring			*/
#define NOTBOTTOM	4	/* Middle or top anchoring		*/
#define TOP		8	/* Top anchoring			*/
#define FLIPINV		16	/* 1 if text is flip-invariant		*/
#define PINVISIBLE	32	/* 1 if pin visible outside of object	*/
#define PINNOOFFSET	64	/* 0 if pin label offset from position	*/
#define LATEXLABEL	128	/* 1 if label is in LaTeX syntax	*/
#define JUSTIFYRIGHT	256	/* Right text justification		*/
#define JUSTIFYBOTH	512	/* Right and left text justification	*/
#define TEXTCENTERED	1024	/* Centered text			*/

#define RLANCHORFIELD	3	/* right-left anchoring bit field	*/
#define TBANCHORFIELD	12	/* top-bottom anchoring bit field	*/
#define NONANCHORFIELD	2032	/* everything but anchoring fields	*/
#define NONJUSTIFFIELD  255	/* everything but justification fields	*/

/*----------------------------------------------------------------------*/
/* Text string part: types						*/
/*----------------------------------------------------------------------*/

#define TEXT_STRING	 0  /* data is a text string 			*/
#define SUBSCRIPT	 1  /* start subscript; no data			*/
#define SUPERSCRIPT	 2  /* start superscript; no data		*/
#define NORMALSCRIPT	 3  /* stop super-/subscript; no data		*/
#define UNDERLINE	 4  /* start underline; no data			*/
#define OVERLINE	 5  /* start overline; no data			*/
#define NOLINE		 6  /* stop over-/underline; no data		*/
#define TABSTOP	 	 7  /* insert tab stop position			*/
#define TABFORWARD 	 8  /* insert tab stop position			*/
#define TABBACKWARD 	 9  /* insert tab stop position			*/
#define HALFSPACE	10  /* insert half-space; no data		*/
#define QTRSPACE	11  /* insert quarter space; no data		*/
#define RETURN		12  /* carriage-return character; no data	*/
#define FONT_NAME       13  /* inline font designator; data = font name */
#define FONT_SCALE	14  /* font scale change; data = scale		*/
#define FONT_COLOR	15  /* font color change; data = color		*/
#define MARGINSTOP	16  /* declare a width limit for the text	*/
#define KERN		17  /* set new kern values; data = kern x, y	*/
#define PARAM_START	18  /* bounds a parameter; data = param key	*/
#define PARAM_END	19  /* bounds a parameter; no data 		*/

/* Actions translated to keystates (numbering continues from above) */

#define TEXT_RETURN	20
#define TEXT_HOME	21
#define TEXT_END	22
#define TEXT_SPLIT	23
#define TEXT_DOWN	24
#define TEXT_UP		25
#define TEXT_LEFT	26
#define TEXT_RIGHT	27
#define TEXT_DELETE	28
#define TEXT_DEL_PARAM	29

#define SPECIAL		63  /* used only when called from menu		*/
#define NULL_TYPE	255 /* used as a placeholder			*/

/*----------------------------------------------------------------------*/
/* Reset modes								*/
/*----------------------------------------------------------------------*/

#define SAVE		1
#define DESTROY		2

/*----------------------------------------------------------------------*/
/* Coordinate display types						*/
/*----------------------------------------------------------------------*/

#define DEC_INCH	0
#define FRAC_INCH	1
#define CM		2
#define INTERNAL	3

/*----------------------------------------------------------------------*/
/* Library types							*/
/*----------------------------------------------------------------------*/

#define FONTENCODING   -1  /* Used only by libopen() */
#define FONTLIB		0
#define PAGELIB		1
#define LIBLIB		2
#define LIBRARY		3
#define USERLIB		(xobjs.numlibs + LIBRARY - 1)

/*----------------------------------------------------------------------*/
/* Object instance styles						*/
/*----------------------------------------------------------------------*/

#define LINE_INVARIANT 1	/* Linewidth is invariant w.r.t. scale	*/
#define INST_NONETLIST 2	/* Instance is not netlistable		*/

/*----------------------------------------------------------------------*/
/* Box styles								*/
/*----------------------------------------------------------------------*/

#define NORMAL 0
#define UNCLOSED 1
#define DASHED 2
#define DOTTED 4
#define NOBORDER 8
#define FILLED 16
#define STIP0 32
#define STIP1 64
#define STIP2 128
#define FILLSOLID 224  /* = 32 + 64 + 128 */
#ifdef OPAQUE
#undef OPAQUE
#endif
#define OPAQUE 256
#define BBOX 512
#define SQUARECAP 1024
#define CLIPMASK  2048
#define FIXEDBBOX 4096

/*----------------------------------------------------------------------*/
/* Box edit styles							*/
/*----------------------------------------------------------------------*/

#define NONE  0
#define MANHATTAN 1
#define RHOMBOIDX 2
#define RHOMBOIDY 4
#define RHOMBOIDA 8

/*----------------------------------------------------------------------*/
/* Path edit styles							*/
/*----------------------------------------------------------------------*/

#define TANGENTS  1		/* (NORMAL = 0) */

/*----------------------------------------------------------------------*/
/* Cycle points (selected points) (flag bits, can be OR'd together)	*/
/*----------------------------------------------------------------------*/

#define EDITX 	  0x01
#define EDITY	  0x02
#define LASTENTRY 0x04
#define PROCESS	  0x08
#define REFERENCE 0x10
#define ANTIXY	  0x20		/* For matched-tangent curves */

/*----------------------------------------------------------------------*/
/* Arc creation and edit styles						*/
/*----------------------------------------------------------------------*/

#define CENTER 1
#define RADIAL 2

/*----------------------------------------------------------------------*/
/* Delete/undelete draw-mode styles					*/
/*----------------------------------------------------------------------*/

#define ERASE 1
#define DRAW  1

/*----------------------------------------------------------------------*/
/* Schematic object types and pin label types	   			*/
/*----------------------------------------------------------------------*/

#define PRIMARY 0		/* Primary (master) schematic page */
#define SECONDARY 1		/* Secondary (slave) schematic page */
#define TRIVIAL 2		/* Symbol as non-schematic element */
#define SYMBOL 3		/* Symbol associated with a schematic */
#define FUNDAMENTAL 4		/* Standalone symbol */
#define NONETWORK 5		/* Do not netlist this object */
#define GLYPH 6			/* Symbol is a font glyph */

#define LOCAL 1
#define GLOBAL 2
#define INFO 3

#define HIERARCHY_LIMIT 256	/* Stop if recursion goes this deep */

/*----------------------------------------------------------------------*/
/* Save types.	This list incorporates "ALL_PAGES", below.		*/
/*----------------------------------------------------------------------*/

#define CURRENT_PAGE	0	/* Current page + all associated pages	*/
#define NO_SUBCIRCUITS	1	/* Current page w/o subcircuit pages	*/

/*----------------------------------------------------------------------*/
/* Modes used when ennumerating page totals.				*/
/*----------------------------------------------------------------------*/

#define INDEPENDENT	0
#define DEPENDENT	1
#define TOTAL_PAGES	2
#define LINKED_PAGES	3
#define PAGE_DEPEND	4
#define ALL_PAGES	5

/*----------------------------------------------------------------------*/
/* Color scheme styles (other than NORMAL)				*/
/*----------------------------------------------------------------------*/

#define INVERSE		1

/*----------------------------------------------------------------------*/
/* Cursor definitions 							*/
/*----------------------------------------------------------------------*/

#define NUM_CURSORS	11

#define ARROW 		appcursors[0]
#define CROSS 		appcursors[1]
#define SCISSORS 	appcursors[2]
#define COPYCURSOR	appcursors[3]
#define ROTATECURSOR    appcursors[4]
#define EDCURSOR	appcursors[5]
#define TEXTPTR 	appcursors[6]
#define CIRCLE		appcursors[7]
#define QUESTION	appcursors[8]
#define WAITFOR		appcursors[9]
#define HAND		appcursors[10]

#define DEFAULTCURSOR	(*areawin->defaultcursor)

/*----------------------------------------------------------------------*/
/* integer and floating-point coordinate list structures		*/
/*----------------------------------------------------------------------*/

typedef XPoint* pointlist;
typedef XfPoint* fpointlist;

/*----------------------------------------------------------------------*/
/* Allowed parameterization types					*/
/*----------------------------------------------------------------------*/

enum paramwhich {
   P_NUMERIC = 0,	/* uncommitted numeric parameter */
   P_SUBSTRING,
   P_POSITION_X,
   P_POSITION_Y,
   P_STYLE,
   P_ANCHOR,
   P_ANGLE1,
   P_ANGLE2,
   P_RADIUS,
   P_MINOR_AXIS,
   P_ROTATION,
   P_SCALE,
   P_LINEWIDTH,
   P_COLOR,
   P_EXPRESSION,
   P_POSITION, 		/* mode only, not a real parameter */
   NUM_PARAM_TYPES
};

/*----------------------------------------------------------------------*/
/* Labels are constructed of strings and executables 			*/
/*----------------------------------------------------------------------*/

typedef struct _stringpart *stringptr;

typedef struct _stringpart {
   stringptr	nextpart;
   u_char	type;
   union {
     u_char	*string;
     int	color;
     int	font;
     int	width;
     int	flags;
     float	scale;
     short	kern[2];
   }		data;
} stringpart;

/*----------------------------------------------------------------------*/
/* structures of all main elements which can be displayed & manipulated	*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Object & object instance parameter structure				*/
/* (Add types as necessary)						*/
/*----------------------------------------------------------------------*/

enum paramtypes {XC_INT = 0, XC_FLOAT, XC_STRING, XC_EXPR};

/* Object parameters: general key:value parameter model */
/* Note that this really should be a hash table, not a linked list. . . */

typedef struct _oparam *oparamptr;

typedef struct _oparam {
   char *	key;		/* name of the parameter */
   u_char	type;		/* type is from paramtypes list above */
   u_char	which;		/* what the parameter represents (P_*) */
   union {
      stringpart *string;	/* xcircuit label type */
      char	 *expr;		/* TCL (string) expression */
      int	  ivalue;	/* also covers type short int by typecasting */
      float	  fvalue;
   } parameter;			/* default or substitution value */
   oparamptr	next;		/* next parameter in linked list */
} oparam;

/* Element parameters: reference back to the object's parameters */
/* These parameters are forward-substituted when descending into */
/* an object instance.						 */
/* Note that this really should be a hash table, not a linked list. . . */

typedef struct _eparam *eparamptr;

typedef struct _eparam {
   char *	key;	    /* name of the parameter */
   u_char	flags;	    /* namely, bit declaring an indirect parameter */
   union {
      int	pointno;    /* point number in point array, for polygons */
      short	pathpt[2];  /* element number and point number, for paths */
      char	*refkey;    /* parameter reference key, for instances */
   } pdata;
   eparamptr	next;	    /* next parameter in linked list */
} eparam;

#define P_INDIRECT	0x01	/* indirect parameter indicator */

/*----------------------------------------------------------------------*/
/* Generic element type	is a superset of all elements.			*/	
/*----------------------------------------------------------------------*/

typedef struct {
   u_short	type;		/* type is LABEL, POLYGON, etc., from below */
   int		color;
   eparamptr	passed;
} generic, *genericptr;		/* (convenience function for retypecasting) */

/*----------------------------------------------------------------------*/
/* selection-mechanism structures					*/
/*----------------------------------------------------------------------*/

typedef struct {
   short number;
   genericptr *element;
   short *idx;
} uselection;

typedef struct {
   short number;
   u_char flags;
} pointselect;

/*----------------------------------------------------------------------*/
/* Bounding box								*/
/*----------------------------------------------------------------------*/

typedef struct {
   XPoint	lowerleft;
   Dimension	width, height;
} BBox;

/*----------------------------------------------------------------------*/
/* Object instance type							*/
/*----------------------------------------------------------------------*/

typedef struct _xcobject *objectptr;

typedef struct {
   u_short	type;
   int		color;
   eparamptr	passed;		/* numerical parameters passed from above */
   u_short	style;
   XPoint	position;
   float	rotation;
   float	scale;
   objectptr	thisobject;
   oparamptr	params;		/* parameter substitutions for this instance */
   BBox		bbox;		/* per-instance bounding box information */
   BBox		*schembbox;	/* Extra bounding box for pin labels */
} objinst, *objinstptr;

/* Linked-list for objects */

typedef struct _objlist *objlistptr;

typedef struct _objlist {
   int		libno;		/* library in which object appears */
   objectptr	thisobject;	/* pointer to the object */
   objlistptr	next;
} objlist;

/* Linked-list for object instances */

typedef struct _pushlist *pushlistptr;

typedef struct _pushlist {
   objinstptr	thisinst;
   char		*clientdata;	/* general-purpose record */
   pushlistptr	next;
} pushlist;

/* Same as above, but for the list of instances in a library, so it	*/
/* needs an additional record showing whether or not the instance is	*/
/* "virtual" (not the primary library instance of the object)		*/

typedef struct _liblist *liblistptr;

typedef struct _liblist {
   objinstptr	thisinst;
   u_char	virtual;
   liblistptr	next;
} liblist;

/*----------------------------------------------------------------------*/
/* Generalized graphic object (Tcl/Tk only)				*/
/*----------------------------------------------------------------------*/

typedef struct {
   u_short	  type;
   int		  color;	/* foreground, for bitmaps only */
   eparamptr	  passed;	/* numerical parameters passed from above */
   XPoint	  position;
   float	  rotation;
   float	  scale;
   xcImage	  *source;	/* source data */
#ifndef HAVE_CAIRO
   xcImage	  *target;	/* target (scaled) data */
   float	  trot;		/* target rotation */
   float	  tscale;	/* target scale (0 = uninitialized) */
   Pixmap	  clipmask;	/* clipmask for non-manhattan rotations */
   Boolean	  valid;	/* does target need to be regenerated? */
#endif /* !HAVE_CAIRO */
} graphic, *graphicptr;

/*----------------------------------------------------------------------*/
/* Label								*/
/*----------------------------------------------------------------------*/

typedef struct {
   u_short	type;
   int		color;
   eparamptr	passed;		/* numerical parameters passed from above */
   pointselect	*cycle;		/* Edit position(s), or NULL */
   XPoint	position;
   float	rotation;
   float	scale;
   u_short	anchor;
   u_char	pin;
   stringpart	*string;
} label, *labelptr;

/*----------------------------------------------------------------------*/
/* Polygon								*/
/*----------------------------------------------------------------------*/

typedef struct {
   u_short	type;
   int		color;
   eparamptr	passed;		/* numerical parameters passed from above */
   u_short	style;
   float	width;
   pointselect	*cycle;		/* Edit position(s), or NULL */
   short	number;
   pointlist	points;
} polygon, *polyptr;

/*----------------------------------------------------------------------*/
/* Bezier Curve								*/
/*----------------------------------------------------------------------*/

typedef struct {
   u_short	type;
   int		color;
   eparamptr	passed;		/* numerical parameters passed from above */
   u_short	style;
   float	width;
   pointselect	*cycle;		/* Edit position(s), or NULL */
   XPoint	ctrl[4];
   /* the following are for rendering only */
   XfPoint	points[INTSEGS];
} spline, *splineptr;

/*----------------------------------------------------------------------*/
/* Arc									*/
/*----------------------------------------------------------------------*/

typedef struct {
   u_short	type;
   int		color;
   eparamptr	passed;		/* numerical parameters passed from above */
   u_short	style;
   float	width;
   pointselect	*cycle;		/* Edit position(s), or NULL */
   short	radius; 	/* x-axis radius */
   short	yaxis;		/* y-axis radius */
   float	angle1;		/* endpoint angles, in degrees */
   float	angle2;
   XPoint	position;
   /* the following are for rendering only */
   short	number;
   XfPoint	points[RSTEPS + 1];
} arc, *arcptr;

/*----------------------------------------------------------------------*/
/* Path									*/
/*----------------------------------------------------------------------*/

typedef struct {
   u_short	type;
   int		color;
   eparamptr	passed;		/* numerical parameters passed from above */
   u_short	style;
   float	width;
   short	parts;
   genericptr	*plist;		/* to be retypecast to polygon, arc, or spline */
} path, *pathptr;

/*----------------------------------------------------------------------*/
/* selection from top-level object 					*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Undo mechanism definitions						*/
/*----------------------------------------------------------------------*/

enum UNDO_MODES {UNDO_DONE = 0, UNDO_MORE, MODE_CONNECT, MODE_RECURSE_WIDE,
	MODE_RECURSE_NARROW};

typedef struct _selection *selectionptr;

typedef struct _selection {
   int selects;
   short *selectlist;
   objinstptr thisinst;
   selectionptr next;
} selection;

#define select_element(a) recurse_select_element(a, UNDO_MORE) 
#define select_add_element(a) recurse_select_element(a, UNDO_DONE) 

#define easydraw(a, b)	 geneasydraw(a, b, topobject, areawin->topinstance)

/*----------------------------------------------------------------------*/
/* Netlist structures for schematic capture				*/
/*----------------------------------------------------------------------*/

/* Structure to hold net and subnet IDs for a bus */

typedef struct {
   int netid;
   int subnetid;
} buslist;

/* Structure mimicking the top part of a Polylist or Labellist	*/
/* when we just want the netlist information and don't care	*/
/* which one we're looking at.					*/

typedef struct {
   union {
      int id;
      buslist *list;
   } net;
   int subnets;
} Genericlist; 

/* Linked polygon list */

typedef struct _Polylist *PolylistPtr;
typedef struct _Polylist
{
   union {
      int  id;		/* A single net ID, if subnets == 0	*/
      buslist *list;	/* List of net and subnet IDs for a bus	*/
   } net;
   int subnets;		/* Number of subnets; 0 if no subnets	*/
   objectptr cschem;	/* Schematic containing the polygon	*/
   polyptr poly;
   PolylistPtr next;	/* Next polygon in the linked list	*/
} Polylist;

/* Linked label list */

typedef struct _Labellist *LabellistPtr;
typedef struct _Labellist
{
   union {
      int  id;		/* A single net ID, if subnets == 0	*/
      buslist *list;	/* List of net and subnet IDs for a bus	*/
   } net;
   int subnets;		/* Number of subnets; 0 if no subnets	*/
   objectptr cschem;	/* Schematic containing the label	*/
   objinstptr cinst;	/* Specific label instance, if applicable */
   labelptr label;
   LabellistPtr next;
} Labellist;

/* List of object's networks by (flattened) name */

typedef struct _Netname *NetnamePtr;
typedef struct _Netname
{
   int netid;
   stringpart *localpin;
   NetnamePtr next;
} Netname;

/* List of object's I/O ports */

typedef struct _Portlist *PortlistPtr;
typedef struct _Portlist
{
   int portid;
   int netid;
   PortlistPtr next;
} Portlist;

/* List of calls to instances of objects */
/* or subcircuit objects.		 */

typedef struct _Calllist *CalllistPtr;
typedef struct _Calllist
{
   objectptr   cschem;		/* Schematic containing the instance called */
   objinstptr  callinst;	/* Instance called */
   objectptr   callobj;		/* Object of instance called */
   char	       *devname;	/* if non-null, name of device in netlist */
   int         devindex;	/* if non-negative, index of device in netlist */
   PortlistPtr ports;
   CalllistPtr next;
} Calllist;

/* PCB netlist structures */

struct Pnet {
   int numnets;
   int *netidx;
   struct Pnet *next;
};

struct Pstr {
   stringpart *string;
   struct Pstr *next;
};

struct Ptab {
   objectptr cschem;
   struct Pnet *nets;
   struct Pstr *pins;
   struct Ptab *next;
};

/*----------------------------------------------------------------------*/
/* Information needed to highlight a net				*/
/*----------------------------------------------------------------------*/

typedef struct {
   Genericlist	*netlist;
   objinstptr	thisinst;
} Highlight;

/*----------------------------------------------------------------------*/
/* Main object structure						*/
/*----------------------------------------------------------------------*/

typedef struct _xcobject {
   char		name[80];
   u_short	changes;	/* Number of unsaved changes to object */
   Boolean	hidden;
   float	viewscale;
   XPoint	pcorner;	/* position relative to window */
   BBox		bbox;		/* bounding box information (excluding */
				/* parameterized elements) */
   short	parts;
   genericptr	*plist;	 	/* to be retypecast to label, polygon, etc. */
   oparamptr	params;		/* list of parameters, with default values */
   
   Highlight	highlight;	/* net to be highlighted on redraw */
   u_char	schemtype;
   objectptr	symschem;	/* schematic page support */
   Boolean	valid;		/* Is current netlist valid? */
   Boolean	traversed;	/* Flag to indicate object was processed */
   LabellistPtr	labels;		/* Netlist pins	 */
   PolylistPtr	polygons;	/* Netlist wires */
   PortlistPtr  ports;		/* Netlist ports */
   CalllistPtr  calls;		/* Netlist subcircuits and connections */
   Boolean	infolabels;	/* TRUE if object contains info-labels */
   NetnamePtr   netnames;	/* Local names for flattening */
				/* (this probably shouldn't be here. . .) */
} object;

/*----------------------------------------------------------------------*/
/* Transformation matrices						*/
/* Works like this (see also PostScript reference manual):		*/
/*   [x' y' 1] = [x y 1] * | a  d  0 |					*/
/*			   | b  e  0 |					*/
/*			   | c  f  1 |					*/
/*----------------------------------------------------------------------*/

typedef struct _matrix *Matrixptr;

typedef struct _matrix {
   float a, b, c, d, e, f;
   Matrixptr nextmatrix;
} Matrix;

/*----------------------------------------------------------------------*/
/* Some convenience functions for matrix manipulation			*/
/*----------------------------------------------------------------------*/

#define DCTM  areawin->MatStack

/*----------------------------------------------------------------------*/
/* button tap/press definitions 					*/
/*----------------------------------------------------------------------*/

#define PRESSTIME	200	/* milliseconds of push to be a "press" */

/*----------------------------------------------------------------------*/
/* object name alias structures						*/
/*----------------------------------------------------------------------*/

typedef struct _stringlist *slistptr;

typedef struct _stringlist {
   char *alias;
   slistptr next;
} stringlist;

typedef struct _alias *aliasptr;

typedef struct _alias {
   objectptr baseobj;       /* pointer to object (actual name) */
   slistptr aliases;    /* linked list of alias names */
   aliasptr next;
} alias;    

/*----------------------------------------------------------------------*/
/* To facilitate compiling the Tcl/Tk version, we make some convenient	*/
/* definitions for types "xc..." and "Xc..." which can be mapped both	*/
/* to X and Xt or to Tk functions and types.				*/
/*----------------------------------------------------------------------*/

#ifdef TCL_WRAPPER

#define xcWidget 			Tk_Window
#define xcWidgetList 			Tk_Window *
#define xcAddEventHandler(a,b,c,d,e) 	Tk_CreateEventHandler(a,b,d,e)
#define xcRemoveEventHandler(a,b,c,d,e) Tk_DeleteEventHandler(a,b,d,e)
#define xcEventHandler			Tk_EventProc *
#define xcWindow			Tk_WindowId
#define xcIsRealized			Tk_IsMapped  /* not sure this is right */
#define xcScreen			Tk_Screen
#define xcParent			Tk_Parent
#define xcDispatchEvent(a)		Tk_HandleEvent(a)
#define xcAddTimeOut(a, b, c, d)	Tcl_CreateTimerHandler((int)(b), c, d)
#define xcRemoveTimeOut			Tcl_DeleteTimerHandler
#define xcTimeOutProc			Tcl_TimerProc *
#define xcIntervalId			Tcl_TimerToken

#else

#define xcWidget 			Widget
#define xcWidgetList 			WidgetList
#define xcAddEventHandler 		XtAddEventHandler
#define xcRemoveEventHandler 		XtRemoveEventHandler
#define xcEventHandler			XtEventHandler
#define xcWindow			XtWindow
#define xcIsRealized			XtIsRealized
#define xcScreen			XtScreen
#define xcParent			XtParent
#define xcDispatchEvent(a)		XtDispatchEvent(a)
#define xcAddTimeOut			XtAppAddTimeOut
#define xcRemoveTimeOut			XtRemoveTimeOut
#define xcTimeOutProc			XtTimerCallbackProc
#define xcIntervalId			XtIntervalId
#ifndef ClientData
   #define ClientData			XtPointer
#endif
#endif

/*----------------------------------------------------------------------*/
/* structures for managing the popup prompt widgets 			*/
/*----------------------------------------------------------------------*/

typedef struct {
   xcWidget	button;
   int		foreground;
   void		(*buttoncall)();
   void		*dataptr;
} buttonsave;

typedef struct {
   xcWidget	popup;		/* Popup widget		*/
   xcWidget	textw;		/* Text entry widget	*/
   xcWidget	filew;		/* File list window 	*/
   xcWidget	scroll;		/* Scrollbar widget	*/
   void		(*setvalue)();	/* Callback function	*/
   buttonsave	*buttonptr;	/* Button widget calling popup	     */
   char		*filter;	/* Extension filter for highlighting */
				/*  files. NULL for no file window.  */
				/*  NULL-string for no highlights.   */
} popupstruct;

typedef struct {
   xcWidget	textw;
   xcWidget	buttonw;
   void		(*setvalue)();
   void		*dataptr;
} propstruct;

typedef struct {
   char		*filename;
   int		filetype;
} fileliststruct;

enum {DIRECTORY = 0, MATCH, NONMATCH};	/* file types */

/*----------------------------------------------------------------------*/
/* Initial Resource Management						*/
/*----------------------------------------------------------------------*/

typedef struct {
   /* schematic layout colors */
   Pixel	globalcolor, localcolor, infocolor, ratsnestcolor;

   /* non-schematic layout color(s) */
   Pixel	fixedbboxpix, bboxpix, clipcolor;

   /* color scheme 1 */
   Pixel	fg, bg;
   Pixel	gridpix, snappix, selectpix, axespix;
   Pixel	buttonpix, filterpix, auxpix, barpix, parampix;

   /* color scheme 2 */
   Pixel	fg2, bg2;
   Pixel	gridpix2, snappix2, selectpix2, axespix2;
   Pixel	buttonpix2, auxpix2, parampix2;

   int		width, height, timeout;
   XFontStruct	*filefont;

#ifndef TCL_WRAPPER
   XFontStruct	*xcfont, *textfont, *titlefont, *helpfont;
#endif

} ApplicationData, *ApplicationDataPtr;

/*----------------------------------------------------------------------*/
/* Macros for GC color and function handling				*/
/*----------------------------------------------------------------------*/

#define XTopSetForeground(a) if (a == DEFAULTCOLOR) SetForeground(dpy, \
	areawin->gc, FOREGROUND); else SetForeground(dpy, areawin->gc, a)

#define XcTopSetForeground(z) XTopSetForeground(z); areawin->gccolor = \
	((z) == DEFAULTCOLOR) ? FOREGROUND : (z)

#define XcSetForeground(z) SetForeground(dpy, areawin->gc, z); \
	areawin->gccolor = z

/*----------------------------------------------------------------------*/
/* Structure for maintaining list of colors				*/
/*----------------------------------------------------------------------*/

typedef struct {
   xcWidget	cbutton;
   XColor	color;
} colorindex;

/*----------------------------------------------------------------------*/
/* Font information structure						*/
/*----------------------------------------------------------------------*/
/* Flags:   bit  description						*/
/* 	    0    bold = 1,  normal = 0					*/
/*	    1    italic = 1, normal = 0					*/
/*	    2    <reserved, possibly for narrow font type>		*/
/*	    3    drawn = 1,  PostScript = 0				*/
/* 	    4	 <reserved, possibly for LaTeX font type>		*/
/*	    5    special encoding = 1, Standard Encoding = 0		*/
/*	    6	 ISOLatin1 = 2, ISOLatin2 = 3				*/
/*	    7+   <reserved for other encoding schemes>			*/
/*----------------------------------------------------------------------*/

typedef struct {
   char *psname;
   char *family;
   float scale;
   u_short flags;
   objectptr *encoding;
#ifdef HAVE_CAIRO
   const char **utf8encoding;
   cairo_font_face_t *font_face;
   unsigned long glyph_index[256];
   double glyph_top[256];
   double glyph_bottom[256];
   double glyph_advance[256];
#endif /* HAVE_CAIRO */
} fontinfo;

/*----------------------------------------------------------------------*/

typedef struct {
   char *name;
   BBox bbox;
} psbkground;

/*----------------------------------------------------------------------*/
/* Key macro information						*/
/*----------------------------------------------------------------------*/

typedef struct _keybinding *keybindingptr;

typedef struct _keybinding {
   xcWidget window;		/* per-window function, or NULL */
   int keywstate;
   int function;
   short value;
   keybindingptr nextbinding;
} keybinding;

/*----------------------------------------------------------------------*/
/* Enumeration of functions available for binding to keys/buttons	*/
/* IMPORTANT!  Do not alter this list without also fixing the text	*/
/* in keybindings.c!							*/
/*----------------------------------------------------------------------*/

enum {
   XCF_ENDDATA = -2,		XCF_SPACER  /* -1 */,
   XCF_Page	 /* 0 */,	XCF_Anchor /* 1 */,
   XCF_Superscript /* 2 */,	XCF_Subscript /* 3 */,
   XCF_Normalscript /* 4 */,	XCF_Font /* 5 */,
   XCF_Boldfont /* 6 */,	XCF_Italicfont /* 7 */,
   XCF_Normalfont /* 8 */,	XCF_Underline /* 9 */,
   XCF_Overline /* 10 */,	XCF_ISO_Encoding /* 11 */,
   XCF_Halfspace /* 12 */,	XCF_Quarterspace /* 13 */,
   XCF_Special /* 14 */,	XCF_TabStop /* 15 */,
   XCF_TabForward /* 16 */,	XCF_TabBackward /* 17 */,
   XCF_Text_Return /* 18 */,	XCF_Text_Delete /* 19 */,
   XCF_Text_Right /* 20 */,	XCF_Text_Left /* 21 */,
   XCF_Text_Up /* 22 */,	XCF_Text_Down /* 23 */,
   XCF_Text_Split /* 24 */,	XCF_Text_Home /* 25 */,
   XCF_Text_End /* 26 */,	XCF_Linebreak /* 27 */,
   XCF_Parameter /* 28 */,	XCF_Edit_Param /* 29 */,
   XCF_ChangeStyle /* 30 */,	XCF_Edit_Delete /* 31 */,
   XCF_Edit_Insert /* 32 */,	XCF_Edit_Append /* 33 */,
   XCF_Edit_Next /* 34 */,	XCF_Attach /* 35 */,
   XCF_Next_Library /* 36 */,	XCF_Library_Directory /* 37 */,
   XCF_Library_Move /* 38 */,	XCF_Library_Copy /* 39 */,
   XCF_Library_Edit /* 40 */,	XCF_Library_Delete /* 41 */,
   XCF_Library_Duplicate /* 42 */, XCF_Library_Hide /* 43 */,
   XCF_Library_Virtual /* 44 */, XCF_Page_Directory /* 45 */,
   XCF_Library_Pop /* 46 */,	XCF_Virtual /* 47 */,
   XCF_Help /* 48 */,		XCF_Redraw /* 49 */,
   XCF_View /* 50 */,		XCF_Zoom_In /* 51 */,
   XCF_Zoom_Out /* 52 */,	XCF_Pan /* 53 */,
   XCF_Double_Snap /* 54 */,	XCF_Halve_Snap /* 55 */,
   XCF_Write /* 56 */,		XCF_Rotate /* 57 */,
   XCF_Flip_X /* 58 */,		XCF_Flip_Y /* 59 */,
   XCF_Snap /* 60 */,		XCF_SnapTo /* 61 */,
   XCF_Pop /* 62 */,		XCF_Push /* 63 */,
   XCF_Delete /* 64 */,		XCF_Select /* 65 */,
   XCF_Box /* 66 */,		XCF_Arc /* 67 */,
   XCF_Text /* 68 */,		XCF_Exchange /* 69 */,
   XCF_Copy /* 70 */,		XCF_Move /* 71 */,
   XCF_Join /* 72 */,		XCF_Unjoin /* 73 */,
   XCF_Spline /* 74 */,		XCF_Edit /* 75 */,
   XCF_Undo /* 76 */,		XCF_Redo /* 77 */,
   XCF_Select_Save /* 78 */,	XCF_Unselect /* 79 */,
   XCF_Dashed /* 80 */,		XCF_Dotted /* 81 */,
   XCF_Solid /* 82 */,		XCF_Prompt /* 83 */,
   XCF_Dot /* 84 */,		XCF_Wire /* 85 */,
   XCF_Cancel /* 86 */,		XCF_Nothing /* 87 */,
   XCF_Exit /* 88 */,		XCF_Netlist /* 89 */,
   XCF_Swap /* 90 */,		XCF_Pin_Label /* 91 */,
   XCF_Pin_Global /* 92 */,	XCF_Info_Label /* 93 */,
   XCF_Graphic	/* 94 */,	XCF_SelectBox /* 95 */,
   XCF_Connectivity /* 96 */,	XCF_Continue_Element /* 97 */,
   XCF_Finish_Element /* 98 */, XCF_Continue_Copy /* 99 */,
   XCF_Finish_Copy /* 100 */,	XCF_Finish /* 101 */,
   XCF_Cancel_Last /* 102 */,	XCF_Sim /* 103 */,
   XCF_SPICE /* 104 */,		XCF_PCB /* 105 */,
   XCF_SPICEflat /* 106 */,	XCF_Rescale /* 107 */,		
   XCF_Reorder /* 108 */,	XCF_Color /* 109 */,
   XCF_Margin_Stop /* 110 */,	XCF_Text_Delete_Param /* 111 */,
   NUM_FUNCTIONS
};

/*----------------------------------------------------------------------*/
/* Per-drawing-page parameters						*/
/*----------------------------------------------------------------------*/

typedef struct {
   /* per-drawing-page parameters */
   objinstptr	pageinst;
   char		*filename;	/* file to save as */
   u_char	idx;		/* page index */
   psbkground	background;	/* background rendered file info */
   float	wirewidth;
   float	outscale;
   float	gridspace;
   float	snapspace;
   short	orient;
   short	pmode;
   short	coordstyle;
   XPoint	drawingscale;
   XPoint	pagesize;	/* size of page to print on */
   XPoint	margins;
} Pagedata;

/*----------------------------------------------------------------------*/
/* Structure holding information about graphic images used.  These hold	*/
/* the original data for the images, and may be shared.			*/
/*----------------------------------------------------------------------*/

typedef struct {
   xcImage	*image;
   int		refcount;
   char		*filename;
} Imagedata;

/*----------------------------------------------------------------------*/
/* The main globally-accessible data structure.  This structure holds	*/
/* all the critical data needed by the drawing window 			*/
/*----------------------------------------------------------------------*/

typedef struct _windowdata *XCWindowDataPtr;

typedef struct _windowdata {

   XCWindowDataPtr next;	/* next window in list */

   /* widgets and X11 parameters */
   xcWidget	area;
   xcWidget	scrollbarh, scrollbarv;
   int		panx, pany;
   Window	window;
   GC 		gc;
#ifndef HAVE_CAIRO
   Pixmap	clipmask;
   Pixmap	pbuf;		/* clipmask buffer for hierarchical clipping */
   signed char	clipped;
   GC		cmgc;
#endif /* !HAVE_CAIRO */
   int		gccolor;
   xcIntervalId time_id;
   Boolean	redraw_needed;
   Boolean	redraw_ongoing;
#ifdef HAVE_CAIRO
   cairo_surface_t *surface;
   cairo_t	*cr;		/* cairo_drawing context */
   cairo_pattern_t *fixed_pixmap; /* pixmap holding the background data of   */
			        /* all fixed element. ie. not including the  */
			        /* element currently being edited.	     */
#else /* HAVE_CAIRO */
   Pixmap       fixed_pixmap;
#endif /* HAVE_CAIRO */
   /* global page parameters */
   short	width, height;
   short	page;
   float	vscale;		/* proper scale */
   XPoint	pcorner;	/* page position */

   /* global option defaults */
   float	textscale;
   float	linewidth;
   float	zoomfactor;
   short	psfont;
   u_short	anchor;
   u_short	style;
   int		color;
   short	filter;		/* selection filter */
   Boolean	manhatn;
   Boolean	boxedit;
   Boolean	pathedit;
   Boolean	snapto;
   Boolean	bboxon;
   Boolean	center;
   Boolean	gridon;
   Boolean	axeson;
   Boolean	invert;
   Boolean	mapped;		/* indicates if window is drawable */
   char		buschar;	/* Character indicating vector notation */
   Boolean	editinplace;
   Boolean	pinpointon;
   Boolean	pinattach;	/* keep wires attached to pins when moving objinsts */
   Boolean	showclipmasks;	/* draw clipmask shape outlines */
#ifndef TCL_WRAPPER
#ifdef HAVE_XPM
   Boolean	toolbar_on;
#endif
#endif

   /* buffers and associated variables */
   XPoint	save, origin;
   short	selects;
   short	*selectlist;
   short	attachto;
   short	lastlibrary;
   short	textpos;
   short	textend;
   objinstptr	topinstance;
   objectptr	editstack;
   Matrixptr	MatStack;
   pushlistptr	stack;
   pushlistptr	hierstack;
   event_mode_t	event_mode;
   char		*lastbackground;
   Cursor	*defaultcursor;
} XCWindowData;

/* Record for undo function */ 

typedef struct _undostack *Undoptr;

typedef struct _undostack {
   Undoptr next;	/* next record in undo stack */
   Undoptr last;	/* double-linked for "redo" function */
   u_int type;		/* type of event */
   short idx;		/* counter for undo event */
   objinstptr thisinst;	/* instance of object in which event occurred */
   XCWindowData *window;/* window in which event occurred */
   int idata;		/* simple undedicated integer datum */
   char *undodata;	/* free space to be malloc'd */
			/* (size dependent on "type") */
} Undostack;

/* This whole thing needs to be cleaned up. . . now I've got objects in */
/* "library" which are repeated both in the "instlist" pair and the	*/
/* "thisobject" pointer in each instance.  The instance is repeated on	*/
/* the library page.  Ideally, the instance list should only exist on	*/
/* the library page, and not be destroyed on every call to "composelib"	*/

typedef struct {
   short	number;
   objectptr	*library;
   liblistptr	instlist;	/* List of instances */
} Library;

typedef struct _Technology *TechPtr;

typedef struct _Technology {
   u_char	flags;		/* Flags for library page (changed, read-only) */
   char		*technology;	/* Namespace name (without the "::") */
   char		*filename;	/* Library file associated with technology */
   TechPtr 	next;		/* Linked list */
} Technology;

/* Known flags for library pages */

#define TECH_CHANGED		0x01
#define TECH_READONLY		0x02	/* Technology file not writable */
#define TECH_IMPORTED		0x04	/* Loaded only part of file */
#define TECH_REPLACE		0x08	/* Replace instances when reading */
#define TECH_REPLACE_TEMP	0x10	/* Temporary store */
#define TECH_USED		0x20	/* Temporary marker flag */
#define TECH_PREFER		0x40	/* Prefer technology on name conflict */

/*----------------------------------------------------------------------*/
/* A convenient structure for holding all the object lists		*/
/*----------------------------------------------------------------------*/

typedef struct {
   char		*libsearchpath;	 /* list of directories to search */
   char		*filesearchpath; /* list of directories to search */
   char		*tempfile;
   char		*tempdir;
   Boolean	retain_backup;
   xcIntervalId	timeout_id;
   int		save_interval;
   Boolean	filefilter;	/* Is the file list filtered? */
   Boolean	hold;		/* allow HOLD modifiers on buttons */
   Boolean	showtech;	/* Write technology names in library */
   u_short	new_changes;
   signed char	suspend;	/* suspend graphics updates if TRUE */
   short	numlibs;
   short	pages;
   Pagedata	**pagelist;
   Undoptr	undostack;	/* actions to undo */
   Undoptr	redostack;	/* actions to redo */
   Library	fontlib;
   Library	*userlibs;
   TechPtr 	technologies;
   objinstptr   *libtop;
   Imagedata	*imagelist;
   short	images;
   XCWindowData *windowlist;	/* linked list of known windows */
} Globaldata;


/*-------------------------------------------------------------------------*/
/* Track state of the ghostscript renderer 				   */
/*-------------------------------------------------------------------------*/

typedef enum {
   GS_INIT,	/* Initial state; gs is idle. */
   GS_PENDING,	/* Drawing in progress; gs is busy. */
   GS_READY	/* Drawing done; gs is waiting for "next". */
} gs_state_t;


/*-------------------------------------------------------------------------*/
/* For the eventmodes (ARC_MODE, EARC_MODE, SPLINE_MODE, etc) the drawing  */
/* and/or redrawing of the elements is centralized in functions		   */
/* corresponding to the eventmode name (arc_mode_draw, spline_mode_draw,   */
/* etc.). Depending on the state these functions clear, draw or whatever   */
/* is needed. A description of the options follows below		   */
/* fixed_pixmap refers to the background pixmap without the element(s)     */
/* currently being edited.						   */
/*-------------------------------------------------------------------------*/

typedef enum {
   xcDRAW_INIT,		/* Initalize fixed_pixmap and draw the element(s)  */
			/* element(s) currently being edited, including	   */
			/* edit lines.					   */

   xcDRAW_EDIT,		/* Draw the currently edited element(s), including */
			/* edit lines.					   */
   
   xcDRAW_FINAL,	/* Draw the finalized element(s), without edit	   */
			/* lines to the fixed_pixmap			   */

   xcDRAW_EMPTY,	/* Refresh the screen by drawing the fixed_pixmap  */
			/* This essentially clear the edited elements      */

   xcREDRAW_FORCED	/* Redraws everything, including edit lines. Only  */
	   		/* used in drawarea. This one should be called     */
			/* when something has changed (eg. zoom).	   */
			/* It will redraw the fixed_pixmap		   */
} xcDrawType;

/*----------------------------------------------------------------------*/
/* structures previously defined in menudefs.h				*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/* Menu Definitions for hierarchical pulldown menus                     */
/*----------------------------------------------------------------------*/

#ifndef TCL_WRAPPER

typedef struct _menustruct *menuptr;

typedef struct _menustruct {
        char *name;
        menuptr submenu;
        short size;
        XtCallbackProc func;
        void *passeddata;
} menustruct;

/*----------------------------------------------------------------------*/
/* Structure for calling routines from the Toolbar icons		*/
/*----------------------------------------------------------------------*/

typedef struct _toolbarstruct *toolbarptr;

typedef struct _toolbarstruct {
	char *name;
	char **icon_data;
        XtCallbackProc func;
        void *passeddata;
	char *hint;
} toolbarstruct;

#endif
/* Menus and Toolbars are taken care of entirely by scripts in the Tcl/	*/
/* Tk version of xcircuit.						*/
/*----------------------------------------------------------------------*/

