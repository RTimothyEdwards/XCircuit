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


/* file utility.c */
/* ------------------------------------------------------------------------
 * Helper / Utility routines for place and route                 spl 7/89
 *
 * ------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>
#include "system_defs.h"
#include "net.h"
#include "network.h"
#include "externs.h"
#include "psfigs.h"
#include "asg_module.h"

/*---------------------------------------------------------------
 * Forward References
 *---------------------------------------------------------------
 */

void add_in_out();
void add_special();

#ifdef TCL_WRAPPER

/*---------------------------------------------------------------
 * my_calloc: replacement for C utility "calloc", using the Tcl
 * routine Tcl_Alloc followed by bzero to initialize memory.
 *---------------------------------------------------------------
 */

void *
my_calloc(size_t nmemb, size_t size)
{
   void *memloc;
   size_t memsiz = nmemb * size;

   memloc = (void *)Tcl_Alloc(memsiz);
   if (memloc) bzero(memloc, memsiz);
   return memloc;
}

#endif  /* TCL_WRAPPER */

/*---------------------------------------------------------------
 * error: generic error reporting, should have more on line numbers etc.
 *
 * Warning:  This routine name conflicts with a C utility, so
 * it is necessary to order the libraries correctly when linking
 * to avoid an incorrect link.
 *---------------------------------------------------------------
 */

error(s1, s2)
    char *s1, *s2;
{
    Fprintf(stderr, "ERROR, line %d: %s\n\t%s\n", lcount, s1, s2);
    Flush(stderr);
    
}

/*---------------------------------------------------------------
 * side_of: return the module side for the terminal t (module is unique)
 *---------------------------------------------------------------
 */
side_of(t)
    term *t;
{
    if (t == NULL)
    {
	error("side_of passed NULL terminal","");
    }
    else if (t->mod == NULL)
    {
	error("side_of passed terminal with no module","");
    }
    else 
    {
	return (t->side);
    }
}

/*---------------------------------------------------------------
 * mark_connection
 * find, and mark the terminals which connect the output of 
 * one module to the input of the next, in a string.
 *---------------------------------------------------------------
 */
mark_connection (m1, m2)
    module *m1, *m2;
{	
    tlist *l, *ll;
    int found = FALSE;
    
    for (l = m1->terms; l != NULL; l = l->next)
    {
	if (l->this->type == OUT)	/* ignore (l->this->type == INOUT) */
	{
	    for(ll = l->this->nt->terms; ll != NULL; ll = ll->next)
	    {
		if((ll->this->mod->index == m2->index)
		   &&
		   (ll->this->type == IN)	/* ignore (ll->this->type == INOUT) */
		   )
		{
		    m1->primary_out = l->this;
		    m2->primary_in = ll->this;
		    found = TRUE;
		}
	    }
	}
    }
    if (!found)
    {
	error("mark_connection: two modules were not connected", m1->name);
    }
}

/* ------------------------------------------------------------------------
 * insert_index_list insert (by index) an element to a 2d list structure
 * 
 * ------------------------------------------------------------------------
 */
llist *insert_index_list(element, indx, lst)
    int *element;
    int indx;
    llist *lst;
{
    llist *tl, *l;
    
    if((lst == NULL)||(lst->index > indx))
    {
	tl = (llist *)calloc(1, sizeof(llist));
	tl->next = lst;
	tl->index = indx;
	tl->values = NULL;
	tl->values = concat_list(element, tl->values);
	return (tl);
    }
    else
    {
	for(l = lst; ; l = l->next)
	{
	    if(l->index == indx)
	    {
		/* we already have a list at this index */
		l->values = concat_list(element, l->values);
		return(lst);
	    }
	    if ( (l->next == NULL) || (l->next->index > indx))
	    {/* note we are counting on order of above exp */
		
		/* we hit the end or fall between two existing lists */
		tl = (llist *)calloc(1, sizeof(llist));
		tl->next = l->next;
		l->next = tl;
		tl->index = indx;
		tl->values = NULL;
		tl->values = concat_list(element, tl->values);
		return(lst);
	    }
	}
    }
}

/* ------------------------------------------------------------------------
 * get_net: do a lookup on all the nets, by name.
 * could be done with a hash table a-la rsim.
 * ------------------------------------------------------------------------
 */

net *get_net(name)
    char *name;
{
    nlist *n;

    for (n = nets; n != NULL ; n = n->next)
	if (!strcmp(n->this->name, name))
	    return(n->this);

    /* error ("get_net, net used before it is declared ", name); */
    return(NULL);
}

/*---------------------------------------------------------------
 * str_end_dup
 *---------------------------------------------------------------
 */
char *str_end_dup(new, old, n)
    char *new, *old;
    int n;
{
    /* Copy the last <n> characters from <old> to <new>. */
    int i, len = strlen(old);

    new = (char *)calloc(NET_NAME_SIZE + 1, sizeof(char));
    if (len > n)
    {
	strncpy(new, old, n);
	for (i=0; i<n; i++)
	{
	    new[i] = old[len-n+i];
	}
	new[n] = '\0';
    }
    else strcpy(new, old);

    return(new);
}

/* ------------------------------------------------------------------------
 * newnode: add a new net to the global list of nets
 * ------------------------------------------------------------------------
 */
