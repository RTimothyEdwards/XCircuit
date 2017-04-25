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
 *	file global_route.c
 * As part of MS Thesis Work, S T Frezza, UPitt Engineering
 * February, 1990 - Major revision July, 1990 - Incremental GR January, 1991
 *
 * This file contains the global and local routing functions associated with the
 * SPAR schematics placer.  It is based on the data-structures created by the 
 * parser (parse.c) and the placer (place.c), and is thus dependent upon:	 */

#include <stdio.h>
#include "route.h"
#include "psfigs.h"

/* It is expected that all modules have been placed.  
 *
 * The routing algorithm breaks the process into two stages, global and then local 
 * routing.  To arrange for the channels used these two schemes, the modules are 
 * indidvidually inserted into a tile space (see cstitch.c) and the set of empty tile 
 * constitutes the available routing area.
 *
 * Global Routing:
 * This process determines the tiles through which each net will pass.  It is an 
 * expansion process, where the nodes that are expanded into are edges of tiles.  The 
 * general gist is to do a first route, and then apply rip-up-&-reroute techniques to 
 * the most expensive of the routes, using cost-based expansion techniques. (Kern-like 
 * expansions).
 *   This is done such that every net is ripped up at least once.  Nets are selected for 
 * rip-up if they happen to be in using the most congested asg_arc on along a net that is 
 * scheduled for rip-up.
 *
 * NEW ADDITIONS:
 * The global router will be a modified Korn [DAC '82]algorithm, somewhat similar to 
 * that used in the PI Place and Route system from MIT.  The similarity is in the 
 * creation of routing channels (via the use of two parallel, 90 deg rotated corner-
 * stitching structures).  The differences lie in the handling of multi-terminal nets.  
 * These will be managed by drawing connecting lines to all points on the net, and 
 * collecting the set of cells through which they pass.  All cells not on this list 
 * will be be given a higher cost,  so as to speed the router up.  These lines will 
 * extend from the target tiles that lay behind each of the terminals in question.
 *
 * Local Routing: see "local_route.c"
 *
 */


/* ---- Global Variables -------------------------------------------------------------*/

char net_under_study[32] = "not in use";
char dfname[40] = "temp";

net *Net = NULL;	/* Used as a temporary global */

FILE *df, *vf, *hf;     /* Where to dump the global route trace/debug info */

int currentPage;	/* The current page on which insertions are being made */

int (*set_congestion_value)();		 /* The congestion function being used */


/* --- forward reference -------------------------------------------------------------:*/
void write_local_route_to_file();
expnlist *list_completed_subnetsA();
void prepare_tile_spaces();
void delete_trail();
void replace_trail();
trail *copy_trail();
void add_trail_copy();
trailist *remove_trailA();
trailist *remove_trailE();
tile *initial_move();
tile *locate_first_tile();

/*-------------------------------------------------------------------------------------
 * Setup stuff - This creates the tile/arc data structures, and the expansion 
 *               elements for a given placement.
 *-------------------------------------------------------------------------------------
 */

/*-------------------------------------------------------------------------------------*/

void do_nothing()
{
    ;
}

/*-------------------------------------------------------------------------------------*/

int contents_equivalentp_for_arcs(t1, t2)
    tile *t1, *t2;
{
    asg_arc *a1 = (asg_arc *)t1->this, *a2 = (asg_arc *)t2->this;
    if (a1->mod == a2->mod) return(TRUE);
    else return(FALSE);
}

/*-------------------------------------------------------------------------------------*/
/* return TRUE iff tile t is used. */
/*-------------------------------------------------------------------------------------*/

int solidp_for_arcs(t)
    tile *t;
{
    asg_arc *a = (asg_arc *)t->this;
    
    return((a->mod != NULL) ? TRUE : FALSE);
}

/*-------------------------------------------------------------------------------------*/

int emptyp_for_arcs(t)
    tile *t;
{
    asg_arc *a = (asg_arc *)t->this;
    
    return((a->mod != NULL) ? FALSE : TRUE);
}

/*-------------------------------------------------------------------------------------*/

asg_arc *create_arc(t, page, modFlag)
    tile *t;
    int page, modFlag;
{    
    /* create a new arc structure, initialize it, and return it. */
    asg_arc *tarc;
    int i;
    
    /* Modifies the tile space by replacing the contents field with an arc struct. */
    tarc = (asg_arc *)calloc(1, sizeof(asg_arc));
    tarc->congestion = 0;
    tarc->page = page;
    tarc->corners = NULL;
    tarc->nets = NULL;			/* Where the 'whos used me' information goes */
    tarc->t = t;			/* Store the tile pointer ( back pointer ) */
    /* if this is a blank tile being transformed, save the module pointer. */
    tarc->mod = (modFlag == TRUE) ? (module *)t->this : NULL;	
    tarc->vcap = (page == VERT) ? t->x_len : t->y_len;
    tarc->hcap = (page == HORZ) ? t->x_len : t->y_len;
    tarc->local = tarc->count = 0;

    for (i=0; i<HASH_SIZE; i++) tarc->trails[i] = NULL;

    t->this = (int *)tarc;      	/* Replace the tile contents with the arc str. */
    
    return (tarc);
}    

/*------------------------------------------------------------------------------------*/

delete_arc(a)
    asg_arc *a;
{
    /* Delete the contents and allocation for this arc: */
    a->nets = (nlist *)free_list(a->nets);
    a->corners = (crnlist *)free_list(a->corners);
    kill_hashA(a);
    my_free(a);
}

/*------------------------------------------------------------------------------------*/

assign_tile_dimensions(m, xPos, yPos, xLen, yLen)
    module *m;
    int *xPos, *yPos, *xLen, *yLen;
{
    switch(validate(m->type))
    {
	case BUFFER : 
	case NOT_  : 
	case AND  : 
	case NAND : 
	case OR   : 
	case NOR  : 
	case XOR  : 
	case XNOR : *xPos = m->x_pos - TERM_MARGIN;
	            *yPos = m->y_pos;
	            *xLen = m->x_size + 2 * TERM_MARGIN;
		    *yLen = m->y_size;
	            break;
	case INPUT_SYM : *xPos = m->x_pos;
	                 *yPos = m->y_pos;
	                 *xLen = m->x_size + TERM_MARGIN;
		         *yLen = m->y_size;
	                 break;
	case OUTPUT_SYM : *xPos = m->x_pos - TERM_MARGIN;
	                  *yPos = m->y_pos;
	                  *xLen = m->x_size;
		          *yLen = m->y_size;
	                  break;
        case BLOCK : 
        default : 
                     *xPos = m->x_pos - TERM_MARGIN;
                     *yPos = m->y_pos - TERM_MARGIN;
                     *xLen = m->x_size + 2 * TERM_MARGIN;
                     *yLen = m->y_size + 2 * TERM_MARGIN;
	  	     break;
   }
}

/*------------------------------------------------------------------------------------*/

insert_all_modules(hroot, vroot, ml)
    tile *hroot, *vroot;
    mlist *ml;
{
    /* This function inserts each listed tile into the tile space containing <hroot>. */
    /* The tile is also inserted into the vertically-oriented space by reversing the 
       x,y ordiering */
    
    mlist *mll;
    tile *t;
    int xPos, yPos, xLen, yLen;

    /* Reset the content-manging stuff for this round of tile dealings: */
    solidp = std_solidp;
    contents_equivalentp = std_contents_equivalentp;
    insert_contents = std_insert_contents;
    manage_contents__hmerge = manage_contents__vmerge = null;
    manage_contents__hsplit= manage_contents__vsplit = null;
    manage_contents__dead_tile = null;

    for (mll = ml; mll != NULL; mll = mll->next)
    {
	mll->this->htile = mll->this->vtile = NULL; 	/* Zero out the tile pointers */

	assign_tile_dimensions(mll->this, &xPos, &yPos, &xLen, &yLen);

	/* Include a buffer around the modules for the terminals */
	t = insert_tile(hroot, xPos, yPos, xLen, yLen, mll->this);
	mll->this->htile = t;			/* save the horiz tile pointer 
						   for cross-reference */
	if (t == NULL)
	{
	    fprintf(stderr, "ERROR: %s module %s did not insert into horz tilespace\n",
		    mll->this->type, mll->this->name, mll->this);
	    exit(-1);
	}

	t = insert_tile(vroot, yPos, xPos, yLen, xLen, mll->this);
	mll->this->vtile = t;			/* save the vert tile pointer for 
						   cross-reference */
	if (t == NULL)
	{
	    fprintf(stderr, "ERROR: %s module %s did not insert into vert tilespace\n",
		    mll->this->type, mll->this->name, mll->this);
	    exit(-1);
	}

	mll->this->placed = FALSE;	/* Reuse of flag, now used for printing */
    }
} 	/* No error checking */

/*------------------------------------------------------------------------------------*/

distribute_corner_list(owner, partner)
    asg_arc *owner, *partner;	/* <owner> contains <corners>, <partner>s ->corners
				   should be NULL. */
{
    /* Redistribute the corners contained in <corners> amongst the two tiles such that
       the range limitations of the various points are observed.  Correct the
       tile/arc pointers within the corner structures themselves.  */

    crnlist *cl, *corners = (crnlist *)rcopy_list(owner->corners);

    for (cl = corners; cl != NULL; cl = cl->next)
    {
	if (corner_falls_in_arc_p(cl->this, owner) == FALSE)
	{
	    pull_corner_from_arc(cl->this, owner);
	}
	if ((partner != NULL) && (corner_falls_in_arc_p(cl->this, partner) == TRUE))
	{
	    add_corner_to_arc(cl->this, partner);
	}
    }
    free_list(corners);
}

/*------------------------------------------------------------------------------------*/

consolodate_corner_list(taker, giver)
    asg_arc *taker, *giver;	
{
    /* Redistribute the corners contained in <giver> into <taker>.
       The range limitations of the various points are observed.  Correct the
       tile/arc pointers within the corner structures themselves.  */

    crnlist *cl;

    for (cl = giver->corners; cl != NULL; cl = cl->next)
    {
	if (corner_falls_in_arc_p(cl->this, taker) == TRUE)
	{
	    add_corner_to_arc(cl->this, taker);
	}
	pull_corner_from_arc(cl->this, taker);
    }
}

/*------------------------------------------------------------------------------------*/

int definite_overlap_p(a1, a2, b1, b2)
    int a1, a2, b1, b2;
{
    if ((a1 < b2) && (a2 > b1)) return(TRUE);	
    else return(FALSE);
}

/*------------------------------------------------------------------------------------*/

int tile_contacts_tile_p(t1, t2)
    tile *t1, *t2;
{
    /* Returns TRUE iff <t1> intersects <t2>.  It is assumed that the two tiles
       are from opposite page orientations. */

    if ((definite_overlap_p(t1->x_pos, t1->x_pos + t1->x_len, 
			t2->y_pos, t2->y_pos + t2->y_len) == TRUE) &&
	(definite_overlap_p(t2->x_pos, t2->x_pos + t2->x_len, 
			t1->y_pos, t1->y_pos + t1->y_len) == TRUE))
    {
	return(TRUE);
    }
    else return(FALSE);
}

/*------------------------------------------------------------------------------------*/

int valid_trail_p(tr)
    trail *tr;
{
    /* Check the given trail <tr> for consitency. */
    tile *termTile;
    tilist *tl;
    int tx, ty;
    term *t;

    for (tl = tr->tr; tl != NULL; tl = tl->next)
    {
	if ((tl == tr->tr) &&		 /* First tile on list && */ 
	    (tr->jEx != NULL))		 /* It's a completed trail */
	{ 
	    termTile = initial_move(tr->jEx, HORZ);
	    if (termTile != tl->this) 
	    {
		return (FALSE);
	    }
	}

	if (tl->next != NULL)	
	{
	    if (tile_contacts_tile_p(tl->this, tl->next->this) == FALSE) 
	    {
		return(FALSE);
	    }
	}
	else 	/* At the beginning of the trail, so check to see that it touches 
		   the terminial listed for tr->ex->t */
	{ 
	    termTile = initial_move(tr->ex, HORZ);
	    if (termTile != tl->this) 
	    {
		return (FALSE);
	    }
	}
    }
    return(TRUE);
}

/*------------------------------------------------------------------------------------*/

