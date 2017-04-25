
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

/* file cstitch.c */
/* ------------------------------------------------------------------------
 * Common definitions for Corner Stiching (see Osterhout '84 DAC paper)  1/90 stf
 *
 * ------------------------------------------------------------------------
 */

#include "cstitch.h"
#include "list.h"

extern int debug_level;

/*------- Forward !#*&^@!*#@ Reference ------------------------- */
int std_solidp();
int std_contents_equivalentp();
void std_insert_contents();

/*---------------------------------------------------------------
 * Templates for corner-stiched data-structures (see cstitch.h)
 *---------------------------------------------------------------
 * 
 *	typedef struct tile_struct tile;	
 *	typedef struct tile_list tilist;	
 *
 *	struct tile_struct
 *	{
 *	    int x_pos, y_pos;		 lower left cartesian coordinates (bl,lb)
 *	    int x_len, y_len;		 relative upper rh coordinates (rt,tr)
 *	    int *this;		         contents of the rectangle
 *	    tile *bl, *lb;		 lower-left (lower left side & bottom) stitches
 *	    tile *rt, *tr;	         upper-right (top & side) stitches 
 *	};

 *	struct tile_list
 *	{
 *	    tile *this;
 *	    tilist *next;
 *	};
 */

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

/* ---- Global variables required by cstitch.c: ---------------------------*/
/* If anything interesting is done with the contents, these functions need to be 
   overloaded so  that the merger/splitting functions can do interesting things... */

  int (*solidp)() = std_solidp;
  int (*contents_equivalentp)() = std_contents_equivalentp;
  void (*insert_contents)() = std_insert_contents;
  void (*manage_contents__hmerge)() = null;
  void (*manage_contents__vmerge)() = null;
  void (*manage_contents__hsplit)() = null;
  void (*manage_contents__vsplit)() = null;
  void (*manage_contents__dead_tile)() = null;


/*------------------------------------------------------------------------ */
tilist *list_neighbors();
int verify_tile_space();

/*------------------------------------------------------------------------ */
void null(t)
    tile *t;
{}
/*------------------------------------------------------------------------ */
/*------------------------------------------------------------------------ */
void reset_boundaries(root, x1, y1, x2, y2)
    tile *root;
    int x1, y1, x2, y2;
{
    /* This will reset the boudaries of the tilespace containing <root> to the
       given dimensions, as long as no tiles are removed.  Otherwise, the
       smallest size is set. 

       This code makes a BIG assumption in that no (colored) tiles run up to 
       the edge of the tile space... */

    int minX, maxX, minY, maxY, diff;
    tilist *tl, *right = NULL, *left = NULL;
    tilist *top = NULL, *bottom = NULL;
    tile *ulhc, *lrhc;

    /* Added by Tim for debugging */
    if (verify_tile_space(root) == FALSE)
    {
	fprintf(stderr, "reset_boundaries: Corrupted tile space! (on entry)\n");
    }
    /* end of addition --Tim */

    ulhc = find_ULHC(root);
    lrhc = find_LRHC(root);

    minX = ulhc->x_pos;			maxY = ulhc->y_pos + ulhc->y_len;
    maxX = lrhc->x_pos + lrhc->x_len;	minY = lrhc->y_pos;

    right = find_all_tiles_along_line(lrhc, maxX - 1, maxY - 1, 0, -1, &right);
    left = find_all_tiles_along_line(ulhc, minX + 1, minY + 1, 0, 1, &left);
    bottom = find_all_tiles_along_line(lrhc, maxX - 1, minY + 1, -1, 0, &bottom);
    top = find_all_tiles_along_line(ulhc, minX + 1, maxY - 1, 1, 0, &top);

    /* be assured that the min/max X settings aren't bad: */
    maxX = MAX(x1, x2);
    for(tl = right; tl != NULL; tl = tl->next)
    {
	if (tl->this->x_pos >= maxX) maxX = tl->this->x_pos + 1;
    }
    minX = MIN(x1, x2);
    for(tl = left; tl != NULL; tl = tl->next)
    {
	if (tl->this->x_pos + tl->this->x_len <= minX) 
	    minX = tl->this->x_pos + tl->this->x_len - 1;
    }
    
    /* be assured that the min/max Y settings aren't bad: */
    maxY = MAX(y1,y2);
    minY = MIN(y1,y2);
    if (ulhc->y_pos >= maxY) maxY = ulhc->y_pos + 1;
    if (lrhc->y_pos + lrhc->y_len <= minY) minY = lrhc->y_pos + lrhc->y_len - 1;

    /* Now reset the boundaries: along the left and right sides: */
    for (tl = left; tl != NULL; tl = tl->next) 
    {
	diff = tl->this->x_pos - minX;
	tl->this->x_pos = minX;
	tl->this->x_len += diff;
    }
    for (tl = right; tl != NULL; tl = tl->next) 
    {
        tl->this->x_len = maxX - tl->this->x_pos;
    }

    /* Now reset the boundaries: along the top and bottom: */
    for (tl = bottom; tl != NULL; tl = tl->next) 
    {
	diff = tl->this->y_pos - minY;
	tl->this->y_pos = minY;
	tl->this->y_len += diff;
	/* This whole loop was:
	   diff = lrhc->y_pos - minY;	   lrhc->y_pos = minY;	   
	   lrhc->y_len += diff;) */	   
    }
    for (tl = top; tl != NULL; tl = tl->next) 
    {
        tl->this->y_len = maxY - tl->this->y_pos;
	/* (This whole loop was :     ulhc->y_len = maxY - ulhc->y_pos; ) */
    }

    /* Cleanup */
    free_list(right);		free_list(top);
    free_list(left);		free_list(bottom);

    if (verify_tile_space(root) == FALSE)
    {
	fprintf(stderr, "reset_boundaries: Corrupted tile space! (on exit)\n");
    }

}
/*-----------------------------------------------------------------------*/
void determine_bounding_box(root, x1, y1, x2, y2, compress)
    tile *root;
    int *x1, *y1, *x2, *y2;
    int compress;
{
    /* Determine the min. bounding box that will contain all of the filled 
       tiles within the tile-space defined by <root>. 
       NOTE: This is a destructive routine, in that the BB for <root> remains 
       set to the minimum discovered.  */

    tile *ul, *lr;

    /* Reset the bb based on the minimum permissible boundry: */
    if (compress)
       reset_boundaries(root, 0, 0, 0, 0);

    ul = find_ULHC(root);
    lr = find_LRHC(root);

    *x1 = ul->x_pos;
    *y1 = lr->y_pos;
    *x2 = lr->x_pos + lr->x_len;
    *y2 = ul->y_pos + ul->y_len;
}
/*------------------------------------------------------------------------ */
tile *create_tile(xsize,ysize,what,xorg,yorg)
    int xorg,yorg,xsize,ysize;
    int *what;
    
/* Create a tile from scratch */
{
    tile *t;
    t = (tile *)calloc(1, sizeof(tile));
    t->x_pos = xorg;
    t->y_pos = yorg;
    t->x_len = xsize;
    t->y_len = ysize;
    t->this = what;
    t->bl = t->lb = t->rt = t->tr = NULL;
    return(t);
}
/*------------------------------------------------------------------------ */
tile *copy_tile(t)
    tile *t;
    /* return a unique copy of the given tile <t>. */
{
    tile *s = create_tile(t->x_len, t->y_len, t->this, t->x_pos, t->y_pos);
    s->bl = t->bl;
    s->lb = t->lb;
    s->rt = t->rt;
    s->tr = t->tr;
    return(s);
}
/*------------------------------------------------------------------------ */
tile *hmerge_tile(t)
    tile *t;
    /* Check <t> for merger with its current right-hand neighbor.  If a merger is
     * appropriate, then nuke the neighbor and expand <t>, returning <t>. 
     * The contents should  be correct before this is called.  */
{
    tile *temp;
    tilist *tl = NULL, *scrap;
    
    if (t != NULL)
    {
	if ((t->tr != NULL) &&		/* If there's a guy to the right */
	    (contents_equivalentp(t, t->tr) == TRUE) &&
	    (t->tr->y_pos == t->y_pos) &&	/* And everything is lined up OK */
	    (t->tr->y_len == t->y_len))	
	{
	    temp = t->tr;			/* Nuke the guy to the right */
	    
	    /* Fixup the tile contents to reflect these changes: */
	    
	    t->x_len += temp->x_len;
	    t->tr = temp->tr;
	    t->rt = temp->rt;
	    /* need to fix all of the pointers pointing to <temp> so they will 
	       point to <t> */
	    scrap = list_neighbors(temp, BOTTOM);
	    for (tl = scrap; tl != NULL; tl = tl->next) 
	    {
		/* if (((tl->this->x_pos + tl->this->x_len) <= (t->x_pos + t->x_len)) && 
		        (tl->this != t)) */
		if (tl->this->rt == temp) tl->this->rt = t;	    
	    }
	    free_list(scrap);

	    scrap = list_neighbors(temp, RIGHT);
	    for (tl = scrap; tl != NULL; tl = tl->next) 
	    {
		/* if ((tl->this->y_pos >= t->y_pos) && (tl->this != t)) */
		if (tl->this->bl == temp) tl->this->bl = t;
	    }
	    free_list(scrap);

	    scrap = list_neighbors(temp, TOP);
	    for (tl = scrap; tl != NULL; tl = tl->next)   
	    {
		/* if ((tl->this->x_pos >= t->x_pos) && (tl->this != t)) */
		if (tl->this->lb == temp)tl->this->lb = t;
	    }
	    free_list(scrap);
	    
	    (*manage_contents__hmerge)(t, temp);
	    (*manage_contents__dead_tile)(temp);
	    my_free(temp);
	    temp = NULL;
	}
    }
    return(t);
}
/*------------------------------------------------------------------------ */
tile *vmerge_tile(t)
    tile *t;
    /* Check <t> for merger with its current upward neighbor.  If a merger is
     * appropriate, then nuke the neighbor and expand <t>, returning <t>. 
     * The contents should be correct before this is called.   */
{
    tile *temp;
    tilist *tl = NULL, *scrap;

    if (t != NULL)
    {
	if ((t->rt != NULL) &&		/* If there's a guy on top */
	    (contents_equivalentp(t, t->rt) == TRUE) &&
	    (t->rt->x_pos == t->x_pos) &&	/* And things line up OK */
	    (t->rt->x_len == t->x_len))	
	{
	    temp = t->rt;			/* nuke the guy on top */
	    
	    /* Fixup the tile contents to reflect these changes: */
	    
	    t->y_len += temp->y_len;
	    t->rt = temp->rt;
	    t->tr = temp->tr;
	    
	    /* need to fix all of the pointers pointing to <temp> so they will 
	       point to <t>*/

	    scrap = list_neighbors(temp, LEFT);
	    for (tl = scrap; tl != NULL; tl = tl->next)  
	    {
		/* if (((tl->this->y_pos + tl->this->y_len) <= (t->y_pos + t->y_len)) && 
		       (tl->this != t)) */
		if (tl->this->tr == temp) tl->this->tr = t;
	    }
	    free_list(scrap);

	    scrap = list_neighbors(temp, RIGHT);
	    for (tl = scrap; tl != NULL; tl = tl->next) 
	    {
		/* if ((tl->this->y_pos >= t->y_pos) && (tl->this != t)) */
		if (tl->this->bl == temp) tl->this->bl = t;     
	    }
	    free_list(scrap);

	    scrap = list_neighbors(temp, TOP);
	    for (tl = scrap; tl != NULL; tl = tl->next)
	    {
		/* if ((tl->this->x_pos >= t->x_pos) && (tl->this != t)) */
		if (tl->this->lb == temp) tl->this->lb = t; 
	    }
	    free_list(scrap);

	    (*manage_contents__vmerge)(t, temp);
	    (*manage_contents__dead_tile)(temp);
	    my_free(temp);
	    temp = NULL;
	}
    }
    return(t);
}