void newnode(name)
    char *name;
{
    net *node;
    tlist *t;


    node = (net *)calloc(1, sizeof(net));
    node->rflag = 0;		/* Added 11-3-90 stf;  Used in Global Router */
    node->expns = NULL;    

    node->name = strdup(name);
    
    /* initialize -- spl 9/30/89 */
    node->terms = NULL;

    /* check to see if this is a global input/output node
     * this is a hack, it would not have to be done execpt that
     * the current ivf format does not list the i/o nodes in the
     * flattened discription so we have to pull that out of the
     * hierarchy first, and leave them on this ext_terms list
     * here is where we tie the info together.
     */

    for(t = ext_terms; t != NULL; t = t->next)
    {
	if (!strcmp(t->this->name, node->name))
	{
	    node->terms = (tlist *)concat_list(t->this, node->terms);
	    
	    node->type = (t->this->type == IN) ? 
	        OUT : (t->this->type == OUT) ? IN : INOUT;
	    
	    nets = (nlist *)concat_list(node, nets);
	    
	    t->this->nt = node; /* have the term point to it's parent node */
	    
	    node_count++; /* for statistics */
	    
	    return;	
	}
    }
    
    /* otherwise we have to do it all */	
    node->terms = NULL;
    node->type = NONE;  /* not global in/out */
    
    nets = (nlist *)concat_list(node, nets);
    
    node_count++; /* for statistics */
}


/* ------------------------------------------------------------------------
 * set_default_object_size
 * match the object type to some defualt sizes, based on the expected 
 * sizes of the icons.
 * NOTE:  This function interacts with the ps_print functions in "psfigs.c"
 * ------------------------------------------------------------------------
 */
set_default_object_size(type, x, y)
    char *type;
    int *x, *y;
{
    switch(validate(type))
    {
	case BUFFER : 
	case INVNOT_:
	case NOT_   :  *x = GATE_HEIGHT;
	               *y = SYSTERM_SIZE;
		       break;
	case AND  : 
	case NAND : 
	case OR   : 
	case NOR  : 
	case XOR  : 
	case XNOR : *x = GATE_LENGTH;
	            *y = GATE_HEIGHT;
	            break;
	case INPUT_SYM : 
	case OUTPUT_SYM : *x = SYSTERM_SIZE;
	                  *y = SYSTERM_SIZE;
	                  break;
        case BLOCK : 
        default : 
	             *x = ICON_SIZE;
		     *y = ICON_SIZE;
	  	     break;
   }
}

/* ------------------------------------------------------------------------
 * newobject
 * create a new object/module and install it in the global list
 * ------------------------------------------------------------------------
 */
module *newobject(gate_name, gate_type)
    char *gate_name, *gate_type;
{
    module *object;

    if(module_count >= MAX_MOD)
    {
	error("newobject: too many modules, change MAX_MOD and recompile",
	      gate_name);
	/* exit(1); */
	return NULL;
    }
    
    object = (module *)calloc(1, sizeof(module));
    object->index = module_count++;
    object->name = strdup(gate_name);
    object->type = strdup(gate_type);
    object->x_pos = 0;
    object->y_pos = 0;    
    object->flag = UNSEEN;
    object->placed = UNPLACED;
    object->rot = LEFT;		/* assme no rotation */
    object->primary_in = NULL;
    object->primary_out = NULL;
    object->terms = NULL;
    object->fx = create_var_struct(ADD, NULL, NULL);
    object->fy = create_var_struct(ADD, NULL, NULL);
    set_default_object_size(object->type, &object->x_size, &object->y_size);
    
    modules = (mlist *)concat_list(object, modules);
    return(object);
}

/* ------------------------------------------------------------------------
 * addin: add a signal to the list of terminals for a module
 * ------------------------------------------------------------------------
 */
void addin(name, index, noOfTerms)
    char *name;
    int index;
    int noOfTerms;
{
	/* MODIFICATION 6-11-90 Skip "_DUMMY_SIGNAL_", "guard","$1" and $0" strings... */
	if ((!strcmp(name, STR_DUMMY_SIGNAL)) ||
	    (!strcmp(name, SIG_ZERO)) ||
	    (!strcmp(name, SIG_ONE)) ||
	    (!strcmp(name, STR_GUARD_SIGNAL)))	    /* See "system_defs.h" */
	{
	    add_special(name, index, noOfTerms, IN);
	}
    
	else add_in_out(name, index, noOfTerms, IN);
}
/* ------------------------------------------------------------------------
 * addout: add a signal to the list of terminals for a module
 * ------------------------------------------------------------------------
 */
void addout(name, index, noOfTerms)
    char *name;
    int index;
    int noOfTerms;
{
	/* MODIFICATION 6-11-90 Skip "_DUMMY_SIGNAL_", "$1" and $0" strings... */
	if ((!strcmp(name, STR_DUMMY_SIGNAL)) ||
	    (!strcmp(name, SIG_ZERO)) ||
	    (!strcmp(name, SIG_ONE)) ||
	    (!strcmp(name, STR_GUARD_SIGNAL)))	    /* See "system_defs.h" */
	{
	    add_special(name, index, noOfTerms, OUT);
	}
    
	else add_in_out(name, index, noOfTerms, OUT);
}
/* ------------------------------------------------------------------------
 * addinout: add a signal to the list of terminals for a module
 * ------------------------------------------------------------------------
 */
void addinout(name, index, noOfTerms)
    char *name;
    int index;
    int noOfTerms;
{
/* MODIFICATION 6-11-90 Skip "_DUMMY_SIGNAL_", "$1" and $0" strings... */
	if ((!strcmp(name, STR_DUMMY_SIGNAL)) ||
	    (!strcmp(name, SIG_ZERO)) ||
	    (!strcmp(name, SIG_ONE)) ||
	    (!strcmp(name, STR_GUARD_SIGNAL)))	    /* See "system_defs.h" */
	{
	    add_special(name, index, noOfTerms, INOUT);
	}
    
	else add_in_out(name, index, noOfTerms, INOUT);
}