int check_trail_p(tr, org, rep)
    trail *tr;
    tile *org, *rep;
{
    /* Check the given trail <tr> for the tile <org>.  For the instance of <org>,
       replace it with <rep> and check to see that the trail is still valid, that
       is that the (two) neighbor(s) of <org> can legally reach <rep>. */
    /* This simply insures that the trail works up to the point where the 
       replacement occurs. */

    tile *termTile;
    tilist *tl;
    term *t;
    int tx, ty, foundIt = FALSE, damnThingFits = 0;

    if (tr->tr->this == org) damnThingFits = 1;      /* <org> is at head of list */

    for (tl = tr->tr; tl != NULL; tl = tl->next)
    {   /* This first check is a validation check */
	if ((tl == tr->tr) &&		 /* First tile on list && */ 
	    (tr->jEx != NULL))		 /* It's a completed trail */
	{ 
	    termTile = initial_move(tr->jEx, HORZ);
	    if (((tl->this == org) && (rep != termTile)) || 
		((tl->this != org) && (termTile != tl->this)))
	    {
		return (FALSE);
	    }
	}

	if (tl->next != NULL)	
	{
	    if ((tl->next->this == org) && 
		(tile_contacts_tile_p(tl->this, rep) == TRUE)) damnThingFits +=1;
	    else if (tl->this == org)
	    {
		foundIt = TRUE;
		if (tile_contacts_tile_p(tl->next->this, rep) == TRUE) 
		    damnThingFits += 1;
	    }
	    else if (tile_contacts_tile_p(tl->this, tl->next->this) == FALSE) 
	    {
		return(FALSE);
	    }
	}
	else 
	{ 
	    termTile = initial_move(tr->ex, HORZ);
	    if (tl->this == org) 
	    { 	/* <org> is at tail end of list */
		foundIt = TRUE;
		if (termTile == rep) damnThingFits += 1;
	    }
	    else if (termTile != tl->this) 
	    {
		return (FALSE);
	    }
	}
    }
    if (foundIt != TRUE)
    {
	fprintf(stderr, "Tile 0x%x got lost (wrt trail 0x%x)\n", org, tr);
    }
    return (((foundIt == TRUE) && (damnThingFits >= 2)) ? TRUE : FALSE);
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

trail *enumerate_trail(pTrail, org, rep)
    trail *pTrail;
    tile *org, *rep;
{
    /* Replicate the given trail with the <org> tile replaced with the <rep> tile.
       Do no error checking or costing */
    trail *t = copy_trail(pTrail);
    repl_item(org, rep, t->tr);
    return(t);
}

/*------------------------------------------------------------------------------------*/

void manage_trails_for_merge(amoeba, food)
    asg_arc *amoeba, *food;
{
    /* This is to merge/validate/correct the trails collected in the <amoeba> arc;
       The appropriate trails are swiped from the <food> arc.  The 'food' arc 
       is about to be swallowed up... */

    /* Also builds the ->nets list for the <amoeba> arc, deleting that for the <food>
       arc.  The congestion values are appropriately set.*/
    int i;
    trail *t;
    trailist *trl, *temp;
    
    if (amoeba == NULL) 
    {
	if (food != NULL)
	{
	    manage_trails_for_merge(food, amoeba);
	    return;
	}
	else return;				/* Bad call... */
    }
    else if (food == NULL) return;		/* Trivial Case */    

    amoeba->nets = (nlist *)free_list(amoeba->nets);

    /* Walk through the table in <food>, copying whatever is of use, and destroying 
       the rest.  When completed, <food>'s hash table should get nuked. */    
    for (i=0; i<HASH_SIZE; i++) 
    {
	temp = (trailist *)rcopy_ilist(food->trails[i]);
	for (trl = temp; trl != NULL; trl = trl->next)
	{
	    if (check_trail_p(trl->this, food->t, amoeba->t) == TRUE)
	    {
		repl_item(food->t, amoeba->t, t->tr);
		insert_trailA(trl->this, food);
		pushnew(t->ex->n, &food->nets);
	    }
	    remove_trailA(trl->this, food);
	}
	temp = (trailist *)free_ilist(temp);		
	food->trails[i] = (trailist *)free_ilist(food->trails[i]);
    }
}

/*------------------------------------------------------------------------------------*/

trlist *manage_trails_for_split(parent, sibling, pFlag)
    asg_arc *parent, *sibling;
    int pFlag;			/* If TRUE, then modify <parent> */
{
    /* This is to validate/correct the trails collected in the <parent> arc;
       The appropriate trails are then recreated for the <sibling> arc. */

    /* Also rebuilds the ->nets list for each of the arcs, and resets the congestion
       value. */

    /* The <pFlag> acts as a delayer;  The clean-up for the <parent> arc is only 
       evaluated if the flag is set. */
    int i;
    tile *p, *s;
    trail *t;
    trailist *temp, *trash, *trl;
    trlist *valid = NULL;
    
    if (parent == NULL) 
    {
	if (sibling != NULL)
	{
	    valid = manage_trails_for_split(sibling, parent, pFlag);
	    return(valid);
	}
	else return;				/* Bad call... */
    }
    
    p = parent->t;
    parent->nets = (nlist *)free_list(parent->nets);

    if ((sibling == NULL) && (pFlag == TRUE))
    {
	/* Any trail that connects <p> validly (ie, now that p has been changed)
	   is still valid, and should remain in the table to be rehashed.
	   Any trail that does not connect should be nuked. 

	   This is a selective rehash of the arc.  */
	for (i=0; i<HASH_SIZE; i++) 
	{
	    temp = (trailist *)rcopy_ilist(parent->trails[i]);
	    
	    for (trl = temp; trl != NULL; trl = trl->next)
	    {
		if (check_trail_p(trl->this, p, p) == TRUE)
		{
		    t = trl->this;
		    t->cost = trail_cost(t);
		    replace_trail(t, t);
		    pushnew(t->ex->n, &parent->nets);
		    if (valid_trail_p(t) == TRUE) push(t, &valid);;
		}
		else if (pFlag == TRUE)
		{ 
		    delete_trail(trl->this);
		}
	    }
	    temp = (trailist *)free_ilist(temp);		
	}	    
    }
    else
    {
	/* Any trail that connects <s> validly (when <p> usage is replaced with <s>)
	   is still valid, and should be added to <sibling>'s table.
	   Any trail that does not connect should be ignored. */

	s = sibling->t;
	sibling->nets = (nlist *)free_list(sibling->nets);

	for (i=0; i<HASH_SIZE; i++) 
	{
	    temp = (trailist *)rcopy_ilist(parent->trails[i]);

	    for (trl = temp; trl != NULL; trl = trl->next)
	    {
		if ((check_trail_p(trl->this, p, p) == TRUE) &&
		    (pFlag == TRUE))
		{
		    t = trl->this;
		    t->cost = trail_cost(t);
		    replace_trail(t, t);
		    pushnew(t->ex->n, &parent->nets);		    
		    if (valid_trail_p(t) == TRUE) push(t, &valid);;

		    if (check_trail_p(trl->this, p, s) == TRUE)
		    {
			t = enumerate_trail(trl->this, p, s);
			t->cost = trail_cost(t);
			add_trail_copy(t);
			if (valid_trail_p(t) == TRUE) push(t, &valid);;
		    }
		}
		else 	/* Blow the trail away.  It is now been proven invalid. */
		{
		    if (check_trail_p(trl->this, p, s) == TRUE)
		    {
			t = enumerate_trail(trl->this, p, s);
			t->cost = trail_cost(t); /* can't break ties if <trl->this> is */
			if (pFlag == TRUE) delete_trail(trl->this); 
			add_trail_copy(t);	 /* addition */
			if (valid_trail_p(t) == TRUE) push(t, &valid);;
		    }
		    else if (pFlag == TRUE)
		    { 
			delete_trail(trl->this);
		    }
		}
	    }
	    temp = (trailist *)free_ilist(temp);		
	}
    }
    return(valid);
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

void manage_arc_during_hmerge(left, right)
    tile *left, *right;
{
    /* <left> will remain, while <right> is destroyed.  All pertinent info from <right>
       needs to be copied into <left>.
     */
    asg_arc *lArc, *rArc;

    if (left == NULL)
    {
	fprintf(stderr, "manage_arc_during_hmerge called with NULL arg...\n");
	return;			/* probably very bad... */
    }
    else if (right != NULL)
    {
	return;	/* Problem ??  Hard to say.  */
    }
    if (solidp_for_arcs(left) == TRUE)
    {
	fprintf(stderr, "problem in manage_arc_during_hmerge\n");
	lArc->mod = NULL;
    }
    
    lArc = (asg_arc *)left->this;
    rArc = (asg_arc *)right->this;

    lArc->hcap = (currentPage == HORZ) ? left->x_len : left->y_len;
    lArc->vcap = (currentPage == VERT) ? left->x_len : left->y_len;
    lArc->local += rArc->local;
    lArc->count += rArc->count;

    consolodate_corner_list(lArc, rArc);
    manage_trails_for_merge(lArc, rArc);
}

/*------------------------------------------------------------------------------------*/

void manage_arc_during_vmerge(bottom, top)
    tile *bottom, *top;
{
    asg_arc *bArc, *tArc;

    if (bottom == NULL)
    {
	fprintf(stderr, "manage_arc_during_hmerge called with NULL arg...\n");
	return;				/* probably very bad... */
    }
    else if (top != NULL)
    {
	return;				/* Problem ??  Hard to say.  */
    }
    if (solidp_for_arcs(bottom) == TRUE)
    {
	fprintf(stderr, "problem in manage_arc_during_vmerge\n");
	bArc->mod = NULL;
    }

    bArc = (asg_arc *)bottom->this;
    tArc = (asg_arc *)top->this;
    
    bArc->hcap = (currentPage == HORZ) ? bottom->x_len : bottom->y_len;
    bArc->vcap = (currentPage == VERT) ? bottom->x_len : bottom->y_len;
    bArc->local += tArc->local;
    bArc->count += tArc->count;

    consolodate_corner_list(bArc, tArc);
    manage_trails_for_merge(bArc, tArc);
}

/*------------------------------------------------------------------------------------*/

void manage_arc_during_hsplit(left, right)
    tile *left, *right;
{
    asg_arc *a, *cArc, *pArc;
    trail *tr;
    trlist *collectedTrails = NULL;

    /* Create the arc for the <child> arc and tie it in: */
    pArc = (asg_arc *)left->this;	
    cArc = create_arc(right, currentPage, FALSE);
    pArc->hcap = (currentPage == HORZ) ? left->x_len : left->y_len;
    pArc->vcap = (currentPage == VERT) ? left->x_len : left->y_len;
    cArc->local = pArc->local;
    cArc->count = pArc->count;
    
    distribute_corner_list(pArc, cArc);
    collectedTrails = manage_trails_for_split(pArc, cArc, TRUE);

    if ((debug_level == 4) && (collectedTrails != NULL))
    {
	fprintf(stderr, "\narc for tiles [%x]c ", right);
	dump_tile(right);
	fprintf(stderr, " & [%x]p ", left);
	dump_tile(left);
	fprintf(stderr," contains %d trails: \n", list_length(collectedTrails));
	dump_trails(collectedTrails);
    }
}

/*------------------------------------------------------------------------------------*/

void manage_arc_during_vsplit(top, bot)
    tile *top, *bot;
{
    asg_arc *a, *cArc, *pArc;
    trail *tr;
    trlist *collectedTrails = NULL;

    /* Create the arc for the <child> arc and tie it in: */
    pArc = (asg_arc *)top->this;	
    cArc = create_arc(bot, currentPage, FALSE);
    pArc->hcap = (currentPage == HORZ) ? top->x_len : top->y_len;
    pArc->vcap = (currentPage == VERT) ? top->x_len : top->y_len;
    cArc->local = pArc->local;
    cArc->count = pArc->count;
    
    distribute_corner_list(pArc, cArc);
    collectedTrails = manage_trails_for_split(pArc, cArc, TRUE);

    if ((debug_level == 4) && (collectedTrails != NULL))
    {
	fprintf(stderr, "\nasg_arc for tiles [%x] ", bot);
	dump_tile(bot);
	fprintf(stderr, " & [%x] ", top);
	dump_tile(top);
	fprintf(stderr," contains %d trails: \n", list_length(collectedTrails));
	dump_trails(collectedTrails);
    }
}

/*------------------------------------------------------------------------------------*/

void manage_arcs_for_modified_tiles(tilePairList)
    list *tilePairList;
{
    /* Given a list of tilists in the form (child, parent), correct the arcs in 
       each.  This essentially means checking all trails contained in each of the
       <parent> tiles, and then checking each trail for all combinations of variations.
       this is a lot of checking, but typically not a lot of work.  Typical trails
       will have only one affected tile, and the rare trail will have two. 

       given the lists (a,b), (c,d),(e,a), (f,d), the following arrangement should
       be made:  (in order!!) 
           All occurances of <b> map into two lists, ...a... and ...b...
	   All occurances of <d> map into two lists, ...d... and ...c...
	   All occurances of <a> map into two lists, ...a... and ...e...
	   All occurances of <d> map into two lists, ...d... and ...f...
       Note that this implies that the <d>'s actually form many more combinations.    */

    list *l;
    tile *parent, *child;
    tilist *tl, *pr, *tilesTouched = NULL;
    asg_arc *a, *cArc, *pArc, *oldArc;
    trail *tr;
    trlist *trl, *enumTrails = NULL, *collectedTrails = NULL;
    trlist *remove_all_trails_from_arc();

    /* Basic Method: 
       (1) Collect all the trails from the parent arc (Remove them from the arcs)...
       (1a)Create arcs for the child tiles.
       (2) Then enumerate the trails, as per the <tilePairList> ordering...
       (3) Given this (enumerated) list of trails, validate each trail...
       (4) reinsert each trail into the arcspace...
       (5) Clean up list pointers.
       */
    
    /* Generate the enuimerated trlist:  (Items (1) & (2) */
    for(l = tilePairList; l != NULL; l = l->next)
    {
	pr = (tilist *)l->this;
	parent = pr->next->this;	child = pr->this;

	/* Create the arc for the <child> arc and tie it in: */
	pArc = (asg_arc *)parent->this;	
	oldArc = (asg_arc *)child->this;
	cArc = create_arc(child, currentPage, FALSE);
	cArc->local = oldArc->local;
	cArc->count = oldArc->count;
	distribute_corner_list(oldArc, cArc);
	distribute_corner_list(pArc, cArc);

	collectedTrails = remove_all_trails_from_arc(parent->this);

	/* Comb through the trails that have already been enumerated; <parent> may
	   or may not be a part of these trails */
	for (trl = enumTrails; trl != NULL; trl = trl->next)
	{
	    if (member_p(parent, trl->this->tr, identity) == TRUE)
	    {
		tr = enumerate_trail(trl->this, parent, child);
		push(tr, &enumTrails);
	    }
	}	    
	    
	/* enumerate all of the <collectedTrails>, as <parent> is a member of each of 
	   these. */
	for (trl = collectedTrails; trl != NULL; trl = trl->next)
	{
	    tr = enumerate_trail(trl->this, parent, child);
	    push(tr, &enumTrails);
	}

	/* Combine to form the complete enumeration list. */
	enumTrails = (trlist *)append_list(enumTrails, collectedTrails);	

	pushnew(parent, &tilesTouched);		pushnew(child, &tilesTouched);
    }

    /* Validate these trails, deleting the bad ones, and inserting the
       good ones. (Items 3 & 4)  */
    for (trl = enumTrails; trl != NULL; trl = trl->next)
    {
	tr = trl->this;
	if (valid_trail_p(tr) == TRUE)
	{
	    tr->cost = trail_cost(tr);
	    add_trail_copy(tr);
	}
	else 
	{
	    delete_trail(tr);
	}
    }
    
    /* finally, (5) Cleanup the list pointers... */
    free_list(enumTrails);	
    free_list(tilesTouched);	
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

void manage_arc_for_new_tile(new)
    tile *new;
{	/* This tile was just created... */
    asg_arc *q = create_arc(new, currentPage, TRUE);
}

/*------------------------------------------------------------------------------------*/

void manage_arc_for_dead_tile(old)
    tile *old;
{	/* This tile is about to go away... */
    /* Check to see that it is not something important like a root tile... */
    asg_arc *a = (asg_arc *)old->this;

    if(routingRoot[a->page][currentPage] == old) 
    {
	if (old->rt != NULL) routingRoot[a->page][currentPage] = old->rt;
	else if (old->tr != NULL) routingRoot[a->page][currentPage] = old->tr;
	else if (old->lb != NULL) routingRoot[a->page][currentPage] = old->lb;
	else if (old->bl != NULL) routingRoot[a->page][currentPage] = old->bl;
	else
	{
	    fprintf(stderr, "dying tile about to blow away rootTile[%d][%d]...\n",
		    a->page, currentPage);
	}
    }
    /* Trash everything in the arc: */
    delete_arc(a);
}

/*------------------------------------------------------------------------------------*/

void manage_arc_for_copied_tile(child, parent)
    tile *child, *parent;
{
    /* <new>->this points to the arc for the tile this was copied from, so be careful */
    asg_arc *cArc, *pArc = (asg_arc *)parent->this, *oldArc = (asg_arc *)child->this;
    tile *oldTile = oldArc->t;
    trlist *q;

    cArc = create_arc(child, currentPage, FALSE);
    cArc->local = oldArc->local;
    cArc->count = oldArc->count;
    distribute_corner_list(oldArc, cArc);
    distribute_corner_list(pArc, cArc);

    /* To handle the trails, probably need to assume that the corner stitches for
       <top> and <bottom> are correct, which is not a safe assumption, I THINK.
       This could be a very critical problem. */

	q = manage_trails_for_split(oldArc, cArc, FALSE);

    if ((debug_level == 4) && (q != NULL))
    {
	fprintf(stderr, "\narc for tiles [%x] ", child);
	dump_tile(child);
	fprintf(stderr, " & [%x] ", oldTile);
	dump_tile(oldTile);
	fprintf(stderr," contains %d trails: \n", list_length(q));
	dump_trails(q);
    }
}

/*------------------------------------------------------------------------------------*/

void manage_arc_for_clipped_tile(copy)
    tile *copy;
{
    /* This tile has been modified, and is now stable.  Correct the contents to
       reflect this change.  */
    asg_arc *qArc = (asg_arc *)copy->this;
    trlist *q;

    distribute_corner_list(qArc, NULL);
    q = manage_trails_for_split(qArc, NULL, TRUE);
    if ((debug_level == 4) && (q != NULL))
    {
	fprintf(stderr, "arc for tile [%x] ", copy);
	dump_tile(copy);
	fprintf(stderr, " contains %d trails: \n", list_length(q));
	dump_trails(q);
    }
}

/*------------------------------------------------------------------------------------*/

void insert_contents_for_arc(t, m)
    tile *t;
    module *m;
{
    /* Given the tile <t> that is part of the modified tile space, insert <m> into
       the arc structure, and return said structure;
       */
    asg_arc *a = (arc *)t->this;
    a->mod = m;
}

/*------------------------------------------------------------------------------------*/

add_modules(ml, p)	
    mlist *ml;		/* List of modules being added */
    int p;		/* the page being dealt with */
{
    /* Designed for use on modified tile spaces. */
    /* This function inserts each listed tile a->trails[index]
       into the tile space containing routingRoot[HORZ][p]. */
    /* The tile is also inserted into the vertically-oriented space by reversing 
       the x,y ordering and inserting into the space containing tile 
       routingRoot[VERT][p]. 

       These two globals must be used, as they can be changed as modifications occur
       within the tile spaces during the calls to "insert_tile." */
    
    mlist *mll;
    asg_arc * a, *temp;
    tile *t;
    int xPos, yPos, xLen, yLen;

    /* Setup all of the specialized functions for handling complex colors in 
       the tile space: */
    /* See "cstitch.h" and "cstitch.c" for the specs and usage of thes functions. */
    manage_contents__hmerge = manage_arc_during_hmerge; 
    manage_contents__vmerge = manage_arc_during_vmerge;  
    manage_contents__hsplit = manage_arc_during_hsplit; 
    manage_contents__vsplit = manage_arc_during_vsplit;  
    manage_contents__dead_tile = manage_arc_for_dead_tile; 

    solidp = solidp_for_arcs;
    contents_equivalentp = contents_equivalentp_for_arcs;
    insert_contents = insert_contents_for_arc;

    for (mll = ml; mll != NULL; mll = mll->next)
    {
	if (debug_level == 4)
	{
	    fprintf(stderr, "module %s being added to the Horizontal Page:\n", 
		    mll->this->name);
	}

	currentPage = HORZ;
	/* Include a buffer around the modules for the terminals */
	assign_tile_dimensions(mll->this, &xPos, &yPos, &xLen, &yLen);

	t = insert_tile(routingRoot[HORZ][p], xPos, yPos, xLen, yLen, mll->this);
	mll->this->htile = t;			/* save the horiz tile pointer 
						   for cross-reference */
	if (t == NULL)
	{
	    fprintf(stderr, "ERROR: %s module %s did not insert into horz tilespace\n",
		    mll->this->type, mll->this->name, mll->this);
	    exit(-1);
	}
	if (debug_level == 4)
	{
	    fprintf(stderr, "inserted @ %x...", t);
	    dump_tile(t);
	    fprintf(stderr, "\nmodule <%s> being added to the Vertical Page:\n", 
		    mll->this->name);
	}

	currentPage = VERT;
	t = insert_tile(routingRoot[VERT][p], yPos, xPos, yLen, xLen, mll->this);
	mll->this->vtile = t;			/* save the vert tile pointer for 
						   cross-reference */
	if (t == NULL)
	{
	    fprintf(stderr, "ERROR: %s module %s did not insert into vert tilespace\n",
		    mll->this->type, mll->this->name, mll->this);
	    exit(-1);
	}

	mll->this->placed = FALSE;	/* Reuse of flag, now used for printing */
	if (debug_level == 4)
	{
	    fprintf(stderr, "inserted @ %x...", t);
	    dump_tile(t);
	    fprintf(stderr,"\n");
	}
    }
} 	/* No error checking */

/*-------------------------------------------------------------------------------------*/

reform_tile_space(Tlist, page)
    tilist *Tlist;
    int page;		/* orientation of the tile-space */
{    
    /* Reform the tile space such that the tile contents point to arcs, and the 
       arc->t slots point to the original tile contents (either NULL or a Module 
       pointer). */
    
    tilist *tl;
    asg_arc *tarc;
    
    for (tl = Tlist; tl != NULL; tl = tl->next)
    {	
	/* Save the module pointer from the communist (unreformed) tile */
	tarc = create_arc(tl->this, page, TRUE);
    }
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/

int not_source_p(ex)
    expn *ex;
{
    /* Return TRUE if the given expansion is associated with a source terminal */
    if (ex->t->type == IN) return(TRUE);
    else return(FALSE);
}

/*------------------------------------------------------------------------*/

int identity(exp1, exp2)
    expn *exp1, *exp2;
{
    return ((int)exp1 == (int)exp2);
}

/*---------------------------------------------------------------------------------*/
/* TRAIL MANIPULATION FUNCTIONS - 					       	   */
/*---------------------------------------------------------------------------------*/

/*------- Manage the Hash table in the expn structure: ----------------------------*/

unsigned hashE(tr)
    trail *tr;
{
    /* Given a trail, hash your way to it */
        
    unsigned hashval = (unsigned)(((int)tr->ex + ((int)tr->ex % 3)) % HASH_SIZE);

    return (hashval);
}

/*------------------------------------------------------------------------------------*/

trailist *lookup_trailE(tr, ex)
    trail *tr;
    expn *ex;
{
    /* This function returns a pointer to the trailist structure where the given 
       trail <tr>  should be stored. */
    trailist *trl =	 ex->queue[ hashE(tr) ];

    if (find_indexed_element(tr->cost, trl) != NULL) return(trl);
    else return(NULL);
}

/*------------------------------------------------------------------------------------*/

int note_tiles_added(trailToCheck, ex)
    trail *trailToCheck;
    expn *ex;
{
    /* This is to maintain the ->seen list in <ex> */

    tile *lastTileAdded;	/* Used to check if an addition has been made... */
    tilist *tilesToCheck = trailToCheck->tr;
    tilist *tl;

    if (trailToCheck->ex == ex)
    {
	for (tl = tilesToCheck; tl != NULL; tl = tl->next)
	{
	    pushnew(tl->this, &ex->seen);
	    lastTileAdded = ex->seen->this;
	    if (tl->this != lastTileAdded) /* If this tile has been 'seen'already,     */
	    {				   /* assume the rest of the tiles on the list */
		break;			   /* have been as well. .   This works on the */
	    }				   /* assumtion that the last tile in the list */
	}				   /* is a source tile for <ex>.               */
    }
    else 				   /* Otherwise, who knows?  Check 'em all...  */
    {
	for (tl = tilesToCheck; tl != NULL; tl = tl->next) pushnew(tl->this, &ex->seen);
    }
}

/*------------------------------------------------------------------------------------*/

int insert_trailE(tr, ex)
    trail *tr;
    expn *ex;
{
    /* This installs <tr> into the hashtable in <ex> */
    int q, hashval = hashE(tr);

    if ((ex->queue[ hashval ] != NULL) && 
	(ex->queue[ hashval ]->this->cost == tr->cost))
    {	
	/* Deal with the race condition: */
	if (break_cost_tie(ex->queue[hashval]->this, tr) == TRUE) tr->cost += 1;
	else ex->queue[hashval]->this->cost += 1;
    }
    q = ordered_ilist_insert(tr, tr->cost, &ex->queue[ hashE(tr) ]);

    note_tiles_added(tr, ex);			/* Record that this tile has been seen */
    return(q);
}

/*------------------------------------------------------------------------------------*/

trailist *remove_trailE(tr, ex)
    trail *tr;
    expn *ex;
{
    /* This removes the given trail from the hash table in <ex>. */
    int index = hashE(tr);
    expn *tempEx;
    expnlist *exl, *newSibList, *oldSibList = NULL;
    trlist *trl, *newConnList;
    trail *q = (trail *)irem_item(tr, &ex->queue[index], NULL);

    /* Maintain the ->siblings and ->connections lists: */
    for (trl = ex->connections; trl != NULL; trl = trl->next)
    {
	if (trl->this == tr)
	{
	    rem_item(tr, &ex->connections);
	    rem_item(ex, &ex->siblings);
	    newSibList = ex->siblings;
	    newConnList = ex->connections;
	    push(ex, &oldSibList);

	    for (exl = newSibList; exl != NULL; exl = exl->next)
	    {
		exl->this->siblings = newSibList;     	       /* may not have changed */
		exl->this->connections = newConnList;
	    }
	    ex->siblings = oldSibList;
	}
    }
    return(ex->queue[index]);
}

/*------------------------------------------------------------------------------------*/

trail *best_trailE(ex)
    expn *ex;
{
    /* This pops the best trail off of the hash table in <ex>: */
    int i, bestIndex = FALSE; 
    int bestVal = FALSE;
    for (i=0; i<HASH_SIZE; i++) 
    {
	if ((ex->queue[i] != NULL) && 
	    ((bestVal == FALSE) || (ex->queue[i]->index <= bestVal)))
	{
	    bestIndex = i;
	    bestVal = ex->queue[i]->index;
	}
    }
    if (bestIndex == FALSE) return(NULL);
    else return((trail *)ipop(&ex->queue[bestIndex]));	
}

/*------------------------------------------------------------------------------------*/

trail *examine_best_trailE(ex)
    expn *ex;
{
    /* simply returns the pointer to the best_trail for examination */
    int i, bestIndex = FALSE; 
    int bestVal = FALSE;

    for (i=0; i<HASH_SIZE; i++) 
    {
	if ((ex->queue[i] != NULL) && 
	    ((bestVal == FALSE) || (ex->queue[i]->this->cost <= bestVal)))
	{
	    bestIndex = i;
	    bestVal = ex->queue[i]->this->cost;
	}
    }
    if (bestIndex == FALSE) return(NULL);
    else return((trail *)(ex->queue[bestIndex]->this));	
}

/*------------------------------------------------------------------------------------*/

void pull_trail_from_all_arcs(t)
    trail *t;
{
    /* Nuke the little commie: */
    tilist *tl;
    for (tl = t->tr; tl != NULL; tl = tl->next)
    {
	remove_trailA(t, (asg_arc *)tl->this->this);   /* Pull from arc hash table */
    }
    free_list(t->used);
}

/*------------------------------------------------------------------------------------*/

void delete_trail(t)
    trail *t;
{
    /* Nuke the little commie: */
    pull_trail_from_all_arcs(t);		/* Pull from arc hash tables */
    remove_trailE(t, t->ex);			/* Pull from expn hash table */
    my_free(t);
}    

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

trail *remove_trail(t)
    trail *t;
{
    /* Pull the little commie out for interrogation: */
    expn *ex = t->ex;
    tilist *tl;

    for (tl = t->tr; tl != NULL; tl = tl->next)
    {
	remove_trailA(t, (asg_arc *)tl->this->this);   /* Pull from arc hash table */
    }
    remove_trailE(t, ex);			   /* Pull from expn hash table*/

    return(t);
}    

/*------------------------------------------------------------------------------------*/

void replace_trail(tOld, tNew)
    trail *tOld, *tNew;
{
    /* Nuke the little commie: */
    expn *ex = tOld->ex;
    tilist *tl;

    for (tl = tOld->tr; tl != NULL; tl = tl->next)
    {
	remove_trailA(tOld, (asg_arc *)tl->this->this);   /* Pull from arc hash table */
	insert_trailA(tNew, (asg_arc *)tl->this->this);
    }
    remove_trailE(tOld, ex);			   /* Pull from expn hash table*/
    insert_trailE(tNew, ex);
    if (tOld != tNew)
    {
	free_list(tOld->used);
	my_free(tOld);
    }
}    

/*------------------------------------------------------------------------------------*/

void add_trail_copy(copy)
    trail *copy;
{
    /* add <copy> where ever it belongs */
    expn *ex = copy->ex;
    asg_arc *a;
    tilist *tl;

    for (tl = copy->tr; tl != NULL; tl = tl->next)
    {
	a = (asg_arc *)tl->this->this;
	insert_trailA(copy, a);
	pushnew(copy->ex->n, &a->nets);
    }
    insert_trailE(copy, ex);
}    

/*------------------------------------------------------------------------------------*/

void kill_hashE(ex)
    expn *ex;
{
    /* This goes through all of the trails listed in ex->queue[] and 
       deletes them, destroying the table. */
    int i;
    trail *t;
    trailist *trash, *temp, *trl;

    for (i=0; i<HASH_SIZE; i++) 
    {
	trl = ex->queue[i];
        while (trl != NULL)
	{
	    temp = trl->next;
	    t = trl->this;
	    delete_trail(t);
	    trl = temp;
	}
	ex->queue[i] = (trailist *)free_ilist(ex->queue[i]);
    }
}

/*------------------------------------------------------------------------------------*/

void rehashE(ex)
    expn *ex;
{
    /* This goes through all of the trails listed in ex->queue[] and 
       recomputes the costs, reordering the table. */
    int i;
    trail *t;
    trailist *temp, *trash, *trl;

    ex->seen = (tilist *)free_list(ex->seen);	/* necessary for yanking dead tiles */

    for (i=0; i<HASH_SIZE; i++) 
    {
	temp = NULL;

	for (trl = ex->queue[i]; trl != NULL; trl = trl->next)
	{
	    t = trl->this;
	    if (valid_trail_p(t) == TRUE)
	    {
		t->cost = trail_cost(t);
		ordered_ilist_insert(t, t->cost, &temp);
		note_tiles_added(t, ex);
	    }
	    else
	    {
		delete_trail(t);
	    }
	}

	trash = ex->queue[i];
	ex->queue[i] = temp;
	free_ilist(trash);
    }
}

/*------- Manage the Hash table in the arc structure: ----------------------------*/

unsigned hashA(ex)
    expn *ex;
{
    /* Given an expansion, hash your way to it */
    unsigned hashval = (unsigned)(((int)ex + ((int)ex % 5)) % HASH_SIZE);
    
    return (hashval);
}

/*------------------------------------------------------------------------------------*/

trailist *lookup_trailA(tr, a)
    trail *tr;
    asg_arc *a;
{
    /* This function returns a pointer to the trailist structure where the given 
       trail <a>  should be stored. */
    int index = hashA(tr->ex);
    if (find_indexed_element(tr->cost, a->trails[index]) != NULL) 
        return(a->trails[index]);
    else return(NULL);
}

/*------------------------------------------------------------------------------------*/

int insert_trailA(tr, a)
    trail *tr;
    asg_arc *a;
{
    /* This installs <tr> into the hashtable in <a> */
    int q = NULL, hashval = hashA(tr->ex);

    if (valid_trail_p(tr) == TRUE)
    {
	if ((a->trails[ hashval ] != NULL) && 
	    (a->trails[ hashval ]->this->cost == tr->cost))
	{	
	    /* Deal with the race condition: */
	    if (break_cost_tie(a->trails[hashval]->this, tr) == TRUE) tr->cost += 1;
	    else a->trails[hashval]->this->cost += 1;
	}
	q = ordered_ilist_insert(tr, tr->cost, &a->trails[ hashval ]);
    }

    return(q);
}

/*------------------------------------------------------------------------------------*/

trailist *remove_trailA(tr, a)
    trail *tr;
    asg_arc *a;
{
    /* This removes the given trail from the hash table in <a>. */
    int index = hashA(tr->ex);
    trail *q = (trail *)irem_item(tr, &a->trails[index], NULL);
    return(a->trails[index]);
}

/*------------------------------------------------------------------------------------*/

trail *best_trailA(a, ex)
    asg_arc *a;
    expn *ex;
{
    /* This examines  the best trail in the hash table in <a>: that belongs to the
       given expansion <ex>.  Faked to be non-destructive.  */

    trailist *temp = NULL;
    int index = hashA(ex), bestIndex = 0; 
    trail *besTrail;

    /* Check to make sure that <ex> did not collide with another expansion in the
       hash table: */
    besTrail = (trail *)ipop(&a->trails[index]);  /* Get the best trail on this list */

    /* If <besTrail> belongs to this expansion <ex>, we're done;  
        Otherwise, save this entry on <temp> and keep looking */
    for (; ((besTrail->ex != ex) && (a->trails[index] != NULL)); 
	 besTrail = (trail *)ipop(&a->trails[index]) ) 
        ipush(besTrail, &temp);

    /* Cleanup everything that has been pushed onto <temp>: 
       (This should correct the destructive ipop)	*/
    for(; temp != NULL; temp = temp->next)
    {
	/* Push everything back onto <a->trails[index]> */
	ordered_ilist_insert(temp->this, temp->index, &a->trails[index]);
    }

    /* Achtung! wichtig! return the <besTrail> to the table: */
    ordered_ilist_insert(besTrail, besTrail->cost, &a->trails[index]);

    return( (besTrail->ex == ex) ? besTrail : NULL );

}

/* This could be written better.... */

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

trail *any_trailA(a)
    asg_arc *a;
{
    /* This examines  the best trail in the hash table in <a>: 
       Faked to be non-destructive.  */

    int i;
    trail *nexTrail;

    for (i=0; i<HASH_SIZE; i++) 
    {
	if (a->trails[i] != NULL)
	{
	    nexTrail = (trail *)ipop(&a->trails[i]);  
	    ordered_ilist_insert(nexTrail, nexTrail->cost, &a->trails[i]);
	    return(nexTrail);
	}
    }
    return( NULL );
}

/*------------------------------------------------------------------------------------*/

kill_hashA(a)
    asg_arc *a;
{
    /* This goes through all of the trails listed in a->trails[] and 
       deletes them, destroying the table. */
    int i;
    trail *t;
    trailist *temp, *trash, *trl;

    for (i=0; i<HASH_SIZE; i++) 
    {
	temp = NULL;

	for (trl = a->trails[i]; trl != NULL; trl = trl->next)
	{
	    t = trl->this;
	    delete_trail(t);
	}
	a->trails[i] = (trailist *)free_ilist(a->trails[i]);
    }
}

/*------------------------------------------------------------------------------------*/

void full_rehashA(a)
    asg_arc *a;
{
    /* This goes through all of the trails listed in a->trails[] 
       and recomputes the costs, reordering the table. */
    int i;
    expn *ex;
    trail *t;
    trailist *temp, *trash, *trl;

    for (i = 0; i < HASH_SIZE; i++)
    {
	temp = NULL;
    
	for (trl = a->trails[i]; trl != NULL; trl = trl->next)
	{
	    t = trl->this;
	    t->cost = trail_cost(t);
	    ordered_ilist_insert(t, t->cost, &temp);
	}

	trash = a->trails[i];
	a->trails[i] = temp;
	free_ilist(trash);
    }
}

/*------------------------------------------------------------------------------------*/

void rehashA(a, ex)
    asg_arc *a;
    expn *ex;
{
    /* This goes through all of the trails listed in a->trails[] that are associated 
       with <ex> and recomputes the costs, reordering the table. */
    int i;
    trail *t;
    trailist *temp, *trash, *trl;

    i = hashA(ex);
    
    temp = NULL;
    
    for (trl = a->trails[i]; trl != NULL; trl = trl->next)
    {
	t = trl->this;
	
	t->cost = trail_cost(t);
	ordered_ilist_insert(t, t->cost, &temp);
    }

    trash = a->trails[i];
    a->trails[i] = temp;
    free_ilist(trash);
}

/*------------------------------------------------------------------------------------*/

trail *create_trail(ti, ex, p, dir)
    tile *ti;
    expn *ex;
    int p, dir;   /* array index to the root tile of the next page to be searched. */
{
    /* create a tile structure from scratch : */
    trail *tr = (trail *)calloc(1, sizeof(trail));
    asg_arc *a;

    tr->cost = 0;
    tr->ex = ex;
    tr->used = NULL;
    tr->tr = NULL;
    tr->page = p;
    tr->direction = dir;
    if (ti != NULL) 
    {
	push(ti, &tr->tr);	/* Add the tile to the list maintained by the trail */
	pushnew(ti, &ex->seen);	/* Add the tile to the list of tiles seen */
	a = (asg_arc *) ti->this;
    }
    /* insert_trailA(tr, a);  Don't add the trail to the initial tile just yet */
    /* insert_trailE(tr, ex); Don't add the trail to the expansion structure, just yet */
    
    return(tr);
}    

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

trail *create_subtrail(newTiles, ex, p, dir)
    tilist *newTiles;
    expn *ex;
    int p, dir;   /* array index to the root tile of the next page to be searched. */
{
    /* create a tile structure from scratch : */
    trail *tr = (trail *)calloc(1, sizeof(trail));
    asg_arc *a;
    tilist *tl;

    tr->cost = trail_cost_est(newTiles, ex, p);
    tr->ex = ex;
    tr->used = NULL;
    tr->tr = NULL;
    tr->page = p;
    tr->direction = dir;
    tr->tr = (tilist *)copy_list(newTiles);

    /* Inform the arcs on the list of the new trail being established: */
    for (tl = newTiles; tl != NULL; tl = tl->next)
    {
	a = (asg_arc *)tl->this->this;
	insert_trailA(tr, a);
    }
    
    /* Clean up the expansion structure: */
    insert_trailE(tr, ex);
    
    return(tr);
}    

/*------------------------------------------------------------------------------------*/

trail *copy_trail(t)
    trail *t;
{
    trail *new = create_trail(NULL, t->ex, t->page, t->direction);

    new->cost = t->cost;
    new->tr = (tilist *)copy_list(t->tr);
    new->jEx = t->jEx;
    new->used = t->used;

    /* Don't forget to update the expansion. (insert_trailE()....) */
    /* Don't forget to update the tile. (insert_trailA()....) */
    return(new);
}      

/*------------------------------------------------------------------------------------*/

trlist *remove_all_trails_from_arc(a)
    asg_arc *a;
{
    /* Remove all trails from the given arc.  Compile them into a list, and return 
       the list. */

    trail *temp = NULL;
    trlist *collectedTrails = NULL;

    do {
	temp = any_trailA(a);
	if (temp != NULL)
	{
	    push(temp, &collectedTrails);
	    remove_trail(temp);
	}
    } while (temp != NULL);

    a->nets = (nlist *)free_list(a->nets);
    a->congestion = 0;

    return(collectedTrails);
}

/*------------------------------------------------------------------------------------*/

trail *add_to_trail(tr, ti)
    trail *tr;		/* Trail being added to */
    tile *ti;		/* Tile being added to the list */
{	
    /* Add this tile to the existing trail <tr>:   Does all of the book keeping for
       the arc/tile structures.  */
    asg_arc *a;
    tilist *tl;

    tr->direction = toggle_direction(tr->direction);
    /* Since you're modifyinging the trail, the previous joint may not be valid.*/
    tr->jEx = NULL;   /* Done so as not to confuse the "valid_trial_p" function ????? */ 

    if (ti != NULL) 
    {
	push(ti, &tr->tr);   /* Add the tile to the tile list maintained by the trail */
	tr->cost = trail_cost(tr);

	/* Add this trail to all of the arcs along the path... */
	for (tl = tr->tr; tl != NULL; tl = tl->next)
	{
	    a = (asg_arc *)tl->this->this;
	    remove_trailA(tr, a);	/* The ordering may have changed... */
	    insert_trailA(tr, a);
	}
			
	a = (asg_arc *)ti->this;

	if ((debug_level == 11) && (!strcmp(net_under_study, tr->ex->n->name)) 
	    && (a->local == 0))
	{
	    a->local = tr->cost;
	}
    }
    return(tr);
}

/*------------------------------------------------------------------------------------*/

void trim_trails_at(ex, ti)
    expn *ex;
    tile *ti;
{
    /* find all trails belonging to <ex> in <ti> and clip them at <ti>.  This involves
       removing all tiles on the trail->tr lists that are in front of <ti>, and 
       then recosting the whole trail.  Later, the set of trails affected needs to be
       checked for duplicate trails.  */
    asg_arc *a = (asg_arc *)ti->this;
    tilist *tl;
    trail *t;
    trailist *trl, *temp = NULL;
    trlist *trll, *trailSeen = NULL, *uniqueTrails = NULL;

    /* get the trails belonging to <ex> that pass through <a>: */
    temp = (trailist *)copy_ilist(a->trails[hashA(ex)]);

    for (trl = temp; trl != NULL; trl = trl->next)
    {
	if (trl->this->ex == ex)	/* Only look at trails belonging to <ex> */
	{
       	    t = trl->this;
	    remove_trailE(t, ex);			   /* Pull from expn hash table*/

	    /* Remove all tiles from the list up to (but not including) <ti>: */
            for (tl = t->tr; ((tl != NULL) && (tl->this != ti));
		 tl = tl->next)
            {   
		remove_trailA(t, (asg_arc *)tl->this->this ); /* Pull from asg_arc hash table */
		pop(&t->tr);			       /* Yank this (extraneous) tile.*/
	    }

            /* Now remove <ti>: */
            if (tl != NULL)
	    {	/* Now remove <ti>: */
		remove_trailA(t, (asg_arc *)tl->this->this );        
		pop(&t->tr);			/* Yank <ti>, then reinsert later... */

		/* Now reconstruct the trails, adding the tile back again, and 
		   recomputing it's cost: */
       /* NOTE: This will screw up a->trails[...], hence the "copy_ilist" call */
		add_to_trail(t, ti); 		/* Does bookeeping (costs, etc.) */
		insert_trailE(t, ex);           /* Add back to expn hash table */
		push(t, &trailSeen);		/* Save for posterity */
	    }
	}
    }
    free_ilist(temp);

    /* Look for, and remove duplicate trails: */
    for (trll  = trailSeen; trll != NULL; trll = trll->next)
    {
	t = trll->this;
	if (unique_trail(t, &uniqueTrails) == FALSE)
	{
	    delete_trail(trll->this);
	}
    }
    free_list(trailSeen);
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

int opposite_side(dir)
    int dir;
{
    switch(dir)
    {
	case LEFT: return(RIGHT);
	case UP:   return(DOWN);
	case RIGHT:return(LEFT);
	case DOWN: return(UP);
    }
    return(dir);
}

/*------------------------------------------------------------------------------------*/

int toggle_direction(lastOrientation)
    int lastOrientation;
{
    /* This toggles the direction that the expansion set is saught from;  That is,
       if the last set of expansions came from the routingRoot[HORZ][p] plane, then
       the next set should come from the routingRoot[VERT][p] plane & vv. */

    if (lastOrientation == HORZ) return(VERT);
    else return(HORZ);
}

/*------------------------------------------------------------------------------------*/

expn *create_expansion(t)
    term *t;
{
    /* create an expansion node for this terminal */
    expn *ex = (expn *)calloc(1, sizeof(expn));
    int i;
    ex->n = t->nt;
    ex->t = t;
    ex->siblings = NULL;	ex->connections = NULL;
    ex->seen = NULL;	
    for (i=0; i<HASH_SIZE; i++) ex->queue[i] = NULL;
    push(ex, &t->nt->expns);	/* Tie the expansion into the net structure */
    push(ex, &ex->siblings);
    return(ex);
}

/*-----------------------------------------------------------------------------------*/

int length_to_next_corner(t1, t2, orgX, orgY, useOrg, x, y)
    tile *t1, *t2;
    int orgX, orgY, useOrg;
    int *x, *y;
{
    /* This returns the length in <t1> up to <t2>, correctly handling the 
       null conditions.  It's side effect is to produce the appropriate
       coordinates for the point of intersection between <t1> & <t2> */
    asg_arc *a1, *a2;
    int q1 = 0, q2 = 0, midpnt = 0;

    if (t1 == NULL) 
    {
	if (t2 == NULL) return (0);
	else 
	{
	    a2 = (asg_arc *)t2->this;
	    *x = t2->x_pos + (t2->x_len / 2);
	    *y = t2->y_pos + (t2->y_len / 2);
	    if (useOrg == FALSE) return(t2->x_len/2);
	    else return( (int)abs(orgX - (t2->x_pos + t2->x_len)) );
	}
    }

    a1 = (asg_arc *)t1->this;    

    if (t2 == NULL) 
    {
	*x = t1->x_pos + (t1->x_len / 2);
	*y = t1->y_pos + (t1->y_len / 2);
	if (useOrg == FALSE) return(t1->x_len/2);
	else return( (int)abs(orgX - (t1->x_pos + t1->x_len)) );
    }


    else       	/* Given both tiles, determine the length of t2 that is available
		   after the intersection point of the two tiles. */
    {
	a2 = (asg_arc *)t2->this;
	if ((a1->page == VERT) && (a2->page == HORZ))
	{
	    find_intersection(t1->x_pos, t1->x_pos + t1->x_len, 
			      t2->y_pos, t2->y_pos + t2->y_len, &q1, &q2);
	    midpnt = (q1 + q2) / 2;
	    *y = midpnt; 	
	    *x = t1->y_pos + (t1->y_len / 2);
	    return((useOrg == FALSE) ? t1->x_len / 2 : (int) abs(orgY - midpnt));
	}
	else if ((a1->page == HORZ) && (a2->page == VERT))	      
	{
	    find_intersection(t2->y_pos, t2->y_pos + t2->y_len, 
			      t1->x_pos, t1->x_pos + t1->x_len, &q1, &q2);
	    midpnt = (q1 + q2) / 2;
	    *x = midpnt; 	
	    *y = t1->y_pos + (t1->y_len / 2);
	    return((useOrg == FALSE) ? t1->x_len / 2 : (int) abs(orgX - midpnt));
	}
	else	/* Adjacent tiles, how odd... */
	{
	    *x = t2->x_pos + (t2->x_len / 2);
	    *y = t2->y_pos + (t2->y_len / 2);
	    if(a2->page == HORZ) 
	        return( (useOrg == FALSE) ? (t2->y_len + t1->y_len) / 2 :
		                            (int)abs (orgY - *y));
	    else return( (useOrg == FALSE) ? (t2->x_len + t1->x_len) / 2 :
			                     (int) abs(orgX - *x));
	}
    }
}

/*-----------------------------------------------------------------------------------*/

int distance_to_go(ex, x, y, closestEx)
    expn *ex;
    int x, y;
    expn **closestEx;
{
    /* Given the (x, y) position of this particular piece of <ex>, find the minimum
       manhattan distance to a legal terminus of <ex>.  The set of legal terminii
       is the terminals corresponding to the expansions that are part of <ex>->n, but
       are not siblings of <ex>.   */

    expnlist *exl;
    term *t;
    int tempX, tempY, dist, minDist = FALSE;

    for (exl = (expnlist *)ex->n->expns; exl != NULL; exl = exl->next)
    {
	if (member_p(exl->this, ex->siblings, identity) == FALSE)
	{
	    t = exl->this->t;
	    tempX = t->x_pos + t->mod->x_pos + 2 * terminal_spacer(t, X);	
	    tempY = t->y_pos + t->mod->y_pos + 2 * terminal_spacer(t, Y);
	    dist = (int)abs(tempX - x) + (int)abs(tempY - y) + 
	           EXPECTED_CORNERS * CORNER_COST;
	    if ((minDist == FALSE) || (dist <= minDist)) 
	    {
		minDist = dist;
		*closestEx = exl->this;
	    }
	}
    }
    
    if (minDist != FALSE) return ((int)(minDist * FORWARD_EST_MULTIPLIER));
    else return(0);				/* Problem */
}

/*-----------------------------------------------------------------------------------*/

int trail_cost(tr)	
    trail *tr;
{
    /* Estimate the cost to use this trail.  */
    int a, b, c, d;
    return(trail_cost_est(tr->tr, tr->ex, tr->page, &a, &b, &c, &d));
}

/*-----------------------------------------------------------------------------------*/

int trail_cost_est(tiles, ex, page, trailLength, trailEst, tileCount, trailCong)	
    tilist *tiles;
    expn *ex;
    int page, *trailLength, *trailEst, *tileCount, *trailCong;

{
    /* Estimate the cost for the given expansion to use this string of tiles.  */

    int lookaheadEstimate, length = 0, cornerCount = 1;
    int congestionCost = 0;
    int tx, ty, cornX, cornY;
    term *t;
    asg_arc *a;
    expn *nearestEx = NULL;
    tile *termTile;
    tile *lasTile= (tiles != NULL) ? tiles->this : NULL;
    tile *curTile = ((lasTile != NULL) && (tiles->next != NULL)) ? 
                      tiles->next->this : NULL;
    tile *nexTile = ((curTile != NULL) && (tiles->next->next != NULL)) ? 
                      tiles->next->next->this : NULL;
    tilist *tl;

    /* What should be be returning in this case (where something has	*/
    /* gone badly wrong)?						*/
    if (lasTile == NULL) {
	error("trail_cost_est: passed a NULL tile", "");
        return 0;
    }

    /* Handle the first tile on the trail: */
    a = (asg_arc *)lasTile->this;

    congestionCost = congestion_cost(a);

    /* Set up the corner point, but ignore the length estimate (no corner to base 
       from yet)  */
    length_to_next_corner(lasTile, curTile, cornX, cornY, FALSE, &cornX, &cornY);

    /* If this is a complete trail, that is it connects to the closest expansion
       not in <ex>'s expansion group, then measure the distance from (<cornX>,<cornY>)
       to <nearestEx>'s terminal.  Otherwise, use the <lookaheadEstimate>. */
    
    lookaheadEstimate = distance_to_go(ex, cornX, cornY, &nearestEx);
    if (nearestEx != NULL)
    { 
	termTile = initial_move(nearestEx, page);
	if (termTile == lasTile)
	{
	    tx = nearestEx->t->mod->x_pos + nearestEx->t->x_pos;
	    ty = nearestEx->t->mod->y_pos + nearestEx->t->y_pos;
	    length += (int)abs(cornX - tx);
	    length += (int)abs(cornY - ty);	    
	}
	else length = lookaheadEstimate;
    }


    /* Set up to evaluate the second tile: */
    lasTile = curTile;
    curTile = nexTile;
    nexTile = ((nexTile != NULL) && (tiles->next->next->next != NULL)) ? 
                tiles->next->next->next->this : NULL;

    /* There are three or more tiles on the trail: */
    for (tl = (nexTile != NULL) ? tiles->next->next->next->next : NULL; 
	 (lasTile != NULL);
	 tl = (tl != NULL) ? tl = tl->next : NULL)
    {
	a = (asg_arc *)lasTile->this;
	congestionCost += congestion_cost(a);
	if (curTile != NULL)
	{
	    length += length_to_next_corner(lasTile, curTile, cornX, cornY, 
					    TRUE, &cornX, &cornY);
	    cornerCount += 1;
	}
	else 
	{
	    length += (int)abs(cornX - (ex->t->x_pos + ex->t->mod->x_pos));
	    length += (int)abs(cornY - (ex->t->y_pos + ex->t->mod->y_pos));
	}

	/* Set up to evaluate the next tile: */
	lasTile = curTile;
	curTile = nexTile;
	nexTile = (tl != NULL) ? tl->this : NULL;
    }

    *trailLength = length;
    *trailEst = lookaheadEstimate;
    *tileCount = cornerCount;
    *trailCong = congestionCost;

    return(length * LENGTH_WEIGHT +  cornerCount * CORNER_COST +  congestionCost );
}	

/*-------------------------------------------------------------------------------------*/

int break_cost_tie(tr1, tr2)
    trail *tr1, *tr2;
{
    /* return TRUE iff the cost of <tr1> is preferred to that of <tr2>: */

    int length1, length2, moveCount1, moveCount2;
    int forwardEst1, forwardEst2;
    int congestion1, congestion2, totalCost1, totalCost2;

    if (valid_trail_p(tr1) == FALSE)
    {
	return(FALSE);
    }
    if (valid_trail_p(tr2) == FALSE)
    {
	return(TRUE);
    }

    totalCost1 = trail_cost_est(tr1->tr, tr1->ex, tr1->page, &length1, 
				&forwardEst1, &moveCount1, &congestion1);

    totalCost2 = trail_cost_est(tr2->tr, tr2->ex, tr2->page, &length2, 
				&forwardEst2, &moveCount2, &congestion2);


    if (totalCost1 < totalCost2) return (TRUE);
    else if (totalCost1 > totalCost2) return(FALSE);
    else
    {
	if (congestion1 < congestion2) return(TRUE);
	else if (congestion1 > congestion2) return(FALSE);
	else
	{
	    if (moveCount1 < moveCount2) return(TRUE);
	    else if (moveCount1 > moveCount2) return(FALSE);
	    else
	    {
		if (length1 > length2) return(FALSE);
		else if (length1 < length2) return(TRUE);
		else 
		{
		    if (forwardEst1 < forwardEst2) return(TRUE);
		    else if (forwardEst1 > forwardEst2) return(FALSE);
		    else 
		    {
			if (tr1->jEx != NULL)return(TRUE);
			else if (tr2->jEx != NULL) return(FALSE);
			else return(TRUE);
		    }
		}
	    }
	}
    }
}	

/*------------------------------------------------------------------------------------*/

int lower_cost(t1, t2)
    trail *t1, *t2;
{
    /* This compares the cost of <t1> & <t2>.  If <t1> is less expensive than <t2>,
       TRUE is returned. */

    if (t1->cost < t2->cost) return(TRUE);
    else return(FALSE);
}

/*------------------------------------------------------------------------------------*/

int set_congestion1(a)
    asg_arc *a;
{
    /* Congestion defaults to corner count.  See `congestion_cost' */
    a->congestion = 0;
}

/*------------------------------------------------------------------------------------*/

int set_congestion2(a)
    asg_arc *a;
{
    int crossoverCount = 0, cornerCount = 0;
    tilist *temp = NULL, *tl;
    asg_arc *perpA;
    nlist *nl;
    
    /* Count the corners in this tile: */
    for(nl = a->nets; nl != NULL; nl = nl->next) cornerCount++;

    /* Examine each tile that crosses this tile... */
    temp = collect_tiles(a->t, routingRoot[toggle_direction(a->page)][currentPartition]);
    for (tl = temp; tl != NULL; tl = tl->next)
    {
	/* Count the number of nets that are in the perpendicular tile: */
	perpA = (asg_arc *)tl->this->this;
	for(nl = perpA->nets; nl != NULL; nl = nl-> next) crossoverCount++;
    }
    free_list(temp);
    a->congestion = cornerCount * crossoverCount + cornerCount;
    return (a->congestion);

}

/*------------------------------------------------------------------------------------*/

select_congestion_rule(congestionRule)
    int congestionRule;
{
    switch(congestionRule)
    {
	case 1 : set_congestion_value = (int (*)())set_congestion1; break;
	case 2 : set_congestion_value = (int (*)())set_congestion2; break;
	default: error("select_congestion_rule - bad rule choice ", "");
	         break;
    }
}

/*------------------------------------------------------------------------------------*/

int congestion_cost(a)
    asg_arc *a;		/* asg_arc to be used... */
{
    /* This is an estimate of what it costs to use the given asg_arc.  This should be
       refined as a real rout becomes a local route... */

    /* The cost of congestion = 2x corner count + crossover count + penalty(if any) 
       (See `set_congestion2 this function to verify this comment) */

    float cost, TracksAvail, TracksUsed;

    TracksUsed = (float)list_length(a->nets);
    TracksAvail = (a->page == VERT) ? (float)a->hcap : (float)a->vcap;

    cost = (TracksUsed / TracksAvail) * TRACK_WEIGHT +      /* Corner-only component */
           a->congestion / TracksAvail;			    /* Crossover component */

    /* Severely penalize overusage: */
    if (TracksUsed >= (int)TracksAvail-1) 
    {
	cost = 5 * cost + 40;
    }
    return( (int)(cost) );
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Algorithmic stuff 
 * given static costs built into the routing space, route all nets:
 *-------------------------------------------------------------------------
 */

/*------------------------------------------------------------------------*/

int on_same_net(exp1, exp2)
    expn *exp1, *exp2;
{
    return (((int)exp1->n == (int)exp2->n) && ((int)exp1 != (int)exp2));
}

/*------------------------------------------------------------------------*/

int free_to_route_in_p(t)
    tile *t;
{
    /* Returns TRUE if the tile is free for routing.... */
    asg_arc *a = (t != NULL) ? (asg_arc *)t->this : NULL;
    if ((a != NULL) && (a->mod == NULL)) return(TRUE);
    else return(FALSE);
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

nlist *list_nets_represented(expns, temp)
    expnlist *expns;
    nlist **temp;
{
    /* Given a list of expansions, create a list of the nets represented */
    expnlist *exl;
    for (exl = expns; exl != NULL; exl = exl->next)
    {
	pushnew(exl->this->n, temp);
    }
    return(*temp);
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

expn *locate_fellow_expansion(exlist, ex_already_found)
    expnlist *exlist;
    expn *ex_already_found;
{
    /* Return the expansion from <exl> that is on the same net as <ex_already_found> */

    expnlist *exl;
    
    for (exl = exlist; exl != NULL; exl = exl->next)
    {
	if (on_same_net(ex_already_found, exl->this) == TRUE) return(exl->this);
    }
    return(NULL);
}

/*------------------------------------------------------------------------------------*/

tilist *collect_tiles(starTile, Root)
    tile *starTile, *Root;
{
    /* given an area-of-interest (AOI) based on the dimensions of <startTile>, find 
       the list of tiles from <Root> that are contained.  It is assumed that <starTile>
       is not in the same tile-space as <Root>, so  the x/y values are inverted.*/
    tilist *temp = NULL;
    
    if (debug_level == 5)
	fprintf(stderr, "Collecting tiles from Root tilespace 0x%x\n", Root);

    temp = area_enumerate(Root, &temp, free_to_route_in_p, 
			  starTile->y_pos, starTile->x_pos,
			  starTile->y_len,  starTile->x_len );

    /* Added by Tim 2/5/05:  If no tiles were found, but the boundary	*/
    /* of Root borders on the space of interest, then expand Root by	*/
    /* one unit and try again.						*/

/*
    if (temp == NULL) {
       if (debug_level == 5)
	  fprintf(stderr, "temp = NULL in given area\n");
       temp = area_enumerate(Root, &temp, free_to_route_in_p, 
			  starTile->y_pos, starTile->x_pos,
			  starTile->y_len + 1,  starTile->x_len + 1);
       if (debug_level == 5) {
          if (temp != NULL)
	     fprintf(stderr, "temp = 0x%x in expanded area\n", temp);
	  else
	     fprintf(stderr, "temp = NULL in expanded area\n");
       }
    }
*/

    /* end of test code. . . ---Tim */

    return(temp);
}

/*------------------------------------------------------------------------------------*/

trlist *list_trails_using_A(ex, A)
    expn *ex;
    asg_arc *A;
{
    trailist *tl;
    trlist *temp = NULL;

    for(tl = A->trails[hashA(ex)]; tl!= NULL; tl = tl->next)
    {
	if (tl->this->ex == ex) push(tl->this, &temp);
    }
    return(temp);
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

tilist *remove_tiles_on_circular_trail(ex, besTrail, completeList)
    expn *ex;			/* Expansion (group) that is being extended */
    trail *besTrail;		/* Trail that got me here... */
    tilist **completeList;	/* What tiles are expansion candidates */
{
    /* This function returns a list without any tiles that would create a circular
       trail.   */

/* Need to insure that the 'NewPathDominates' only applys to tiles on the 'wavefront.' */

    tilist *tl, *copy;
    tile *newTile;
    expnlist *exl;


    copy = (tilist *)rcopy_list(*completeList);

    for (tl = copy; tl != NULL; tl = tl->next)
    {
	newTile = tl->this;
	if (member_p (newTile, besTrail->tr, identity) == TRUE)		/* No duplicate tiles */
	    rem_item(newTile, completeList);
	
	else 
	{
	    for (exl = ex->siblings; exl != NULL; exl = exl->next)
	    {
		ex = exl->this;	
		if (member_p (newTile, ex->seen, identity) == TRUE)
		{
		    /* Visited this tile before... */
		    rem_item(newTile, completeList);
		}
		/* else - <ex> hasn't visited this guy, so keep checking in the group */
	    }
	}
    }
    free_list(copy);
    return (*completeList);
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

trail *flip_trail(t)
    trail *t;
{
    trail *newTrail;
    trlist *trl, *temp, *newConnList = NULL;
    expn *ex = t->ex;
    expnlist *exl;

    /* create a new trail for t->jEx:   The old one gets trashed. */
    newTrail = create_trail(NULL, t->jEx, t->page, VERT);
    newTrail->jEx = t->ex;
    newTrail->tr = (tilist *)rcopy_list(t->tr);
    newTrail->cost = trail_cost(newTrail);

    add_trail_copy(newTrail);				/* Add to all asg_arc hash tables */
    remove_trailE(newTrail, newTrail->ex);	        /* Pull from expn hash table */
    
    queue(newTrail, &ex->connections);	/* gs Risky Business... */
    pull_trail_from_all_arcs(t);        /* Perhaps it should be returned to the expn
					   hash table, but then it would compete... */
    /* Explicity maintain the ->connections lists: */
    trl = ex->connections;
    while (trl != NULL)
    {
	temp = trl->next;
	if (trl->this == t)
	{
	    rem_item(t, &ex->connections);
	    newConnList = ex->connections;

	    for (exl = ex->siblings; exl != NULL; exl = exl->next)
	    {
		exl->this->connections = newConnList;
	    }
	}
	trl = temp;
    }
    my_free(t);                       /* BOOM ! */                 
    return(newTrail);
}

/*------------------------------------------------------------------------------------*/

adjust_trailset(expnToSet, expnToFlip)	
    expn *expnToSet, *expnToFlip;
{
    trail *newTrail = NULL;
    trlist *trl;
    
    /* Fix the expnToFlip->connections so that <expnToSet> is represented:  To be
       represented, an expansion must have a single trail that contains the given 
       expansion in the ->ex field.  
       
       This gets interesting, because <expnToSet> is the expansion known to be unique, 
       ie: desirous of a trail;  The routine is designed to set up the trail for 
       <expnToFlip>, so if <expnToFlip> has already terminated, then some existing 
       trail needs to be 'flipped,' making it now representative of its ->jEx
       expansion;  This 'flip' may require other trails to be flipped, as long
       as the <expnToSet> winds up with a trail.
       */
	    
    /* find the <expnToSet> and do so: */
    for (trl=expnToFlip->connections; 
	 ((trl != NULL) && (trl->this->jEx != expnToSet)); trl=trl->next);

    if (trl != NULL)
    {
	newTrail = flip_trail(trl->this);

	if ((newTrail != NULL) && (newTrail->jEx != expnToFlip))
            adjust_trailset(newTrail->jEx, expnToFlip);
    }    
}

/*------------------------------------------------------------------------------------*/

void terminate_expansion(ex, jEx, jArc, besTrail, ActiveExpns)
    expn *ex, *jEx;
    asg_arc *jArc;
    trail *besTrail;
    expnlist **ActiveExpns;
{
    /* Record the termination position (asg_arc) of the expansion, notify the net that 
       the expn is done, and clip it from the <ActiveExpn> list. */
    int q, xAv, yAv, xPos, yPos;
    int startCost = besTrail->cost;
    net *n = ex->n;
    term *t;
    trail *sibTrail;
    tile *firsTile;
    tilist *tl, *completionList;
    expn *newJex = NULL, *whoStopped = NULL;
    expnlist *exl;

    for (exl = ex->siblings; ((exl != NULL) && (whoStopped == NULL)); exl = exl->next)
    {
	/* Make sure somebody we care about stops  */
	whoStopped = (expn *)rem_item(exl->this, ActiveExpns);
    }

    if (whoStopped != NULL)
    {
	push(whoStopped, &n->done);	
	if (whoStopped != ex)		/* Indirection! */
	{
	    /* The end object is to arrange that at the end of the termination process, 
	       the global route needs to consist of a set of trails where each trail 
	       represents a unique terminal.  */

	    adjust_trailset(whoStopped, ex);

	}	
	jEx->siblings = (expnlist *)append_list(jEx->siblings, ex->siblings);

	/* Now create the trail that comprises the complete path from <ex> to <jEx>: */
	sibTrail = best_trailA(jArc, jEx);
	for (tl = sibTrail->tr; ((tl != NULL) && (tl->this != jArc->t)); tl = tl->next);
	completionList = tl;
	for (tl = completionList->next; tl != NULL; tl = tl->next)
	{
	    add_to_trail(besTrail, tl->this);
	}

	besTrail->jEx = jEx;
	push(besTrail, &ex->connections);
	jEx->connections = (trlist *)append_list(jEx->connections, ex->connections);

	if (debug_level >= 4)
	{
	    /* print out something useful... */
	    q = list_length(ex->n->done);
	    xAv = (jArc->t->x_pos + jArc->t->x_len)/ 2;
	    yAv = (jArc->t->y_pos + jArc->t->y_len)/ 2;
	    if (jArc->page == VERT)
	    {
		yAv = xAv;
		xAv = (jArc->t->y_pos + jArc->t->y_len)/ 2;
	    }
	    
	    if (whoStopped != ex)
	    {
		fprintf(stderr, 
      "****<%s>\t{%s %s} terminated via <%s>{%s %s} near (%d, %d)%s after %d visits\n",
		    whoStopped->n->name, whoStopped->t->name, whoStopped->t->mod->name,
			ex->n->name, ex->t->name, ex->t->mod->name,
			xAv, yAv, (jArc->page == VERT) ? "v" : "h", ex->eCount);
	    }
	    else 
	    {
		fprintf(stderr, 
      "****<%s>\t{%s %s} terminated near (%d, %d)%s after %d visits[%d/%d]\n",
			ex->n->name, ex->t->name, ex->t->mod->name,
			xAv, yAv, (jArc->page == VERT) ? "v" : "h", 
			ex->eCount, startCost, besTrail->cost);
	    }
	    fprintf(stderr, "****<%s> \t{%s %s} merged with <%s> {%s %s}\n",
		    ex->n->name, ex->t->name, ex->t->mod->name,
		    jEx->n->name, jEx->t->name, jEx->t->mod->name);
	}	
	
	if (list_length(n->done) + 1 == list_length(n->expns))
	{
	    if (member_p(jEx, *ActiveExpns, identity) == TRUE)
	    {
		/* Explicitly terminate <jEx>:  */
		completionList = (tilist *)rcopy_list(sibTrail->tr);
		firsTile = completionList->this;
 		free_list(completionList);  
		sibTrail = create_trail(firsTile, jEx, sibTrail->page, VERT);
		insert_trailA(sibTrail, sibTrail->tr->this->this);
		sibTrail->jEx = jEx;    /* This is a terminated trail, so set ->jEx... */
		newJex = jEx;
	    }

	    else 
	    {
		/* Need to explicitly terminiate the remaining active expansion that 
		   <jEx> belongs to, so first find it... */
		exl = (expnlist *)member(jEx, *ActiveExpns, on_same_net);
		newJex = exl->this;

		/* Then set up a trail with only the first move on it... */

		t = newJex->t;
		xPos = t->x_pos + t->mod->x_pos + terminal_spacer(t, X);
		yPos = t->y_pos + t->mod->y_pos + terminal_spacer(t, Y);

		firsTile = locate_first_tile(t->side, xPos, yPos, sibTrail->page);
		sibTrail = create_trail(firsTile, newJex, sibTrail->page, VERT);
		sibTrail->jEx = newJex; /* This is a terminated trail, so set ->jEx... */
		insert_trailA(sibTrail, sibTrail->tr->this->this);
	    }

	    /* Wrap up the <newJx> as well: */
	    if (firsTile == (tile *)nth(besTrail->tr, list_length(besTrail->tr)))
	        sibTrail->jEx = besTrail->ex;
	    push(sibTrail, &jEx->connections);	
	    push(newJex, &n->done);
	    rem_item(newJex, ActiveExpns);
	    
	    if (debug_level >= 4)
	    {
		/* print out something useful... */
		fprintf(stderr, "****<%s>\t{%s %s} completes the GR for <%s>.\n",
			newJex->n->name, newJex->t->name, newJex->t->mod->name, 
			ex->n->name);
	    }	
	}
	/* Complete the merger of the two expansions: */
	for (exl = jEx->siblings; exl != NULL; exl = exl->next)
	{
	    exl->this->connections = jEx->connections;
	    exl->this->siblings = jEx->siblings;
	    /* The merger changes the "distance_to_go" measure, so recalculate the 
	       trail costs */
	    rehashE(exl->this);		/* if (newJex == NULL) rehashE(exl->this); */
	}
    }
}

/*------------------------------------------------------------------------------------*/

int meets_termination_conditions_p(tr, parentEx, besTrail)
    trail *tr;			/* Trail being checked */
    expn *parentEx;		/* Expansion that is being checked for termination */
    trail *besTrail;		/* Best trail so far in the given asg_arc */
{
    if ((tr->ex->n == parentEx->n) &&	      /* IF it belongs to the same net */
	
	(member_p(tr->ex, parentEx->siblings, 
		  identity) == FALSE) &&      /* AND is not part of <parent>'s 
						 group */
	
	((besTrail == NULL) || 		     /* AND is the best trail so far */
	 (tr->cost < besTrail->cost) ||
	 ((besTrail->jEx != NULL) && (tr->cost == besTrail->cost))))
    {
	return(TRUE);
    }
    else
    {
	return(FALSE);
    }
}

/*------------------------------------------------------------------------------------*/

int check_for_AND_terminate_expn(t, expnsStillActive)
    trail *t;
    expnlist **expnsStillActive;
{
    /*  This seasg_arches through the list of trails that have been hashed into the 
	proposed jointArc <a>.	Within this list of trails, each is checked to see 
	if it completes a subnet of the <parent> expansion. 

	IF it so happens that a subnet that is a <besTrail> is discovered (and can
	be legally terminated), it is - <expnsStillActive> is decremented, and TRUE 
	is returned.  				
     */
    
    expn *parentEx = t->ex;
    asg_arc *a = (asg_arc *)t->tr->this->this;		  /* Proposed joint arc for <parent> */
    trail *besTrail = NULL, *temp;
    trailist *trll;

    int i;

    for (i=0; i<HASH_SIZE; i++)
    {
	for (trll = a->trails[i]; trll != NULL; trll = trll->next)
	{
	    /* the arc lists all trails that cross it.  Find all that are on the same 
	       net, but not the same expansion.  If any are found, then this expansion
	       may terminate.  Do it.*/
	    
	    temp = trll->this;
	    
	    if (meets_termination_conditions_p(temp, parentEx, besTrail) == TRUE)
	    {
		besTrail = temp;
	    }
	}
    }	
    if (besTrail != NULL)
    {
	terminate_expansion(parentEx, besTrail->ex, a, t, expnsStillActive);
	return(TRUE);
    }
    else return(FALSE);
}

/*------------------------------------------------------------------------------------*/

expnlist *check_for_termination(t)
    trail *t;
{
    /*  This searches through the list of trails that have been hashed into the 
	proposed jointArc <a>.	Within this list of trails, each is checked to see 
	if it completes a subnet of the <parent> expansion. 

	IF it so happens that a subnet that is a <besTrail> is discovered (and can
	be legally terminated), the pointer the expansion that caused the best terminus 
	is returned.

	NOTE:  There may be more than one of these.
     */
    
    asg_arc *a;

    if (t->tr == NULL) {
        error("check_for_termination:  trail component tr is NULL", "");
	return NULL;
    }

    a = (asg_arc *) t->tr->this->this;	/* Proposed joint arc for <parent> */

    expn *parent = t->ex;
    expnlist *terminationCandidateList = NULL;
    trail *temp, *besTrail = NULL;
    trailist *trll;

    int i;

    for (i=0; i<HASH_SIZE; i++)
    {
	for (trll = a->trails[i]; trll != NULL; trll = trll->next)
	{
	    /* the asg_arc lists all trails that cross it.  Find all that are on the same 
	       net, but not the same expansion.  If any are found, then this expansion
	       may terminate. */
	    
	    temp = trll->this;
	    
	    if ((temp != besTrail) &&
		(meets_termination_conditions_p(temp, parent, besTrail) == TRUE))
	    {
		besTrail = temp;
		pushnew(temp->ex, &terminationCandidateList);
	    }
	}
    }	
    if (terminationCandidateList != NULL)
    {
	return(terminationCandidateList);
    }
    else return(NULL);
}

/*------------------------------------------------------------------------------------*/

trail *examine_best_trail_in_expn_group(ex)
    expn *ex;
{	/* Generalized form of "examine_best_trailE" */
    expnlist *exl;
    trail *t = NULL, *tempTrail;

    /* Get the cell to move from-- This is the best trail for the given expansion: */
    for (exl = ex->siblings; exl != NULL; exl = exl->next)
    {
	/* Check <ex> and all of <ex>'s siblings for the best trail... */
	tempTrail = examine_best_trailE(exl->this);
	if ((t == NULL) || 
	    ((tempTrail != NULL) && (tempTrail->cost <= t->cost))) t = tempTrail;
    }
    return(t);
}

/*------------------------------------------------------------------------------------*/

trail *extract_best_trail_in_expn_group(ex)
    expn *ex;
{	/* Generalized form of "best_trailE" */
    trail *t = NULL;
    t = examine_best_trail_in_expn_group(ex);
    remove_trailE(t, t->ex);
    return(t);
}

/*------------------------------------------------------------------------------------*/

trail *restart_expansion(ex)
    expn *ex;			/* expansion being restarted */
{	
    /* Delete all discovered trails for this expansion, and start over. */

    static expn *lastEx;
    tile *ti;
    trail *t, *make_startup_trail();
    asg_arc *a;

    if (lastEx == ex) return(NULL);
    else lastEx = ex;

    kill_hashE(ex);
    ex->connections = NULL;  	 			   /* Should be NULL already */
    ex->seen = (tilist*) free_list(ex->seen);	

    t = make_startup_trail(ex, initial_move(ex, 0), 0);
    if (t->tr == NULL) {
	error("restart_expansion: trail has NULL component", "");
        return NULL; 	/* Something has gone wrong. . . */
    }
    a = (asg_arc *)t->tr->this->this;
    insert_trailA(t, a);  	
    insert_trailE(t, ex);	     /* Will be the only trail in the expansion group */

    return(t);
}

/*------------------------------------------------------------------------------------*/

int make_next_best_move(ex, trailsCompleted)
    expn *ex;
    trlist **trailsCompleted;
{
    /* make the next best move for this expansion.  This involves removing the best trail
     * from the priority queue, expanding from the end of that trail,  */
    tile *lasTile, *starTile, *nexTile;
    asg_arc *a;
    int done_count, undone_count, expnsToGo;
    tilist *tl, *tileSet = NULL;
    trail *tempTrail, *t = NULL, *trailToUse;
    trlist *activeTrails = NULL;
    expn *nextEx, *tempEx;
    expnlist *possibleTerminations = NULL, *secondaryTerminations, *exl;


    /* Get the cell to move from-- This is the best trail for the given expansion: */
    t = examine_best_trail_in_expn_group(ex);

    if ((t == NULL) && 
	((ex->connections == NULL) ||  
	 (list_length(ex->n->expns) - list_length(ex->n->done) < 2)))
    {
	t = restart_expansion(ex);
	if (t == NULL)
	{
	    fprintf(stderr, "ERROR:  <%s> \t{%s %s} CANCELLED - no more paths!!! \n",
		    ex->n->name, ex->t->name, ex->t->mod->name);
	    /* exit(1); */
	    return -1;
	}
	else
	{
	    fprintf(stderr, "WARNING: <%s> \t{%s %s} LOST - being restarted!!! \n",
		    ex->n->name, ex->t->name, ex->t->mod->name);
	    /* Treat this as an error condition and abort */
	    /* (previously, routine continued, but is in infinite loop at this point) */
	    /* return -1; */
	}
    }

    else if (t != NULL)
    {
	ex = t->ex;
	t = extract_best_trail_in_expn_group(ex);
	
	ex->eCount += 1;
	starTile = t->tr->this;
	lasTile = (t->tr->next != NULL) ? t->tr->next->this : t->tr->this;
	a = (asg_arc *)starTile->this;

	/* Check to insure that this trail is not already 'Completed' : */
	possibleTerminations = check_for_termination(t);    

	if (possibleTerminations == NULL)    
	{
	    /* Continue expanding */
	    if (debug_level == 5)
	    fprintf(stderr, "**<%s> \t{%s %s} visiting (%d, %d)%s [%d]\n",
		    ex->n->name, ex->t->name, ex->t->mod->name,
		    (a->page == HORZ) ? lasTile->y_pos + lasTile->y_len/2 :
		    starTile->y_pos + starTile->y_len/2,
		    (a->page == HORZ) ? starTile->y_pos + starTile->y_len/2 :
		    lasTile->y_pos + lasTile->y_len/2,
		    (a->page == VERT) ? "v" : "h",
		    t->cost);    
	    
	    /* Collect the set of tiles that can be moved into:  */
	    tileSet = collect_tiles(starTile, routingRoot[t->direction][t->page]);  
	    tileSet = remove_tiles_on_circular_trail(ex, t, &tileSet);
	    
	    /* Take these tiles, and build them into trail structures that can be tracked 
	       through the system... Create a new trail for every tile on the list.  The 
	       old trail has the first element of <tileSet> added to it explicitly;  All 
	       of the other elements have copies of <t> made, and added to.  */
	    if (tileSet != NULL) 
	    {
		for (tl = tileSet->next; tl != NULL; tl = tl->next)
		{
		    tempTrail = copy_trail(t);
		    add_to_trail (tempTrail, tl->this);  /* !Does bookeeping for tile str.!*/
		    insert_trailE(tempTrail, ex);        /* Add to expn hash table */
		    push(tempTrail, &activeTrails);
		    
		    a = (asg_arc *)tl->this->this;
		    if (debug_level == 5)
		    fprintf(stderr, "<%s> \t{%s %s} expanding to (%d, %d)%s [%d] trail==%x\n",
			    ex->n->name, ex->t->name, ex->t->mod->name,
			    (a->page == HORZ) ? starTile->y_pos + starTile->y_len/2 :
			    tl->this->y_pos + tl->this->y_len/2,
			    (a->page == HORZ) ? tl->this->y_pos + tl->this->y_len/2 :
			    starTile->y_pos + starTile->y_len/2,
			    (a->page == VERT) ? "v" : "h",
			    tempTrail->cost, (int)tempTrail);
		}
		
		for (tl = t->tr; tl != NULL; tl = tl->next) 
		{   
		    remove_trailA(t, (asg_arc *)tl->this->this );  /* Pull from arc hash table */
		}
		add_to_trail(t, tileSet->this);		/* Do bookeeping for tile str. */
		insert_trailE(t, ex);		             /* Add to expn hash table */
		push(t, &activeTrails);
		
		a = (asg_arc *)tileSet->this->this;
		if (debug_level == 5)
		fprintf(stderr, "<%s> \t{%s %s} expanding to (%d, %d)%s [%d] trail==%x\n",
			ex->n->name, ex->t->name, ex->t->mod->name,
			(a->page == HORZ) ? starTile->y_pos + starTile->y_len/2 :
			tileSet->this->y_pos + tileSet->this->y_len/2,
			(a->page == HORZ) ? tileSet->this->y_pos + tileSet->this->y_len/2 :
			starTile->y_pos + starTile->y_len/2,
			(a->page == VERT) ? "v" : "h",
			t->cost, (int)t);	
		free_list(tileSet); 
	    }

	    else 	/* Got a problem.... */
	    {  
		/* Let the expansion spin;  This is an infinite loop,  which can only be 
		   terminated by having another expansion terminate. */
		/* However, this permision should only occur iff the current expansion 
		   <ex> is NOT part of a terminated series (ie: his ->siblings field is
		   of length 2 or more & this trail has a valid ->jEx field). 	*/
		if ((list_length(*trailsCompleted) > 1) && /* Not a pathological case */
		    (t->jEx != NULL) &&    	           /* This trail has completed */
		    (list_length(ex->siblings) >= 2) &&	   /* somewhere AND */
		    (list_length(ex->n->expns) - 	   /* There is some other expn */
		     list_length(ex->n->done) >= 2))	   /* waiting to complete... */
		{
		    insert_trailE(t, t->ex);        /* Add it back to the hash table so 
						       others can find it... */
		    t->ex->eCount = FALSE;	    /* Mark it */
		    if (debug_level == 4) 
		    {
			fprintf(stderr, "<%s> \t{%s %s} spinning on trail %x.\n",
				ex->n->name, ex->t->name, ex->t->mod->name, t);
		    }
		    /* This needs a clause to catch the completley pathological case. */
		}
		else 	    
		/* Since you couldn't expand this trail any more, simply delete it: */
		{
		    delete_trail(t);
		}
	    }

	    if ((debug_level == 1) && 
		(net_name_located_p(net_under_study, ex->n) == TRUE))
	    {
		a->count += 1;
		a->local = t->cost;
	    }
	    return 0;
	}
	else /* There is at least one possbile termination */	    
	{
	    insert_trailE(t, t->ex);     /* Add it back to the hash table so others 
					    can find it... */
	    
	    for (exl = possibleTerminations; exl != NULL; exl = exl->next) 
/*	    exl = possibleTerminations;
	    if (exl != NULL) */
	    {
		nextEx = exl->this;
		
		if (debug_level == 5)
		{
		    fprintf(stderr, "**<%s> \t{%s %s} may terminate w/ {%s %s} in (%d, %d)%s [%d]\n",
			    ex->n->name, ex->t->name, ex->t->mod->name,
			    nextEx->t->name, nextEx->t->mod->name,
			    (a->page == HORZ) ? lasTile->y_pos + lasTile->y_len/2 : 
			    starTile->y_pos + starTile->y_len/2,
			    (a->page == HORZ) ? starTile->y_pos + starTile->y_len/2 : 
			    lasTile->y_pos + lasTile->y_len/2,
			    (a->page == VERT) ? "v" : "h",
			    t->cost);    
		}
		
		/* Now check to see if this trail is better than the trails in <nextEx>;
		   If it is, then go ahead and terminate it;  Otherwise, leave it in 
		   the hash table (to be found again, and the whole process to repeat) 
		   until either t->cost < min-cost {all trails in <nextEx>} OR <ex> 
		   is terminated by another expansion.  */
	    
		tempTrail = examine_best_trail_in_expn_group(nextEx);
		if ((tempTrail == NULL) || (tempTrail->ex->eCount == FALSE) ||
		    (t->cost <= tempTrail->cost)) 
		pushnew(t, trailsCompleted);
		else
		{
		    secondaryTerminations = check_for_termination(tempTrail);
		    if (member_p(t->ex, secondaryTerminations, identity) == TRUE)
		    {
			if (t->cost < tempTrail->cost) push(t, trailsCompleted);
			else push(tempTrail, trailsCompleted);
		    }
		    free_list(secondaryTerminations);
		}
	    }
	    free_list(possibleTerminations);		/* Collect Garbage */

	    return 0;
	}
    }
    return 0;
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

int unique_trail(t, uniqueTraiList)
    trail *t;
    trlist **uniqueTraiList;
{
    /* Return true, and add <t> to <uniqueTraiList> iff <t> is unique from all other
       trails in <uniqueTraiList>.  Otherwise, return FALSE.  */

    trlist *trl;
    tilist *tl, *tll;
    int mayBeDuplicate = FALSE;

    for(trl = *uniqueTraiList; trl != NULL; trl = trl->next)
    {
	for(tl = trl->this->tr; tl != NULL; tl = tl->next)
	{
	    for(tll = t->tr; tll != NULL; tll = tll->next)
	    {
		if (tll->this != tl->this) 
		{
		    push(t, uniqueTraiList);
		    tl = tll = NULL;		/* Break */
		}
		else mayBeDuplicate = TRUE;
	    }
	}
	if (mayBeDuplicate == TRUE) return(FALSE);
    }
    return(TRUE);
}

/*-------------------------------------------------------------------------------------*/

int terminal_spacer(t, axis)
    term *t;
    int axis;
{
    /* This returns the margin along the given axis that should be added to the
       given terminal.  Typically 1 or -1. */

    int margin = 0;
    if (axis == X)
    {
	if (t->side == RIGHT) margin = TERM_MARGIN;
	else if (t->side == LEFT) margin = -TERM_MARGIN;
    }
    else if (axis == Y)
    {
	if (t->side == UP) margin = TERM_MARGIN;
	else if (t->side == DOWN) margin = -TERM_MARGIN;
    }
    return (margin);
}

/*------------------------------------------------------------------------------------*/

tile *locate_first_tile(side, x_pos, y_pos, page)
    int side, x_pos, y_pos, page;
{
    tile *hTile = NULL;

    if ((side == LEFT) || (side == RIGHT))	/* IN or OUT terminals... */
    {
	hTile = locate(routingRoot[HORZ][page], x_pos, y_pos);
	if (!hTile || (hTile->y_pos + hTile->y_len == y_pos)) {
	    hTile = locate(hTile, x_pos, y_pos + 1);
	}
	if (!hTile) {
	    /* Maybe type inout without being on the left or right? */
	    hTile = locate(routingRoot[VERT][page], y_pos, x_pos);
	}
    }
    else 	/* For inout terminals.... */
    {
	hTile = locate(routingRoot[VERT][page], y_pos, x_pos);
    }
    
    return(hTile);
}

/*------------------------------------------------------------------------------------*/

tile *initial_move(ex, page)
    expn *ex;
    int page;
{
    /* Return the horizontal Tile in which to start a move */
    /* This takes an expansion and moves it into the htile that the terminal 
       must be routed through.  NOTE:  All first moves occur in the hspace.    This 
       may be incorrect;  If a terminal starts on the UP/DOWN side, perhaps they 
       should start in the vspace.      */
    term *t = ex->t;
    module *m = t->mod;

    int x_pos = t->x_pos + m->x_pos + terminal_spacer(t, X);
    int y_pos = t->y_pos + m->y_pos + terminal_spacer(t, Y);
    tile *hTile = locate_first_tile(t->side, x_pos, y_pos, page);

    return(hTile);
}

/*------------------------------------------------------------------------------------*/

trail *make_startup_trail(ex, starTile, page)
    expn *ex;
    tile *starTile;
    int page;
{
    trail *tr;

    if ((ex->t->side == LEFT) || (ex->t->side == RIGHT))    /* IN or OUT terminals... */
    {
	tr = create_trail(starTile, ex, page, VERT);
    }
    else 	/* For INOUT terminals.... */
    {
	tr = create_trail(starTile, ex, page, HORZ);
    }

    tr->cost = trail_cost(tr);
    
    return(tr);
}    

/*------------------------------------------------------------------------------------*/

trail *old_make_first_move(ex, page)
    expn *ex;
    int page;
{
    /* This takes an expansion and moves it into the htile that the terminal 
       must be routed through.  NOTE:  All first moves occur in the hspace.    This 
       may be incorrect;  If a terminal starts on the UP/DOWN side, perhaps they 
       should start in the vspace.      */
    term *t = ex->t;
    module *m = t->mod;

    int x_pos = t->x_pos + m->x_pos + terminal_spacer(t, X);
    int y_pos = t->y_pos + m->y_pos + terminal_spacer(t, Y);
    tile *hTile = locate_first_tile(t->side, x_pos, y_pos, page);
    trail *tr;

    if ((t->side == LEFT) || (t->side == RIGHT))	/* IN or OUT terminals... */
    {
	tr = create_trail(hTile, ex, page, VERT);
    }
    else 	/* For inout terminals.... */
    {
	tr = create_trail(hTile, ex, page, HORZ);
    }

    tr->cost = trail_cost(tr);
    
    return(tr);
}    

/*------------------------------------------------------------------------------------*/

expn *make_first_moves(ActiveExpns, AllExpns, p)
    expnlist **ActiveExpns;	/* source list for active expansions */
    expnlist *AllExpns;
    int p;		/* partition being worked on */
{
    expnlist *exl;
    expn *ex;
    tile *ti;
    trail *t;
    asg_arc *a;
    
    /* make the first (required) moves for this set of expansions.  */
    for (exl = AllExpns; exl != NULL; exl = exl->next)
    {
	ex = exl->this;
	ex->eCount += 1;		/* Keep Statistics */

	t = make_startup_trail(ex, initial_move(ex, p), p);
	if (t->tr == NULL) {
	    error("make_first_moves: trail has NULL component", "");
	    break;	/* Something has gone wrong. . . */
	}
	a = (asg_arc *)t->tr->this->this;
	insert_trailA(t, a);  
	

	if (check_for_AND_terminate_expn(t, ActiveExpns) == FALSE)
	{
	    insert_trailE(t, ex);
	}

	if ((debug_level == 1) && 
	    (net_name_located_p(net_under_study, ex->n) == TRUE))
	{
	    a->count += 1;
	}
    }
}    

/*------------------------------------------------------------------------------------*/

int trail_cost_order(t1, t2)
    trail *t1, *t2;
{
    if (t1 == NULL) return(FALSE);		/* Not good... */
    else if (t2 == NULL) return(TRUE);		/* Not good... */
    else if (t1->cost <=  t2->cost) return(TRUE);
    else return(FALSE);
}

/*------------------------------------------------------------------------------------*/

int in_trailCost_order(e1, e2)
    expn *e1, *e2;
{
    trail *t1 = examine_best_trail_in_expn_group(e1);
    trail *t2 = examine_best_trail_in_expn_group(e2);

    if (t1 == NULL) return(FALSE);		/* Not good... */
    else if (t2 == NULL) return(TRUE);		/* Not good... */
    else if (t1->cost <=  t2->cost) return(TRUE);
    else return(FALSE);
}

/*------------------------------------------------------------------------------------*/

int move_expns(expnsToMove)
    expnlist **expnsToMove;
{
    /* this function performs a breadth-first Lee-type weighted, multi-source maze 
       route to arrange the given <nets> within the routing space.  <expns> is a 
       list of source terminals/expansions, such that there is one terminal/expansion
       listed per net. */

    /* As this may well be operating on a partial schematic, only those terminals 
       listed are acted upon; In this same way, expansion information is maintained, 
       even as sections of the route are completed, and ranges are placed in the 
       appropriate arcs(tiles).  */

    expn *jEx;
    expnlist *exl, *etemp, *terminationCandidates;
    trail *t, *temp;
    trlist *trl, *ttemp, *trailsCompleted = NULL;

    while(*expnsToMove != NULL)
    {
	/* have all expansions (there is one for every terminal) made the least-expensive
	   (upward?) expansions: 
	   - Do not enter an area to which this terminal has already expanded.   */

	/* Move all active expansion groups one step... */
	for (exl = (expnlist *)sort_list(*expnsToMove, in_trailCost_order); 
	     exl != NULL; exl = exl->next)
	{
	    if (make_next_best_move(exl->this, &trailsCompleted) < 0) {
	       error("move_expns:  make_next_best_move returned error condition");
	       return -1;
	    }
	}

	/* check all expansion groups for terminations: */
	trl = (trlist *)sort_list(trailsCompleted, trail_cost_order);
	while (trl != NULL)
	{	    

	    ttemp = trl->next;
	    t = trl->this;

	    terminationCandidates = check_for_termination(t);
	    exl = terminationCandidates;
	    while (exl != NULL)
	    {
		etemp = exl->next;
		jEx = exl->this;/* Still to be terminated? */		
		if (jEx != NULL)/* Other terminations may have superceded this one... */
		{
		    temp = examine_best_trail_in_expn_group(t->ex);
		    if (temp == t)
		    {
			remove_trailE(t, t->ex);
			terminate_expansion(t->ex, jEx, t->tr->this->this, t, expnsToMove);
		    }
		    free_list(terminationCandidates);
		    /* Break added by Tim (9/24/03);		*/
		    /* otherwise, exl is invalid on next loop	*/
		    /* break; */
		}
		else
		{
		    jEx = jEx;		/* NOP */
		}
		exl = etemp;
	    }
	    trailsCompleted = (trlist *)free_list(trailsCompleted); /* Collect garbage */
	    trl = ttemp;
	}
    }
    return 0;
}

/*------------------------------------------------------------------------------------*/

int first_cut(expns, part)
    expnlist *expns;
    int part;
{
    expnlist *exl, *ExpCopy = (expnlist *)rcopy_list(expns);
    /* <ExpCopy> is the list of expansions that have not terminated. Its length 
       corresponds to <expCount>.  */

    
    /* Make the first set of expansions, checking them to see if there are any 
       expansions that are immediately terminated: */
    make_first_moves(&ExpCopy, expns, part);	

    return move_expns(&ExpCopy);
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

asg_arc *Nth(modTlist, n)
    tilist *modTlist;
    int n;
{
    tile *t = (tile *)nth(modTlist, n);
    return((asg_arc *)t->this);
}

/*------------------------------------------------------------------------------------*/

int filled_p(a)
    asg_arc *a;
{
    /* returns TRUE if the given asg_arc is filled */
    if (a->mod == NULL) return(FALSE);
    else return(TRUE);
}

/*------------------------------------------------------------------------------------*/

void set_arc_costs(netsToSet)
    nlist *netsToSet;
{
    /* This walks through the connection trails for each of the given nets,
       painting the tiles traversed with an estimate of their usage(congestion). */

    expn *ex;
    nlist *nl;
    trail *bt;
    trlist *trl;
    tilist *tl;
    asg_arc *a;

    for (nl = netsToSet; nl!=NULL; nl = nl->next)
    {
	ex = (expn *)nl->this->done->this;
	for (trl = ex->connections; trl != NULL; trl = trl->next)
	{
	    bt = trl->this;
	    for (tl = bt->tr; tl != NULL; tl = tl->next)
	    {
		/* Paint each arc with the usage - assume VERT tiles essentially have 
		   only the vertical tracks employed, and HORZ tiles the horizontal 
		   tracks. */
		a =(asg_arc *)tl->this->this;
		pushnew(nl->this, &a->nets);		/* Mark the global route used */
		(*set_congestion_value)(a);  
	    }
	}
    }
}

/*------------------------------------------------------------------------------------*/

return_best_trails_to_arcs(n,expnsToReroute, part)
    net *n;
    expnlist *expnsToReroute;
    int part;		/* Not used */
{
    /* This returns the best trails contained in <n> to the appropriate hash tables. */
    asg_arc *ar;
    expnlist *exl, *sibList;
    expn *ex, *temp;
    trail *t;
    trlist *trl;
    tilist *tl, *affectedTiles = NULL;
        
    if (n->done == NULL) return;	/* Nothing to return */

    ex = (expn *)n->done->this;

    /* return the 'best routes' to the expansion hash tables for the next go-round 
       It is important to create an extra trail for to insure that all of the known
       (especially the implicitly-known) complete trails are available to be found.  */
    for (trl = ex->connections; trl != NULL; trl = trl->next)
    {
	insert_trailE(trl->this, trl->this->ex);
	for (tl = trl->this->tr; tl != NULL; tl = tl->next)
	{
	    ar = (asg_arc *)tl->this->this;
	    rem_item(n, &ar->nets);
	}
    }
    ex->connections = (trlist *)free_list(ex->connections);

    /* Now reset all of the information in the expansion structures: */
    sibList = ex->siblings;

    for (exl = sibList; exl != NULL; exl = exl->next)
    {
	ex = exl->this;

	/* Clean up the parent net: */
	rem_item(ex, &n->done);
	ex->siblings = NULL;
	push(ex, &ex->siblings);
	ex->connections = NULL;
	ex->eCount = 0;		     /* Start the visitation count over */

	rehashE(ex);
    }
    free_list(sibList);

    /* Reorder the hash tables for those arcs that have had things added back to them */
    for (tl = ex->seen; tl != NULL; tl = tl->next)
    {
	rehashA(tl->this->this, ex);
    }
}

/*------------------------------------------------------------------------------------*/

reset_all_trail_costs(nets, part)
    nlist *nets;
    int part;
{
    /* This resets the trail costs for the entire seasg_arch space. */
    asg_arc *ar;
    nlist *nl;
    expnlist *exl;
    expn *ex;
    trlist *trl;
    tilist *tl, *affectedTiles = NULL;

    /* Reorder the hash tables for all arcs in the routing space */

    affectedTiles = allspace(routingRoot[VERT][part], &affectedTiles);
    for (tl = affectedTiles; tl != NULL; tl = tl->next)
    {
	ar = (asg_arc *)tl->this->this;
	full_rehashA(ar);
    }
    affectedTiles = (tilist *)free_list(affectedTiles);

    affectedTiles = allspace(routingRoot[HORZ][part], &affectedTiles);
    for (tl = affectedTiles; tl != NULL; tl = tl->next)
    {
	full_rehashA(tl->this->this);
    }
    affectedTiles = (tilist *)free_list(affectedTiles);	

    for (nl = nets; nl != NULL; nl = nl->next)
    {
	ex = (expn *)nl->this->done->this;

	/* For each net, */
	/* Rehash the expansion tables and the current (active) trail: */
	for (exl = ex->siblings; exl != NULL; exl = exl->next)
	{
	    rehashE(ex);
	}
	
	/* Recost the trails in the net structure: (probably pointless, as these get
	   added back to the arcs again later...) */
	for (trl = ex->connections; trl != NULL; trl = trl->next)
	{
	    trl->this->cost = trail_cost(trl->this);
	}
    }
}

/*------------------------------------------------------------------------------------*/

improve_route(n, part)
    net *n;
    int part;
{
    /* This function takes the most best arcs on the net, returns them to the hash table
       (rips them up) and tries again after rehashing the tables.  This causes a 
       rehash on all of the expansion tables for this net, and for all of the tiles
       traversed by the trails returned to an `unused' status (see `return_best_trails_
       to_arcs' above).  */

    expnlist *expns_to_reroute = (expnlist *)rcopy_list(n->done);
        
    /* List the expansion nodes from these affected nets: (Controls the re-route) */
    /* Then rip out the nets from the page :*/

    /* Only expansions belonging to <n> will be re-routed (and hopefully improved...)*/

    n->rflag = RIPPED;

    return_best_trails_to_arcs(n, expns_to_reroute, part);

    /* Reroute the expansions: (This means, continue where the routing left off...*/
    move_expns(&expns_to_reroute);
    free_list(expns_to_reroute);
}

/*------------------------------------------------------------------------------------*/

allow_for_further_routing(activeNets, part)
    nlist *activeNets;
    int part;				/* Not Used */
{
    /* Trash all of the existing trails, then return all of the completed routes 
       belonging to <activeNets>.  Arrange the data structures so that 
       the insertion and routing algorithms can be rerun */

    nlist *nl;
    expnlist *exl;

    for (nl = activeNets; nl != NULL; nl = nl->next)
    {
	/* Blow away the extraneous Global Routing stuff... */
	for (exl = (expnlist *)nl->this->expns; exl != NULL; exl = exl->next)
	{
	    kill_hashE(exl->this);		/* Blam! */
	}

	/* Blow away the Local Routing stuff... */
	if (nl->this->done != NULL)
	{
	    delete_all_corners(nl->this->route); 
	    nl->this->route = NULL;
	}
 
	return_best_trails_to_arcs(nl->this, part);
	nl->this->rflag = NULL;

	if (debug_level > 2)
	{
	    fprintf(stderr,"...net<%s> reset", nl->this->name);
	}
    }
    if (debug_level > 2)
    {
	fprintf(stderr,"...done\n");
    }
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

int no_terminals(n)
    net *n;
{
    /* This fuction determines if the given net has no terminals (Dummy Net) */
    if (n->terms == NULL) return(TRUE);
    else return(FALSE);
}

/*------------------------------------------------------------------------------------*/

int been_rerouted(n)
    net *n;
{
    /* This fuction determines if the given net has been rerouted. */
    return((n->rflag == RIPPED));
}

/*------------------------------------------------------------------------------------*/

asg_arc *worst_arc_on_net(n)
    net *n;
{
    /* This returns the most congested arc on the current route path of the given net */

    expn *ex;
    asg_arc *a, *worstArc = NULL;
    int congestionLevel = 0;
    trail *besTrail;
    trlist *trl;
    tilist *tl;

    ex = (expn *)n->expns->this;
    for (trl = ex->connections; trl != NULL; trl = trl->next)
    {
	/* Pick the best trail, so as to trace along it: */
	besTrail = trl->this;

	/* Walk back along the trail, looking for the most congested arc: */
	for (tl = besTrail->tr; tl != NULL; tl = tl->next)
	{
	    a = (asg_arc *)tl->this->this;
	    if (congestion_cost(a) >= congestionLevel)
	    {
		congestionLevel = congestion_cost(a);
		worstArc = a;
	    }
	}
    }
    return(worstArc);
}

/*------------------------------------------------------------------------------------*/

nlist *non_ripped_up_nets(nl)
    nlist *nl;
{
    /* return a list of non-ripped-up nets.  (Unsorted) */
    nlist *nll = (nlist *)copy_list(nl);
    return((nlist *)delete_if(&nll, been_rerouted));
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

void ps_print_arc(f, t)
    FILE *f;
    tile *t;
{
    /* This function prints the contents of the given arc in PostScript
       format to file <F>. */
    asg_arc *a = (arc *)t->this;
    int x1, x2, y1, y2, count = 0;
    nlist *nl;
    net *n;
    rnglist *rl;

    float gray_level = 1.0 - (float)list_length(a->nets) / 
                                  (2.5 * (float)list_length(nets));
    
    x1 = t->x_pos;
    y1 = t->y_pos;
    x2 = x1 + t->x_len;
    y2 = y1 + t->y_len;

    if (a->mod == NULL)
    {
	/* Fill in the area with gray, shaded to show the number of nets contained */
	fprintf(f, "newpath %d %d moveto %d %d lineto ", x1, y1, x2, y1);
	fprintf(f, "%d %d lineto %d %d lineto ", x2, y2, x1, y2);
	fprintf(f, "closepath %.3f setgray fill 0 setgray\n", gray_level);

	y2 -= 1;		/* <y2> becomes the y pos to print at, <x1> the x pos.*/
	x1 += 1;
	    
	/* Type in the net names: */
	for (nl = a->nets; nl != NULL; nl = nl->next)
	{
	    n = nl->this;			/* Only print the net_under_study */
	    if (net_name_located_p(net_under_study, n) == TRUE) 
	    {
		if ((x1 + strlen(n->name)) <= x2) 
		{
		    ps_put_label(f, n->name, (float)(x1), (float)(y2));
		    x1 += strlen(n->name) + 2;
		}
		else if (y2 >= y1)	
		{
		    y2 -= 1;
		    x1 = t->x_pos + 1;
		    ps_put_label(f, n->name, (float)(x1), (float)(y2));
		    x1 += strlen(n->name) + 2;
		}
	    }
	}
    }
    
    else 	/* Print out the module icon */
    {
	ps_print_mod(f, a->mod);
    }
}

/*------------------------------------------------------------------------------------*/

int check_terminal_p(ter, ex)
    expn *ex;
    term *ter;
{
    if (ter == ex->t) return(TRUE);	
    else return(FALSE);
}

/*------------------------------------------------------------------------------------*/

map_terminals_into_active_expansions(ml, ActiveExpns)
    mlist *ml;				/* List of modules to expand on */
    expnlist **ActiveExpns;
{	
    /* This takes a list of modules and digs out all of the terminals contained therein.
       These terminals are then mapped into the appropriate points on the appropriate
       edge structures.  It is expected that the list of modules being routed is given

       Active expansions are the expansions associated with source (OUT|INOUT) terminals.
       Each active expansion is places on the <ActiveExpns> list for use in first_cut, 
       etc. all inactive expansions make their first moves, and then wait to be located 
       by the active expansions. (see first_cut, make next_best_move, etc..)	*/
    
    tilist *tl;		/* Dummy tile list */
    tlist *trl, *trll;		/* terminal list */
    term *ter;
    module *mod;	/* */
    mlist *mll;
    expn *ex;
    expnlist *oldExpns;
    
    asg_arc *tarc, *ArcUsed;
    
    for (mll = ml; mll != NULL; mll = mll->next)
    {
	/* Walk through the list of terminals in each tile... */
	mod = mll->this;
	tarc = (asg_arc *)mod->htile->this;	     /* As set in 1st loop in "map_into_arcs" */
     
	/* For each terminal belonging to the module, create the expansion structure 
	   associated with it, and add this structure to the list of active terminals. */
	for (trl = mod->terms; trl != NULL; trl = trl->next)
	{
	    ter = trl->this;

	    /* Only create expansions for terminals that do not already have
	       expansions created for them, and that have nets that can terminate
	       (more than just one one terminal on the net) */
	    if ((ter->nt != NULL) && (list_length(ter->nt->terms) > 1) &&
		    /* Not obvious... It is needed for partial routing of a schematic */
		(member_p(ter, ter->nt->terms, identity) == TRUE))
	    {
		/* If you've got one piece of a good net, get all of it */
		for(trll = ter->nt->terms; trll != NULL; trll = trll->next)
		{
		    if (member_p(trll->this, *ActiveExpns, check_terminal_p)== FALSE) 
		    {
			oldExpns = (expnlist *)member(trll->this, ter->nt->expns, 
						 check_terminal_p);
			if ((oldExpns == NULL) && 
			    (trll->this->nt != NULL)) /* Skip Specialized terminals */
			{
			    ex = create_expansion(trll->this);
			    /* Save all expansions as 'Active' ones: (terminals) */
			    push(ex, ActiveExpns);
			}
			/* Otherwise look for old expansions to put on the list */
			else
			{
			    pushnew(oldExpns->this, ActiveExpns);
			}
		    }
		}
	    }
	}
    }
}

/*------------------------------------------------------------------------------------*/

void set_file_and_net()
{
    /* This prompts the user and gets a filename to which the global route and expansion
       details are to be dumped.   This sets the globals <df> and <net_under_study>.  */

    strcpy(dfname, "asg_groute.info");
/*
    fprintf(stderr, "File to dump global route info to -> ");
    scanf("%s", dfname);	
*/
   
   if ((df = fopen(dfname, "w")) == NULL) 
    {
	fprintf(stderr,"can't open %s\n", dfname);
	/* exit(1); */
	return;
    }
    else fprintf(stderr,"%s, used for global routing dump file\n", dfname); 

    sprintf(stderr, "gnd");
/*
    fprintf(stderr, "Please name the net being traced: ");
    scanf("%s", net_under_study);
*/

    /* now start the dump file: */
    if (latex != TRUE) 
    {
	ps_print_standard_header(df);
	fprintf(df, "\n(%s: global route trace for net %s) %d %d %d %d init\n",
                getenv("USER"), net_under_study, xfloor, yfloor, x_sizes[0], y_sizes[0]);
    }
    else 
    {
	ps_print_standard_header(df);
	fprintf(df, "swidth setlinewidth\n/Courier findfont fwidth scalefont setfont\n");
	fprintf(df,"%%%%BoundingBox: %d %d %d %d\n",
		xfloor - CHANNEL_LENGTH, yfloor - CHANNEL_HEIGHT, 
		x_sizes[0] + CHANNEL_LENGTH, y_sizes[0] + CHANNEL_HEIGHT);
    }
}    

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

int net_cost(n)
    net *n;
{
    /* estimate the cost of the global route for this net: */
    expn *ex = (expn *)n->done->this;
    trail *besTrail;
    trlist *trl;
    int cost = 0;

    for (trl = ex->connections; trl != NULL; trl = trl->next)
    {
	besTrail = trl->this;
	cost += trail_cost(besTrail);
    }
    return (cost);
}

/*------------------------------------------------------------------------------------*/

int in_cost_order(n1, n2)
    net *n1, *n2;
{
    /* order the two nets - the more expensive net goes to the left */
    if (net_cost(n1) > net_cost(n2)) return(TRUE);
    else return(FALSE);
}

/*------------------------------------------------------------------------------------*/

void reset_net_structures(nl)
    nlist **nl;
{
    /* Remove any NULL Nets from the list.*/

    nlist *nll, *last;
    for (nll = *nl; nll != NULL; nll = nll->next)
    {
	if (nll->this->terms != NULL)
	{
	    /* Non-Null net: */
	    nll->this->rflag = NULL;
	}
	else rem_item(nll->this, nl);
    }
}

/*------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------*/

void create_routing_space(ml, p, xlen, ylen, xorg, yorg)
    mlist *ml;
    int p, xorg, yorg;
{
    /* This function creates the data structures used for global routing on the
       given page <p>.  This should be called only once, and prior to the call
       to "first_global_route". */

    expnlist *ActiveExpns = NULL;  /* list of all active terminals */
    tilist *allVSpace = NULL, *allHSpace = NULL;

    /* See 'BoundingBox' definition in "debug.c";  <routingRoot>s are globals */
    routingRoot[HORZ][p] =
             create_tile(xlen + 2 * CHANNEL_LENGTH + 2 * TERM_MARGIN, 
			 ylen + 2 * CHANNEL_HEIGHT + 2 * TERM_MARGIN, 
			 NULL, 
			 xorg - CHANNEL_LENGTH - TERM_MARGIN, 
			 yorg - CHANNEL_LENGTH - TERM_MARGIN);

    routingRoot[VERT][p] = 
            create_tile(ylen + 2 * CHANNEL_HEIGHT + 2 * TERM_MARGIN, 
			xlen + 2 * CHANNEL_LENGTH + 2 * TERM_MARGIN, 
			NULL, 
			yorg - CHANNEL_HEIGHT - TERM_MARGIN, 
			xorg - CHANNEL_LENGTH - TERM_MARGIN);  


    insert_all_modules(routingRoot[HORZ][p], routingRoot[VERT][p], ml);

    /* the next stage is to create the structures that are used to control this
     * stage of the routing process.  */
    allHSpace = allspace(routingRoot[HORZ][p], &allHSpace);
    allVSpace = allspace(routingRoot[VERT][p], &allVSpace);

    reform_tile_space(allHSpace, HORZ);
    reform_tile_space(allVSpace, VERT);
    
    /* Assign the global 'solidp' function for insertions, now that the tile-space 
       has been reformed */
    solidp = solidp_for_arcs;
    
    reset_net_structures(&nets);
}

/*------------------------------------------------------------------------------------*/

nlist *first_global_route(ml, p, onceOnly, congestionRule)	
    mlist *ml;
    int p, onceOnly, congestionRule;
{
    /* This function performs the global routing for the circuit.  This begins by 
       evaluating where the available routing space is. */

    /* This fn must be called at least once */

    /* This function is meant to serve in two modes: Either for creating a completed
       route on a completed routing space (non-incremental-mode:<onceOnly> == FALSE), 
       or for completing the first-pass global route for a route that will be 
       incrementally added to. (<onceOnly> == TRUE) */

    expnlist *expns = NULL, *exl;
    nlist *non_ripped = NULL, *nl, *nll, *ActiveNets = NULL;
    asg_arc *worstArc;

    /* First, set the congestion evaluation function... */
    select_congestion_rule(congestionRule);

    map_terminals_into_active_expansions(ml, &expns);    

    /* Collect the nets that are being dealt with: */
    for (exl = expns; exl != NULL; exl = exl->next)
    {
	pushnew(exl->this->n, &ActiveNets);
    }

    if (debug_level == 1) set_file_and_net();
    else if (debug_level == 10) prepare_tile_spaces(FALSE);
    else if (debug_level == 11) prepare_tile_spaces(TRUE);

    if (first_cut(expns, p) < 0) {
	free_list(expns);
        return NULL;
    }

    if (onceOnly == FALSE)
    {
	reset_all_trail_costs(ActiveNets, p);

	/* Remove any dummy terminals from the master netlist, as they can 
	   cause problems: */
	non_ripped = (stopAtFirstCut == TRUE) ? NULL :
	             (nlist *)delete_if(&ActiveNets, no_terminals); 
	
	if (debug_level >= 3)
	{
	    for (nl = non_ripped; nl != NULL; nl = nl->next)
	    fprintf(stderr,"cost for net <%s> is %d\n", 
		    nl->this->name, net_cost(nl->this));
	}
	if (debug_level >= 3) 
	{
	    fprintf(stderr, "..............First Cut at GR Completed..............\n");
	    fprintf(stderr, "Before Improvement...");
	    dump_net_costs(ActiveNets);
	}
	
	/* Iteratively improve the route until all nets have been relaid once. */
	for(nl = (nlist *)sort_list(non_ripped, in_cost_order); 
	    nl != NULL; nl = nl->next)
	{
	    if(debug_level >= 3)
	    {
		fprintf(stderr,"\nBefore improving net <%s>:\n", nl->this->name);
		exl = (expnlist *)nl->this->done;
		long_dump_trails(exl->this->connections);
	    }
	    
	    /* worstArc = worst_arc_on_net(nl->this); */
	    improve_route(nl->this, p);
	    set_arc_costs(ActiveNets);
	    
	    if (debug_level >= 3)
	    {
		fprintf(stderr, "\nAfter improving net %s:\n",
			nl->this->name);
		exl = (expnlist *)nl->this->done;
		long_dump_trails(exl->this->connections);
		dump_net_costs(ActiveNets);
	    }
	}
    }

    free_list(expns);

    return(ActiveNets);
}    

/*------------------------------------------------------------------------------------*/

nlist *incremental_global_route(newMods, modsSoFar, oldNets, p)
    mlist *newMods;			/* Modules to be added to diagram */
    mlist *modsSoFar;			/* Modules to form global route for */
    nlist *oldNets;
    int p;				/* Virtual Page No. to add them to */
{
    /* This function performs the global routing for the circuit.  This begins by 
       evaluating where the available routing space is. */

    expnlist *expns = NULL, *exl;
    nlist *non_ripped = NULL, *nl, *ActiveNets = NULL;
    tlist *tl;
    asg_arc *worstArc;

    allow_for_further_routing(oldNets, p);

    add_modules(newMods, p);
    map_terminals_into_active_expansions(modsSoFar, &expns);    

    /* Must collect ALL nets that are to be routed. */
    for (exl = expns; exl != NULL; exl = exl->next)
    {
	pushnew(exl->this->n, &ActiveNets);
    }

    first_cut(expns, p);

    reset_all_trail_costs(ActiveNets, p);

    if (debug_level >= 3) fprintf(stderr, "...First Cut at Incremental GR Completed.\n");

    /* Remove any dummy terminals from the master netlist, as they can cause problems: */
    non_ripped = (stopAtFirstCut == TRUE) ? NULL :
                 (nlist *)delete_if(&ActiveNets, no_terminals); 

    if(debug_level >= 3)
    {
	fprintf(stderr, "Before Improvement...");
	dump_net_costs(ActiveNets);
    }

    /* Iteratively improve the route until all nets have been relaid at least once, or
       no improvement is found. */
    for(nl = (nlist *)sort_list(non_ripped, in_cost_order); 
	nl != NULL; nl = nl->next)
    {
	if(debug_level >= 3)
	{
	    fprintf(stderr,"\nBefore improving net <%s>:\n", nl->this->name);
	    exl = (expnlist *)nl->this->done;
	    long_dump_trails(exl->this->connections);
	}

/*	worstArc = worst_arc_on_net(nl->this); */
	improve_route(nl->this, worstArc, p);
	set_arc_costs(ActiveNets);

	if(debug_level >= 3)
	{
	    fprintf(stderr,"\nAfter improving net <%s>:\n", nl->this->name);
	    exl = (expnlist *)nl->this->done;
	    long_dump_trails(exl->this->connections);
	    dump_net_costs(ActiveNets);
	}
    }
    
    free_list(oldNets);
    free_list(expns);

    return(ActiveNets);
}

/*------------------------------------------------------------------------------------*/

void print_global_route(f)
    FILE *f;
{
    /* for date and time */
    long time_loc;
    extern long int time();
    
   /* Define the bounding box for the post-script code: */
    fprintf(f,"%%%%BoundingBox: %d %d %d %d\n",
            xfloor - CHANNEL_LENGTH, yfloor - CHANNEL_HEIGHT, 
            x_sizes[0] + CHANNEL_LENGTH, y_sizes[0] + CHANNEL_HEIGHT);


    /* now dump the result */
    time_loc = time(0L);

    if (latex != TRUE) /* Not in latex mode */
    {
	ps_print_standard_header(f);
	fprintf(f, "\n(N2A ?? %s %s %s)\n",
                getenv("USER"), ctime(&time_loc),  
		(stopAtFirstCut == TRUE) ? "(first cut only)" : "with rip-up");

	fprintf(f, "(using flags-> rule #%d s:%d c:%d part:%d dir:%d)\n",
                partition_rule, max_partition_size, max_partition_conn, 
                partition_count, matrix_type);

	fprintf(f,"%d %d %d %d init\n", xfloor, yfloor, x_sizes[0], y_sizes[0]);
    }
    else /* This stuff replaces the init for latex mode */
    {
	ps_print_standard_header(f);
	fprintf(f, "swidth setlinewidth\n/Courier findfont fwidth scalefont setfont\n");
	fprintf(f,"%%%%BoundingBox: %d %d %d %d\n",
		xfloor - CHANNEL_LENGTH, yfloor - CHANNEL_HEIGHT, 
		x_sizes[0] + CHANNEL_LENGTH, y_sizes[0] + CHANNEL_HEIGHT);
    }

    ps_print_tile_space(routingRoot[HORZ][0], f, ps_print_arc, TRUE);

    if (latex != TRUE) fprintf(f, "showpage\n");
}


/*---------------------------------------------------------------------------------*/

dump_nets(nets)
    nlist *nets;
{
    nlist *nl;
    for(nl=nets; nl!=NULL; nl = nl->next)
    {
	fprintf(stderr, "<%s>...", nl->this->name);
    }
    fprintf(stderr, "\n");
}

/*---------------------------------------------------------------------------------*/

dump_net_costs(nets)
    nlist *nets;
{
    nlist *nl;
    for(nl=nets; nl!=NULL; nl = nl->next)
    {
	fprintf(stderr, "<%s>=%d...", nl->this->name, net_cost(nl->this));
    }
    fprintf(stderr, "\n");
}

/*---------------------------------------------------------------------------------*/

dump_modules(mods)
    mlist *mods;
{
    mlist *nl;
    for(nl=mods; nl!=NULL; nl = nl->next)
    {
	fprintf(stderr, "<%s>...", nl->this->name);
    }
    fprintf(stderr, "\n");
}

/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/

dump_tile(t)
    tile *t;
{
    asg_arc *a = (asg_arc *)t->this;
    if (a->page == HORZ)
    fprintf(stderr, "(%d,%d),(%d,%d)H...", 
	    t->x_pos, t->y_pos, t->x_pos + t->x_len, t->y_pos + t->y_len);
    else
    fprintf(stderr, "(%d,%d),(%d,%d)V...", 
	    t->y_pos, t->x_pos, t->y_pos + t->y_len, t->x_pos + t->x_len);
}

/*---------------------------------------------------------------------------------*/

dump_trail(t)
    trail *t;
{
    tilist *temp = NULL, *tl;

    if (t->jEx == NULL)
    {
	fprintf(stderr, "\tn<%s> t<%s>-> ", t->ex->t->nt->name, t->ex->t->mod->name);
    }
    else
    {
	fprintf(stderr, "\tn<%s> t<%s> & <%s>-> ", t->ex->t->nt->name, 
		t->ex->t->mod->name, t->jEx->t->mod->name);
    }
    temp = (tilist *)rcopy_list(t->tr);
    for (tl = temp; tl != NULL; tl = tl->next)
    {
	fprintf(stderr, "%x...", tl->this);
    }
    fprintf(stderr, "[%d]...", list_length(temp));
    
    free_list(temp);
}

/*------------------------------------------------------------------------------------*/

dump_tiles(tilesToSee)
    tilist *tilesToSee;
{
    tilist *tl;
    /* Dump Tile List to the screen: */
    for (tl = tilesToSee; tl != NULL; tl = tl->next)
    {	
	dump_tile(tl->this);
    }
    fprintf (stderr, "\n");
}

/*---------------------------------------------------------------------------------*/

long_dump_trail(t)
    trail *t;
{
    tilist *temp = NULL, *tl;

    if (t->jEx == NULL)
    {
	fprintf(stderr, "\tn<%s> t<%s>-> ", t->ex->t->nt->name, t->ex->t->mod->name);
    }
    else
    {
	fprintf(stderr, "\tn<%s> t<%s> & <%s>-> ", t->ex->t->nt->name, 
		t->ex->t->mod->name, t->jEx->t->mod->name);
    }
    temp = (tilist *)rcopy_list(t->tr);
    dump_tiles(temp);
    fprintf(stderr, "[%d]$%d...", list_length(temp), trail_cost(t));
   
    free_list(temp);
}

/*------------------------------------------------------------------------------------*/

long_dump_trails(trailsToSee)
    trlist *trailsToSee;
{
    trlist *trl;
    /* Dump Trail List to the screen: */
    for (trl = trailsToSee; trl != NULL; trl = trl->next)
    {	
	long_dump_trail(trl->this);
	fprintf (stderr, "\n");
    }
    fprintf (stderr, "\n");
}

/*------------------------------------------------------------------------------------*/

dump_trails(trailsToSee)
    trlist *trailsToSee;
{
    trlist *trl;
    /* Dump Trail List to the screen: */
    for (trl = trailsToSee; trl != NULL; trl = trl->next)
    {	
	dump_trail(trl->this);
	fprintf (stderr, "\n");
    }
    fprintf (stderr, "\n");
}

/*------------------------------------------------------------------------------------*/

dump_arcs(arcsToSee)
    arclist *arcsToSee;
{
    arclist *al;
    tile *t;
    /* Dump Arc List to the screen: */
    for (al = arcsToSee; al != NULL; al = al->next)
    {	
	t = al->this->t;
	if (al->this->page == HORZ)
	fprintf(stderr, "(%d,%d),(%d,%d)H...", 
		t->x_pos, t->y_pos, t->x_pos + t->x_len, t->y_pos + t->y_len);
	else
	fprintf(stderr, "(%d,%d),(%d,%d)V...", 
		t->y_pos, t->x_pos, t->y_pos + t->y_len, t->x_pos + t->x_len);
    }
    fprintf (stderr, "\n");
}

/*-------------------------------------------------------------------------------------*/

void prepare_tile_spaces(netFlag)
{
    strcpy(dfname, "asg_vtiles.info");
/*
    fprintf(stderr, "File to dump vertical tiles to -> ");
    scanf("%s", dfname);
*/
    if ((vf = fopen(dfname, "w")) == NULL) 
    {
	fprintf(stderr,"Sorry, can't open %s\n", dfname);
	/* exit(1); */
	return;
    }

    strcpy(dfname, "asg_htiles.info");
/*
    fprintf(stderr, "File to dump horizontal tiles to -> ");
    scanf("%s", dfname);    
*/
    if ((hf = fopen(dfname, "w")) == NULL) 
    {
	fprintf(stderr,"Sorry, can't open %s\n", dfname);
	/* exit(1); */
	return;
    }

    if (netFlag == TRUE) 
    {
	/* Get the net name to study: */
        strcpy(net_under_study, "gnd");
/*
	fprintf(stderr, "Please name the net being traced: ");
	scanf("%s", net_under_study);
*/
    }
}    

/*------------------------------------------------------------------------------------*/
/* END OF FILE "global_route.c"  */
