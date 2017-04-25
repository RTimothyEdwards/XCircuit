#------------------------------------------
# sue_xc.tcl
#------------------------------------------
# This script should be sourced into
# XCircuit and provides the capability to 
# translate .sue files into XCircuit
# schematics.  This script works properly
# with XCircuit version 3.2.24 or newer.
#------------------------------------------
# The primary routine is "make_sue_all",
# which is a TCL procedure to be run from
# the XCircuit command line in the directory
# containing .sue format files.  Without
# options, it creates a single XCircuit
# PostScript output file "sue_gates.ps",
# containing all of the gate symbols and
# associated schematics in a single file.
#------------------------------------------
# Written by R. Timothy Edwards
# 8/23/04
# MultiGiG, Inc.
# Scotts Valley, CA
#------------------------------------------

global fscale
set fscale 1.6

#------------------------------------------------------------
# scale an {x y} list value from SUE units to XCircuit units
#------------------------------------------------------------

proc scale_coord {coord} {
   global fscale
   set x [lindex $coord 0]
   set y [lindex $coord 1]
   set newc [lreplace $coord 0 1 [expr {int($x * $fscale)}] \
	[expr {int(-$y * $fscale)}]]
   return $newc
}

#------------------------------------------------------------
# make and make_wire: create the schematic elements
#------------------------------------------------------------

proc make {type args} {
   global fscale

   if {[llength $args] == 1} {
      set args [lindex $args 0]
   }

   # default values
   set flipped {}
   set angle 0
   set width 1
   set length 1
   set name bad_element
   set origin {0 0}
   set instance_params {}

   foreach {key value} $args {
      switch -- $key {
	 -orient {
	    switch -- $value {
	       RXY {
		 set angle 180
	       }
	       RX {
		 set flipped horizontal
	       }
	       RY {
		 set flipped vertical
		 set angle 180
	       }
	       R270 {
		 set angle 270
	       }
	       R90 {
		 set angle 90
	       }
	       R0 {
		 # do nothing
	       }
	    }
	 }
	 -W {
	    set width $value
	 }
	 -L {
	    set length $value
	 }
	 -origin {
	    set origin $value
	 }
	 -name {
	    set name $value
	 }
	 -text {
	    set name $value
	 }
	 default {
	    lappend instance_params [list [string range $key 1 end] $value]
	 }
      }
   }

   set origin [scale_coord $origin]

   switch -- $type {
      pmos {
	 set newgate [instance make pMOS $origin]
	 # SUE pMOS is wider than xcircuit pMOS.
	 set x1 [lindex $origin 0]
	 set s1 [lreplace $origin 0 0 [expr {$x1 - 64}]]
	 set s2 [lreplace $origin 0 0 [expr {$x1 - 96}]]
	 set newpoly [polygon make 2 $s1 $s2]
 	 select $newgate
	 parameter set width $width -forward
	 parameter set length $length -forward
	 select [list $newgate $newpoly]
 	 rotate $angle $origin
 	 if {$flipped != {}} {
	    select [list $newgate $newpoly]
 	    flip $flipped $origin
 	 }
      }

      nmos {
	 set newgate [instance make nMOS $origin]
	 # SUE nMOS is wider than xcircuit nMOS.
	 set x1 [lindex $origin 0]
	 set s1 [lreplace $origin 0 0 [expr {$x1 - 64}]]
	 set s2 [lreplace $origin 0 0 [expr {$x1 - 96}]]
	 set newpoly [polygon make 2 $s1 $s2]
	 select $newgate
	 parameter set width $width -forward
	 parameter set length $length -forward
	 select [list $newgate $newpoly]
 	 rotate $angle $origin
 	 if {$flipped != {}} {
	    select [list $newgate $newpoly]
 	    flip $flipped $origin
 	 }
      }

      input -
      output -
      name_net -
      name_net_s {
	 set newtext [label make pin $name $origin]
 	 rotate $newtext $angle $origin
 	 if {$flipped != {}} {
 	    flip $newtext $flipped $origin
 	 }
      }

      global {
	 set newtext [label make global $name $origin]
 	 rotate $newtext $angle $origin
 	 if {$flipped != {}} {
 	    flip $newtext $flipped $origin
 	 }
      }

      text {
	 set newtext [list $name]
	 while {[set rp [string first \n $newtext]] >= 0} {
	    set newtext [string replace $newtext $rp $rp "\} \{return\} \{"]
	    set rp [string first \n $newtext]
	 }
	 set newtext [label make normal $newtext $origin]
 	 rotate $newtext $angle $origin
 	 if {$flipped != {}} {
 	    flip $newtext $flipped $origin
 	 }
      }

      # Default behavior is to generate an object instance of the
      # given name.  This assumes that these are only objects that
      # have been defined in .sue files already.

      default {
	 set newgate [instance make $type $origin]
	 select $newgate
 	 rotate $angle $origin
 	 if {$flipped != {}} {
	    select $newgate
 	    flip $flipped $origin
 	 }
	 if {$instance_params != {}} {
	    select $newgate
	    foreach pair $instance_params {
	       set key [lindex $pair 0]
	       set value [lindex $pair 1]
	       parameter set $key $value -forward
	    }
	    deselect
	 }
      }
   }
   deselect
}