void addoutin(name, index, noOfTerms)
    char *name;
    int index;
    int noOfTerms;
{
/* MODIFICATION 6-11-90 Skip "_DUMMY_SIGNAL_", "$1" and $0" strings... */
	if ((!strcmp(name, STR_DUMMY_SIGNAL)) ||
	    (!strcmp(name, SIG_ZERO)) ||
	    (!strcmp(name, SIG_ONE)) ||
	    (!strcmp(name, STR_GUARD_SIGNAL)))	    /* See "system_defs.h" */
	{
	    add_special(name, index, noOfTerms,OUTIN);
	}
    
	else add_in_out(name, index, noOfTerms,OUTIN);
}

    
/* ------------------------------------------------------------------------
 * relative_term_location: determine where a signal should be located on the module:
 * ------------------------------------------------------------------------
 */
int relative_term_location(termNo, termsOnEdge, edgeLength, spacesPerPin)
    int termNo, termsOnEdge, edgeLength, spacesPerPin;
{
    int offset, space = edgeLength / (termsOnEdge + 1);

    if (termsOnEdge > edgeLength) return(termNo);
    else if (termsOnEdge == edgeLength) return(termNo + 1);
    else if (termNo > termsOnEdge) return(termNo);		/* Problem !! */
    else if (termsOnEdge <= 3)
    {
	switch (termsOnEdge)
	{
	    case 1 : return(edgeLength/2);
	    case 2 : if (termNo == 0) return(space);
	             else return(edgeLength - space);
	    case 3 : switch (termNo)
	             {
			 case 0 : return(edgeLength - space);
			 case 1 : return(edgeLength/2);
			 case 2 : return(space);
		     }
	}
    }
    else /* Space things out evenly via spacesPerPin (used to set <edgeLength>)*/
    {
	space = edgeLength / termsOnEdge;
	offset = (edgeLength - (termsOnEdge * space) + space)/2;

	if (offset < 0) offset = 0;

	if ((termsOnEdge > edgeLength/spacesPerPin) && 
	    (termNo > termsOnEdge/spacesPerPin))
	{ 	/* Overlap... */
	    /* This should never be an important result - see "addpositions" */
	    return((termNo - termsOnEdge/spacesPerPin) * space);
	}
	else return((termNo * space) + offset);
    }
}
/*
 if space == 0, the progression is 1, 2, 3, 4 ...
 if space == 1, the progression is 1, 3, 5, 7 ...
 if space == 2, the progression is 1, 4, 7, 10 ...
*/

/* ------------------------------------------------------------------------
 * add_in_out: add a signal to the list of terminals for a module
 * ------------------------------------------------------------------------
 */
void add_in_out(name, index, noOfTerms, dir)
    char *name;
    int index, noOfTerms, dir;
{
    tlist *tl;
    term *tt;
    net *tn;
    
    tt = (term *)calloc(1, sizeof(term));

    /* find this net in the global database */
    tn = get_net(name);
    tt->nt = tn;    
    tt->mod = curobj_module;
    tt->name = (char *)calloc(1, TERM_NAME_SIZE);

switch (dir)
{
case IN:
	    sprintf(tt->name, "input_%d", index);
	    tt->side = LEFT;
	    tt->type = IN;
	    tt->x_pos = -1; /* -1 is the old hack to put terms outside of module */
	    /* hack to center inputs on left side. Also see "addpositions" */
	    tt->y_pos = relative_term_location(index, noOfTerms, tt->mod->y_size, SPACES_PER_PIN);
	    break;
case OUT:
	    sprintf(tt->name, "output_%d", index);
	    tt->side = RIGHT;
	    tt->type = OUT;
	    tt->x_pos = tt->mod->x_size + 1;   /* add 1 hack to center outputs on 
						  right side */
	    tt->y_pos = relative_term_location(index, noOfTerms, tt->mod->y_size, SPACES_PER_PIN);
	    tt->mod->primary_out = tt;	       /* hack to see that all mods have a PO */
	    break;
case INOUT:

	    sprintf(tt->name, "inout_%d", index);
	    tt->side = UP;     /* MODIFICATION 20-10-89 stf (labels are up) */
	                       /* MOD 6-11-90 (SHOULD BE 'UP'- SCED LABELS AREN'T USED */
	    tt->type = INOUT;
    
	    /* MOD 5-2-91 INOUT's are distributed accross the top of the icon */
	    tt->x_pos = relative_term_location(index, noOfTerms, tt->mod->x_size, SPACES_PER_PIN);
	    tt->y_pos = tt->mod->y_size + 1;   /* Put at top of icon... */
	    break;

case OUTIN:
	    sprintf(tt->name, "inout_%d", index);
	    tt->side = BOTTOM;     /* MODIFICATION 20-10-89 stf (labels are up) */
	                       /* MOD 6-11-90 (SHOULD BE 'UP'- SCED LABELS AREN'T USED */
	    tt->type = OUTIN;
    
	    /* MOD 5-2-91 INOUT's are distributed accross the top of the icon */
	    tt->x_pos = relative_term_location(index, noOfTerms, tt->mod->x_size, SPACES_PER_PIN);
	    tt->y_pos = -1;   /* Put at top of icon... */
	    break;
default: error("add_in_out: terminal with bad direction", name);
 }
    
    curobj_module->terms = (tlist *)concat_list(tt, curobj_module->terms);

	/* tie the term back into the list of terms on this net */

         tn->terms = (tlist *)concat_list(tt, tn->terms);
}