/*------------------------------------------------------------------------ */
/*------------------------------------------------------------------------ */
tile *locate(start, x, y)
    tile *start;
    int x, y;
/* locate the tile containing the point (x,y) given a <start> tile.  NOTE:  This
 * is slightly different from the algorithm in [Ost84]. 
 * NOTE:  This arranges tiles such that the top and right edges contain points, whereas
 *        the left and bottom edges do not.  Also, a tile is considered to contain its
 *        top & right-side edges, but not its left/bottom edges.  */
{
    tile *t = start;
    if (t == NULL) return (NULL);	/* bad requests cause big problems */
    
/* first find the right 'row' of tiles */
    else if (y < t->y_pos) t = locate(t->lb, x, y);
    else if ((y == t->y_pos) && (t->lb != NULL)) t = locate(t->lb, x, y);
    else if ((t->y_len + t->y_pos) < y) t = locate(t->rt, x, y);
    
    if (t == NULL) return (NULL);	/* bad requests cause big problems */

/* walk down the 'row' */
    if (t->x_pos > x) return(locate(t->bl, x, y));
    if ((t->x_pos == x) && (t->bl != NULL)) return(locate(t->bl, x, y));
    else if ((t->x_len + t->x_pos) < x) return(locate(t->tr, x, y));
    else return(t);
}

/*------------------------------------------------------------------------ */
tilist *list_neighbors(who, side)
    tile *who;
    int side;	/* {TOP, BOTTOM, LEFT, RIGHT } */
/* create a list of pointers for all of the neighboring tiles on the given side */
/* This is simply a pointer-tracing routine. */
{
    tilist *tlist = NULL;
    tile *s;
    
    switch(side)
    {
	case(TOP):   { s = who->rt;
		       if (s != NULL) 
		       {
			   push(s, &tlist);
			   s = s->bl;
			   while((s != NULL) && (s->x_pos >= who->x_pos))
			   {
			       push(s, &tlist);
			       s = s->bl;
			   }
		       }
		       break;
		      }

	case(RIGHT): { s = who->tr;
		       if (s != NULL) 
		       {
			   queue(s, &tlist);
			   s = s->lb;
			   while((s != NULL) && (s->y_pos >= who->y_pos))
			   {
			       queue(s, &tlist);
			       s = s->lb;
			   }
		       }
		       break;
		      }

	case(BOTTOM): { s = who->lb;
			if (s != NULL) 
			{
			    queue(s, &tlist);
			    s = s->tr;
			    while((s != NULL) && (s->x_pos < who->x_pos + who->x_len))
			    {
				queue(s, &tlist);
				s = s->tr;
			    }
			}
			break;
		      }

	case(LEFT):  { s = who->bl;
		       if (s != NULL) 
		       {
			   push(s, &tlist);
			   s = s->rt;
			   while((s != NULL) && (s->y_pos < who->y_pos + who->y_len))
			   {
			       push(s, &tlist);
			       s = s->rt;
			   }
		       }
		       break; 
		   }
    }
    return(tlist);
}
/*------------------------------------------------------------------------ */
tile *area_search(start, xorg, yorg, xsize, ysize)
    tile *start;			/* a starting tile */
    int xorg, yorg, xsize, ysize;	/* defines the area of interest */
/* This returns a pointer to the first 'used' tile in the given area.  If
 * none are found, then NULL is returned. */
{
    int x = xorg + xsize;
    tile *s = locate(start, xorg, yorg + ysize);

    /* Boundry condition problem */
    if (((s->x_pos + s->x_len) <= xorg) && (s->tr != NULL)) 
        s = locate(s->tr, xorg + 1, yorg + ysize);	

    if ((*solidp)(s) == TRUE) return(s);
    else if ((s->x_pos + s->x_len) < x) return(s->tr);
    else if (s->y_pos > yorg) 
        return(area_search(s, xorg, yorg, xsize, s->y_pos - yorg));
    else return (NULL);
}

/*------------------------------------------------------------------------ */
int std_solidp(t)
    tile *t;
