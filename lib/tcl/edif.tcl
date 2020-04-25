#------------------------------------------------------------------------
# EDIF file parser for XCircuit
#------------------------------------------------------------------------
# Written by Tim Edwards, MultiGiG, Inc., Scotts Valley, CA
#------------------------------------------------------------------------
# Revision history:
#   Revision 0:  July 18, 2006 by Tim Edwards
#   Revision 1:  July 24, 2006 by Jan Sundermeyer  
#   Revision 2:  July 30, 2006 by Jan Sundermeyer
#   Revision 3:  July 31, 2006 by Jan Sundermeyer
#------------------------------------------------------------------------

set XCOps(module,edif) 1

# "Standard" scaling:  converts a typical factors-of-10 coordinate system
# into XCircuit's factors-of-16 coordinate system (16/10 = 8/5)

set Opts(scalen) 8
set Opts(scaled) 5

set symbolview {symbol symbolr spectre hspice spectreS}
set subst_list {}

#------------------------------------------------------------------------
# EDIF rotations are the reverse of xcircuit rotations.
#------------------------------------------------------------------------

proc rotinvert {value} {
   set newvalue [expr 360 - $value]
   if {$newvalue == 360} {set newvalue 0}
   return $newvalue
}

#------------------------------------------------------------------------
# Scale a point by factor (scalen) / (scaled)
# "point" is a point in EDIF format:  {pt <x> <y>}
#------------------------------------------------------------------------

proc scalepoint {point} {
   global Opts

   set xin [lindex $point 1]
   set yin [lindex $point 2]

   set xout [expr $xin * $Opts(scalen) / $Opts(scaled)]
   set yout [expr $yin * $Opts(scalen) / $Opts(scaled)]

   return [list $xout $yout]
}

#------------------------------------------------------------------------
# Convert arc 3-point form to center-radius-endpoint angles
# Formula thanks to Yumnam Kirani Singh, found on geocities.com.
# Saved me from having to work it out for myself.
#------------------------------------------------------------------------

proc convert_arc {point1 point2 point3} {
   set x1 [lindex $point1 0]
   set y1 [lindex $point1 1]
   set x2 [lindex $point2 0]
   set y2 [lindex $point2 1]
   set x3 [lindex $point3 0]
   set y3 [lindex $point3 1]

   set m11 [expr $x2 * $x2 + $y2 * $y2 - ($x1 * $x1 + $y1 * $y1)]
   set m21 [expr $y2 - $y1]
   set m12 [expr $x3 * $x3 + $y3 * $y3 - ($x1 * $x1 + $y1 * $y1)]
   set m22 [expr $y3 - $y1]

   set n11 [expr $x2 - $x1]
   set n21 $m11
   set n12 [expr $x3 - $x1]
   set n22 $m12

   set d11 $n11
   set d21 $m21
   set d12 $n12
   set d22 $m22

   set absm [expr ($m11 * $m22) - ($m12 * $m21)]
   set absn [expr ($n11 * $n22) - ($n12 * $n21)]
   set absd [expr ($d11 * $d22) - ($d12 * $d21)]

   set cx [expr $absm / (2.0 * $absd)]
   set cy [expr $absn / (2.0 * $absd)]

   set r [expr sqrt(($cx - $x2) * ($cx - $x2) + ($cy - $y2) * ($cy - $y2))]

   set a1 [expr atan2($y1 - $cy, $x1 - $cx)]
   set a2 [expr atan2($y2 - $cy, $x2 - $cx)]
   set a3 [expr atan2($y3 - $cy, $x3 - $cx)]

   # To be certain that the end angles are correct, make sure a2 is between
   # a1 and a3.

   if {($a1 < $a2) && ($a2 > $a3)} {
	set a3 [expr $a3 + 3.1415927]
   } elseif {($a1 > $a2) && ($a2 < $a3)} {
	set a3 [expr $a3 - 3.1415927]
   }

   set cx [expr round($cx)]
   set cy [expr round($cy)]
   set r [expr round($r)]
   set a1 [expr $a1 * 180 / 3.1415927]
   set a3 [expr $a3 * 180 / 3.1415927]

   return [list [list $cx $cy] $r $a1 $a3]
}

