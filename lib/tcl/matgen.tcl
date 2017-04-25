#----------------------------------------------------------------------
# matgen.tcl v0.7 --- An xcircuit Tcl extension for importing matlab
#                     postscript files
# Copyright (c) 2008  Wim Vereecken 
#                     Wim.Vereecken@gmail.com
#                     Wim.Vereecken@esat.kuleuven.be				
#									
# This program is free software; you can redistribute it and/or modify 
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or	
# (at your option) any later version.					
#									
# This program is distributed in the hope that it will be useful,	
# but WITHOUT ANY WARRANTY; without even the implied warranty of	
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#----------------------------------------------------------------------

# "lset" forward compatibility routine from wiki.tcl.tk---"lset" is
# used in these routines, and was new with Tcl version 8.4.

if {[package vcompare [package provide Tcl] 8.4] < 0} {
   proc tcl::K {a b} {return $a}
   proc lset {listName index val} {
      upvar $listName list
      set list [lreplace [tcl::K $list [set list {}]] $index $index $val]
   }
}

#----------------------------------------------------------------------

proc xcircuit::promptimportmatlab {} {
   .filelist.bbar.okay configure -command \
      {matgen [.filelist.textent.txt get]; wm withdraw .filelist}
   .filelist.listwin.win configure -data "ps"
   .filelist.textent.title.field configure -text "Select file to import:"
   .filelist.textent.txt delete 0 end
   xcircuit::popupfilelist
}

#----------------------------------------------------------------------

