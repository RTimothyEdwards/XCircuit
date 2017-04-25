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
 *      file route.h
 * As part of MS Thesis Work, S T Frezza, UPitt Engineering
 * February, 1990
 *
 * This file contains the global and local routing structures used to perform the routing fns
 * in SPAR.  
 */

#include <math.h>
#include "externs.h"

#define RIPPED 1
#define HASH_SIZE 43

#define NO_SIDE -1
#define TERM_MARGIN 1
#define GRAY_MARGIN 0.05

#define TRACK_WEIGHT 30.0
#define LENGTH_WEIGHT 1.0
#define FORWARD_EST_MULTIPLIER 0.80
#define EXPECTED_CORNERS 3
#define CORNER_COST 4

#define TOP_EDGE 1
#define BOT_EDGE 2
#define LEFT_EDGE 4
#define RIGHT_EDGE 8

#define UPWARD_U 1
#define DOWNWARD_U 2
#define HORZ_JOG 3
#define LEFT_U 4
#define UL_CORNER 5
#define LL_CORNER 6
#define VERT_LT 7
#define RIGHT_U 8
#define UR_CORNER 9
#define LR_CORNER 10
#define VERT_RT 11
#define VERT_JOG 12
#define HORZ_UT 13
#define HORZ_DT 14
#define X_CORNER 15

#define EXPAND_BOTH 2
#define EXPAND_MIN 3
#define EXPAND_MAX 4
#define EXPAND_NONE 5

/* Structure Definitions and type definitions: */
typedef struct arc_struct 	asg_arc;
typedef struct arc_list_struct 	arclist;
typedef struct expansion_struct expn;
typedef struct expn_list_struct expnlist;
typedef struct trail_struct 	trail;
typedef struct trail_ilist_str  trailist;
typedef struct trail_list_str   trlist;
typedef struct range_struct 	range;
typedef struct range_list_str 	rnglist;
typedef struct corner_struct    corner;
typedef struct corner_list_str	crnlist;

struct arc_struct
/* structure to manage arcs in the global router */
{ 
    nlist *nets;	/* nets that use this arc for their GR */
    crnlist *corners;	/* local routing corners that currently use this arc */
    int congestion;	/* congestion level */
    int page;		/* Indicates which page this tile belongs to. */
    int vcap, hcap;	/* Vertical and Horizontal capacities (# of tracks) */
    tile *t;		/* tile associated with this arc */
    module *mod;        /* Used to hold the original contents of the tile */
    trailist *trails[HASH_SIZE];/* pointers to trails that use this tile */   
    int local, count;		/* used for stats */
};

struct expansion_struct
/* structure to manage multipoint, pseudo-similtaneous expansions within the 
 * arc/edge configuration.  One of these should be used for every terminal in
 * a given net.  */
{
    net *n;		/* Net to which the terminal belongs */
    term *t;		/* Terminal from which the expansions began */
    tilist *seen;       /* list of tiles that this expn has visited */
    trailist *queue[HASH_SIZE];	/* Hash table of trails, from which the 
				   next move is selected */
    expnlist *siblings;	/* (shared) list of expns that make up this expansion group */
    trlist *connections;/* (shared) list of trails that form complete paths */
    int eCount;		/* for statistics */
};

struct trail_struct
{
    /* This is a possible global route;  Only if *used is set are there 
       ranges associated with the trail. */
    int cost;
    expn *ex;		/* Parent expansion */
    expn *jEx;		/* Who was contacted at termination */
    tilist *tr;		/* ordered list of tiles traversed */
    int page, direction;/* array indecies to the root tile for the next expansion set */
    crnlist *used;	/* Pointer to the set of corners that 
			   comprise this trail (when constructed) */
};


struct trail_ilist_str	/* Indexed form */
{
    int index;		/* trail cost */
    trail *this;
    trailist *next;
};

struct trail_list_str 	/* Non-indexed form */
{
    trail *this;
    trlist *next;
};


struct expn_list_struct
{
    expn *this;
    expnlist *next;
};

struct arc_list_struct
{
    asg_arc *this;
    arclist *next;
};


/* The following are used to implement the local router: */
struct range_struct
{
    srange *sr;		    /* The min/max limits of this range. */
    int use, flag;
    net *n;		    /* The net that this range is a part of */
    crnlist *corners;	    /* Corners linked by this range */
    crnlist *dep;	    /* Corners who's placement this range is constrained by */
};

struct range_list_str
{
    range *this;
    rnglist *next;
};

struct corner_struct
{
    range *cx;
    range *cy;
    int vertOrder, horzOrder;
    arclist *al;
    term *t;
};

struct corner_list_str
{
    corner *this;
    crnlist *next;
};



/* Shared info between global and local router: */
/*--------------------------------------------------------------- 
 * Defined in global_route.c
 *---------------------------------------------------------------
 */
extern asg_arc *next_arc();
extern int net_using_edge();
extern int congestion_cost();
extern int decision_box[];
extern tilist *collect_tiles();

/*--------------------------------------------------------------- 
 * Defined in local_route.c
 *---------------------------------------------------------------
 */
extern corner *create_corner();
extern int in_x_order();	
extern int divide_sranges();
extern int overlapped_p();
