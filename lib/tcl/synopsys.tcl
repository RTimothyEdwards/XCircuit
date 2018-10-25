#------------------------------------------------------------------------
# Synopsys symbol library file parser for XCircuit
#------------------------------------------------------------------------
# Written by Tim Edwards, Open Circuit Design
#------------------------------------------------------------------------
# Revision history:
#   Revision 0:  October 23, 2018 by Tim Edwards
#	copied from edif.tcl and modified.
#------------------------------------------------------------------------

set XCOps(module,synopsys) 1

# "Standard" scaling:  Factor of 32 is typical.

set Opts(scalen) 32
set Opts(scaled) 1

#------------------------------------------------------------------------
# Reverse an xcircuit rotation.
#------------------------------------------------------------------------

proc rotinvert {value} {
   set newvalue [expr {360 - $value}]
   if {$newvalue == 360} {set newvalue 0}
   return $newvalue
}

#------------------------------------------------------------------------
# Scale a value by factor (scalen) / (scaled)
#------------------------------------------------------------------------

proc scaleval {value} {
   global Opts
   return [expr {int(0.5 + $value * $Opts(scalen) / $Opts(scaled))}]
}

#------------------------------------------------------------------------
# Convert arc from center-endpoint-endpoint to center-radius-endpoint
# angles
#------------------------------------------------------------------------

proc convert_arc {x1 y1 x2 y2 cx cy} {

   set r [expr {sqrt(($cx - $x1) * ($cx - $x1) + ($cy - $y1) * ($cy - $y1))}]

   set a1 [expr {atan2($y1 - $cy, $x1 - $cx)}]
   set a2 [expr {atan2($y2 - $cy, $x2 - $cx)}]

   set a1 [expr {$a1 * 180 / 3.1415927}]
   set a2 [expr {$a2 * 180 / 3.1415927}]

   return [list $cx $cy $r $a1 $a2]
}

#------------------------------------------------------------------------
# Quiet deselection
#------------------------------------------------------------------------

proc quiet_deselect {} {
   set handle [select get]
   if {$handle != {}} {deselect}
}

#------------------------------------------------------------------------
# Evaluate the argument as an expression, using name/value pairs from the
# dictionary libdict.
#------------------------------------------------------------------------

proc eval_arg {argument libdict} {
    dict for {key value} $libdict {
	set argument [eval [subst {string map {$key $value} \$argument}]]
    }
    set value [expr $argument]
    return $value
}

#------------------------------------------------------------------------
# Trim space from either end of the argument and return it.
#------------------------------------------------------------------------

proc parse_arg {argument} {
    set argument [string trimright $argument]
    set argument [string trimleft $argument]
    return $argument
}

#------------------------------------------------------------------------
# Parse a set statement 
#	name = value ;
# or 
#	name : value
#
# Add the name/value pair to the dictionary "libdict".  Note that the
# value may be an expression using values from other dictionary keys.
# The expression is assumed to be parseable by "eval" (probably true).
#------------------------------------------------------------------------

proc parse_set_statement {keyword libdict statement} {
    set statement [parse_arg $statement]
    set statement [string trimright $statement ";"]
    set statement [string trimleft $statement =]
    dict for {key value} $libdict {
	set statement [eval [subst {string map {$key $value} \$statement}]]
    }
    dict set libdict $keyword [expr $statement]
    return $libdict
}

#------------------------------------------------------------------------
# Like the parse_set_statement routine, but values are names and
# do not involve calculations.  The dictionary is separate.
#------------------------------------------------------------------------

proc parse_assign_statement {keyword namedict statement} {
    set statement [parse_arg $statement]
    set statement [string trimleft $statement :]
    set statement [string trimleft $statement]
    dict set namedict $keyword $statement
    return $namedict
} 

#------------------------------------------------------------------------
# Parse a function, which has arguments in parentheses.  Parse arguments
# according to the function name.
#------------------------------------------------------------------------