#------------------------------------------------------------------------
# Quiet deselection
#------------------------------------------------------------------------

proc quiet_deselect {} {
   set handle [select get]
   if {$handle != {}} {deselect}
}

#------------------------------------------------------------------------
# Parse a "shape" statement
#------------------------------------------------------------------------

proc parse_shape {elemlist} {
   set handle {}
   set shapedata [lindex $elemlist 0]
   set shapetype [lindex $shapedata 0]

   # Any keywords to handle other than "curve"?
   switch $shapetype {
      curve {
	 foreach phrase [lrange $shapedata 1 end] {
	    # Can a curve have any statement other than "arc"?
	    set curveType [lindex $phrase 0]
	    switch $curveType {
	       arc {
	          set pointlist {}
	          foreach point [lrange $phrase 1 end] {
	             lappend pointlist [scalepoint $point]
		  }
	          set xca [eval "convert_arc $pointlist"]
	          set handle [eval "arc make $xca"]
	       }
	       default {puts "Unknown curve type $curveType"}
	    }
	 }
      }
      default {puts "Unsupported shape type $shapetype"}
   }
   return $handle
}

#------------------------------------------------------------------------
# decimal to octal code conversion
#------------------------------------------------------------------------

proc dec_to_oct {dval} {
   set oval "\\"
   set o3 [expr $dval / 64]
   set r3 [expr $dval % 64]
   set o2 [expr $r3 / 8]
   set o1 [expr $r3 % 8]
   set oval "\\$o3$o2$o1"
}

#------------------------------------------------------------------------
# Parse a raw geometry statement in the EDIF data.
#------------------------------------------------------------------------

proc parse_geometry {elemtype elemdata} {
   global Opts

   quiet_deselect
   switch $elemtype {
      openshape -
      shape {
	 set handle [parse_shape $elemdata]
      }

      path -
      polygon {
	 set polydata [lrange [lindex $elemdata 0] 1 end]
	 set pointlist {}
	 foreach point $polydata {
	    lappend pointlist [scalepoint $point]
	 }
	 set numpoints [llength $pointlist]
	 # puts "polygon make $numpoints $pointlist"
	 set handle [eval "polygon make $numpoints $pointlist"]
	 if {"$elemtype" == "polygon"} {
	    element $handle border closed
	 }
      }
      rectangle {
	 set pointlist {}
	 foreach point $elemdata {
	    lappend pointlist [scalepoint $point]
	 }
	 # puts "polygon make box $pointlist"
	 set handle [eval "polygon make box $pointlist"]
      }
      name -
      stringDisplay {
         # Parse various kinds of strings used by Cadence
	 set dstring [lindex $elemdata 0]
	 if {[lindex $dstring 0]== "array"} {
	    set dstring [subst_name [lindex $dstring 1]]
	 } else {
	    set dstring [subst_name $dstring]
	 }

	 # Do EDIF-format ASCII escape sequence substitution
	 while {[regexp -- {%([0-9]+)%} $dstring temp code] > 0} {
	    set oval [dec_to_oct $code]
	    regsub -- {%[0-9]+%} $dstring [subst "$oval"] dstring
	 }
	 set handle {}
         if {[string first "cds" $dstring] == 0} {
	    switch -glob $dstring {
	       cdsName* {set handle [label make normal [object name] {0 0}]}
	       cdsParam* {set handle {}}
	       cdsTerm* {
	   if {[regexp {"(.+)"} $dstring temp pinname] > 0} {
		     set handle [label make pin $pinname {0 0}]
		  } else {
		     set handle [label make pin $dstring {0 0}]
		  }
	       }
	       default {set handle [label make normal $dstring {0 0}]}
	    }
	    set cds 1
	 } else {
     	    set cds 0
	 }
         foreach dparams [lrange $elemdata 1 end] {
 	 if {$cds == 0} { 
		quiet_deselect
	 	set handle [label make normal $dstring {0 0}]
	 }
	 if {$handle != {}} {
	    foreach dtext $dparams {
	       set dtk [lindex $dtext 0]
	       switch $dtk {
	          anchor {
		     set jval [lindex $dtext 1]
		     switch $jval {
		        UPPERRIGHT {
			    label $handle anchor top
			    label $handle anchor right
			}
		        CENTERRIGHT {
			    label $handle anchor middle
			    label $handle anchor right
			}
		        LOWERRIGHT {
			    label $handle anchor bottom
			    label $handle anchor right
			}
		        UPPERCENTER {
			    label $handle anchor top
			    label $handle anchor center
			}
		        CENTERCENTER {
			    label $handle anchor middle
			    label $handle anchor center
			}
		        LOWERCENTER {
			    label $handle anchor bottom
			    label $handle anchor center
			}
		        UPPERLEFT {
			    label $handle anchor top
			    label $handle anchor left
			}
		        CENTERLEFT {
			    label $handle anchor middle
			    label $handle anchor left
			}
		        LOWERLEFT {
			    label $handle anchor bottom
			    label $handle anchor left
			}
		     }
	          }
	          orientation {	
		     set oval [lindex $dtext 1]
		     set odeg [string range $oval 1 end]
		     element $handle rotate [rotinvert $odeg]
	          }
	          origin {
		     set plist [lindex $dtext 1]
		     label $handle position [scalepoint $plist]
		  }
	       }
	    }
	 }
	 }
      }
      default {
	 puts "Unsupported geometry block keyword $elemtype"
         set handle {}
      }
   }
   return $handle
}

