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
 *	file var.c
 * As part of MS Thesis Work, S T Frezza, UPitt Engineering
 * June, 1990
 *
 * This file contains the structure and union manipulation functions that 
 * operate on VAR types and the underlying structures.  These are based on the 
 * following definitions:

 * 	typedef union  var_item_union	varItem;
 * 	typedef struct var_item_struct  varStruct;
 * 	typedef struct var_struct 	var;
 * 	typedef struct var_list_struct  varlist;

 * 	struct var_struct
 * 	{
 *     		int 	action;
 *     		varStruct 	*item1;
 *     		varStruct	*item2;
 * 	};

 * 	struct var_item_struct
 * 	{
 *    		varItem   *this;
 *     		int       type;
 * 	};

 * 	union var_union_type
 * 	{
 *     		int     *i;
 *     		srange  *r;
 *     		var *v;
 * 	};
 
 * 	struct var_struct_list
 * 	{
 *     		var       *this;
 *     		varlist   *next;
 * 	};
 */

/*
 * These structures essentially create an evaluation tree for operations on ranges;  It
 * also implements a delayed evaluation mechanism.  This means 
 * that there is an entire algebra being implemented (at least partially) within the following
 * code.  What is here is not that smart, and only has for operations { +, -, /, * }.

 * For example, the sum of two ranges (caused by the evaluation of a _var_ structure with
 * the operation ADD, and having two _varStruct_s which are both ranges yeilds a single 
 * range, which is the first range having both it's min and max fields augmented by the
 * (integer) result of evaluating the second range.  Note that the second range is altered
 * (reduced) during this operation.
 */

#include "externs.h"


/* ---- Local Variables ---------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------*/
/* NEW STUFF - */
/*-------------------------------------------------------------------------------------*/
varStruct *create_int_varStruct(i)
    int *i;
{
    varStruct *vs;
    vs = (varStruct *)calloc(1, sizeof(varStruct));
    vs->type = INT;
    vs->this = (varItem *)i;
    return(vs);
}
/*-------------------------------------------------------------------------------------*/
varStruct *create_rng_varStruct(sr)
    srange *sr;
{
    varStruct *vs;
    vs = (varStruct *)calloc(1, sizeof(varStruct));
    vs->type = RNG;
    vs->this = (varItem *)sr;
    return(vs);
}
/*-------------------------------------------------------------------------------------*/
varStruct *create_var_varStruct(v)
    var *v;
{
    varStruct *vs;
    vs = (varStruct *)calloc(1, sizeof(varStruct));
    vs->type = VAR;
    vs->this = (varItem *)v;
    return(vs);
}

/*-------------------------------------------------------------------------------------*/
var *create_var_struct(a, i1, i2)
    int a;
    varStruct *i1, *i2;
{
    /* Creates a var structure, with the given action and the two item pointers set. */
    var *v;
    v = (var *)calloc(1, sizeof(var));
    v->action = a;
    v->item1 = i1;
    v->item2 = i2;
    return(v);
}
/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/
srange *create_srange(q1, q2, or)
    int q1, q2, or;
{
    srange *r = (srange *)calloc(1, sizeof(srange));
    r->q1 = MIN(q1, q2);
    r->q2 = MAX(q1, q2);
    r->orient = or;
    return(r);
}


/*-------------------------------------------------------------------------------------*/
varStruct *reduce_var_struct(vs)
    varStruct *vs;
{
    /* NOTE: - this does not change <vs>, but if it is made up of vars, then it will
       evaluate all of the lower var structures.  */
    int temp;
    var *v;
    
    
    switch (vs->type)
    {
	case INT:   
	case RNG:   return(vs);
	            break;

	case VAR:   v = (var *)vs->this;
		    temp = eval_var_struct(v->item2);
	            return(eval_action(v->action, 
				       v->item1, v->item2));   
	            break;	
	default:    fprintf(stderr, "screwy varStruct passed to REDUCE_VAR_STRUCT\n");
    }
}
/*-------------------------------------------------------------------------------------*/
int eval_srange(r)
    srange *r;
{
    /* This returns an integer equivalent to the center of the given range: */
    int sum;
    if (r->q1 != r->q2)
    {
	sum = r->q1 + r->q2;
	r->q1 = r->q2 = sum/2;
    }
    return(r->q1);
}
	
