#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "list.h"
#include "var.h"
#include "cstitch.h"

/* 
 * Definition of the Design
 * M is the set of all modules
 * N is the set of all nets
 * ST the set of system terminals
 * T the set of sub-system terminals
 * terms(module) : returns the subset of T for that module
 * type(terminal(tUst)): returns its type (in, out, inout)
 * postion-terminal(terminal(t)) : returns its (x,y) position
 * net(terminal(tUst)) : returns the Net for this terminal
 * size(module) : returns its x,y size
 * side(terminal(t)) : returns (left, right, up, down)
 */


#define STF(x) \
(((x) == FALSE) ? "FALSE" : "TRUE")

/* only extract_tree in place.c uses these two: */
#define CHILDOK 2
#define CHILDNOTOK 3
#define LEAF 2
#define CLIPPED 4

/* used in most files, but also defined in net.h */
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

/* I think the only hack that counts on these values is in 
 * utility.c rot_side().  Everyone else cases them out.
 */

/* mod->flag values, used in placement */
#define SEEN 1
#define UNSEEN 0
#define UNPLACED -1
#define PLACED 1

/* the type of connection matrix built may be */
#define FORWARD 1
#define REVERSE -1
#define UNSIGNED 0


/* sides of module, clockwise order */
#define LEFT 0
#define UP 1
#define RIGHT 2
#define DOWN 3

#define SSIDE(x) \
(((x) == LEFT) ? "LEFT":\
((x) == RIGHT) ? "RIGHT":\
((x) == UP) ? "UP" : "DOWN")

/* clockwise rotation amounts */
#define ZERO 0
#define NINETY 1
#define ONE_EIGHTY 2
#define TWO_SEVENTY 3

#define SROT(x) \
(((x) == ZERO) ? "ZERO":\
((x) == NINETY) ? "NINETY":\
((x) == ONE_EIGHTY) ? "ONE_EIGHTY" : "TWO_SEVENTY")


#define NONE 0	/* used with those below for system net types */

/* directions of terminals */
#define IN 1		/* Left side of icon */
#define OUT 2		/* Right side of icon */
#define INOUT 3		/* Bottom of icon */
#define OUTIN 7		/* Top of icon */
#define GND 4
#define VDD 5
#define DUMMY 6

#define SNTYPE(x) \
(((x) == IN) ? "IN":\
((x) == OUT) ? "OUT":\
((x) == INOUT) ? "INOUT" :\
((x) == OUTIN) ? "OUTIN" :"NONE")

#define ICON_SIZE 8		/* default object icon sizes are 8 x 8 for sced */
#define TERM_NAME_SIZE 32       /* size of strings for terminal names */
#define GATE_NAME_SIZE 10       /* Aesthetic limit on icon name sizes */
#define NET_NAME_SIZE 14        /* limit on net name sizes (fits _DUMMY_SIGNAL_) */

#define XCANVAS_SCALE_FACTOR 6	/* Used to scale XCanvas bitmaps (see -b flag) */

#define MAX_MOD	256	/* maximum number of modules */

#define CHANNEL_HEIGHT 48
#define CHANNEL_LENGTH 48
#define MAX_DEPTH 12	/* maximum length of module box string */
#define WHITE_SPACE 36 	/* multiplier * # terms on a side for routing space */
#define CC_SPACE 48 	/* min routing space between cross-coupled modules*/

/* 
 * Rents rule: SIZE = A |Connections| ** B with  .5 < A < 5 and .4 < B < .8 
 * I have no idea for good values.
 * Try A = 1 B = .5 and Size =5 Conn = 25 (note values below are -1)
 */

/* these are now just the default maxes, see the command line */
#define MAX_PARTITION_SIZE 4		/* 4 should we follow rent's rule? */
#define MAX_PARTITION_CONN 24		/* 24 connections from inside to outside */
#define DEF_PARTITION_RULE 3    	/* enable the -c and -s flags for partitioning */
#define DEF_PARTITION_RATIO 3.0 	/* sets the ratio used by rule number 2 */
#define CC_SEARCH_LEVEL 2		/* only look for modules that are cross 
					   coupled to two levels of removal */
#define DEF_TURN_MODE 0			/* .5 Turns */
#define TIGHT 1


/* offsets around things for routing */
#define TERM_SIZE 2
#define OUTSIDE_BORDER 100