#------------------------------------------------------------------------
# execute substitution of net names
#------------------------------------------------------------------------

proc subst_name {net} {

   global subst_list

   if {[llength $net] > 1} {
      if {[lindex $net 0] == "rename"} {
	 lappend subst_list [lrange $net 1 2]
	 return [lindex $net 2]		
      } else {
      	 return $net
      }
   } else {
      set pos [lsearch $subst_list "$net *"]
      if {$pos > -1} {
	 return [lindex [lindex $subst_list $pos] 1]
      } else {
 	 return $net
      }
   }
}

#------------------------------------------------------------------------
# Parse a "net" statement in the EDIF data.
#------------------------------------------------------------------------

proc parse_net {netdata} {

   set netString [lindex $netdata 0]

   if {[llength $netString] > 1} {
      if {[lindex $netString 0] == "name"} {
	 parse_geometry [lindex $netString 0] [lrange $netString 1 end]
      } elseif {[lindex $netString 0] == "rename"} {
		set elemdata [lrange $netString 1 end]
		if { [llength [lindex $elemdata 1]]>1 } {
			parse_geometry [lindex [lindex $elemdata 1] 0] [lrange [lindex $elemdata 1] 1 end]
		}
      } elseif {[lindex $netString 0] == "array"} {
	 parse_net [list [lindex $netString 1]]
      } else {
	 puts "Unknown net name $netString"
	 return
      }
   }

   foreach phrase [lrange $netdata 1 end] {
      set keyword [lindex $phrase 0]
      switch $keyword {
	 joined {
	    # ignore net information (for now)
	 }
	 figure {
	    # Figure types defined in the technology block.
	    set figureType [lindex $phrase 1]
	    set geolist [lindex $phrase 2]
	    parse_geometry [lindex $geolist 0] [lrange $geolist 1 end]
	 }
         default {puts "Unsupported net block keyword $keyword"}
      }
   }
}

#------------------------------------------------------------------------
# Parse an "instance" statement in the EDIF data.
#------------------------------------------------------------------------

