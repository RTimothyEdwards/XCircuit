/************************************************************
**
**       COPYRIGHT (C) 1993 UNIVERSITY OF PITTSBURGH
**       COPYRIGHT (C) 1996 GANNON UNIVERSITY
**                  ALL RIGHTS RESERVED
**
**        This software is distributed on an as-is basis
**        with no warranty implied or intended.  No author
**        or distributor takes responsibility to anyone 
**        regarding its use of or suitability.
**
**        The software may be distributed and modified 
**        freely for academic and other non-commercial
**        use but may NOT be utilized or included in whole
**        or part within any commercial product.
**
**        This copyright notice must remain on all copies 
**        and modified versions of this software.
**
************************************************************/


/* file cstitch.h */
/* ------------------------------------------------------------------------
 * Common definitions for Corner Stiching (see Osterhout '84 DAC paper)  1/90 stf
 *
 * ------------------------------------------------------------------------
 */
/* 
 */

/* These have different values from xcircuit.h, so undefine them for ASG */
#undef RIGHT
#undef TOP

#define LEFT 0
#define TOP 1		/* Used to define the desired side in neighbor finding */
#define UP 1
#define RIGHT 2
#define BOTTOM 3
#define DOWN 3

#define FURTHER 2
#define CLOSER -2

/*---------------------------------------------------------------
 * Templates for corner-stiched data-structures and operations.
 *---------------------------------------------------------------
 */

typedef struct tile_struct tile;	/* the basic cs element */
typedef struct tile_list tilist;	/* list of tile structure pointers */


struct tile_struct
/* tiles are rectangles at a given location, with corner pointers to the adjacent
 * tiles.  If a corner pointer is NULL, this indicates that the given tile is on the
 * edge of its respective universe. */
{

    int x_pos, y_pos;		/* lower left cartesian coordinates (bl,lb)*/
    int x_len, y_len;		/* relative upper rh coordinates (rt,tr) */
    int *this;			/* contents of the rectangle */
    tile *bl, *lb;		/* lower-left (lower left side & bottom) stitches */
    tile *rt, *tr;		/* upper-right (top & side) stitches */
};

struct tile_list
/* Used to maintain lists of tiles */
{
    tile *this;
    tilist *next;
};

/*------------------------------------------------------------------------ */
/* function specifications : */
/*------------------------------------------------------------------------ */
extern tile *create_tile();
/*    int xsize, ysize, *what, xorg, yorg;   /* Create a tile from scratch */

/*------------------------------------------------------------------------ */
extern tile *copy_tile();
/*    tile *t;		    /* return a unique copy of the given tile <t>. */

/*------------------------------------------------------------------------ */
extern tile *hmerge_tile();
/*    tile *t;    /* Check <t> for merger with its current right-hand neighbor.  
                     If a merger is appropriate, then nuke the neighbor and 
                     expand <t>, returning <t>. */

/*------------------------------------------------------------------------ */
extern tile *vmerge_tile();
/*    tile *t;    /* Check <t> for merger with its current upward neighbor.  
                     If a merger is appropriate, then nuke the neighbor and 
		     expand <t>, returning <t>. */

/*------------------------------------------------------------------------ */
extern tile *locate();
/*    tile *start;   /* locate the tile containing the point (x,y) given a <start> tile. 
 *    int x, y; 	NOTE:  This is slightly different from the algorithm in [Ost84]. 
 *		        This arranges tiles such that the top and right edges contain points, 
 *        		whereas the left and bottom edges do not. */

/*------------------------------------------------------------------------ */
extern tilist *list_neighbors();
/*    tile *who;
 *    int side;	/* {TOP, BOTTOM, LEFT, RIGHT } */
/* create a list of pointers for all of the neighboring tiles on the given side */

/*------------------------------------------------------------------------ */
extern tile *area_search();
/*    tile *start;			/* a starting tile 
 *    int xorg, yorg, xsize, ysize;	/* defines the area of interest 
 * This returns a pointer to the first 'used' tile in the given area.  If none are found, 
 *then NULL is returned. */

/*------------------------------------------------------------------------ */
extern void null();
/*    tile *t;		...DO NOTHING...   	*/
/*------------------------------------------------------------------------ */
extern int containsp();
/*    tile *t;	return TRUE if the given tile contains the given (x,y) point. 
 *    int x, y;	NOTE: This definition arranges the tiles st a tile contains all of the 
 *		points on its top and right edges. */

/*------------------------------------------------------------------------ */
extern tile *insert_tile();
/*    tile *start;			a starting tile 
 *    int xorg, yorg, xsize, ysize;	defines the area of interest
 *    int *what;				contents of the new tile
    
 * This returns a pointer to a newly-created tile of the given size, at the
 * given position if one can be made.  Otherwise NULL is returned. */
 
/*------------------------------------------------------------------------ */
extern tile *angled_tile_insertion();
/*    tile *start;   	 the starting (reference) tile */
/*    int anchX, anchY;	 XY origin of axis of motion (anchor point).
/*    int delX, delY;	 slope+dir of the axis of motion (line along which the new tile moves) */
/*    int pivX, pivY;	 the pivot point of the <new> tile. (relative to (0,0), 
                         (sizeX, sizeY) */