/*-------------------------------------------------------------------------------------*/
int eval_var_struct(vs)
    varStruct *vs;
{
    /* NOTE: - this changes <vs> */
    int temp = 0;
    
    switch (vs->type)
    {
	case INT:   temp = (vs->this == NULL) ? 0 : *(int *)vs->this;
	            break;
	case RNG:   temp = eval_srange((srange *)vs->this);  
	            break;
	case VAR:   temp = eval_var((var *)vs->this);
	            break;	
	default:    fprintf(stderr, "screwy varStruct passed to EVAL_VAR_struct\n");
    }
    vs->this = (varItem *)&temp;
    vs->type = INT;
    return(temp);
}
/*-------------------------------------------------------------------------------------*/
varStruct *add_var_structs(i1,i2)
    varStruct *i1, *i2;
{
    /* adds <i1> to <i2>.  <i2> MUST  (or will be coerced)resolve to an integer.
     * This modifies the internal values within <i1> and <i2>.  <i1>'s union remains
     * of the same type as it starts;  i2 is reduced to an integer, if it is not one 
     * already.  */

    int addend = eval_var_struct(i2), t, *temp;
    srange *sr;
    var *v;
    
    switch (i1->type)
    {
	case INT:   t = *(int *)i1->this + addend;
	            temp = &t;
	            i1->this = (varItem *)&temp;
	            break;
	case RNG:   sr = (srange *)i1->this;
                    sr->q1 = sr->q1 + addend;
                    sr->q2 = sr->q2 + addend;
	            break;
	case VAR:   v = (var *)i1->this;
                    /* Recursively distribute the denom throughout the entire 
		       nested var structure */
	            v->item1 = add_var_structs(v->item1, i2);
	            v->item2 = add_var_structs(v->item2, i2);
	            break;	
	default:    fprintf(stderr, "screwy varStruct passed to ADD_VAR_structS\n");
    }
    return(i1);
}
    
/*-------------------------------------------------------------------------------------*/
varStruct *subtract_var_structs(i1,i2)
    varStruct *i1, *i2;
{
    /* subtracts <i2> from <i1>.  <i2> MUST resolve (or will be coerced) to an integer.
     * This modifies the internal values within <i1> and <i2>.  <i1>'s union remains 
     * of the same type as it starts;  i2 is reduced to an integer, if it is not one 
     * already.  */

    int subtrahend = eval_var_struct(i2), t, *temp;
    srange *sr;
    var *v;
    
    switch (i1->type)
    {
	case INT:   t = *(int *)i1->this - subtrahend;
	            i1->this = (varItem *)&t;
	            break;
	case RNG:   sr = (srange *)i1->this;
                    sr->q1 = sr->q1 - subtrahend;
                    sr->q2 = sr->q2 - subtrahend;
	            break;
	case VAR:   v = (var *)i1->this;
                    /* Recursively distribute the denom throughout the entire 
		       nested var structure */
	            v->item1 = subtract_var_structs(v->item1, i2);
	            v->item2 = subtract_var_structs(v->item2, i2);
	            break;	
	default:    fprintf(stderr, "screwy varStruct passed to SUBTRACT_VAR_structS\n");
    }
    return(i1);
}
    