proc parse_instance {instName instdata} {
   quiet_deselect
   set handle {}
   if {[lindex $instName 0] == "array"} {
   	set instName [subst_name [lindex $instName 1]]
   } else {
   	set instName [subst_name $instName]
   }
   foreach phrase $instdata {
      set keyword [lindex $phrase 0]
      switch $keyword {
	 viewRef {
	    # Next keyword should be "symbol" or "symbolr"?
	    set objtype [lindex $phrase 1]
	    set instinfo [lindex $phrase 2]
	    # 1st keyword should be "cellRef", 3rd "libraryRef".  Ignore these.
	    set cellname [lindex $instinfo 1]
	    # Create the instance
	    if {[catch {set handle [instance make $cellname "0 0"]}]} {
	       puts "ERROR:  Attempt to instance non-existant object $cellname"
	    } else {
	       puts "Created instance $instName of $cellname"
	    }
	 }
	 transform {
	    if {$handle == {}} {
	       puts "Error: Transform specified without reference instance"
	    } else {
	       foreach trans [lrange $phrase 1 end] {
	          set trk [lindex $trans 0]
	          switch $trk {
	             orientation {	
			set ogood 0
		        set oval [lindex $trans 1]

			if {[string first "MX" $oval] >= 0} {
			   element $handle flip vertical
			   set ogood 1
			} elseif {[string first "MY" $oval] >= 0} {
			   element $handle flip horizontal
			   set ogood 1
		        }
		        set rpos [string first "R" $oval]
			if {$rpos >= 0} {
			   incr rpos
		           element $handle rotate [rotinvert [string range \
					$oval $rpos end]]
			   set ogood 1
			}
		 	if {$ogood == 0} {
			   puts "Unsupported orientation $oval in transform"
		        }
	             }
	             origin {
		        set plist [lindex $trans 1]
			instance $handle center [scalepoint $plist]
		     }
		  }
	       }
	    }
	 }
	 property {
	    if {$handle != {}} {
	       push $handle
	       parse_property [lindex $phrase 1] [lrange $phrase 2 end]
	       pop
	    }
	 }

         default {puts "Unsupported instance block keyword $keyword"}
      }
   }

   # Set standard parameters idx and class from the instance name

   if {$handle != {}} {
      push $handle
      if {[catch {parameter set class [string range $instName 0 0]}]} {
         if {[catch {parameter set instName $instName}]} {
	    parameter make substring instName ?
	    parameter set instName $instName
	 }
      } else {
	 parameter set idx [string range $instName 1 end]
      }
      pop
   }
}

#------------------------------------------------------------------------
# Parse a "portImplementation" statement
#------------------------------------------------------------------------

proc parse_port {labellist portdata} {

   set portString [lindex $portdata 0]
   set plabel {}

   if {[llength $portString] > 1} {
      if {[lindex $portString 0] == "name"} {
	 set plabel [parse_geometry [lindex $portString 0] [lrange $portString 1 end]]
	 set portName [subst_name [lindex $portString 1]]
      } else {
	 puts "Unknown portImplementation pin $portString"
	 return
      }
   } else {
      set portName [subst_name $portString]
   }

   if {$labellist != {}} {
      foreach llabel $labellist {
         if {"$portName" == [label $llabel text]} {
	    set plabel $llabel
	    break
	 }
      }
   } else {
   	if {$plabel == {}} {
   	  quiet_deselect
   	  set plabel [label make normal $portName {0 0}]
	}
   }
   
   
   if {$plabel == {}} {
      puts "Can't determine port name in $portdata"
      return
   }
 
   # If we were not given a label list, then this is a pin on the schematic
   # and should be converted into a pin.  

   if {[label $plabel type] == "normal"} {
      label $plabel type local
   }
   
   foreach portProp [lrange $portdata 1 end] {
      set portkey [lindex $portProp 0]
      switch $portkey {
	 figure {
	    set figureType [lindex $portProp 1]
	    set geolist [lindex $portProp 2]
	    parse_geometry [lindex $geolist 0] [lrange $geolist 1 end]
	 }
	 connectLocation {
	    # Expect a (figure pin (dot (pt x y))))
	    # Use the point position to reposition the label.
	    set cpoint [scalepoint [lindex [lindex [lindex $portProp 1] 2] 1]]
	    label $plabel position $cpoint
	 }
	 instance {
	    parse_instance [lindex $portProp 1] [lrange $portProp 2 end]
	 }
	 default {puts "Unsupported portImplementation keyword $portkey"}
      }
   }
}