/*    int sizeX, sizeY;	 defines the size of the tile to be installed */
/*    int *box;		 contents of the <new> tile.  Referenced to (0,0). */

/* This returns a pointer to a newly-created tile of the given size, at some position
 * lying along the line given by the slope <delY>/<delX> and the point (<anchX>,<anchY>).
 * The point (<pivX>, <pivY>) is the point within the new tile that lies on this line.
 * The coordinate reference for the block being inserted is (0,0), whereas that for the 
 * for the tile(s) is the same as that for the tile <start>.
 * The new tile is placed as close to the given anchor point as possible. */

/*------------------------------------------------------------------------ */
tile *angled_group_tile_insertion();
/*    tile *start;   	 the starting (reference) tile */
/*    int anchX, anchY;	 origin of axis of motion (anchor pnt) in <start>'s tile space */
/*    int delX, delY;	 slope+dir of the axis of motion (line along which tiles move) */
/*    int pivX, pivY;	 the pivot point of the <new> tile set. (relative to (0,0)) */
/*    int *orgX, *orgY;  The solution found for this tile set. */
/*    tilist *tlist;	 list of tiles to be inserted into <start>'s tile space */

/* This returns a pointer to one of a set of newly-created tiles, created by adding
 * all of the tiles on <tlist> to the tile space defined by <start>.  They are added
 * at some position lying along a line given at an angle defined by <ydir>/<xdir>.
 * The position closest to the anchor point is selected. */

/*------------------------------------------------------------------------*/
extern tile *delete_tile();
/*    tile *deadTile;	This functions removes <t> fromthe set of tiles, performing 
                        all of the necessary mergers, etc..	*/
    /* NOTE:  This function depends upon the notion that the upper and right
     *        border of a tile are included in the tile, but not the lower &
     * 	      left sides.							*/

/*------------------------------------------------------------------------ */
extern int clean_up_neighbors();
/*    tile *who;	/* See that all of <who>'s neighbors point to him. */

/*------------------------------------------------------------------------ */
extern tilist *translate_origin();
/*    tilist *tl;	list of tiles to modify */
/*    int x, y; 	what to add to each x_pos, y_pos */

/*------------------------------------------------------------------------ */
tilist *allspace();
/*    tile *t;		Some tile in area to search */
/*    tilist **tl;	Where to list the located tiles */

/*------------------------------------------------------------------------ */
extern tilist *freespace();
/*    tile *t;		 Some tile in area to search */
/*    tilist **tl	  Where to list the empty tiles */

/*  Form a list of all tiles within the area containing <t> that have
    no valid <this> entry: */

/*------------------------------------------------------------------------ */
extern tilist *nonFreespace();
/*    tile *t;	   	Some tile in area to search */
/*    tilist **tl	        Where to list the used tiles */
    
/*------------------------------------------------------------------------ */
extern tilist *area_enumerate();
/*    tile *t;			*/
/*    tilist **soFar;		*/
/*    int (*filterFn)();        given a tile, returns TRUE if the tile is to be listed */
/*    int xorg, yorg;		Origin of area of interest (AOI) */
/*    int xext, yext;		Extent of area of interest */

/* Return a list containing all tiles within the AOI: */

/*------------------------------------------------------------------------ */
extern tilist *enumerate();
/*    tile *t;			*/
/*    tilist **soFar;   	*/
/*    int (*filterFn)(); 	given a tile, returns TRUE if the tile is to be listed */
    
/* Return a list containing all tiles to the right or below the given tile: */

/*------------------------------------------------------------------------ */
extern void reset_boundaries();
/*    tile *root;			tile in affected tilespace*/
/*    int x1, y1, x2, y2;		llhc and urhc of new space dimensions */
/* This will reset the boundaries of the tilespace containing <root> to the
   given dimensions, as long as no tiles are removed.  Otherwise, the
   smallest size is set. */

/*------------------------------------------------------------------------ */
extern void determine_bounding_box();
/*    tile *root;		   tile in affected tilespace*/
/*    int x1, y1, x2, y2;	   llhc and urhc of new (min)space dimensions */
/* Determine the min. bounding box that will contain all of the filled 
   tiles within the tile-space defined by <root>. 
   NOTE: This is a destructive routine, in that the BB for <root> remains 
   set to the minimum discovered.  */

/*------------------------------------------------------------------------ */
extern tile *find_ULHC();	
/*    tile *t;		Where to start looking */
/* Find the upper-left-hand corner tile in the space containing <t>. */

/*------------------------------------------------------------------------ */
extern tile *find_LRHC();	
/*    tile *t;		Where to start looking */
/* Find the lower-right-hand corner tile in the space containing <t>. */

/*------------------------------------------------------------------------ */
extern tilist *find_all_tiles_along_line();
/*    tile *start;		Tile in which to start looking */
/*    int srcX, srcY;		Point on line (in start?) */
/*    int delX, delY;		Slope of line */
/*    tilist **soFar;		Where to list the tiles found */