proc matgen {argv} {
global XCOps XCWinOps
# PART 1: Command line parser_______________________________
# load-axes property is set
set axes 1
# set default scale factor
set scalefactor 0.2
# set default line width
set linewidth 1.00
# set default line width
set bold 0
# set default file name
set filenames {}
# set default text size
set matlab_textsize 100
# parse all command line args
puts "Checking options..."
foreach arg $argv {
	# if -noaxes key is found, disable axes
	if [string compare $arg "-noaxes"]==0 then {
		set axes 0
		puts "  *** Axes are disabled"
	}
	if [regexp {\-scale=(\d+\.*\d*)} $arg matchresult sub1] then {
		set scalefactor [expr $sub1*0.20]
		puts "  *** Scalefactor set to $scalefactor"
	}
	if [regexp {\-lw=(\d+\.*\d*)} $arg matchresult sub1] then {
		set linewidth $sub1
		puts "  *** Linewidth set to $linewidth"
	}
	if [regexp {\-bold} $arg matchresult] then {
		set bold 1
		puts "  *** Bold fonts are enabled"
	}
	if [regexp {(.*\.e?ps)} $arg matchresult sub1] then {
		lappend filenames $sub1
		set file [lindex $filenames [expr [llength $filenames]-1]]
	}
}

if {[llength $argv] < 1} then {
	puts "  *** no arguments found...\n"
	puts "matgen.tcl 0.7"
	puts "Written by Wim Vereecken\n"
	puts "Usage:"
	puts "   matgen \"\[options\] file.eps \[file2.eps\] ...\"\n"
	puts "Options:"
	puts "   -noaxes: strip axes data"
	puts "   -scale=x: rescale by factor x"
	puts "   -lw=x: scale linewidth by factor x"
	puts "   -bold: use bold text labels\n"
	puts "Example:"
	puts "   matgen \"-scale=0.7 -lw=3 file.eps\"\n"
	return 0
}

# PART 2: Parse and process the source matlab eps file_______
# The extension of the source files that will be examined
puts "Checking for valid matlab postscript (eps) files..."
## Search in the current directory for files with an .eps
## extension. If the file is recognized as a matlab .eps
## file, add the source file to @foundsources.

set validfiles {};
foreach file $filenames {
	if [regexp {e?ps} $file matchresult] then {
		# try to open $file
		if [catch {open $file r} result] {
			puts "Warning: $result"
			return -1
		} else {
			set fp $result
			set data [read $fp]
			close $fp
		}
		# split data in seperate lines
		set data [split $data "\n"]
		# read every line and search for "Creator: MATLAB"
	        foreach line $data {
			# check if the source is a matlab eps
			if [regexp {Creator: MATLAB} $line matchresult] then {
				lappend validfiles $file
				puts "  *** $file is a valid Matlab plot";
				# break searching here
				break 
		  	}
	    	}
	}
}

# exit with message when no source files are found 
if [llength $validfiles]<1 {
	puts "  *** No valid matlab .eps files found. Bye!";
	return -1
}

# PART 3____________________________________________________
foreach file $validfiles {
	puts "Processing $file...";
	# open each matlab eps file
	if [catch {open $file r} result] {
		puts "Warning: $result"
		return -1
	} else {
		set fp $result
		set data [read $fp]
		close $fp
	}
	# the stack emulator
	set source_stack ""
	# the xcircuit output vector
	set xcircuit_output ""
	# the xcircuit lib definitions
	set xcircuit_libdefs ""
	# the color definitions library
	set colordefs ""

	# split data in seperate lines
	set data [split $data "\n"]
	set object_list {}
	foreach line $data {
		append source_stack "\x0a$line"
		# Skip first part of matlab $source_stack if noaxes is set
		if [regexp {\/c8} $line matchresult] then {
			if {$axes == 0} then {
				# clear output
				set xcircuit_output ""
			} else {
				# select all items in page, make object, and deselect
				if {[llength $object_list] > 0} then {
					select $object_list
					set axes_object [object make "axes"]
					deselect $axes_object
				}
			}
		}
		# Extract polylines from the matlab $source_stack
		if [regexp {(\d+)\s*MP\sstroke} $line matchresult sub1] {
			# extract the number of vertices of the polyline
			set num_vertices $sub1
			# the number of arguments to read is thus 2*num_vertices
			set num_args [expr $num_vertices*2]
			# extract the vertices list of the polyline
			set re "\\x0a+\\s*((?:\-?\\d+\\x20*\\x0a*)+?)\\s+$num_vertices\\s+MP\\sstroke"
			if [regexp $re $source_stack matchresult sub1] then {
				# replace 1+ spaces by the ":" separator
				regsub -all {\s+} $sub1 ":" sub1
				# split $sub1 based on the ":" separator
				set polyline_vertices [split $sub1 ":"]
				# the polyline_vertices list contains all vertices
				# --> {<undefined data> x1 y1 x2 y2 ... xn yn}
			} else {
				puts "Error in polyline regular expression. Exiting..."
				return -1
			}
			# convert the arguments to the xcircuit coordinate format.
			# The matlab polyline coordinates are relative to each other,
			# starting for the last value of the postscript stack.
			# XCircuit needs absolute coordinates for the polyline vertices.
			# Some recalculation is thus needed to obtain absolute coordinates.
			for {set i [expr $num_args-4]} {$i>=0} {incr i -2} {
				# calculate x-coordinates
				set prev_point_x [lindex $polyline_vertices $i]
				set current_point_x [lindex $polyline_vertices [expr $i+2]]
				lset polyline_vertices $i [expr $prev_point_x+$current_point_x]
				# calculate y-coordinates
				set prev_point_y [lindex $polyline_vertices [expr $i+1]]
				set current_point_y [lindex $polyline_vertices [expr $i+3]]
				lset polyline_vertices [expr $i+1] [expr $prev_point_y+$current_point_y]
			}
			# Rescaling feature. Should be re-implemented in a decent way
			for {set i 0} {$i<$num_args} {incr i 1} {
				lset polyline_vertices $i [expr int([lindex $polyline_vertices $i]*$scalefactor)]
			}
			# Flip the y-axis coordinates. XCircuit's coordinate system seems to use
			# the left hand rule.
			for {set i 1} {$i<$num_args} {incr i 2} {
				lset polyline_vertices $i [expr [lindex $polyline_vertices $i]*-1]
			}
			# create the tcl command and draw the polygon
			set prev_point_x 0
			set prev_point_y 0
			set num_vertices 0
			set tcl_command ""
			eval "fill 0"
			for {set i 0} {$i<$num_args} {incr i 2} {
				# the number of vertices xcircuit can handle is limited to 250
				if {$num_vertices == 200} then {
					eval "lappend object_list \[polygon make 200 $tcl_command\]"
					set prev_point_x 0
					set prev_point_y 0
					set num_vertices 0
					set tcl_command ""
					# resume last point in next polyline
					incr i -2
				}
				# extract the current x- and y-coordinates
				set current_point_x [lindex $polyline_vertices $i]
				set current_point_y [lindex $polyline_vertices [expr $i+1]]
				# drop the current vertex if distance from previous point is too small
				if {   abs([expr $current_point_x - $prev_point_x]) < 1
				    && abs([expr $current_point_y - $prev_point_y]) < 1} then {
					# drop the current vertex
					continue
				# add the current vertex to the tcl_command string
				} else {
					# increase the number of vertices in the tcl_command
					incr num_vertices 1
					# append the current vertex to the tcl_command string
					# format: "polygon make N {x1 y1} {x2 y2} ... {xn yn}"
					append tcl_command "\{$current_point_x $current_point_y\} "
					# save the current vertex for future reference
					set prev_point_x $current_point_x
					set prev_point_y $current_point_y
				}
			}
			# Evaluate the tcl_command
			eval "lappend object_list \[polygon make $num_vertices $tcl_command\]"
			# flush stack
			set source_stack ""
			set line ""
		}
		# Extract filled polylines from the matlab $source_stack
		if [regexp {(\d+)\s+MP} $line matchresult sub1] {
			set num_vertices $sub1
			set num_args [expr $num_vertices*2]
			# extract the vertices list of the polyline
			set re "\\x0a+\\s*((?:\-?\\d+\\x20*\\x0a*)+)\\s+$num_vertices\\s+MP"
			if [regexp $re $source_stack matchresult sub1] then {
				# replace 1+ spaces by the ":" separator
				regsub -all {\s+} $sub1 ":" sub1
				# split $sub1 based on the ":" separator
				set polyline_vertices [split $sub1 ":"]
				# the polyline_vertices list contains all vertices
				# --> {<undefined data> x1 y1 x2 y2 ... xn yn}
			} else {
				puts "Error in filled polyline regular expression. Exiting..."
				return -1
			}
			# convert the arguments to the xcircuit coordinate format.
			# The matlab polyline coordinates are relative to each other,
			# starting for the last value of the postscript stack.
			# XCircuit needs absolute coordinates for the polyline vertices.
			# Some recalculation is thus needed to obtain absolute coordinates.
			for {set i [expr $num_args-4]} {$i>=0} {incr i -2} {
				# calculate x-coordinates
				set prev_point_x [lindex $polyline_vertices $i]
				set current_point_x [lindex $polyline_vertices [expr $i+2]]
				lset polyline_vertices $i [expr $prev_point_x+$current_point_x]
				# calculate y-coordinates
				set prev_point_y [lindex $polyline_vertices [expr $i+1]]
				set current_point_y [lindex $polyline_vertices [expr $i+3]]
				lset polyline_vertices [expr $i+1] [expr $prev_point_y+$current_point_y]
			}
			# Rescaling feature. Should be re-implemented in a decent way
			for {set i 0} {$i<$num_args} {incr i 1} {
				lset polyline_vertices $i [expr int([lindex $polyline_vertices $i]*$scalefactor)]
			}
			# Flip the y-axis coordinates. XCircuit's coordinate system seems to use
			# the left hand rule.
			for {set i 1} {$i<$num_args} {incr i 2} {
				lset polyline_vertices $i [expr [lindex $polyline_vertices $i]*-1]
			}
			# create the tcl command and draw the polygon
			set prev_point_x 0
			set prev_point_y 0
			set num_vertices 0
			set tcl_command ""
			for {set i 0} {$i<$num_args} {incr i 2} {
				# the number of vertices xcircuit can handle is limited to 250
				if {$num_vertices > 200} then break
				# extract the current x- and y-coordinates
				set current_point_x [lindex $polyline_vertices $i]
				set current_point_y [lindex $polyline_vertices [expr $i+1]]
				# drop the current vertex if distance from previous point is too small
				if {   abs([expr $current_point_x - $prev_point_x]) < 2
				    && abs([expr $current_point_y - $prev_point_y]) < 2} then {
					# drop the current vertex
					continue
				# add the current vertex to the tcl_command string
				} else {
					# increase the number of vertices in the tcl_command
					incr num_vertices 1
					# append the current vertex to the tcl_command string
					# format: "polygon make N {x1 y1} {x2 y2} ... {xn yn}"
					append tcl_command "\{$current_point_x $current_point_y\} "
					# save the current vertex for future reference
					set prev_point_x $current_point_x
					set prev_point_y $current_point_y
				}
			}
			# Evaluate the tcl_command. Only write if we have a valid color,
			# different from white (white color index is 1 in xcircuit)
			if {[color get] != 1} then {
				eval "fill 100"
				eval "lappend object_list \[polygon make $num_vertices $tcl_command\]"
				eval "fill 0"
			}
			# flush stack
			set source_stack ""
			set line ""
		}
		# Extract lines from the matlab $source_stack
		set re "(\\d+)\\s+(\\d+)\\s+mt\\s+(\\d+)\\s+(\\d+)\\sL"
		if [regexp $re $line matchresult sub1 sub2 sub3 sub4] then { 
			set x1 [expr int($sub1*$scalefactor)]
			set y1 [expr int(-$sub2*$scalefactor)]
			set x2 [expr int($sub3*$scalefactor)]
			set y2 [expr int(-$sub4*$scalefactor)]
			# Create the tcl command and draw the line
			eval "lappend object_list \[polygon make 2 {$x1 $y1} {$x2 $y2}\]"
			# clear the source stack
			set source_stack ""
			set line ""
		}
		# Extract points from the matlab $source_stack
		set re "(\\d+)\\s+(\\d+)\\s+PD"
		if [regexp $re $line matchresult sub1 sub2] then {
			set x1 [expr int($sub1*$scalefactor)]
			set y1 [expr -int($sub2*$scalefactor)]
			set dotsize [expr int(15*$scalefactor)]
			# concatenate all lines in one vector
			eval "fill 100"
			eval "lappend object list \[arc make \{$x1 $y1\} $dotsize\]"
			eval "fill 0"
			# clear the source stack
			set source_stack ""
			set line ""
		}
		# Extract axes labels from the matlab $source_stack
		set re "(\\d+)\\s+(\\d+)\\s+mt\\s*\\n\\((.*)\\)"
		if [regexp $re $source_stack matchresult sub1 sub2 sub3] then {
			set x1 [expr int($sub1*$scalefactor)]
			set y1 [expr -int($sub2*$scalefactor)]
			set labeltext $sub3
			# some experimental rescaling. Should be reimplemented in the future
			set textsize [expr $matlab_textsize*4*$scalefactor/168]
			# concatenate all text in one vector
			# also remove empty labels (causes segfault in xcircuit-3.4.26)
			if {$bold == 1 && [regexp {[^\s]+} $labeltext]} then {
				eval "label style bold"
				eval "label scale $textsize"
				eval "lappend object_lsit \[label make normal \"$labeltext\" \{$x1 $y1\}\]"
			} elseif [regexp {[^\s]+} $labeltext] {
				eval "label scale $textsize"
				eval "lappend object_list \[label make normal \"$labeltext\" \{$x1 $y1\}\]"
			}
			# clear the source stack
			set source_stack ""
			set line ""
		}
		# Extract color definitions from the matlab $source_stack
		set re "(\\/c\\d+\\s+\\{\\s\\d+\\.\\d+\\s+\\d+\\.\\d+\\s+\\d+\\.\\d+\\s+sr\\}\\sbdef)"
		if [regexp $re $source_stack matchresult sub1] then {
			# add color definition to library
			append colordefs $sub1
			# clear the source stack
			set source_stack ""
			set line ""
		}
		# Extract the text size from the matlab $source_stack
		set re "(\\d+) FMSR"
		if [regexp $re $line matchresult sub1] then {
			set matlab_textsize $sub1
			append colordefs $sub1
			# clear the source stack
			set source_stack ""
			set line ""
		}
		# Retrieve the color specified on the matlab $source_stack
		set re "(c\\d+)"
		if [regexp $re $line matchresult sub1] then {
			# extract color from the library
			set re "\\/$sub1\\s+\\{\\s(\\d+\\.\\d+)\\s+(\\d+\\.\\d+)\\s+(\\d+\\.\\d+)\\s+sr\\}\\sbdef"
			if [regexp $re $colordefs matchresult sub1 sub2 sub3] then {
				set red [expr int($sub1*255)]
				set green [expr int($sub2*255)]
				set blue [expr int($sub3*255)]
				set rgb [dec2rgb $red $green $blue] 
				# add color to xcircuit output
				set color_index [eval "color add $rgb"]
				eval "color set $color_index"
				# clear the source stack
				set source_stack ""
				set line ""
			}
		}			
		# Extract grayscale colors from the matlab $source_stack
		set re "(\[0-9\]*\\.\[0-9\]+|\[0-9\]+)\\ssg"
		if [regexp $re $line matchresult sub1] then {
			set greylevel [expr int($sub1*255)]
			set rgb [dec2rgb $greylevel $greylevel $greylevel] 
			# add color to xcircuit output
			set color_index [eval "color add $rgb"]
			eval "color set $color_index"
			# clear the source stack
			set source_stack ""
			set line ""
		}	
		# Extract solid/dotted linestyle ftom the matlab $source_stack
		set re "((?:SO)|(?:DO)|(?:DA)|(?:DD))"
		if [regexp $re $line matchresult sub1] then {
			if {$sub1 == "DO"} then {
				eval "border dotted"
			} elseif {$sub1 == "SO"} then {
				eval "border solid"
			} elseif {$sub1 == "DA"} then {
				eval "border dashed"
			} elseif {$sub1 == "DD"} then {
				eval "border solid"
			}
			# clear the source stack
			set source_stack ""
			set line ""
		}
	}
	puts "  *** $file import OK";
# >> end of foreach file
}
# >> end of proc matgen 
}