#------------------------------------------------------------------------
# Parse a "page" statement in the EDIF data.
#------------------------------------------------------------------------

proc parse_page {cellname portlist pagedata} {

   # Go to the next empty page
   set cpage 1
   page $cpage -force
   while {[object parts] != {}} {incr cpage ; page $cpage -force}
   page label $cellname

   foreach phrase $pagedata {
      set keyword [lindex $phrase 0]
      switch $keyword {
	 instance {
	    set instName [lindex $phrase 1]
	    parse_instance $instName [lrange $phrase 2 end]
	 }
	 net {
	    parse_net [lrange $phrase 1 end]
	 }
	 portImplementation {
	    parse_port {} [lrange $phrase 1 end]
	 }
         commentGraphics {
	    set comment [lindex $phrase 1]
	    set ctype [lindex $comment 0]	;# from the technology block...
	    set geolist [lindex $comment 1]
	    parse_geometry [lindex $geolist 0] [lrange $geolist 1 end]
	 }
         default {puts "Unsupported page block keyword $keyword"}
      }
   }

   if {$portlist != {}} {
      quiet_deselect
      set obbox [object bbox]
      set x [expr [lindex $obbox 0] + 128]
      set y [expr [lindex $obbox 1] - 64]
      set itext [list {Text "spice1:.subckt %n"}]
      set pstring ""
      foreach pname $portlist {
	 if {[string length $pstring] > 60} {
	    lappend itext [subst {Text "$pstring"}]
	    lappend itext {Return}
	    set pstring "+"
	    incr y -32
	 }
	 set pstring [join [list $pstring "%p$pname"]]	;# preserves whitespace
      }
      lappend itext [subst {Text "$pstring"}]
      label make info "$itext" "$x $y"
      
      incr y -32
      label make info "spice-1:.ends" "$x $y"
   }
}

#------------------------------------------------------------------------
# Parse a "contents" statement in the EDIF data.
#------------------------------------------------------------------------

proc parse_contents {celltype cellname portlist contentdata} {

   global symbolview

   set labellist {}
   foreach phrase $contentdata {
      set keyword [lindex $phrase 0]
      switch $keyword {
	 boundingBox {
	    set bbox [lindex $phrase 1]
	    set handle [parse_geometry [lindex $bbox 0] [lrange $bbox 1 end]]
	    element $handle border bbox true
	 }
	 commentGraphics {
	    set comment [lindex $phrase 1]
	    set ctype [lindex $comment 0]	;# from the technology block...
	    set geolist [lindex $comment 1]
	    set plabel [parse_geometry [lindex $geolist 0] [lrange $geolist 1 end]]
	    if {$plabel != {}} {lappend labellist $plabel}
	 }
	 figure {
	    # Figure type is defined in the technology section, and defines
	    # color, linewidth, etc.
	    set figureType [lindex $phrase 1]
	    set geolist [lindex $phrase 2]
	    parse_geometry [lindex $geolist 0] [lrange $geolist 1 end]
	 }
	 portImplementation {
	    parse_port $labellist [lrange $phrase 1 end]
	 }
	 page {
	    # Note:  Handle multiple pages here!
	    parse_page $cellname $portlist [lrange $phrase 1 end]
	 }
	 default {puts "Unsupported content/symbol block keyword $keyword"}
      }
   }

   # search for all symbol views
   # following list contains these viewnames

   if {[lsearch $symbolview $celltype] > -1} {
      # Create standard parameters
      parameter make substring class "X"
      parameter make substring idx "?"

      if {$portlist != {}} {
         quiet_deselect
         set obbox [object bbox]
         set x [expr [lindex $obbox 0] + 128]
         set y [expr [lindex $obbox 1] - 64]
         set itext [list {Text "spice:"} {Parameter class} {Parameter idx}]
         set pstring ""
         foreach pname $portlist {
	    if {[string length $pstring] > 60} {
	       lappend itext [subst {Text "$pstring"}]
	       lappend itext {Return}
	       set pstring "+"
	       incr y -32
	    }
	    set pstring [join [list $pstring "%p$pname"]]  ;# preserves whitespace
         }
         set pstring [join [list $pstring "%n"]]	   ;# preserves whitespace
         lappend itext [subst {Text "$pstring"}]

         label make info "$itext" "$x $y"
      }
   }

}