/* default sizes for the virtual space tiles used for each partition: */
#define TILE_SPACE_X_LEN 15000
#define TILE_SPACE_Y_LEN 15000
#define TILE_SPACE_X_POS -5000
#define TILE_SPACE_Y_POS -5000


/* types of obstacles */
#define MODULE 0
#define NET 1
#define ACTIVE_A 2
#define ACTIVE_B 3

#define STYPE(x) \
(((x) == ACTIVE_A) ? "ACTIVE_A" :\
((x) == ACTIVE_B) ? "ACTIVE_B" :\
((x) == MODULE) ? "MODULE" : "NET")

/* for add_obst */
#define HORZ 0
#define VERT 1

#define SDIR(x) \
(((x) == HORZ) ? "HORZ" : "VERT")

/* for init_actives */
    /* signed sixteen bit max value */
#define INF 32767
    /* perhaps 0x7FFFFFFF (32 bit max value) would be better ?*/


/* for end segment types */
#define LEFT_C 0
#define RIGHT_C 1
#define SEG 2

#define SSTYPE(x) \
(((x) == LEFT_C) ? "LEFT_C" :\
((x) == RIGHT_C) ? "RIGHT_C" : "SEG")

/*- begin psfigs support ------------------------------------------------------*/
#define XNOR_GATE "XNOR"	/* Used to identify the Type of the module */
#define BUFFER_GATE "BUFFER"
#define INPUT_TERM "IN"
#define OUTPUT_TERM "OUT"
#define INOUT_TERM "INOUT"
#define OUTIN_TERM "OUTIN"
#define GENERIC_BLOCK "BLOCK"

/* The following icons are directly supported: 
   (See "validate" in "psfigs.c") */
#define NOT_ 1
#define AND 2
#define NAND 3
#define OR 4
#define NOR 5
#define XOR 6
#define XNOR 7
#define BUFFER 8
#define INPUT_SYM 9
#define OUTPUT_SYM 10
#define INOUT_SYM 11
#define BLOCK 12
#define INVNOT_ 13

#define DONT_KNOW 20

#define CONTACT_WIDTH .350
#define ICON_LEN 4
#define ICON_HEIGHT 6
#define CIRCLE_SIZE .75
#define ANDARC_RAD 3.0625
#define ANDARC_THETA 69.09
#define ANDARC_CENTER_X_OFFSET 3.854 - 4.0

#define ORARC_RAD 3.75
#define ORARC_THETA 53.13
#define ORARC_CENTER_X_OFFSET 2.75 - 8.0

#define XOR_OFFSET .5

#define BUFFER_SIZE 6.0
#define GATE_LENGTH 8
#define GATE_HEIGHT 6
#define SYSTERM_SIZE 6
#define BLOCK_SPACES_PER_PIN 3
#define SPACES_PER_PIN 2

/*- end psfigs support ------------------------------------------------------*/

  
typedef struct info_struct
{
    int in, out, used, order;
} info_type;


/*---------------------------------------------------------------
 * Templates for major linked list data structures.
 * We use a general paradigm of a linked list of "things"
 * the list has a "this" field and a "next" field
 * the "this" field is coerced into a pointer to a type "thing"
 * the "next" field is coerced into a pointer to a list of "things"
 * We then use generic list routines (in utility.c) to manipulate 
 * these.
 *---------------------------------------------------------------
 */

typedef struct term_struct term;	/* the terminals */
typedef struct mod_struct module;	/* the modules */
typedef struct net_struct net;		/* the nets */
typedef struct obst_struct obst;	/* the obstacles */
typedef struct act_struct act;		/* the active segments */
typedef struct end_struct end;		/* the endpoints of segments */
typedef struct rng_struct rng;		/* the ranges of segments */
typedef struct clu_struct cluster;      /* the clusters of modules */

/* typedef struct list_struct list;	Moved to 'list.h' */
typedef struct tnode_struct tnode;        /* for binary trees */
typedef struct term_list tlist;
typedef struct mod_list mlist;
typedef struct net_list nlist;
typedef struct obst_list olist;
typedef struct act_list alist;
typedef struct end_list elist;
typedef struct rng_list rlist;
typedef struct cl_list clist;
typedef struct cluster_tree ctree;

typedef struct link_list llist; 	/* for 2d linked lists */
/* typedef struct indexed_list ilist;	Moved to 'list.h'	*/


struct term_struct
/* terminals are where nets attach to modules */
{
    char *name;
    int x_pos;		/* we will eventually have to look this up */
    int y_pos;
    module *mod;
    int side; 		/* side of the module on which it apears */
    int type; 		/* direction in, out, inout, outin for this module */
    net *nt;   		/* the net that this terminal belongs to */
};