# PART 4____________________________________________________
# Extract from util-color.tcl
# Copyright (c) 1998 Jeffrey Hobbs
#
# dec2rgb --
#
# See the file "license.terms" for information on usage and
# redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
#
#   Takes a color name or dec triplet and returns a #RRGGBB color.
#   If any of the incoming values are greater than 255,
#   then 16 bit value are assumed, and #RRRRGGGGBBBB is
#   returned, unless $clip is set.
#
# Arguments:
#   r		red dec value, or list of {r g b} dec value or color name
#   g		green dec value, or the clip value, if $r is a list
#   b		blue dec value
#   clip	Whether to force clipping to 2 char hex
# Results:
#   Returns a #RRGGBB or #RRRRGGGGBBBB color
#
proc dec2rgb {r {g 0} {b UNSET} {clip 0}} {
    if {![string compare $b "UNSET"]} {
	set clip $g
	if {[regexp {^-?(0-9)+$} $r]} {
	    foreach {r g b} $r {break}
	} else {
	    foreach {r g b} [winfo rgb . $r] {break}
	}
    } 
    set max 255
    set len 2
    if {($r > 255) || ($g > 255) || ($b > 255)} {
	if {$clip} {
	    set r [expr {$r>>8}]; set g [expr {$g>>8}]; set b [expr {$b>>8}]
	} else {
	    set max 65535
	    set len 4
	}
    }
    return [format "#%.${len}x%.${len}x%.${len}x" \
	    [expr {($r>$max)?$max:(($r<0)?0:$r)}] \
	    [expr {($g>$max)?$max:(($g<0)?0:$g)}] \
	    [expr {($b>$max)?$max:(($b<0)?0:$b)}]]
}