#------------------------------------------------------------
# Draw text on the schematic
#------------------------------------------------------------

proc make_text {args} {
   make text $args
}

#------------------------------------------------------------
# Draw a wire into the schematic
#------------------------------------------------------------

proc make_wire {x1 y1 x2 y2} {
   global fscale
   # Scale the origin from SUE units to XCircuit units
   set sx1 [expr {int($x1 * $fscale)}]
   set sy1 [expr {int(-$y1 * $fscale)}]
   set sx2 [expr {int($x2 * $fscale)}]
   set sy2 [expr {int(-$y2 * $fscale)}]
   polygon make 2 [list $sx1 $sy1] [list $sx2 $sy2]
}

proc make_line {args} {
   eval "make_wire $args"
}

#------------------------------------------------------------
# icon_*: create the symbol
#------------------------------------------------------------

#------------------------------------------------------------
# default parameters (deferred)
#------------------------------------------------------------

proc icon_setup {icon_args params} {
   global icon_params
   set icon_params [concat $icon_params $params]
}

#------------------------------------------------------------
# pins
#------------------------------------------------------------

proc icon_term {args} {
   set pintype "no_pin"
   set origin {0 0}
   set name "bad_pin_name"

   foreach {key value} $args {
      switch -- $key {
	 -type {
	    set pintype $value
	 }
	 -origin {
	    set origin $value
	 }
	 -name {
	    set name $value
	 }
      }
   }
   set newtext [label make pin $name $origin]
}

#------------------------------------------------------------
# instance parameters and symbol text labels
#------------------------------------------------------------

proc icon_property {args} {

   set proptype {}
   set origin {0 0}
   set name "bad_parameter"

   foreach {key value} $args {
      switch -- $key {
	 -origin {
	    set origin $value
         }
	 -name {
	    set name $value
	 }
	 -type {
	    set proptype $value
	 }
	 -size {
	    # label size.  Ignore, for now.
	 }
	 -label {
	    label make normal "$value" [scale_coord $origin]
	 }
      }
   }
}
   
#------------------------------------------------------------
# Line drawing on the symbol
#------------------------------------------------------------

proc icon_line {args} {
   set coords {}
   set i 0
   foreach {x y} $args {
      set s [scale_coord [list $x $y]]
      lappend coords $s
      incr i
   }
   eval "polygon make $i $coords"
}

#------------------------------------------------------------
# Main routine:  Load the .sue file for the indicated
# gate.  Draw the schematic and the (user library) symbol,
# and associate them.
#------------------------------------------------------------

proc make_sue_gate {name} {
   global icon_params
   source ${name}.sue

   # Go to a new page unless the current one is empty
   while {[llength [object parts]] > 0} {
      set p [page]
      incr p
      while {[catch {page $p}]} {
	 page make
      }
   }

   # Evaluate the symbol.  Generate the symbol in xcircuit.
   # Then clear the page to make the schematic
   set icon_params {}
   eval "ICON_${name}"
   set hlist [object parts]
   object make $name $hlist
   set hlist [object parts]
   push $hlist
   foreach pair $icon_params {
      set key [lindex $pair 0]
      set value [lindex $pair 1]
      switch -- $key {
	 origin -
	 orient {
	    # Do nothing for now.  These are library instance values
	    # in xcircuit, and could be set as such.
	 }
	 default {
	    parameter make substring $key [list $value]
	 }
      }
   }
   pop
   delete $hlist

   eval "SCHEMATIC_${name}"
   wm withdraw .select
   schematic associate $name
   zoom view
}
 
#------------------------------------------------------------
# Top-level routine:  Find all the .sue files in the
# current directory and generate a library from them
#------------------------------------------------------------

proc make_all_sue {{name sue_gates}} {
   set files [glob \*.sue]
   foreach i $files {
      set filename [file tail [file root $i]]
      make_sue_gate $filename
      page filename $name
      page orientation 90
      page encapsulation full
      page fit true
      if {[page scale] > 1.0} {
	 page fit false
         page scale 1.0
      }
   }
}