/* ------------------------------------------------------------------------
 * add_xc_term: add a signal to the list of terminals for a module
 * based on the XCircuit object for gate "name".
 *
 * If "pinName" is non-NULL, it points to a string containing the
 * pin name.  If "pinName" is NULL, then the target symbol is
 * searched for a SPICE info label, and the pin name is pulled
 * from that.
 * ------------------------------------------------------------------------
 */

void add_xc_term(type, pinName, netName, index, numpins)
    char *type;
    char *pinName;
    char *netName;
    int index;
    int numpins;
{
    tlist *tl;
    term *tt;
    net *tn;
    objinstptr xc_inst;
    int result, x, y, a, b, c;
    char *locPin;
    
    /* Get the xcircuit object name corresponding to the module */
    if (NameToObject(type, &xc_inst, FALSE) == NULL) return;

    /* If the pinName is NULL, try go get the pin name from the object */
    if (pinName == NULL) {
	locPin = parsepininfo(xc_inst, "spice", index);
	if (locPin == NULL) locPin = defaultpininfo(xc_inst, index);
	if (locPin == NULL) return;	/* No pins; can't do much! */
    }
    else locPin = pinName;

    /* Find the location of the pin named "pinName" relative to the object position */
    result = NameToPinLocation(xc_inst, locPin, &x, &y);


    /* Temporary hack---shift all coordinates relative to the bounding box corner */
    /* x -= xc_inst->bbox.lowerleft.x; */
    /* y -= xc_inst->bbox.lowerleft.y; */

    if (result < 0) {
       if (locPin != pinName) free(locPin);
       add_in_out(netName, index, numpins, INOUT);	/* backup behavior */
       return;						/* if pin name not found */
    }

    tt = (term *)calloc(1, sizeof(term));

    /* find this net in the global database */
    tn = get_net(netName);
    tt->nt = tn;    

    tt->mod = curobj_module;
    if (locPin != pinName)
       tt->name = locPin;
    else {
       tt->name = (char *)calloc(1, TERM_NAME_SIZE);
       sprintf(tt->name, pinName);
    }

    /* Treat all (analog) signals as "inout", but determine which side	*/
    /* is closest to the pin position.					*/
    /* sprintf(tt->name, "inout_%d", index); */ /* former behavior */

    tt->x_pos = x;
    tt->y_pos = y;

    /* Check for position of label relative to the object center and 	*/
    /* set value as IN/LEFT, OUT/RIGHT, etc.				*/
    /* Divide the object's bounding box into quadrants based on		*/
    /* diagonals through the center, and give the pin a placement LEFT	*/
    /* (IN), RIGHT (OUT) accordingly.					*/

    a = (x - xc_inst->bbox.lowerleft.x) * xc_inst->bbox.height;
    b = (y - xc_inst->bbox.lowerleft.y) * xc_inst->bbox.width;
    c = a - (xc_inst->bbox.width * xc_inst->bbox.height);

    if ((a < b) && (c < -b)) {
       tt->type = IN;
       tt->side = LEFT;
    }
    else if (a < b) {
       tt->side = UP;
       tt->type = INOUT;
    }
    else if (c < -b) {
       tt->side = BOTTOM;
       tt->type = OUTIN;
    }
    else {
       tt->side = RIGHT;
       tt->type = OUT;
    }
    
    curobj_module->terms = (tlist *)concat_list(tt, curobj_module->terms);

    /* tie the term back into the list of terms on this net */

    tn->terms = (tlist *)concat_list(tt, tn->terms);
}

/* ------------------------------------------------------------------------
 * add_special: add a special terminal to a module
 * ------------------------------------------------------------------------
 */
void add_special(name, index, noOfTerms, dir)
    char *name;
    int index, noOfTerms, dir;
{
    tlist *tl;
    term *tt;
    net *tn;
    
    tt = (term *)calloc(1, sizeof(term));
    tt->nt = NULL;	/* Special terminals have no nets */

    tt->mod = curobj_module;
    tt->name = (char *)calloc(1, TERM_NAME_SIZE);

switch (dir)
{
case IN:
	    sprintf(tt->name, "%s_input_%d", name, index);
	    tt->side = LEFT;
	    tt->x_pos = -1; /* -1 is the old hack to put terms outside of module */
	    /* hack to center inputs on left side. Also see "addpositions" */
	    tt->y_pos = relative_term_location(index, noOfTerms, tt->mod->y_size, SPACES_PER_PIN);
	    break;
case OUT:
	    sprintf(tt->name, "%s_output_%d", name, index);
	    tt->side = RIGHT;
	    tt->x_pos = tt->mod->x_size + 1;   /* add 1 hack to center outputs on 
						  right side */
	    tt->y_pos = relative_term_location(index, noOfTerms, tt->mod->y_size, SPACES_PER_PIN);
	    tt->mod->primary_out = tt;	       /* hack to see that all mods have a PO */
	    break;
case INOUT:
	    sprintf(tt->name, "%s_inout_%d", name, index);
	    tt->side = UP;     /* MODIFICATION 20-10-89 stf (labels are up) */
	                       /* MOD 6-11-90 (SHOULD BE 'UP'- SCED LABELS AREN'T USED */
    
	    /* MOD 5-2-91 INOUT's are distributed accross the top of the icon */
	    tt->x_pos = relative_term_location(index, noOfTerms, tt->mod->x_size, SPACES_PER_PIN);
	    tt->y_pos = tt->mod->y_size + 1;   /* Put at top of icon... */
	    break;

case OUTIN:
	    sprintf(tt->name, "inout_%d", index);
	    tt->side = BOTTOM;     /* MODIFICATION 20-10-89 stf (labels are up) */
	                       /* MOD 6-11-90 (SHOULD BE 'UP'- SCED LABELS AREN'T USED */
	    tt->type = OUTIN;
    
	    /* MOD 5-2-91 INOUT's are distributed accross the top of the icon */
	    tt->x_pos = relative_term_location(index, noOfTerms, tt->mod->x_size, SPACES_PER_PIN);
	    tt->y_pos = -1;   /* Put at top of icon... */
	    break;
default: error("add_special: terminal with bad direction", name);
}
    /* The TYPE is based on the signal name: */
    if (!strcmp(name, SIG_ZERO)) tt->type = GND;
    else if (!strcmp(name, SIG_ONE)) tt->type = VDD;
    else tt->type = DUMMY;

    /* Add the terminal to the growing list of terminals for this module: */
    curobj_module->terms = (tlist *)concat_list(tt, curobj_module->terms);
}

