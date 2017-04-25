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


/*  
 *	file local_route.c
 * As part of MS Thesis Work, S T Frezza, UPitt Engineering
 * February, 1990 - Major revision July, 1990 - Tuning for incremental route - Jan, 1991
 *
 * This file contains the global and local routing functions associated with the
 * SPAR schematics placer.  It is based on the data-structures created by the 
 * parser (parse.c) and the placer (place.c), and is thus dependent upon:	 */

#include <stdio.h>
#include <time.h>	// For ctime()
#include <stdlib.h>	// For getenv()
#include "route.h"

/* It is expected that all modules have been placed, and that global routes have been 
 * completed.  A global route consists of a list of completed trails, as set in
 * "terminate_expansion" in global_route.c, and are stored in the (common) trailist
 * ->connections in each of the expansions that are listed in the ->expns list
 * given in the net structure.  
 *
 * Local Routing:
 * Once all nets have arcs assigned to them, (The result of the global routing process) 
 * then the task remains to determine the corner points for jogs, angles, etc..  This 
 * is a process of routing within boxes, where some items are constrained, others are 
 * not, and some become constrained.  For example, a net that turns a corner within a 
 * box is not completely constrained until the other corner of the net is reached.
 *
 * This is done by mapping each arc containing nets into a set of `constraints'.  
 * These constraints are structures that represent where corners are to be placed.
 * They are linked by using common range structures, and are resolved when
 * the ranges are reduced to single points.  It is this resolution process that 
 * determines how the nets are ordered within a given area.  Each constraint structure 
 * is then complete, and is individually mapped into the segments that are printed on 
 * the page.
 *
 * The constraint resolution process is not trivial, and is somewhat interesting in its
 * own right.  It is a heuristic process of seperating overlapping ranges such that the
 * corners and connections within each box are nicely placed.
 */


/* ---- Local Variables ---------------------------------------------------------------*/
#define TRIED -2

#define X_SHIFT -50

#define Y_SHIFT -50

extern char net_under_study[];

extern net *Net;		/* Used as a temporary global */

extern FILE *df, *vf, *hf;     /* Where to dump the global route trace/debug info */

extern int currentPage;		/* The current page on which insertions are being made */

static srange spanRange;
/* static int idx = 0; */	/* Used for printing edge visitation order */

int *levelMap;		 	/* Array used to trace the dependency depth of ranges */
rnglist **reverseDepSets;	/* Array used to trace reverse dependency (break cyles)*/

ilist *congestion = NULL;	/* Indexed list of the tiles w/ worst congestion */
ilist *crossovers = NULL;	/* Indexed list of the tiles w/ worst crossover */