proc parse_function {funcname libdict argstring} {
    set argstring [parse_arg $argstring]
    set argstring [string trimright $argstring ";"]
    set argstring [string trimright $argstring]
    set argstring [string trimright $argstring )]
    set argstring [string trimleft $argstring (]
    set arguments [split $argstring ,]

    switch $funcname {
	set_route_grid {
	}
	set_meter_scale {
	}
	set_minimum_boundary {
	    set x1 [scaleval [eval_arg [lindex $arguments 0] $libdict]]
	    set y1 [scaleval [eval_arg [lindex $arguments 1] $libdict]]
	    set x2 [scaleval [eval_arg [lindex $arguments 2] $libdict]]
	    set y2 [scaleval [eval_arg [lindex $arguments 3] $libdict]]
	    set handle [polygon make box [list $x1 $y1] [list $x2 $y2]]
	    polygon $handle border bbox
	    deselect
	}
	pin {
	    set pinname [parse_arg [lindex $arguments 0]]
	    set pinx [scaleval [eval_arg [lindex $arguments 1] $libdict]]
	    set piny [scaleval [eval_arg [lindex $arguments 2] $libdict]]
	    set rotation [parse_arg [lindex $arguments 3]]
	    # puts stdout "Make pin $pinname"
	    set handle [label make pin $pinname [list $pinx $piny]]
	}
	line {
	    set x1 [scaleval [eval_arg [lindex $arguments 0] $libdict]]
	    set y1 [scaleval [eval_arg [lindex $arguments 1] $libdict]]
	    set x2 [scaleval [eval_arg [lindex $arguments 2] $libdict]]
	    set y2 [scaleval [eval_arg [lindex $arguments 3] $libdict]]
	    # puts stdout "Make line $x1 $y1 $x2 $y2"
	    set handle [polygon make 2 [list $x1 $y1] [list $x2 $y2]]
	}
	arc {
	    set x1 [eval_arg [lindex $arguments 0] $libdict]
	    set y1 [eval_arg [lindex $arguments 1] $libdict]
	    set x2 [eval_arg [lindex $arguments 2] $libdict]
	    set y2 [eval_arg [lindex $arguments 3] $libdict]
	    set cx [eval_arg [lindex $arguments 4] $libdict]
	    set cy [eval_arg [lindex $arguments 5] $libdict]
	    # puts stdout "Convert arc $x1 $y1 $x2 $y2 $cx $cy"
	    if {![catch {set xca [convert_arc $x1 $y1 $x2 $y2 $cx $cy]}]} {
	       set x [scaleval [lindex $xca 0]]
	       set y [scaleval [lindex $xca 1]]
	       set r [scaleval [lindex $xca 2]]
	       set a1 [lindex $xca 3]
	       set a2 [lindex $xca 4]
	       # puts stdout "Make arc $x $y $r $a1 $a2"
	       set handle [arc make [list $x $y] $r $a1 $a2]
	   }
	}
	circle {
	    set x1 [scaleval [eval_arg [lindex $arguments 0] $libdict]]
	    set y1 [scaleval [eval_arg [lindex $arguments 1] $libdict]]
	    set r  [scaleval [eval_arg [lindex $arguments 2] $libdict]]
	    # puts stdout "Make circle $x1 $y1 $r"
	    set handle [arc make [list $x1 $y1] $r]
	}
    }
}

#------------------------------------------------------------------------
# Main file reader routine.  Read the file into a list.
#------------------------------------------------------------------------