/* ------------------------------------------------------------------------
 * named_net_p - Return TRUE if the net name matches the given string.
 *-------------------------------------------------------------------------
 */
int named_net_p(nName, t)
    char *nName;
    term *t;
{
    if (!strcmp(nName, t->nt->name)) return(TRUE);
    else return(FALSE);
}

/* ------------------------------------------------------------------------
 * classify_terminal - return the type (IN, OUT, INOUT, OUTIN) for the term
 * based on it's relative position on the module.
 *-------------------------------------------------------------------------
 */
int classify_terminal(m, t)
    module *m;
    term *t;
{
    if (t->x_pos >= m->x_size)
    {
	if (t->y_pos >= m->y_size) 
	{
	    t->side = UP;
	    return(OUTIN);
	}
	else if (t->y_pos <= 0) 
	{
	    t->side = DOWN;
	    return(INOUT);
	}
	else 
	{
	    t->side = RIGHT;
	    return(OUT);
	}
    }
    else if (t->x_pos <= 0)
    {
	if (t->y_pos >= m->y_size) 
	{
	    t->side = UP;
	    return(OUTIN);
	}
	else if (t->y_pos <= 0) 
	{
	    t->side = DOWN;
	    return(INOUT);
	}
	else
	{
	    t->side = LEFT;
	    return(IN);
	}
    }
    else if (t->y_pos >= m->y_size) 
    {
	t->side = UP;
	return(OUTIN);
    }
    else 
    {
	t->side = DOWN;
	return(INOUT);
    }
}
	
/* ------------------------------------------------------------------------
 * fix_pin_position - for a given named net, part of the given module,
 * fix the pin position to the (x,y) coordinates given:
 *-------------------------------------------------------------------------
 */
int fix_pin_position(m, nn, x, y)
    module *m;
    char *nn;		/* The name of the net being fixed */
    int x,y;
{
    tlist *tl = (tlist *)member(nn, m->terms, named_net_p);

    int adjX, adjY;
    
    /* Adjust the pin position to be just outside the icon, rather than on 
       the edge: */
    if (Xcanvas_scaling == FALSE)
    {
	if (x == 0) adjX = -1;
	else if (x == m->x_size)  adjX = x+1;
	else adjX = x;

	if (y == 0) adjY = -1;
	else if (y == m->y_size) adjY =  y+1;
	else adjY = y;
    }
    else	/* Treat for XCanvas conventions - note that the origin of the
		   module is at the top left corner, not lower left corner: */
    {
	if (x == 0)adjX = -1;
	else if (x/XCANVAS_SCALE_FACTOR == m->x_size)  adjX = (x/XCANVAS_SCALE_FACTOR)+1;
	else adjX = x/XCANVAS_SCALE_FACTOR;

	if (y == 0) adjY = m->y_size + 1;
	else if (y/XCANVAS_SCALE_FACTOR == m->y_size) adjY = -1;
	else adjY = m->y_size - (y/XCANVAS_SCALE_FACTOR);
    }

    if (tl != NULL)
    {
	/* put tl->this at the end of the terminal list, and hopefully enable 
	   other terminals on the same net <nn> to be found: */
	rem_item(tl->this, &m->terms);		
	queue(tl->this, &m->terms);

	/* Assign the new position to the terminal: */
	tl->this->x_pos = adjX;
	tl->this->y_pos = adjY;

	/* NOW - reclassify the terminal: */
	tl->this->type = classify_terminal(m, tl->this);
    }
    else 
    {
	Fprintf(stderr, "Bad netname %s trying to be fixed for module %s\n",
		nn, m->name);
    }
}

/* ------------------------------------------------------------------------
 * reset_default_icon_size - use the x,y dimensions  added 3-92 (stf)
 *-------------------------------------------------------------------------
 */
int reset_default_icon_size(m, x, y)
    module *m;
    int x, y;
{
    m->x_size = (Xcanvas_scaling == FALSE) ? x : x/XCANVAS_SCALE_FACTOR;
    m->y_size = (Xcanvas_scaling == FALSE) ? y : y/XCANVAS_SCALE_FACTOR;
}
/* ------------------------------------------------------------------------
 * adjust_icon_size - make the x-y ratio nice (1-4)
 *-------------------------------------------------------------------------
 */