/* This function walks down the given line, and adds each tile it encounters to 
   the list returned. */

/*------------------------------------------------------------------------ */
/*extern tilist *find_all_tiles_along_swath();
/*    tile *start;		Tile in which to start looking */
/*    int srcX, srcY;		Center Point on line (in start?) */
/*    int delX, delY;		Slope of line */
/*    int width, length;        Width & Length of swath being checked */
/*    tilist **soFar;		Where to list the tiles found */

/* This function walks down the given line, and adds each tile it encounters to 
   the list returned.    A tile is encountered if it touches the given swath at
   any point.
 */

/*------------------------------------------------------------------------ */
extern tile *first_noncontiguous_full_tile();
/*     tilist *tlist;		list to look in. */

/*------------------------------------------------------------------------ */
extern tile *best_tile_in_space();
/*    tile *t;			some tile in the desired tile space */
/*    int (*filterFn)();	returns TRUE if <t1> is better than <t2>. */
    
    /* This returns the best filled tile in the given tile space. */

/*------------------------------------------------------------------------ */
extern int adjacent_tiles_p();
/* 	tile *t1, *t2;   */
/* Return TRUE iff t1 & t2 share a common edge */

/*------------------------------------------------------------------------ */
extern int most_right();
/*    tile *t1, t2;	*/
/* given two tiles, return TRUE if the right-edge of <t1> is further to 
   the right than the right edge of <t2>:	*/

/*------------------------------------------------------------------------ */
extern int most_left();
/*    tile *t1, t2;	*/

/* given two tiles, return TRUE if the left edge of <t1> is further to 
   the left than the left edge of <t2>:	*/
/*------------------------------------------------------------------------ */
extern int most_up();
/*    tile *t1, t2;	*/

/* given two tiles, return TRUE if the top-edge of <t1> is further up 
   than the top edge of <t2>:	*/

/*------------------------------------------------------------------------ */
extern int most_down();
/*    tile *t1, t2;	*/

/* given two tiles, return TRUE if the bottom-edge of <t1> is further 
   down than the bottom edge of <t2>:	*/

/*------------------------------------------------------------------------ */
extern void ps_print_tile_space();
/*    tile *t;		    A tile within the tilespace to be printed. */   
/*    FILE *f;		    Where to print the stuff to */
/*    int (*ps_printFn)();  given a file followed by a tile, print post-script code to 
                            represent the contents of the tile.  Should be able to 
			    handle NULL. */
/*    int edgesP;           if == TRUE, the PS code for the tile borders is made */



/*-- Managing Internal Data: ---------------------------------------------- */
/* THESE FUNCTIONS NEED TO BE DEFINED!  Without them, the program will not link.
 * They allow for the corner-stitching routines to apply to more-than just simple 
 * contents.  
/*------------------------------------------------------------------------ */

/*------------------------------------------------------------------------ */
extern void (*manage_contents__hmerge)();
/* tile *left;      This is the tile that will remain, with the contents of <right>.
 * tile *right;     This is the tile to be nuked (just before modifications happen) */

/*------------------------------------------------------------------------ */
extern void (*manage_contents__vmerge)();
/* tile *bottom;  This is the tile that will remain, with the contents of <top>.
 * tile *top;     This is the tile to be nuked (just before modifications happen) */

/*------------------------------------------------------------------------ */
extern void (*manage_contents__hsplit)();
/* tile *left;      This is the parent tile after mods.
 * tile *right;     This is the resulting child tile (created from <left>) */

/*------------------------------------------------------------------------ */
extern void (*manage_contents__vsplit)();
/* tile *top;     This is the parent tile (after mods) */
/* tile *bottom;  This is the resulting child tile created from <top>. */

/*------------------------------------------------------------------------ */
extern void (*manage_contents__dead_tile)();
/* tile *old;  This is the tile that was just wasted;  Its neighbors are correct,
               and may be searched for info concerning content information. */
/*------------------------------------------------------------------------ */
extern int (*solidp)();
/*    tile *t;	 return TRUE iff tile t is used. */
/*------------------------------------------------------------------------ */
extern int (*contents_equivalentp)();
/*    tile *t1, *t2;    TRUE iff tiles <t1> & <t2> have the same color */
/*------------------------------------------------------------------------ */
extern void (*insert_contents)();
/*    tile *t;   Return the pointer to the proper contents for the given tile, */
/*    module *m; given that <m> is being inserted into the tile <t>.           */

/*------------------------------------------------------------------------ */
/* The standard assignments for these are as follows:			 */

extern int std_solidp();			/* return TRUE iff tile t is used. */
/*    tile *t;		*/

extern void std_insert_contents();		 /* Make the standard assignment for 
/*    tile *t;					    tile contents (add <w> to <t>) 
      int *w;		*/

extern int std_contents_equivalentp();		/* Return TRUE if the tiles are of the 
/*    tile *t1, *t2;				   same color */

extern void null();				/* Do nothing */
/*    tile *t;		*/






/*---------------------------------------------------------------
 * END OF FILE
 *---------------------------------------------------------------
 */
