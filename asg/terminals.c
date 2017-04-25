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


/* file terminals.c */
/* ------------------------------------------------------------------------
 * Terminal Placement routines                         stf 1/91
 * Implemented as part of the incremental routing solution for SPAR
 * MS Thesis work, UPittsburgh School of Engineering.
 * ------------------------------------------------------------------------
 */
#include "terminals.h"	/* The conn structures, used to link modules to net fragments */
#include "psfigs.h"

/*---------------------------------------------------------------
 * Forward Declarations
 *---------------------------------------------------------------
 */

static int startX, endX;	/* globals , of sorts */
/*===================================================================================*/
int srange_width(sr)
    srange *sr;
{
    return(sr->q2 - sr->q1);
}
/*===================================================================================*/
int spans_range_p(t)
    tile *t;
{
    /* Return TRUE if the given tile spans the x-range specified by the variables
       <startX> and <endX>, and has an empty arc (no module contained). */

    if ((t->x_pos <= startX) && 
	(t->x_pos + t->x_len >= endX) &&
	(emptyp_for_arcs(t) == TRUE))		return(TRUE);
    else 					return (FALSE);
}
/*===================================================================================*/
connect *create_connect(m, sr, crl)
    module *m;
    srange *sr;
    crnlist *crl;
{
    term *t = m->terms->this;
    
    connect *cn = (connect *)calloc(1, sizeof(connect));
    cn->exit = sr;					/* Fill in later */
    cn->up = m->y_size - t->y_pos;
    cn->down = t->y_pos;
    cn->mod = m;
    cn->corners = crl;

    return(cn);
}
/*===================================================================================*/
int spans_overlapped_p(s1, s2)
    connect *s1, *s2;
{
    /* Returns TRUE if the two spans overlapp in the least. */
    srange *r1 = s1->exit, *r2 = s2->exit;
    if ((r1->q2 + s1->up >= r2->q1 - s2->down) && 
	 (r1->q1 - s1->down <= r2->q1 + s2->up))  return(TRUE);
    else return(FALSE);
}
/*------------------------------------------------------------------------------------*/
paint_corners(m)
    module *m;
{ 
    /* Paint all of the corners that are fixed in the y dimension to terminals to 
       be painted with the terminal to whom they are fixed. */
    term *t;
    crnlist *cl, *cll;
    range *fixedRange;
    net *n;

    n = m->terms->this->nt;
    for(cl = (crnlist *)n->route; cl != NULL; cl = cl->next)
    {
	if (cl->this->t != NULL)
	{
	    t = cl->this->t;
	    fixedRange = cl->this->cy;
	    for (cll = (crnlist *)n->route; cll != NULL; cll = cll->next)
	    {
		if (cll->this->cy == fixedRange) cll->this->t = t;
	    }
	}
    }    
}
/*------------------------------------------------------------------------------------*/
int terminal_point_to_save(sr, corners)
    srange *sr;
    crnlist *corners;
{
    crnlist *cl;
    int cy = TILE_SPACE_Y_POS; 				/* big negative number */

    for (cl = corners; cl != NULL; cl = cl->next)
    {
	if (cl->this->t != NULL)
	{
	    sr->orient = cl->this->cy->sr->orient;
	    if ((overlapped_p(cl->this->cy->sr, sr) == TRUE) &&
		(cl->this->cy->sr->q1 > cy))
	    {
		cy = cl->this->cy->sr->q1;	/* Get the highest corner in the range */
	    }		
	}
    }
    return(cy);	
}   
/*------------------------------------------------------------------------------------*/
int insert_gap_between_sranges(sr1, sr2, gap, crl1, crl2)
    srange *sr1, *sr2;  	/* sranges associated with each net that overlap */
    int gap;			/* How much of a gap goes between the two */
    crnlist *crl1, *crl2;	/* The corners that produced these ranges */
{	
    /* This puts <sr1> to the right of (above) <sr2> by <gap> */
    int midPoint, c1, c2, c;
    int min1 = sr1->q1, min2 = sr2->q1;
    int max1 = sr1->q2, max2 = sr2->q2;

    if (min2 == max2) /* Handle the point cases - these are the most common */
    {
	if (min1 >= min2 + gap) return(TRUE);	  	/* <sr2> to right of <sr1> */
	else if (max1 <= min2 - gap) return(TRUE);	/* <sr1> to right of <sr2> */
	else if (min1 <= min2 - gap) 		    /* fit <sr1> to right of <sr2> */
	{
	    sr1->q2 = min2 - gap;
	    return(TRUE);
	}
	else if (max1 >= max2 + gap)	             /* fit <sr1> to left of <sr2> */
	{
	    sr1->q1 = max2 + gap;
	    return(TRUE);
	}
	else  return(FALSE);    			/* <sr1> doesn't fit!!! */
    }

    if (min1 == max1) /* Handle the point cases - these are the most common */
    {
	if (min2 >= min1 + gap) return(TRUE);	  	/* <sr1> to right of <sr2> */
	else if (max2 <= min1 - gap) return(TRUE);	/* <sr2> to right of <sr1> */
	else if (min2 <= min1 - gap) 		    /* fit <sr2> to right of <sr1> */
	{
	    sr2->q2 = min1 - gap;
	    return(TRUE);
	}
	else if (max2 >= max1 + gap)	             /* fit <sr2> to left of <sr1> */
	{
	    sr2->q1 = max1 + gap;
	    return(TRUE);
	}
	else  return(FALSE);    			/* <sr1> doesn't fit!!! */
    }	
    else
    {
	/* Is there any overlap? */
	if ((max1 + gap < min2) || (max2 + gap < min1)) return(TRUE);  /* Fits already */
	else if (min2 + max1 < gap + 2) return(FALSE);		       /* Can't fit */

	midPoint = (max2 + min1) / 2;

	/* Set the beginning/end points of the sranges: */
	if (max1 < midPoint) min1 = sr1->q1 = sr1->q2 = min1;
	else if (min2 > midPoint) max2 = sr2->q1 = sr2->q2 = min2;

	else if ( (midPoint >= min2) || ((max1 - min1) >= (max2 - min2)) )
	{   	/* <sr1> goes to the right of <sr2> */
	    sr1->q1 = MIN(MAX(min1, midPoint), max1);
	    sr2->q2 = MAX(MIN(midPoint, max2), min2);
	}	
	else 
	{   /* <sr2> goes to the right of <sr1> */
	    sr2->q1 = MIN(MAX(min1, midPoint), max2);
	    sr1->q2 = MAX(MIN(midPoint, max2), min1);
	}
	/* now figure out how to fit the <gap> in: (It should fit) */
	min1 = sr1->q1; 	min2 = sr2->q1;
	max1 = sr1->q2; 	max2 = sr2->q2;

	c1 = terminal_point_to_save(sr1, crl1);	/* maintain corners whenever possible */
	c2 = terminal_point_to_save(sr2, crl2);

	if (((c1 == TILE_SPACE_Y_POS) && (c2 == TILE_SPACE_Y_POS)) ||
	    ((c1 != TILE_SPACE_Y_POS) && (c2 != TILE_SPACE_Y_POS) && 
	     (MAX(c1, c2) - MIN(c1, c2) <= gap)) )
	{
	    /* Ignore corner inclusion: */
	    /* put <sr1> to left of <sr2> with the gap midway between them */
	    if (min1 <= min2)
	    {
		if ((min1 <= midPoint - gap/2) && (max2 >= midPoint + gap/2))
		{
		    sr1->q2 = midPoint - gap/2;
		    sr2->q1 = midPoint + gap/2;
		}
		else if (max2 >= min2 + gap)	sr2->q1 += gap;   /* clip 2 */
		else if (min1 <= max1 - gap)	sr1->q2 -= gap;   /* clip 1 */
		else 
		{
		    gap -= (max2 - min2);
		    sr2->q1 = max2;
		    sr1->q2 -= gap;
		}
		return(TRUE);
	    }
	    else  /* put <sr2> to left of <sr1> with the gap midway between them */
	    {
		if (min1 >= midPoint - gap/2)
		{
		    sr2->q2 = midPoint - gap/2;
		    sr1->q1 = midPoint + gap/2;
		}
		else if (min2 <= max2 - gap)	sr2->q2 -= gap;   /* clip 2 */
		else if (max1 >= min1 + gap)	sr1->q1 += gap;   /* clip 1 */
		else /* Stuff it */
		{
		    sr2->q1 = max2 - (max2 - gap - min1)/2;
		    sr1->q2 = min1 + (max2 - gap - min1)/2;
		}	
		return(TRUE);
	    }
	}
	else if ((c1 != TILE_SPACE_Y_POS) && (c2 != TILE_SPACE_Y_POS))
	{
	    /* Deal with both corners */
	    /* put <sr1> to left of <sr2> with the gap midway between them */
	    if ((min1 <= midPoint - gap/2) && (max2 >= midPoint + gap/2))
		
	    {
		if ((c1 <= midPoint - gap/2) && (c2 >= midPoint + gap/2))
		{
		    sr1->q2 = midPoint - gap/2;
		    sr2->q1 = midPoint + gap/2;
		}
		else return(FALSE);
	    }
	    /* put <sr2> to left of <sr1> with the gap midway between them */
	    else if (min1 >= midPoint - gap/2)  /* Can't trim sr1 */
	    {
		if (c2 - gap >= max1)    sr2->q1 = sr2->q2 = c2;
		
		else if ((c1 <= min1 + (max2 - min1 - gap)/2) && 
		    ((c2 >= max2 - (max2 - min1 - gap)/2)))
		{
		    sr1->q2 = min1 + (max2 - min1 - gap)/2;
		    sr2->q1 = max2 - (max2 - min1 - gap)/2;
		}
		else return(FALSE);
	    }
	    else if (max2 <= midPoint + gap/2)	/* Can't trim sr2 */
	    {
		if (c1 + gap <= min2) sr1->q1 = sr1->q2 = c1;
		
		else return(FALSE);
	    }

	    /* See if they both work: */
	    else if ((c1 < c2) && (c1 + gap <= c2))
	    {
		sr1->q1 = sr1->q2 = c1;
		sr1->q2 = sr1->q2 = c2;
	    }

	    else return(FALSE);

	    return(TRUE);	    
	}
	else	/* deal with one corner or less */
	{
	    if ((c1 != TILE_SPACE_Y_POS) && (c1 - gap > min2))
	    {	/* Put <sr1> to the right of <sr2>, fixing <sr1> at point <c1>: */
		sr1->q1 = sr1->q2 = c1;		
		sr2->q2 = MAX(min2, MIN(c1 - gap, max2));
	    }
	    else if ((c2 != TILE_SPACE_Y_POS) && (c2 - gap < max1))
	    {	/* put <sr1> to the right of <sr2>, fixing <sr2> at point <c2>: */
		sr2->q1 = sr2->q2 = c2;
		sr1->q1 = MIN(max1, MAX(c2 + gap, min1));
	    }

	    /* Bag it */
	    /* put <sr1> to left of <sr2> with the gap midway between them */
	    else if ((min1 <= midPoint - gap/2) && (max2 >= midPoint + gap/2))
	    {
		sr1->q2 = midPoint - gap/2;
		sr2->q1 = midPoint + gap/2;
	    }
	    /* put <sr2> to left of <sr1> with the gap midway between them */
	    else if (min1 >= midPoint - gap/2)
	    {
		sr1->q2 = midPoint + gap/2;
		sr2->q1 = midPoint - gap/2;
	    }
	    else /* Stuff it */
	    {
		sr2->q1 = max2 - (max2 - gap - min1)/2;
		sr1->q1 = min1 + (max2 - gap - min1)/2;
	    }	
	    return(TRUE);
	}
    }
}
/*===================================================================================*/
int separate_spans(s1, s2)
    connect *s1, *s2;
{
    /* return TRUE and modify the two spans if some legal selections of <s1>->exit
       and <s2>->exit can be chosen so as to avoid overlap between the two:
     */
    srange *r1 = s1->exit, *r2 = s2->exit;

    if (((r1->q2 + s1->up > r2->q1 - s2->down) && 
	 (r1->q1 - s1->down < r2->q1 + s2->up)) ||
	((r1->q2 + s1->up < r2->q1 - s2->down) && 
	 (r1->q1 - s1->down > r2->q1 + s2->up)))
	
    {
	if ((r1->q1 < r2->q1) || 			 /* Put <r1> below <r2> */
	    ((r1->q1 == r2->q1) && (r1->q2 < r2->q2)))	
	{
	    if (srange_width(r1) == 0) 
	    {
		if (r2->q2 >= r1->q2 + s1->up + s2->down)	/* Close shave */
		{
		    r2->q1 = r1->q2 + s1->up;
		}
		else return(FALSE);   	/* No Go */	       	
	    }
	    else if (r1->q1 + r2->q2 <= s1->up + s2->down + 2) 
	    {
		return(FALSE);   	/* No Go */	       	
	    }
	    else /* Normal Case */
	    {
		return(insert_gap_between_sranges(r2, r1, s2->up + s1->down, 
						  s1->corners, s2->corners));
	    }
	}
	else 			/* Put <r2> below <r1> */
	{
	    if (srange_width(r1) == 0) 
	    {
		if (r1->q2 >= r2->q2 + s2->up + s1->down)	/* Close shave */
		{
		    r1->q1 = r2->q2 + s2->up;
		}
		else 
		{
		    return(FALSE);   	/* No Go */	       	
		}
	    }
	    else if (r2->q1 + r1->q2 <= s2->up + s1->down + 2) 
	    {
		return(FALSE);   	/* No Go */	       	
	    }	
	    else
	    {
		return(insert_gap_between_sranges(r1, r2, s1->up + s2->down, 
						  s1->corners, s2->corners));
	    }
	}
	
	if((srange_width(r2) < 0) || (srange_width(r1) < 0))
	{
	    return(FALSE);			/* Somethin's screwed */
	}
	else return(TRUE);	/* No overlap problems. */	
    }
    else return(TRUE);	/* No overlap problems. */
}
/*===================================================================================*/
int descending_order(sp1, sp2)
    connect *sp1, *sp2;
{
    /* Return TRUE iff sp1 is less than sp2 */
    if (sp1->exit->q2 <= sp2->exit->q1) return(TRUE);
    else return(FALSE);
}
/*===================================================================================*/
int ascending_order(sp1, sp2)
    connect *sp1, *sp2;
{
    /* Return TRUE iff sp1 is less than sp2 */
    if (sp1->exit->q2 <= sp2->exit->q1) return(FALSE);
    else return(TRUE);
}
/*===================================================================================*/
int check_spanset_p(span, spanSet)
    connect *span;
    connlist *spanSet;
{
    /* Return TRUE iff the given <span> fits in the <spanset>   Make no alterations. */
    connlist *srl;
    int min1 = span->exit->q1, min2, max1 = span->exit->q2, max2;

    if (spanSet == NULL) return(TRUE);

    sort_list(spanSet, descending_order);
    for (srl = spanSet; srl != NULL; srl = srl->next) /* <spanSet> is sorted */
    {
	min2 = srl->this->exit->q1;
	max2 = srl->this->exit->q2;
	if (separate_spans(span, srl->this) != TRUE)
	{
	    /* restore values */
	    span->exit->q1 = min1; 	srl->this->exit->q1 = min2; 
	    span->exit->q2 = max1;	srl->this->exit->q2 = max2;
	    return(FALSE);
	}
	else 
	{   /* Repair side effects */
	    srl->this->exit->q1 = min2; 	srl->this->exit->q2 = max2;
	}
    }
    /* restore values */
    span->exit->q1 = min1;	span->exit->q2 = max1;	
    return(TRUE);
}
/*===================================================================================*/
int fits_in_spanset_p(span, spanSet)
    connect *span;
    connlist *spanSet;
{
    /* Return TRUE iff the given <span> fits in the <spanset>   Make no alterations. */
    connlist *srl;
    int min1 = span->exit->q1, min2, max1 = span->exit->q2, max2;

    if (spanSet == NULL) return(TRUE);

    sort_list(spanSet, descending_order);
    for (srl = spanSet; srl != NULL; srl = srl->next) /* <spanSet> is sorted */
    {
	min2 = srl->this->exit->q1;
	max2 = srl->this->exit->q2;
	if (separate_spans(span, srl->this) != TRUE)
	{
	    /* restore values */
	    span->exit->q1 = min1; 	srl->this->exit->q1 = min2; 
	    span->exit->q2 = max1;	srl->this->exit->q2 = max2;
	    return(FALSE);
	}
	if (span->exit->q2 < min2) break; 
    }
    return(TRUE);
}
/*===================================================================================*/
int unobstructed_p(vertSRange, horzSRange, span, usedSpans, exitSide)
    srange *vertSRange, *horzSRange;
    connect *span;
    connlist *usedSpans;
    int exitSide;    
    