#------------------------------------------------------------------------
# Parse a "property" statement in the EDIF data.
# (To do:  generate an xcircuit parameter for the object)
#------------------------------------------------------------------------

proc parse_property {key value} {
   set keylen [llength $key]
   set property [lindex $value 0]
   set proptype [lindex $property 0]
   set pvalue [lindex $property 1]

   # Any other proptypes to handle?
   switch $proptype {
      string { set paramtype substring }
      integer { set paramtype numeric }
      boolean {
	 set paramtype numeric
	 switch $pvalue {
	    true -
	    True -
	    TRUE -
	    t -
	    T { set pvalue 1}
	    false -
	    False -
	    FALSE -
	    f -
	    F { set pvalue 0}
	 }
      }
      default { puts "Unknown property type $proptype" ; return }
   }

   # Attempt to set the parameter value.  If the parameter doesn't
   # exist, then create it.

   if [catch {parameter set $key $pvalue}] {
      parameter make $paramtype $key $pvalue
   }
}

#------------------------------------------------------------------------
# Parse an "interface" block in the EDIF data.
#------------------------------------------------------------------------

proc parse_interface {cellname ifacedata} {
   set portlist {}
   foreach phrase $ifacedata {
      set keyword [lindex $phrase 0]

      # Possible other keywords to handle:
      # port (ignoring ports for now, just dealing with portImplementation)
      # property (is a property of what??) --- should translate instNamePrefix
      #    to parameter "class".

      switch $keyword {
	 port {
	    if {[lindex [lindex $phrase 1] 0] == "array"} {
	       set zw [subst_name [lindex [lindex $phrase 1] 1]]
	    } else {
	       set zw [subst_name [lindex $phrase 1]]
	    }
	    lappend portlist $zw
	 }
	 symbol {parse_contents symbol $cellname $portlist [lrange $phrase 1 end]}
	 default {puts "Unsupported interface block keyword $keyword"}
      }
   }
   return $portlist
}

#------------------------------------------------------------------------
# Parse a "view schematic" statement in the EDIF data.
#------------------------------------------------------------------------

proc parse_schematic {cellname viewdata} {

   set portlist {}
   foreach phrase $viewdata {
      set keyword [lindex $phrase 0]
      # Possible other keywords to handle:
      # viewType, interface
      switch $keyword {
	 interface {set portlist [parse_interface $cellname [lrange $phrase 1 end]]}
	 contents {parse_contents schematic $cellname $portlist [lrange $phrase 1 end]}
	 property {parse_property [lindex $phrase 1] [lrange $phrase 2 end]}
	 default {puts "Unsupported schematic view block keyword $keyword"}
      }
   }

}

#------------------------------------------------------------------------
# Parse a "view symbol" statement in the EDIF data.
#------------------------------------------------------------------------

proc parse_symbol {libname cellname viewdata} {

   # If an object of this name exists, rename it first.
   catch {object [object handle $cellname] name "_$cellname"}
   set handle [object make $cellname $libname -force]
   if {$handle == ""} {
      puts "Error:  Couldn't create new object!"
      return
   }
   set cpage [page]
   push $handle
   symbol type fundamental
   set portlist {}

   foreach phrase $viewdata {
      set keyword [lindex $phrase 0]
      # Possible other keywords to handle:
      # viewType, interface
      switch $keyword {
	 interface {set portlist [parse_interface $cellname [lrange $phrase 1 end]]}
	 contents {parse_contents fundamental $cellname $portlist [lrange $phrase 1 end]}
	 property {parse_property [lindex $phrase 1] [lrange $phrase 2 end]}
	 default {puts "Unsupported symbol view block keyword $keyword"}
      }
   }

   pop
   if [catch {delete $handle}] {
      page $cpage
      if [catch {delete $handle}] {
         puts "Error: Element handle $handle does not exist?"
         puts "Page objects: [object parts]"
      }
   }

   # Find the instance in the library and force a recomputation of its
   # bounding box.

   library $libname goto
   foreach inst [library $libname handle] {
      if {[instance $inst object] == $cellname} {
         instance $inst bbox recompute
	 break
      }
   }
   page $cpage
}

