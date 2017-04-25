/********************
    This file is part of the software library CADLIB written by Conrad Ziesler
    Copyright 2003, Conrad Ziesler, all rights reserved.

*************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

******************/
/* eval.h, support for evaluation 
   Conrad Ziesler
*/
#ifndef __EVAL_H__
#define __EVAL_H__

typedef enum eval_op_en
{
  EOnone,
  EOadd,
  EOsub,
  EOneg,
  EOmul,
  EOdiv,
  EOmod,
  EOleft,
  EOright,
  EObitxor,
  EObitand,
  EObitor,
  EObitnot,
  EOor,
  EOand,
  EOnot,
  EOlist,
  EOliteral,
  EOlistitem,
  EOlast
}eval_op_t;

#define EVAL_ARGS_OF_OP { 0, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 1, 1 }
#define EVAL_OP_NAMES { "none", "add", "sub", "neg", "mul", "div", "mod", "left", "right", "bitxor", "bitand" \
                        "bitor", "bitnot", "or", "and", "not", "list", "literal", "listitem", "last" }

extern const char *eval_op_names[EOlast+1];
extern const int eval_args_of_op[EOlast];

typedef struct eval_expr_st
{
  eval_op_t op;
  int args[2];
}eval_expr_t;


#endif