{
    /* Return TRUE if the some of the given <vertSRange> casn be traced from <horzSRange>
       to the given <exitSide> of the page.  If it can, correct the <vertSRange> to the 
       part of the range that will connect. */

    int yExtent = srange_width(vertSRange), xExtent;
    connect vSpan;
    srange vSRange;
    tile *corn, *lasTile = NULL;
    tilist *tl, *marker, *goodTiles = NULL;
    
    /* Set up the local span variables.  These are used to verify the usefulness 
       of a located <horzSRange> so that a good one is returned, if one exists.  */
    vSRange.orient = VERT;
    vSpan.exit = &vSRange;
    vSpan.up = span->up;
    vSpan.down = span->down;
    vSpan.mod = span->mod;
    vSpan.corners = span->corners;

    /* The <vertSRange>, <horzSRange> and the <exitSide> determine a rectangle that can
       be searched to see if there is anything that spans the distance:
     */

    if (exitSide == RIGHT) 
    {
	corn = find_LRHC(routingRoot[0][HORZ]);
	xExtent = corn->x_pos + corn->x_len - horzSRange->q1;
    }
    else 
    {
	corn = find_ULHC(routingRoot[0][HORZ]);
	xExtent = corn->x_pos - horzSRange->q2;
    }

    /* Set the span range for the filter function: */	
    startX = MIN(horzSRange->q1, horzSRange->q1 + xExtent);
    endX = MAX(horzSRange->q1, horzSRange->q1 + xExtent);

    area_enumerate(corn, &goodTiles, spans_range_p, 
		   horzSRange->q1, vertSRange->q1 ,xExtent, yExtent);

    /* Something to deal with - form the real continuous span from this info: */
    if (exitSide == LEFT) 
        sort_list(goodTiles, most_up); /* Start placing outputs to LL */
    else
        sort_list(goodTiles, most_down);   /* Start placing inputs to UR */

    /* Set up the loop stuff: */
    marker = goodTiles;
    while(marker != NULL)
    {
	lasTile = marker->this;
	vSRange.q1 = lasTile->y_pos;
	vSRange.q2 = lasTile->y_pos + lasTile->y_len;
	tl = marker->next;

	/* Setup the next contiunous, unobstructed vertical srange:	*/
	while ((tl != NULL) && (adjacent_tiles_p(tl->this, lasTile) == TRUE))
	{
	    /* Now find the maximum y-span available. (Best-First) */
	    vSRange.q1 = MIN(vSRange.q1, tl->this->y_pos);
	    vSRange.q2 = MAX(vSRange.q2, tl->this->y_pos + tl->this->y_len);	
	    lasTile = tl->this;	/* Advance the loop index */
	    tl = tl->next;
	}
	marker = tl;	/* Update the marker */

	/* Check to see if this unobstructed range will work: */
	if (fits_in_spanset_p(&vSpan, usedSpans) == TRUE)
	{
	    /* Reset the range to that which is unobstructed */
	    vertSRange->q1 = vSRange.q1;
	    vertSRange->q2 = vSRange.q2;
	    free_list(goodTiles);
	    return(TRUE);
	}
	/* Otherwise, Loop again to see if there is another one that will work */
    }
    free_list(goodTiles);
    return(FALSE);
}
/*===================================================================================*/
crnlist *find_next_closest_range(vertSRange, horzSRange, orderedCorners)
    srange *vertSRange, **horzSRange;
    crnlist **orderedCorners;
{
    /* Given the list of <orderedCorners> to chose from, pop the next set of corners
       that shares the same <horzSRange>.  From this set of corners, fill in the superset
       of xpositions into the <vertSRange> slots.  Stuff the corner(s) used to create
       this range into a list and return it.

       LOTS OF SIDE EFFECTS!!!
     */

    crnlist *cl, *used = NULL;
    range *xR;
    if (*orderedCorners != NULL)
    {
	xR = (*orderedCorners)->this->cx;
	vertSRange->q1 = (*orderedCorners)->this->cy->sr->q1;
	vertSRange->q2 = (*orderedCorners)->this->cy->sr->q2;
	
	for(cl = *orderedCorners; ((cl != NULL) && (cl->this->cx == xR)); 
	    cl = *orderedCorners)
	{
	    /* Shorten <orderedCorners>, lengthen <used> & update the <vertSRange> : */
	    trans_item(cl->this, orderedCorners, &used);
	    vertSRange->q1 = MIN(vertSRange->q1, cl->this->cy->sr->q1);
	    vertSRange->q2 = MAX(vertSRange->q2, cl->this->cy->sr->q2);
	}
	*horzSRange = xR->sr;
	return(used);	

    }
    else /* Given nothing to work with */
    {
	*horzSRange = NULL;
	vertSRange->q1 = 0;
	vertSRange->q2 = 0;
	return(NULL);		
    }
}
/*===================================================================================*/
term *select_non_systerm(terms, side)
    tlist *terms;
    int side;
{
    /* Return the first non-system terminal. */
    module *m;
    tile *corn;
    term *closest = NULL;
    tlist *tl;
    int mType, base, smallestDist;

    if (side == RIGHT) corn = find_LRHC(routingRoot[0][HORZ]);
    else corn = find_ULHC(routingRoot[0][HORZ]);
    base = (side == LEFT) ? corn->x_pos : corn->x_pos + corn->x_len;
    smallestDist = -1;

    for (tl = terms; tl != NULL; tl = tl->next)
    {
	mType = validate(tl->this->mod->type);
	if ((mType != INPUT_SYM) && (mType != OUTPUT_SYM) &&
	    (mType != INOUT_SYM)) 
	{
	    m = tl->this->mod;
	    if ((abs(base - m->x_pos - tl->this->x_pos) <= smallestDist) ||
		(smallestDist == -1));
	    {
		closest = tl->this;
		smallestDist = abs(base - m->x_pos - closest->x_pos);
	    }
	}
    }
    if (closest == NULL)
        fprintf(stderr, "error in select_non_systerm --> no terminal found!\n");
    return(closest);
}