/* --- forward reference -------------------------------------------------------------:*/
void write_local_route_to_file(FILE *f, nlist *nets);
void xc_write_local_route(XCWindowData *areastruct, nlist *nets);
void print_tile_spaces();

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
correct_overlap(a, aLen, b, bLen, beg, NewLen)	/* Not Used */
    int a, aLen, b, bLen, *beg, *NewLen;
{
    int End;		/* Wouldn't compile with "end" - something about type problems */
    
    /* Don't ask - takes two line segments, as determined by an axis point and length pair,
     * and resolves the overlap between the two of them [(a, aLen) and (b, bLen)]  */
    if ((a + aLen) >= (b + bLen)) End = b + bLen;
    else End = a + aLen;
    
    if (a >= b) *beg = a;
    else *beg = b;

    *NewLen = End - *beg;
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
/* Local Routing Stuff:							       	   */
/*---------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/
int determine_entry_side(Parc, Carc, refX, refY)
    asg_arc *Parc, *Carc;
    int refX, refY;	/* Reference point in HORZ coordinates */
{
    /* This determines the side of entry when moving from <Parc> to <Carc>. */
    int side;

    if (Parc->page == VERT)
    {
	/* Check to see if the reference point is valid (contained in Parc): */
	if ((refX < Parc->t->y_pos) ||
	    (refX > Parc->t->y_pos + Parc->t->y_len) ||
	    (refY < Parc->t->x_pos) || 
	    (refY > Parc->t->x_pos + Parc->t->x_len))
	{
	    fprintf(stderr, "determine_entry_side : Bad reference point(%d, %d)\n",
		    refX, refY);
	    return(FALSE);
	}

	else if (Carc->page == VERT)
	{
	    if (refX <= Carc->t->y_pos) return(LEFT);
	    else return(RIGHT);
	}
	
	else if (Carc->page == HORZ)
	{
	    if (refY > Carc->t->y_pos) return(UP);
	    else return(DOWN);
	}
    }

    else /* Parc->page == HORZ */
    {
	/* Check to see if the reference point is valid (contained in Parc): */
	if ((refX < Parc->t->x_pos) ||
	    (refX > Parc->t->x_pos + Parc->t->x_len) ||
	    (refY < Parc->t->y_pos) || 
	    (refY > Parc->t->y_pos + Parc->t->y_len))
	{
	    fprintf(stderr, "determine_entry_side : Bad reference point(%d, %d)\n",
		    refX, refY);
	    return(FALSE);
	}

	else if (Carc->page == VERT)
	{
	    if (refX <= Carc->t->y_pos) return(LEFT);
	    else return(RIGHT);
	}
	
	else if (Carc->page == HORZ) /* may want to look at y_pos + y_len... */
	{
	    if (refY > Carc->t->y_pos) return(UP);
	    else return(DOWN);
	}
    }	
}
/*-------------------------------------------------------------------------------------*/
int find_intersection(x1, x2, X1, X2, res1, res2)
    int x1, x2, X1, X2;
    int *res1, *res2;
{
    /* this resolves the intersection between two ranges.  It does funny things 
       if they are non-overlapping ranges. */

    int xEndsFirst = (int)(x2 < X2);
    int xStartsFirst = (int)(x1 < X1);

    if ((x2 < X1) || (x1 > X2))
    {
	fprintf(stderr, "find_intersection - NON-OVERLAPPING! (%d, %d) vs (%d, %d)\n",
	    x1, x2, X1, X2);
	if (xEndsFirst && xStartsFirst)
	{ *res1 = X1;	*res2 = x2; }
	else 
	{ *res1 = x1; 	*res2 = X2; }

	return(FALSE);
    }
    if (xEndsFirst) *res2 = x2;	
    else            *res2 = X2; 

    if (xStartsFirst) *res1 = X1;
    else  	      *res1 = x1;	

    return(TRUE);
}
/*-------------------------------------------------------------------------------------*/
range *create_range(srg, n)
    srange *srg;
    net *n;
{
    /* This function serves the significant purpose of updating the tile/arc structure
       to include the <srg> and what side it occurs on.  This information is terribly
       useful  for figuring out how to resolve overlapping ranges. */
    range *r = (range *)calloc(1, sizeof(range));
    r->sr = srg;
    r->n = n;
    r->corners = NULL;
    r->use = r->flag = NULL;

    return(r);
}
/*-------------------------------------------------------------------------------------*/
range *quick_copy_range(oldR)
    range *oldR;
{
    /* Make a new copy of <oldR> */
    range *r = (range *)calloc(1, sizeof(range));
    r->sr = oldR->sr;
    r->n = oldR->n;
    r->corners = oldR->corners;			/* Not quite right, but...*/
    r->use = oldR->use;
    r->flag = oldR->flag;

    return(r);
}
/*-------------------------------------------------------------------------------------*/
range *copy_range(oldR)
    range *oldR;
{
    /* Make a new copy of <oldR> */
    range *r = (range *)calloc(1, sizeof(range));
    r->sr = create_srange(oldR->sr->q1, oldR->sr->q2, oldR->sr->orient);
    r->n = oldR->n;
    r->corners = NULL;
    r->use = oldR->use;
    r->flag = oldR->flag;

    return(r);
}
/*-------------------------------------------------------------------------------------*/
int some_overlap_p(a1, a2, b1, b2)
    int a1, a2, b1, b2;
{
    if ((a1 <= b2) && (a2 >= b1)) return(TRUE);	
    else return(FALSE);
}
/*-------------------------------------------------------------------------------------*/
int midpoint(range *rng)
{
    return(  (rng->sr->q1 + rng->sr->q2 + 1) / 2 );
}
/* NOTE: The "+1" is a hack to insure that narrow tiles that are saught via some
   spanning range (See replace_corner_instance for an example) can be located
   (has to do with the fact that the left edge of a tile does not 'locate' it, 
   whereas the right edge does - the SLIVER bug bites again!)	*/
/*-------------------------------------------------------------------------------------*/
int corner_falls_in_arc_p(c, a)
    corner *c;
    asg_arc *a;
{
    int b1 = a->t->y_pos, b2 = a->t->y_pos + a->t->y_len;
    int d1 = a->t->x_pos, d2 = a->t->x_pos + a->t->x_len;

    if ((a->page == HORZ) && 
	((some_overlap_p(b1, b2, c->cy->sr->q1,  c->cy->sr->q2) == FALSE) ||
	 (d1 > c->cx->sr->q1) || (d2 < c->cx->sr->q2)))
    {
	return(FALSE);
    }
    else if ((a->page == VERT) && 
	((some_overlap_p(b1, b2, c->cx->sr->q1,  c->cx->sr->q2) == FALSE) ||
	 (d1 > c->cy->sr->q1) || (d2 < c->cy->sr->q2)))
    {
	return(FALSE);
    }
    else return (TRUE);
}

/*-------------------------------------------------------------------------------------*/
int add_corner_to_arc(c, a)
    corner *c;
    asg_arc *a;
{
    int b1 = a->t->y_pos, b2 = a->t->y_pos + a->t->y_len;
    int d1 = a->t->x_pos, d2 = a->t->x_pos + a->t->x_len;

    if ((corner_falls_in_arc_p(c,a) == FALSE) || (a->mod != NULL))
    {
	if (a->mod != NULL)
	{
	    fprintf(stderr, "Attempt to add corner of net %s to module %s rejected!!\n",
		    c->cx->n->name, a->mod->name);
	}
	if ((a->page == HORZ) && (debug_level > 0))
	{
	    fprintf(stderr, "([%d,%d],[%d,%d])Corner addition screwed...HORZ\n",
		    c->cx->sr->q1, c->cx->sr->q2, c->cy->sr->q1, c->cy->sr->q2);
	}
	else if ((a->page == VERT) && (debug_level > 0))
	{
	    fprintf(stderr, "([%d,%d],[%d,%d])Corner addition screwed...VERT\n",
		    c->cx->sr->q1, c->cx->sr->q2, c->cy->sr->q1, c->cy->sr->q2);
	}
	return(FALSE);
    }
    else
    {
	pushnew(a, &c->al);	
	pushnew(c, &a->corners);
	pushnew(c->cx->n, &a->nets);
	return(TRUE);
    }
}
/*-------------------------------------------------------------------------------------*/
int pull_corner_from_arc(c, a)
    corner *c;
    asg_arc *a;
{
    int b1 = a->t->y_pos, b2 = a->t->y_pos + a->t->y_len;
    int d1 = a->t->x_pos, d2 = a->t->x_pos + a->t->x_len;

    if (corner_falls_in_arc_p(c,a) == TRUE)
    {
	if ((a->page == HORZ) && (debug_level > 0))
	{
	    fprintf(stderr, "([%d,%d],[%d,%d])Corner removal screwed...HORZ\n",
		    c->cx->sr->q1, c->cx->sr->q2, c->cy->sr->q1, c->cy->sr->q2);
	}
	else if ((a->page == VERT) && (debug_level > 0))
	{
	    fprintf(stderr, "([%d,%d],[%d,%d])Corner removal screwed...VERT\n",
		    c->cx->sr->q1, c->cx->sr->q2, c->cy->sr->q1, c->cy->sr->q2);
	}
	return(FALSE);
    }
    else
    {
	rem_item(a, &c->al);	
	rem_item(c, &a->corners);
        rem_item(c->cx->n, &a->nets);
	return(TRUE);
    }
}
/*-------------------------------------------------------------------------------------*/
corner *create_corner(rX, rY, a, ter)
    range *rX, *rY;
    asg_arc *a;
    term *ter;
{
    tile *t;

    corner *c = (corner *)calloc(1, sizeof(corner));
    c->cx = rX;
    c->cy = rY;
    c->vertOrder = c->horzOrder = 5000;
    c->al = NULL;
    c->t = ter;		/* Specialized use */

    if (a != NULL) add_corner_to_arc(c, a);  /* Add to the arc we expect <c> to be in. */

    pushnew(c, &rX->corners);   
    pushnew(c, &rY->corners);
    return(c);
}
/*-------------------------------------------------------------------------------------*/
corner *set_corner_point(n, curTile, prevTile, xlink, ylink, refX, refY)
    net *n;
    tile *curTile, *prevTile;
    range *xlink, *ylink;	/* Typically only one is set. */
    int refX, refY;
{
    /* This procedure analyzes how <curTile> overlaps with <prevTile>, and sets up 
       the ranges for the placement of the corner points (<cornerX>, <cornerY>).
       If either link is set, it is tied in. */

    /* The X and Y designations are relative to the HORZ page set. */

    range *cornerX, *cornerY;
    asg_arc *Carc = (asg_arc *)curTile->this, *Parc = (asg_arc *)prevTile->this;
    int Cpage = Carc->page, Ppage = Parc->page;
    int q1, q2, q3, q4, side;
    
    side = determine_entry_side(Parc, Carc, refX, refY);

    if (ylink != NULL) 
    {	
	if (xlink != NULL)
	{
	    /* odd use... */
	    cornerX = xlink;
	    cornerY = ylink;
	    return(create_corner(cornerX, cornerY, Carc, NULL));
	}
	
	else /* Use the <ylink>, calculate the <cornerX>: */
	{
	    if ((Cpage == HORZ) && (Ppage == VERT))
	    {
		find_intersection(curTile->x_pos, curTile->x_pos + curTile->x_len,
				  prevTile->y_pos, prevTile->y_pos + prevTile->y_len,
				  &q1, &q2);
	    }
	    else if ((Cpage == VERT) && (Ppage == HORZ))
	    {
		find_intersection(curTile->y_pos, curTile->y_pos + curTile->y_len,
				  prevTile->x_pos, prevTile->x_pos + prevTile->x_len,
				  &q1, &q2);
	    }
	    else if (Cpage == HORZ)
	    {
		find_intersection(curTile->x_pos, curTile->x_pos + curTile->x_len,
			 	  prevTile->x_pos, prevTile->x_pos + prevTile->x_len,
				  &q1, &q2);
	    }
	    else /* (Cpage == VERT) */
	    {
		find_intersection(curTile->y_pos, curTile->y_pos + curTile->y_len,
				  prevTile->y_pos, prevTile->y_pos + prevTile->y_len,
				  &q1, &q2);
	    }
	    cornerX = create_range(create_srange(q1,q2,Cpage), n);
	    cornerY = ylink;
	    return(create_corner(cornerX, cornerY, Carc, NULL));
	}      
    }

    else if (xlink != NULL)
    { 
	/* Use the <xlink>, calculate the <cornerY>: */
	if ((Cpage == VERT) && (Ppage == HORZ))
	{
	    find_intersection(curTile->x_pos, curTile->x_pos + curTile->x_len,
			      prevTile->y_pos, prevTile->y_pos + prevTile->y_len,
			      &q3, &q4);
	}
	else if ((Cpage == HORZ) && (Ppage == VERT))
	{
	    find_intersection(curTile->y_pos, curTile->y_pos + curTile->y_len,
			      prevTile->x_pos, prevTile->x_pos + prevTile->x_len,
			      &q3, &q4);
	}
	else if (Cpage == VERT)
	{
	    find_intersection(curTile->x_pos, curTile->x_pos + curTile->x_len,
			      prevTile->x_pos, prevTile->x_pos + prevTile->x_len,
			      &q3, &q4);
	}	
	else
	{
	    find_intersection(curTile->y_pos, curTile->y_pos + curTile->y_len,
			      prevTile->y_pos, prevTile->y_pos + prevTile->y_len,
			      &q3, &q4);
	}
	cornerY = create_range(create_srange(q3,q4,Cpage), n);
	cornerX = xlink;
	return(create_corner(cornerX, cornerY, Carc, NULL));
    }
    
    else /* No links - create both corners... Also Strange usage...*/
    {
	if ((Cpage == HORZ) && (Ppage == VERT))
	{
	    find_intersection(curTile->x_pos, curTile->x_pos + curTile->x_len,
			      prevTile->y_pos, prevTile->y_pos + prevTile->y_len,
			      &q1, &q2);
	    find_intersection(curTile->y_pos, curTile->y_pos + curTile->y_len,
			      prevTile->x_pos, prevTile->x_pos + prevTile->x_len,
			      &q3, &q4);
	}
	else if ((Cpage == VERT) && (Ppage == HORZ))
	{
	    find_intersection(curTile->y_pos, curTile->y_pos + curTile->y_len,
			      prevTile->x_pos, prevTile->x_pos + prevTile->x_len,
			      &q1, &q2);
	    find_intersection(curTile->x_pos, curTile->x_pos + curTile->x_len,
			      prevTile->y_pos, prevTile->y_pos + prevTile->y_len,
			      &q3, &q4);
	}
	else if (Cpage == HORZ)
	{
	    find_intersection(curTile->x_pos, curTile->x_pos + curTile->x_len,
			      prevTile->x_pos, prevTile->x_pos + prevTile->x_len,
			      &q1, &q2);
	    find_intersection(curTile->y_pos, curTile->y_pos + curTile->y_len,
			      prevTile->y_pos, prevTile->y_pos + prevTile->y_len,
			      &q3, &q4);
	}
	else /* (Cpage == VERT) */
	{
	    find_intersection(curTile->y_pos, curTile->y_pos + curTile->y_len,
			      prevTile->y_pos, prevTile->y_pos + prevTile->y_len,
			      &q1, &q2);
	    find_intersection(curTile->x_pos, curTile->x_pos + curTile->x_len,
			      prevTile->x_pos, prevTile->x_pos + prevTile->x_len,
			      &q3, &q4);
	}

/* sides get screwed up badly.  ?? */
	cornerX = create_range(create_srange(q1,q2,Cpage), n);
	cornerY = create_range(create_srange(q3,q4,Cpage), n);
        return(create_corner(cornerX, cornerY, Carc, NULL));
    }
}
/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/
int map_net_into_constraint_sets(n)
    net *n;
{
    /* This function walks along the global route assigned to the given net, assigning 
     * constraints to the various corners detected.  This involves the creation of srange
     * structures that will be resolved into points.  

     * To determine which global route is to be used, check each expansion: 
     * If the expansions belonging to the net have been properly terminated, then
     * there is a single (shared) list of trails that comprise the net.  
     
     * There is no guarantee that there is not any overlap abongst these trails */


     /* There are several conditions for these jointArcs that determine how the 
	ranges should be set up and linked:  First, a jointArc may be shared by 
	several (more than two) expansions (Rat's Nest condition);  
	Secondly, a joint arc may occur as a non-joint tile in the middle of another 
	path (T condition) and lastly a jointArc may be shared by two expansions
	(Simple Connection).
	*/

     expn *ex = (expn *)n->done->this;
     trail *besTrail;
     trlist *trl;
     tilist *temp, *tl, *tll;
     tile *lasTile, *thisTile;
     asg_arc *ar, *lar;
     range *xlink = NULL, *ylink = NULL;
     corner *c;
     int refX, refY;
	 
     for (trl = ex->connections; trl != NULL; trl = trl->next)
     {
	 besTrail = trl->this;
	 ex = besTrail->ex;
	 
	 /* Walk along the trail, starting at the source terminal: */
	 /* Need to maintain a reference point, and the link ranges <xlink> & <ylink> */
	 /* Assume that the starting tile (<lasTile>) has HORZonatal orientation. */
	 temp = (tilist *)rcopy_list(besTrail->tr);
	 lasTile = temp->this;
	 ar = (asg_arc *)lasTile->this;
	 refX = ex->t->x_pos + ex->t->mod->x_pos;
	 refY = ex->t->y_pos + ex->t->mod->y_pos;	
	 /* This is a terminal point, and should not be added to the VERT tile space.. */ 
	 c = create_corner(create_range(create_srange(refX, refX, VERT), n),
			   create_range(create_srange(refY, refY, HORZ), n),
			   ar, ex->t);
	 push(c, &besTrail->used);
	 pushnew(lasTile, &n->route);
	 
	 for (tl = temp->next; tl != NULL; tl = tl->next)
	 {
	     thisTile = tl->this;
	     ar = (asg_arc *)thisTile->this;		lar = (asg_arc *)lasTile->this;
	     
	     /* Standard Linkage control: */
	     if ( ((asg_arc *)(thisTile->this))->page == VERT )
	     {
		 ylink = c->cy;	xlink = NULL;
	     }
	     else
	     {
		 xlink = c->cx;	ylink = NULL;
	     }
	     
	     /* At the intersection of <lasTile> & <thisTile>, set a corner point. */
	     c = set_corner_point(n, thisTile, lasTile, xlink, ylink, refX, refY);
	     refX = midpoint(c->cx);
	     refY = midpoint(c->cy);
	     add_corner_to_arc(c, lar);
	     push(c, &besTrail->used);
	     pushnew(thisTile, &n->route);
	     
	     lasTile =  thisTile;	/* Advance the window... */	     
	 }
	 /* Now that the loop is done, <lasTile> should point to the the <joinTile>
	    for this trail. */
	 free_list(temp);
    }
}	
/*------------------------------------------------------------------------------------*/
int in_congestion_order(t1, t2)
    tile *t1, *t2;
{
    asg_arc *a1 = (asg_arc *)t1->this, *a2= (asg_arc *)t2->this;
    
    /* Return TRUE if <t1> has a higher congestion than <t2>. */
/*     if (a1->congestion >= a2->congestion) return(TRUE);  */
    if (list_length(a1->corners) < list_length(a2->corners)) return(FALSE);
    else if (a1->congestion >= a2->congestion) return(TRUE); 
    else return(TRUE);
}
/*------------------------------------------------------------------------------------*/
int int_overlapped_p(min1, max1, min2, max2)
    int min1, max1, min2, max2;
{
    /* This returns true if there is overlap between the ranges r1 & r2: */
    if (((max1 >= min2) && (min1 <= max2)) || 
	 ((min1 <= max2) && (max1 >= min2))) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int overlapped_p(r1, r2)
    srange *r1, *r2;
{
    /* This returns true if there is overlap between the ranges r1 & r2: */
    if (r1->orient == r2->orient) 
	return(int_overlapped_p(r1->q1, r1->q2, r2->q1, r2->q2));
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
corner *next_vert_corner(c)
    corner *c;
{
    /* return the next corner in the vertical direction.  This is meant to be
       pumped for info;  If NULL is passed in, the last <c> given is used again.. */	
    static crnlist *cl = NULL; 
    static corner *oldC = NULL;

    if ((int)c == VERT) return(oldC);
    else if (c != NULL) 
    {
	cl = c->cx->corners;
	oldC = c;
    }
    else c = oldC;

    for (; cl != NULL; cl = cl->next)
    {
	if (cl->this != c)
	{
	    if(cl->this->cy->n  == c->cy->n) 
	    {
		c = cl->this;
		cl = cl->next;	  /* Advance <cl> so we can restart where we left off */
		return(c);
	    }
	}
    }
    return(c);		/* Somethin's probably screwed... */
}
/*------------------------------------------------------------------------------------*/
corner *next_horz_corner(c)
    corner *c;
{
    /* return the next corner in the horizontal direction.This is meant to be
       pumped for info;  If NULL is passed in, the last <c> given is used again.. */	
    static crnlist *cl = NULL; 
    static corner *oldC = NULL;
    
    if ((int)c == HORZ) return(oldC); 	/* Special action */
    else if (c != NULL) 
    {
	cl = c->cy->corners;
	oldC = c;
    }
    else c = oldC;

    for (; cl != NULL; cl = cl->next)
    {
	if (cl->this != c)
	{
	    if(cl->this->cx->n == c->cx->n) 
	    {
		c = cl->this;
		cl = cl->next;	  /* Advance <cl> so we can restart where we left off */
		return(c);
	    }
	}
    }
    return(c);		/* Somethin's probably screwed... */
}
/*------------------------------------------------------------------------------------*/
int in_x_order(c1, c2)
    corner *c1, *c2;
{
    if (c1->cx->sr->q1 < c2->cx->sr->q1) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int in_y_order(c1, c2)
    corner *c1, *c2;
{
    if (c1->cy->sr->q1 < c2->cy->sr->q1) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
corner *most_vert_corner(c)
    corner *c;
{
    /* return the next corner, that is highest in the vertical direction. */
    corner *largest = c;
    crnlist *cl = NULL; 

    if (c != NULL) 
    {
	cl = c->cx->corners;
    }
    for (; cl != NULL; cl = cl->next)
    {
	if((cl->this->cy->n  == c->cy->n)  &&
	   (cl->this->cy->sr->q2 > largest->cy->sr->q2))  largest = cl->this;
    }
    return(largest);		
}
/*------------------------------------------------------------------------------------*/
corner *least_vert_corner(c)
    corner *c;
{
    /* return the next corner, that is lowest in the vertical direction. */
    corner *smallest = c;
    crnlist *cl = NULL; 

    if (c != NULL) 
    {
	cl = c->cx->corners;
    }
    for (; cl != NULL; cl = cl->next)
    {
	if((cl->this->cy->n  == c->cy->n)  &&
	   (cl->this->cy->sr->q1 < smallest->cy->sr->q1))  smallest = cl->this;
    }
    return(smallest);	
}
/*------------------------------------------------------------------------------------*/
corner *most_horz_corner(c)
    corner *c;
{
    /* return the next corner, that is highest in the horizontal direction. */
    corner *largest = c;
    crnlist *cl = NULL; 

    if (c != NULL) 
    {
	cl = c->cy->corners;
    }
    for (; cl != NULL; cl = cl->next)
    {
	if((cl->this->cx->n  == c->cx->n)  &&
	   (cl->this->cx->sr->q2 > largest->cx->sr->q2))  largest = cl->this;
    }
    return(largest);		
}
/*------------------------------------------------------------------------------------*/
corner *least_horz_corner(c)
    corner *c;
{
    /* return the next corner, that is lowest in the horizontal direction. */
    corner *smallest = c;
    crnlist *cl = NULL; 

    if (c != NULL) 
    {
	cl = c->cy->corners;
    }
    for (; cl != NULL; cl = cl->next)
    {
	if((cl->this->cy->n  == c->cy->n)  &&
	   (cl->this->cx->sr->q1 <  smallest->cx->sr->q1))  smallest = cl->this;
    }
    return(smallest);	
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
int forms_T_p(c)
    corner *c;
{
    crnlist *cl;

    /* This returns VERT if <c> is the center of a vertical T, and HORZ if it is
       the center of a horizontal T.  Otherwise, it returns FALSE. */

    /* If <c> has multiple (3 or more) points at the same y value, and <c> is more 
       than just a single point: */
    if ((list_length(c->cy->corners) > 2) && (list_length(c->cx->corners) > 1)) 
    {

	/* There is something worth looking at: */
	cl = (crnlist *)sort_list(c->cy->corners, in_x_order);
	
	if (cl->next->this == c)
	{
	    return(TRUE);
	}

	for (cl = cl->next; ((cl != NULL) && (cl->this != c) && (cl->next != NULL)); 
	     cl = cl->next)
	{
	    if ((cl->next->this == c) && (cl->next->next != NULL)) 
	    {
		return(TRUE);
	    }
	}
    }

    /* Not this axis, so now check the Y axis: */
    /* If <c> has multiple (3 or more) points at the same x value, and <c> is more 
       than just a single point: */
    if ((list_length(c->cx->corners) > 2) && (list_length(c->cy->corners) > 1))
    {
	/* There is something worth looking at: */
	cl = (crnlist *)sort_list(c->cx->corners, in_y_order);
	
	if (cl->next->this == c)
	{
	    return(TRUE);
	}

	for (cl = cl->next; ((cl != NULL) && (cl->this != c) && (cl->next != NULL)); 
	     cl = cl->next)
	{
	    if ((cl->next->this == c) && (cl->next->next != NULL)) 
	    {
		return(TRUE);
	    }
	}
    }
    return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int turns_up_p(c)
    corner *c;
{
    /* Returns TRUE iff this corner represents an upward turn.  This is rather tricky, 
     and is highly dependent on the vaguarities of the "next_vert_corner" function. */
    corner *nextY = next_vert_corner(c);
    range *r, *nextR;

    if (c == NULL) c = next_vert_corner(VERT);   /* Get the old C for comparison */
    if (c == nextY)  return(FALSE); 	/* (No turns) */

    r = c->cy;
    nextR = nextY->cy;

    /* if <r> is below <nextR> then return TRUE. */
    if ((r == NULL) || (nextR == NULL))
    {
	fprintf(stderr, "Major screwup 'in turns_up_p'\n");
	return(FALSE);
    }
    else if (r->sr->q2 < nextR->sr->q1) return(TRUE);
    else return(turns_up_p(NULL));		    /* Pump the "next_horz_corner" fn */
}
/*------------------------------------------------------------------------------------*/
int turns_right_p(c)
    corner *c;
{
    /* Returns TRUE iff this corner represents an right turn.  This is rather tricky, 
     and is highly dependent on the vaguarities of the "next_horz_corner" function. */
    corner *nextX = next_horz_corner(c);
    range *r, *nextR;

    if (c == NULL) c = next_horz_corner(HORZ);   /* Get the old C for comparison */
    if (c == nextX)  return(FALSE); 	/* (No turns) */

    r = c->cx;
    nextR = nextX->cx;

    /* if <r> is below <nextR> then return TRUE. */
    if ((r == NULL) || (nextR == NULL))
    {
	fprintf(stderr, "Major screwup 'in turns_right_p'\n");
	return(FALSE);
    }
    else if (r->sr->q2 < nextR->sr->q1) return(TRUE);
    else return(turns_right_p(NULL));		    /* Pump the "next_horz_corner" fn */
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
int form_vertical_ordering(c1, c2)
    corner *c1, *c2;
{
    /* RULE 1:  bends gravitate toward the corner in the direction of the turn.
       RULE 2:  T's are placed on the side of the off-angle.
       RULE 3:  Throughs & jogs gravitate toward the center of the area.
     */
    /* Returns TRUE iff <c1> should be placed above <c2> in the same box.  This
       fact depends on the context in which <c1> occurs within it's net, and <c2> 
       occurs within its net. */

    range *r1, *r2;
    corner *nextC1, *nextC2;
    int hOrder;
    int c1LeftMost, c1RightMost, c2LeftMost, c2RightMost;

    if (c1 == NULL) return (FALSE);
    else if (c2 == NULL) return (TRUE);
    else
    {
	r1 = c1->cy;
	r2 = c2->cy;

	/* Ignore corners beloning to the same net */
	if (r1->n == r2->n) return(FALSE);

	/* First, check the simple case that there is no overlapp to resolve: */
	if (overlapped_p(r1->sr, r2->sr) == FALSE)
	{
	    if (r1->sr->q1 > r2->sr->q1) return(TRUE);
	    else return(FALSE);
	}

	/* OK, there is vertical overlap to deal with, so... */
	/* Check to see if they turn in opposite directions: */
	c1RightMost = turns_up_p(most_horz_corner(c1));
	c1LeftMost = turns_up_p(least_horz_corner(c1));
	c2RightMost = turns_up_p(most_horz_corner(c2));
	c2LeftMost = turns_up_p(least_horz_corner(c2));

	if ((c1RightMost == TRUE) && (c2RightMost == FALSE)) return(TRUE);
	else if ((c1RightMost == FALSE) && (c2RightMost == TRUE)) return(FALSE);
	else if ((c1LeftMost == TRUE) && (c2LeftMost == FALSE)) return(TRUE);
	else if ((c1LeftMost == FALSE) && (c2LeftMost == TRUE)) return(FALSE);
	else 	/* They both turn the same way */
	{
	    /* special case for T's */

	    /* look again... */
	    /* Look along the X Axis to see where the next corners are.  If the turn 
	       of the next corners is opposite, that is one turns up, while the other 
	       turns down, then the resolution is complete.  The net that turns up gets
	       placed above the net that turns down. */
	
	    nextC1 = next_horz_corner(c1);
	    nextC2 = next_horz_corner(c2);

	    if ((nextC1 == c1) && (nextC2 == c2))
	    {
		/* Terminal points;  look the other way */
		nextC1 = next_vert_corner(c1);
		nextC2 = next_vert_corner(c2);
		
		if ((nextC1 != c1) && (nextC2 != c2))
		{
		    return(form_vertical_ordering(nextC1, nextC2));
		}
		else if (c1->cx->sr->q2 < c2->cx->sr->q1) return(FALSE);
		else return(TRUE);
	    }
	    else  
	    {
		/* Check to see if they turn in opposite directions: */
		c1RightMost = turns_up_p(most_horz_corner(nextC1));
		c1LeftMost = turns_up_p(least_horz_corner(nextC1));
		c2RightMost = turns_up_p(most_horz_corner(nextC2));
		c2LeftMost = turns_up_p(least_horz_corner(nextC2));
		
		if ((c1RightMost == TRUE) && (c2RightMost == FALSE)) return(TRUE);
		else if ((c1RightMost == FALSE) && (c2RightMost == TRUE)) return(FALSE);
		else if ((c1LeftMost == TRUE) && (c2LeftMost == FALSE)) return(TRUE);
		else if ((c1LeftMost == FALSE) && (c2LeftMost == TRUE)) return(FALSE);
		else 	
		{
		    /* The next corners also both turn the same way */
		    if (debug_level > 2)
		    {
			fprintf(stderr, "recursion in form_vertical_ordering "); 
			fprintf(stderr, "<%s>([%d, %d],[%d,%d]) and ", c1->cx->n->name,
				c1->cx->sr->q1, c1->cx->sr->q2, 
				c1->cy->sr->q1, c1->cy->sr->q2);
			
			fprintf(stderr, "<%s>([%d, %d],[%d,%d])\n", r2->n->name,
				c2->cx->sr->q1, c2->cx->sr->q2, 
				c2->cy->sr->q1, c2->cy->sr->q2);
		    }
		    hOrder = form_horizontal_ordering(nextC1, nextC2);
		    if (c1RightMost == TRUE) return( NOT(hOrder) );
		    else if (c1LeftMost == TRUE) return( NOT(hOrder) );
		    else return (hOrder);
		}
	    }
	}
    }
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
int form_horizontal_ordering(c1, c2)
    corner *c1, *c2;
{
     /* RULE 1:  bends gravitate toward the corner in the direction of the turn.
	RULE 2:  T's are placed on the side of the off-angle.
	RULE 3:  Throughs & jogs gravitate toward the center of the area.
     */

    /* Returns TRUE iff <c1> should be placed to the left of <c2> in the same box.  This
       fact depends on the context in which <c1> occurs within it's net, and <c2> 
       occurs within its net. */
    range *r1, *r2;
    corner *nextC1, *nextC2;
    int vOrder;
    int c1TopMost, c1BotMost, c2TopMost, c2BotMost;

    if (c1 == NULL) return (FALSE);
    else if (c2 == NULL) return (TRUE);
    else
    {
	r1 = c1->cx;
	r2 = c2->cx;

	/* Ignore corners beloning to the same net */
	if (r1->n == r2->n) return(FALSE);

	/* First, check the simple case that there is no overlapp to resolve: */
	if (overlapped_p(r1->sr, r2->sr) == FALSE)
	{
	    if (r1->sr->q1 > r2->sr->q1) return(FALSE);
	    else return(TRUE);
	}

	/* OK, there is vertical overlap to deal with, so... */
	/* Check to see if they turn in opposite directions: */
	c1TopMost = turns_right_p(most_vert_corner(c1));
	c1BotMost = turns_right_p(least_vert_corner(c1));
	c2TopMost = turns_right_p(most_vert_corner(c2));
	c2BotMost = turns_right_p(least_vert_corner(c2));

	if ((c1TopMost == TRUE) && (c2TopMost == FALSE)) return(TRUE);
	else if ((c1TopMost == FALSE) && (c2TopMost == TRUE)) return(FALSE);
	else if ((c1BotMost == TRUE) && (c2BotMost == FALSE)) return(TRUE);
	else if ((c1BotMost == FALSE) && (c2BotMost == TRUE)) return(FALSE);
	else 	/* They both turn the same way */
	{
	    /* special case for T's */

	    /* look again... */
	    /* Look along the Y Axis to see where the next corners are.  If the turn 
	       of the next corners is opposite, that is one turns up, while the other 
	       turns down, then the resolution is complete.  The net that turns up gets
	       placed above the net that turns down. */
	
	    nextC1 = next_vert_corner(c1);
	    nextC2 = next_vert_corner(c2);
	    if ((nextC1 == c1) && (nextC2 == c2)) 
	    {
		/* Terminal points;  look the other way */
		nextC1 = next_horz_corner(c1);
		nextC2 = next_horz_corner(c2);
		
		if ((nextC1 != c1) && (nextC2 != c2))
		{
		    return(form_horizontal_ordering(nextC1, nextC2));
		}
		else if (c1->cy->sr->q2 < c2->cy->sr->q1) return(FALSE);
		else return(TRUE);

	    }
	    else  
	    {
		/* Check to see if they turn in opposite directions: */
		c1TopMost = turns_right_p(most_vert_corner(nextC1));
		c1BotMost = turns_right_p(least_vert_corner(nextC1));
		c2TopMost = turns_right_p(most_vert_corner(nextC2));
		c2BotMost = turns_right_p(least_vert_corner(nextC2));
		
		if ((c1TopMost == TRUE) && (c2TopMost == FALSE)) return(TRUE);
		else if ((c1TopMost == FALSE) && (c2TopMost == TRUE)) return(FALSE);
		else if ((c1BotMost == TRUE) && (c2BotMost == FALSE)) return(TRUE);
		else if ((c1BotMost == FALSE) && (c2BotMost == TRUE)) return(FALSE);
		else   
		{
		    /* The next corners also turn the same way */
		    /* dangerous */
		    if (debug_level > 2)
		    {
			fprintf(stderr, "recursion in form_horzontal_ordering ");
			fprintf(stderr, "<%s>([%d, %d],[%d,%d]) and ", c1->cx->n->name,
				c1->cx->sr->q1, c1->cx->sr->q2, 
				c1->cy->sr->q1, c1->cy->sr->q2);
			
			fprintf(stderr, "<%s>([%d, %d],[%d,%d])\n", r2->n->name,
				c2->cx->sr->q1, c2->cx->sr->q2, 
				c2->cy->sr->q1, c2->cy->sr->q2);
		    }
		    vOrder = form_vertical_ordering(nextC1, nextC2);
		    if (c1TopMost == TRUE) return (vOrder);
		    else if (c1BotMost == TRUE) return (vOrder);
		    else return( NOT(vOrder) );
		}
	    }
	}
    }
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
int in_vert_order(c1, c2)
    corner *c1, *c2;
{
    if (c1->vertOrder >= c2->vertOrder) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int in_horz_order(c1, c2)
    corner *c1, *c2;
{
    if (c1->horzOrder <= c2->horzOrder) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int best_vert_order(cl)
    crnlist *cl;
{
    crnlist *cll;
    int min = 5000;
    for (cll = cl; cll != NULL; cll = cll->next)
    {
	min = MIN(min, cll->this->vertOrder);
    }
    return(min);
}
/*------------------------------------------------------------------------------------*/
int best_horz_order(cl)
    crnlist *cl;
{
    crnlist *cll;
    int min = 5000;
    for (cll = cl; cll != NULL; cll = cll->next)
    {
	min = MIN(min, cll->this->horzOrder);
    }
    return(min);
}
/*------------------------------------------------------------------------------------*/
int in_reasonable_vert_orderR(r1, r2)
    range *r1, *r2;
{
    /* indicates <r1> belongs above <r2> */
    crnlist *cl1 = r1->corners, *cl2 = r2->corners;
    int o1 = best_vert_order(cl1);
    int o2 = best_vert_order(cl2);

    if ((o1 >= o2) && (r2->sr->q2 > r1->sr->q1)) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int in_reasonable_horz_orderR(r1, r2)
    range *r1, *r2;
{
    /* indicates <r1> belongs to the left of <r2> */
    crnlist *cl1 = r1->corners, *cl2 = r2->corners;
    int o1 = best_horz_order(cl1);
    int o2 = best_horz_order(cl2);

    if ((o1 <= o2) && (r2->sr->q2 > r1->sr->q1)) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int merge_sranges(sr1, sr2)
    srange **sr1, **sr2;   		/* sranges associated that overlap */
{	
    /* This resolves <sr1> and <sr2> to the (same) smallest subset of the two. */
    
    int min1 = (*sr1)->q1, min2 = (*sr2)->q1;
    int max1 = (*sr1)->q2, max2 = (*sr2)->q2;
    int min = MAX(min1, min2),   max = MIN(max1, max2);
    srange *temp;
    
    /* if there is overlap, then do something... */
    if (((min1 >= min2) && (min1 <= max2)) || 
	((min2 >= min1) && (min2 <= max1)))
    {
	(*sr1)->q1 = min;
	(*sr1)->q2 = max;
	temp = *sr2;
	*sr2 = *sr1;
	my_free(temp);
	return(TRUE);
    }
    return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int divide_sranges(sr1, sr2)
    srange *sr1, *sr2;  	 /* sranges associated with each net that overlap */
{	
    /* This puts <sr1> to the right of <sr2> */
    int midPoint;
    int min1 = sr1->q1, min2 = sr2->q1;
    int max1 = sr1->q2, max2 = sr2->q2;

    if (min1 == max1) /* Handle the point cases - these are the most common */
    {
	if ((min2 == max2) && (min1 != min2)) return(TRUE);
	else if ((min2 == max2) && (min1 == min2)) return(FALSE);    /* Real Problem! */
	
	if (max2 > min1)
	{
	    if (min2 >= min1) 
	    {
		sr2->q1 = MAX(min1 + 1, min2);	/* Not good */
		return(FALSE);
	    }
	    else sr2->q2 = min1 - 1;
	}
	else sr2->q2 = MIN(min1 - 1, max2);
	return(TRUE);
    }
    else if (min2 == max2)
    {
	if (max1 >= min2) 
	{
	    if (min1 <= min2) sr1->q1 = min2 + 1;
	    return(TRUE);
	}
	return(FALSE);
    }
    else
    {
	/* Is there any overlap? */
	if ((max1 < min2) || (min1 == max2)) return(FALSE);
	else if ((min1 > max2) || (max1 == min2)) return(TRUE);

	midPoint = (max1 + min2) / 2;

	/* Set the beginning/end points of the sranges: */
	if (max1 < midPoint)
	{
	    sr1->q1 = max1;
	    sr2->q2 = max1 - 1;
	}
	else if (min2 > midPoint)
	{
	    sr1->q1 = min2;
	    sr2->q2 = min2 + 1;	    
	}
	else if ( ((midPoint - 1) < min2) || ((max1 - min1) >= (max2 -min2)) )
	{
	    sr1->q1 = MAX(min1, midPoint + 1);
	    sr2->q2 = MIN(midPoint, max2);
	}	
	else 
	{
	    sr1->q1 = MAX(min1, midPoint);
	    sr2->q2 = MIN(midPoint - 1, max2);
	}
	return(TRUE);
    }
}

/*------------------------------------------------------------------------------------*/
int ranged_overlap_p(r1a, r2a, r1b, r2b)
    srange *r1a, *r2a, *r1b, *r2b;
{
    /* This returns true if there is overlap between the line from [r1a to r2a] and
       the line from [r1b to r2b]: */
    int a1 = MIN(r1a->q1, r2a->q1), a2 = MAX(r1a->q2, r2a->q2);
    int startA = MIN(a1, a2),     endA = MAX(a1, a2);
    int b1 = MIN(r1b->q1, r2b->q1), b2 = MAX(r1b->q2, r2b->q2);
    int startB = MIN(b1, b2),     endB = MAX(b1, b2);

    if ((endA < startB) || (startA > endB)) return(FALSE);
    else return(TRUE);
}
/*-------------------------------------------------------------------------------------*/
int segments_overlapped_p(c1, c2, orient)
    corner *c1, *c2;
    int orient;
{
    /* check to see if these two corners overlap in the given direction.  If so,
       return TRUE. */

    crnlist *cl = (orient == Y) ? c1->cx->corners : c1->cy->corners;
    crnlist *cll, *clll = (orient == Y) ? c2->cx->corners : c2->cy->corners;
    srange *srA1, *srA2, *srB1, *srB2;

    if (orient == X)
    {
	srA1 = c1->cx->sr;	srB1 = c1->cx->sr;
	if (overlapped_p(srA1, srB1) == TRUE) return(TRUE);
    }
    else /* (orient == Y) */
    {
	srA1 = c1->cy->sr;	srB1 = c2->cy->sr;
	if (overlapped_p(srA1, srB1) == TRUE) return(TRUE);
    }

    for(; cl != NULL; cl = cl->next)
    {
	srA2 = (orient == X) ? cl->this->cx->sr : cl->this->cy->sr;
	for(cll = clll; cll != NULL; cll = cll->next)
	{
	    srB2 = (orient == X) ? cll->this->cx->sr : cll->this->cy->sr;
	    if (ranged_overlap_p(srA1, srA2, srB1, srB2) == TRUE) return(TRUE);
	}
    }
    return(FALSE);
}
/*-------------------------------------------------------------------------------------*/
/* Should check for function round() in configure. . . 	*/

/*
int round(f)
    float f;
{
    int d = (int)f;
    if (f - (float)d > 0.5) return(d + 1);
    else return(d);
}
*/

/*-------------------------------------------------------------------------------------*/
int set_step_size(orderedRanges, newStep)
    rnglist *orderedRanges;		/* These should be ordered l->r, or b->t */
    float *newStep;
{
    /* Look through the entire set of <orderedRanges> to see if there is an 
       uncoupleable condition (overlapped fixed points, start/end screwed up...) 
       This also dynamically resizes the step size...

       Want to find the minimum step size for the rangelist, assuming that the 
       first element on the list is the one that will use the result.
       */
    int n = list_length(orderedRanges);
    range *last = (range *)nth(orderedRanges, n);
    rnglist *rl;
    float min = (float)orderedRanges->this->sr->q1, max = (float)last->sr->q2;
    float step = (max - min - (float)n + 1) / n;

    n = 2;
    for (rl = orderedRanges->next; (rl != NULL); rl = rl->next)
    {
	max = (float)rl->this->sr->q2;
	if ((round(max) - round(min)) < (n - 1)) return(TRUE);
	step = MIN(MAX(((max - min - (float)n + 1) / n), 0.0), step);
	n += 1;
    }
    *newStep = step;
    return(FALSE);
}
/*-------------------------------------------------------------------------------------*/
void decouple_ordered_ranges(orderedRanges)
        rnglist *orderedRanges;		/* These should be ordered l->r, or b->t */
{
    int n = list_length(orderedRanges);
    range *r, *nextR, *last;
    rnglist *rl, *rll;
    float min, max, step, next;

    if ((debug_level >= 2) && (n > 1))
    {
	fprintf(stderr, "Attempt to Decouple ");
	dump_ranges(orderedRanges);
    }

    /* The first range should be limited to [min...min+step] or its current limit, 
       whichever is smaller.  The process repeats until all ranges have been 
       allotted non-overlapping slots. */

    form_into_decoupleable_order(orderedRanges, list_length(orderedRanges));


    /* As the call to "form_into_decoupleable_order" may rearange <orderedRanges>, 
       the following must be set after the call: */

        last = (range *)nth(orderedRanges, n);
        min = (float)orderedRanges->this->sr->q1;
        max = (float)last->sr->q2;
        step = MAX(((max - min - (float)n + 1) / n), 1.0);
        next = min;


    if (n > 1)
    {
	for (rl = orderedRanges; rl != NULL; rl = rl->next)
	{
	    r = rl->this;
	    if (set_step_size(rl, &step) == TRUE)
	    {
		/* Serious problems */
		fprintf(stderr, "Cannot decouple ranges ");
		dump_ranges(rl);
		break;
	    }

	    /* Advance the <min> and <next> pointers... */
	    min = (r->sr->q1 > round(next)) ? (float)r->sr->q1 : next;
	    next = (round(min + step) > r->sr->q2) ? (float)r->sr->q2 : min + step;
	    
	    if (rl->next != NULL)	     /* Lookahead to avoid needless constraint */
	    {
		nextR = rl->next->this;
		
		if (r->n == nextR->n)
		{
		    /* No need to constrain <r> as much. This allows <r> and <nextR>
		       to coincide.  */
		    next = (next < round(r->sr->q2)) ? next + step : (float)r->sr->q2;
		    nextR->sr->q1 = (round(min) > nextR->sr->q1) ? round(min) : 
		    nextR->sr->q1;
		}
		
		else  /* find the appropriate <next> point (where to end <r>) and set
			 the beginning point for <nextR>:         	      	*/
		{
		    if (round(next) < (nextR->sr->q1 - 1))
		    {
			if ((nextR->sr->q1 - 1) <=  r->sr->q2) 
			next = (float)nextR->sr->q1 - 1.0;
		    }
		    else if (round(next) >= nextR->sr->q2)
		    {
			/* Problem */
			next = (float)nextR->sr->q2 - 1.0;
			nextR->sr->q1 = nextR->sr->q2;
		    }
		    else if (round(next + step) >= nextR->sr->q2)
		    {
			/* problem... Should reset (shrink) the step size */
			step = MAX(((max - (float)nextR->sr->q1 - (float)n + 1.0) 
				    / (n - 1)), 1.0); 
			nextR->sr->q1 = round(next) + 1;
		    }
		    else 
		    {
			nextR->sr->q1 = round(next) + 1;
		    }
		}
	    }
	    else 
	    {
		next = max;
	    }
	    r->sr->q2 = round(next);
	    n -= 1;
	}

	if (debug_level >= 2)
	{
	    fprintf(stderr, "       Resulted in  ");
	    dump_ranges(orderedRanges);
	}
    }
    else if (debug_level >= 2)
    {
	r = orderedRanges->this;
	fprintf(stderr, "Skipping <%s>[%d,%d] \n", r->n->name, r->sr->q1, r->sr->q2);
    }
}
/*-------------------------------------------------------------------------------------*/
void decouple_ranges(r1, r2)
        range *r1, *r2;
{
    int flag;
    /* <r1> goes to the (bot/right) of <r2>: */
    flag = divide_sranges(r2->sr, r1->sr);
    
    if ((flag == FALSE) && (debug_level >= 2))
    fprintf(stderr, "<%s> range [%d,%d] cannot go to left of <%s> range [%d,%d]\n",
	    r1->n->name, r1->sr->q1, r1->sr->q2, 
	    r2->n->name, r2->sr->q1, r2->sr->q2);
}
/*-------------------------------------------------------------------------------------*/
rnglist *old_rev_decoupleable_p(orderedRanges, stopAt)
    rnglist *orderedRanges;		/* These should be ordered l->r, or b->t */
    int stopAt;
{
    /* Look through the entire set of <orderedRanges> to see if there is an 
       uncoupleable condition (overlapped fixed points, start/end screwed up...) 

       The object is to return a list pointer to the range who's ordering screws
       up the range set. (Or return NULL)

       This is the parallel to "decoupleable_p", in that it works by starting 
       with the list and trims from the front to find the error. zb. The list now 
       starts  1-2-3-4, 2-3-4, 3-4, 4 until the removal of a range fixes the problem.
       When the removal of a range fixes the problem, this is the list pointer
       to return.
     */

    int n = stopAt + 1;
    range *last = (range *)nth(orderedRanges, stopAt);
    rnglist *rl, *rLast = orderedRanges;
    int min, max;

    if (stopAt == 1) return(orderedRanges);

    /* Step forward through the list [1-2-3-4, 2-3-4, 3-4, 4] */
    max = last->sr->q2;
    for (rl = orderedRanges; ((rl != NULL) && (n-- > 0)); rl = rl->next)
    {
	min = rl->this->sr->q1;
	if ((max - min) >= (n - 1)) 			/* Someone's not fouled up */
	{
	    return(rLast);
	}
	if (rl != orderedRanges) rLast = rLast->next; 
    }
    return(NULL);
}
/*-------------------------------------------------------------------------------------*/
rnglist *rev_decoupleable_p(orderedRanges, stopAt)
    rnglist *orderedRanges;		/* These should be ordered l->r, or b->t */
    int stopAt;
{
    /* Look through the entire set of <orderedRanges> to see if there is an 
       uncoupleable condition (overlapped fixed points, start/end screwed up...) 

       The object is to return a list pointer to the range who's ordering screws
       up the range set. (Or return NULL)

       This is the parallel to "decoupleable_p", in that it works by starting 
       with the list and trims from the front to find the error. zb. The list now 
       starts  1-2-3-4, 2-3-4, 3-4, 4 until the removal of a range fixes the problem.
       When the removal of a range fixes the problem, this is the list pointer
       to return.
     */

    /* There is a strange sub-problem that exists when one of the internal elements
       has a smaller <max> than the end of the list;  This breaks the problem into
       similar subproblems at that point;  The hard part is keeping the left-hand
       list pointers straight with the number of elements that must be fit between
       the <min> (defined by the LH list pointer) and the <max>, which may be
       in the middle of the list somewhere.
     */

    int maxIndex, i1 = 0, i2 = 0, n, works = TRUE;
    range *last = (range*)nth(orderedRanges, stopAt);
    rnglist *rLast = orderedRanges;
    rnglist *RHs, *LHs = orderedRanges;
    int min = LHs->this->sr->q1, max = last->sr->q2;


    if (stopAt == 1) return(orderedRanges);

    /* Set the <max> for the full list: (may not be at <last>) */
    for (RHs = orderedRanges; ((RHs != NULL) && (i1++ < stopAt)); RHs = RHs->next)
    {
	if (max < RHs->this->sr->q2) 
	{
	    max = RHs->this->sr->q2;

	    /* Step forward through the list [1-2-3-4, 2-3-4, 3-4, 4] */
	    for (; ((LHs != NULL) && (i2++ <= i1)); LHs = LHs->next)
	    {
		min = MAX(min, LHs->this->sr->q1);
		n = i1 - i2 + 1;			 /* Number of ranges to fit in */

		/* Found a problem already, and Someone's not fouled up */
		if ((works == FALSE) && ((max - min) >= (n - 1))) return(rLast);  
		else if ((max - min) < (n - 1)) 
		{
		    works = FALSE;	if (LHs != orderedRanges) rLast = rLast->next; 
		}
		else if (LHs != orderedRanges) rLast = rLast->next; 
	    }
	    /* At this point --> ((LHs == RHs) && (n == 0)) should be the TRUE. */
	}
    }

    n = stopAt - i2 + 1;
    for (; ((LHs != NULL) && (n-- > 0)); LHs = LHs->next)
    {
	min = LHs->this->sr->q1;	

	/* Found a problem already, and Someone's not fouled up */
	if ((works == FALSE) && ((max - min) >= (n - 1))) return(rLast);  
	else if ((works == TRUE) && ((max - min) < (n - 1)))
	{
	    works = FALSE;	if (LHs != orderedRanges) rLast = rLast->next; 
	}
	else if (LHs != orderedRanges) rLast = rLast->next; 
    }
 
    return(NULL);
}
/*-------------------------------------------------------------------------------------*/
rnglist *decoupleable_p(orderedRanges, stopAt)
    rnglist *orderedRanges;		/* These should be ordered l->r, or b->t */
    int stopAt;
{
    /* Look through the entire set of <orderedRanges> to see if there is an 
       uncoupleable condition (overlapped fixed points, start/end screwed up...) 

       The object is to return a list pointer to the range who's ordering screws
       up the range set.

       This works by starting at the beginning of the list and working 
       forward through the list (1, 1-2, 1-2-3, 1-2-3-4, etc.) checking to see
       if the addition of any particular range destroys the decoupleability of the
       range set.  If no error is found, then NULL is returned.

     */

    int n = 1, idx = 1;
    range *last = (range *)nth(orderedRanges, stopAt);
    rnglist *rl, *fixup, *rLast = orderedRanges;
    int min, max;

    if (stopAt == 1) return(NULL);

    /* Search forward through the list [1, 1-2, 1-2-3, 1-2-3-4] */
    min = orderedRanges->this->sr->q1;
    for (rl = orderedRanges->next; ((rl != NULL) && (idx++ < stopAt)); rl = rl->next)
    {
	n++;
	max = rl->this->sr->q2;

	if (rl->this->sr->q1 > min)		/* reset <min> if a more stringent */
	{					/* constraint is found */

	    /* Tough example: { [137..150], [137..150], [137..150], [138..150], etc..} */

	    if ((rl->this->sr->q2 - min) < (n - 1))   
	    {   /* Someone doesn't fit the new constraints... */
		fixup = rev_decoupleable_p(orderedRanges, idx);
		if ((fixup == NULL) || (fixup == orderedRanges))return(rLast);
		else return(fixup);
	    }
	    else if ((rl->this->sr->q1 - min) < (n - 1))  
	    { /* Need to be careful how the min is reset */
		min = rl->this->sr->q1 + (n - 1) - (rl->this->sr->q1 - min);
	    }
	    else /* Don't need to be careful how min is reset */
	        min = rl->this->sr->q1;
	    n = 1;				/* Everything fits; restart the count */
	}

	if ((max - min) < (n - 1)) 			     /* Someone's fouled up */
	{
	    fixup = rev_decoupleable_p(orderedRanges, idx);
	    if (fixup == NULL) return(rLast);
	    else return(fixup);
	}
	rLast = rLast->next;	
    }
    return(NULL);
}
/*------------------------------------------------------------------------------------*/
int form_into_decoupleable_order(orderedRanges, maxRecDepth)
    rnglist *orderedRanges;
    int maxRecDepth;
{
    /* step through the given ordered set of ranges;  correct defaults by
       swapping.  keep track of the recursion depth to avoid serious problems.
       */

    range *temp;
    rnglist *rl, *badSpot = NULL;
    int i, r;

    if (maxRecDepth-- <= 0) 
    {
	fprintf(stderr, "Cannot reform ");
	dump_ranges(orderedRanges);
	return(FALSE);
    }

    for (i = 1; i <= list_length(orderedRanges); i++)
    {
	badSpot = decoupleable_p(orderedRanges, i);
	if (badSpot != NULL)
	{
	    /* swap some elements, and try again */
	    temp = badSpot->this;
	    badSpot->this = badSpot->next->this;
	    badSpot->next->this = temp;
	    
	    break;
	}
    }
    if (badSpot != NULL) r = form_into_decoupleable_order(orderedRanges, maxRecDepth);

    if ((debug_level >= 2) && (badSpot != NULL))
    {
	fprintf(stderr, "Reordered list into ");
	dump_ranges(orderedRanges);
    }
    return(TRUE);
}
/*------------------------------------------------------------------------------------*/
int in_length_order(l, ll)
    list *l, *ll;
{
    /* Return TRUE if <l> is longer than <ll>: */
    if (list_length(l) >= list_length(ll)) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
void perpendicular_expanse(r, min, max)
    range *r;
    int *min, *max;
{
    /* This walks through all of the corners associated with <r>.
       The extent of the ranges seen among the corners associated with <r> is 
       returned; */

    crnlist *cl = r->corners;
    range *oppRange = (cl->this->cx == r) ? cl->this->cy : cl->this->cx;
    
    *min = oppRange->sr->q1;
    *max = oppRange->sr->q2;


    for (; cl != NULL; cl = cl->next)
    {
	oppRange = (cl->this->cx == r) ? cl->this->cy : cl->this->cx;
	*min = MIN(oppRange->sr->q1, *min);
	*max = MAX(oppRange->sr->q2, *max);
    }
}
/*------------------------------------------------------------------------------------*/
int check_sranges(sr, r)
    srange *sr;
    range *r;
{
    if (r->sr == sr) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
int by_length(l1, l2)
    list *l1, *l2;
{
    /* return TRUE if list_length(<l1>) > list_length(<l2>)	*/

    if (list_length(l1) > list_length(l2)) return (TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int subset_p(s, set)
    list *s, *set;
{
    int i, len = list_length(s);
    list *l;
    
    if (len > list_length(set)) return(FALSE);
    for (l = s; l != NULL; l = l->next)
    {
	if (member_p(l->this, set, identity) == FALSE) return(FALSE);
    }
    return(TRUE);
}
/*------------------------------------------------------------------------------------*/
int subset_of_any_p(s, setOfSets)
    list *s, *setOfSets;		/* a list, and a list of lists */
{
    list *l;
    
    for (l = setOfSets; l != NULL; l = l->next)
    {
	if (subset_p(s, l->this) == TRUE) return(TRUE); /* <s> is subset of <l>->this */
    }
    return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int copy_of_list_p(l1, l2)
    list *l1, *l2;
{
    int i, len = list_length(l1);
    list *lptr1 = l1, *lptr2 = l2;
    
    if (len != list_length(l2)) return(FALSE);
    for (i = len; i >0; i--)
    {
	if (lptr1 == lptr2) 
	{
	    if (lptr1 == NULL) return(TRUE);
	    lptr1 = lptr1->next;
	    lptr2 = lptr2->next;
	}
	else if (lptr1->this != lptr2->this) return(FALSE);
    }
    return(TRUE);
}
/*------------------------------------------------------------------------------------*/
int seen_by_others_p(range *r,rnglist *path)

{
    /* Return TRUE if this range <r> depends on any ranges that are yet unseen */
    /* The additional caveat is that for the unseen 'viewer' to qualify, it must be a 
       part of the given <path> */
    rnglist *rl;
    for(rl = reverseDepSets[r->use]; rl != NULL; rl = rl->next)
    {
	if ((rl->this->flag == UNSEEN) &&
	    (member_p(rl->this, path, identity) == TRUE)) return(TRUE);
    }
    return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int lowest_level_seen_at(pr, cr, dChains)
    range *pr, *cr;	
    rnglist **dChains;
{
    /* Return the lowest level at which the child range <cr> is used in the chain
       containing the given parent range <pr>. */
    int pi = pr->use, ci = cr->use, lowest, bottomNotReached = TRUE;
    range *r = pr;
    rnglist *rl;

    if (member_p(cr, dChains[pi], identity) == TRUE)
    {
	lowest = levelMap[pi];
	while (bottomNotReached == TRUE)
	{
	    for(rl = dChains[r->use]->next; rl != NULL; rl = rl->next)
	    {
		r = rl->this;
		if (member_p(cr, dChains[r->use]->next, identity) == TRUE)
		{
		    lowest = MIN(levelMap[r->use], lowest);
		}
		if ((r == cr) || 
		    (dChains[r->use]->next == NULL)) bottomNotReached = FALSE;
	    }
	}
	return(lowest);
    }

    /* The default case: */
    else return(levelMap[ci]);

}
/*------------------------------------------------------------------------------------*/
rnglist *enumerate_dependencies(parentRange, dChains, status, soFar)
    range *parentRange;	
    rnglist **dChains;
    int *status;
    rnglist **soFar;
{
    /* Depth-first traversal of the dependencies.   This just builds, and marks.
       It does not distinguish that a graph is redundant, saving that the status will
       never change to TRUE.  
       
       The graph is built up from the bottom of a recursion chain, and a graph 
       will be built if somewhere in the chain a previously-unseen range is included. */

    int i = parentRange->use, currentLevel, noneSeen = TRUE;
    range *nextParent;
    rnglist *rl, *growingChain = NULL;

    /* simple-case: bad chain */
    if (dChains[i] == NULL) return(NULL);

    /* No dependencies */
    else if (dChains[i]->next == NULL)		
    {
	if ((parentRange->flag == UNSEEN) || (*status == TRUE))
	{
	    /* Start building a chain - there's something worth building */
	    if (parentRange->flag == UNSEEN) *status = TRUE;
	    if (seen_by_others_p(parentRange, *soFar) == FALSE) parentRange->flag = SEEN;
	    push(parentRange, &growingChain);
	    return(growingChain);
	}
	else return(NULL);		/* There's nothing worth building */
    }

    else /* There are Mulitple Dependencies... */
    {
	/* Note the status: */
	if (parentRange->flag == UNSEEN) 
	{
	    *status = TRUE;
	    if (seen_by_others_p(parentRange, *soFar) == FALSE) parentRange->flag = SEEN;
	}

	/* Check all of the dependents to see if they should be selected for
	   branching.  To qualify, they can only branch on UNTRIED nodes */
	nextParent = dChains[i]->next->this;		/* default = Depth-first */
	currentLevel = levelMap[i];
	for (rl = dChains[i]->next; rl != NULL; rl = rl->next)
	{
	    if ((rl->this->flag != TRIED) &&
		(currentLevel == lowest_level_seen_at(parentRange, rl->this, dChains)))
	    {
		nextParent = rl->this;
		push(nextParent, soFar);

		/* See if this was successful: */
		growingChain = enumerate_dependencies(nextParent, dChains,
						      status, soFar);
		if (growingChain != NULL) 		/* It Worked */
		{
		    push(parentRange, &growingChain);	/* Add to the chain */
		    return(growingChain);        		
		}
		else pop(soFar);			/* It didn't */
	    }
	}
	parentRange->flag = TRIED;  /* The possibilities have all failed */
	return(growingChain);        		
    }
}
/*------------------------------------------------------------------------------------*/
int in_depth_order(r1, r2)
    range *r1, *r2;
{
    if (levelMap[r1->use] < levelMap[r2->use]) return (TRUE);
    else if (r1->use < r2->use) return(TRUE);
    else return (FALSE);
}
/*------------------------------------------------------------------------------------*/
int dependence_level(idx, dChains)
    int idx;
    rnglist **dChains;		/* Deeply recursive */
{
    /* Find the maximum dependence level from this set of chains */
    rnglist *rl, *chain = dChains[idx];
    int lev = 0;

    if (list_length(chain) <= 1) return(0);

    for (rl = chain->next; rl != NULL; rl = rl->next)
    {
	lev = MAX(lev, dependence_level(rl->this->use, dChains));
    }
    return(1 + lev);
}
/*------------------------------------------------------------------------------------*/
int help_setup_dependence_levels(idx, level, dChains)
    int idx, level;
    rnglist **dChains;		/* Deeply recursive */
{
    /* Find the maximum dependence level from this set of chains */
    rnglist *rl, *chain = dChains[idx];
    int deepest = 0;

    chain->this->flag = MAX(level, chain->this->flag);

    for (rl = chain->next; rl != NULL; rl = rl->next)
    {
	if (rl->this->flag >= level + 1) 
	{
	    deepest = MAX(deepest, rl->this->flag);
	}
	else
	{
	    rl->this->flag = level + 1;
	    deepest = MAX(help_setup_dependence_levels(rl->this->use, level + 1, dChains),
			  deepest);
	}
    }
    return(MAX(deepest, chain->this->flag));
}
/*------------------------------------------------------------------------------------*/
int setup_dependence_levels(count, dChains)
    int count;
    rnglist **dChains;		/* Deeply recursive */
{
    /* Set the ->flag fields to contain the maximum depth of the 
       given range in the graph set. */

    rnglist *rl, *chain; 
    int idx;

    for (idx = 0; idx < count; idx++)
    {
	chain = dChains[idx];
	for (rl = chain->next; rl != NULL; rl = rl->next)
	{
	    help_setup_dependence_levels(rl->this->use, 
					 MAX(1, rl->this->flag), dChains);
	}
    }
}
/*------------------------------------------------------------------------------------*/
list *form_dependency_graph(orderedRanges)
    rnglist *orderedRanges;
{
    /* This returns a list of range lists, such that each list is a subgraph of the
       dependency graph for the given ranges.  The ordering is the left->right/bottom->
       top ordering for the given set of ranges. 
       This is not easy.  Start with an ordered list of ranges, ordered bottom->top.
       Then start with the bottom-most range, and sweep up the list;  The sweep front
       has the width of the last range encountered, and every range that overlaps the
       sweep-front is picked up.  This process continues until all the list has been
       checked once, and produes one dependency graph (stored in <temp>)

       Next, start over, to see if there are more...       */

    rnglist *rl, *rll, *graph, *temp = NULL, *scratch = NULL, **dependenceSets;
    list *nonOverlap = NULL;
    int i = 0, min, max, rMin, rMax, tot = list_length(orderedRanges);
    int maxLevel, maxIndex, notDone = TRUE, status = FALSE;

    /* create an array to hold the dependency chains : */
    dependenceSets = (rnglist **)calloc(tot, sizeof(rnglist *));

    /* Assign the global pointer to the depth map: */
    levelMap = (int *)calloc(tot, sizeof(int));		
    reverseDepSets = (rnglist **)calloc(tot, sizeof(rnglist *));

    /* Set up the chain of dependencies : */
    for (rl = orderedRanges; rl != NULL; rl = rl->next)
    {
	perpendicular_expanse(rl->this, &rMin, &rMax);    /* The parent extent */
	temp = NULL;
	for (rll = rl->next; rll != NULL; rll = rll->next)
	{
	    perpendicular_expanse(rll->this, &min, &max); /* Wavefront  Extent */
	    if (int_overlapped_p(min,max,rMin,rMax) == TRUE) 	  /* overlap */
	    {
		push(rll->this, &temp);		/* These are sorted later */
	    }
	}
	push(rl->this, &temp);
	dependenceSets[i] = temp;	
	rl->this->use = i++;			 /* Save the idx */
	rl->this->flag = 0;			 /* Assume everyone starts at level 0 */
    }

    /* From the dependence set, create the level map: */
    maxIndex = tot - 1;
    maxLevel = 0;

    setup_dependence_levels(tot, dependenceSets);
    for(i = tot - 1; i >= 0; i--)
    {
/* 	OLD->	levelMap[i] = dependence_level(i, dependenceSets); */
	levelMap[i] = dependenceSets[i]->this->flag;
	if (levelMap[i] > maxLevel)
	{
	    maxIndex = i;
	    maxLevel = levelMap[i];
	}
    }

    for (rl = orderedRanges; rl != NULL; rl = rl->next)
    {
	rl->this->flag = UNSEEN;	 /* Used to distinguish a unique graph */

	/* sort the chains in dependency order */
	sort_list(dependenceSets[rl->this->use], in_depth_order);

	/* Set up the reverse dependency lists */
	for (rll = dependenceSets[rl->this->use]->next; rll != NULL; rll = rll->next)
	{
	    push(rl->this, &reverseDepSets[rll->this->use]);
	}
    }

    /* From the dependence set and the level map, create the dependency graph: */
    for(rl = orderedRanges; rl != NULL; rl = rl->next)
    {
	if (rl->this->flag == UNSEEN)
	{
	    notDone = TRUE;				/* Start a new search */

	    /* enumerate into a set of complete graphs: */
	    while (notDone == TRUE)
	    {
		/* Make the next dependency graph: */
		graph = enumerate_dependencies(rl->this, dependenceSets, 
					       &status, &scratch);
		scratch = (rnglist *)free_list(scratch);
		if ((status == TRUE) && (graph != NULL))
		{
		    push(graph, &nonOverlap);			/* Keep it  */
		    status = FALSE;
		}
		else if (graph == NULL) 
		{
		    notDone = FALSE;				/* Quit looking */
		    /* Clear the TRIED flags set */
		    for(rll = orderedRanges; rll != NULL; rll = rll->next) 
		        if (rll->this->flag == TRIED) rll->this->flag = SEEN;
		}
		else free_list(graph);				/* Scrap it */
	    }
	}
    }

    return(nonOverlap);
}       
/*------------------------------------------------------------------------------------*/
int range_exits_top(r, a)
    range *r;
    asg_arc *a;
{
    /* This function returns TRUE if any of the corners along this range (assumed
       to be representative of a y-segment) are at or beyond the top of the given
       tile. */
    int y = (a->page == HORZ) ? a->t->y_pos + a->t->y_len : 
                                a->t->x_pos + a->t->x_len;
    corner *c;
    crnlist *cl;

    for (cl = r->corners; cl != NULL; cl = cl->next)
    {
	for (c = next_vert_corner(cl->this);(c != cl->this); c = next_vert_corner(NULL))
	{
	    if (cl->this->cy->sr->q1 >= y) return(TRUE);
	}
	if (cl->this->cy->sr->q1 >= y) return(TRUE);
    }
    return (FALSE);
}    
/*------------------------------------------------------------------------------------*/
int range_exits_bottom(r, a)
    range *r;
    asg_arc *a;
{
    /* This function returns TRUE if any of the corners along this range (assumed
       to be representative of a y-segment) are at or beyond the bottom of the given
       tile. */
    int y = (a->page == HORZ) ? a->t->y_pos : a->t->x_pos;
    corner *c;
    crnlist *cl;

    for (cl = r->corners; cl != NULL; cl = cl->next)
    {
	for (c = next_vert_corner(cl->this);(c != cl->this); c = next_vert_corner(NULL))
	{
	    if (c->cy->sr->q2 <= y) return(TRUE);
	}
	if (c->cy->sr->q2 <= y) return(TRUE);
    }
    return (FALSE);
}    
/*------------------------------------------------------------------------------------*/
int range_exits_right(r, a)
    range *r;
    asg_arc *a;
{
    /* This function returns TRUE if any of the corners along this range (assumed
       to be representative of an x-segment) are at or beyond the right side of the 
       given tile. */
    int x = (a->page == HORZ) ? a->t->x_pos + a->t->x_len : a->t->y_pos + a->t->y_len;
    corner *c;
    crnlist *cl;

    for (cl = r->corners; cl != NULL; cl = cl->next)
    {
	for (c = next_horz_corner(cl->this); (c != cl->this); c = next_horz_corner(NULL))
	{
	    if (c->cx->sr->q1 >= x) return(TRUE);
	}
	if (c->cx->sr->q1 >= x) return(TRUE);
    }
    return (FALSE);
}    
/*------------------------------------------------------------------------------------*/
int range_exits_left(r, a)
    range *r;
    asg_arc *a;
{
    /* This function returns TRUE if any of the corners along this range (assumed
       to be representative of an x-segment) are at or beyond the left side of the 
       given tile. */
    int x = (a->page == HORZ) ? a->t->x_pos : a->t->y_pos;
    corner *c;
    crnlist *cl;

    for (cl = r->corners; cl != NULL; cl = cl->next)
    {
	for (c = next_horz_corner(cl->this);(c != cl->this); c = next_horz_corner(NULL))
	{
	    if (c->cx->sr->q2 <= x) return(TRUE);
	}
	if (c->cx->sr->q2 <= x) return(TRUE);
    }
    return (FALSE);
}    
/*------------------------------------------------------------------------------------*/
int natural_break_p(r1, r2)
    range *r1, *r2;
{
    if ((r1->sr->q2 <= r2->sr->q1) ||
	(r2->sr->q2 <= r1->sr->q1)) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int follow_natural_break(r1, r2)
    range *r1, *r2;
{
    /* There is a 'natural' break, so return it */
    if (r1->sr->q2 <= r2->sr->q1) return(TRUE);	
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
int exits_right_p(c)	/* Not very bulletproof. */
    corner *c;
{
    corner *nextC = next_horz_corner(c);
    corner *otherNextC = next_horz_corner(NULL);
    
    if (nextC->cx->sr->q1 >= c->cx->sr->q2) return(TRUE);
    else if ((otherNextC != c) && 
	     (otherNextC->cx->sr->q1 >= c->cx->sr->q2)) return(TRUE);
    else return(FALSE);
}    
/*------------------------------------------------------------------------------------*/
int exits_top_p(c)
    corner *c;
{
    corner *nextC = next_vert_corner(c);
    corner *otherNextC = next_vert_corner(NULL);
    
    if (nextC->cy->sr->q1 >= c->cy->sr->q2) return(TRUE);
    else if ((otherNextC != c) &&
	     (otherNextC->cy->sr->q1 >= c->cy->sr->q2)) return(TRUE);
    else return(FALSE);
}    
/*------------------------------------------------------------------------------------*/
int share_bound_p(r1, r2, boundC1, boundC2)
    range *r1, *r2;
    corner **boundC1, **boundC2;
{
    /* This returns TRUE if any terminus of <r1> overlaps that of <r2> */
    /* As a useful side-effect, return the pointers to the corners from which the
       determiniation was concluded... */
    crnlist *cl, *cll;
    srange *t1, *t2;
    int orient = r1->sr->orient;

    *boundC1 = r1->corners->this;
    *boundC2 = r2->corners->this;

    for(cl = r1->corners; cl != NULL; cl = cl->next)
    {
	for(cll = r2->corners; cll != NULL; cll = cll->next)
	{
	    if (orient == HORZ)
	    {
		t1 = cl->this->cx->sr;
		t2 = cll->this->cx->sr;
	    }
	    else 
	    {
		t1 = cl->this->cy->sr;
		t2 = cll->this->cy->sr;
	    }		
	    if ((overlapped_p(t1, t2) == TRUE) &&
		(t1->q1 == t1->q2) && (t2->q1 == t2->q2))
	    {
		*boundC1 = cl->this;
		*boundC2 = cll->this;		
		return(TRUE);
	    }
	}
    } 
    return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int share_lower_bound_p(r1, r2, minC1, minC2)
    range *r1, *r2;
    corner **minC1, **minC2;
{
    /* This returns TRUE if the lesser terminus of <r1> overlaps that of <r2> */
    /* As a useful side-effect, return the pointers to the corners from which the
       determiniation was concluded... */
    crnlist *cl;
    srange *min1, *min2, *temp;
    int orient = r1->sr->orient;

    min1 = (orient == HORZ) ? r1->corners->this->cx->sr : r1->corners->this->cy->sr;
    min2 = (orient == HORZ) ? r2->corners->this->cx->sr : r2->corners->this->cy->sr;

    *minC1 = r1->corners->this;
    *minC2 = r2->corners->this;

    for(cl = r1->corners; cl != NULL; cl = cl->next)
    {
	temp = (orient == HORZ) ? cl->this->cx->sr : cl->this->cy->sr;
	if (temp->q1 < min1->q1)
	{
	    min1 = temp;
	    *minC1 = cl->this;
	}
    } 
    for(cl = r2->corners; cl != NULL; cl = cl->next)
    {
	temp = (orient == HORZ) ? cl->this->cx->sr : cl->this->cy->sr;
	if (temp->q1 < min2->q1)
	{
	    min2 = temp;
	    *minC2 = cl->this;
	}
    } 
    
    /* Now compare <min1> to <min2>: The lower bound is considered to be shared
       whenever the two overlapp, and are restrictive... */
    if ((overlapped_p(min1, min2) == TRUE) &&
	(min1->q1 == min1->q2) && (min2->q1 == min2->q2)) return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int find_lowest_exits(r1, r2, minC1, minC2)
    range *r1, *r2;
    corner **minC1, **minC2;
{
    /* As a useful side-effect, return the pointers to the corners which exit the
       right/top side at the smallest (lowest/left-most)points.  */
    crnlist *cl;
    srange *min1, *min2, *temp;
    int orient = r1->sr->orient;

    min1 = (orient == HORZ) ? r1->corners->this->cx->sr : r1->corners->this->cy->sr;
    min2 = (orient == HORZ) ? r2->corners->this->cx->sr : r2->corners->this->cy->sr;

    *minC1 = r1->corners->this;
    *minC2 = r2->corners->this;

    for(cl = r1->corners; cl != NULL; cl = cl->next)
    {
	temp = (orient == HORZ) ? cl->this->cx->sr : cl->this->cy->sr;
	if ((temp->q1 < min1->q1) || 
	    ((orient == HORZ) && (exits_top_p(*minC1) == FALSE)) || 
	    ((orient == VERT) && (exits_right_p(*minC1) == FALSE)))
	{
	    min1 = temp;
	    if (((orient == HORZ) && (exits_top_p(cl->this) == TRUE)) ||
		((orient == VERT) && (exits_right_p(cl->this) == TRUE)))
	    *minC1 = cl->this;
	}
    } 
    for(cl = r2->corners; cl != NULL; cl = cl->next)
    {
	temp = (orient == HORZ) ? cl->this->cx->sr : cl->this->cy->sr;
	if ((temp->q1 < min2->q1) ||
	    ((orient == HORZ) && (exits_top_p(*minC2) == FALSE)) || 
	    ((orient == VERT) && (exits_right_p(*minC2) == FALSE)))
	{
	    min2 = temp;
	    if (((orient == HORZ) && (exits_top_p(cl->this) == TRUE)) ||
		((orient == VERT) && (exits_right_p(cl->this) == TRUE)))
	    *minC2 = cl->this;
	}
    } 
    return(1);
}
/*------------------------------------------------------------------------------------*/
int find_highest_exits(r1, r2, maxC1, maxC2)
    range *r1, *r2;
    corner **maxC1, **maxC2;		/* Untested */
{
    /* As a useful side-effect, return the pointers to the corners which exit the
       right/top side at the largest (highest/right-most)points.  */
    crnlist *cl;
    srange *max1, *max2, *temp;
    int orient = r1->sr->orient;

    max1 = (orient == HORZ) ? r1->corners->this->cx->sr : r1->corners->this->cy->sr;
    max2 = (orient == HORZ) ? r2->corners->this->cx->sr : r2->corners->this->cy->sr;

    *maxC1 = r1->corners->this;
    *maxC2 = r2->corners->this;

    for(cl = r1->corners; cl != NULL; cl = cl->next)
    {
	temp = (orient == HORZ) ? cl->this->cx->sr : cl->this->cy->sr;
	if ((temp->q1 > max1->q1) || 
	    ((orient == HORZ) && (exits_top_p(*maxC1) == FALSE)) || 
	    ((orient == VERT) && (exits_right_p(*maxC1) == FALSE)))
	{
	    max1 = temp;
	    if (((orient == HORZ) && (exits_top_p(cl->this) == TRUE)) ||
		((orient == VERT) && (exits_right_p(cl->this) == TRUE)))
	    *maxC1 = cl->this;
	}
    } 
    for(cl = r2->corners; cl != NULL; cl = cl->next)
    {
	temp = (orient == HORZ) ? cl->this->cx->sr : cl->this->cy->sr;
	if ((temp->q1 > max2->q1) ||
	    ((orient == HORZ) && (exits_top_p(*maxC2) == FALSE)) || 
	    ((orient == VERT) && (exits_right_p(*maxC2) == FALSE)))
	{
	    max2 = temp;
	    if (((orient == HORZ) && (exits_top_p(cl->this) == TRUE)) ||
		((orient == VERT) && (exits_right_p(cl->this) == TRUE)))
	    *maxC2 = cl->this;
	}
    } 
    return(1);
}
/*------------------------------------------------------------------------------------*/
corner *greatest_corner(r)
    range *r;
{
    /* Return TRUE if the given <r> has no connected corners lower/left of it */

    corner *greatest = r->corners->this;
    crnlist *cl;
    int orient = r->sr->orient;
    int max, temp;

    max = (orient == VERT) ? greatest->cy->sr->q2 : greatest->cx->sr->q2;
    for (cl = r->corners->next; cl != NULL; cl = cl->next)
    {
	temp = (orient == VERT) ? cl->this->cy->sr->q2 : cl->this->cx->sr->q2;
	if (temp > max) greatest = cl->this;	
    }
    return(greatest);
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
corner *least_corner(r)
    range *r;
{
    /* Return TRUE if the given <r> has no connected corners above/right of it */

    corner *least = r->corners->this;
    crnlist *cl;
    int orient = r->sr->orient;
    int min, temp;

    min = (orient == VERT) ? least->cy->sr->q1 : least->cx->sr->q1;
    for (cl = r->corners->next; cl != NULL; cl = cl->next)
    {
	temp = (orient == VERT) ? cl->this->cy->sr->q1 : cl->this->cx->sr->q1;
	if (temp < min) least = cl->this;	
    }
    return(least);
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
int internal_horz_ordering(c1, c2)
    corner *c1, *c2;
{
    range *r1 = c1->cx, *r2 = c2->cx;
    int min, max, r1Extent, r2Extent, temp;
    int result;

    /* return TRUE if <r1> should be placed to the left of <r2>: */

    if (natural_break_p(r1, r2) == TRUE) result = follow_natural_break(r1, r2);

    else if ((r1->use == NULL) || (r2->use == NULL))
    {
    	fprintf(stderr, "error in internal_horz_ordering; NULL ->use assignment\n");
	result = TRUE;
    }

    else 
    {
	switch(r1->use)
	{
	    case UL_CORNER: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - */ 
	    if (r2->use != UL_CORNER) result = TRUE;
	    else 
	    {
		/* Ye who exits to the left uppermost is left-most */
		result = form_vertical_ordering(c1, c2);
	    }
	    break;


	    case LL_CORNER: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	    if (r2->use == UL_CORNER) result = FALSE;
	    else if (r2->use == LL_CORNER) 
	    {
		/* Ye who exit to the left lower-most is left-most */
		result = form_vertical_ordering(c2, c1);
	    }
	    else result = TRUE;
	    break;


	    case LEFT_U: /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	    if ((r2->use == UL_CORNER) || (r2->use == LL_CORNER)) result = FALSE;
	    else if (r2->use == LEFT_U) 
	    {
		/* place the shortest vertical segment left-most */
		perpendicular_expanse(c1->cx, &min, &max);
		r1Extent = max - min;
		perpendicular_expanse(c2->cx, &min, &max);
		r2Extent = max - min;
		
		if (r1Extent >= r2Extent) result = FALSE;
		else result = TRUE;
	    }
	    else result = TRUE;
	    break;
	    
	    
	    case VERT_LT: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	    if ((r2->use == UL_CORNER) || (r2->use == LL_CORNER) ||
		(r2->use == LEFT_U)) result = FALSE;
	    else if (r2->use == VERT_LT)
	    {
		/* Ye who exits to the left uppermost is left-most */
		temp = form_vertical_ordering(c1, c2);
		result = temp;
	    }
	    else result = TRUE;
	    break;
	    
	    
	    case HORZ_JOG:
	    case HORZ_UT:
	    case HORZ_DT:
	    case UPWARD_U:
	    case DOWNWARD_U:/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	    if ((r2->use == UL_CORNER) || (r2->use == LL_CORNER) ||
		(r2->use == LEFT_U) || (r2->use == VERT_LT)) result = FALSE;
	    else if ((r2->use == UR_CORNER) || (r2->use == LR_CORNER) ||
		     (r2->use == RIGHT_U) || (r2->use == VERT_RT)) result = TRUE;
	    else if ((r2->use == X) || (r2->use == VERT_JOG)) result = TRUE;
	    else
	    {
		/* These are a bit tricky: Follow the external restrictions; */
		temp = form_horizontal_ordering(c1, c2);
		result = temp;
	    }
	    break;
	    
	    case X_CORNER:
	    case VERT_JOG: /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	    if ((r2->use == UL_CORNER) || (r2->use == LL_CORNER) ||
		(r2->use == LEFT_U) || (r2->use == VERT_LT)) result = FALSE;
	    else if ((r2->use == UR_CORNER) || (r2->use == LR_CORNER) ||
		     (r2->use == RIGHT_U) || (r2->use == VERT_RT)) result = TRUE;
	    else if ((r2->use == HORZ_JOG) || (r2->use == HORZ_UT) ||
		     (r2->use == HORZ_DT) || (r2->use == UPWARD_U) ||
		     (r2->use == DOWNWARD_U)) result = FALSE;
	    else 
	    {
		/* ye who exits to the right the lowest is left-most, except where there
		   are restrictions on the y-segment overlaps...*/
		if (share_bound_p(r1, r2, &c1, &c2) == TRUE) 
	        temp = exits_right_p(c2);
		else if ((find_lowest_exits(r1, r2, &c1, &c2)) &&
			 ((c1 == least_corner(r1)) || (c2 == least_corner(r2))))
		{	/* True, if thing are generally turning downward */
		    temp = form_vertical_ordering(c2, c1);
		}
		else /* Things are turning generally upward */
		{
		    find_highest_exits(r1, r2, &c1, &c2);
		    temp = form_vertical_ordering(c1, c2);
		}
		result = temp;
	    }
	    break;
	    
	    case VERT_RT: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	    if ((r2->use == UR_CORNER) || (r2->use == LR_CORNER) || 
		(r2->use == RIGHT_U)) result = TRUE;
	    else if (r2->use == VERT_RT)
	    {
		/* Ye who exit to the right lower-most is right-most */
		temp = form_vertical_ordering(c1, c2);
		result = temp;
	    }
	    else result = FALSE;
	    break;
	    
	    case RIGHT_U: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	    if ((r2->use == UR_CORNER) || (r2->use == LR_CORNER)) result = TRUE;
	    else if (r2->use == RIGHT_U)
	    {
		/* place the shortest vertical segment right-most */	
		perpendicular_expanse(c1->cx, &min, &max);
		r1Extent = max - min;
		perpendicular_expanse(c2->cx, &min, &max);
		r2Extent = max - min;
		
		if (r1Extent >= r2Extent) result = TRUE;
		else result = FALSE;
	    }
	    else result = FALSE;
	    break;
	    
	    
	    case UR_CORNER: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	    if (r2->use == LR_CORNER) result = TRUE;
	    else if (r2->use == UR_CORNER)
	    {
		/* Ye who exit to the right lower-most is right-most */
		result = form_vertical_ordering(c1, c2);
	    }	
	    else result = FALSE;
	    break;
	    
	    
	    case LR_CORNER: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	    if (r2->use == LR_CORNER)
	    {
		/* Ye who exit to the right upper-most is right-most */
		result = form_vertical_ordering(c2, c1);
	    }
	    else result = FALSE;
	    break;
	    
	    
	    default:  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	    fprintf(stderr, "error in internal_horz_ordering; unknown ->use assignment\n");
	    result = TRUE;
	    break;
	}
    }

    /* Now check to see if there are any constraint problems: */
    if ((result == TRUE) && (constrained_in_x(r1, c2) == TRUE)) result = FALSE;
    else if ((result == FALSE) && (constrained_in_x(r2, c1) == TRUE)) result = TRUE;

    return(result);
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
int internal_vert_ordering(c1, c2)
    corner *c1, *c2;
{
    range *r1 = c1->cy, *r2 = c2->cy;
    int min, max, r1Extent, r2Extent, temp;

    /* return TRUE if <r1> should be placed on top of <r2>: */

    if (natural_break_p(r1, r2) == TRUE) return(follow_natural_break(r2, r1));

    if ((r1->use == NULL) || (r2->use == NULL))
    {
    	fprintf(stderr, "error in internal_vert_ordering; NULL ->use assignment\n");
	return(TRUE);
    }

    switch(r1->use)
    {
	case UL_CORNER: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - */ 
	if (r2->use != UL_CORNER) return(TRUE);
	else 
	{
	    /* Ye who exits to the left-most is upper-most */
	    return(form_horizontal_ordering(c1, c2));
	}
	break;


	case UR_CORNER: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	if (r2->use == UL_CORNER) return(FALSE);
	else if (r2->use == UR_CORNER)
	{
	    /* Ye who exit to the right-most is upper-most */
	    return(form_horizontal_ordering(c2, c1));
	}	
	else  return(TRUE);
	break;


	case UPWARD_U: /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	if ((r2->use == UL_CORNER) || (r2->use == UR_CORNER)) return (FALSE);
	else if (r2->use == UPWARD_U) 
	{
	    /* place the shortest horizontal segment upper-most */
	    perpendicular_expanse(c1->cy, &min, &max);
	    r1Extent = max - min;
	    perpendicular_expanse(c2->cy, &min, &max);
	    r2Extent = max - min;

	    if (r1Extent >= r2Extent) return(FALSE);
	    else return(TRUE);
	}
	else return(TRUE);
	break;


	case HORZ_UT: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	if ((r2->use == UL_CORNER) || (r2->use == UR_CORNER) ||
	    (r2->use == UPWARD_U)) return (FALSE);
	else if (r2->use == HORZ_UT)
	{
	    /* Ye who exits to the left-most is upper-most */
	    temp = form_horizontal_ordering(c1, c2);
	    return(temp);
	}
	else return(TRUE);
	break;


	case VERT_JOG:
	case VERT_LT:
	case VERT_RT:
	case LEFT_U:
	case RIGHT_U:/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	if ((r2->use == UL_CORNER) || (r2->use == UR_CORNER) ||
	    (r2->use == UPWARD_U) || (r2->use == HORZ_UT)) return (FALSE);
	else if ((r2->use == LL_CORNER) || (r2->use == LR_CORNER) ||
		 (r2->use == DOWNWARD_U) || (r2->use == HORZ_DT)) return(TRUE);
	else if ((r2->use == X) || (r2->use == HORZ_JOG)) return (TRUE);
	else
	{
	    /* These are a bit tricky: Follow the external restrictions; */
	    temp = form_vertical_ordering(c1, c2);
	    return(temp);
	}
	break;

	case X_CORNER:
	case HORZ_JOG: /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	if ((r2->use == UL_CORNER) || (r2->use == UR_CORNER) ||
	    (r2->use == UPWARD_U) || (r2->use == HORZ_UT)) return (FALSE);
	else if ((r2->use == LL_CORNER) || (r2->use == LR_CORNER) ||
		 (r2->use == DOWNWARD_U) || (r2->use == HORZ_DT)) return(TRUE);
	else if ((r2->use == VERT_JOG) || (r2->use == VERT_LT) ||
		 (r2->use == VERT_RT) || (r2->use == LEFT_U) ||
		 (r2->use == RIGHT_U)) return(FALSE);
	else 
	{
	    /* ye who enters the top right-most is lower-most.   This rule applies
	       unless there is interference that may lead the router to step on itself.*/
	    if (share_bound_p(r1, r2, &c1, &c2) == TRUE) 
	        temp = exits_top_p(c2);
	    else if ((find_lowest_exits(r1, r2, &c1, &c2)) &&
		     ((c1 == least_corner(r1)) || (c2 == least_corner(r2))))
	    {	/* True, if thing are generally turning downward */
		temp = form_horizontal_ordering(c2, c1);
	    }
	    else /* Things are turning generally upward */
	    {
		find_highest_exits(r1, r2, &c1, &c2);
		temp = form_horizontal_ordering(c1, c2);
	    }
	    return(temp);
	}
	break;

	case HORZ_DT: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	if ((r2->use == LL_CORNER) || (r2->use == LR_CORNER) || 
	    (r2->use == DOWNWARD_U)) return(TRUE);
	else if (r2->use == HORZ_DT)
	{
	    /* Ye who exit to the right-most is lower-most */
	    temp = form_horizontal_ordering(c1, c2);
	    return(temp);
	}
	else return(FALSE);
	break;

	case DOWNWARD_U: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	if ((r2->use == LL_CORNER) || (r2->use == LR_CORNER)) return(TRUE);
	else if (r2->use == DOWNWARD_U)
	{
	    /* place the shortest horizontal segment lower-most */	
	    perpendicular_expanse(c1->cy, &min, &max);
	    r1Extent = max - min;
	    perpendicular_expanse(c2->cy, &min, &max);
	    r2Extent = max - min;

	    if (r1Extent >= r2Extent) return(TRUE);
	    else return(FALSE);
	}
	else return(FALSE);
	break;


	case LL_CORNER: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	if (r2->use == LR_CORNER) return(FALSE);
	else if (r2->use == LL_CORNER) 
	{
	    /* Ye who exit to the left-most is lower-most */
	    return(form_horizontal_ordering(c2, c1));
	}
	else return(TRUE);
	break;


	case LR_CORNER: /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	if (r2->use == LR_CORNER)
	{
	    /* Ye who exit to the right-most is lower-most */
	    return(form_horizontal_ordering(c1, c2));
	}
	else return(FALSE);
	break;


	default:  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
	fprintf(stderr, "error in internal_vert_ordering; unknown ->use assignment\n");
	return(TRUE);
	break;
    }
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
void separate_corners_for_jog(c, middleC, nextC, a)    
    corner *c;
    corner **middleC, **nextC;
    asg_arc *a;
{
    /* This takes a corner belonging to a completely connected net, and breaks it,
       duplicating an X or Y range (HORZ or VERT arc).  All corners below <middleC>
       will be connected via the duplicate range, all those above by the original
       <linkRange>. */
    int i, len = 0;
    range *linkRange, *newLink;
    crnlist *cl, *newCornerList = NULL;

    if (a->page == VERT)
    {
	linkRange = c->cx;
	sort_list(linkRange->corners, in_x_order);	/* left-right ordering */
    }
    else
    {
	linkRange = c->cy;
	sort_list(linkRange->corners, in_y_order);	/* bottom-top ordering */
    }

    /* find the <middleC> */
    len = list_length(linkRange->corners);
    for (cl = linkRange->corners, i = 1; cl != NULL; cl = cl->next, i++)
    {
	/* Insure that <c> becomes either <middleC> or <nextC>: */
	if ( ((cl->this == c) && (i <= len/2)) ||    	/* <c> -> <middleC> */
	     (cl->next->this == c) )			/* <c> -> <nextC>   */
	{
	    *middleC = cl->this;
	    *nextC = cl->next->this;
	    newLink = copy_range(linkRange);	  /* Make the new range */
	    newLink->corners = newCornerList = cl->next;
	    cl->next = NULL;		            /* Clip the old corner list */
	    break;
	}
    }
    for (cl = newLink->corners, i = 1; cl != NULL; cl = cl->next, i++)
    {
	/* make the attachments for the remaining ranges: */
	if (a->page == VERT) cl->this->cx = newLink;
	else cl->this->cy = newLink;
    }
}
/*------------------------------------------------------------------------------------*/
range *set_horz_constraints(c1, c2, a)
    corner *c1, *c2;
    asg_arc *a;
{
    range *r1 = c1->cx, *r2 = c2->cx;
    range *newRange = NULL, *insert_jog();
    corner *leftJogC, *rightJogC;
    crnlist *cl, *cll;

    /* This function assigns pointers to corners attached to <c1> and <c2>
       that constrain their placement in the horizontal direction, that is
       when <c1> MUST be placed to the right or left of <c2> and vv.. */

    /* If such a condition exists, AND it causes a cycle, that is an overly-
       constrained placement problem, then an undesired (meaning otherwise 
       unnecessary) jog MUST be introduced to enable the problem to be solved. */

    /* Loop through all of the important corners of <r1>: */
    for (cl = r1->corners; cl != NULL; cl = cl->next)
    {
	/* Loop through all of the important corners of <r2>: */
	for (cll = r2->corners; cll != NULL; cll = cll->next)	
	{
	    if ((cl->this->cx->n != cll->this->cx->n) &&
		(fixed_point_p(cl->this->cy) == TRUE) && 
		(repeated_srange_p(cl->this->cy, cll->this->cy) == TRUE)) 
	    {
		if ((exits_right_p(cl->this) == TRUE) &&
		    (exits_right_p(cll->this) == FALSE))
		{
		    /* There's a problem... */
		    pushnew(c2, &r1->dep);		/* Record the dependency */

		    /* lookahead one step for the nasty 'completely constrained' 
		       problem, which is recognized as a simple cycle in the 
		       ->dep pointer-graph: */
		    if (member_p(c1, c2->cx->dep, identity) == TRUE)
		    {
			/* BIG Problem - now the cycle needs to be broken, and a jog 
			   arbitrarily inserted.  This will cause all sorts of problems.
			   */
			rem_item(c1, &r2->dep);	/* Break the cycle... */
			if (debug_level > 0)
			fprintf(stderr,"Nets %s and %s completely constrainted in X.\n", 
				r1->n->name, r2->n->name);
			separate_corners_for_jog(cl->this, &leftJogC, &rightJogC, a); 
			newRange = insert_jog(leftJogC, rightJogC, a, TRUE);
		    }
		}
		else if  ((exits_right_p(cl->this) == FALSE) &&
			  (exits_right_p(cll->this) == TRUE))
		{
		    /* There is a known dependency, but things are ok. */
		    pushnew(c1, &r2->dep);

		    /* lookahead one step for the nasty 'completely constrained' 
		       problem, which is recognized as a simple cycle in the 
		       ->dep pointer-graph: */
		    if (member_p(c2, c1->cx->dep, identity) == TRUE)
		    {
			/* BIG Problem - now the cycle needs to be broken, and a jog 
			   arbitrarily inserted.  This will cause all sorts of problems.
			   */
			rem_item(c2, &r1->dep);	/* Break the cycle... */
			if (debug_level > 0) 
			fprintf(stderr,"Nets %s and %s completely constrainted in X.\n", 
				r1->n->name, r2->n->name);
			separate_corners_for_jog(cl->this, &leftJogC, &rightJogC, a); 
			newRange = insert_jog(leftJogC, rightJogC, a, TRUE);
		    }
		}
	    }
	    /* Otherwise, we don't care. */
	}
    }
    return(newRange);	
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
range *set_vert_constraints(c1, c2, a)
    corner *c1, *c2;
    asg_arc *a;
{
    range *r1 = c1->cy, *r2 = c2->cy;
    range *newRange = NULL, *insert_jog();
    corner *botJogC, *topJogC;
    crnlist *cl, *cll;

    /* This function assigns pointers to corners attached to <c1> and <c2>
       that constrain their placement in the vertical direction, that is
       when <c1> MUST be placed to the right or left of <c2> and vv.. */

    /* If such a condition exists, AND it causes a cycle, that is an overly-
       constrained placement problem, then an undesired (meaning otherwise 
       unnecessary) jog MUST be introduced to enable the problem to be solved. */

    /* Loop through all of the important corners of <r1>: */
    for (cl = r1->corners; cl != NULL; cl = cl->next)
    {
	/* Loop through all of the important corners of <r2>: */
	for (cll = r2->corners; cll != NULL; cll = cll->next)	
	{
	    if ((cl->this->cx->n != cll->this->cx->n) &&
		(fixed_point_p(cl->this->cx) == TRUE) && 
		(repeated_srange_p(cl->this->cx, cll->this->cx) == TRUE)) 
	    {
		if ((exits_top_p(cl->this) == FALSE) &&
		    (exits_top_p(cll->this) == TRUE))
		{
		    /* There's a problem... */
		    pushnew(c2, &r1->dep);		/* Record the dependency */

		    /* lookahead one step for the nasty 'completely constrained' 
		       problem, which is recognized as a simple cycle in the ->dep 
		       pointer-graph: */
		    if (member_p(c1, c2->cy->dep, identity) == TRUE)
		    {
			/* BIG Problem - now the cycle needs to be broken, and a jog 
			   arbitrarily inserted.  This will cause all sorts of problems.
			   */
			rem_item(c1, &r2->dep);
			if (debug_level > 0)
			fprintf(stderr,"Nets %s and %s completely constrainted in Y.\n", 
				r1->n->name, r2->n->name);
			separate_corners_for_jog(cl->this, &botJogC, &topJogC, a);
			newRange = insert_jog(botJogC, topJogC, a, TRUE);
		    }	
		}
		else if ((exits_top_p(cl->this) == FALSE) &&
			 (exits_top_p(cll->this) == TRUE))
		{
		    /* There is a known dependency, but things are ok. */
		    pushnew(c1, &r2->dep);		/* Record the dependency */

		    /* lookahead one step for the nasty 'completely constrained' 
		       problem, which is recognized as a simple cycle in the ->dep 
		       pointer-graph: */
		    if (member_p(c2, c1->cy->dep, identity) == TRUE)
		    {
			/* BIG Problem - now the cycle needs to be broken, and a jog 
			   arbitrarily inserted.  This will cause all sorts of problems.
			   */
			rem_item(c2, &r1->dep);
			if (debug_level > 0)
			fprintf(stderr,"Nets %s and %s completely constrainted in Y.\n", 
				r2->n->name, r1->n->name);
			separate_corners_for_jog(cl->this, &botJogC, &topJogC, a);
			newRange = insert_jog(botJogC, topJogC, a, TRUE);
		    }	
		}

	    }
	    /* Otherwise, we don't care. */
	}
    }
    return(newRange);
}

/*------------------------------------------------------------------------------------*/
int constrained_in_x(r, c)
    range *r;
    corner *c;
{
    /* return TRUE if the corner <c> appears anywhere along the dependency chain
       of <r>. */
    /* This had better be an acyclic graph. */
    crnlist *cl;

    for (cl = r->dep; cl != NULL; cl = cl->next)
    {
	if (cl->this == c) return(TRUE);
	if (constrained_in_x(cl->this->cx, c) != FALSE) return(TRUE);
    }
    return(FALSE);
}
/*------------------------------------------------------------------------------------*/
int constrained_in_y(r, c)
    range *r;
    corner *c;
{
    /* return TRUE if the corner <c> appears anywhere along the dependency chain
       of <r>. */
    /* This had better be an acyclic graph. */
    crnlist *cl;

    for (cl = r->dep; cl != NULL; cl = cl->next)
    {
	if (cl->this == c) return(TRUE);
	if (constrained_in_y(cl->this->cy, c) != FALSE) return(TRUE);
    }
    return(FALSE);
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
void evaluate_corner_placement_constraints(cornersOfNote, a)
    crnlist **cornersOfNote;
    asg_arc *a;
{
    range *r = NULL;
    rnglist *rl, *jogList = NULL;
    corner *c = (*cornersOfNote)->this;
    crnlist *cl, *cll;

    r = (a->page == VERT) ? c->cx : c->cy;

    /* Record the physical constraints on the ranges: */

    for(cl = *cornersOfNote; cl != NULL; cl = cl->next)
    {
        if (cl->next != NULL)
	{
	    for(cll = cl->next; cll != NULL; cll = cll->next)
	    {
		r = (a->page == VERT) ?
		    set_horz_constraints(cl->this, cll->this, a):
		    set_vert_constraints(cl->this, cll->this, a);
		if (r != NULL)		    
		{
		    /* Somehow add this range into the set being considered */
		    push(r, &jogList);
		}
	    }
	}
    }
    /* For each range located (there is one per jog introduced), add the 
       corresponding corner to the <cornersOfNote> list: */
    for(rl = jogList; rl != NULL; rl = rl->next)
    {
	for(cl = rl->this->corners; cl != NULL; cl = cl->next)
	if (member_p(cl->this, *cornersOfNote, identity) != TRUE)
	{
	    push(cl->this, cornersOfNote);
	    break;
	}
    }
}

/*------------------------------------------------------------------------------------*/
void horz_decouple_corners(a)
    asg_arc *a;
{
    /* This function decouples the given set of corners in the vertical
       dimension.  All of these corners are in the same arc and need decoupling.
       This function aims at decoupling all of the vertical segments in the
       given set of corners, which is the unique set of x ranges.

       This is accomplished by forming a list of all horizontal ranges that exit on 
       the left side of the box, and those that exit on the right.  These are ordered
       (bottom to top) based on their external connections.

       Given this ordering, a given vertical segment can be ordered based on the 
       following set of rules aimed at optimizing the internal characteristics of 
       the box:

       Looking from the right-side of the box:
       The segment that is part of a uturn with the shortest y extent is right most,
       followed in turn by all other uturns.  All segments that form an L (angle)
       through the top and right are ordered in order of their top-bottom ordering
       (top-most -> right-most).  Then the segments that form Z or bottom L turns are 
       ordered left to right as per their top-bottom ordering (top-most -> left-most).  
       The process is then repeated for the left-side.

       NOTE: the only special case is with T formations, which are considered as
       Z arangements as opposed to their U or L arrangements. 
       */

    crnlist *cl, *cll, *temp, *cornersOfNote = NULL;
    range *r, *set_horz_constraints();
    rnglist *unique, *uniqueRanges = NULL;
    list *nonOverlap, *l;
    int horzIndex = 0, maxIndex = list_length(a->corners);
    
    /* exit quickly if there's nothing to do... */
    if (a->corners == NULL) return;

/* Form the ordering that defines how the ranges are to be decoupled: */
    /* Start by characterizing the usage of the vertical segments in the tile: */
    for (cl = a->corners; cl != NULL; cl = cl->next)
    {
	/* Each unique x-range represents a segment that runs parallel to the 
	   Y-Axis, and probably two corners in the box (most formations are Z,
	   rather than L arrangements).  
	   By the way, these are the segements that we need to decouple.*/

	cl->this->horzOrder = maxIndex;	    /* Need to insure that the ordering scheme 
					       used by "in_reasonable_horz_orderR" 
					       works */
	unique = (rnglist *) member(cl->this->cx->sr, uniqueRanges, check_sranges);
	if (unique == NULL)
	{
	    r = cl->this->cx;
	    r->use = 0;

	    push(cl->this, &cornersOfNote);
	    push(r, &uniqueRanges);
	    if (range_exits_bottom(r, a) == TRUE) r->use += BOT_EDGE;
	    if (range_exits_top(r, a) == TRUE) r->use += TOP_EDGE;
	    if (range_exits_left(cl->this->cy, a) == TRUE) r->use += LEFT_EDGE;
	    if (range_exits_right(cl->this->cy, a) == TRUE) r->use += RIGHT_EDGE;
	}
	else /* treat Jog/ U combinations as U's: (gives better aesthetics) */
	{
	    r = unique->this;
	    if (range_exits_left(cl->this->cy, a) == TRUE)
	    {
		if (r->use == RIGHT_U)	r->use += LEFT_EDGE;
		else if (r->use == LEFT_EDGE + RIGHT_EDGE) r->use -= RIGHT_EDGE; 
	    }
	    else if (range_exits_right(cl->this->cy, a) == TRUE) 
	    {
		if (r->use == LEFT_U) r->use += RIGHT_EDGE;
 		else if (r->use == LEFT_EDGE + RIGHT_EDGE) r->use -= LEFT_EDGE; 
	    }
	}
	if (r->use == NULL)
	{
	    r->use = VERT_JOG;
	}
    }

    /* Setup constraints, and add jogs as required: */
    evaluate_corner_placement_constraints(&cornersOfNote, a);


    /* Now each range of interest is listed in <uniqueRanges> and has been characterized
       for its usage within the given arc.  Given this arrangement, the ranges are
       ordered, first for aesthetics, and then for physical constraints.  */
    merge_sort_list(&cornersOfNote, internal_horz_ordering); 
    uniqueRanges = (rnglist *)free_list(uniqueRanges);

    /* Now map this corner list into the all-important range list: */
    for (cl = cornersOfNote; cl != NULL; cl = cl->next)
    {
	queue(cl->this->cx, &uniqueRanges);	/* Use queue to preserve order... */
	cl->this->horzOrder = horzIndex++;	/* Mark the ordering for the sorter */
    }


    /* There are several problems to be resolved - the first of which is the 
       column reuse problem - If permitted, then there are subsets of ranges that
       do not interfere with each other.  To solve this, the ordered set of corners
       is broken into a set of ordered ranges;  The longest set of these (interfering)
       ranges is then decoupled, and then the next longest, until all ranges have
       been checked.  */

    /* First - form sets of non-overlapping ranges:  That is, to be part of a set, 
       the given corner must overlapp with ALL of the corners in the set;  From
       this set use the L->R ordering to form a dependency graph..	*/

    /* From this list of ranges, develop a dependency graph: */
    nonOverlap = form_dependency_graph(uniqueRanges);
    
    /* OK - Now the dependency graph has been made - so use it... */
    for(l = sort_list(nonOverlap, in_length_order); l != NULL; l = l->next)
    {
	unique = (rnglist *)slow_sort_list(rcopy_list(l->this), in_reasonable_horz_orderR); 
	decouple_ordered_ranges(unique);
	free_list(l->this);
	free_list(unique);
    }

    free_list(uniqueRanges);
    free_list(nonOverlap);
}
/*------------------------------------------------------------------------------------*/
void vert_decouple_corners(a)
    asg_arc *a;
{
    /* This function decouples the given set of corners in the vertical
       dimension.  All of these corners are in the same asg_arc and need decoupling.
       This function aims at decoupling all of the vertical segments in the
       given set of corners, which is the unique set of x ranges.

       This is accomplished by forming a list of all horizontal ranges that exit on 
       the left side of the box, and those that exit on the right.  These are ordered
       (bottom to top) based on their external connections.

       Given this ordering, a given vertical segment can be ordered based on the 
       following set of rules aimed at optimizing the internal characteristics of 
       the box:

       Looking from the right-side of the box:
       The segment that is part of a uturn with the shortest y extent is right most,
       followed in turn by all other uturns.  All segments that form an L (angle)
       through the top and right are ordered in order of their top-bottom ordering
       (top-most -> right-most).  Then the segments that form Z or bottom L turns are 
       ordered left to right as per their top-bottom ordering (top-most -> left-most).  
       The process is then repeated for the left-side.

       NOTE: the only special case is with T formations, which are considered as
       Z arangements as opposed to their U or L arrangements. 
       */

    crnlist *cl, *cll, *cornersOfNote = NULL;
    range *r, *set_vert_constraints();
    rnglist *unique, *uniqueRanges = NULL;
    list *nonOverlap, *l;
    int vertIndex = 0, maxIndex = list_length(a->corners);
    
    /* exit quickly if there's nothing to do... */
    if (a->corners == NULL) return;

/* Form the ordering that defines how the ranges are to be decoupled: */
    /* Start by characterizing the usage of the vertical segments in the tile: */
    for (cl = a->corners; cl != NULL; cl = cl->next)
    {
	/* Each unique x-range represents a segment that runs parallel to the 
	   Y-Axis, and probably two corners in the box (most formations are Z,
	   rather than L arrangements).  
	   By the way, these are the segements that we need to decouple.*/
	
	cl->this->vertOrder = maxIndex;	    /* Need to insure that the ordering scheme 
					       used by "in_reasonable_vert_orderR" 
					       works */
	unique = (rnglist *) member(cl->this->cy->sr, uniqueRanges, check_sranges);
	if (unique == NULL)
	{
	    r = cl->this->cy;
	    r->use = 0;

	    push(cl->this, &cornersOfNote);
	    push(r, &uniqueRanges);
	    if (range_exits_bottom(cl->this->cx, a) == TRUE) r->use += BOT_EDGE;
	    if (range_exits_top(cl->this->cx, a) == TRUE) r->use += TOP_EDGE;
	    if (range_exits_left(r, a) == TRUE) r->use += LEFT_EDGE;
	    if (range_exits_right(r, a) == TRUE) r->use += RIGHT_EDGE;
	}
	else /* Classify Jog/U combinations as U's: (gives better aesthetics) */
	{
	    r = unique->this;
	    if (range_exits_bottom(cl->this->cx, a) == TRUE)
	    {
		if (r->use == UPWARD_U) r->use += BOT_EDGE;
		else if (r->use == BOT_EDGE + TOP_EDGE) r->use -= TOP_EDGE;  
	    }
	    else if (range_exits_top(cl->this->cx, a) == TRUE)
	    {
		if (r->use == DOWNWARD_U) r->use += TOP_EDGE;
 		else if (r->use == BOT_EDGE + TOP_EDGE) r->use -= BOT_EDGE; 
	    }
	}
	if (r->use == NULL)
	{
	    r->use = HORZ_JOG;
	}
    }

    /* Setup constraints, and add jogs as required: */
    evaluate_corner_placement_constraints(&cornersOfNote, a);


    /* Now each range of interest is listed in <uniqueRanges> and has been characterized
       for its usage within the given asg_arc.  Given this arrangement, the ranges are
       ordered, first for aesthetics, then for physical constraints.  */
    merge_sort_list(&cornersOfNote, internal_vert_ordering); 
    uniqueRanges = (rnglist *)free_list(uniqueRanges);

    /* Now map this corner list into the all-important range list: */
    for (cl = cornersOfNote; cl != NULL; cl = cl->next)
    {
	queue(cl->this->cy, &uniqueRanges);	/* Use queue to preserve order... */
	cl->this->vertOrder = vertIndex++;	/* Mark the ordering for the sorter */
    }


    /* There are several problems to be resolved - the first of which is the 
       column reuse problem - If permitted, then there are subsets of ranges that
       do not interfere with each other.  To solve this, the ordered set of corners
       is broken into a set of ordered ranges;  The longest set of these (interfering)
       ranges is then decoupled, and then the next longest, until all ranges have
       been checked.  */

    /* First - form sets of non-overlapping ranges:  That is, to be part of a set, 
       the given corner must overlapp with ALL of the corners in the set;  From
       this set use the L->R ordering to form a dependency graph..	*/

    /* From this list of ranges, develop a dependency graph: */
    nonOverlap = form_dependency_graph(uniqueRanges);
    
    /* OK - Now the dependency graph has been made - so use it... */
    for(l = sort_list(nonOverlap, in_length_order); l != NULL; l = l->next)
    {
	unique = (rnglist *)slow_sort_list(l->this, in_reasonable_vert_orderR); 
	decouple_ordered_ranges(unique);
	free_list(l->this);
    }

    free_list(uniqueRanges);
    free_list(nonOverlap);
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
void form_into_nonoverlapping_ranges(tl)
    tilist *tl;
{
    tilist *tll;
    asg_arc *a;
    crnlist *cl;

    int x1, y1, x2, y2;

    for (tll = tl; tll != NULL; tll = tll->next)
    {
	/* Walk down this ordered list of asg_arcs, resolving the overlapped ranges */
	a = (asg_arc *)tll->this->this;

	/* Divide the ranges listed in the corner structures into two lists, so they can 
	   be compared to each other: */
	
	if (debug_level == 4) 
	{
	    if (a->page == VERT)
	    {
		x1 = tll->this->y_pos;		y1 = tll->this->x_pos;	
		x2 = x1 + tll->this->y_len;	y2 = y1 + tll->this->x_len;
	    }
	    else
	    {
		x1 = tll->this->x_pos;		y1 = tll->this->y_pos;	
		x2 = x1 + tll->this->x_len;	y2 = y1 + tll->this->y_len;
	    }
	    
	fprintf(stderr, "\nExamining box at (%d, %d) to (%d, %d)[%s]\n",
		x1, y1, x2, y2, (a->page == VERT) ? "v" : "h");
	}

	if (a->page == VERT) horz_decouple_corners(a);
	else vert_decouple_corners(a);
    }
}
/*------------------------------------------------------------------------------------*/
int link_value(r)
    range *r;
{
    if (r != NULL) return (r->sr->q2 - r->sr->q1);
    else return (HASH_SIZE * 10);			/* Some big number */
}
/*------------------------------------------------------------------------------------*/
trail *locate_parent_trail(n, c)
    net *n;
    corner *c;
{
    /* given a corner, and the net to which it belongs, locate the expansion that
       contains the corner (VERY SPECIAL -> depends on the trail->used pointers) */

    expn *ex = (expn *)n->done->this;
    trlist *trl;
    crnlist *cl;

    for (trl = ex->connections; trl != NULL; trl = trl->next)
    {
	for (cl = trl->this->used; cl != NULL; cl = cl->next)
	{
	    if (cl->this == c) return(trl->this);
	}
    }
    return(NULL);
}
	    
/*------------------------------------------------------------------------------------*/
tile *locate_for_corner(ts, x, y)
    tile * ts;
    int x, y;
{
    /* Locate a tile in the tilespace dentoed by <ts> at the given point. 
       Because this is used for locating a corner, the boundry conditions for 
       location are modified from those given in "locate" as defined in cstitch.c. */

    tile *t = locate(ts, x, y);
    asg_arc *a = (t != NULL) ? (asg_arc *)t->this : NULL;

    if (a->mod != NULL) 
    {
	if (t->x_pos + t->x_len == x) return(locate(t, ++x, y));
	if (t->x_pos == x) return(locate(t, --x, y));
	if (t->y_pos + t->y_len == y) return(locate(t, x, ++y));
	if (t->y_pos == y) return(locate(t, x, --y));
    }
    return(t);
}

/*------------------------------------------------------------------------------------*/
corner* create_linkage_between(c1, c2, linkRange, a, forcedJog)
    corner *c1, *c2;
    range *linkRange;
    asg_arc *a;
    int forcedJog;
{
    int orient = a->page;
    corner *link1, *link2;
    tile *newTile1, *newTile2;
    asg_arc *newArc;
    arclist *al;
    net *n = c1->cx->n;
    trail *tr = locate_parent_trail(n, c1);

    if (a->page == VERT)
    {
	link1 = create_corner(c1->cx, linkRange, c1->al->this, NULL);
	newTile1 = locate_for_corner(routingRoot[HORZ][0], midpoint(c1->cx), 
				     midpoint(linkRange));
	add_corner_to_arc(link1, newTile1->this);
	for (al = c1->al->next; al != NULL; al = al->next) 
	    add_corner_to_arc(link1, al->this);
	    
	link2 = create_corner(c2->cx, linkRange, c2->al->this, NULL);
	newTile2 = locate_for_corner(routingRoot[HORZ][0], midpoint(c2->cx), 
				     midpoint(linkRange));
	add_corner_to_arc(link2, newTile2->this);
	for (al = c2->al->next; al != NULL; al = al->next) 
	    add_corner_to_arc(link2, al->this);
    }

    else  /* (a->Page == HORZ)  */
    {
	link1 = create_corner(linkRange, c1->cy, c1->al->this, NULL);
	newTile1 = locate_for_corner(routingRoot[VERT][0], midpoint(c1->cy), 
				     midpoint(linkRange));
	add_corner_to_arc(link1, newTile1->this);
	for (al = c1->al->next; al != NULL; al = al->next)
	    add_corner_to_arc(link1, al->this);
	    
	link2 = create_corner(linkRange, c2->cy, c2->al->this, NULL);
	newTile2 = locate_for_corner(routingRoot[VERT][0], midpoint(c2->cy), 
				     midpoint(linkRange));
	add_corner_to_arc(link2, newTile2->this);
	for (al = c2->al->next; al != NULL; al = al->next) 
	    add_corner_to_arc(link2, al->this);
    }
    if (tr != NULL) 	/* At a later stage of the local router (this is a hack) */
    {
	push(link1, &tr->used);
	pushnew(newTile1, &n->route);
	push(link2, &tr->used);
	pushnew(newTile2, &n->route);    
    }
    else if (forcedJog == TRUE) 	/* When the jog has been forced... */
    {
	push(link1, &n->route);
	push(link2, &n->route);
    }
    
    if (debug_level > 0)
    if (tr != NULL)
    fprintf(stderr, "Adding jog to <%s>{%s %s} from ([%d,%d],[%d,%d]) to ([%d,%d],[%d,%d]) \n",
	    n->name, tr->ex->t->name, tr->ex->t->mod->name, 
	    link1->cx->sr->q1, link1->cx->sr->q2, link1->cy->sr->q1, link1->cy->sr->q2,
	    link2->cx->sr->q1, link2->cx->sr->q2, link2->cy->sr->q1, link2->cy->sr->q2);
    else
    fprintf(stderr, "Adding jog to <%s> from ([%d,%d],[%d,%d]) to ([%d,%d],[%d,%d]) \n",
	    n->name,
	    link1->cx->sr->q1, link1->cx->sr->q2, link1->cy->sr->q1, link1->cy->sr->q2,
	    link2->cx->sr->q1, link2->cx->sr->q2, link2->cy->sr->q1, link2->cy->sr->q2);

    return(link1);
}
/*------------------------------------------------------------------------------------*/
range *insert_jog(c, linkC, a, forcedJog)
    corner *c, *linkC;
    asg_arc *a;
    int forcedJog;	/* Flag */
{
    /* This adds the corners necessary to <a> (if any) to connect <c> to <c2>. */
       
    /* There are two cases: either a contains a range that can be exploited to link the
       two, or it doesn't. */

    corner *link = NULL;
    crnlist *cl;
    range *linkRange;
    int min, max;

    for (cl = a->corners; ((cl != NULL) && (forcedJog == FALSE)); cl = cl->next)
    {
	if ((cl->this->cx->n == c->cx->n) &&	      	/* Only unique corners of */
	    ((cl->this != c) && (cl->this != linkC)))	/* the same net qualify.  */
	{
	    if (a->page == VERT)
	    {
		if ((overlapped_p(cl->this->cx->sr, c->cx->sr) == TRUE) &&
		    (overlapped_p(cl->this->cx->sr, linkC->cx->sr) == TRUE))
		{
		    link = create_linkage_between(c, linkC, cl->this->cy, a, forcedJog);
		    break;
		}
	    }
	    else /* (a->Page == HORZ)  */
	    {
		if ((overlapped_p(cl->this->cy->sr, c->cy->sr) == TRUE) &&
		    (overlapped_p(cl->this->cy->sr, linkC->cy->sr) == TRUE))
		{
		    link = create_linkage_between(c, linkC, cl->this->cx, a, forcedJog);
		    break;
		}
	    }
	}
    }

    /* OK - There's nothing to leverage from... */
    if (link == NULL)
    {
	if (a->page == VERT)
	{
	    min = MIN(linkC->cy->sr->q1, c->cy->sr->q1);
	    max = MAX(linkC->cy->sr->q2, c->cy->sr->q2);

	    if (min != max)
	    {
		linkRange = create_range(create_srange(min, max, HORZ), c->cx->n);
	    }
	    else 
	    {
		/* There is the odd case of where a U needs to be formed, in which case
		   the link range min/max is determined by the tile that the corners
		   fall into. */

		if (a->t->x_pos == min)  /* Left-side U */
		{
		    linkRange = create_range
		    (create_srange(min, a->t->x_pos + a->t->x_len, HORZ), c->cx->n);
		}
		else if (a->t->x_pos + a->t->x_len == max)  /* Right-side U */
		{
		    linkRange = create_range
		    (create_srange(min, a->t->x_pos, HORZ), c->cx->n);
		}
		else /* NO CLUE! */
		{
		    linkRange = create_range(create_srange(min, max, HORZ), c->cx->n);
		}		    
	    }
	    create_linkage_between(c, linkC, linkRange, a, forcedJog);
	}
	else
	{
	    min = MIN(linkC->cx->sr->q1, c->cx->sr->q1);
	    max = MAX(linkC->cx->sr->q2, c->cx->sr->q2);

	    if (min != max)
	    {
		linkRange = create_range(create_srange(min, max, VERT), c->cx->n);
	    }
	    else
	    {
		/* There is the odd case of where a U needs to be formed */
		if (a->t->x_pos == min)  /* Left-side U */
		{
		    linkRange = create_range
		    (create_srange(min, a->t->x_pos + a->t->x_len, VERT), c->cx->n);
		}
		else if (a->t->x_pos + a->t->x_len == max)  /* Right-side U */
		{
		    linkRange = create_range
		    (create_srange(min, a->t->x_pos, VERT), c->cx->n);
		}
		else /* NO CLUE! */
		{
		    linkRange = create_range(create_srange(min, max, VERT), c->cx->n);
		}		    
	    }
	    create_linkage_between(c, linkC, linkRange, a, forcedJog);
	}
    }
    return(linkRange);
}
/*------------------------------------------------------------------------------------*/
corner *choose_best_link(n, a, setJogsP)
    net *n;
    asg_arc *a;
    int setJogsP;
{
    /* find the best link in the given direction among the ranges given */
    /* This is done by running through the candidates for the best link, and
       counting the number of jogs that they require.  the one with the lowest 
       count wins.  (This is still pretty arbitrary) 
       */
       
    /* There is a difficult problem to catch in the determination of the need for
       a jog - that is when there is a corner that two (or more) other corners
       can jog to, but if one jogs to it, the ensuing constraint does not 
       permit the successive corners from being able to jog.

       To avoid this problem, temp ranges are maintained and used to indicate
       where added jogs will be required due to dependencies among the
       links saught.
       */

    crnlist *cl, *cll;
    corner *linkCorner = NULL;
    int orientation = a->page;
    range *bestLink = NULL, *testLink = NULL;
    range *testLinkCopy, *copy, *r;
    int count, bestCount = (HASH_SIZE * 50) - 1;		/* Some big number */

    for (cl = a->corners; cl != NULL; cl = cl->next)
    {
	/* Try cl->this as the <testLink> corner: */
	/* Only pay attention to corners belonging to the given net: */
	if ((cl->this->cy != NULL) && (cl->this->cy->n == n))
	{
	    count = 0;
	    testLink = (orientation == VERT) ? cl->this->cx : cl->this->cy;
	    testLinkCopy = quick_copy_range(testLink);

	    for (cll = a->corners; cll != NULL; cll = cll->next)
	    {
		/* Only pay attention to corners belonging to the given net: */
		if ((cll != cl) && 
		    ((cll->this->cy != NULL) && (cll->this->cy->n == n)))
		{
		    if (orientation == VERT)
		    {
			r = cl->this->cx;		/* Used for debugging */
			if (overlapped_p(cll->this->cx->sr, testLinkCopy->sr) != TRUE)
			{
			    /* need to insert a jog of some sort.... */
			    count++;
			}
                        else /* Restrict the <testLinkCopy> */
			{
			    copy = quick_copy_range(cll->this->cx);
			    reduce_to_least_common_range(testLinkCopy->sr, copy->sr);
			    free(copy);
			}
		    }		    
		    else
		    {
			r = cl->this->cy;
			if (overlapped_p(cll->this->cy->sr, testLinkCopy->sr) != TRUE) 
			{
			    /* time to insert a jog of some sort.... */
			    count++;
			}
                        else /* Restrict the <testLinkCopy> */
			{
			    copy = quick_copy_range(cll->this->cy);
			    reduce_to_least_common_range(testLinkCopy->sr, copy->sr);
			    free(copy);
			}
		    }
		    if (count > bestCount) break;
		}
	    } /* end inner loop */


	    /* Now check and conditionally update the bestLink...*/
	    if ((count <= bestCount) && 
		((bestLink == NULL) ||(link_value(testLink) >= link_value(bestLink))))
	    {
		linkCorner = cl->this;
		bestLink = quick_copy_range(testLinkCopy);
		bestCount = count;
	    }

	    free(testLinkCopy);		/* Collect Garbage... */

	} /* end IF right net...*/
    } /* end outer loop */

    /* Hopefully, the best link corner has now been discovered, so use it <linkCorner>. 
       See if any jogs need to be calculated: */

    if (setJogsP == TRUE)
    {
	/* cleanup requested, so do it... */
	for (cl = a->corners; cl != NULL; cl = cl->next)
	{
	    /* Only pay attention to corners belonging to the given net: */
	    if ((cl->this != linkCorner) && 
		(cl->this->cx != NULL) && (cl->this->cx->n == n))
	    {
		if (orientation == VERT)
		{
		    if (overlapped_p(cl->this->cx->sr, bestLink->sr) != TRUE)
		    {
			insert_jog(cl->this, linkCorner, a, FALSE);
		    }
		}
		else
		{
		    if (overlapped_p(cl->this->cy->sr, bestLink->sr) != TRUE) 
		    {
			insert_jog(cl->this, linkCorner, a, FALSE);			
		    }
		}
	    }
	}
    }	
    free(bestLink);	/* Garbage */

    return(linkCorner);
}
/*------------------------------------------------------------------------------------*/
corner *old_choose_best_link(n, a, setJogsP)
    net *n;
    asg_arc *a;
    int setJogsP;
{
    /* find the best link in the given direction among the ranges given */

    crnlist *cl, *cll;
    corner *linkCorner = NULL;
    int orientation = a->page;
    range *bestLink = NULL;
    int linkVal = (HASH_SIZE * 50) - 1;			/* Some big number */;

    for (cl = a->corners; cl != NULL; cl = cl->next)
    {
	/* Only pay attention to corners belonging to the given net: */
	if ((cl->this->cy != NULL) && (cl->this->cy->n == n)) 
	{
	    if (orientation == VERT)
	    {
		if ((bestLink == NULL) ||
		    (link_value(cl->this->cx) <= linkVal))
		{
		    if ((bestLink != NULL) && (setJogsP == TRUE) &&
			(overlapped_p(cl->this->cx->sr, bestLink->sr) != TRUE))
		    {
			/* time to insert a jog of some sort.... */
			insert_jog(cl->this, linkCorner, a, FALSE);
		    }
		    linkCorner = cl->this;
		    bestLink = cl->this->cx;
		    linkVal = link_value(bestLink);  /* BUGG ??? */
		}
	    }
	    
	    else
	    {
		if ((bestLink == NULL) ||
		    (link_value(cl->this->cy) <= linkVal))
		{
		    if ((bestLink != NULL) && (setJogsP == TRUE) &&
			(overlapped_p(cl->this->cy->sr, bestLink->sr) != TRUE)) 
		    {
			/* time to insert a jog of some sort.... */
			insert_jog(cl->this, linkCorner, a, FALSE);			
		    }
		    linkCorner = cl->this;
		    bestLink = cl->this->cy;
		    linkVal = link_value(bestLink);
		}
	    }
	}
    }
    return(linkCorner);
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
int adjacent_p(t1, t2, axis)
    tile *t1, *t2;
    int axis;
{
    if ((axis == X) && 
	((t1->x_pos == t2->x_pos + t2->x_len) ||
	 (t2->x_pos == t1->x_pos + t1->x_len)))	return(TRUE);
    else if ((axis == Y) && 
	     ((t1->y_pos == t2->y_pos + t2->y_len) ||
	      (t2->y_pos == t1->y_pos + t1->y_len)))	return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------*/
int free_to_include_p(t)
    tile *t;
{
    /* Returns TRUE if the tile is free for routing, and completly contains the
       spanRange (global)... */

    asg_arc *a = (t != NULL) ? (asg_arc *)t->this : NULL;
    int x1, x2;
    
    x1 = t->x_pos;    	x2 = x1 + t->x_len;

    if ((a != NULL) && (a->mod == NULL)) 
    {
	/* Look for overlap */
	if ((x1 <= spanRange.q1) && (x2 >= spanRange.q2)) return(TRUE);
    }

    return(FALSE);
}
/*------------------------------------------------------------------------------------*/
tilist *valid_slivers(root, x1, y1, x2, y2, min, max)
    tile *root;
    int x1, y1, x2, y2;
    int *min, *max;
{
    int started = FALSE, foundRoot = FALSE;
    tile *t;
    tilist *tl, *temp = NULL, *valid = NULL;
    
    spanRange.q1 = x1;	
    spanRange.q2 = x2;

    temp = area_enumerate(root, &temp, free_to_include_p, x1, y1, x2 - x1, y2 - y1);

    /* Now evaluate for continuity, and set the *min, *max values: */
    for (tl = (tilist *)sort_list(temp, most_down); tl != NULL; tl = tl->next)
    {
	t = tl->this;

	/* Should contain some of y, and all of x: */
	if ((t == root) ||
	    ((t->x_pos <= x1) && (t->x_pos + t->x_len >= x2) &&
	     (t->y_pos <= y2) && (t->y_pos + t->y_len >= y1)))
	{
	    if (valid == NULL) push(t, &valid);
	    /* Be sure to include <root> in the list... (Otherwise, start over...) */
	    else if (adjacent_p(t, valid->this, Y) == TRUE) push(t, &valid);
	    else if (foundRoot == FALSE) 
	    {
		valid = (tilist *)free_list(valid); 
		push(t, &valid);
		*min = MAX(y1, t->y_pos);
	    }
	    else break;

	    if (t == root) foundRoot = TRUE;
	    if (started == FALSE) 
	    {
		*min = MAX(y1, t->y_pos);
		started = TRUE;
	    }
	}
    }
    
    *max = (valid != NULL) ? MIN(y2, valid->this->y_pos + valid->this->y_len) : FALSE;

    free_list(temp);
    return(valid);
}
/*------------------------------------------------------------------------------------*/
find_and_merge_slivers(a, n)
    asg_arc *a;
    net *n;
{
    /* Look to see if there are other adjacent asg_arcs/tiles that can be used.
       If so, then modify the corner limits, and do the book keeping for the 
       asg_arc/corner structures...*/

    /* All such slivers located are added to the n->route list */

    int contRangeMin, contRangeMax;
    int minX, minY, maxX, maxY;
    tilist *tl, *sliverSet;
    asg_arc *ar;
    srange *xSR, *ySR, *x1SR, *y1SR, *x2SR, *y2SR;
    corner *prev, *next;
    crnlist *cl, *cll;

    for (cl = a->corners; cl != NULL; cl = cl->next)
    {
	/* Only look at corners belonging to this net: */
	if (((cl->this->cy != NULL) && (cl->this->cy->n == n)) ||
	    ((cl->this->cx != NULL) && (cl->this->cx->n == n)))
	{
	    ySR = cl->this->cy->sr;		xSR = cl->this->cx->sr;

	    if ((a->page == VERT) && (fixed_point_p(cl->this->cx) == FALSE))
	    {
		/* Find the corners of the legal routing space: (mega-tile) */
		next = next_horz_corner(most_vert_corner(cl->this));
		y1SR = next->cy->sr;		x1SR = next->cx->sr;
		prev = next_horz_corner(least_vert_corner(cl->this));
		y2SR = prev->cy->sr;		x2SR = prev->cx->sr;

		minX = MIN(xSR->q1, MIN(x1SR->q1, x2SR->q1));		
		minY = MIN(ySR->q1, MIN(y1SR->q1, y2SR->q1));
		maxX = MAX(xSR->q2, MAX(x1SR->q2, x2SR->q2));		
		maxY = MAX(ySR->q2, MAX(y1SR->q2, y2SR->q2));

		/* Special case to detect vertical U's: */
		if ((x1SR->q2 <= xSR->q1) && (x2SR->q2 <= xSR->q1))
		{
		    /* left-pointing U */
		    maxX += WHITE_SPACE;
		    if (debug_level > 0) fprintf(stderr," left-pointing U for net %s\n", n->name);
		}
		    
		else if ((x1SR->q1 >= xSR->q2) && (x2SR->q2 >= xSR->q2))
		{
		    /* right-pointing U */
		    minX -= WHITE_SPACE;
		    if (debug_level > 0) fprintf(stderr," right-pointing U for net %s\n", n->name);
		}

		sliverSet = valid_slivers(a->t, minY, minX, maxY, maxX, 
					  &contRangeMin, &contRangeMax);

		if (member_p(a->t, sliverSet, identity) == FALSE)
		{
		    /* Major Problem... */
		    fprintf(stderr, "(v) sliver detector screwed {(%d,%d) & (%d,%d)}\n",
			    minX, minY, maxX, maxY);
		    fprintf(stderr, "given %d tiles, couldn't find @(%d, %d) for net %s\n",
			    list_length(sliverSet), a->t->y_pos + (a->t->y_len/2),
			    a->t->x_pos + (a->t->x_len/2), n->name);

		    return(NULL);
		}
		else
		{
		    if ((cl->this->cx->sr->q1 > contRangeMin) ||
			(cl->this->cx->sr->q2 < contRangeMax))
		    {
			if ((debug_level >= 2) && (sliverSet->next != NULL))
			{
			    fprintf(stderr, "<%s> expanding from x = [%d,%d] to [%d,%d] (%d slivers added)\n",
				    cl->this->cx->n->name, cl->this->cx->sr->q1,
				    cl->this->cx->sr->q2, contRangeMin, contRangeMax,
				    list_length(sliverSet) - 1);
			}

			/* Expand the range: */
			cl->this->cx->sr->q1 = MIN(contRangeMin, cl->this->cx->sr->q1);
			cl->this->cx->sr->q2 = MAX(contRangeMax, cl->this->cx->sr->q2);
			
			/* Add the expanded corners to all tiles that it uses. */
			for(tl = sliverSet; tl != NULL; tl = tl->next)
			{
			    ar = (asg_arc *)tl->this->this;
			    for(cll = cl->this->cx->corners;cll != NULL;cll = cll->next)
			    {
				if (corner_falls_in_arc_p(cll->this,ar) == TRUE)
				{
				    add_corner_to_arc(cll->this, ar);
				}
			    }
			    pushnew(tl->this, &n->route);
			}
		    }
		}
	    }
	    else if ((a->page == HORZ) && (fixed_point_p(cl->this->cy) == FALSE))
	    {
		/* Find the corners of the legal routing space: (mega-tile) */

		next = next_vert_corner(most_horz_corner(cl->this));
		y1SR = next->cy->sr;		x1SR = next->cx->sr;
		prev = next_vert_corner(least_horz_corner(cl->this));
		y2SR = prev->cy->sr;		x2SR = prev->cx->sr;

		minX = MIN(xSR->q1, MIN(x1SR->q1, x2SR->q1));		
		minY = MIN(ySR->q1, MIN(y1SR->q1, y2SR->q1));
		maxX = MAX(xSR->q2, MAX(x1SR->q2, x2SR->q2));		
		maxY = MAX(ySR->q2, MAX(y1SR->q2, y2SR->q2));

		sliverSet = valid_slivers(a->t,  minX, minY, maxX, maxY, 
					  &contRangeMin, &contRangeMax);

		if (member_p(a->t, sliverSet, identity) == FALSE)
		{
		    /* Major Problem... */
		    fprintf(stderr, "(h) sliver detector screwed {(%d,%d) & (%d,%d)}\n",
			    minX, minY, maxX, maxY);
		    fprintf(stderr, "given %d tiles, couldn't find @(%d, %d)for net %s\n",
			    list_length(sliverSet), a->t->x_pos + (a->t->x_len/2), 
			    a->t->y_pos + (a->t->y_len/2), n->name);

		    return(NULL);
		}
		else
		{
		    if ((cl->this->cy->sr->q1 > contRangeMin) ||
			(cl->this->cy->sr->q2 < contRangeMax))
		    {
			if ((debug_level >= 2) && (sliverSet->next != NULL))
			{
			    fprintf(stderr, "<%s> expanding from y = [%d,%d] to [%d,%d] (%d slivers added)\n",
				    cl->this->cy->n->name, cl->this->cy->sr->q1,
				    cl->this->cy->sr->q2, contRangeMin, contRangeMax,
				    list_length(sliverSet) - 1);
			}

			/* Expand the range: */
			cl->this->cy->sr->q1 = MIN(contRangeMin, cl->this->cy->sr->q1);
			cl->this->cy->sr->q2 = MAX(contRangeMax, cl->this->cy->sr->q2);

			/* Add the expanded corners to all tiles that it uses. */
			for(tl = sliverSet; tl != NULL; tl = tl->next)
			{
			    ar = (asg_arc *)tl->this->this;
			    for(cll = cl->this->cy->corners;cll != NULL;cll = cll->next)
			    {
				if (corner_falls_in_arc_p(cll->this,ar) == TRUE)
				{
				    add_corner_to_arc(cll->this, ar);
				}
			    }
			    pushnew(tl->this, &n->route);
			}
		    }
		}
	    }
	}
    }
}
/*------------------------------------------------------------------------------------*/
expand_slivers(tilesUsed, n)
    tilist *tilesUsed;
    net *n;
{
    tilist *tl;
    asg_arc *a;

    for (tl = tilesUsed; tl != NULL; tl = tl->next)
    {
	a = (asg_arc *)tl->this->this;

	/* Merge/expand slivered tiles for this net: */
	find_and_merge_slivers(a, n);
    }	
}
/*------------------------------------------------------------------------------------*/
o_complete_linkages(n, setJogsP)
    net *n;
    int setJogsP;
{
    old_complete_linkages(n->route, n, setJogsP);
}
/*------------------------------------------------------------------------------------*/
old_complete_linkages(usedTiles, n, setJogsP)
    tilist *usedTiles;
    net *n;
    int setJogsP;
{
    /* This walks through the list of tiles, and completes the links among the 
       corner structures for the given net. */
    /* This is very vicious - it introduces links at EVERY point where they can be 
       made (common use of a tile by two pieces of the global route for the same
       net */

    tilist *tl;
    asg_arc *a;
    corner *linkCorner;
    crnlist *cl, *cll;
    range *link;

    for (tl = usedTiles; tl != NULL; tl = tl->next)
    {
	a = (asg_arc *)tl->this->this;

	linkCorner = choose_best_link(n, a, setJogsP);
	if (linkCorner == NULL) break;

	link = (a->page == VERT) ? linkCorner->cx : linkCorner->cy;

	for (cl = a->corners; cl != NULL; cl = cl->next)
	{	
	    /* Only look at corners belonging to this net: */
	    if (((cl->this->cy != NULL) && (cl->this->cy->n == n)) ||
		((cl->this->cx != NULL) && (cl->this->cx->n == n)))
	    {
		/* Set everybody's ylink to the same range */
		if (a->page == VERT)
		{
		    if (cl->this->cx != link)  /* Skip the link; it should be OK */
		    {
			if (overlapped_p(cl->this->cx->sr, link->sr) == TRUE)
			{
			    reduce_to_least_common_range(link->sr, cl->this->cx->sr);
			    replace_range_instance(cl->this, linkCorner, X);
			}
			else 
			{
			    a = a;
			}
		    }
		}
		else	/* Horizontal Tile */
		{
		    if (cl->this->cy != link)   /* Skip the link; it should be OK */
		    {
			if (overlapped_p(cl->this->cy->sr, link->sr) == TRUE) 
			{
			    reduce_to_least_common_range(link->sr, cl->this->cy->sr);
			    replace_range_instance(cl->this, linkCorner, Y);
			}
			else 
			{
			    a = a;
			}
		    }
		}
	    }
	}
    }
}
/*------------------------------------------------------------------------------------*/
complete_linkages(n, setJogsP)
    net *n;
    int setJogsP;
{
    /* This walks through the list of tiles, and completes the links among the 
       corner structures for the given net. */
    /* This function is intended to do this linking only in the designated
       linkage tiles (where the global routes for two expansions meet).
       Albiet this is a rare case, probably some consideration needs to be made 
       for global routes that overlap each other for more than one tile.  */

    tilist *tl, *connectionTiles = NULL;
    expn *ex;
    trlist *trl;
    tile *first, *last;

    ex = (expn *) n->done->this;
    for (trl = ex->connections; trl != NULL; trl = trl->next)
    {
	/* Find the <first> and <last> tiles for the set of expansions that
	   makes up the global route for this net. */
	
	last = trl->this->tr->this;
	for (tl = trl->this->tr; tl != NULL; tl = tl->next)
	{
	    if (tl->next == NULL) first = tl->this;
	}
	pushnew(last, &connectionTiles);
	pushnew(first, &connectionTiles);
    }
    
    old_complete_linkages(connectionTiles, n, setJogsP);
    free_list(connectionTiles);
}
/*------------------------------------------------------------------------------------*/
center_range_on_single_point(sr)
    srange *sr;
{
    /*This reduces the range of <sr> to a single point */
    int mid = (sr->q1 + sr->q2)/2;
    sr->q1 = mid;
    sr->q2 = mid;
}
/*------------------------------------------------------------------------------------*/
void center_ranges_on_points(TilesToFix)
    tilist *TilesToFix;
{
    /* this takes all ranges contained in all tiles on the route, and centers them. */
    /* This will screw-up royally if the overlap resolution is incomplete. */
    
    tilist *tl;
    asg_arc *a;
    crnlist *cl;

    for (tl = TilesToFix; tl != NULL; tl = tl->next)
    {	/* these should only consist of one range: */
	a = (asg_arc *)tl->this->this;
	for (cl = a->corners; cl != NULL; cl = cl->next) 
	{
	    center_range_on_single_point(cl->this->cy->sr);
	    center_range_on_single_point(cl->this->cx->sr);
	}
    }	
}
/*------------------------------------------------------------------------------------*/
int reduce_to_least_common_range(target, other)
    srange *target, *other;
{
    /* Reduce the <target> range to the least common overlapped range between
       <target> and <other> */
    int min, max;

    find_intersection(target->q1, target->q2, 
		      other->q1, other->q2,
		      &min, &max);
    target->q1 = min;
    target->q2 = max;
}
    
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
int net_name_located_p(net_name, n)
    char *net_name;
    net *n;
{
    if (!strcmp(net_name, n->name)) return(TRUE);
    return(FALSE);
}
/*------------------------------------------------------------------------------------*/
void ps_dump_arc(f, t)
    FILE *f;
    tile *t;
{
    /* This function prints the contents of the given tile in PostScript format t       file <f>. */
    asg_arc *a = (asg_arc *)t->this;
    float x1, y1, x2, y2;

    if (a->mod == NULL)
    {
	if (a->page == HORZ)
	{
	    x1 = (float)a->t->x_pos + GRAY_MARGIN;
	    x2 = (float)a->t->x_pos + (float)a->t->x_len - GRAY_MARGIN;
	    y1 = (float)a->t->y_pos + GRAY_MARGIN;
	    y2 = (float)a->t->y_pos + (float)a->t->y_len - GRAY_MARGIN;
	}
	else 
	{
	    x1 = (float)a->t->y_pos + GRAY_MARGIN;
	    x2 = (float)a->t->y_pos + (float)a->t->y_len - GRAY_MARGIN;
	    y1 = (float)a->t->x_pos + GRAY_MARGIN;
	    y2 = (float)a->t->x_pos + (float)a->t->x_len - GRAY_MARGIN;
	}

	/* print the asg_arcs that contain the <net_under_study> */
	
	if (debug_level == 1)
	{
	    if (member_p(net_under_study, a->nets, net_name_located_p) == TRUE)
	    {
		/* Fill in the area with gray, shaded to show global route */
		fprintf(f, "newpath %f %f moveto %f %f lineto ", x1, y1, x2, y1);
		fprintf(f, "%f %f lineto %f %f lineto ", x2, y2, x1, y2);
		fprintf(f, "closepath %.3f setgray fill 0 setgray\n", 0.7);
		
		/* ps_put_label(f, net_under_study, x1 + 0.9, y2 - 0.9); */
	    }
	}
    }

    else 	/* Print out the module icon */
    {
	//ps_print_mod(f, a->mod);
    }
}
/*------------------------------------------------------------------------------------*/
void ps_invert_arc(f, t)
    FILE *f;
    tile *t;
{
    /* This function prints the contents of the given tile in PostScript format to 
       file <f>. */
    asg_arc *a = (asg_arc *)t->this;
    float x1, y1, x2, y2;

    if (a->mod == NULL)
    {
	/* print the arcs that contain the <net_under_study> */
	
	if (member_p(net_under_study, a->nets, net_name_located_p) == TRUE)
	{
	    if (a->page == HORZ)
	    {
		x1 = (float)a->t->x_pos + GRAY_MARGIN;
		x2 = (float)a->t->x_pos + (float)a->t->x_len - GRAY_MARGIN;
		y1 = (float)a->t->y_pos + GRAY_MARGIN;
		y2 = (float)a->t->y_pos + (float)a->t->y_len - GRAY_MARGIN;
	    }
	    else 
	    {
		x1 = (float)a->t->y_pos + GRAY_MARGIN;
		x2 = (float)a->t->y_pos + (float)a->t->y_len - GRAY_MARGIN;
		y1 = (float)a->t->x_pos + GRAY_MARGIN;
		y2 = (float)a->t->x_pos + (float)a->t->x_len - GRAY_MARGIN;
	    }

	    /* Fill in the area with gray, shaded to show global route */
	    fprintf(f, "newpath %f %f moveto %f %f lineto ", x1, y1, x2, y1);
	    fprintf(f, "%f %f lineto %f %f lineto ", x2, y2, x1, y2);
	    fprintf(f, "closepath %.3f setgray fill 0 setgray\n", 0.9);
    
	    ps_put_label(f, net_under_study, x1 + 0.9, y2 - 0.9);
	}
    }
    
    else 	/* Don't print the module icon */
    {
/*	ps_print_mod(f, a->mod);        */
    }
}
/*------------------------------------------------------------------------------------*/
int solidp_for_printing(t)
    tile *t;
{
    return(TRUE);
}
/*-------------------------------------------------------------------------------------*/
replace_range(oldR, newR, axis)
    range *oldR, *newR;
    int axis;
{
    asg_arc *a;
    arclist *al;
    crnlist *cll;

    for (cll = oldR->corners; cll != NULL; cll = cll->next)
    {
	if (axis == X) cll->this->cx = newR;
	else cll->this->cy = newR;

	push(cll->this, &newR->corners);
    }
    free_list(oldR->corners);
    my_free(oldR);
}
/*-------------------------------------------------------------------------------------*/
restrict_corner_usage(c, axis)
    corner *c;
    int axis;
{
    /* This function checks the corner <c> and all linked corners along the given 
       axis for problems arising from incorrect tile inclusions (a->corners lists) */
    asg_arc *a;
    arclist *al;
    crnlist *cll;
    range *oldR = (axis == X) ? c->cx : c->cy;

    for (cll = oldR->corners; cll != NULL; cll = cll->next)
    {
	for (al = cll->this->al; al != NULL; al = al->next)
	{
	    if (corner_falls_in_arc_p(cll->this, al->this) == FALSE)
	    {
		a = al->this;
		rem_item(cll->this, &a->corners);
		rem_item(a, &cll->this->al);

		if (a->corners == NULL)
		{
		    /* Got other problems, like cleaning up the usedTiles lists */
		}
	    }
	}
    }
}
/*-------------------------------------------------------------------------------------*/
replace_range_instance(c, newC, axis)
    corner *c, *newC;
    int axis;
{
    asg_arc *a;
    arclist *al, *atemp;
    crnlist *cll;
    range *oldR = (axis == X) ? c->cx : c->cy;
    range *newR = (axis == X) ? newC->cx : newC->cy;

    for (cll = oldR->corners; cll != NULL; cll = cll->next)
    {
	if (axis == X) cll->this->cx = newR;
	else cll->this->cy = newR;

	al = cll->this->al;
	while (al != NULL)
	{
	    atemp = al->next;
	    if (corner_falls_in_arc_p(cll->this, al->this) == FALSE)
	    {
		a = al->this;
		rem_item(cll->this, &a->corners);
		rem_item(a, &cll->this->al);

		if (a->corners == NULL)
		{
		    /* Got other problems, like cleaning up the usedTiles lists */
		}
	    }
	    al = atemp;
	}

	push(cll->this, &newR->corners);
    }
    my_free(oldR->sr);
    free_list(oldR->corners);
    my_free(oldR);
}
/*-------------------------------------------------------------------------------------*/
replace_corner_instance(c, newC)
    corner *c, *newC;
{
    /* This function traces the usage of <c> and replaces all such occurances
       with <newC>.  If <newC> is already referenced, then <c> is simply deleted. */
    /* Hunt down the little bugger in the two tile spaces, and kill it: */
    /* Hunt down the little bugger in it's range connections, and kill it: */

    tile *newTile;
    asg_arc *a;
    arclist *al;
    range *r;
    corner *e1 = NULL, *e2 = NULL, *e3 = NULL;

    r = c->cx;
    e1 = (corner *)rem_item(c, &r->corners);
    pushnew(newC, &r->corners);

    r = c->cy;
    e2 = (corner *)rem_item(c, &r->corners);
    pushnew(newC, &r->corners);

    /* Now yank <c> from the all arc(s) where it is referenced:  */
    for (al = c->al; al != NULL; al = al->next)
    {
	a = al->this;
	e3 = (corner *)rem_item(c, &a->corners);
	if ((e3 == NULL) && (corner_falls_in_arc_p(c, a) == TRUE))
	{
	    fprintf(stderr, "Screwedup corner deletion attempt...");
	    /* exit(1); */
	    return;
	}
	add_corner_to_arc(newC, a);
    }
    my_free(c);
}
/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/
remove_corner_from_used_lists(c, a)
    corner *c;
    asg_arc *a;
{
    int i;
    trail *t;
    trailist *trl;

    for (i=0; i<HASH_SIZE; i++)
    {
	for (trl = a->trails[i]; trl != NULL; trl = trl->next)
	{
	    t = trl->this;
	    if (t->used != NULL) rem_item(c, &t->used);
	}
    }
}
/*-------------------------------------------------------------------------------------*/
delete_corner(c, deadRanges)
    corner *c;
    rnglist **deadRanges;
{
    asg_arc *a;
    arclist *al;
    range *r;

    r = c->cx;
    rem_item(c, &r->corners);
    pushnew(r, deadRanges);

    r = c->cy;
    rem_item(c, &r->corners);
    pushnew(r, deadRanges);

    for (al = c->al; al !=NULL; al = al->next)
    {
	a = al->this;
	rem_item(c, &a->corners);
	remove_corner_from_used_lists(c, a);
    }
    free_list(c->al);
    my_free(c);
}
/*-------------------------------------------------------------------------------------*/
int border_condition(sr1, sr2)
    srange *sr1, *sr2;
{
    if ((sr1->q1 == sr2->q2) || (sr1->q2 == sr2->q1)) return(TRUE);
    else return(FALSE);
}
/*-------------------------------------------------------------------------------------*/
int fixed_point_p(r1)
    range *r1;
{
    if (r1->sr->q1 != r1->sr->q2) return(FALSE);
    return(TRUE);
}
/*-------------------------------------------------------------------------------------*/
int repeated_srange_p(r1, r2)
    range *r1, *r2;
{
    /* This retuurns TRUE iff <r2> has the same underlying ranges as <r1>: */
    if ((r1->sr->q1 == r2->sr->q1) && (r1->sr->q2 == r2->sr->q2)) return (TRUE);
    else return(FALSE);
}
/*-------------------------------------------------------------------------------------*/
int repeated_corner_p(c1, c2)
    corner *c1, *c2;
{
    /* This returns TRUE iff <c2> has the same underlying ranges as <c1>: */
    if ((c1->cx->sr == c2->cx->sr) && (c1->cy->sr == c2->cy->sr)) return (TRUE);
    else return(FALSE);
}
/* ------------------------------------------------------------------------------------*/
crnlist *delete_duplicate_corners(cl)
    crnlist **cl;
{
    corner *targ;
    crnlist *p, *pp, *trash, *last;

    if (*cl == NULL)
    {
	/* error: "delete_duplicate_corners passed null list" */
	return(NULL);
    }
    
    for (p = *cl; ((p != NULL) && (p->next != NULL)); p = p->next)
    {
	targ = p->this;
	last = p;
	for (pp = p->next; pp != NULL;)
	{
	    if (targ->cx->sr != pp->this->cx->sr)
	    {
		if ((repeated_srange_p(targ->cx, pp->this->cx) == TRUE)  &&
		    (repeated_srange_p(targ->cy, pp->this->cy) == TRUE))
		{
		    /* Replace pp->this->cx with targ->cx */
		    replace_range_instance(pp->this, targ, X);
		}
		else if ((overlapped_p(targ->cx->sr, pp->this->cx->sr) == TRUE) &&
/*			 ((fixed_point_p(targ->cx) == FALSE) &&
			  (fixed_point_p(pp->this->cx) == FALSE))  &&		*/
			 (border_condition(targ->cx->sr, pp->this->cx->sr) == FALSE)) 
		{
		    /* collapse these into one srange... */	
		    merge_sranges(&targ->cx->sr, &pp->this->cx->sr);
		    /* Now check to see that the corners are in(ex)cluded in(from) 
		       the right arcs... */
		    restrict_corner_usage(pp->this, X);
		    restrict_corner_usage(targ, X); 	
		}

		/* Very specific, strange condition that occurs (BUG fix) */
		else if((repeated_srange_p(targ->cy, pp->this->cy) == TRUE) &&
			(list_length(pp->this->cx->corners) <= 1) &&
			(pp->this->t == NULL) &&
			(fixed_point_p(pp->this->cx) == FALSE))
		{
		    /* delete the range pp->this->cx, and replace it with targ->cx */
		    merge_sranges(&targ->cx->sr, &pp->this->cx->sr);
		    replace_range(pp->this->cx, targ->cx, X);
		}

		/* Very specific, strange condition that occurs (BUG fix continued) */
		else if((repeated_srange_p(targ->cy, pp->this->cy) == TRUE) &&
			(list_length(targ->cx->corners) <= 1) &&
			(targ->t == NULL) &&
			(fixed_point_p(targ->cx) == FALSE))
		{
		    /* delete the range targ->cx, and replace it with pp->this->cx */
		    merge_sranges(&targ->cx->sr, &pp->this->cx->sr);
		    replace_range(targ->cx, pp->this->cx, X);
		}
	    }
	    else if ((targ->cx != pp->this->cx) &&
		     (targ->cx->sr == pp->this->cx->sr))
	    {
		replace_range(pp->this->cx, targ->cx, X);
	    }

	    if (targ->cy->sr != pp->this->cy->sr)
	    {
		if ((repeated_srange_p(targ->cy, pp->this->cy) == TRUE) &&
		    (repeated_srange_p(targ->cx, pp->this->cx) == TRUE))
		{
		    /* Replace pp->this->cy with targ->cy */
		    replace_range_instance(pp->this, targ, Y);
		}
		else if ((overlapped_p(targ->cy->sr, pp->this->cy->sr) == TRUE) &&
			 ((fixed_point_p(targ->cy) == FALSE) &&
			  (fixed_point_p(pp->this->cy) == FALSE)) )/* &&		
			 (border_condition(targ->cx->sr, pp->this->cx->sr) == FALSE)) */
		{
		    /* collapse these into one srange... */	
		    merge_sranges(&targ->cy->sr, &pp->this->cy->sr);
		    /* Now check to see that the corners are in(ex)cluded in(from) 
		       the right arcs... BUT- Don't blow away ranges, because 
		       this causes BIG problems... */
/*		    restrict_corner_usage(pp->this, Y);
		    restrict_corner_usage(targ, Y);		*/
		}

		/* Very specific, strange condition that occurs (BUG fix) */
		else if ((repeated_srange_p(targ->cx, pp->this->cx) == TRUE) &&
			  ((list_length(pp->this->cy->corners) <= 1) &&
			   (pp->this->t == NULL) &&
			   (fixed_point_p(pp->this->cy) == FALSE)))
		{
		    /* delete the range pp->this->cy, and replace it with targ->cy */
		    merge_sranges(&targ->cy->sr, &pp->this->cy->sr);
		    replace_range(pp->this->cy, targ->cy, Y);
		}
		/* Bug Fix continued */
		else if ((repeated_srange_p(targ->cx, pp->this->cx) == TRUE) &&
			  ((list_length(targ->cy->corners) <= 1) &&
			   (targ->t == NULL) &&
			   (fixed_point_p(targ->cy) == FALSE)))
		{
		    /* delete the range targ->cy, and replace it with pp->this->cy */
		    merge_sranges(&targ->cy->sr, &pp->this->cy->sr);
		    replace_range(targ->cy, pp->this->cy, Y);
		}
	    }
	    else if ((targ->cy != pp->this->cy) &&
		     (targ->cy->sr == pp->this->cy->sr))
	    {
		replace_range(pp->this->cy, targ->cy, Y);
	    }

	    if (repeated_corner_p(targ, pp->this) == TRUE)
	    {
		replace_corner_instance(pp->this, targ);
		last->next = pp->next;
		trash = pp;
		pp = pp->next;
		my_free(trash);
	    }
	    else 
	    {
		last = pp;
		pp = pp->next;
	    }
	}
    }
    return(*cl);
}
/*------------------------------------------------------------------------------------*/
void delete_all_corners(deadCorners)
    crnlist *deadCorners;
{
    rnglist *rangesToKill = NULL, *rl;
    crnlist *cl;
    for(cl = deadCorners; cl != NULL; cl = cl->next)
    {
	delete_corner(cl->this, &rangesToKill);
    }
    
    for(rl = rangesToKill; rl != NULL; rl = rl->next)
    {
	my_free(rl->this->sr);
	my_free(rl->this);
    }
    free_list(rangesToKill);    
}
/*------------------------------------------------------------------------------------*/
void merge_global_subroutes(n)
    net *n;
{	
    /* This looks at the corners that have been saved in the global sub-routes
       that exist in the net (ex->connections);  These corner lists are merged,
       and all duplicates are removed.   The results are stored in n->route.  */

    expn *ex = (expn *)n->done->this;
    trlist *trl;

    n->route = free_list(n->route);

    for (trl = ex->connections; trl != NULL; trl = trl->next)
    {
	/* Collect all of the corners used by this net: */
	n->route = append_list(n->route, trl->this->used);
	trl->this->used = NULL;
    }	
    delete_duplicate_corners(&n->route); 
}

/*------------------------------------------------------------------------------------*/
void reset_arc_costs(tilesToReset)
    tilist *tilesToReset;
{
    /* This walks through the given set of tiles, and resets the congestion costs. */

    crnlist *cl;
    tilist *tl;
    asg_arc *a;

    for (tl = tilesToReset; tl != NULL; tl = tl->next)
    {
	/* Paint each arc with the usage - assume VERT tiles essentially have 
	   only the vertical tracks employed, and HORZ tiles the horizontal 
	   tracks. */
	a =(asg_arc *)tl->this->this;

	for (cl = a->corners; cl != NULL; cl = cl->next)
	{
	    pushnew(cl->this->cx->n, &a->nets);		/* Mark the route used */
	}
	(*set_congestion_value)(a);
    }
}
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
dump_ranges(rangesToSee)
    rnglist *rangesToSee;
{
    rnglist *rl;
    /* Dump Corner List to the screen: */
    if (rangesToSee == NULL) fprintf (stderr, "NONE...");
    for (rl = rangesToSee; rl != NULL; rl = rl->next)
    {
	fprintf(stderr, "<%s> [%d,%d]...", 
		rl->this->n->name, rl->this->sr->q1, rl->this->sr->q2);
    }
    fprintf (stderr, "\n");
}
/*------------------------------------------------------------------------------------*/
dump_corners(cornersToSee)
    crnlist *cornersToSee;
{
    crnlist *cl;
    /* Dump Corner List to the screen: */
    if (cornersToSee == NULL) fprintf (stderr, "NONE...");
    for (cl = cornersToSee; cl != NULL; cl = cl->next)
    {
	fprintf(stderr, "([%d,%d],[%d,%d])...", 
		cl->this->cx->sr->q1, cl->this->cx->sr->q2,
		cl->this->cy->sr->q1, cl->this->cy->sr->q2);
    }
    fprintf (stderr, "\n");
}
/*------------------------------------------------------------------------------------*/
dump_corner_names(cornersToSee)
    crnlist *cornersToSee;
{
    crnlist *cl;
    /* Dump Corner List to the screen: */
    if (cornersToSee == NULL) fprintf (stderr, "NONE...");
    for (cl = cornersToSee; cl != NULL; cl = cl->next)
    {
	fprintf(stderr, "%s...", cl->this->cx->n->name);
    }
    fprintf (stderr, "\n");
}
/*------------------------------------------------------------------------------------*/
void local_route(nl, page, printFlag)
    nlist *nl;
    int page, printFlag;
{
    /* Perform the local routing tasks, now that "global_route" is complete. 
       The global route that is used consists of the collection of the 'Best Trails' 
       contained in each of the expansions.  (One expansion per terminal)  */

    nlist *nll, *temp = NULL;
    tilist *tl, *TilesUsed = NULL;
    corner *corn;
    crnlist *cl;
    trlist *trl;
    expn *ex;
    int setupFlag = TRUE;
    
    /* Create the corner and srange structures from the best trails for each
       expansion of each net: */
    for (nll = nl; ((nll != NULL) && (setupFlag != FALSE)); nll = nll->next)
    {
	if (list_length(nll->this->done) > 1)	/* If there's something to local route */
	{
	    map_net_into_constraint_sets(nll->this);
	    /* This creates all of the corner and srange structures, and appropriately
	       paints the tiles used with the corners that occur within them. */
	    /* Here also, the usage of the list pointer n->route is converted from 
	       saving all of the tiles visited, to the corners that comprise the local
	       route for the net */
	    
	    complete_linkages(nll->this, TRUE);
	    
	    if (useSlivering == TRUE)
	    {
		expand_slivers(nll->this->route, nll->this);		    
		o_complete_linkages(nll->this, FALSE);
	    }

	    for (tl = (tilist *)nll->this->route; tl != NULL; tl = tl->next)
	    {
		pushnew(tl->this, &TilesUsed);
	    }
	    
	    /* At this point, the global routes, which have been calculated, need to be 
	       combined and duplicates removed.  The global router is very prone to give 
	       results where (to insure completion) tiles are repeated in two or more 
	       global sub-routes. [This sets up n->route as a corner list]  */
	    merge_global_subroutes(nll->this);
	}
    }
	
    if (debug_level == 4)
    {
	fprintf(stderr, "\nGlobal Routes are as follows:\n");
	for (nll = nl; nll != NULL; nll = nll->next)
	{
	    /* Dump the local routes to the screen: */
	    fprintf(stderr, "<%s>\t", nll->this->name);
	    dump_corners(nll->this->route);
	}
	fprintf (stderr, "\n\n");
    }
    
    /* As the nets have now been expanded to fill the available (legal) spaces
       (Sliver nonsense), the congestion is now inappropriate: */
    reset_arc_costs(TilesUsed);
    
    /* Resolve sranges */
    /* This is a two-step procecess - firstly, the nets have their (currently) overlapped
       sranges resolved into non-overlapping ranges.  Secondly, these non-overlapping
       sranges are resolved into points so that wires can be drawn. */
    
    TilesUsed = (tilist *)sort_list(TilesUsed, in_congestion_order);
    form_into_nonoverlapping_ranges(TilesUsed);
    
    if (debug_level >= 2)
    {
	fprintf(stderr, "\nLocal Routes are as follows:\n");
	for (nll = nl; nll != NULL; nll = nll->next)
	{
	    /* Dump the local routes to the screen: */
	    fprintf(stderr, "<%s>\t", nll->this->name);
	    dump_corners(nll->this->route);
	}
	fprintf (stderr, "\n\n");
    }
    
    center_ranges_on_points(TilesUsed);
    
    /* Ready for printing, mein Herr */
    
    if (printFlag == TRUE)
    {
	/* finish the dump file(s) */
	if (debug_level == 1) 
	{
	    solidp = solidp_for_printing;
	    ps_print_tile_space(routingRoot[VERT][page], df, ps_invert_arc, FALSE); 
	    ps_print_tile_space(routingRoot[HORZ][page], df, ps_dump_arc, TRUE); 
	}    
	
	if (debug_level == 10) print_tile_spaces(hf, vf);    
	else if (debug_level == 11) print_tile_spaces(hf, vf);
    }
}   
/*------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/
int printed_p(m)
    module *m;
{
    if (m->placed == TRUE)return(TRUE);
    else return(FALSE);
}
/*-------------------------------------------------------------------------------------*/
int mark_as_printed(m)
    module *m;
{
    m->placed = TRUE;
}
/*-------------------------------------------------------------------------------------*/
int manhattan_p(c1, c2)
    corner *c1, *c2;
{
    /* Returns TRUE iff the two corners do not form a manhattan-oriented segment. */
    int x1 = c1->cx->sr->q1, x2 = c2->cx->sr->q1;
    int y1 = c1->cy->sr->q1, y2 = c2->cy->sr->q1;
    
    if ((x1 == x2) || (y1 == y2)) return(TRUE);	
    else return(FALSE);
}
/*-------------------------------------------------------------------------------------*/
int not_on_Net(r)
    range *r;
{
    if ((r->n == Net) && r->n != NULL) return(FALSE);
    return(FALSE);
}
/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/
//ASG Code
list *xc_extract_corner_list(cd,n)
    XCWindowData *cd;
    net *n;
{
    /* This loops through the corners from <n>->route, and builds a
       list of point lists (corners) that are to be exported to XCircuit. 
	The <cd> argument allows wire intersections to be printed as they are discovered.*/

    crnlist *allCorners = (crnlist *)n->route;
    crnlist *cl, *cll, *temp;
    list *l, *results = NULL;
    int x1, y1;

    if (debug_level == 4)
    {
	fprintf(stderr, "given corners for <%s>@ ", n->name);
    }

    /* This works by copying the main segments to the (ordered) <temp>list, and then
       picking each corner, and adding the segments needed to fill out the lists.
       It is critical that points be liad in a connected (manhattan_p) order */

    for (cl = allCorners; cl != NULL; cl = cl->next)
    {
	temp = NULL;

	/* look along the Y Axis: */
	for (cll = cl->this->cx->corners; cll != NULL; cll = cll->next)
	{
	    if (cll->this != cl->this) push(cll->this, &temp);
	}
     
	/* Now add the point */
	push(cl->this, &temp);

	/* look along the X axis: */
	for (cll = cl->this->cy->corners; cll != NULL; cll = cll->next)
	{
	    if (cll->this != cl->this) push(cll->this, &temp);
	}

	/* If this corner is a T, then print out the dot to <cd>: */
	if (forms_T_p(cl->this) != FALSE)
	{
	    x1 = cl->this->cx->sr->q1;
	    y1 = cl->this->cy->sr->q1;
	    // ps_print_contact(f, (float)x1, (float)y1); // Dump the "DOT" to XC at the location
		
	    if (debug_level == 4) fprintf(stderr, "*");
	}
	
	if (debug_level == 4)
	fprintf(stderr, "(%d,%d)...", cl->this->cx->sr->q1, cl->this->cy->sr->q1);
	
	if (temp != NULL) push(temp, &results);
    }

    if (debug_level == 4)
    {
	fprintf(stderr, "\n printing corners for <%s> @ ",n->name);
	for (l =results; l != NULL; l = l->next)
	{
	    fprintf(stderr, " [");
	    for (cl = (crnlist *)l->this; cl != NULL; cl = cl->next)
	    {
		fprintf(stderr, "(%d,%d)...", cl->this->cx->sr->q1, cl->this->cy->sr->q1);
	    }
	    fprintf(stderr, "] ");
	}
	fprintf (stderr, "\n");
    }
    return(results);
}
/*-------------------------------------------------------------------------------------*/
list *extract_corner_list(f, n)
    FILE *f;
    net *n;
{
    /* This loops through the corners from <n>->route, and builds a
       list of point lists (corners) that are to be printed. The <f> argument allows 
	wire intersections to be printed as they are discovered. */

    crnlist *allCorners = (crnlist *)n->route;
    crnlist *cl, *cll, *temp;
    list *l, *results = NULL;
    int x1, y1;

    if (debug_level == 4)
    {
	fprintf(stderr, "given corners for <%s>@ ", n->name);
    }

    /* This works by copying the main segments to the (ordered) <temp>list, and then
       picking each corner, and adding the segments needed to fill out the lists.
       It is critical that points be liad in a connected (manhattan_p) order */

    for (cl = allCorners; cl != NULL; cl = cl->next)
    {
	temp = NULL;

	/* look along the Y Axis: */
	for (cll = cl->this->cx->corners; cll != NULL; cll = cll->next)
	{
	    if (cll->this != cl->this) push(cll->this, &temp);
	}
     
	/* Now add the point */
	push(cl->this, &temp);

	/* look along the X axis: */
	for (cll = cl->this->cy->corners; cll != NULL; cll = cll->next)
	{
	    if (cll->this != cl->this) push(cll->this, &temp);
	}

	/* If this corner is a T, then print out the dot to <f>: */
	if (forms_T_p(cl->this) != FALSE)
	{
	    x1 = cl->this->cx->sr->q1;
	    y1 = cl->this->cy->sr->q1;
	    ps_print_contact(f, (float)x1, (float)y1); 

	    if (debug_level == 4) fprintf(stderr, "*");
	}
	
	if (debug_level == 4) 
	fprintf(stderr, "(%d,%d)...", cl->this->cx->sr->q1, cl->this->cy->sr->q1);
	
	if (temp != NULL) push(temp, &results);
    }

    if (debug_level == 4)
    {
	fprintf(stderr, "\n printing corners for <%s> @ ",n->name);
	for (l =results; l != NULL; l = l->next)
	{
	    fprintf(stderr, " [");
	    for (cl = (crnlist *)l->this; cl != NULL; cl = cl->next)
	    {
		fprintf(stderr, "(%d,%d)...", cl->this->cx->sr->q1, cl->this->cy->sr->q1);
	    }
	    fprintf(stderr, "] ");
	}
	fprintf (stderr, "\n");
    }
    return(results);
}
/*-------------------------------------------------------------------------------------*/
void write_local_route_to_file(f, nets)
    FILE *f;
    nlist *nets;
{
    net *n;
    nlist *nl;
    expn *ex;
    trlist *trl;
    crnlist *cl;
    list *l, *ll;
    int x1, y1, x2, y2;


    for (nl = nets; nl != NULL; nl = nl->next)
    {
	n = nl->this;
	if (n->expns != NULL)
	{
	    ex = (expn *)n->expns->this;
	    
	    fprintf(f, "\n%%%% postscript code for modules associated with net <%s> %%%%\n", n->name);
	    
	    /* Print as much of each net as possible: */
	    for (trl = ex->connections; trl != NULL; trl = trl->next)
	    {
		/* If it has been routed, print it: */
		if (printed_p(trl->this->ex->t->mod) == FALSE)
		{
		    ps_print_mod(f, trl->this->ex->t->mod);
		    mark_as_printed(trl->this->ex->t->mod);
		}
	    }

	   fprintf(f, "\n%%%% postscript code for net <%s> %%%%\n", n->name);
	    
	   /* Arrange the corners for this expansion in printing order: */
	   l = extract_corner_list(f, n);	/* Locates and prints contacts */
	
	   for (ll = l; ll != NULL; ll = ll->next)
	    {
		fprintf(f,"newpath ");
		for (cl = (crnlist *)ll->this; cl != NULL; cl = cl->next)
		{
		    if ((cl->next != NULL) && 
			(manhattan_p(cl->this, cl->next->this) == TRUE))
		    {
			x1 = cl->this->cx->sr->q1;
			y1 = cl->this->cy->sr->q1;
			x2 = cl->next->this->cx->sr->q1;
			y2 = cl->next->this->cy->sr->q1;
			fprintf(f, "%d %d moveto %d %d lineto \n", x1, y1, x2, y2);
		    }
		}
		fprintf(f, "closepath stroke [] 0 setdash\n");  
		ll->this = (int *)free_list(ll->this);
	    }
	    free_list(l);
	}
    }
}

/*-------------------------------------------------------------------------------------*/
void xc_write_local_route(areastruct, nets)
    XCWindowData *areastruct;
    nlist *nets;
{
    net *n;
    nlist *nl;
    expn *ex;
    trlist *trl;
    crnlist *cl;
    list *l, *ll;
    float scale;

    for (nl = nets; nl != NULL; nl = nl->next)
    {
	n = nl->this;
	if (n->expns != NULL)
	{
	    ex = (expn *)n->expns->this;
	    	    
	    /* Print as much of each net as possible: */
	    for (trl = ex->connections; trl != NULL; trl = trl->next)
	    {
		/* If it has been routed, print it: */
		if (printed_p(trl->this->ex->t->mod) == FALSE)
		{
		    scale = (float)(trl->this->ex->t->mod->x_size)
				/ (float)(ICON_SIZE * ICON_SIZE / 2);
		    xc_print_asg_module(areastruct, trl->this->ex->t->mod);
		    mark_as_printed(trl->this->ex->t->mod);
		}
	    }
	    /* Arrange the corners for this expansion in printing order: */
	    l = xc_extract_corner_list(areastruct, n);	/* Locates and prints contacts */	
	 
	    for (ll = l; ll != NULL; ll = ll->next)
	    {
		int *x, *y;
	        int num_points = 0;

		for (cl = (crnlist *)ll->this; cl != NULL; cl = cl->next)
	           num_points++;
		x = (int *)malloc(num_points * sizeof(int));
		y = (int *)malloc(num_points * sizeof(int));

		num_points = 0;
		for (cl = (crnlist *)ll->this; cl != NULL; cl = cl->next)
		{
	            /* x[num_points] = 15 * cl->this->cx->sr->q1 + X_SHIFT; */
		    /* y[num_points] = 15 * cl->this->cy->sr->q1 + Y_SHIFT; */
		    x[num_points] = cl->this->cx->sr->q1;
		    y[num_points] = cl->this->cy->sr->q1;
		    num_points++;
		}
			
		asg_make_polygon(areastruct, num_points, x, y);			
		free(x);
		free(y);

		ll->this = (int *)free_list(ll->this);
	    }
	    free_list(l);

   	    calcbbox(areastruct->topinstance);
  	    centerview(areastruct->topinstance);
	}
    }
}

/*-------------------------------------------------------------------------------------*/

void xc_print_local_route(areastruct, nets)
    XCWindowData *areastruct;
    nlist *nets;
{
    FILE *hfile, *fopen ();
    nlist *netToPrint, *nl;	/* For debug == 1 stuff... */
    int c;
    
    /* Insert code to put useful comments on XC diagram */
	    
    /* Write the local route now: */
    xc_write_local_route(areastruct, nets);

    if (debug_level == 1)
    {
	/* Write the local route now: */
	for (nl = nets; ((nl != NULL) && (strcmp(nl->this->name, net_under_study)));
	     nl = nl->next);
	if (nl != NULL)
	{
	    push(nl->this, &netToPrint);
	    xc_write_local_route(areastruct, netToPrint);
	}
    }
}

/*-------------------------------------------------------------------------------------*/
void print_local_route(f, nets)
    FILE *f;
    nlist *nets;
{
    FILE *hfile, *fopen ();
    nlist *netToPrint, *nl;	/* For debug == 1 stuff... */
    int c;
    
    /* for date and time */
    long time_loc;
    extern long int time();

    /* now dump the result */
    time_loc = time(0L);

    if (latex != TRUE) /* Not in latex mode */
    {
	ps_print_standard_header(f);
	fprintf(f,"(N2A %s %s ?? %s)\n",
                getenv("USER"), ctime(&time_loc), 
		(stopAtFirstCut == TRUE) ? "(first cut only)" : "with rip-up");

	fprintf(f,"\n(with flags-> rule #%d s:%d c:%d part:%d dir:%d)\n",
                partition_rule, max_partition_size, max_partition_conn, 
                partition_count, matrix_type);

	fprintf(f, "%d %d %d %d init\n", xfloor, yfloor, x_sizes[0], y_sizes[0]);

	fprintf(f,"\n(N2A %s %s input:?? s:%d c:%d r:%-5.3f rule #%d part:%d dir:%d) %d %d %d %d init\n", 
	getenv("USER"), ctime(&time_loc), max_partition_size,
	max_partition_conn, partition_ratio, partition_rule,
	partition_count, matrix_type, xfloor, yfloor, x_sizes[0], y_sizes[0]);

    }
    else /* This stuff replaces the init for latex mode */
    {
	ps_print_standard_header(f);
	fprintf(f,"%%%%BoundingBox: %d %d %d %d\n",
		xfloor - CHANNEL_LENGTH, yfloor - CHANNEL_HEIGHT, 
		x_sizes[0] + CHANNEL_LENGTH, y_sizes[0] + CHANNEL_HEIGHT);
	fprintf(f, "swidth setlinewidth\n/Courier findfont fwidth scalefont setfont\n");
    }

    /* Write the local route now: */
    write_local_route_to_file(f, nets);

    if (latex != TRUE) fprintf(f, "showpage\n");


    if (debug_level == 1)
    {
	/* Write the local route now: */
	for (nl = nets; ((nl != NULL) && (strcmp(nl->this->name, net_under_study)));
	     nl = nl->next);
	if (nl != NULL)
	{
	    push(nl->this, &netToPrint);
	    write_local_route_to_file(df, netToPrint);
	}
	if (latex != TRUE) fprintf(df, "showpage\n");
    }
}
/*------------------------------------------------------------------------------------*/
int ignore_contents(t)
    tile *t;
{
    return(FALSE);
}
/*-------------------------------------------------------------------------------------*/
void ps_ignore_arc(f, t)
    FILE *f;
    tile *t;
{
    /* This function prints the statistical info for the given tile in PostScript 
       to the file <f>. */
    asg_arc *a = (asg_arc *)t->this;
    float x1, y1, x2, y2;

    if (a->mod == NULL)
    {
	x1 = (float)a->t->x_pos + GRAY_MARGIN;
	x2 = (float)a->t->x_pos + (float)a->t->x_len - GRAY_MARGIN;
	y1 = (float)a->t->y_pos + GRAY_MARGIN;
	y2 = (float)a->t->y_pos + (float)a->t->y_len - GRAY_MARGIN;

	if (member_p(net_under_study, a->nets, net_name_located_p) == TRUE)
	{
	/* print fancy stuff in the arcs that contain the <net_under_study> */
	    if (a->page == VERT)
	    {
		x1 = (float)a->t->y_pos + GRAY_MARGIN;
		x2 = (float)a->t->y_pos + (float)a->t->y_len - GRAY_MARGIN;
		y1 = (float)a->t->x_pos + GRAY_MARGIN;
		y2 = (float)a->t->x_pos + (float)a->t->x_len - GRAY_MARGIN;
	    }

	    /* Fill in the area with gray, shaded to show global route */
	    fprintf(f, "newpath %f %f moveto %f %f lineto ", x1, y1, x2, y1);
	    fprintf(f, "%f %f lineto %f %f lineto ", x2, y2, x1, y2);
	    fprintf(f, "closepath %.3f setgray fill 0 setgray\n", 0.9);
	    
	    ps_put_int(f, a->local, x1, y2);
	    ps_put_label(f, net_under_study, x1 + 2.0, y2);
	}
	else 
	{
	    ps_put_int(f, a->local, x1, y2);
	}
    }
}
/*-------------------------------------------------------------------------------------*/
void print_tile_spaces(f1, f2)
    FILE *f1, *f2;
{
    long time_loc;
    extern long int time();

   /* Define the bounding box for the post-script code: */
    fprintf(f1,"%%%%BoundingBox: %d %d %d %d\n",
            xfloor - CHANNEL_LENGTH, yfloor - CHANNEL_HEIGHT, 
            x_sizes[0] + CHANNEL_LENGTH, y_sizes[0] + CHANNEL_HEIGHT);
    fprintf(f2,"%%%%BoundingBox: %d %d %d %d\n",
            yfloor - CHANNEL_HEIGHT, xfloor - CHANNEL_LENGTH, 
            y_sizes[0] + CHANNEL_HEIGHT, x_sizes[0] + CHANNEL_LENGTH);

    /* now dump the result */
    time_loc = time(0L);

    if (latex != TRUE) /* Not in latex mode */
    {
	ps_print_standard_header(f1);
	fprintf(f1,
                "\n(Horizontal Tilespace for N2A %s %s input:?? s:%d rule #%d part:%d dir:%d) %d %d %d %d init\n",
                getenv("USER"), ctime(&time_loc),
                max_partition_size, partition_rule,
                partition_count, matrix_type,
                xfloor, yfloor, x_sizes[0], y_sizes[0]);

	ps_print_standard_header(f2);
	fprintf(f2,
                "\n(Vertical Tilespace for N2A %s %s input:?? s:%d rule #%d part:%d dir:%d) %d %d %d %d init\n",
                getenv("USER"), ctime(&time_loc),
                max_partition_size, partition_rule,
                partition_count, matrix_type,
                xfloor, yfloor, x_sizes[0], y_sizes[0]);

	/* Special stuff for the vertical tile space: */
	fprintf(f2, "%% Flip the figure, as x's and y's are reversed: %%\n");
	fprintf(f2, "    -1 1 scale %d neg %d translate\n", 
		y_sizes[0] + CHANNEL_HEIGHT, xfloor - CHANNEL_LENGTH);
	fprintf(f2, "    180 rotate %d neg %d neg translate \n", 
		xfloor - CHANNEL_LENGTH, y_sizes[0] + CHANNEL_HEIGHT);
    }
    else /* This stuff replaces the init for latex mode */
    {
	ps_print_standard_header(f1);
	fprintf(f1, "swidth setlinewidth\n/Courier findfont fwidth scalefont setfont\n");
	fprintf(f1,"%%%%BoundingBox: %d %d %d %d\n",
		xfloor - CHANNEL_LENGTH, yfloor - CHANNEL_HEIGHT, 
		x_sizes[0] + CHANNEL_LENGTH, y_sizes[0] + CHANNEL_HEIGHT);

	ps_print_standard_header(f2);
	fprintf(f2, "swidth setlinewidth\n/Courier findfont fwidth scalefont setfont\n");
	fprintf(f2,"%%%%BoundingBox: %d %d %d %d\n",
		yfloor - CHANNEL_HEIGHT, xfloor - CHANNEL_LENGTH, 
		y_sizes[0] + CHANNEL_HEIGHT, x_sizes[0] + CHANNEL_LENGTH);

	/* Special stuff for the vertical tile space: */
	fprintf(f2,"%% Flip the figure, as x's and y's are reversed: %%\n");
	fprintf(f2,"    -1 1 scale %d neg 0 translate\n", y_sizes[0] + CHANNEL_HEIGHT);
	fprintf(f2,"    90 rotate 0 %d neg translate\n", y_sizes[0] + CHANNEL_HEIGHT);
    }

    
    /* Write the tile spaces now: */
    solidp = (debug_level != 11) ? ignore_contents : solidp_for_printing; 
    ps_print_tile_space(routingRoot[HORZ][0], f1, ps_ignore_arc, TRUE); 
    ps_print_tile_space(routingRoot[VERT][0], f2, ps_ignore_arc, TRUE);

/*    write_local_route_to_file(f, nets); */

    if (latex != TRUE) 
    {
	fprintf(f1, "showpage\n");
	fprintf(f2, "showpage\n");
    }
}
/*------------------------------------------------------------------------------------*/
/* --------------------------------------------------------------
 * quality_statistics
 * This function provides statistics on how well the routing was 
 * done:  Statistics are # modules, # nets, # terminals, #cc @ level <l>...
 * # cc @ level 1, # foul-ups.
 * A signal flow foulup is defined as any non-cc module who's child is not
 * in an x-position greater-than its parent.
 * Other useful info is the filename, date, and all of the (specified?) 
 * command-line args.  All information is printed to a file "quality_stats"
 *---------------------------------------------------------------
 */
void quality_statistics()
{
    int cc_count[10];
    int xfouls = 0, yfouls = 0, term_count = 0, lev;
    int out_x, in_x, out_y, in_y, possibly_screwed, also_unscrewed;
    
    mlist *ml;
    module *m, *last_m, *cc_m, *top_cc_mod, *bot_cc_mod;
    tlist *t, *tt;

    FILE *f = fopen("quality_stats", "a"); 
    
    for (lev=0; lev < cc_search_level; lev++) cc_count[lev] = 0;
      
    last_m = modules->this;
    last_m->placed = UNPLACED;	/* for cross_coupled_p to work properly */
     
    term_count = list_length(last_m->terms);
    
    
    for(ml = modules->next; ml != NULL; ml = ml->next)
    {
	m = cc_m = ml->this;
	term_count += list_length(m->terms);
	
	/* Set up all the strangeness for cross-coupled modules: */
	for (lev = 1; (lev <= cc_search_level) && (cc_m != NULL); lev++)
	{
	    cc_m->placed = UNPLACED;
	    
	    if (cross_coupled_p(last_m, cc_m, lev) != FALSE)
	    {
		top_cc_mod = last_m;
		bot_cc_mod = cc_m;
		cc_count[lev-1] += 1;
		break;
	    }
	    cc_m = (ml->next != NULL) ? ml->next->this : NULL;
	}
	
	for (t = last_m->terms; t != NULL; t = t->next)
	{
	    if ((t->this->type == OUT) || (t->this->type == INOUT)) 
	    {
		possibly_screwed = FALSE;
		also_unscrewed = FALSE;
		
		for (tt = t->this->nt->terms; tt != NULL; tt = tt->next)
		{
		    if ((tt->this->type == IN) || (tt->this->type == INOUT)) 
		    {
			out_x = t->this->mod->x_pos + t->this->x_pos;
			in_x = tt->this->mod->x_pos + tt->this->x_pos;
			out_y = t->this->mod->y_pos + t->this->y_pos;
			in_y = tt->this->mod->y_pos + tt->this->y_pos;
			
			if (((t->this->mod == top_cc_mod)&&(tt->this->mod == bot_cc_mod)) ||
			    ((t->this->mod == bot_cc_mod)&&(tt->this->mod == top_cc_mod)))
			{  /* Skip cc modules */
			    also_unscrewed = TRUE;
			}
			
			else if (out_x > in_x) 
			{ xfouls += 1; }
			
			else if (out_y < in_y) 
			{ possibly_screwed = TRUE; }

			else 
			{ also_unscrewed = TRUE; }
			
		    }
		}
		if ((possibly_screwed == TRUE) && (also_unscrewed == FALSE)) yfouls += 1;
	    }
	}
	/* for next iteration: */
	last_m = m;
	last_m->placed = UNPLACED;
	
    }

    /* Done gathering statistics! */
    terminal_count = term_count;		/* Save this quantity in the golbal */

    fprintf(f,"------------------------------------------------------------------------------\n");
    
    fprintf(f,"| ?? -p%d -s%d -c%d -r%-4.2f -m%d \t ||",
	    partition_rule, max_partition_size, max_partition_conn,
	    partition_ratio, matrix_type);

    fprintf(f, "\t %d \t %d \t %d \t %dx%d = %d", 
	    xfouls, yfouls, xfouls+yfouls, x_sizes[0], y_sizes[0], x_sizes[0] * y_sizes[0]);
    
    fprintf(f," |\n");
    fclose(f);
    
}

/*------------------------------------------------------------------------------------*/
int falls_in_tile_p(t, c)
    tile *t;
    corner *c;
{
    /* returns TRUE iff corner <c> falls in tile <t> */
    return(corner_falls_in_arc_p(c, t->this));
}
/*------------------------------------------------------------------------------------*/
int crossover_count(t)
    tile *t;
{
    /* Count the number of crossovers within this tile */
    int crossoverCount = 0;
    asg_arc *pa, *a = (asg_arc *)t->this;
    tilist *tl, *tileSet = NULL;
    nlist *nl, *nll;
    corner *inCorn, *outCorn;
    crnlist *cl;
    int minXi, maxXi, minYi, maxYi;
    int minXo, maxXo, minYo, maxYo;

    /* get the set of perpendicular tiles that cross this tile */
    tileSet = (tilist*)collect_tiles(t, routingRoot[toggle_direction(a->page)][0]);

    for(tl = tileSet; tl != NULL; tl = tl->next)
    {
	pa = (asg_arc *)tl->this->this;
	
	for (nll = pa->nets; nll != NULL; nll = nll->next)
	{
	    cl = (crnlist *)member(tl->this, nll->this->route, falls_in_tile_p);
	    if (cl == NULL) break;
	    outCorn = cl->this;

	    for (nl = a->nets; nl != NULL; nl = nl->next)
	    {
		cl = (crnlist *)member(t, nl->this->route, falls_in_tile_p);
		if (cl == NULL) break;
		inCorn = cl->this;
		    
		/* NOTE: if <inCorn> and <outCorn> share a similar range, 
		   ignore them */
		if ((inCorn->cx != outCorn->cx) && (inCorn->cy != outCorn->cy))
		{
		    /* Skip it */
		}
		else if (a->page == VERT)
		{
		    /* Find the max vertical extent and lowest position for the 
		       segment associated with <inCorn>, and compare it to the max 
		       horizontal extent and position for the segment associated 
		       with <outCorn>.  If these two segments cross, count 'em. */
		    perpendicular_expanse(inCorn->cx, &minYi, &maxYi);
		    minXi = maxXi = inCorn->cx->sr->q1;
		    perpendicular_expanse(outCorn->cy, &minXo, &maxXo);
		    minYo = maxYo = outCorn->cy->sr->q1;

		    if ((int_overlapped_p(minXi, maxXi, minXo, maxXo) == TRUE) &&
			(int_overlapped_p(minYi, maxYi, minYo, maxYo) == TRUE))
		    {
			/* There is a crossover, so count it: */
			crossoverCount += 1;
		    }
		}
		else /* (a->page == HORZ) */
		{
		    /* Find the max horizontal extent and lowest position for the 
		       segment associated with <inCorn>, and compare it to the max 
		       vertical extent and position for the segment associated 
		       with <outCorn>.  If these two segments cross, count 'em. */
		    perpendicular_expanse(outCorn->cx, &minYo, &maxYo);
		    minXo = maxXo = outCorn->cx->sr->q1;
		    perpendicular_expanse(inCorn->cy, &minXi, &maxXi);
		    minYi = maxYi = inCorn->cy->sr->q1;

		    if ((int_overlapped_p(minXi, maxXi, minXo, maxXo) == TRUE) &&
			(int_overlapped_p(minYi, maxYi, minYo, maxYo) == TRUE))
		    {
			/* There is a crossover, so count it: */
			crossoverCount += 1;
		    }
		}
	    }
	}
    }
    return(crossoverCount);
}
/*------------------------------------------------------------------------------------*/
float crossover_value(t)
    tile *t;
{
    static int max = 0;
    int count, area;
    asg_arc *a;

    if (t == NULL) return ((float)max);

    a = (asg_arc*)t->this;
    count = crossover_count(t);
    max = MAX(max, count);
    area = t->x_len * t->y_len;

    ordered_ilist_insert(t, count, &crossovers);
    a->local = area;

    return((float)count/(float)area);
}
/*------------------------------------------------------------------------------------*/
float congestion_value(t)
    tile *t;
{
/* NOTE: This definition differs from that used in the global routing 
   routines.  See "congestion_cost" in file global_route.c	*/

    asg_arc *a = (asg_arc*)t->this;
    int count = list_length(a->corners);	
    int area = t->x_len * t->y_len;

    ordered_ilist_insert(t, count, &congestion);
    a->local = area;

    return((float)count/(float)area);
}
/*------------------------------------------------------------------------------------*/
reset_tile_congestion(t)
    tile *t;
{
    asg_arc *a = (asg_arc *)t->this;
    crnlist *cl;

    free_list(a->nets);		/* Scrap the old ->nets list */
    a->nets = NULL;

    for (cl = a->corners; cl != NULL; cl = cl->next)	/* Build a new ->nets list */
    {
	pushnew(cl->this->cx->n, &a->nets);		/* Mark the route used */
    }
    (*set_congestion_value)(a);
}
/*------------------------------------------------------------------------------------*/
void ps_print_tile_border(f, a)
    FILE *f;		/* Where to write things */
    asg_arc *a;		/* The asg_arc to be printed */
{
    /* This function prints a dotted line that corresponds to the border of the
       tile in question. */
    tile *t = a->t;		/* The tile to be printed */
    int x1, y1, x2, y2;
    
    if (t != NULL)
    {
	if (a->page == HORZ)	/* Get the orientation straight */
	{
	    x1 = t->x_pos;		y1 = t->y_pos;
	    x2 = x1 + t->x_len;		y2 = y1 + t->y_len;
	}
	else 
	{
	    y1 = t->x_pos;		x1 = t->y_pos;
	    y2 = y1 + t->x_len;		x2 = x1 + t->y_len;
	}
	
        fprintf(f,"%%%% Tile border for %x LLHC(%d,%d), URHC=(%d,%d) %%%%\n", 
		t, x1, y1, x2, y2);
        fprintf(f,"[1 2] 0.2 setdash\n");
	fprintf(f, "newpath %d %d moveto %d %d lineto ", x1, y1, x1, y2); /* left edge */
	fprintf(f, "%d %d lineto %d %d lineto ", x2, y2, x2, y1); /* top & right edges */
	fprintf(f, "%d %d lineto stroke\n", x1, y1);			/* bottom edge */
	fprintf(f, "[] 0 setdash\n");
    }
}
/*------------------------------------------------------------------------------------*/
float quality_ranking(manPWS,congDenAve,congCount, termCount)
    float manPWS,congDenAve;
    int congCount, termCount;
{
    /* This returns the appropriate scaling for the relative ranking of
       this diagram.  Ideally, among a set of diagrams, the one with the
       lowest value is "the best" diagram, etc.. */

    float PWS_const = 4.0;
    float CD_const = 12.0;
    float CC_const = 4.0;
    float sum;
    
    sum = (PWS_const/manPWS) + (CD_const * congDenAve) + 
          ((float)congCount/(float)termCount/CC_const);
    return(sum);
}
/*------------------------------------------------------------------------------------*/
void collect_congestion_stats(page)
    int page;
{
    /* This function evaluates the route for congestion statistics information */
    /* The desired values are the average congestion-per-tile, the standard deviation
       of the congestion-per-tile, and the tile that is most congested. */

    tile *vRoot = routingRoot[VERT][page], *hRoot = routingRoot[HORZ][page], *t;
    tilist *vRoutingTiles = NULL, *hRoutingTiles = NULL, *tl;
    asg_arc *a, *worstCong, *worstCross;
    ilist *il, *ill;
    int area, i, congCount = 0, crossCount = 0, conge=0, cros=0;
    float congSum = 0.0, congSumSquared = 0.0, count = 0.0, cong;
    float crosSum = 0.0, crosSumSquared = 0.0, cross;
    float vCongSum, vCongSumSquared, vCrosSum, vCrosSumSquared, vnum;
    float hCongSum, hCongSumSquared, hCrosSum, hCrosSumSquared, hnum;

    float vCongStandardDeviation, vCongAverage, hCongStandardDeviation, hCongAverage;
    float vCrosStandardDeviation, vCrossAverage, hCrosStandardDeviation, hCrossAverage;
    float congDeviation, congAverage, crossDeviation, crossAverage;

    FILE *qsf = fopen("qual_stats", "a");

    fprintf (stderr, "Collecting Routing statistics in file qual_stats.\n");

    freespace(vRoot, &vRoutingTiles);
    freespace(hRoot, &hRoutingTiles);

    /* Collect congestion and crossover stats for the vertical tiles: */
    for (tl = vRoutingTiles; tl != NULL; tl = tl->next)
    {
	reset_tile_congestion(tl->this);
	cong = congestion_value(tl->this);	cross = crossover_value(tl->this);
	congSum += cong;			crosSum += cross;
	congSumSquared += cong * cong;		crosSumSquared += cross * cross;
	count += 1.0;
    }

    vnum = count;
    vCongSum = congSum;			vCrosSum = crosSum;
    vCongSumSquared = congSumSquared;	vCrosSumSquared = crosSumSquared;

    vCongAverage = vCongSum/vnum;
    vCongStandardDeviation = sqrt(vCongSumSquared/vnum - vCongAverage * vCongAverage);

    vCrossAverage = vCrosSum/vnum;
    vCrosStandardDeviation = sqrt(vCrosSumSquared/vnum - vCrossAverage * vCrossAverage);

    /* Collect congestion and crossover stats for the horizontal tiles: */
    for (tl = hRoutingTiles; tl != NULL; tl = tl->next)
    {
	reset_tile_congestion(tl->this);
	cong = congestion_value(tl->this);	cross = crossover_value(tl->this);
	congSum += cong;			crosSum += cross;
	congSumSquared += cong * cong;		crosSumSquared += cross * cross;
	count += 1.0;
    }

    hnum = count - vnum;
    hCongSum = congSum - vCongSum;	     	hCrosSum = crosSum - vCrosSum;
    hCongSumSquared = congSumSquared - vCongSumSquared;	
    hCrosSumSquared = crosSumSquared - vCrosSumSquared;

    hCongAverage = (float)hCongSum/(float)hnum;
    hCongStandardDeviation = sqrt((float)hCongSumSquared/(float)hnum - 
				  hCongAverage * hCongAverage);

    hCrossAverage = (float)hCrosSum/(float)hnum;
    hCrosStandardDeviation = sqrt((float)hCrosSumSquared/(float)hnum -
				  hCrossAverage * hCrossAverage);

    /* Now do funny things to only do statistics on the worst 10% of the tiles */
    congSum = 0.0;	crosSum = 0.0;
    count = (float)(int)((ilist_length(congestion) * 0.100) + 0.5);

    ill = reverse_copy_ilist(congestion);
    for (il = ill; ((il != NULL) && (il->index > 0)); il = il->next)
    {
	t = (tile*)il->this;
	a = (asg_arc*)t->this;	
	area = a->local;
	congSum += (float)il->index/(float)area;
	congCount += 1;
	conge += il->index;			/* Total congestion */
	if (il == ill)
	{
	    worstCong = (asg_arc*)t->this;
	    worstCong->congestion = ill->index;
	    ps_print_tile_border(stdout, worstCong);
	}
    }
    free_ilist(ill);
    ill = reverse_copy_ilist(crossovers);
    for (il = ill; ((il != NULL) && (il->index > 0)); il = il->next)
    {
	t = (tile*)il->this;
	a = (asg_arc*)t->this;	
	area = a->local;
	crosSum += (float)il->index/(float)area;
	crossCount += 1;
	cros += il->index;			/* Total Crossovers */
	if (il == ill) 
	{
	    worstCross = (asg_arc*)t->this;
	    worstCross->count = ill->index;
	}
    }
    free_ilist(congestion);	free_ilist(crossovers);

    congAverage = (float)congSum/(float)congCount;
    congDeviation = sqrt((float)congSumSquared/(float)congCount - 
			 congAverage * congAverage);

    crossAverage = (float)crosSum/(float)crossCount;
    crossDeviation = sqrt((float)crosSumSquared/(float)crossCount - 
			  crossAverage * crossAverage);

    /* Now that the calculations have been done, print out the line with the info: */
    fprintf(qsf,"------------------------------------------------------------------------------\n");
    
    fprintf(qsf,"?? -p%d -s%d -c%d -r%-4.2f %s %s %s\t ||",
	    partition_rule, max_partition_size, max_partition_conn,
	    partition_ratio, (stopAtFirstCut == TRUE) ? "-f" : "--", 
	    (useSlivering == FALSE) ? "-v" : "--", (useAveraging == TRUE) ? "-w" : "--");

    fprintf(qsf, "\t %-6.4f \t ", congAverage);

    fprintf(qsf, "%d \t %d  |", conge, worstCong->congestion);

    fprintf(qsf, "\t %-6.4f \t", crossAverage);

    fprintf(qsf, "%d \t %d  |", cros, worstCross->count);
    
    fprintf(qsf, "\t| %-6.4f |", quality_ranking(manhattan_PWS(), congAverage, 
						 conge, terminal_count));

    fprintf(qsf,"\n");
    fclose(qsf);

    /* Now print the outline of the worst congested tile: */
}			

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

/* END OF FILE "local_route.c" */