/* return TRUE iff tile t is used. */
{
    return((t->this != NULL) ? TRUE : FALSE);
}
/*------------------------------------------------------------------------ */
void std_insert_contents(t, w)
    tile *t;
    int *w;
{	/* Make the standard assignment for tile contents (add <w> to <t>) */
    t->this = w;
}
/*------------------------------------------------------------------------ */
int std_contents_equivalentp(t1, t2)
    tile *t1, *t2;
{	/* Return TRUE if the tiles are of the same color */
    if (t1->this == t2->this) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------ */
int containsp(t, x, y)
    tile *t;
    int x, y;
    /* return TRUE if the given tile contains the given (x,y) point. 
     * NOTE: This definition arranges the tiles st a tile contains all of the points
     * on its top and right edges. */
{
     if ((t->x_pos < x) && ((t->x_pos + t->x_len) >= x) &&
	 (t->y_pos < y) && ((t->y_pos + t->y_len) >= y)) return (TRUE);
     return(FALSE);
 }
/*------------------------------------------------------------------------ */
list *create_tile_pair(t1, t2)
    tile *t1, *t2;
{
    /* Create a list that is an ordered pair of tiles */
    list *l = (list *)calloc(1, sizeof(tilist));
    push(t2, &l);
    push(t1, &l);
    return(l);
}
/*------------------------------------------------------------------------ */
tile *vsplit_tile(p, y)
    tile *p;
    int y;
{
    /* Split <p> along the axis <y>.  Return the lower tile.  All pointers should be 
       correct. Opposite of "vmerge" */

    tile *c;
    if ((p != NULL) && (y > p->y_pos) && (y < (p->y_pos + p->y_len)))
    {
	c = copy_tile(p);	/* <c> becomes the lower tile */
	p->lb = c;
	p->y_len = p->y_pos + p->y_len - y;
	p->y_pos = y;

	c->rt = p;
	c->y_len = y - c->y_pos;
	c->tr = locate(p, c->x_pos + c->x_len + 1, c->y_pos + c->y_len + 1);
	if ((c->tr != NULL) && (c->tr->y_pos >= y))
	    c->tr = locate(p, c->x_pos + c->x_len + 1, c->y_pos + c->y_len);

	p->bl = locate(p, p->x_pos - 1, y);

	/* Now hunt down all of the special cases where the <= vs < issues can bite
	   you:  (These work around weaknesses in the definition of a tile as being
	   inclusive vs being exclusive of its borders) */
	if ((p->bl != NULL) && (p->bl->y_pos + p->bl->y_len <= y))
	    p->bl = locate(p, p->x_pos - 1, y + 1);
	if ((p->bl != NULL) && (p->bl->y_pos > p->y_pos))
	    p->bl = locate(p, p->x_pos - 1, y - 1);
	if ((p->bl != NULL) && (p->bl->x_pos + p->bl->x_len < p->x_pos))
	    p->bl = locate(p, p->x_pos, y);

	clean_up_neighbors(p);
	clean_up_neighbors(c);

	if ((verify_tile_space(p) == FALSE) || (verify_tile_space(c) == FALSE))
	{
	    fprintf(stderr, "vsplit_tile: Corrupted tile space!\n");
	}    
	/* Clean up the contents of these tiles: */
	(*manage_contents__vsplit)(p, c);
	return(c);
    }
    else if (y == p->y_pos) return(p->lb);
    else if (y == (p->y_pos + p->y_len)) return(p);
    else return(NULL);		/* Big Problem */
}
/*------------------------------------------------------------------------ */
tile *hsplit_tile(p, x)
    tile *p;
    int x;
{
    /* Split <p> along the axis <x>.  Return the rh tile.  All pointers should be 
       correct. Opposite of "hmerge" */

    tile *c;
    if ((p != NULL) && (x > p->x_pos) && (x < (p->x_pos + p->x_len)))
    {
	c = copy_tile(p);	/* <c> becomes the right side tile */
	c->bl = p;
	c->x_len = p->x_pos + p->x_len - x;
	c->x_pos = x;
	p->tr = c;

	p->x_len = x - p->x_pos;
	c->lb = locate(p, x + 1, c->y_pos);

	/* Now hunt down all of the special cases where the <= vs < issues can bite
	   you:  (These work around weaknesses in the definition of a tile as being
	   inclusive vs being exclusive of its borders) */
	if (c->lb != NULL && (c->lb->x_pos > c->x_pos))
	   c->lb = locate(p, x, c->y_pos);
	p->rt = locate(p, x - 1, p->y_pos + p->y_len + 1);
	if ((p->rt != NULL) && (p->rt->x_pos + p->rt->x_len <= x))
	    p->rt = locate(p, x, p->y_pos + p->y_len + 1);
	clean_up_neighbors(p);
	clean_up_neighbors(c);

	if ((verify_tile_space(p) == FALSE) || (verify_tile_space(c) == FALSE))
	{
	    fprintf(stderr, "hsplit_tile: Corrupted tile space!\n");
	}    

	/* Clean up the contents of these tiles: */
	(*manage_contents__hsplit)(p, c);
	return(c);
    }
    else if (x == p->x_pos) return(p);
    else if (x == (p->x_pos + p->x_len)) return(p->tr);
    else return(NULL);		/* Big Problem */
}
/*------------------------------------------------------------------------ */
tile *insert_tile(start, xorg, yorg, xsize, ysize, what)
    tile *start;			/* a starting tile */
    int xorg, yorg, xsize, ysize;	/* defines the area of interest */
    int *what;				/* contents of the new tile */
    
/* This returns a pointer to a newly-created tile of the given size, at the
 * given position if one can be made.  Otherwise NULL is returned. */

/* To maintain the internals of the tiles, as each tile is cut or made, it is 
   added to a list of tiles to be handled specially. */
{
    tile *left, *right, *lefTop, *righTop, *leftBot, *rightBot;
    tile *temp, *bot, *top, *new;
    tilist *tl, *Copied = NULL, *Clipped = NULL, *New = NULL;
    
    int x = xorg + xsize;
    int y = yorg + ysize;
    int ymark = y;

    /* First see if such a tile can be made: */
    if (area_search(start, xorg, yorg, xsize, ysize) != NULL) return(NULL);

    /* locate the top left-hand (LH) corner tile for the AOI */
    top = locate(start, xorg + 1, y);
    
    /* Now split this tile vertically, as necessary: <leftBot> becomes the lower tile */
    leftBot = vsplit_tile(top, y);	          /* <leftBot> becomes the lower tile */

    /* Do the same to the bottom of the AOI: */
    temp = locate(top, xorg, yorg);
    bot = vsplit_tile(temp, yorg);


    /* Now begin a loop, splitting tiles as you go.  Note that the pointers for, and
       the size of <new> changes as you go along to insure correctness. */
    left = leftBot;

    while (ymark > yorg)
    {
	new = hsplit_tile(left, xorg);
	right = hsplit_tile(new, x);

	insert_contents(new, what);
	vmerge_tile(left);
	vmerge_tile(right);
	vmerge_tile(new);
	
	ymark = new->y_pos;
	left = locate(new, xorg, ymark);
	if ((left != NULL) && (left->y_pos >= ymark))
	    left = locate(new, xorg, ymark - 1);
	if ((left != NULL) && ((left->x_pos + left->x_len) <= xorg))
	{
	    if (new->lb->y_len == 1) left = locate(new, xorg + 1, ymark);
	    else left = locate(new, xorg + 1, ymark - 1);
	}
    }
    /* Cleanup the tiles directly below the area of interest: */
    if ((new->bl != NULL) && (new->bl->lb != NULL)) vmerge_tile(new->bl->lb);  
    if ((right != NULL) && (right->lb != NULL)) vmerge_tile(right->lb);	

    if (verify_tile_space(new) == FALSE)
    {
	fprintf(stderr, "insert_tile: Corrupted tile space! abort! (on exit)\n");
	/* exit(1); */
	return NULL;
    }
    
    return(new);
}
/*------------------------------------------------------------------------ */
int next_move(min, max, current, direction, epsilon)
    int *min, *max, current, *epsilon;
{
    /* return the new value of current, if a newtonian search step is taken in 
     * the direction of <del>.  <epsilon is reset to reflect the move. */

    if (direction == FURTHER)
    {
	/* Move in the direction of <max> */
	*epsilon = (*epsilon == 1) ? 0 : (*max - current + 1)/ 2;    /* roundoff bias */
	*min = current;
	return (*min + *epsilon);
    }
    else/* (direction == CLOSER) */
    {
	/* Move in the direction of <min> */
	*epsilon = (*epsilon == 1) ? 0 : (current - *min + 1)/ 2;
	*max = current;
	return (*max - *epsilon);
    }
}	
/*------------------------------------------------------------------------ */
tile *angled_tile_help(start, anchX, anchY, delX, delY, 
		       pivX, pivY, sizeX, sizeY, box, cornerTile) 
    tile *start;   	/* the starting (reference) tile */
    int anchX, anchY;	/* XY origin of axis of motion (anchor point).	     */
    int delX, delY;	/* slope+dir of the axis of motion (line along which the
			   new tile moves) */
    int pivX, pivY;	/* the pivot point of the <new> tile. (relative to (0,0), 
			   (sizeX, sizeY) */
    int sizeX, sizeY;	/* defines the size of the tile to be installed */
    int *box;		/* contents of the <new> tile.  Reference to (0,0). */
    tile *cornerTile;	/* tile on the edge of the area to search in */
    
{    
/* This returns a pointer to a newly-created tile of the given size, at the
 * some position lying along a line given at an angle defined by <ydir>/<xdir>.
 * The position closest to the given <start> tile is selected.  The line is
 * drawn from the origin of <start> to the origin of the new tile.  */

/* NO ATTEMPT HAS BEEN MADE TO OPTIMIZE THIS CODE, so it is rather redundant. */

    tilist *tl;
    tile *new, *flag;
    int epsilon, b, currentX, lastX, maxX, minX;
    int orgX, orgY, tempOrgX, tempOrgY;
    int x, y, foundOne = FALSE;

    /* This section of code locates the closest, viable origin for <new>: */

/* Operate on the left and right 90 degree arcs: */
    if ((delX != 0) && (abs(delX) > abs(delY)))
    {	    /* Given a useful delta-X value, move along the X-Axis: */

	/* Now if there is no such tile, reset corner based on the edge of the 
	   known universe: */
	if (cornerTile != NULL)
	{
	    /* There is a tile <cornerTile> lying along the line that may interfere: */
	    if (delX > 0)
	    {	/* Moving to the right: */
		minX = anchX;
		y = (int)((float)delY/(float)delX * (cornerTile->x_pos - anchX)) + anchY;

		if (delY == 0) maxX = cornerTile->x_pos;

		else if (delY > 0)	/* either maxX = corner->x_pos or @ 
					   y = corner->y_pos */
		{ 
		    /* Moving up and to the right: */
		    if (y < cornerTile->y_pos)
		    {	/* intersection occurs along bottom edge of <cornerTile> */
			maxX = (int)((float)delX/(float)delY * 
				     (cornerTile->y_pos - anchY)) + anchX;
		    }
		    else
		    {	/* intersection occurs along left edge of <cornerTile> */
			maxX = cornerTile->x_pos;
		    }
		}
		else /* either maxX = corner->x_pos or @ y = corner->y_pos + 
			                                     corner->y_len     */
		{
		    /* Moving down and to the right: */
		    if (y > (cornerTile->y_pos + cornerTile->y_len))
		    {	
			/* intersection occurs along top edge of <cornerTile> */
			maxX = (int)((float)delX/(float)delY * 
				     (cornerTile->y_pos + cornerTile->y_len - anchY)) + 
				     anchX;
		    }
		    else
		    {	
			/* intersection occurs along left edge of <cornerTile> */
			maxX = cornerTile->x_pos;
		    }
		}
	    }
	    else
	    {	/* Moving to the left: */
		minX = anchX;
		y = (int)((float)delY/(float)delX * 
			  (cornerTile->x_pos + cornerTile->x_len - anchX)) + anchY;

		if (delY == 0) maxX = cornerTile->x_pos + cornerTile->x_len;

		else if (delY > 0)	/* either minX = corner->x_pos + corner->x_len 
					   or @ y = corner->y_pos + corner->y_len */
		{
		    /* Moving up and to the left: */
		    if (y < (cornerTile->y_pos + cornerTile->y_len))
		    {	/* intersection occurs along bottom edge of <cornerTile> */
			maxX = (int)((float)delX/(float)delY * 
				     (cornerTile->y_pos - anchY)) + anchX;
		    }
		    else
		    {	/* intersection occurs along right edge of <cornerTile> */
			maxX = cornerTile->x_pos + cornerTile->x_len;
		    }
		}
		else /* either minX = corner->x_pos + corner->x_len or @ y = 
			corner->y_pos */
		{
		    /* Moving down and to the left: */
		    if (y > (cornerTile->y_pos + cornerTile->y_len))
		    {	
			/* intersection occurs along top edge of <cornerTile> */
			maxX = (int)((float)delX/(float)delY * 
				     (cornerTile->y_pos + cornerTile->y_len - anchY)) + 
				     anchX;
		    }
		    else
		    {	/* intersection occurs along right edge of <cornerTile> */
			maxX = cornerTile->x_pos + cornerTile->x_len;
		    }
		}
	    }
	}
	else	/* cornerTile = NULL -- The simple case */
	{
	    /* First find the corner tile on the edge of the given space in the 
	       direction that the new tile is to be moved:			*/
	    if (delX > 0)
	    {	/* Moving to the right: */
		cornerTile = find_LRHC(start);
		minX = anchX;
		maxX = cornerTile->x_pos + cornerTile->x_len;
	    }
	    else
	    {	/* Moving to the left: */
		cornerTile = find_ULHC(start);
		minX = anchX;
		maxX = cornerTile->x_pos;
	    }
	}
	
       	/* Choose a first point to start at */
	epsilon = currentX = (minX + maxX) / 2;
	b = ( anchY - (int)((float)delY/(float)delX * (float)anchX) );
	
	/* Check the initial points to insure that they fall within the tile 
	   space defined */
	tempOrgX = currentX - pivX;
	tempOrgY = (int)(((float)delY/(float)delX) * (float)currentX) + b - pivY;
	if (locate(start, tempOrgX, tempOrgY) == NULL)		 /* This is a problem */
	{
	    epsilon = currentX = currentX/2;				/* A guess. */
	    fprintf(stderr, "Bad origin point selected; trying again...\n");
	    
	}


	/* Perform a newtonian search (along the X-Axis) to find the closest 
	   point to use: */
	while (abs(epsilon) > 1) 
	{
	    /* define the where <new>'s origin should go, based on the axis of motion: */
	    tempOrgX = currentX - pivX;
	    tempOrgY = (int)(((float)delY/(float)delX) * (float)currentX) + b - pivY;

	    /* See if <new> can be placed: */
	    flag = area_search(start, tempOrgX, tempOrgY, sizeX, sizeY);

	    if (flag != NULL)	
	    {	/* The tile can't go here. - try going further away */
		currentX = next_move(&minX, &maxX, currentX, FURTHER, &epsilon);
	    }
	    else 
	    {	/* The tile can go here - try going closer */
		currentX = next_move(&minX, &maxX, currentX, CLOSER, &epsilon);
		orgX = tempOrgX;
		orgY = tempOrgY;
		foundOne = TRUE;
	    }
	}
    }
    
/* Otherwise, operate on the top  and bottom 90 degree arcs: */
    else if (delY != 0)
    {	/* The delY value is more useful, so use y dimensions: */
	/* This section of code locates the closest, viable origin for <new>: */
	if(cornerTile != NULL)
	{
	    if (delY > 0)
	    {	/* Moving Up: */
		minX = anchY;
		x = (int)((float)delX/(float)delY * (cornerTile->y_pos - anchY)) + anchX;

		if (delX == 0) maxX = cornerTile->y_pos;

		else if (delX > 0)	/* either maxX = corner->y_pos or @ 
					   x = corner->x_pos */
		{
		    /* Moving up and to the right: */
		    if (x > cornerTile->x_pos)
		    {	
			/* intersection occurs along bottom edge of <cornerTile> */
			maxX = cornerTile->y_pos;
		    }
		    else
		    {	
			/* intersection occurs along left edge of <cornerTile> */
			maxX = (int)((float)delY/(float)delX * 
				     (cornerTile->x_pos - anchX)) + anchY;
		    }
		}
		else /* either maxX = corner->y_pos or @ x = corner->x_pos + 
			                                     corner->x_len        */
		{
		    /* Moving up and to the left: */
		    if (x > (cornerTile->x_pos + cornerTile->x_len))
		    {	
			/* intersection occurs along right edge of <cornerTile> */
			maxX = (int)((float)delY/(float)delX * 
				     (cornerTile->x_pos + cornerTile->x_len - anchX)) + 
				     anchY;
		    }
		    else
		    {	
			/* intersection occurs along bottom edge of <cornerTile> */
			maxX = cornerTile->y_pos;
		    }
		}
	    }
	    else
	    {	/* Moving Down: (delY < 0) */
		minX = anchY;
		x = (int)((float)delX/(float)delY * 
			  (cornerTile->y_pos + cornerTile->y_len - anchY)) + anchX;

		if (delX == 0) maxX = cornerTile->y_pos + cornerTile->y_len;

		else if (delX > 0)	/* either minY = corner->y_pos + corner->y_len 
					   or @ x=corner->x_pos 	*/
		{
		    /* Moving down and to the right: */
		    if (x > cornerTile->x_pos)
		    {	/* intersection occurs along top edge of <cornerTile> */
			maxX = cornerTile->y_pos + cornerTile->y_len;
		    }
		    else
		    {	/* intersection occurs along left edge of <cornerTile> */
			maxX = (int)((float)delY/(float)delX * 
				     (cornerTile->x_pos - anchX)) + anchY;
		    }
		}
		else /* either minY = corner->y_pos + corner->y_len or @ x = 
			corner->x_pos */
		{
		    if (x > (cornerTile->y_pos + cornerTile->y_len))
		    {	/* intersection occurs along right edge of <cornerTile> */
			maxX = (int)((float)delY/(float)delX * 
				     (cornerTile->x_pos + cornerTile->x_len - anchX)) + 
				     anchY;
		    }
		    else
		    {	
			/* intersection occurs along top edge of <cornerTile> */
			maxX = cornerTile->y_pos + cornerTile->y_len;
		    }
		}
	    }
	}
	else	/* cornerTile == NULL -- The simple case: */
	{
	    /* First find the corner tile on the edge of the given space in the 
	       direction that the new tile is to be moved:     			*/
	    if (delY > 0)
	    {	/* Moving Up: */
		cornerTile = find_ULHC(start);
		minX = anchY;
		maxX = cornerTile->y_pos + cornerTile->y_len;
	    }
	    else
	    {	/* Moving Down: */
		cornerTile = find_LRHC(start);
		maxX = cornerTile->y_pos;
		minX = anchY;
	    }
	}
	
       	/* Choose a first point to start at */
	epsilon = currentX = (minX + maxX) / 2;
	b = (anchX - (int)(((float)delX/(float)delY) * (float)anchY));
	
	/* Check the initial points to insure that they fall within the tile 
	   space defined */
	tempOrgY = currentX - pivY;
	tempOrgX = (int)(((float)delX/(float)delY) * (float)currentX) + b - pivX;
	if (locate(start, tempOrgX, tempOrgY) == NULL)		 /* This is a problem */
	{
	    epsilon = currentX = currentX / 2;				/* A guess. */
	    fprintf(stderr, "Bad origin point selected; trying again...\n");
	    
	}

	/* Perform a newtonian search (along the Y-Axis) to find the closest point 
	   to use:  Note that <currentX>, <maxX>, & <minX> are now misnamed.	*/
	while (abs(epsilon) > 1) 
	{
	    /* define the where <new>'s origin should go, based on the axis of motion: */
	    tempOrgY = currentX - pivY;
	    tempOrgX = (int)(((float)delX/(float)delY) * (float)currentX) + b - pivX;

	    /* See if <new> can be placed: */
	    flag = area_search(start, tempOrgX, tempOrgY, sizeX, sizeY);

	    if (flag != NULL)	
	    {	/* The tile can't go here. - try going further away */
		currentX = next_move(&minX, &maxX, currentX, FURTHER, &epsilon);
	    }
	    else 
	    {	/* The tile can go here - try going closer */
		currentX = next_move(&minX, &maxX, currentX, CLOSER, &epsilon);
		orgX = tempOrgX;
		orgY = tempOrgY;
		foundOne = TRUE;
	    }
	}
    }
    else 
    {
	fprintf(stderr, "angled_tile_help: NULL <delY>, <delX> values given.\n");
    }
    

    /* Given a corner point where to insert the new area, adding it to the new space: */
    new = (foundOne == TRUE) ? insert_tile(start, orgX, orgY, sizeX, sizeY, box) : NULL;

    /*Check to see that the insertion was successful: */
    if (new == NULL) ;/*fprintf(stderr, "angled_tile_help: failure to install tile\n");*/
    
    return(new);
}
/*------------------------------------------------------------------------ */
tile *angled_tile_insertion(start, anchX, anchY, delX, delY, pivX, pivY, sizeX, sizeY, box) 
    tile *start;   	/* the starting (reference) tile */
    int anchX, anchY;	/* XY origin of axis of motion (anchor point).	     */
    int delX, delY;	/* slope+dir of the axis of motion (line along which the 
			   new tile moves) */
    int pivX, pivY;	/* the pivot point of the <new> tile. (relative to (0,0), 
			   (sizeX, sizeY) */
    int sizeX, sizeY;	/* defines the size of the tile to be installed */
    int *box;		/* contents of the <new> tile.  Reference to (0,0). */
{    
/* This returns a pointer to a newly-created tile of the given size, at the
 * some position lying along a line given at an angle defined by <ydir>/<xdir>.
 * The position closest to the given <start> tile is selected.  The line is
 * drawn from the origin of <start> to the origin of the new tile.  */

    tilist *lineList = NULL, *tl;
    
    tile *cornerTile, *new = NULL;
    int x, y;
    int newAnchX = anchX, newAnchY = anchY;

    /* Make sure there is a useful axis of motion to use: */
    if ((delX == 0) && (delY == 0)) 
    {
	fprintf(stderr, "angled_tile_insertion: Bad Axis of motion given.\n");
	return(NULL);	/* Bad call */
    }    

    /* To locate the proper search space, map the axis of motion onto the tile-space:
     * this creates a list of tiles, starting with the anchor tile that will indicate
     * the limits of the search (maxX and minX) 
     * COMMENT:  This expansion should probably be more like a swath then a line.
     *  */
    lineList = find_all_tiles_along_line(start, newAnchX, newAnchY, delX, delY, &lineList);

    /* Find the next corner-tile candidate and advance the <lineList> pointer to indicate
       what tiles have been examined */
    cornerTile = first_noncontiguous_full_tile(&lineList);

    /* See if this <cornerTile> defines an area where the tile can go: */
    while (lineList != NULL)
    {
	new = angled_tile_help(start, newAnchX, newAnchY, delX, delY,
				     pivX, pivY, sizeX, sizeY, box, cornerTile);

	if (new != NULL) break; 	/* The thing fit! */
	else				/* The bloody thing didn't fit, so keep trying */
	{
	    /* reset the anchor-point and <cornerTile> and try again */
	    /* The new anchor point is some point in the current cornerTile that
	       lies along the axis of motion: */
	    if (delX != 0) 
	    {
		y = (int)((float)delY/(float)delX * (cornerTile->x_pos - newAnchX)) + newAnchY;
	    }
	    if (delY != 0)
	    {
		x = (int)((float)delX/(float)delY * (cornerTile->x_pos - newAnchY)) + newAnchX;
	    }
	    if ((y >= cornerTile->y_pos) && (y <= (cornerTile->y_pos + cornerTile->y_len)))
	    {
		newAnchY = y;
	    }
	    else newAnchY = cornerTile->y_pos;
	    
	    if ((x >= cornerTile->x_pos) && (x <= (cornerTile->x_pos + cornerTile->x_len)))
	    {
		newAnchX = x;
	    }
	    else newAnchY = cornerTile->x_pos;
	    
	    /* Find the next <cornerTile> off the unused portion of <lineList> */
	    cornerTile = first_noncontiguous_full_tile(&lineList);
	    
	    /*fprintf(stderr, "angled_tile_insertion: retrying at (%d, %d)\n",
	              newAnchX, newAnchY);*/
	}
    }

    if (new == NULL) new = angled_tile_help(start, newAnchX, newAnchY, delX, delY,
					    pivX, pivY, sizeX, sizeY, box, cornerTile);

    /*Check to see that the insertion was successful: */
    if (new == NULL) fprintf(stderr, "angled_tile_insertion: Complete Failure\n");
    
    return(new);
}

/*------------------------------------------------------------------------ */
tile *angled_group_tile_help(start, anchX, anchY, delX, delY, pivX, pivY, 
			     orgX, orgY, tlist, cornerTile) 
    tile *start;   	/* the starting (reference) tile */
    int anchX, anchY;	/* XY origin of axis of motion (anchor point) in <start>'s
			   tile space.  */
    int delX, delY;	/* slope+dir of the axis of motion (line along which the 
			   new tile moves) */
    int pivX, pivY;	/* the pivot point of the <new> tile set. (relative to (0,0)) */
    int *orgX, *orgY;   /* to return the solution found */
    tilist *tlist;	/* list of tiles to be inserted into <start>'s tile space */
    tile *cornerTile;   /* defines the end of the area to search in along the
			   axis of motion */
    
{    
/* This returns a pointer to a newly-created tile of the given size, at the
 * some position lying along a line given at an angle defined by <ydir>/<xdir>.
 * The position closest to the given <start> tile is selected.  The line is
 * drawn from the origin of <start> to the origin of the new tile.  */

/* NO ATTEMPT HAS BEEN MADE TO OPTIMIZE THIS CODE, so it is rather redundant. */

    tilist *tl;
    tile *new = NULL, *snew, *flag = NULL, *sflag;
    int epsilon, b, currentX, lastX, maxX, minX;
    int tempOrgX, tempOrgY;
    int x, y, foundOne = FALSE;

    /* This section of code locates the closest, viable origin for <new>: */

/* Operate on the left and right 90 degree arcs: */
    if ((delX != 0) && (abs(delX) > abs(delY)))
    {	    /* Given a useful delta-X value, move along the X-Axis: */

	/* Now if there is no such tile, reset corner based on the edge of the 
	   known universe: */
	if (cornerTile != NULL)
	{
	    /* There is a tile <cornerTile> lying along the line that may interfere: */
	    if (delX > 0)
	    {	/* Moving to the right: */
		minX = anchX;
		y = (int)((float)delY/(float)delX * (cornerTile->x_pos - anchX)) + anchY;

		if (delY == 0) maxX = cornerTile->x_pos;

		else if (delY > 0)	/* either maxX = corner->x_pos or @ y=corner->y_pos */
		{ 
		    /* Moving up and to the right: */
		    if (y < cornerTile->y_pos)
		    {	/* intersection occurs along bottom edge of <cornerTile> */
			maxX = (int)((float)delX/(float)delY * 
				     (cornerTile->y_pos - anchY)) + anchX;
		    }
		    else
		    {	/* intersection occurs along left edge of <cornerTile> */
			maxX = cornerTile->x_pos;
		    }
		}
		else /* either maxX = corner->x_pos or @ y=corner->y_pos + corner->y_len*/
		{
		    /* Moving down and to the right: */
		    if (y > (cornerTile->y_pos + cornerTile->y_len))
		    {	
			/* intersection occurs along top edge of <cornerTile> */
			maxX = (int)((float)delX/(float)delY * 
				     (cornerTile->y_pos + cornerTile->y_len - anchY)) + anchX;
		    }
		    else
		    {	
			/* intersection occurs along left edge of <cornerTile> */
			maxX = cornerTile->x_pos;
		    }
		}
	    }
	    else
	    {	/* Moving to the left: */
		minX = anchX;
		y = (int)((float)delY/(float)delX * 
			  (cornerTile->x_pos + cornerTile->x_len - anchX)) + anchY;

		if (delY == 0) maxX = cornerTile->x_pos + cornerTile->x_len;

		else if (delY > 0)	/* either minX = corner->x_pos + corner->x_len or 
					   @ y=corner->y_pos + corner->y_len */
		{
		    /* Moving up and to the left: */
		    if (y < (cornerTile->y_pos + cornerTile->y_len))
		    {	/* intersection occurs along bottom edge of <cornerTile> */
			maxX = (int)((float)delX/(float)delY * 
				     (cornerTile->y_pos - anchY)) + anchX;
		    }
		    else
		    {	/* intersection occurs along right edge of <cornerTile> */
			maxX = cornerTile->x_pos + cornerTile->x_len;
		    }
		}
		else /* either minX = corner->x_pos + corner->x_len or @ y=corner->y_pos */
		{
		    /* Moving down and to the left: */
		    if (y > (cornerTile->y_pos + cornerTile->y_len))
		    {	
			/* intersection occurs along top edge of <cornerTile> */
			maxX = (int)((float)delX/(float)delY * 
				     (cornerTile->y_pos + cornerTile->y_len - anchY)) + anchX;
		    }
		    else
		    {	/* intersection occurs along right edge of <cornerTile> */
			maxX = cornerTile->x_pos + cornerTile->x_len;
		    }
		}
	    }
	}
	else	/* cornerTile = NULL -- The simple case */
	{
	    /* First find the corner tile on the edge of the given space in the direction that
	       the new tile is to be moved:						*/    
	    if (delX > 0)
	    {	/* Moving to the right: */
		cornerTile = find_LRHC(start);
		minX = anchX;
		maxX = cornerTile->x_pos + cornerTile->x_len;
	    }
	    else
	    {	/* Moving to the left: */
		cornerTile = find_ULHC(start);
		maxX = cornerTile->x_pos;
		minX = anchX;
	    }
	}
	
       	/* Choose a first point to start at */
	epsilon = currentX = (minX + maxX) / 2;
	b = ( anchY - (int)((float)delY/(float)delX * (float)anchX) );

	/* Check the initial points to insure that they fall within the tile space defined */
	tempOrgX = currentX - pivX;
	tempOrgY = (int)(((float)delY/(float)delX) * (float)currentX) + b - pivY;
	if (locate(start, tempOrgX, tempOrgY) == NULL)	 		/* This is a problem */
	{
	    epsilon = currentX = currentX / 2;				/* A guess. */
	    fprintf(stderr, "Bad origin point selected; trying again...\n");
	}
	

	/* Perform a newtonian search (along the X-Axis) to find the closest point to use: */
	while (abs(epsilon) >= 1) 
	{
	    /* define the where <new>'s origin should go, based on the axis of motion: */
	    tempOrgX = currentX - pivX;
	    tempOrgY = (int)(((float)delY/(float)delX) * (float)currentX) + b - pivY;

	    /* See if <new> can be placed:   This involves checking everybody on the list: */
	    for (tl = tlist; ((tl != NULL) && (flag == NULL)); tl = tl->next)
	    {
		sflag = area_search(start, 
				   tempOrgX + tl->this->x_pos, tempOrgY + tl->this->y_pos, 
				   tl->this->x_len, tl->this->y_len);
		if (flag == NULL) flag = sflag;
	    }

	    if (flag != NULL)	
	    {	/* The tile set can't go here. - try going further away */
		currentX = next_move(&minX, &maxX, currentX, FURTHER, &epsilon);
	    }
	    else 
	    {	/* The tiles can go here - try going closer */
		currentX = next_move(&minX, &maxX, currentX, CLOSER, &epsilon);
		*orgX = tempOrgX;
		*orgY = tempOrgY;
		foundOne = TRUE;
	    }
	    flag = NULL;
	}
    }
    
/* Otherwise, operate on the top  and bottom 90 degree arcs: */
    else if (delY != 0)
    {	/* The delY value is more useful, so use y dimensions: */
	/* This section of code locates the closest, viable origin for <new>: */
	if(cornerTile != NULL)
	{
	    if (delY > 0)
	    {	/* Moving Up: */
		minX = anchY;
		x = (int)((float)delX/(float)delY * (cornerTile->y_pos - anchY)) + anchX;

		if (delX == 0) maxX = cornerTile->y_pos;

		else if (delX > 0)	/* either maxX = corner->y_pos or @ x=corner->x_pos */
		{
		    /* Moving up and to the right: */
		    if (x > cornerTile->x_pos)
		    {	
			/* intersection occurs along bottom edge of <cornerTile> */
			maxX = cornerTile->y_pos;
		    }
		    else
		    {	
			/* intersection occurs along left edge of <cornerTile> */
			maxX = (int)((float)delY/(float)delX * 
				     (cornerTile->x_pos - anchX)) + anchY;
		    }
		}
		else /* either maxX = corner->y_pos or @ x=corner->x_pos + corner->x_len*/
		{
		    /* Moving up and to the left: */
		    if (x > (cornerTile->x_pos + cornerTile->x_len))
		    {	
			/* intersection occurs along right edge of <cornerTile> */
			maxX = (int)((float)delY/(float)delX * 
				     (cornerTile->x_pos + cornerTile->x_len - anchX)) + anchY;
		    }
		    else
		    {	
			/* intersection occurs along bottom edge of <cornerTile> */
			maxX = cornerTile->y_pos;
		    }
		}
	    }
	    else
	    {	/* Moving Down: (delY < 0) */
		minX = anchY;
		x = (int)((float)delX/(float)delY * 
			  (cornerTile->y_pos + cornerTile->y_len - anchY)) + anchX;

		if (delX == 0) maxX = cornerTile->y_pos + cornerTile->y_len;

		else if (delX > 0)	/* either minY = corner->y_pos + corner->y_len or 
					   @ x=corner->x_pos */
		{
		    /* Moving down and to the right: */
		    if (x > cornerTile->x_pos)
		    {	/* intersection occurs along top edge of <cornerTile> */
			maxX = cornerTile->y_pos + cornerTile->y_len;
		    }
		    else
		    {	/* intersection occurs along left edge of <cornerTile> */
			maxX = (int)((float)delY/(float)delX * 
				     (cornerTile->x_pos - anchX)) + anchY;
		    }
		}
		else /* either minY = corner->y_pos + corner->y_len or @ x=corner->x_pos */
		{
		    if (x > (cornerTile->y_pos + cornerTile->y_len))
		    {	/* intersection occurs along right edge of <cornerTile> */
			maxX = (int)((float)delY/(float)delX * 
				     (cornerTile->x_pos + cornerTile->x_len - anchX)) + 
				     anchY;
		    }
		    else
		    {	
			/* intersection occurs along top edge of <cornerTile> */
			maxX = cornerTile->y_pos + cornerTile->y_len;
		    }
		}
	    }
	}
	else	/* cornerTile == NULL -- The simple case: */
	{
	    /* First find the corner tile on the edge of the given space in the 
	       direction that the new tile is to be moved:	       		*/	
	    if (delY > 0)
	    {	/* Moving Up: */
		cornerTile = find_ULHC(start);
		minX = anchY;
		maxX = cornerTile->y_pos + cornerTile->y_len;
	    }
	    else
	    {	/* Moving Down: */
		cornerTile = find_LRHC(start);
		maxX = cornerTile->y_pos;
		minX = anchY;
	    }
	}
	
       	/* Choose a first point to start at */
	epsilon = currentX = (minX + maxX) / 2;
	b = (anchX - (int)(((float)delX/(float)delY) * (float)anchY));
	
	/* Check the initial points to insure that they fall within the tile 
	   space defined */
	tempOrgY = currentX - pivY;
	tempOrgX = (int)(((float)delX/(float)delY) * (float)currentX) + b - pivX;
	if (locate(start, tempOrgX, tempOrgY) == NULL)	       /* This is a problem */
	{
	    epsilon = currentX = currentX / 2;				/* A guess. */
	    fprintf(stderr, "Bad origin point selected; trying again...\n");
	    
	}

	
	/* Perform a newtonian search (along the Y-Axis) to find the closest point 
	   to use:  Note that <currentX>, <maxX>, & <minX> are now misnamed.  */
	while (abs(epsilon) >= 1) 
	{
	    /* define the where <new>'s origin should go, based on the axis of motion: */
	    tempOrgY = currentX - pivY;
	    tempOrgX = (int)(((float)delX/(float)delY) * (float)currentX) + b - pivX;

	    /* See if <new> can be placed:   This involves checking everybody on 
	       the <tlist>: */
	    for (tl = tlist; ((tl != NULL) && (flag == NULL)); tl = tl->next)
	    {
		sflag = area_search(start, 
				   tempOrgX + tl->this->x_pos, tempOrgY + tl->this->y_pos, 
				   tl->this->x_len, tl->this->y_len);
		if (flag == NULL) flag = sflag;
	    }

	    if (flag != NULL)	
	    {	/* The tile set can't go here. - try going further away */
		currentX = next_move(&minX, &maxX, currentX, FURTHER, &epsilon);
	    }
	    else 
	    {	/* The tiles can go here - try going closer */
		currentX = next_move(&minX, &maxX, currentX, CLOSER, &epsilon);
		*orgX = tempOrgX;
		*orgY = tempOrgY;
		foundOne = TRUE;
	    }
	    flag = NULL;
	}
    }
    else 
    {
	fprintf(stderr, "angled_group_tile_help: NULL <delY>, <delX> values given.\n");
    }    

    /* Given a corner point where to insert the new area, add all of them to the new space: */
    for (tl = tlist; ((tl != NULL) && (foundOne == TRUE)); tl = tl->next)
    {
	new = insert_tile(start, *orgX + tl->this->x_pos, *orgY + tl->this->y_pos, 
			    tl->this->x_len, tl->this->y_len, tl->this->this); 
	if (new == NULL) foundOne = FALSE;
    }

    /*Check to see that the insertion was successful: */
    if (new == NULL) 
    {
	/* fprintf(stderr, "angled_group_tile_help: failure to install tile\n"); */
	return(NULL);
    }
    return(new);
}
/*------------------------------------------------------------------------ */
tile *angled_group_tile_insertion(start, anchX, anchY, delX, delY, pivX, pivY, 
				  orgX, orgY, tlist) 
    tile *start;   	/* the starting (reference) tile */
    int anchX, anchY;	/* XY origin of axis of motion (anchor point) in <start>'s 
			   tile space */
    int delX, delY;	/* slope+dir of the axis of motion (line along which the new 
			   tiles move) */
    int pivX, pivY;	/* the pivot point of the <new> tile set. (relative to (0,0)) */
    int *orgX, *orgY;   /* to return the solution found */
    tilist *tlist;	/* list of tiles to be inserted into <start>'s tile space */
{    
/* This returns a pointer to one of a set of newly-created tiles, created by adding
 * all of the tiles on <tlist> to the tile space defined by <start>.  They are added
 * at some position lying along a line given at an angle defined by <ydir>/<xdir>.
 * The position closest to the anchor point is selected. */

    tilist *lineList = NULL, *tl;
    
    tile *cornerTile, *new = NULL;
    int x, y;
    int newAnchX = anchX, newAnchY = anchY;

    /* Make sure there is a useful axis of motion to use: */
    if ((delX == 0) && (delY == 0)) 
    {
	fprintf(stderr, "angled_group_tile_insertion: Bad Axis of motion given.\n");
	return(NULL);	/* Bad call */
    }    

    /* To locate the proper search space, map the axis of motion onto the tile-space:
     * this creates a list of tiles, starting with the anchor tile that will indicate
     * the limits of the search (maxX and minX) 
     * COMMENT:  This expansion should probably be more like a swath then a line.
     *  */
    lineList = find_all_tiles_along_line(start, newAnchX, newAnchY, 
					 delX, delY, &lineList);

    /* Find the next corner-tile candidate and advance the <lineList> pointer to indicate
       what tiles have been examined */
    cornerTile = first_noncontiguous_full_tile(&lineList);

    /* See if this <cornerTile> defines an area where the tile can go: */
    while (lineList != NULL)
    {
	new = angled_group_tile_help(start, newAnchX, newAnchY, delX, delY,
				     pivX, pivY, orgX, orgY, tlist, cornerTile);

	if (new != NULL) break; 	/* The thing fit! */
	else				/* The bloody thing didn't fit, so keep trying */
	{
	    /* reset the anchor-point and <cornerTile> and try again */
	    /* The new anchor point is some point in the current cornerTile that
	       lies along the axis of motion: */
	    if (delX != 0) 
	    {
		y = (int)((float)delY/(float)delX * (cornerTile->x_pos - newAnchX)) + 
		          newAnchY;
	    }
	    if (delY != 0)
	    {
		x = (int)((float)delX/(float)delY * (cornerTile->x_pos - newAnchY)) + 
                         newAnchX;
	    }
	    if ((y >= cornerTile->y_pos)&&(y <= (cornerTile->y_pos + cornerTile->y_len)))
	    {
		newAnchY = y;
	    }
	    else newAnchY = cornerTile->y_pos;
	    
	    if ((x >= cornerTile->x_pos) && (x <= (cornerTile->x_pos + cornerTile->x_len)))
	    {
		newAnchX = x;
	    }
	    else newAnchX = cornerTile->x_pos;
	    
	    /* Find the next <cornerTile> off the unused portion of <lineList> */
	    cornerTile = first_noncontiguous_full_tile(&lineList);
	    
	    /*fprintf(stderr, "angled_group_tile_insertion: retrying at (%d, %d)\n", 
		    newAnchX, newAnchY); */
	}
    }

    if (new == NULL) new = angled_group_tile_help(start, newAnchX, newAnchY, delX, delY,
						  pivX, pivY, orgX, orgY, tlist, 
						  cornerTile);

    /*Check to see that the insertion was successful: */
    if (new == NULL) fprintf(stderr, "angled_group_tile_insertion: Complete Failure\n");
    
    return(new);
}

/*------------------------------------------------------------------------*/
tile *delete_tile(deadTile)
    tile *deadTile;
    /* This functions removes <t> from the set of tiles, performing all of 
     * the necessary mergers, etc.. The function returns an active pointer
     * to some tile in the space, often near the location of <deadTile>.  */
    /* NOTE:  This function depends upon the notion that the upper and right
     *        border of a tile are included in the tile, but not the lower &
     * 	      left sides.							*/
{	
    tile *temp1, *temp2, *scrub_right();
    tilist *tl, *tll, *rightList, *leftList, *scrap;
    
    deadTile->this = NULL;
    rightList = list_neighbors(deadTile, RIGHT);
    leftList = list_neighbors(deadTile, LEFT);
    
    for (tl = rightList; tl != NULL; tl = tl->next)
    {	/* Loop through <deadTile>'s right-hand neighbors */

	temp2 = scrub_right(deadTile, tl->this);
    }   

    for (tl = leftList; tl != NULL; tl = tl->next)
    {	/* Loop through the right-hand neighbors to <deadTile>'s left hand neighbors:*/

	temp1 = tl->this;
	scrap = list_neighbors(temp1, RIGHT);
	for (tll = scrap; tll != NULL; tll = tll->next)
	{
	    temp2 = scrub_right(temp1, tll->this);
	}
	free_list(scrap);
    }   

    /*    Do a Vmerge on the bottom */
    if (temp2->lb != NULL) return(vmerge_tile(temp2->lb));
    (*manage_contents__dead_tile)(deadTile);

    free_list(leftList);	free_list(rightList);
    return(temp2);
}

/*----------------------------------------------------------------------------*/
tile *scrub_right(left, right)
    tile *left, *right;

/* given two tiles <right> & <left>, split and merge them appropriately
   to insure that they maintain the horizontal-strip relationship  */
{
    int Ltop, Lbase, Rtop, Rbase;
    tile *left2, *right2;

    if (right == NULL) return(left);
    
    Rtop = right->y_pos + right->y_len;
    Rbase = right->y_pos;
    
    if (left != NULL)
    {						/* Not on a left edge */
	Ltop = left->y_pos + left->y_len;
	Lbase = left->y_pos;
	
	if (Ltop > Rtop) 		   /* split left @ y = Rtop */
	{
	    left2 = copy_tile(left);	   /* left2 becomes the lower tile */
	    left2->rt = left;
	    left2->y_len = Rtop - left2->y_pos;
	    
	    left->lb = left2;		   /* left becomes the upper tile */
	    left->y_pos = Rtop;
	    left->y_len -= left2->y_len;
	    left->tr = locate(right, left2->x_pos + left2->x_len, Rtop);
	    clean_up_neighbors(left);
	    clean_up_neighbors(left2);
	    
	    hmerge_tile(left);
	    vmerge_tile(left);
	    hmerge_tile(left2);
	    vmerge_tile(left2);
	    return(left2);
	}
	
	else if (Ltop < Rtop) 		  /* spilt right @ y = Ltop */
	{
	    right2 = copy_tile(right);	  /* right2 becomes the lower tile */
	    right2->rt = right;
	    right2->y_len = Ltop - right2->y_pos;
	    
	    right->lb = right2;	     	/* right becomes the upper tile */
	    right->bl = locate(left, right2->x_pos, Ltop);
	    right->y_pos = Ltop;
	    right->y_len -= right2->y_len;
	    
	    clean_up_neighbors(right);
	    clean_up_neighbors(right2);
	    
	    /* Look for other pieces above left to deal with */
	    hmerge_tile(left->rt);
	    vmerge_tile(left->rt);
	    hmerge_tile(left);
	    vmerge_tile(left);
	    return(left);
	}		
	
	if (Lbase < Rbase)			/* split left @ y = Rbase */
	{
	    left = locate(left, Rbase, Rtop); 	/* Redundant ? */
	    left2 = copy_tile(left);		/* left2 becomes the lower tile */
	    left2->rt = left;
	    left2->y_len = Rbase - left2->y_pos;
	    left2->tr = locate(right, left2->x_pos + left2->x_len, Rbase);
	    
	    left->lb = left2;	   /* left becomes the upper tile */
	    left->y_pos = Rbase;
	    left->y_len -= left2->y_len;
	    clean_up_neighbors(left);
	    clean_up_neighbors(left2);
	    
	    hmerge_tile(left);
	    vmerge_tile(left);
	    hmerge_tile(left2);
	    vmerge_tile(left2);
	    return (left2);
	}
	else 			   /* No splits required, just do mergers */
	{
	    hmerge_tile(left);
	    vmerge_tile(left);
	    return(left);
	}
    }
}
/*------------------------------------------------------------------------ */
int clean_up_neighbors(who)
    tile *who;
{	/* See that all of <who>'s neighbors point to him. */
    tilist *tl, *temp;
    
    temp = list_neighbors(who, LEFT);
    for (tl = temp; tl != NULL; tl = tl->next)   
        if (((tl->this->y_pos + tl->this->y_len) <= (who->y_pos + who->y_len)) && 
	    (tl->this != who)) tl->this->tr = who;
    free_list(temp);

    temp = list_neighbors(who, RIGHT);
    for (tl = temp; tl != NULL; tl = tl->next)  
        if ((tl->this->y_pos >= who->y_pos) && (tl->this != who))
	    tl->this->bl = who;
    free_list(temp);

    temp = list_neighbors(who, TOP);
    for (tl = temp; tl != NULL; tl = tl->next) 
        if ((tl->this->x_pos >= who->x_pos) && (tl->this != who))
	    tl->this->lb = who;
    free_list(temp);

    temp = list_neighbors(who, BOTTOM);
    for (tl = temp; tl != NULL; tl = tl->next) 
        if (((tl->this->x_pos + tl->this->x_len) <= (who->x_pos + who->x_len)) && 
	    (tl->this != who)) tl->this->rt = who;
    free_list(temp);
}

/*------------------------------------------------------------------------ */
tilist *translate_origin(tl, x, y)
    tilist *tl;
    int x, y;
{
    /*  Translate the location of all tiles on <tl> by the <x> & <y> given: */
    tilist *tll;
    for (tll = tl; tll != NULL; tll = tll->next)
    {
	tll->this->x_pos += x;
	tll->this->y_pos += y;
    }
    return(tl);
}
/*------------------------------------------------------------------------ */
int TileP(t)
    tile *t;
{
    return (TRUE);
}
/*------------------------------------------------------------------------ */
tilist *allspace(t, tl)
    tile *t;		/* Some tile in area to search */
    tilist **tl;	/* Where to list the located tiles */
{
    /*  Form a list of all tiles within the area containing <t>.*/
    return(enumerate(t, tl, TileP));
}
/*------------------------------------------------------------------------ */
int freeSpaceP(t)
    tile *t;
{
    return (((*solidp)(t) == FALSE) ? TRUE : FALSE);
}
/*------------------------------------------------------------------------ */
tilist *freespace(t, tl)
    tile *t;		/* Some tile in area to search */
    tilist **tl;	/* Where to list the empty tiles */
{
    /*  Form a list of all tiles within the area containing <t> that have
	no valid <this> entry: */
    return(enumerate(t, tl, freeSpaceP));
}

/*------------------------------------------------------------------------ */
int usedSpaceP(t)
    tile *t;
{
    return (((*solidp)(t) == FALSE) ? FALSE : TRUE);
}
/*------------------------------------------------------------------------ */
tilist *nonFreespace(t, tl)
    tile *t;		/* Some tile in area to search */
    tilist **tl;	/* Where to list the used tiles */
{
    /*  Form a list of all tiles within the area containing <t> that have
	no valid <this> entry: */
    return(enumerate(t, tl, usedSpaceP));
}
    
/*------------------------------------------------------------------------ */
tilist *helpAreaEnumerate(t, soFar, filterFn, xorg, yorg, maxX, maxY)
    tile *t;
    tilist **soFar;
    int (*filterFn)();	/* given a tile, returns TRUE if the tile is to be 
			   listed */
    int xorg, yorg;	/* Origin of area of interest (AOI) */
    int maxX, maxY;	/* Extent of area of interest */
    
    
{ /* Return a list containing all tiles to the right of the given 
     tile, within the prescribed area: */
    tilist *tl, *temp;
    int x = t->x_pos + t->x_len;
    int y = t->y_pos + t->y_len;

    if (debug_level == 5)
       fprintf(stderr, "t=0x%x (%d, %d)->(%d, %d) in (%d, %d)->(%d, %d) ? ",
		t, t->x_pos, t->y_pos, x, y, xorg, yorg, maxX, maxY);

    /* If <t> is in the specified range, add it to the listing: */
    if ((filterFn(t) != FALSE) && 
	((((x <= maxX) && (x > xorg)) || 
	  ((t->x_pos < maxX) && (t->x_pos >= xorg)) ||
	  ((x >= maxX) && (t->x_pos <= xorg))) 
	 &&
	 (((y <= maxY) && (y > yorg)) || 
	  ((t->y_pos < maxY) && (t->y_pos >= yorg)) ||
	  ((y >= maxY) && (t->y_pos <= yorg)))))
    {
	push(t, soFar);		/* Enumerate the little bugger */
        if (debug_level == 5)
	   fprintf(stderr, "yes!\n");
    }
    else
       if (debug_level == 5)
	  fprintf(stderr, "no.\n");
    

    if (t->tr != NULL)
    {	/* Enmumerate all of the tiles on the right-side of the given tile: */
	temp = list_neighbors(t, RIGHT);
	for (tl = temp; (tl != NULL) && (tl->this->x_pos < maxX); tl = tl->next) 
	{
	    if (tl->this->bl == t) 
	        helpAreaEnumerate(tl->this, soFar, filterFn, xorg, yorg, maxX, maxY);
	}
	free_list(temp);
    }
    return(*soFar);
}

/*------------------------------------------------------------------------ */
tilist *area_enumerate(t, soFar, filterFn, xorg, yorg, xext, yext)
    tile *t;
    tilist **soFar;
    int (*filterFn)();	/* given a tile, returns TRUE if the tile is to be 
			   listed */
    int xorg, yorg;	/* Origin of area of interest (AOI) */
    int xext, yext;	/* Extent of area of interest */
    
{ /* Return a list containing all tiles within the area defined by <t> 
     which return TRUE when passed to the <filterFn>.			 */
    tile *temp;
    int maxX = xorg + xext;
    int maxY = yorg + yext;

    /* Walk down the left edge of the AOI: */
    for (temp = locate(t, xorg, maxY); 
	 ((temp != NULL) && ((temp->y_pos >= yorg) || 
			     (temp->y_pos + temp->y_len > yorg)));
	 temp = locate(temp->lb, xorg, temp->y_pos))
    {
	*soFar = helpAreaEnumerate(temp, soFar, filterFn, xorg, yorg, maxX, maxY);
    }
    return(*soFar);
}
/*------------------------------------------------------------------------ */
tilist *helpEnumerate(t, soFar, filterFn)
    tile *t;
    tilist **soFar;
    int (*filterFn)();	/* given a tile, returns TRUE if the tile is to be 
			   listed */
    
{ /* Return a list containing all tiles to the right of the given tile, 
     which return TRUE when passed to the <filterFn>.		     */
    tilist *tl, *temp;

    /* Add the given tile to the listing: */
    if (filterFn(t) != FALSE) push(t, soFar);

    if (t->tr != NULL)
    {	/* Enmumerate all of the tiles on the right-side of the given tile: */
	temp = list_neighbors(t, RIGHT);
	for (tl = temp; tl != NULL; tl = tl->next) 
	{
	    if (tl->this->bl == t) helpEnumerate(tl->this, soFar, filterFn);
	}
	free_list(temp);
    }
    return(*soFar);
}

/*------------------------------------------------------------------------------- */
tilist *enumerate(t, soFar, filterFn)
    tile *t;
    tilist **soFar;
    int (*filterFn)();	/* given a tile, returns TRUE if the tile is to be listed */

{ /* Return a list containing all tiles within the area defined by <t> which
     return TRUE when passed to the <filterFn>.			 */
    tile *temp;
    for (temp = find_ULHC(t); temp != NULL; temp = temp->lb)
    {
	*soFar = helpEnumerate(temp, soFar, filterFn);
    }
    return(*soFar);
}

/*------------------------------------------------------------------------ */
tile *find_ULHC(t)
    tile *t;
{
    tile *temp = t;
    
    /* find the upper-left-hand corner of the given tile-set */
    if ((temp->bl == NULL) && (temp->rt == NULL)) return(temp);
    
    /* Walk up to the proper 'row':*/
    while (temp->rt != NULL) temp = temp->rt;

    /* Walk over to the proper 'column': */
    while (temp->bl != NULL) temp = temp->bl;

    return(find_ULHC(temp));
}
/*------------------------------------------------------------------------ */
tile *find_LRHC(t)
    tile *t;
{
    tile *temp = t;
    
    /* find the lower-right-hand corner of the given tile-set */
    if ((temp->lb == NULL) && (temp->tr == NULL)) return(temp);
    
    /* Walk up to the proper 'row':*/
    while (temp->lb != NULL) temp = temp->lb;

    /* Walk over to the proper 'column': */
    while (temp->tr != NULL) temp = temp->tr;

    return(find_LRHC(temp));
}
/*------------------------------------------------------------------------ */
tilist *find_all_tiles_along_line(start, srcX, srcY, delX, delY, soFar)
    tile *start;
    int srcX, srcY, delX, delY;
    tilist **soFar;
{
    /* This function walks down the given line, and adds each tile it encounters to 
       the list returned. */

    tile *temp = NULL, *neighbor;
    tilist *tl;		   /* Used to search through <soFar> to avoid replicate entry */
    
    int x, y, xx, yy;	   /* point on <temp>'s far edge that lies on the line */
    int bx, by;
    int yset = FALSE, xset = FALSE;
    int nextX, nextY;
    
    temp = locate(start, srcX, srcY);
    if (temp == NULL) return(*soFar);

    if (temp == start) 		       /* <srcX> and <srcY> are in <start> */
    {
	/* Recursion Stop?? */
	if ((temp->x_pos == srcX) || ((temp->x_pos + temp->x_len == srcX)))
	{
/*	  fprintf(stderr, 
	    "find_all_tiles_along_line: Hit edge of space along Y-dimension\n"); */
	  return(*soFar);
	}
	
	else if ((temp->y_pos == srcY) || 
		 ((temp->y_pos + temp->y_len == srcY)))
	{
/*	  fprintf(stderr, 
	    "find_all_tiles_along_line: Hit edge of space along X-dimension\n"); */
	  return(*soFar);
	}
/*	else
	{
	    fprintf(stderr, 
		    "find_all_tiles_along_line: Problem, am totally lost! \n");
	}
*/
    }

    /* Due to the strageness introduced by adding all tiles around a corner 
       that lies on a line, check to see if this tile is on the list already,
       so he isn't added twice.  Note that this means that only the last two
       entries on the list need be checked.  If <temp> is not on <soFar>, 
       then add him.  If so, don't add him. */

    for (tl = *soFar; ((tl != NULL) && (tl->next != NULL) && 
		       (tl->next->next == NULL));
	 tl = tl->next);

    /* Now at or near the end of <soFar> */
    if (tl == NULL) queue(temp, soFar);	        /* list_length(*soFar) = 0 */
    else if ((tl->next != NULL) && ((tl->this != temp) && (tl->next->this != temp)))
        queue(temp, soFar);			/* list_length(*soFar) = 2 */
    else if ((tl->this != temp) && (tl->next == NULL))  queue(temp, soFar);
                                                /* list_length(*soFar) = 1*/

    /* Find the points where the line intersects the edge of the tile: */
    if (delX != 0)	
    {
	xx = (delX < 0) ? temp->x_pos : temp->x_pos + temp->x_len;
	by = ( srcY - (int)((float)delY/(float)delX * (float)srcX) );
	y = (int)((float)delY/(float)delX * (float)xx) + by;
	nextY = (int)((float)delY/(float)delX * 
		      ((float)xx + (float)delX/abs(delX))) + by;
	if ((y <= (temp->y_pos + temp->y_len)) && (y >= temp->y_pos)) 
	    yset = TRUE;
	
    }
    if (delY != 0)
    {
	yy = (delY < 0) ? temp->y_pos : temp->y_pos + temp->y_len;
	bx = ( srcX - (int)((float)delX/(float)delY * (float)srcY) );
	x = (int)((float)delX/(float)delY * (float)yy) + bx;
	nextX = (int)((float)delX/(float)delY * 
		      ((float)yy + (float)delY/abs(delY))) + bx;
	if ((x <= (temp->x_pos + temp->x_len)) && (x >= temp->x_pos)) 
	    xset = TRUE;
    }

    /* now that these calculations are done, locate the edge intersection: */
    if ((yset == TRUE) && (xset == TRUE))	/* The point is a corner @ (x,y) */
    {
	y = yy;		/* This causes other problems */
	x = xx;
	if (delY > 0) 	/* Upper... */
	{
	    if (delX > 0)	/* Right-Hand Corner */
	    {
/* 		nextX = x + 1;		Might be needed....
		nextY = y + 1; */
		if (temp->tr != NULL) queue(temp->tr, soFar);
		if (temp->rt != NULL) queue(temp->rt, soFar);		
	    }
	    else		/* Left Hand Corner */
	    {
		neighbor = locate(temp, x - 1, y + 1);
		if (neighbor != NULL) queue(neighbor, soFar);  
		    /* tiles might get on <soFar>2x */
		neighbor = locate(temp, x - 1, y - 1);
		if (neighbor != NULL) queue(neighbor, soFar);
	    }
	}
	else	/* Lower... */
	{
	    if (delX > 0)	/* Right-Hand Corner */
	    {
		neighbor = locate(temp, x + 1, y - 1);
		if (neighbor != NULL) queue(neighbor, soFar);
		neighbor = locate(temp, x - 1, y - 1);
		if (neighbor != NULL) queue(neighbor, soFar);
	    }
	    else		/* Left Hand Corner */
	    {
		if (temp->lb != NULL) queue(temp->lb, soFar);
		if (temp->bl != NULL) queue(temp->bl, soFar);
	    }
	}	    
    }
    
    else if (yset == TRUE) 			/* The point is along the 
						   left/right side */
    {
	if (delX < 0) nextX = x = temp->x_pos; 		/* left side */
	else nextX = x = temp->x_pos + temp->x_len + 1;	/* right side */
    }
    else if (xset == TRUE)			/* The point is along the 
						   top/bottom edge */
    {
	if (delY < 0) nextY = y = temp->y_pos;		/* bottom */
	else nextY = y = temp->y_pos + temp->y_len + 1;	/* top */
    }
    else 	/* Problem... */
    {
	fprintf(stderr, "find_all_tiles_along_line: can't find anything! \n");
    }
    

    return(find_all_tiles_along_line(temp, nextX, nextY, delX, delY, soFar));
}

/*------------------------------------------------------------------------ */
tilist *find_all_tiles_along_swath(start, srcX, srcY, delX, delY, 
				   width, length, soFar)
    tile *start;		/* Tile in which to start looking */
    int srcX, srcY;		/* Center Point on line (in start?) */
    int delX, delY;		/* Slope of line */
    int width, length;          /* Width & Length of swath being checked */
    tilist **soFar;		/* Where to list the tiles found */

/* This function walks down the given line, and adds each tile it encounters to 
   the list returned.   A tile is encountered if it touches the given swath at
   any point. */
{
    int cx1, cx2, cx3, cx4, lxProj;
    int cy1, cy2, cy3, cy4, lyProj;
    float cosTheta, sinTheta, a = (float)width/2.0;
    /* the tricky part is to determine the corner points for the AOI given by the 
       swath: */
    
    if (delX == 0)	/* Handle the special cases: */
    {
	cx1 = srcX - (int)(a + 0.5);		cx3 = srcX + (int)(a + 0.5);
	cx2 = srcX - (int)(a + 0.5);		cx4 = srcX + (int)(a + 0.5);	
	cy1 = srcY;	    		cy4 = srcY;

	if (delY >= 0)
	{
	    cy2 = srcY + length;	cy3 = srcY + length;
	}
	else 
	{
	    cy2 = srcY - length;	cy3 = srcY - length;
	}
    }
    else if (delY == 0)
    {
	cy1 = srcY + (int)(a + 0.5);		cy2 = srcY + (int)(a + 0.5);
	cy3 = srcY - (int)(a + 0.5);		cy3 = srcY - (int)(a + 0.5);
	cx1 = srcX;			cx4 = srcX;
	if (delX >= 0)
	{
	    cx2 = srcX + length;	cx3 = srcX + length;
	}
	else 
	{
	    cx2 = srcX - length;	cx3 = srcX - length;
	}
    }

    /* Now the arctangent is defined, so use it: */
    else
    {
	cosTheta = (float) cos((double)atan((double)delY/(double)delX));
	sinTheta = (float) sin((double)atan((double)delY/(double)delX));
	lxProj = (int)(((double)length * cosTheta) + 0.5);
	lyProj = (int)(((double)length * sinTheta) + 0.5);
	cx1 = srcX + (int)((a * cosTheta) + 0.5);
	cx4 = srcX - (int)((a * cosTheta) + 0.5);
	cx2 = srcX + (int)((a * cosTheta) + 0.5) + lxProj;
	cx3 = srcX - (int)((a * cosTheta) + 0.5) + lxProj;

	cy1 = srcY + (int)((a * sinTheta) + 0.5);
	cy4 = srcY - (int)((a * sinTheta) + 0.5);
	cy2 = srcY + (int)((a * sinTheta) + 0.5) + lyProj;
	cy3 = srcY - (int)((a * sinTheta) + 0.5) + lyProj;
    }

    /* INCOMPLETE>>>>>> */
	return(NULL);
}
/*------------------------------------------------------------------------ */
/*------------------------------------------------------------------------ */
tile *first_noncontiguous_full_tile(tlist)
    tilist **tlist;
{   /* Return the first non-empty ((*solidp)) tile on the given <tlist> */
    /* <tlist> had best be an ordered list. */

    tilist *tl;
    int returnNextFullTile = FALSE;
    
    for (tl = *tlist; (tl != NULL); tl = tl->next) 
    {
	if ((returnNextFullTile == TRUE) && ((*solidp)(tl->this) == TRUE))
	{
	    *tlist = tl;
	    return(tl->this);
	}
	else if (((*solidp)(tl->this) == FALSE) && (tl->next != NULL)) 
	    returnNextFullTile = TRUE;
    }
    *tlist = tl;
    return(NULL);
}
	
/*---------------------------------------------------------------
 * Space statistics:
 *---------------------------------------------------------------
 */
/*------------------------------------------------------------------------ */
tile *best_tile_in_space(t, filterFn)
    tile *t;			/* some tile in the desired tile space */
    int (*filterFn)();		/* returns TRUE if <t1> is better than <t2>. */
    
{
    /* This returns the best filled tile in the given tile space. */

    tilist *tl = NULL;
    tile *temp;

    tl = nonFreespace(t, &tl);
    temp = tl->this;
    
    for (tl = tl->next; (tl != NULL); tl = tl->next)
    {
	temp = (filterFn(temp, tl->this) != FALSE) ? temp : tl->this;
    }
    return(temp);
}

/*------------------------------------------------------------------------ */
int adjacent_tiles_p(t1, t2)
    tile *t1, *t2;  
{
    /* Return TRUE iff t1 & t2 share a common edge */
    if ((t1 == NULL) || (t2 == NULL))			return(TRUE);
    else if ((t1->x_pos == t2->x_pos + t2->x_len) ||
	     (t2->x_pos == t1->x_pos + t1->x_len) ||
	     (t1->y_pos == t2->y_pos + t2->y_len) ||
	     (t2->y_pos == t1->y_pos + t1->y_len))	return(TRUE);
    else 						return(FALSE);
}
/*------------------------------------------------------------------------ */
int most_right(t1, t2)
    tile *t1, *t2;
{
    /* given two tiles, return TRUE if the right-edge of <t1> is further to 
       the right than the right edge of <t2>:	*/
    if ((t1->x_pos + t1->x_len) > (t2->x_pos + t2->x_len)) return(TRUE);
    else return(FALSE);
}

/*------------------------------------------------------------------------ */
int most_left(t1, t2)
    tile *t1, *t2;
{
    /* given two tiles, return TRUE if the left edge of <t1> is further to 
       the left than the left edge of <t2>:	*/
    if (t1->x_pos < t2->x_pos) return (TRUE);
    else return(FALSE);
}

/*------------------------------------------------------------------------ */
int most_up(t1, t2)
    tile *t1, *t2;
{
    /* given two tiles, return TRUE if the top-edge of <t1> is further up 
       than the top edge of <t2>:	*/
    if ((t1->y_pos + t1->y_len) > (t2->y_pos + t2->y_len)) return (TRUE);
    else return(FALSE);
}

/*------------------------------------------------------------------------ */
int most_down(t1, t2)
    tile *t1, *t2;
{
    /* given two tiles, return TRUE if the bottom-edge of <t1> is further 
       down than the bottom edge of <t2>:	*/
    if (t1->y_pos < t2->y_pos) return (TRUE);
    else return(FALSE);
}

/*------------------------------------------------------------------------ */
int verify_tile_space(t)
    tile *t;
{
    /* This checks to see if the tile space is corrupted: */
    tilist *tl = NULL, *tll = NULL, *errorList = NULL;
    tile *tp;
    int error = FALSE;
    
    if (t == NULL) return(TRUE);	/* Let someone else catch this error */

    /* First check the given tile... */
    tp = t;
    if(((tp->rt != NULL) && (tp->rt->y_pos != (tp->y_pos + tp->y_len))) ||
       ((tp->rt == NULL) && (tp->tr != NULL) && 
	((tp->tr->y_pos + tp->tr->y_len) > tp->y_pos + tp->y_len)) ||
       ((tp->rt != NULL) && ((tp->x_pos + tp->x_len) <= tp->rt->x_pos)) ||
       ((tp->rt != NULL) && ((tp->x_pos + tp->x_len) > 
			     (tp->rt->x_pos + tp->rt->x_len))))
    
    {	/* the top tile/tile pointer is screwed */
	error = TRUE;
	push(tp, &errorList);
    }
    else if (((tp->tr != NULL) && (tp->tr->x_pos != (tp->x_pos + tp->x_len))) ||
	     ((tp->tr == NULL) && (tp->rt != NULL) && 
	      ((tp->rt->x_pos + tp->rt->x_len) > tp->x_pos + tp->x_len)) ||
	     ((tp->tr != NULL) && ((tp->y_pos + tp->y_len) <= tp->tr->y_pos)) ||
	     ((tp->tr != NULL) && ((tp->y_pos + tp->y_len) > 
				   (tp->tr->y_pos + tp->tr->y_len))))
    {	/* the right tile/tile pointer is screwed */ 
	error = TRUE;
	push(tp, &errorList);
    }
    else if (((tp->lb != NULL) && ((tp->lb->y_pos + tp->lb->y_len) != tp->y_pos)) ||
	     ((tp->lb == NULL) && (tp->bl != NULL) && (tp->bl->y_pos < tp->y_pos)) ||
	     ((tp->lb != NULL) && (tp->x_pos < tp->lb->x_pos)) ||
	     ((tp->lb != NULL) && (tp->x_pos >= (tp->lb->x_pos + tp->lb->x_len))))
    {	/* the bottom tile/tile pointer is screwed */
	error = TRUE;
	push(tp, &errorList);
    }
    else if (((tp->bl != NULL) && ((tp->bl->x_pos + tp->bl->x_len) != tp->x_pos)) ||
	     ((tp->bl == NULL) && (tp->lb != NULL) && (tp->lb->x_pos < tp->x_pos)) ||
	     ((tp->bl != NULL) && (tp->y_pos < tp->bl->y_pos)) ||
	     ((tp->bl != NULL) && (tp->y_pos >= (tp->bl->y_pos + tp->bl->y_len))))
    {	/* The left tile/tile pointer is screwed */
	error = TRUE;
	push(tp, &errorList);
    }

    /* Then check the whole tile space */
    tl = allspace(t, &tl);
    for (tll = tl; (tll != NULL); tll = tll->next)
    {
	tp = tll->this;
	if(((tp->rt != NULL) && (tp->rt->y_pos != (tp->y_pos + tp->y_len))) ||
	   ((tp->rt == NULL) && (tp->tr != NULL) && 
	    ((tp->tr->y_pos + tp->tr->y_len) > tp->y_pos + tp->y_len)) ||
	   ((tp->rt != NULL) && ((tp->x_pos + tp->x_len) <= tp->rt->x_pos)) ||
	   ((tp->rt != NULL) && ((tp->x_pos + tp->x_len) > 
				 (tp->rt->x_pos + tp->rt->x_len))))

	{	/* the top tile/tile pointer is screwed */
	    error = TRUE;
	    push(tp, &errorList);
	}
	else if (((tp->tr != NULL) && (tp->tr->x_pos != (tp->x_pos + tp->x_len))) ||
		 ((tp->tr == NULL) && (tp->rt != NULL) && 
		  ((tp->rt->x_pos + tp->rt->x_len) > tp->x_pos + tp->x_len)) ||
		 ((tp->tr != NULL) && ((tp->y_pos + tp->y_len) <= tp->tr->y_pos)) ||
		 ((tp->tr != NULL) && ((tp->y_pos + tp->y_len) > 
		                       (tp->tr->y_pos + tp->tr->y_len))))
	{	/* the right tile/tile pointer is screwed */ 
	    error = TRUE;
	    push(tp, &errorList);
	}
	else if (((tp->lb != NULL) && ((tp->lb->y_pos + tp->lb->y_len) != tp->y_pos)) ||
		 ((tp->lb == NULL) && (tp->bl != NULL) && (tp->bl->y_pos < tp->y_pos)) ||
		 ((tp->lb != NULL) && (tp->x_pos < tp->lb->x_pos)) ||
		 ((tp->lb != NULL) && (tp->x_pos >= (tp->lb->x_pos + tp->lb->x_len))))
	{	/* the bottom tile/tile pointer is screwed */
	    error = TRUE;
	    push(tp, &errorList);
	}
	else if (((tp->bl != NULL) && ((tp->bl->x_pos + tp->bl->x_len) != tp->x_pos)) ||
		 ((tp->bl == NULL) && (tp->lb != NULL) && (tp->lb->x_pos < tp->x_pos)) ||
		 ((tp->bl != NULL) && (tp->y_pos < tp->bl->y_pos)) ||
		 ((tp->bl != NULL) && (tp->y_pos >= (tp->bl->y_pos + tp->bl->y_len))))
	{	/* The left tile/tile pointer is screwed */
	    error = TRUE;
	    push(tp, &errorList);
	}
    }
    if (error == FALSE) return(TRUE);
    return(FALSE);
}

/*------------------------------------------------------------------------ */
void ps_print_tile_edges(f, t)
    FILE *f;		/* Where to write things */
    tile *t;		/* The tile to be printed */
{
    /* This function prints a dotted line that corresponds to the border of the
       tile in question.  Normally, print only the left and top borders of the 
       tile, and let the other tiles fill in the bottom and right sides.  This
       leaves the special cases, where the tile is on the edge of the tilespace.  */

    int x1, y1, x2, y2;
    
    if (t != NULL)
    {
	x1 = t->x_pos;		y1 = t->y_pos;
	x2 = x1 + t->x_len;	y2 = y1 + t->y_len;
	
        fprintf(f,"%%%% Tile and contents for %x LLHC(%d,%d), URHC=(%d,%d) %%%%\n", 
		t, x1, y1, x2, y2);
        fprintf(f,"[1 2] 0.2 setdash\n");
	fprintf(f, "newpath %d %d moveto %d %d lineto ", x1, y1, x1, y2); /* left edge */
	fprintf(f, "%d %d lineto stroke \n", x2, y2);			  /* top edge */

	if (t->tr == NULL)
	{
	    fprintf(f, "newpath %d %d moveto %d %d lineto stroke\n", x2, y2, x2, y1);
	}

	if (t->lb == NULL)
	{
	    fprintf(f, "newpath %d %d moveto %d %d lineto stroke\n", x1, y1, x2, y1);
	}
	fprintf(f, "[] 0 setdash\n");
    }
}
/*------------------------------------------------------------------------ */
/*------------------------------------------------------------------------ */
void enumerateForPrinting(t, f, printFn, edgesP)
    tile *t;
    FILE *f;
    int (*printFn)();	/* given a tile, produces POSTSCRIPT output to file <f>*/
    int edgesP;		/* Flag to indicate edge printing */
    
{ /* Return a list containing all tiles to the right of the given tile, which return
   * TRUE when passed to the <filterFn>.					 */
    tilist *tl, *temp;

    /* Print the given tile to the listing: */
    if(edgesP == TRUE) ps_print_tile_edges(f, t);
    
    if ((*solidp)(t) == TRUE) printFn(f,t);

    /* Print the remaining tiles in the current 'row': */
    if (t->tr != NULL)
    {	/* Enmumerate all of the tiles on the right-side of the given tile: */
	temp = list_neighbors(t, RIGHT);
	for (tl = temp; tl != NULL; tl = tl->next) 
	{
	    if (tl->this->bl == t) enumerateForPrinting(tl->this, f, printFn, edgesP);
	}	
	free_list(temp);
    }
}
/*------------------------------------------------------------------------ */
void ps_print_tile_space(t, f, ps_printFn, edgesP)
    tile *t;
    FILE *f;		 /* Where to print the stuff to */
    int (*ps_printFn)(); /* given a file and a tile, print post-script code to represent
			  * the contents of the tile.  Should be able to handle NULL. */
    int edgesP;		 /* Flag that indicates the inclusion of the tile edges */
    
{   tile *temp;
    
    for (temp = find_ULHC(t); temp != NULL; temp = temp->lb)
    {
	enumerateForPrinting(temp, f, ps_printFn, edgesP);
    }
}
/*------------------------------------------------------------------------ */
/*---------------------------------------------------------------
 * END OF FILE
 *---------------------------------------------------------------
 */