/*===================================================================================*/

srange *locate_best_placement_range_for_systerm(span, fromSide, usedSpans)
    connect *span;		/* The Span being built */
    int fromSide;		/* LEFT or RIGHT */
    connlist *usedSpans;
{
    /* This is a best-first algorithm;  Other (collective) limitations are not
       included (yet).  Probably a running list of available ranges should be maintained
       and checked against in the "unobstructed_p" function.
     */
    module *systerm = span->mod;
    srange *dummyVRange, *dummyHRange;
    net *n = systerm->terms->this->nt;
    corner *c;
    crnlist *cl, *AllCorners, *used;
    term *t;
    int refX, refY;

    if (fromSide == RIGHT) 
    {
	AllCorners = (crnlist *)rcopy_list(sort_list(n->route, in_x_order));
    }
    else 
    {
	AllCorners = (crnlist *)copy_list(sort_list(n->route, in_x_order));
    }
    
    if (AllCorners == NULL) /* There is only one terminal connected, so fake it : */
    {
	t = select_non_systerm(n->terms, fromSide);
	refX = t->x_pos + t->mod->x_pos;
	refY = t->y_pos + t->mod->y_pos;	
	c = create_corner(create_range(create_srange(refX, refX, VERT), n),
			  create_range(create_srange(refY, refY, HORZ), n), NULL, t);
	push(c, &AllCorners);
    }
    for (cl = AllCorners; cl != NULL; cl = cl->next)
    {
	/* Walk through the corner list, and find the left/right-most unobstructed
	   range to use: */	
	/* First, create the dummy range to fill: */
	dummyVRange = create_srange(0, 0, VERT);

	/* Next fill the range: */
	used = find_next_closest_range(dummyVRange, &dummyHRange, &AllCorners);

	while((AllCorners != NULL) && (srange_width(dummyHRange) > 0))
	{
	    span->corners = used;
	    if (unobstructed_p(dummyVRange, dummyHRange, span, usedSpans, fromSide)
		== TRUE)
	    {
		span->exit = dummyHRange;
		if (fits_in_spanset_p(span, usedSpans) == TRUE)
		{
		    /* If there is a corner in this span, pick the highest one and
		       fix the location to it: */
		    refY = terminal_point_to_save(dummyVRange, span->corners);
		    if (refY != TILE_SPACE_Y_POS)
		    {
			dummyVRange->q1 = dummyVRange->q2 = refY;
		    }
		    span->exit = dummyVRange;
	            return(dummyVRange);
		}
	    }
	    else
	        used = find_next_closest_range(dummyVRange, &dummyHRange, &AllCorners);
	}

	span->corners = used;
	if (unobstructed_p(dummyVRange, dummyHRange, span, usedSpans, fromSide) == TRUE)
	{
	    span->exit = dummyVRange;
	    if (fits_in_spanset_p(span, usedSpans) == TRUE)
	    {
		refY = terminal_point_to_save(dummyVRange, span->corners);
		if (refY != TILE_SPACE_Y_POS)
		{
		    dummyVRange->q1 = dummyVRange->q2 = refY;
		}
		span->exit = dummyVRange;
	        return(dummyVRange);
	    }
	}
    }
    span->exit = dummyVRange;		/* Give it something to work with */
    return(NULL);			/* Notify that this range doesn't fit */
}
/*===================================================================================*/
int falls_in_srange_p(x, sr)
    int x;
    srange *sr;
{
    if ((sr->q1 <= x) && (x <= sr->q2)) return(TRUE);
    else return(FALSE);
}
/*===================================================================================*/
int fits_in_before(sp, fixedSpan, side, usedSpans)
    connect *sp, *fixedSpan;
    int side;
    connlist *usedSpans;
{
    int p = sp->exit->q1;
    term *closesTerm;
    srange dummyXRange, dummyYRange;

    /* First see if a range can be found between <p> and <fixedSpan>->exit in which
       to locate <sp> */
    if (p > fixedSpan->exit->q1)
    {
	dummyYRange.q1 = fixedSpan->exit->q2;		dummyYRange.q2 = p;
    }
    else
    {
	dummyYRange.q1 = p;		dummyYRange.q2 = fixedSpan->exit->q1;
    }

    /* find the nearest (horizontally) terminal to line up on. */
    closesTerm = select_non_systerm(sp->mod->terms->this->nt->terms, side);    
    dummyXRange.q1 = dummyXRange.q2 = closesTerm->mod->x_pos + closesTerm->x_pos;

    if (unobstructed_p(&dummyYRange, &dummyXRange, sp, usedSpans, side) == TRUE)
    {
	sp->exit->q1 = dummyYRange.q1;
	sp->exit->q2 = dummyYRange.q2;
	if (fits_in_spanset_p(sp, usedSpans) == TRUE) return(TRUE);
    }

    /* Otherwise try fitting <sp> immediately before <fixedSpan> */
    sp->exit->q2 = fixedSpan->exit->q1 - sp->up - fixedSpan->down;
    sp->exit->q1 = sp->exit->q2;
    return (fits_in_spanset_p(sp, usedSpans));
}
/*===================================================================================*/
int fits_in_after(sp, fixedSpan, side, usedSpans)
    connect *sp, *fixedSpan;
    int side;
    connlist *usedSpans;
{
    int p = sp->exit->q1;
    term *closesTerm;
    srange dummyXRange, dummyYRange;

    /* First see if a range can be found between <p> and <fixedSpan>->exit in which
       to locate <sp> */
    if (p > fixedSpan->exit->q1)
    {
	dummyYRange.q1 = fixedSpan->exit->q2;		dummyYRange.q2 = p;
    }
    else
    {
	dummyYRange.q1 = p;		dummyYRange.q2 = fixedSpan->exit->q1;
    }

    /* find the nearest (horizontally) terminal to line up on. */
    closesTerm = select_non_systerm(sp->mod->terms->this->nt->terms, side);    
    dummyXRange.q1 = dummyXRange.q2 = closesTerm->mod->x_pos + closesTerm->x_pos;

    if (unobstructed_p(&dummyYRange, &dummyXRange, sp, usedSpans, side) == TRUE)
    {
	sp->exit->q1 = dummyYRange.q1;
	sp->exit->q2 = dummyYRange.q2;
	if (fits_in_spanset_p(sp, usedSpans) == TRUE) return(TRUE);
    }

    /*  Otherwise, try fitting <sp> immediately after <fixedSpan> */
    sp->exit->q1 = fixedSpan->exit->q2 + sp->down + fixedSpan->up;
    sp->exit->q2 = sp->exit->q1;
    return(fits_in_spanset_p(sp, usedSpans));
}
/*===================================================================================*/
fit_into_available_space(sp, side, usedSpans)
    connect *sp;
    int side;
    connlist **usedSpans;
{
    /* find something near sp->exit that will accomodate <sp> in <usedSpans> */
    srange *yRng;
    int p;
    connlist *before = NULL, *after = NULL;
    connlist *cnl, *bl, *al;

    center_range_on_single_point(sp->exit);
    p = sp->exit->q1;
    sort_list(*usedSpans, descending_order);
    
    /* Create a list of spans that occur <before> and <after> <sp>.  The break occurs
       before the span near where <sp> wants to fall: */
    for (cnl = *usedSpans; cnl != NULL; cnl = cnl->next)
    {
	if ((falls_in_srange_p(p, cnl->this->exit) == TRUE) ||
	    (p < cnl->this->exit->q1))
	{
	    after = cnl;
	    break;
	}
	else
	{
	    push(cnl->this, &before);
	}
    }
    
    /* (useAveraging == TRUE)??   Something to try later... */

    if ((after != NULL) && (before != NULL))
    {
	/* Which is closer? */
	if ((after->this->exit->q1 - p) < (p - before->this->exit->q2))
	{
	    /* after->this is, so try next to there first: */
	    if (fits_in_before(sp, after->this, side, *usedSpans) == TRUE)
	    {
		push(sp, usedSpans);
		return(TRUE);
	    }
	}
	else
	{
	    if (fits_in_after(sp, before->this, side, *usedSpans) == TRUE)
	    {
		push(sp, usedSpans);
		return(TRUE);
	    }

	}
    }

    /* Toggle back and forth between the before & after lists looking for a 
       legal place to insert <sp>: */
    bl = before;
    al = after;
    while ((al != NULL) && (bl != NULL))
    {
	if ((bl == NULL) || 				/* No before list left... */
	    (((al != NULL) && (bl != NULL)) && 		/* the <al> entry is closer */
	      ((al->this->exit->q1 - p) < (p - bl->this->exit->q2))))
	{
	    if ((fits_in_before(sp, al->this, side, *usedSpans) == TRUE) ||
		(fits_in_after(sp, al->this, side, *usedSpans) == TRUE))
	    {
		push(sp, usedSpans);
		return(TRUE);
	    }
	    else al = al->next;
	}

	if ((al == NULL) ||				/* no after list left... */
	    ((al != NULL) && (bl != NULL)))		/* the <bl> entry is closer */
	{
	    if ((fits_in_after(sp, bl->this, side, *usedSpans) == TRUE) ||
		(fits_in_before(sp, bl->this, side, *usedSpans) == TRUE))
	    {
		push(sp, usedSpans);
		return(TRUE);
	    }
	    else bl = bl->next;
	}
    }

    /* If you get this far, <sp> could not be fit into the middle of <usedSpans> */
    /* Force it on the end somewhere */

    bl = *usedSpans;
    for (cnl = after; ((cnl != NULL) && (cnl->next != NULL)); cnl = cnl->next);

    if ((cnl != NULL) && 
	((cnl->this->exit->q1 - p) < (p - bl->this->exit->q2)))
    {
	/* if closer to the top of the page: */
	sp->exit->q1 = cnl->this->exit->q2 + sp->down + cnl->this->up;
	sp->exit->q2 = sp->exit->q1;
    }
    else 
    {
	/* force it to the begining of <usedSpans>: */
	sp->exit->q2 = bl->this->exit->q1 - sp->up - bl->this->down;
	sp->exit->q1 = sp->exit->q2;
    }
    push(sp, usedSpans);	/* It Better Fit!!  BB Problems??? */

    return(FALSE);
}
/*===================================================================================*/
repair_failed_spans(failedSpans, side, goodSpans)
    connlist *failedSpans;
    int side;
    connlist **goodSpans;
{
    srange *yRng;
    connect *span;
    connlist *cnl;

    sort_list(failedSpans, ascending_order);
    for (cnl = failedSpans; cnl != NULL; cnl = cnl->next)
    {
	span = cnl->this;
	if (*goodSpans == NULL) push(span, goodSpans);
	else fit_into_available_space(span, side, goodSpans);
    }
}
/*===================================================================================*/
verify_all_spans(goodSpans, side, spanStillScrewed)
    connlist **goodSpans;
    int side;
    connlist **spanStillScrewed;
{
    srange *yRng;
    connect *span;
    connlist *master = NULL, *cnl;

    master = (connlist *)copy_list(sort_list(*goodSpans, ascending_order));
    for (cnl = master; cnl != NULL; cnl = cnl->next)
    {
	span = (connect *)rem_item(cnl->this, goodSpans);	
	if (fits_in_spanset_p(span, *goodSpans) == TRUE) push(span, goodSpans);
	else 
	{
	    fit_into_available_space(span, side, goodSpans);
	    push(span, spanStillScrewed);
	}
    }
    free_list(master);
}
/*===================================================================================*/
int locate_systerm(span, x_pos)
    connect *span;
    int x_pos;
{
    /* Finds and sets the best Y location for this module: use the given span info
     RULES:  
        (1) Pick a corner. Highest corner is prefered for inputs, lowest for outputs
	(2) Pick the center of the range spanned by the corners chosen
	(3) Pick the center of the available exit range
     */

    int min, max, cornerY;
    module *systerm;
    crnlist *cl;
    srange *exit;

    if ((span == NULL) || (span->exit == NULL)) return(FALSE);

    exit = span->exit;	
    systerm = span->mod;
    min = span->exit->q1;
    max = span->exit->q2;

    /* Rule (0) -- Use terminal poins */
    cornerY = terminal_point_to_save(exit, span->corners);
    if (cornerY != TILE_SPACE_Y_POS)
    {
	systerm->y_pos = cornerY - span->down;
	systerm->x_pos = x_pos;
	exit->q1 = exit->q2 = cornerY;
	return(TRUE);
    }
    
    /* Rule (1) */
    for (cl = span->corners; cl != NULL; cl = cl->next)
    {
	cornerY = cl->this->cy->sr->q1;

	/* If there is a corner that connects inside the  exit range, use it: */
	if (falls_in_srange_p(cornerY, exit) == TRUE)
	{
	    /* Found One! */
	    systerm->y_pos = cornerY - span->down;
	    systerm->x_pos = x_pos;
	    exit->q1 = exit->q2 = cornerY;
	    return(TRUE);
	}
	min = MIN(cornerY, min);	/* Expand the range that the term can fall in */
	max = MAX(cornerY, max);
    }
    /* Rule (2) */
    if (falls_in_srange_p((min+max)/2, exit) == TRUE)
    {
	systerm->y_pos = (min+max)/2 - span->down;
	systerm->x_pos = x_pos;
	exit->q1 = exit->q2 = (min+max)/2;
	return(TRUE);
    }
    else if (falls_in_srange_p((min+max+1)/2, exit) == TRUE)
    {
	systerm->y_pos = (min+max+1)/2 - span->down;
	systerm->x_pos = x_pos;
	exit->q1 = exit->q2 = (min+max+1)/2;
	return(TRUE);
    }
    else
    {
	/* Rule (3) */
	center_range_on_single_point(exit);		      /* see "local_route.c" */
	systerm->y_pos = exit->q1 - span->down;
	systerm->x_pos = x_pos;
	return(TRUE);
    }
}
/*===================================================================================*/
set_spans(spans, x_pos, leftovers, base, height)	
    connlist *spans;
    int x_pos;
    mlist **leftovers;		/* Things not dealt with that should have been */
    int *base, *height;
{
    connlist *col;
    module *m;

    /* Now that valid ranges have been set up, localize the ranges */
    for (col = spans; col != NULL; col = col->next)
    {
	/* Now use this information to place the modules.  Any terminal with an entry 
	   sould be placed directly.  Anything with a NULL ->exit field needs to be 
	   handled seperately. */
	m = col->this->mod;
	locate_systerm(col->this, x_pos);
	if (col->this->exit == NULL)
	{
	    push(m, leftovers);
	    m->placed = UNPLACED;
	}
	else
	{
	    /* Create the fuzzy (delayed-evaluation) position...  (not quite right) */
	    m->fx->item1 = create_int_varStruct(&m->x_pos);
	    m->fy->item1 = create_int_varStruct(&m->y_pos);

	    /* record the base & height values (for BB calculation) */
	    *base = MIN(*base, col->this->exit->q1 - col->this->down - CHANNEL_HEIGHT);
	    *height = MAX(*height, col->this->exit->q2 + col->this->up - 
			           *base + CHANNEL_HEIGHT);
	}
    }
}