#------------------------------------------------------------------------
# Parse a "cell" statement in the EDIF data.
#------------------------------------------------------------------------

proc parse_cell {libname cellname celldata} {

   global symbolview

   foreach phrase $celldata {
      set keyword [lindex $phrase 0]
      # Possible other keywords to handle:
      # cellType
      switch $keyword {
	 view {
	    set viewType [lindex $phrase 1]
	    if {[lsearch $symbolview $viewType] > -1} {
	    	parse_symbol $libname $cellname [lrange $phrase 2 end]
	    } else {
	       switch $viewType {
	          schematic {parse_schematic $cellname [lrange $phrase 2 end]}
	          default {puts "Unsupported view type $viewType"}
	       }
	    }
	 }
	 default {puts "Unsupported cell block keyword $keyword"}
      }
   }
}


#------------------------------------------------------------------------
# Parse a "library" statement in the EDIF data.
#------------------------------------------------------------------------

proc parse_library {libname libdata} {
   library make $libname
   foreach phrase $libdata {
      set keyword [lindex $phrase 0]
      # Possible other keywords to handle:
      # basic, edifLevel, technology
      switch $keyword {
	 cell {
	    parse_cell $libname [lindex $phrase 1] [lrange $phrase 2 end]
	 }
	 default {puts "Unsupported library block keyword $keyword"}
      }
   }

   # Regenerate the library
   library $libname compose
}

#------------------------------------------------------------------------
# Remove C-style comments
#------------------------------------------------------------------------

proc remove_comments {text} {
   regsub -all {[/][*].*?[*][/]} $text "" text
   return $text 
}

#------------------------------------------------------------------------
# Main file reader routine.  Read the file into a single string, and
# replace parentheses so we turn the LISP phrasing into a nested TCL
# list.
#------------------------------------------------------------------------

proc read_edif {filename} {

   if [catch {open $filename r} fileIn] {
      puts stderr "Cannot find file $filename"
      return;
   }

   # Remove some tag callbacks that cause problems. . .

   set paramtag [tag parameter]
   tag parameter {}

   # Convert the file into a nested list by converting () to {}.

   set everything [remove_comments [read $fileIn]]
   set masterlist [lindex [string map {( \{ ) \} \n " "} $everything] 0]
   unset everything
   close $fileIn

   # Now parse the file. . .

   set cpage [page]
   config suspend true

   foreach phrase $masterlist {
      set keyword [lindex $phrase 0]
      # Possible other keywords to handle:
      # edif, edifVersion, edifLevel, keywordMap, status
      switch $keyword {
	 library {parse_library [lindex $phrase 1] [lrange $phrase 2 end]}
	 default {puts "Unsupported primary keyword $keyword"}
      }
   }

   tag parameter $paramtag

   # Return to (and redraw) original page
   page $cpage
   zoom view
   config suspend false

}

#------------------------------------------------------------------------
# Procedure that creates the dialog to find an EDIF file to parse and
# calls procedure read_edif.
#---------------------------------------------------------------------------

proc xcircuit::promptreadedif {} {
  global XCOps
  .filelist.bbar.okay configure -command \
        {read_edif [.filelist.textent.txt get] ; \
        wm withdraw .filelist}
  .filelist.listwin.win configure -data "edf edif"
  .filelist.textent.title.field configure -text "Select EDIF 2.0.0 file to parse:"
  .filelist.textent.txt delete 0 end
  xcircuit::popupfilelist
  xcircuit::removelists .filelist
}

