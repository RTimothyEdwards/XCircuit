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



/*---------------------------------------------------------------
 * External References
 *---------------------------------------------------------------
 */
#include "network.h"
/*--------------------------------------------------------------- 
 * Defined in main.c
 *---------------------------------------------------------------
 */

extern mlist *modules;		/* M the set of all modules */
extern nlist *nets;		/* N the set of all nets */
extern tlist *ext_terms;	/* ST the set of all system terminals */
extern mlist *ext_mods;         /* the modules associated w/ ST */

/* NOT USED */
extern tlist *int_terms;	/* T the set of all inter-module terminals */

/* FOR THE ROUTER (only) */
extern tile **routingRoot[2];
extern int currentPartition;

/* NOT USED MUCH */
extern mlist **boxes;		/* the sets of unit boxes */
extern ctree **parti_stats;	/* partition statistics */

/* FOR THE PLACER (only) */
extern mlist **strings;		/* the set of strings */
extern mlist **partitions;	/* the set of PARTITIONS (lists) of modules */
extern tile **rootTiles;        /* the tile-spaces that correspond to each partition */
extern int *x_positions;	/* and their postions, after placement */
extern int *y_positions;
extern int *str_length;
extern int *str_height;

/* Used by a lot of files: */
extern int *x_sizes;	  	/* max x size of each partition */
extern int *y_sizes;		/* max y size of each partition */
extern int xfloor, yfloor;


/* NOT USED ENOUGH */
extern int partition_count;	/* the total number of partitions created */
extern int lcount;		/* number of input lines read */
extern int module_count;	/* number of modules/objects read in */
extern int node_count;		/* number of nodes/signals read in */
extern int terminal_count;	/* number of terminals read in */

extern int connected[MAX_MOD][MAX_MOD]; /* BIG array of connection counts */
extern info_type module_info[MAX_MOD];	/* reused  module information array */

extern int compute_aspect_ratios;  /* command line flag: a(spect ratios) */
extern int Xcanvas_scaling;	   /* command line flag: b (use XCanvas scaling) */
extern int max_partition_conn;  /* command line arg:  c(onnections) */
extern int debug_level;		/* command line arg:  d(ebug) */
extern int stopAtFirstCut;	/* command line flag: f(irst cut @ Global Route) */
extern int congestion_rule;     /* command line flag: conGestion Rule */ 
extern int do_hand_placement;   /* command line flag: h(and placement) */
extern int includeSysterms;	/* command line flag: i(donot Include system terminals)*/
extern int cc_search_level;     /* command line arg:  l(evel) */
extern int matrix_type;		/* command line arg:  m(atrix type) */
extern int do_routing;		/* command line flag: n(o routing) */
extern FILE *outputFile;	/* command line arg:  o(utput to file) */
extern int partition_rule;	/* command line arg:  p(artition_rule) */
extern int recordStats;		/* command line flag: q(uality statistics) */
extern float partition_ratio;	/* command line arg:  r(atio) */
extern int max_partition_size;  /* command line arg:  s(ize)  */
extern int turn_mode;		/* command line arg:  t(urn mode) */   
extern int useSlivering;	/* command line flag: v(bypass sliver correction code) */
extern int useAveraging;	/* command line flag: w(use Weighted averaging) */
extern int latex;		/* command line flag: x(lateX mode) */
extern int recordTiming;	/* command line flag: y(collect timing stats) */
extern int useBlockPartitioning ;		/* -u flag; skips automatic partitioning */
extern int collapseNullModules ;		/* -k flag;denies kollapse of NL_GATES */

/*--------------------------------------------------------------- 
 * Defined in n2a.c
 *---------------------------------------------------------------
 */
void GenerateXcircuitFormat(XCWindowData *areastruct, FILE *ps, Boolean bIsSparmode);
void Route(XCWindowData *areastruct, Boolean bIsSparmode);
int  SetDebugLevel(int *level);
int  ReadHSpice(char *fname);

/*--------------------------------------------------------------- 
 * Defined in asg.c
 *---------------------------------------------------------------
 */

extern int asg_make_instance(XCWindowData *, char *, int, int, int, int);
extern void asg_make_polygon(XCWindowData *, int, int x[], int y[]);
extern void asg_make_label(XCWindowData *, u_char, int, int, char *);

/*--------------------------------------------------------------- 
 * Defined in parse.c
 *---------------------------------------------------------------
 */

extern module *curobj_module;
extern char *str_end_copy();

void   AddModule(char *name, char *type, char *innode, char *outnode);
void   Add2TermModule(char *, char *, char *, char *);
void   AddNTermModule(char *, char *, int, ...);
void   AddModuleTerm(char *, char *, int, int);

/*--------------------------------------------------------------- 
 * Defined in place.c
 *---------------------------------------------------------------
 */

extern int partition();
extern void place_first_strings();
extern void box_placement();
extern void placement_prep();
extern mlist *partition_placement();
extern mlist *gross_systerm_placement();
extern mlist *make_room_for_systerm_placement();
extern int cross_coupled_p();
extern int on_output_net_p();

/*--------------------------------------------------------------- 
 * Defined in terminals.c
 *---------------------------------------------------------------
 */
extern mlist *fine_systerm_placement();

/*--------------------------------------------------------------- 
 * Defined in hand_place.c
 *---------------------------------------------------------------
 */
extern mlist *hand_placement();

/*--------------------------------------------------------------- 
 * Defined in global_route.c
 *---------------------------------------------------------------
 */
extern void create_routing_space();
extern nlist *first_global_route();
extern nlist *incremental_global_route();
extern void print_global_route();
extern int identity();
extern int (*set_congestion_value)();

/*--------------------------------------------------------------- 
 * Defined in local_route.c
 *---------------------------------------------------------------
 */
extern void local_route();
extern void print_local_route_to_file(FILE *f, nlist *nets);
extern void xc_print_local_route(XCWindowData *areastruct, nlist *nets);
extern void collect_congestion_stats();

/*--------------------------------------------------------------- 
 * Defined in psfigs.c
 *---------------------------------------------------------------
 */
extern int ps_print_seg();
extern int ps_print_mod();
extern int xc_print_mod(XCWindowData *areastruct, module *m);
extern int ps_print_contact();
extern int ps_print_border();
extern int ps_put_label();
extern int ps_put_int();

extern int ps_print_standard_header();

/*--------------------------------------------------------------- 
 * Defined in debug.c
 *---------------------------------------------------------------
 */
extern void print_modules();
extern void print_nets();
extern void print_ext();
extern void print_connections();
extern void p_cons();
extern int print_ctree();
extern void print_info();
extern void print_boxes();
extern void print_strings();
extern void print_result();
extern void draft_statistics();
extern float manhattan_PWS();
extern void print_distance_stats();
extern void print_distance_matricies();

/*--------------------------------------------------------------- 
 * Defined in utility.c
 *---------------------------------------------------------------
 */
extern void newnode();
extern module *newobject();
extern void addin();
extern void addout();
extern void adddelay();
extern void add_port();

extern llist *insert_index_list();
extern module *get_module();
extern mark_connection();
extern count_terms();
extern count_terms_part();

extern tnode *build_leaf();
extern tnode *build_node();

extern int pull_terms_from_nets();
extern int add_terms_to_nets();

extern void clip_null_gates();

extern int pin_spacing();
extern int get_terminal_counts();
extern int reposition_all_terminals();
extern int reset_default_icon_size();
extern int fix_pin_position();