/*===================================================================================*/
/*---------------------------------------------------------------
 * place_systerm_partitions(part, x_pos)	(Level 2)
 * place the modules in the given partition, given that all those in 
 * partitions[0] have been placed AND ROUTED (See "local_route.c")
 *
 * This is supposed to use the routing and terminal locations available to 
 * place the given terminals on the edge of the page (The x position given)

 *  So here is the plan:
    Find the list of terminals to be placed, and then see if there are nets associated 
    with them extant in the (routed) partition[0].  There will either be a wire (corner
    list) or at least a terminal.

    now the trick is to align as many terminals as possible on _unobstructed_ corners.
    So, what is an _unobstructed_ corner?  It is one in which no bends are required 
    to connect to it.  Properly done, this process should result in a set of modules	
    and a (vertical) range to tie them to.  Based on these range, place the modules.
    Anything without a placement range still needs to be placed as well, and there is
    the problem of insuring that the various ranges do not force the modules to
    overlap vertically.
 *
 *---------------------------------------------------------------
 */
place_systerm_partitions(part, x_pos, bb_base, bb_height)
    int part, x_pos;
    int *bb_base, *bb_height;
{
    int side;
    module *m;
    mlist *ml, *mml, *leftovers = NULL;
    srange *sr;
    connect *newSpan;
    connlist *col, *screwedSpans = NULL, *bestSpans = NULL;

    /* The partition number indexes either the input system terminals, or the
       output system terminals.  (see "place.c") */
    for (ml = partitions[part]; ml != NULL; ml = ml->next)
    {
	/* Reset the terminal information within the net structures: */
	add_terms_to_nets(ml->this);
	paint_corners(ml->this);

	newSpan = create_connect(ml->this, NULL, NULL);

	/* Find the best unobstructed range in which to place this module, 
	   and push it onto the <bestRanges> list */
	if (part == partition_count + 1) side = RIGHT;
	else side = LEFT;

	sr = locate_best_placement_range_for_systerm(newSpan, side, bestSpans);

	if (sr != NULL)
	{
	    newSpan->exit = sr;
	    queue(newSpan, &bestSpans);
	}
	else
	{
	    if (debug_level > 0)
	    fprintf(stderr, "Placement will be irregular for system terminal <%s>.\n",
		    ml->this->name);
	    /* Now mark the span, so it can be fitted later: */
	    push(newSpan, &screwedSpans);
	}
    }

    /* Now fix those that we are happy with: */
    set_spans(bestSpans, x_pos, &leftovers, bb_base, bb_height);

    /* Now find a way to adjust the failed spans so they will fit: */
    repair_failed_spans(screwedSpans, side, &bestSpans);
    set_spans(screwedSpans, x_pos, &leftovers, bb_base, bb_height);

    screwedSpans = (connlist *)free_list(screwedSpans);
    verify_all_spans(&bestSpans, side, &screwedSpans);
    set_spans(screwedSpans, x_pos, &leftovers, bb_base, bb_height);

    if (leftovers != NULL)
    {
	fprintf(stderr, "WARNING:  System Terminals on the %s side may overlap.\n",
		SSIDE(side));
    }

/* Cleanup: */
    for (ml = partitions[part]; ml != NULL; ml = ml->next)
    {
        /* move the partition to partition 0 */
	partitions[0] = (mlist *)concat_list(ml->this, partitions[0]);
	
	/* and mark it as placed */
	ml->this->placed = PLACED;
    }
    free_list(screwedSpans);
    free_list(leftovers);
    free_list(bestSpans);
}		
/*===================================================================================*/
/*---------------------------------------------------------------
 * fine_systerm_placement (Level 1)
 * Put all the partitions into one bounding box in partition 0, 
 * based on picking the largest number of connections off the diagonal 
 * of the connection matrix; (which has been built to match the 
 * partitions, seen as clusters)
 *---------------------------------------------------------------
 */
