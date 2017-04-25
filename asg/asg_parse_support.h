#ifndef _ASG_PARSE_SUPPORT
#define _ASG_PARSE_SUPPORT

#include <stdarg.h>

#ifdef _USING_CC
/* for use with g++ code: */
	extern "C" void Add2TermModule(char*,char*,char*,char*);
	extern "C" void Add3TermModule(char*,char*,char*,char*,char*);
	extern "C" void AddNTermModule(char*,char*,int, ...);
	extern "C" void AddModuleTerm(char*,char*,int,int);
#else
/* for use with standard C code: */

	void Add2TermModule(char *name,char *type,char *inNode,char *outNode);
	void Add3TermModule(char *name,char *type,char *inNode,char *outNode, char *inoutNode);
	void AddNTermModule(char *name, char *type, int N, ...);
	void AddModuleTerm(char *type, char *netName, int portnum, int allports);

/* Assumes the variable arguments form a list of nets corresponding to 
 * each terminal of the module <name> of the given <type>.
 */

#endif
#endif
