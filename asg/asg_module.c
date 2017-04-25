#include "network.h"
#include "psfigs.h"
#include "asg_module.h"
#include <math.h>


/*----------------------------------------------------------------------*/
int toModuleType(char *mtName) {
    char *s = toupper(mtName);
    if (!strcmp(s, GATE_AND_STR)) return AND_ASG;
    if (!strcmp(s, GATE_OR_STR)) return OR_ASG;
    if (!strcmp(s, GATE_NAND_STR)) return NAND_ASG;
    if (!strcmp(s, GATE_NOR_STR)) return NOR_ASG;
    if (!strcmp(s, GATE_CAPC_STR)) return CAPACITOR_ASG;
    if (!strcmp(s, GATE_RESTR_STR)) return RESISTOR_ASG;
    if (!strcmp(s, GATE_INDR_STR)) return INDUCTOR_ASG;
    if (!strcmp(s, GATE_XOR_STR)) return XOR_ASG;
    if (!strcmp(s, GATE_XNOR_STR)) return XNOR_ASG;
    if (!strcmp(s, GATE_NOT_STR)) return NOT_ASG;
    if (!strcmp(s, GATE_INVNOT_STR)) return INVNOT_ASG;
    if (!strcmp(s, GATE_NULL_STR)) return BUFFER_ASG;
    if (!strcmp(s, BUFFER_GATE)) return BUFFER_ASG;
    if (!strcmp(s, INPUT_TERM)) return INPUT_SYM_ASG;
    if (!strcmp(s, OUTPUT_TERM)) return OUTPUT_SYM_ASG;
    if (!strcmp(s, INOUT_TERM)) return INOUT_SYM_ASG;
    if (!strcmp(s, GENERIC_BLOCK)) return BLOCK_ASG;
    if (!strcmp(s, GATE_NMOS_STR)) return NMOS_ASG;
    if (!strcmp(s, GATE_PMOS_STR)) return PMOS_ASG;
    if (!strcmp(s, GATE_NMOS_4_STR)) return NMOS_4_ASG;
    if (!strcmp(s, GATE_PMOS_4_STR)) return PMOS_4_ASG;
    if (!strcmp(s, GATE_VAMP_STR)) return VSOURCE_ASG;
    if (!strcmp(s, GATE_IAMP_STR)) return ISOURCE_ASG;
    if (!strcmp(s, GATE_MSFET_STR)) return PMOS_ASG;
    else return(UNKNOWN_ASG);
}
/*----------------------------------------------------------------------*/

char *toString(int mt) {
	char *rv;
	switch(mt) {
	    case UNKNOWN_ASG : rv = strdup("UNKNOWN"); break;
	    case CAPACITOR_ASG : rv = strdup("CAPACITOR"); break;
	    case INDUCTOR_ASG: rv = strdup("INDUCTOR"); break;
	    case RESISTOR_ASG: rv = strdup("RESISTOR"); break;
	    case NMOS_ASG : rv = strdup("NMOS"); break;
	    case PMOS_ASG : rv = strdup("PMOS"); break;
	    case PMOS_4_ASG : rv = strdup("PMOS_4"); break;
 	    case NMOS_4_ASG :rv = strdup("NMOS_4");	break;
	    case VSOURCE_ASG : rv = strdup("VSOURCE"); break;
	    case ISOURCE_ASG : rv = strdup("ISOURCE");  break;
	    case NOT_ASG: rv = strdup("NOT"); break;
	    case INVNOT_ASG : rv = strdup("INVNOT"); break;
	    case AND_ASG : rv = strdup("AND"); break;
	    case NAND_ASG : rv = strdup("NAND"); break;
	    case OR_ASG : rv = strdup("OR"); break;
	    case NOR_ASG : rv = strdup("NOR"); break;
 	    case XOR_ASG : rv = strdup("XOR"); break;
	    case XNOR_ASG : rv = strdup("XNOR"); break;
            case BUFFER_ASG : rv = strdup("BUFFER"); break;
	    case INPUT_SYM_ASG : rv = strdup("IN");	break;
            case OUTPUT_SYM_ASG : rv = strdup("OUT"); break;
            case INOUT_SYM_ASG: rv = strdup("INOUT"); break;
	    case BLOCK_ASG : rv = strdup("BLOCK"); break;
	    others:
		rv = strdup("UNKNOWN");
	  	break;
	}
	return(rv);
}
/* ------------------------------------------------------------------------
 * new_asg_module()
 * create a new object/module and install it in the global module list
 * ------------------------------------------------------------------------
 */
//
// module *new_asg_module(char *module_name, int gate_type)
// {
//     module *object;
// 
//     object = (module *)calloc(1, sizeof(module));
//     object->index = module_count++;
//     object->name = strdup(gate_name);
//     object->type = toString(gate_type);
//     object->x_pos = 0;
//     object->y_pos = 0;    
//     object->flag = UNSEEN;
//     object->placed = UNPLACED;
//     object->rot = LEFT;		/* assme no rotation */
//     object->primary_in = NULL;
//     object->primary_out = NULL;
//     object->terms = NULL;
//     object->fx = create_var_struct(ADD, NULL, NULL);
//     object->fy = create_var_struct(ADD, NULL, NULL);
//     set_default_object_size(object->type, &object->x_size, &object->y_size);
//     
//     modules = (mlist *)concat_list(object, modules);
//     return(object);
// }