mlist *fine_systerm_placement(x1, y1, x2, y2)
    int *x1, *y1, *x2, *y2;			/* The current Bounding Box */
{
/* Now there are three partitions:  0, pc+1, & pc+2, which contain the (now placed)
 * internal modules, the output only system terminals, and the remaining systerms.
 * These remaining two sets of unplaced modules are placed using the information found by
 * examining the placement of the internal modules.  NOTE:  The systerms have been 
 * 'placed' by the boxing routines, so the information within the mod structures must 
 * be ignored.
 *
 */
    int outputColumn = *x2 - CHANNEL_LENGTH - SYSTERM_SIZE;
    int inputColumn = *x1 + CHANNEL_LENGTH;
    int yP = *y1, yL = *y2 - *y1;

/* Reset the placement information within the module structures: */

/* place the systerm modules within their partitions: */
    place_systerm_partitions(partition_count + 2, inputColumn, &yP, &yL); 
    place_systerm_partitions(partition_count + 1, outputColumn, &yP, &yL);

/* cleanup the bounding box (ganz wigtig! very important!) */
    if(*y1 != yP)
    {
	/*	if (debug_level != 0)  */
	fprintf(stderr, "Adjusting Bounding Box base from (%d,%d) to (%d,%d)\n", 
		*x1, *y1, *x1, yP - CHANNEL_HEIGHT);
	*y1 = yP - CHANNEL_HEIGHT; 
	*y2 += CHANNEL_HEIGHT;
    }
    if (*y2 - *y1 != yL)
    {
	/*	if (debug_level != 0)  */
	fprintf(stderr, "Adjusting Bounding Box top from (%d,%d) to (%d,%d)\n", 
		*x2, *y2, *x2, yL - yP);
	*y2 = yL - *y1 + CHANNEL_HEIGHT;
    }

/* build the return list: */
    return((mlist *)append_list(rcopy_list(partitions[partition_count+2]),
				rcopy_list(partitions[partition_count+1])));    
}

