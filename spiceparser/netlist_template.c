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
/* netlist_template.c, code for building graph matching templates.
   Conrad Ziesler
*/

/* primary goal:

   We want to build a data structure which represents all library graphs merged into one, 
   in such a way that each merge keeps a list of library cells that were merged at that point.
   With such a sturcture, we should be able to decompose a complex netlist into its component pieces.



   Ref1-> cell (nfet_sd) (lib 1,2) -g> 
                                   -sd>

       -> cell (pfet_g)  (lib 3,5) -sd> 
                                   -sd> 


   netlist nodes are common and simple
   netlist devices are complex, shared/merged keeping a list of the library cell associations.



   to decompose a big netlist,
   1. initialize bitmap of library cell numbers
   2. start traversing depth first from some reference.
   2.1. keep track of traversal in template, eliminating at each step cells in the bitmap.
   2.2. keep track of devices and nodes traversed and matched with template in gate internal list.
   2.3. stop traversing at template boundaries.
   3.   examine bitmap.  if any remain, we have successful match.

*/


