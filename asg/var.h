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
 *      file var.h
 * As part of MS Thesis Work, S T Frezza, UPitt Engineering
 * June, 1990
 *
 */

typedef union  var_item_union	varItem;
typedef struct var_item_struct  varStruct;
typedef struct var_struct 	var;
typedef struct var_list_struct  varlist;

/* ---------- Srange structures lifted from "route.h" ---------------------------- */
typedef struct short_range_struct       srange;
typedef struct short_range_list_struct  srnglist;

struct short_range_struct
{
    int q1, q2, orient;
};

struct short_range_list_struct
{
    srange *this;
    srnglist *next;
};


/* --------------------- var type definition and supports -------------------------*/

/* Used to identify the action associated with the var*/
#define MULTIPLY  1
#define DIVIDE   2
#define ADD      3
#define SUBTRACT 4

#define VAR 1
#define INT 2
#define RNG 3

#define X 1	/* The orientation of a given edge */
#define Y 2

#define XFUZZ 4
#define YFUZZ 4

struct var_struct
/* structure to assemble strings of ranges;  Each of these is meant to collapse into a
 * single point (integer).  In lisp, the structure is like an unevaluated list:
* '(function item1 item2), which is waiting for evaluation.        */
{
    int 	action;
    varStruct 	*item1;
    varStruct	*item2;
};

struct var_item_struct
/* This is a structure to contain the union of *int, *srange, and *varItem (s)
 * and a code of what the union actually contains. */
{
    varItem   *this;
    int       type;
};


union var_union_type
/* This is a union of *int, *srange, and *varItems: */
{
    int     *i;
    srange  *r;
    var	    *v;
};


struct var_struct_list
{
    var       *this;
    varlist   *next;
};


/*--------------------------------------------------------------- 
 * Defined in var.c
 *---------------------------------------------------------------
 */
extern int eval_var();
/* 	var *v		*/

extern var *reduce_var();
/* 	var *v		*/

extern varStruct *eval_action();
/*     int a			 */
/*     varStruct *i1, *i2	 */

extern varStruct *create_rng_varStruct();
/*     srange *sr 		 */

extern varStruct *create_var_varStruct();
/*     var *v			 */

extern varStruct *create_int_varStruct();
/*     int *i                    */

extern var *create_var_struct();
/*     int a			 */
/*     varStruct *i1, *i2	 */

extern srange *create_srange();
/*     int q1, q2, or */