/*===================================================================================*/

mlist *make_room_for_systerm_placement(systermNets, x1, y1, x2, y2)
    nlist **systermNets;
    int *x1, *y1, *x2, *y2;
{
    /* Use the old systerm placement routine, so as to allow enough room for the 
       system terminals.  The main purpose is to set xfloor, yfloor, etc. for
       partition 0;  Also assigns X columns for systerm placement.
       */

    mlist *ml, *systerms = NULL;
    systerms = (mlist *)append_list(rcopy_list(partitions[partition_count+2]),
				    rcopy_list(partitions[partition_count+1]));
    expand_bounding_box_for_systerms(x1, y1, x2, y2);
    for (ml = systerms; ml != NULL; ml = ml->next)
    {
	push(ml->this->terms->this->nt, systermNets);
    }
    return(systerms);
}

/*===================================================================================*/

expand_bounding_box_for_systerms(x1, y1, x2, y2)
    int *x1, *y1, *x2, *y2;
{
    /* Determine the new bounding box llhc and urhc: */
    mlist *ml;
    int bestX, max_wid = 0, max_height = 0;
    int left = partition_count + 2, right = partition_count + 1;
    int noInputs, noOutputs, maxCount;

    noInputs = list_length(partitions[left]);
    noOutputs = list_length(partitions[right]);
    maxCount = MAX(noInputs, noOutputs);
    
    *y1 = yfloor - CHANNEL_HEIGHT; /* - (SYSTERM_SIZE) * maxCount; */
    *y2 = *y1 + y_sizes[0] + CHANNEL_HEIGHT; 
/*          2 * (CHANNEL_HEIGHT + (SYSTERM_SIZE) * maxCount); */

    *x1 = xfloor - CHANNEL_LENGTH - noInputs - SYSTERM_SIZE;
    *x2 = *x1 + x_sizes[0] + 2 * (CHANNEL_LENGTH + noInputs + SYSTERM_SIZE); 
}