/*--------------------------------------------------------------------------------- */

/* ModuleType validate_module_type(s) */
int validate(s)
    char *s;
{			
    /* SEE string definitions in "network.h" */
    if (!strcmp(s, GATE_AND_STR)) return AND;
    if (!strcmp(s, GATE_OR_STR)) return OR;
    if (!strcmp(s, GATE_NAND_STR)) return NAND;
    if (!strcmp(s, GATE_NOR_STR)) return NOR;
    if (!strcmp(s, GATE_PMOS_STR)) return PMOS;
    if (!strcmp(s, GATE_CAPC_STR)) return CAPC;
    if (!strcmp(s, GATE_RESTR_STR)) return RESTR;
    if (!strcmp(s, GATE_INDR_STR)) return INDR;
    if (!strcmp(s, GATE_XOR_STR)) return XOR;
    if (!strcmp(s, XNOR_GATE)) return XNOR;
    if (!strcmp(s, GATE_NOT_STR)) return NOT_;
    if (!strcmp(s, GATE_INVNOT_STR)) return INVNOT_;
    if (!strcmp(s, GATE_NULL_STR)) return BUFFER;
    if (!strcmp(s, BUFFER_GATE)) return BUFFER;
    if (!strcmp(s, INPUT_TERM)) return INPUT_SYM;
    if (!strcmp(s, OUTPUT_TERM)) return OUTPUT_SYM;
    if (!strcmp(s, INOUT_TERM)) return INOUT_SYM;
    if (!strcmp(s, GENERIC_BLOCK)) return BLOCK;
    if (!strcmp(s, GATE_NMOS_STR)) return NMOS;
    if (!strcmp(s, GATE_VAMP_STR)) return VAMP;
    if (!strcmp(s, GATE_MSFET_STR)) return MSFET;
    else return(DONT_KNOW);
}
/* Also adjust "set_default_object_size" whenever this is modified. */

/*------------------------------------------------------------------------ */

int xc_print_asg_module(XCWindowData *areastruct, module *m)
{
    int type, n, i;
    float arcLen, noOfExtensions, fractional, theta, mid_y, mid_x;
    
    if (m != NULL)
    {
	type = validate(m->type);

	xc_print_sym(areastruct, m->type, m->name, m->x_pos,  m->y_pos,
			m->x_size, m->y_size, m->rot);

	if ((type == BLOCK) || (type == DONT_KNOW)) xc_attach_outerms(areastruct, m);
	xc_attach_interms(areastruct, m);


	/* Special case stuff for mulit-terminal ANSI AND, NAND, OR, NOR, XOR, and
	   XNOR gates: */

	if ((m->y_size > ICON_SIZE) && (type != BLOCK) && (type != DONT_KNOW))
	{
	    if ((type == AND) || (type == NAND))
	    {   /* AND family - display a line to connect all inputs to the icon */
		mid_x = (float)m->x_pos + (float)m->x_size / 2.0;

		//code to be attached 
	    }

	    else if ((type == OR) || (type == NOR))
	    {   /* OR family -- Add line to the 1st Back Curve:  */
		mid_x = (float)m->x_pos + (float)m->x_size/2.0 - 0.4;
		arcLen = 18.8232;
		noOfExtensions = ((float)m->y_size - arcLen)/(2.0 * arcLen);
		n = (int)noOfExtensions;
		fractional = (noOfExtensions - (float)n) * arcLen;
		for (i = n; i >= 0; i--)
		{	/* put down one arc, or fraction thereof */
		    theta = (i > 0) ? 28.072 : 
		            (180.0/M_PI) * asin(fractional/20) - 28.072;
		    if (theta > (-28.072 * 1.10))	/* 10% margin */
		    {
			mid_y = (float)m->y_pos + (float)m->y_size/2.0 + 
			        (float)(n - i + 1) * arcLen;
			
			mid_y = (float)m->y_pos + (float)m->y_size/2.0 - 
			        (float)(n - i + 1) * arcLen;
			//code to be attached
		    }
		}
	    }
	    else if ((type == XOR) || (type == XNOR))
	    {   /* OR family -- Add line to the 2nd Back Curve:  */
		mid_x = (float)m->x_pos + (float)m->x_size/2.0;
		arcLen = 18.8232;
		noOfExtensions = ((float)m->y_size - arcLen)/(2.0 * arcLen);
		n = (int)noOfExtensions;
		fractional = (noOfExtensions - (float)n) * arcLen;
		for (i = n; i >= 0; i--)
		{	/* put down one arc, or fraction thereof */
		    theta = (i > 0) ? 28.072 : 
		            (180.0/M_PI) * asin(fractional/20) - 28.072;
		    if (theta > (-28.072 * 1.10))	/* 10% margin */
		    {
			mid_y = (float)m->y_pos + (float)m->y_size/2.0 + 
			        (float)(n - i + 1) * arcLen;
			/*fprintf(f, "newpath %f %f %f %f %f arc stroke\n", 
				mid_x - 31.5, mid_y, 20.0, 
				-28.072, theta);*/
			mid_y = (float)m->y_pos + (float)m->y_size/2.0 - 
			        (float)(n - i + 1) * arcLen;
			//code to be attached
		    }
		}
	    }
	    /* height of arc in Y is 2*rad*sin(28.072) = 40*.47058 = 
	       18.8232. */
	}
    }
}


