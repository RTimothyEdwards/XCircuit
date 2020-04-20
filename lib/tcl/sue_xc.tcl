#------------------------------------------
# sue_xc.tcl
#------------------------------------------
# This script should be sourced into
# XCircuit and provides the capability to 
# translate .sue files into XCircuit
# schematics.  This script works properly
# with XCircuit version 3.10.21 or newer.
#------------------------------------------
# The primary routine is "make_all_sue",
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
# Updated 4/17/2020:  Stopped hard-coding the primitive
# devices in favor of making a "sue.lps" library.
#------------------------------------------------------------

global xscale
global sscale
# sue puts things on grids of 10, xcircuit on grids of 16.
set xscale 16
set sscale 10

#------------------------------------------------------------
# scale an {x y} list value from SUE units to XCircuit units
#------------------------------------------------------------

proc scale_coord {coord} {
   global xscale
   global sscale
   set x [lindex $coord 0]
   set y [lindex $coord 1]
   set x2 [expr {int(($x * $xscale) / $sscale)}]
   set y2 [expr {int((-$y * $xscale) / $sscale)}]
   set newc [lreplace $coord 0 1 $x2 $y2]
   return $newc
}

namespace eval sue {
    namespace export make make_wire make_line make_text

    #------------------------------------------------------------
    # make and make_wire: create the schematic elements
    #------------------------------------------------------------

    proc make {type args} {

	if {[llength $args] == 1} {
	    set args [lindex $args 0]
	}

	# Default values
	# Note that the inverted Y axis reverses the meaning of Y in the
	# orientations.

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
			    # defaults
			    set flipped {}
			    set angle 0
			}
		    }
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
	    input -
	    output -
	    inout -
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
		    deselect selected
		}
	    }
	}
	deselect selected
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
	# Scale the origin from SUE units to XCircuit units
	set s1 [scale_coord [list $x1 $y1]]
	set s2 [scale_coord [list $x2 $y2]]
	polygon make 2 $s1 $s2
    }

    proc make_line {args} {
	eval "make_wire $args"
    }
}

#------------------------------------------------------------
# icon_*: create the symbol
#------------------------------------------------------------

#------------------------------------------------------------
# default parameters (deferred)
#------------------------------------------------------------

proc icon_setup {icon_args params} {
    puts stdout "icon_setup $icon_args $params"

    foreach pair $params {
        set key [lindex $pair 0]
        set value [lindex $pair 1]
        if {$value == {}} {set value ""}
        switch -- $key {
	    origin -
	    orient {
		# Do nothing for now.  These are library instance values
		# in xcircuit, and could be set as such.
	    }
	    default {
		parameter make substring $key [list [list Text $value]]
	    }
        }
    }
}

#------------------------------------------------------------
# pins
#------------------------------------------------------------

proc icon_term {args} {
    puts stdout "icon_term $args"
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
    set newtext [label make pin $name [scale_coord $origin]]
    label $newtext anchor center
    label $newtext anchor middle
    deselect selected
}

#------------------------------------------------------------
# instance parameters and symbol text labels
#------------------------------------------------------------

proc icon_property {args} {

    puts stdout "icon_property $args"

    set name {}
    set origin {0 0}
    set proptype {}
    set lscale 0.7

    foreach {key value} $args {

	switch -- $key {
	    -origin {
		set origin $value
	    }
	    -name {
		set lhandle [label make normal [list [list Parameter $value]] [scale_coord $origin]]
		label $lhandle anchor center
		label $lhandle anchor middle
		label $lhandle scale $lscale
		deselect selected
	    }
	    -type {
		set proptype $value
	    }
	    -size {
		# label size.  Ignore, for now.
		switch -- $value {
		    -small {
			set lscale 0.5
		    }
		    -large {
			set lscale 0.9
		    }
		    default {
			set lscale 0.7
		    }
		}
	    }
	    -label {
		set lhandle [label make normal "$value" [scale_coord $origin]]
		label $lhandle anchor center
		label $lhandle anchor middle
		label $lhandle scale $lscale
		deselect selected
	    }
	}
    }
}
   
#------------------------------------------------------------
# Line drawing on the symbol
#------------------------------------------------------------

proc icon_line {args} {
    puts stdout "icon_line $args"
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
# Recast schematic commands in a namespace used for a
# preliminary parsing to discover dependencies
#------------------------------------------------------------

namespace eval parse {
    namespace export make make_wire make_line make_text

    proc make {type args} {
	global deplist

	switch -- $type {
	    input -
	    output -
	    inout -
	    name_net -
	    name_net_s -
	    global -
	    text {
	    }
	    default {
		lappend deplist $type
	    }
	}
    }

    proc make_line {args} {
    }

    proc make_wire {x1 y1 x2 y2} {
    }

    proc make_text {args} {
    }
}

#------------------------------------------------------------
# Main routine:  Load the .sue file for the indicated
# gate.  Draw the schematic and the (user library) symbol,
# and associate them.
#------------------------------------------------------------

proc make_sue_gate {filename libname} {
   global deplist

   set name [file tail [file root $filename]]

   # Check if this gate exists and ignore if so (may have been
   # handled already as a dependency to another gate)
   if {![catch {object handle ${name}}]} {return}

   # DIAGNOSTIC
   puts stdout "Sourcing ${filename}"
   source ${filename}

   set deplist {}

   # DIAGNOSTIC
   puts stdout "Evaluating SCHEMATIC_${name} in namespace parse"
   namespace import parse::*
   eval "SCHEMATIC_${name}"

   if {[llength $deplist] > 0} {
       # DIAGNOSTIC
       puts stdout "Handling dependency list."
       foreach dep $deplist {
	   make_sue_gate ${dep}.sue $libname
       }
   }

   # DIAGNOSTIC
   puts stdout "Generating new page"

   # Go to a new page unless the current one is empty
   while {[llength [object parts]] > 0} {
      set p [page]
      incr p
      while {[catch {page $p}]} {
	 page make
      }
   }

   puts stdout "Evaluating ICON_${name}"
   namespace forget parse::*
   namespace import sue::*

   # Evaluate the symbol.  Generate the symbol in xcircuit.
   # Then clear the page to make the schematic
   eval "ICON_${name}"
   set hlist [object parts]
   object make $name $hlist
   set hlist [object parts]
   push $hlist
   pop
   delete $hlist

   # DIAGNOSTIC
   puts stdout "Evaluating SCHEMATIC_${name} in namespace sue"

   eval "SCHEMATIC_${name}"
   catch {wm withdraw .select}
   schematic associate $name
   zoom view

   # DIAGNOSTIC
   puts stdout "Done."
   namespace forget sue::*
}
 
#------------------------------------------------------------
# Read a .sue file and source it, then format a page around
# the schematic contents.
#------------------------------------------------------------

proc read_sue_file {filename name} {
    config suspend true
    make_sue_gate $filename $name
    page filename $name
    page orientation 90
    page encapsulation full
    page fit true
    if {[page scale] > 1.0} {
	page fit false
        page scale 1.0
    }
    config suspend false
}

#------------------------------------------------------------
# Top-level routine:  Find all the .sue files in the
# current directory and generate a library from them
#------------------------------------------------------------

proc make_all_sue {{name sue_gates}} {
   set files [glob \*.sue]

   foreach filename $files {
      read_sue_file $filename $name
   }
}

#------------------------------------------------------------
# Make sure that the sue technology (.lps file) has been read
# 
#------------------------------------------------------------

if {[lsearch [technology list] sue] < 0} {
   library load sue
   technology prefer sue
}