/*===================================================================================*/

/*---------------------------------------------------------------
 * gross_systerm_placement (Level 1)
 * Put all the partitions into one bounding box in partition 0, 
 * based on picking the largest number of connections off the diagonal 
 * of the connection matrix; (which has been built to match the 
 * partitions, seen as clusters)
 *---------------------------------------------------------------
 */
mlist *gross_systerm_placement(x1, y1, x2, y2)
    int *x1, *y1, *x2, *y2;
{
/* Now there are three partitions:  0, pc+1, & pc+2, which contain the (now placed)
 * internal modules, the output only system terminals, and the remaining systerms.
 * These remaining two sets of unplaced modules are placed using the information found by
 * examining the placement of the internal modules.  NOTE:  The systerms have been 
 * 'placed' by the boxing routines, so the information within the mod structures must 
 * be ignored.
 */

/* Reset the placement information within the module structures: 
   This is the OLD placement algorithm.  It is not good, but it will complete, and serve 
   well as a space-holder function.  These results will need to be overwritten later.
*/

    /* place the systerm modules within their partitions: */
    old_place_systerm_partitions(partition_count + 2, xfloor);     /* input terminals */
    old_place_systerm_partitions(partition_count + 1, x_sizes[0]); /* output terminals */

    /* cleanup */
    if((xfloor < 0)||(yfloor < 0))
    {
	reset_borders(xfloor,yfloor);
	
	/* put the lower left corner of the box back at 0,0 */
	fixup_partition(0, xfloor, yfloor);	
    }

/* build the return list: */
    return((mlist *)append_list(rcopy_list(partitions[partition_count+2]),
				rcopy_list(partitions[partition_count+1])));    
}
/*===================================================================================*/
/*---------------------------------------------------------------
 * old_place_systerm_partitions(part, x_pos)	(Level 2)
 * place the modules in p1, given that all those in <p2> have already been 
 * put down.  TOO MESSY TO DESCRIBE!
 *---------------------------------------------------------------
 */
