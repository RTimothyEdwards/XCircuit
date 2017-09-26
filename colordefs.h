/*-------------------------------------------------------------------------*/
/* colordefs.h 								   */
/* Copyright (c) 2002  Tim Edwards, Johns Hopkins University       	   */
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/* Add colors here as needed. 						   */
/* Reflect all changes in the resource manager, xcircuit.c		   */
/* And ApplicationDataPtr in xcircuit.h					   */
/*-------------------------------------------------------------------------*/

#define NUMBER_OF_COLORS	18

#define BACKGROUND		0
#define FOREGROUND		1
#define SELECTCOLOR		2
#define FILTERCOLOR		3
#define GRIDCOLOR		4
#define SNAPCOLOR		5
#define AXESCOLOR		6
#define OFFBUTTONCOLOR		7
#define AUXCOLOR		8
#define BARCOLOR		9
#define PARAMCOLOR		10

/* The rest of the colors are layout colors, not GUI colors */

#define BBOXCOLOR		11
#define LOCALPINCOLOR		12
#define GLOBALPINCOLOR		13
#define INFOLABELCOLOR		14
#define RATSNESTCOLOR		15
#define CLIPMASKCOLOR		16
#define FIXEDBBOXCOLOR		17

#define DEFAULTCOLOR	-1		/* Inherits color of parent */
#define DOFORALL	-2		/* All elements inherit same color */
#define DOSUBSTRING	-3		/* Only selected substring drawn */
					/* in the SELECTCOLOR		*/
#define BADCOLOR	-1		/* When returned from query_named_color */
#define ERRORCOLOR	-2		/* When returned from query_named_color */

/*-------------------------------------------------------------------------*/