int adjust_icon_size(m, vTerminals, hTerminals)	
/* Adjust the x side of tall blocks, etc.. Return TRUE if something was modified */
    module *m;
{
    int mType = validate(m->type), mX = m->x_size, mY = m->y_size;
    if ((mType == BLOCK) || (mType == DONT_KNOW))
    {
	if (m->x_size / m->y_size < 3) 
	{
	    if ((m->x_size >= 2 * ICON_SIZE) && (m->y_size/3 >= vTerminals * 2)) 
		m->x_size = m->y_size/3;
	    else if (m->y_size/2 >= vTerminals * 2) 
		m->x_size = m->y_size/2;
	}
	else if (m->y_size / m->x_size < 3) 
	{
	    if ((m->y_size >= 2 * ICON_SIZE) && (m->x_size/3 >= hTerminals * 2))
	        m->y_size = m->x_size/3;
	    else if (m->x_size/2 >= hTerminals * 2)m->y_size = m->x_size/2;
	}

	if ((m->x_size != mX) || (m->y_size != mY)) return(TRUE);
    }
    else return(FALSE);
}

/* ------------------------------------------------------------------------
 * addpositions: add the real postions for the terminals, as set by the
 * icon file, if we can find it. Otherwise use the defaults that were
 * done above in add_in_out.
 * Note we do open and read each icon file for each occurance, which might
 * be slow. If this turns out to be the case, we have started to keep
 * track of things: we can re-use the range-list (rlist) we use for the
 * terminal positions, we just have to keep it somewhere.  See the
 * hack note in sced.c
 * ------------------------------------------------------------------------
 */
void addpositions(cur_module, incount, inoutcount, outcount)
    module *cur_module;
    int incount, inoutcount, outcount;
{
    tlist *t;
    int adjustTerminals = FALSE; 
    int type = validate(cur_module->type);
    int spacesPerPin = ((type == BLOCK) || (type == DONT_KNOW)) ? 
                       BLOCK_SPACES_PER_PIN : SPACES_PER_PIN;

     if (compute_aspect_ratios == TRUE) 
    {
	/* Might need to resize the bloody icon if its too bloody small. */
	if (inoutcount > cur_module->x_size/spacesPerPin)
	{
	    /* the default iconsize won't cut it. This will fail in screwey sits. */
	    cur_module->x_size = SPACES_PER_PIN * inoutcount;
	    adjustTerminals = TRUE;
	}

	if ((incount > cur_module->y_size / spacesPerPin) || 
	    (outcount > cur_module->y_size / spacesPerPin))
	{
	    /* the default iconsize won't cut it */
	    cur_module->y_size = spacesPerPin * MAX(incount, outcount);
	    adjustTerminals = TRUE;
	}

	/* Check to see if there is wasted terminal space: (Icon is too big) */
	if ( ((cur_module->y_size / spacesPerPin) > MAX(incount + 1, outcount + 1)) ||
	     ((cur_module->x_size / SPACES_PER_PIN) > inoutcount + 1))
	{
	    adjustTerminals = TRUE;
	}

	if ((adjust_icon_size(cur_module, MAX(incount, outcount), inoutcount) == TRUE)
	    ||
	    (adjustTerminals == TRUE))
	{
	    /* now reset all terminal positions to reflect this: */
	    reposition_all_terminals(cur_module, incount, inoutcount, 
				     outcount, spacesPerPin);
	}
    }
}

/* ------------------------------------------------------------------------ */
int get_terminal_counts(m, ins, inouts, outs)
    module *m;
    int *ins, *inouts, *outs;
{
    tlist *tl;
    for (tl = m->terms; tl != NULL; tl = tl->next)
    {
	switch(tl->this->type)	
	{
	    case IN:    *ins += 1;
	                break;
	    case INOUT: *inouts += 1;
	                break;
	    case OUT:   *outs += 1;
	                break;
	}
    }
}

/* ------------------------------------------------------------------------ */
int pin_spacing(m)
    module *m;
{
    /* Get the expected pin spacing for the given module type: */
    int type = validate(m->type);
    int spacesPerPin = ((type == BLOCK) || (type == DONT_KNOW)) ? 
                       BLOCK_SPACES_PER_PIN : SPACES_PER_PIN;
    return(spacesPerPin);
}

/* ------------------------------------------------------------------------ */
int reposition_all_terminals(m, incount, inoutcount, outcount, spacesPerPin)
    module *m;
    int incount, inoutcount, outcount, spacesPerPin;
{
    /* Reset the positions of all terminals, based on the counts and spacings
       given...*/

    tlist *tl;
    int INindex = 0, INOUTindex = 0, OUTindex = 0;

    for (tl = m->terms; tl != NULL; tl = tl->next)
    {
	switch(tl->this->type)
	{
	    case IN :
	    tl->this->y_pos = relative_term_location(INindex++, incount, 
						     m->y_size, spacesPerPin);
	    tl->this->x_pos = -1;
	    break;

	    case INOUT :
	    tl->this->x_pos = relative_term_location(INOUTindex++, inoutcount, 
						     m->x_size, spacesPerPin);
	    tl->this->y_pos =  m->y_size + 1;
	    break;

	    case OUT :  
            tl->this->y_pos = relative_term_location(OUTindex++, outcount, 
						     m->y_size, spacesPerPin);
	    tl->this->x_pos =  m->x_size + 1;
	    break;
	}
    }
}

   
/* ------------------------------------------------------------------------
 * add_port: add a terminal to the list of system terminals
 * this list is different from the other terminal lists:
 * the names are realy net-names, and there is no module 
 * or net associated with them. We add the net (which 
 * has the same name) later, when we find it.
 * ------------------------------------------------------------------------
 * MODIFICATION (stf 9-89) additions made to create a module, and maintain
 * a list of external modules. 
 */