old_place_systerm_partitions(part, x_pos)
    int part, x_pos;
{
    mlist *ml, *mml;
    int ysum, count, y_pos, y_max = 0, x_max = 0;
    int temp, max_wid = 0, max_height = 0, placement_ok = FALSE;
    int left_side = partition_count + 2, right_side = partition_count + 1;
    tlist *tl, *ttl;

    int systermX, systermY, lineupX, placement_set, bestX;
    
    for (ml = partitions[part]; ml != NULL; ml = ml->next)
    {
	/* Reset the terminal information within the net structures: */
	add_terms_to_nets(ml->this);

	/* walk through the list of modules in this partition, and find their ideal 
	   vertical location wrt partition 0, the set of already-placed partitions.  */
	
	bestX = x_sizes[0] - xfloor;   /* used to choose the best term to line up on */
	placement_set = FALSE;
	max_wid = MAX(ml->this->x_size, max_wid);
	max_height = MAX(ml->this->y_size, max_height);

	for (tl = ml->this->terms;  tl != NULL; tl = tl->next) 
	                                                /* typically there's only one */
 	{
/* Average the position amongst the terminal positions on the net */
	    count = ysum = 0;
	    
	    for (ttl = tl->this->nt->terms; ttl != NULL; ttl = ttl->next)
	    {
		if ((ttl->this->mod != tl->this->mod) &&
		    (((part == left_side)  && (ttl->this->type != OUT)) ||
		     ((part == right_side) && (ttl->this->type == OUT))))
		{
		    ysum += ttl->this->mod->y_pos + ttl->this->y_pos;
		    count += 1;
		    /* Try lining up on the first source (desination) terminal*/;
		    y_pos = ttl->this->mod->y_pos + ttl->this->y_pos - tl->this->y_pos;
		    
		    /* save the distance to this module, as we want the closest one.*/
		    lineupX = ttl->this->mod->x_pos;
		    
		    /* save it's position (if it's ok, we'll keep it) */
		    systermY = y_pos;			      /* (int)(ysum / count); */
		    systermX = (part == right_side) ? 
		        x_pos + (WHITE_SPACE * count_terms_part(part, LEFT)) : 
			x_pos - ml->this->x_size - 
			    (WHITE_SPACE * count_terms_part(part, RIGHT));
		    
		    /* Look for conflicts amongst the already-placed systerm modules: */
		    for (mml = partitions[part]; (mml != NULL);)
		    {
			if (((mml->this != ml->this) && (mml->this->placed == PLACED))
			    &&
			    (!((mml->this->y_pos >= 
				systermY + ml->this->y_size) ||
			       (mml->this->y_pos + mml->this->y_size <= 
			        systermY))))
			{
			    /* there's a problem, so fix it. */
			    placement_ok = FALSE;
			    break;
			}
			else 
			{
			    placement_ok = TRUE;   /* keep checking */
			    mml = mml->next;
			}
		    }
		    if (placement_ok == TRUE)  /* got a good one, so save whilst ahead */
		    {
			if(abs(lineupX - x_pos) <= bestX)
			{
			    bestX = abs(lineupX - x_pos);
			    y_max = MAX(y_max, systermY);
			    x_max = MAX(x_max, systermX);
			    ml->this->x_pos = systermX;
			    ml->this->y_pos = systermY;		
			    placement_set = TRUE;

			    /* Create the fuzzy (delayed-evaluation) position... 
			       (not quite right) */
			    ml->this->fx->item1 = create_int_varStruct(&ml->this->x_pos);
			    ml->this->fy->item1 = create_int_varStruct(&ml->this->y_pos);
			    
			    /*   now keep looking (see if there is a closer one)   */
			}
		    }
		}
	    }
	}

	if (placement_set == FALSE) 
	{
	    ml->this->x_pos = systermX;
	    ml->this->y_pos = systermY;		

	    /* Create the fuzzy (delayed-evaluation) position...  (not quite right) */
	    ml->this->fx->item1 = create_int_varStruct(&ml->this->x_pos);
	    ml->this->fy->item1 = create_int_varStruct(&ml->this->y_pos);
			    
       	/*     The module in question <ml->this> has now been placed in a (hopefully) 
	       legal position amongst its brother systerms.  This should be a lineup on
	       the closest terminal that will yield a known legal position.  This may 
	       not have worked, so the following code is to insure that a legal position
	       is found: */

	    /* Check again for conflicts amongst the already-placed systerm modules: */
	    for (mml = partitions[part]; (mml != NULL);)
	    {
		if (((mml->this != ml->this) && (mml->this->placed == PLACED))
		    &&
		    (!((mml->this->y_pos > 
			ml->this->y_pos + ml->this->y_size) ||
		       (mml->this->y_pos + mml->this->y_size <
			ml->this->y_pos))))
		{
		    /* there's a problem, so fix it. (brutal) */
		    ml->this->y_pos = CHANNEL_HEIGHT + mml->this->y_size +	
		    MAX(mml->this->y_pos, ml->this->y_pos);
		    
		    mml = partitions[part]; /* restart the loop, to see if this worked.*/
		}
		else mml = mml->next;
	    }
	}
	
	/* move the partition to partition 0 */
	partitions[0] = (mlist *)concat_list(ml->this, partitions[0]);
	
	/* and mark it as placed */
	ml->this->placed = PLACED;

	/* Readjust the bounding box: */
	if (y_sizes[0] <= ml->this->y_pos + ml->this->y_size)
	{
	    y_sizes[0] = ml->this->y_pos + ml->this->y_size + 
	                 CHANNEL_HEIGHT + max_height;
	}
	if (x_sizes[0] <= ml->this->x_pos + ml->this->x_size)
	{
	   x_sizes[0] = ml->this->x_pos + ml->this->x_size + 
	                (WHITE_SPACE * count_terms_part(part, LEFT)) + max_wid; 
        }
	if (yfloor >= ml->this->y_pos)
	{
	    yfloor = ml->this->y_pos - ((WHITE_SPACE + CHANNEL_HEIGHT) + max_height);
	}
	if (xfloor >= ml->this->x_pos)
	{
	    xfloor = ml->this->x_pos - (WHITE_SPACE * count_terms_part(part, RIGHT)) - 
	             max_wid - CHANNEL_LENGTH;
	}
    }
}		
/*===================================================================================*/
/* end of file "terminals.c" */