proc read_synopsys {filename} {

   if [catch {open $filename r} fileIn] {
      puts stderr "Cannot find file $filename"
      return;
   }

   # Parse the file line-by-line, removing C-style /* ... */ comments
   # and removing line-continuation backslashes.  After reading scale
   # and snap values from the header, generate symbols as they are
   # encountered.

   set everything [read $fileIn]
   # Remove C-style comments
   regsub -all {[/][*].*?[*][/]} $everything "" everything
   # Remove line-continuation backslashes.
   set masterlist [string map {"\\\n" " "} $everything]
   unset everything
   set linedata [split $masterlist \n]
   unset masterlist
   close $fileIn

   # Set up xcircuit for processing

   set cpage [page]
   config suspend true
   set in_library false
   set in_symbol false

   # Now parse the file. . .

   set libname ""
   set libdict [dict create]
   set namedict [dict create]
   set lineno 0
   set cpage 1
   foreach line $linedata {
      incr lineno
      if {![regexp {[ \t]*([^ \t\n"\(=]+)(.*)} $line lmatch keyword rest]} {continue}
      if {!$in_library} {
         switch $keyword {
	    library {
               regexp {\([ \t]*["]?[ \t]*([^ \t"]+)[ \t]*["]?[ \t]*\)} $rest lmatch libname
               # If 'libname' is a filename w/extension, then remove the extension
	       set libname [file root $libname]
	       set in_library true
	    }
	    default {
	       puts "Unsupported keyword $keyword"
	    }
         }
      } elseif {!$in_symbol} {
         switch $keyword {
	    symbol {
               regexp {\([ \t]*["]?[ \t]*([^ \t"]+)[ \t]*["]?[ \t]*\)} $rest lmatch symbolname
	       set in_symbol true

	       catch {object [object handle $symbolname] name "_$symbolname"}
	       set handle [object make ${libname}::${symbolname} -force]
	       if {$handle == ""} {
		  puts "Error:  Couldn't create new object!"
		  return
	       }
	       set cpage [page]
	       push $handle
	       symbol type fundamental
	    }
	    logic_1_symbol -
	    logic_0_symbol -
	    in_port_symbol -
	    out_port_symbol -
	    inout_port_symbol {
	       set namedict [parse_assign_statement $keyword $namedict $rest]
	    }
	    SYMBOLU_PER_UU -
	    METER_PER_UU -
	    ROUTE_GRID -
	    SNAP_SPACE -
	    SCALE -
	    METER_SCALE {
	       set libdict [parse_set_statement $keyword $libdict $rest]
	    }
	    set_route_grid -
	    set_meter_scale {
		parse_function $keyword $libdict $rest
	    }
	    \} {
	       set in_library false
	    }
	    default {
	       puts "Unsupported keyword $keyword in library definition"
	    }
	 }
      } else {
         switch $keyword {
	    set_minimum_boundary -
	    pin -
	    line -
	    arc -
	    circle {
		parse_function $keyword $libdict $rest
	    }
	    \} {
	       set in_symbol false
	       pop
	       if [catch {delete $handle}] {
		  page $cpage
		  if [catch {delete $handle}] {
		     puts "Error:  Element handle $handle does not exist?"
		     puts "Page objects: [object parts]"
		  }
	       }
	    }
	    default {
	       puts "Unsupported keyword $keyword in symbol definition"
	    }
	 }
      }
   }
   unset linedata

   # Recompute bounding box on all library objects
   puts stdout "Computing symbol bounding boxes"
   library "User Library" goto
   set liblist [library "User Library" handle]
   foreach handle $liblist {
      if {[element $handle type] == "Object Instance"} {
	 instance $handle bbox recompute
      }
   }

   # Regenerate the library
   puts stdout "Composing library page"
   library "User Library" compose

   # Return to (and redraw) original page
   page $cpage
   zoom view
   config suspend false
}

#------------------------------------------------------------------------
# Procedure that creates the dialog to find a Synopsys symbol library
# file to parse and calls procedure read_synopsys.
#---------------------------------------------------------------------------

proc xcircuit::promptreadsynopsys {} {
  global XCOps
  .filelist.bbar.okay configure -command \
        {read_synopsys [.filelist.textent.txt get] ; \
        wm withdraw .filelist}
  .filelist.listwin.win configure -data "slib synopsys"
  .filelist.textent.title.field configure -text \
	"Select Synopsys symbol library file to parse:"
  .filelist.textent.txt delete 0 end
  xcircuit::popupfilelist
  xcircuit::removelists .filelist
}
