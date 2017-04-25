/************************************************************
**
**       COPYRIGHT (C) 2004 GANNON UNIVERSITY 
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


/* file netlist_structs.h */


#if defined(DARWIN)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif

#include <time.h>
#include <pwd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <unistd.h>   /* for unlink() */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#ifdef TCL_WRAPPER
#include <tk.h>
#else
#include "Xw/Xw.h"
#include "Xw/Form.h"
#include "Xw/WorkSpace.h"
#include "Xw/PButton.h"
#include "Xw/SText.h"
#include "Xw/Cascade.h"
#include "Xw/PopupMgr.h"
#include "Xw/MenuBtn.h"
#include "Xw/BBoard.h"
#include "Xw/TextEdit.h"
#include "Xw/Toggle.h"
#endif

#ifndef P_tmpdir
#define P_tmpdir TMPDIR
#endif

#ifdef TCL_WRAPPER
  #define malloc Tcl_Alloc
  #define calloc(a,b) my_calloc(a, b)		/* see utility.c */
  #define free(a) Tcl_Free((char *)(a))
  #define realloc(a,b) Tcl_Realloc((char *)(a), b)

  #define fprintf tcl_printf		/* defined by xcircuit */
  #define fflush tcl_stdflush
#endif


/*----------------------------------------------------------------------*/
/* Local includes							*/
/*----------------------------------------------------------------------*/

#include "xcircuit.h"

/*----------------------------------------------------------------------*/
/* Function prototype declarations                                      */
/*----------------------------------------------------------------------*/
#include "prototypes.h"
#include "list.h"
#include "var.h"
#include "cstitch.h"


/* 
 * Definition of the Design
 * M is the set of all modules
 * N is the set of all nets 
 *    SEE "netlistptr globallist" declared in "netlist.c"
 * ST the set of system ports (terminals)
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
#define BOTTOM 4

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
#define CC_SPACE 48  	/* min routing space between cross-coupled modules*/

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

/* Used in routing, tile definitions */
#define HORZ 0
#define VERT 1

/*- begin psfigs support ------------------------------------------------------*/
#define XNOR_GATE "XNOR"	/* Used to identify the Type of the module */
#define BUFFER_GATE "BUFFER"
#define INPUT_TERM "IN"
#define OUTPUT_TERM "OUT"
#define INOUT_TERM "INOUT"
#define OUTIN_TERM "OUTIN"
#define GENERIC_BLOCK "BLOCK"

 
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

typedef struct mod_struct module;	/* the modules */
typedef struct net_struct net;		/* the nets */
typedef struct rng_struct rng;		/* the ranges of segments */
typedef struct clu_struct cluster;      /* the clusters of modules */

/* typedef struct list_struct list;	Moved to 'list.h' */
typedef struct tnode_struct tnode;        /* for binary trees */
typedef struct mod_list mlist;
typedef struct net_list nlist;
typedef struct rng_list rlist;
typedef struct cl_list clist;
typedef struct cluster_tree ctree;

typedef struct link_list llist; 	/* for 2d linked lists */
/* typedef struct indexed_list ilist;	Moved to 'list.h'	*/

/* PORTS AND TERMINALS
/* Structures for managing the placement and management of ports 
/* ports AKA terminals */

/* List of object's I/O ports included from xcircuit.h:
/* typedef struct _Portlist *PortlistPtr;
/* typedef struct _Portlist
/* {
/*    int portid;
/*    int netid;
/*    PortlistPtr next;
/* } Portlist;
/* */

typedef struct term_struct term;	/* the terminals */
typedef struct term_list tlist;
struct term_struct
/* terminals are where nets attach to modules */
{
    char *name;		/* Label associated with this terminal */
    int x_pos;		/* we will eventually have to look this up */
    int y_pos;
    module *mod;	/* Module to which this terminal is connected */
    int side; 		/* side of the module on which the terminal apears */
/*  int type; 		/* direction in, out, inout, outin for this module */
    NetlistPtr nt;   	/* the XC net that this terminal belongs to */
    PortlistPtr pl;	/* the XC port that this terminal corresponds to */
};




struct mod_struct
/* modules are the "gates" both simple and complex, rectangles are assumed */
{
    int index;		/* unique number */
    char *name;		/* unique instance name */
    char *type;   	/* gate types will be our key for icons */
/*    int dly_type;	/* inertial or transport -- for simulation */
/*    int delay;		/* the delay time -- for simulation */
    int x_pos;		/* we will determine this in "place" */
    int y_pos;		/* we will determine this in "place" */
    int x_size;		/* we will have to look this up */
    int y_size;		/* we will have to look this up */
    int flag;		/* flag for recursive marking */
    int placed;		/* marks that the critter has been placed within a partition */
    int rot;		/* rotation */
    tile *htile;	/* Tile in the horizontal routing space to which this module is assigned */
    tile *vtile;	/* Tile in the vertical routing space to which this module is assigned */
/*  term *primary_in;	/* the primary input terminal */
/*  term *primary_out;	/* the primary output terminal */
    tlist *terms;	/* the terminals on this module */
/*  var  *fx, *fy;     	/* Fuzzy X and Fuzzy Y positions for this module */ 
    CalllistPtr XCcl; 	/* The XCircuit call list object that corresponds to this module. Note that the XCcl->callinst gets the object instance that corresponds to the module. */
    
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
/* a net is a list of terminals, one electrical or inductive node */
{
    char *name;
    tlist *terms;
    int type;		/* identify system terminal nets */
    int rflag;		/* Routing status flag */
    list *expns;	/* Used in both local & global routing; */
    list *done;		/* Records expansions that are completed */
    list *route;	/* used in local routing */
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