void add_port(name, dir)
    char *name, *dir;
{
    tlist *tl;
    term *tt;
    net *tn;
    module *tmod;
    
    tt = (term *)calloc(1, sizeof(term));
    
    tt->nt = NULL; /* will be filled in in newnode */
    
    tt->name = (char *)calloc(1, TERM_NAME_SIZE);
    sprintf(tt->name, name);
    
    tmod = newobject(name, dir);
    
    if (!strcmp(dir, "IN"))
    {
        tt->side = RIGHT;
        tt->type = OUT;
        tt->x_pos = tmod->x_size+1;
        tt->y_pos = tmod->y_size/2;
	
    }
    else if (!strcmp(dir, "OUT"))
    {
        tt->side = LEFT;
        tt->type = IN;
        tt->x_pos = -1;		/* (mod 9/90 stf) */
        tt->y_pos = tmod->y_size/2;

    }
    else if (!strcmp(dir, "INOUT"))
    {
        tt->side = UP;		/* Modification 6-11-90 (inouts should be on top.  */
        tt->type = INOUT;
        tt->x_pos = tmod->x_size/2;
        tt->y_pos = tmod->y_size + 1;

    }
    else error("add_port: terminal with bad direction", name);

    /* tie the terminal to the module, and vise-versa */
    
    tmod->terms = (tlist *)concat_list(tt, tmod->terms);
    tt->mod = tmod;

    /* that gave the default postions, but if we can find it in the
     * icon file, we should use those instead. NOTE for real modules,
     * this call is made in parse.c since only it knows,
     * by the order of the ivf file, when all i/o's for the current
     * module are finished. -- spl 9/30/89
     */
    if (tt->type == OUT) /* its an input port, it has one output */
    {
        addpositions(tmod, 0, 0, 1);
    }
    else if (tt->type == IN) /* its an output port, it has one input */
    {
        addpositions(tmod, 1, 0, 0);
    }
    /* for inouts just use the defaults above */
    
    /*  add it to the external connection lists: */
    
    ext_terms = (tlist *)concat_list(tt, ext_terms);
    
    ext_mods = (mlist *)concat_list(tmod, ext_mods);
    
}

/* ------------------------------------------------------------------------
 * get_module: do a lookup on all the modules, by index.
 * could be done with a hash table a-la rsim.
 * ------------------------------------------------------------------------
 */

module *get_module(index)
    int index;
{
    mlist *m;

    for (m = modules; m != NULL ; m = m->next)
    {
	if (m->this->index == index)
	{
	    return(m->this);
	}
    }
    error ("get_module, module of this index not found ", index);
    return(NULL);
}

/* ------------------------------------------------------------------------
 * rot_x_y
 * manhattan rotate y coordinates of terminal positions of an icon.
 * Rotation is done such that the resulting box still has its lower
 * left corner at the (relative) 0, 0 postion.
 * ------------------------------------------------------------------------
 */

int rot_x_y(rot, x_size, y_size, t)
    int rot, x_size, y_size;
    term *t;
{
    int x_old = t->x_pos;    
    int y_old = t->y_pos;
    
    switch(rot)
    {
	case ZERO:
	{
	    error("rot_x_y called with no needed rotation","");
	}
	break;
	case NINETY:
	{
	    t->x_pos = y_old;
	    t->y_pos = (x_size - x_old);
	}
	break;
	case ONE_EIGHTY:
	{
	    t->x_pos = (x_size - x_old);
	    t->y_pos = (y_size - y_old);
	}
	break;
	case TWO_SEVENTY:
	{
	    t->x_pos = (y_size - y_old);
	    t->y_pos = x_old;
	}
	break;
	default: error("rot_x_y: called with bad rotation","");
    }
}
/* ------------------------------------------------------------------------
 * rot_side
 * manhattan rotate terminal side info
 * ------------------------------------------------------------------------
 */
int rot_side(rot, side)
    int rot, side;
{
    /* Im a sucker for a hack */
    
    return((rot + side) % 4);
}
	
/* ------------------------------------------------------------------------
 * count_terms
 * Count all of the terms on the module that exit on a given side:
 * ------------------------------------------------------------------------
 */
int count_terms(m, s)
    module *m;
    int s;
{
    tlist *t;
    int count = 0;
    
    for(t = m->terms; t != NULL; t = t->next)
    {
	if (t->this->side == s) count++;
    }
    return(count);
}
/* ------------------------------------------------------------------------
 * count_terms_part
 * Count all the unique terms in the partition that point in the given direction...
 * ------------------------------------------------------------------------
 */
int count_terms_part(part, s)
    int part;
    int s;
{
    mlist *ml;
    int count = 0;
    tlist *tl;
    nlist *nets = NULL;
    
    for(ml = partitions[part]; ml != NULL; ml = ml->next)
    {
	for (tl = ml->this->terms; tl != NULL; tl = tl->next)
	{
	    if (tl->this->side == s) pushnew(tl->this->nt, &nets, identity);
	}
    }
    count = list_length(nets);
    free_list(nets);
    return(count);
}
/*----------------------------------------------------------------------------
 * count_terms_between_modules(sourceM, destM)
 * similar to "count_terms_part" only it counts the nets between the two mods.
 *----------------------------------------------------------------------------
 */
int count_terms_between_mods(sourceM, destM)
    module *sourceM, *destM;
{
    int count = 0;
    tlist *tl;
    nlist *nets = NULL;
    
    for (tl = sourceM->terms; tl != NULL; tl = tl->next)
    {
	if ((tl->this->type == OUT) || (tl->this->type == INOUT))
	    pushnew(tl->this->nt, &nets, identity);
    }
    for (tl = destM->terms; tl != NULL; tl = tl->next)
    {
	if (tl->this->type == IN)
	    pushnew(tl->this->nt, &nets, identity);
    }
    count = list_length(nets);
    free_list(nets);
    return(count);
}

