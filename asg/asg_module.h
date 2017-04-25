/* asg_module.h */

#ifndef _ASG_MODULE_H
#define _ASG_MODULE_H

/* The "ModuleType" enum represents the types of modules that are 
 * available in the xcircuit library asgspice.lps
 */
enum ModuleType { UNKNOWN_ASG, CAPACITOR_ASG, INDUCTOR_ASG, RESISTOR_ASG,
    NMOS_ASG, PMOS_ASG, PMOS_4_ASG, NMOS_4_ASG, VSOURCE_ASG, ISOURCE_ASG, 
   NOT_ASG, INVNOT_ASG, AND_ASG, NAND_ASG, OR_ASG, NOR_ASG, XOR_ASG, XNOR_ASG,
   BUFFER_ASG, INPUT_SYM_ASG, OUTPUT_SYM_ASG, INOUT_SYM_ASG, BLOCK_ASG };

/* OLD mod->type string comparisons, used mostly in printing */
#define GATE_AND_STR    "AND"
#define GATE_OR_STR     "OR"
#define GATE_NAND_STR   "NAND"
#define GATE_NOR_STR    "NOR"
#define GATE_XOR_STR    "XOR"
#define GATE_XNOR_STR   "XNOR"
#define GATE_NOT_STR    "NOT"
#define GATE_NMOS_STR   "NMOS"
#define GATE_PMOS_STR	"PMOS"
#define GATE_NMOS_4_STR "NMOS_4"
#define GATE_PMOS_4_STR	"PMOS_4"
#define GATE_CAPC_STR	"CAPC"
#define GATE_RESTR_STR	"RESTR"
#define GATE_INDR_STR	"INDR"
#define GATE_VAMP_STR   "VAMP"
#define GATE_IAMP_STR   "IAMP"
#define GATE_MSFET_STR  "MSFET"
#define GATE_INVNOT_STR "INVNOT"
#define GATE_VSOURCE	"VSOURCE"
#define GATE_ISOURCE	"ISOURCE"
#define GATE_NULL_STR   "NL_GATE"


int xc_print_asg_module(XCWindowData *areastruct, module *m);

char *toString(int mt);
int toModuleType(char *mtName);

#endif