struct mod_struct
/* modules are the "gates" both simple and complex, rectangles are assumed */
{
    int index;		/* unique number */
    char *name;		/* unique instance name */
    char *type;   	/* gate types will be our key for icons */
    int dly_type;	/* inertial or transport -- for simulation */
    int delay;		/* the delay time -- for simulation */
    int x_pos;		/* we will determine this in "place" */
    int y_pos;		/* we will determine this in "place" */
    int x_size;		/* we will have to look this up */
    int y_size;		/* we will have to look this up */
    int flag;		/* flag for recursive marking */
    int placed;		/* marks that the critter has been placed within a partition */
    int rot;		/* rotation */
    tile *htile;	/* Tile in the horizontal routing space to which this module is assigned */
    tile *vtile;	/* Tile in the vertical routing space to which this module is assigned */
    term *primary_in;	/* the primary input terminal */
    term *primary_out;	/* the primary output terminal */
    tlist *terms;	/* the terminals on this module */
    var  *fx, *fy;     	/* Fuzzy X and Fuzzy Y positions for this module */ 
    
};

struct clu_struct
/* clusters are collections of gates */
{
    int index;		/* unique number */
    int size;		/* number of elements in branch */
    int connects;       /* number of connections external to the cluster */
    module *contents;	/* the module within this cluster (leaf nodes) */
};

struct net_struct
/* a net is a list of terminals, one electrical node */
{
    char *name;
    tlist *terms;
    int type;		/* identify system terminal nets */
    int rflag;		/* Routing status flag */
    list *expns;	/* Used in both local & global routing; */
    list *done;		/* Records expansions that are completed */
    list *route;	/* used in local routing */
};

struct obst_struct
/* an obstacle is a 5-tuple representing a line segment */
{
    int i;	/* index along the coordinate axis (vert or horz) */
    int x;	/* lower index in the other direction */
    int y;	/* upper index */
    int type;	/* one of MODULE, NET, ACTIVE_A or ACTIVE_B */
    char *name;	/* only used if it is a net */
};

struct act_struct
/* an active segment is a 10-tuple */
{
    int b;          /* wave number, number of bends in the path */
    int c;          /* number of crossed nets */
    act* prev;      /* pointer to previous/origninator of segment */
    int i;          /* description of this segment: index */
    int x;          /*  "" : lower index in other direction */
    int y;          /*  "" : upper index */
    int d;          /* one of LEFT RIGHT UP DOWN */
    int type;	    /* one of MODULE, NET, ACTIVE_A or ACTIVE_B */
    int expanded;   /* one of TRUE, FALSE */
};

struct end_struct
/* an end segment is a 4-tuple */
{
    int i;	/* description of this segment: index */
    int x;	/*  "" : lower index in the other direction */
    int y;	/*  "" : upper index */
    int type;	/* one of LEFT_C RIGHT_C SEG */
};

struct rng_struct
/* a range of a segment is a 3-tuple */
{
    int x;	/*  "" : lower index in the other direction */
    int y;	/*  "" : upper index */
    int c;	/*  number of crossed wires */
};

/*-----------------------------------------------------*/
struct tnode_struct
/* these are coerced to be binary trees of the sort below */
{
    int *this;
    tnode *left, *right;
};

struct cluster_tree
{
    cluster *this;
    ctree *left, *right;
};
	

/*-----------------------------------------------------*/
struct term_list
/*  */
{
    term *this;
    tlist *next;
};
struct mod_list
/*  */
{
    module *this;
    mlist *next;
};

struct cl_list
/* Used to maintain clusters as they are built into and taken from a tree. */
{
    int index;
    ctree *this;
    clist *next;
};

struct net_list
/*  */
{
    net *this;
    nlist *next;
};

struct obst_list
/*  */
{
    obst *this;
    olist *next;
};

struct act_list
/*  */
{
    act *this;
    alist *next;
};

struct end_list
/*  */
{
    end *this;
    elist *next;
};

struct rng_list
/*  */
{
    rng *this;
    rlist *next;
};

struct link_list
/*  this is different for 2 - d lists */
{
    int index;		/*  index */
    list *values;	/*  list  */
    llist *next;	/* next link */ 
};

/*---------------------------------------------------------------
 * END OF FILE
 *---------------------------------------------------------------
 */