/* file util - 9/89 stf */
/* more support functions on basic data types: (llist and tnode) */

/*---------------------------------------------------------------
 * collection of node manipulation functions for use on binary trees of the 
 * type 
 *
 *      struct tnode_struct
 *      {
 *          int *this;
 *          tnode *left, *right;
 *      }
 *
 * where node is defined as:
 *
 *      typedef struct tnode_struct tnode;
 */
 
 
tnode *build_leaf(e)
    int *e;
/* used to build nodes, given a pointer to the info to hang there. The pointers are
 * set to NULL */
{
    tnode *temp = (tnode *)calloc(1, sizeof(tnode));
    temp->left = NULL;
    temp->right = NULL;
    temp->this = e;
    return(temp);
}

tnode *build_node(l, r, e)
    tnode *l, *r;
    int *e;
    
/* used to build internal nodes */
{
    tnode *temp = (tnode *)calloc(1, sizeof(tnode));
    temp->left = l;
    temp->right = r;
    temp->this = e;
    return(temp);
}
/*===================================================================================*/
int pull_terms_from_nets(m)
    module *m;
{
    /* This removes the given module from all of the nets in which it's terminals are
       listed;  This is only a one-way removal;  The information in the module structure
       and within the terminal structures remains unchanged.
     */
    net *n;
    tlist *tl;

    for (tl = m->terms; tl != NULL; tl = tl->next)
    {
	/* pull this term from it's net */
	n = tl->this->nt;
	rem_item(tl->this, &n->terms);
    }
}
/*===================================================================================*/
int add_terms_to_nets(m)
    module *m;
{
    /* This adds the given module to all of the nets in which it's terminals are
       listed;  This is the converse operation to "pull_terms_from_nets".
     */
    net *n;
    tlist *tl;

    for (tl = m->terms; tl != NULL; tl = tl->next)
    {
	/* Add this term to it's net */
	n = tl->this->nt;
	pushnew(tl->this, &n->terms);
    }
}

/*===================================================================================*/
void clip_null_gates()
{
    /* This procedure walks through all of the modules being placed, and
       folds all NL_GATE entries, removing them from the completed diagram. */

    int part, termCount = 0, i;
    mlist *temp, *ml;
    module *m;
    term *nullInpuTerm, *nullOutpuTerm, *inpuTerm;
    tlist *tl, *tll;

    for (ml = modules; ml != NULL; ml = ml->next)
    {
	m = ml->this;
	termCount = list_length(m->terms);
	if ((termCount <= 2) && (!strcmp(m->type, GATE_NULL_STR)))
	{
	    if (termCount == 2)
	    {
		nullOutpuTerm = m->primary_out;
		rem_item(nullOutpuTerm, &m->terms);
		nullInpuTerm = m->terms->this;

		/* Fix the net reference in the inputs to bypass the null gate: */
		if ((nullInpuTerm->type != GND) && 
		    (nullInpuTerm->type != VDD) && 
		    (nullInpuTerm->type != DUMMY))
		{
		    for (tl=nullInpuTerm->nt->terms; tl != NULL; tl = tl->next)
		    {
			if (tl->this != nullInpuTerm)
			{
			    tl->this->nt = nullOutpuTerm->nt;
			    inpuTerm = tl->this;
			}
		    }
		    
		    /* Fix the net reference in the output to bypass the null gate: */
		    for (tl=nullOutpuTerm->nt->terms; tl != NULL; tl = tl->next)
		    {
			if (tl->this == nullOutpuTerm)
			{
			    tl->this = inpuTerm;
			}
		    }
		    rem_item(nullInpuTerm->nt, nets);
		}
		else /* Hang the special term at all connected terminals */
		{
		    inpuTerm = nullInpuTerm;
		    /* Fix the net reference in the output to bypass the null gate: */
		    for (tl=nullOutpuTerm->nt->terms; tl != NULL; tl = tl->next)
		    {
			if (tl->this != nullOutpuTerm)
			{
			    tl->this = inpuTerm;
			}
		    }
		    tll = nullOutpuTerm->nt->terms;
		    rem_item(nullOutpuTerm, &tll);
		}
	    }
	    /* The next two lines are needed if deletion occurs after partitioning */
	    part = module_info[m->index].used;
	    rem_item(m, &partitions[part]);

	    rem_item(m, &modules);
	    module_count -= 1;
	}
    }

    /* Now go through the modules and renumber them... */
    /* The module index should */
    temp = (mlist*)rcopy_list(modules);
    for (ml = temp, i=0; ml != NULL; ml = ml->next, i++)
    {
	m = ml->this;
	module_info[i] = module_info[m->index];
	m->index = i;
    }
    free_list(temp);
}

/*---------------------------------------------------------------
 * str_end_copy
 *---------------------------------------------------------------
 */

char *str_end_copy(new, old, n)
    char *new, *old;
    int n;
{
    /* Copy the last <n> characters from <old> to <new>. */
    int i, len = strlen(old);
    if (len > n)
    {
	strncpy(new, old, n);
	for (i=0; i<n; i++)
	{
	    new[i] = old[len-n+i];
	}
	new[n] = '\0';
    }
    else strcpy(new, old);

    return(new);
}

/* ------------------------------------------------------------------------
 * END OF FILE
 * ------------------------------------------------------------------------
 */