/*-------------------------------------------------------------------------------------*/
varStruct *multiply_var_structs(i1,i2)
    varStruct *i1, *i2;
{
    /* multiply <i1> by <i2>.  <i2> MUST resolve (or will be coerced) to an integer.
     * This modifies the internal values within <i1> and <i2>.  <i1>'s union remains
     * of the same type as it starts;  i2 is reduced to an integer, if it is not one 
     * already.  */

    int multiplicand = eval_var_struct(i2), t,  *temp;
    srange *sr;
    var *v;
    
    switch (i1->type)
    {
	case INT:   t = *(int *)i1->this * multiplicand;
	            i1->this = (varItem *)&t;
	            break;
	case RNG:   sr = (srange *)i1->this;
                    sr->q1 = sr->q1 * multiplicand;
                    sr->q2 = sr->q2 * multiplicand;
	            break;
	case VAR:   v = (var *)i1->this;
                    /* Recursively distribute the denom throughout the entire 
		       nested var structure */
	            v->item1 = multiply_var_structs(v->item1, i2);
	            v->item2 = multiply_var_structs(v->item2, i2);
	            break;	
	default:    fprintf(stderr, "screwy varStruct passed to MULTIPLY_VAR_structS\n");
    }
    return(i1);
}
    
/*-------------------------------------------------------------------------------------*/
varStruct *divide_var_structs(i1,i2)
    varStruct *i1, *i2;
{
    /* divide <i1> by <i2>.  <i2> MUST resolve (or will be coerced) to an integer.
     * This modifies the internal values within <i1> and <i2>.  <i1>'s union remains 
     * of the same type as it starts;  i2 is reduced to an integer, if it is not one 
     * already.  */

    int denom = eval_var_struct(i2), t, *temp;
    srange *sr;
    var *v;
    
    switch (i1->type)
    {
	case INT:   t = *(int *)i1->this / denom;
	            i1->this = (varItem *)&t;
	            break;
	case RNG:   sr = (srange *)i1->this;
                    sr->q1 = sr->q1 / denom;
                    sr->q2 = sr->q2 / denom;
	            break;
	case VAR:   v = (var *)i1->this;
                    /* Recursively distribute the denom throughout the entire 
		       nested var structure */
	            v->item1 = divide_var_structs(v->item1, i2);
	            v->item2 = divide_var_structs(v->item2, i2);
	            break;	
	default:    fprintf(stderr, "screwy varStruct passed to DIVIDE_VAR_STRUCTS\n");
    }
    return(i1);
}
    
/*-------------------------------------------------------------------------------------*/
varStruct *eval_action(a,i1,i2)
    int a;
    varStruct *i1, *i2;
{
    /* This takes an action and a pair of items and performs the intended action */
    switch (a)
    {
	case ADD:      return(add_var_structs(i1, i2));
	               break;
	case SUBTRACT: return(subtract_var_structs(i1, i2));
	               break;
	case MULTIPLY: return(multiply_var_structs(i1, i2));
	               break;
	case DIVIDE:   return(divide_var_structs(i1, i2));
	               break;
	default :      fprintf(stderr, "screwed up call to EVAL_ACTION\n");
	return(NULL);
    }
 }   
/*-------------------------------------------------------------------------------------*/
var *reduce_var(v)
    var *v;
{
    /* This reduces a complex var structure to a simplified one, that is where only 
       ->item1 is active, and the action is ADD. */
    varStruct *vt = reduce_var_struct(v->item1);
    switch (v->action)
    {
	case ADD:      vt = add_var_structs(vt, v->item2);
	               break;
	case SUBTRACT: vt = subtract_var_structs(vt, v->item2);
	               break;
	case MULTIPLY: vt = multiply_var_structs(vt, v->item2);
	               break;
	case DIVIDE:   vt = divide_var_structs(vt, v->item2);
	               break;
	default :      fprintf(stderr, "screwed up call to REDUCE_VAR\n");
	               vt = NULL;
	               break;
    }   
    v->item1 = vt;
    v->item2 = NULL;
    v->action = ADD;
    
    return(v);
}
/*-------------------------------------------------------------------------------------*/
int eval_var(v)
    var *v;
{
    /* This reduces a complex var structure to a simplified one, and then performs the 
     * operation.  All open range structures are replaced by fixed, arbitrary, legal 
     * values.
     */
 
    varStruct *vt = eval_action(v->action, v->item1, v->item2);   
    int *temp = (int *)vt->this;
    
    return(*temp);
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/
/* end of file "var.c" */
