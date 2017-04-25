#---------------------------------------------------------------------------
# symbol.tcl ---
#
#	xcircuit Tcl script for generating
#	a canonical circuit symbol
#
#	Tim Edwards 12/1/05 for MultiGiG
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
# Get the info label line declaring the text subcircuit, if one exists.
#---------------------------------------------------------------------------

proc xcircuit::getsubckttext {name} {
   global XCOps

   set curhandle [object handle]
   set handle [page handle $name]
   if {$curhandle != $handle} {schematic goto}

   set pageparts [object $handle parts]

   set itext {}
   foreach j $pageparts {
      set etype [element $j type]
      if {$etype == "Label"} {
	 set ltype [label $j type]
	 if {$ltype == "info"} {
	    set itext [label $j text]
	    if {[string first ".subckt" $itext] >= 0} {
   	       if {$curhandle != $handle} {symbol goto}
	       return $itext
	    }
	 }
      }
   }
   deselect selected
   return ""
}

#---------------------------------------------------------------------------
# Create a matching symbol for a subcircuit page or SPICE file.
#
# This is a replacement for the procedure makesymbol in wrapper.tcl.
# We make a sub-widget to list, change, and reorder the symbol pins,
# similar to the "addliblist" procedure in wrapper.tcl
#
# Modified 1/5/06:  If "filename" is non-null, then it points to a SPICE
# netlist containing a subcircuit.  A symbol is made to correspond to the
# subcircuit definition, with a link to include the file.
#
# Modified 1/7/06:  If "orderedpins" is non-null, then it contains the
# list of pins in proper order.
#---------------------------------------------------------------------------

proc xcircuit::makesymbol {{filename ""} {orderedpins ""}} {
   global XCOps

   config suspend true	;# suspend graphics and change count

   set techname [.makesymbol.techself.techselect cget -text]
   set symbolname [.makesymbol.textent.txt get]
   if {[string length $symbolname] == 0} {
      set symbolname [page label]
      if {[string length $symbolname] == 0 || [string first "Page " $symbolname] >= 0} {
         puts stderr "Symbol/Schematic has no name!"
         consoleup
         return
      }
   }

   if {[string first :: $symbolname] >= 0} {	;# symbolname has tech name embedded
      set techname ""
   } elseif {[string first ( $techname] >= 0} {	;# "(user)" specified
      set techname "::"
   } elseif {[string first :: $techname] < 0} {
      set techname "${techname}::"
   }

   # If "filename" is specified then we have a netlist, not a schematic.
   # Therefore, create an object but don't use xcircuit::symbol

   if {$filename == ""} {
      deselect selected
      xcircuit::symbol make ${techname}${symbolname} $XCOps(library);
      set schematicname [schematic get]
      set noschem 0
   } else {
      set schematicname $symbolname
      set noschem 1
   }

   set pinspace 64
   set halfspace [expr $pinspace / 2]
   set qtrspace [expr $pinspace / 4]

   # remove the old pin labels

   set oldpinlabels [object parts]
   foreach j $oldpinlabels {
      delete $j
   }
   set leftpins [.makesymbol.pinself.left.list index end]
   set toppins [.makesymbol.pinself.top.list index end]
   set rightpins [.makesymbol.pinself.right.list index end]
   set botpins [.makesymbol.pinself.bottom.list index end]

   set hpins $leftpins
   if {$rightpins > $leftpins} {set hpins $rightpins}
   set vpins $toppins
   if {$botpins > $toppins} {set vpins $botpins}

   set boxwidth [expr ($vpins + 1) * $pinspace]
   set boxheight [expr ($hpins + 1) * $pinspace]

   set hwidth [expr $boxwidth / 2]
   if {$hwidth < 256} {set hwidth 256}
   set hheight [expr $boxheight / 2]
   if {$hheight < 256} {set hheight 256}

   set sbox [polygon make box "-$hwidth -$hheight" "$hwidth $hheight"]
   set pinlabels {}

   # If we didn't make a symbol using xcircuit::symbol, now is the time
   # to generate the object.

   if {$noschem == 1} {
      select $sbox
      set handle [object make ${techname}${symbolname} $XCOps(library)]
      push $handle
   }

   # Ordered right->left->bottom->top, on logical grounds.

   set x [expr $hwidth + $qtrspace]
   set y [expr -($rightpins - 1) * $halfspace]
   for {set j 0} {$j < $rightpins} {incr j} {
      set tabx [expr $x - $qtrspace]
      polygon make 2 "$x $y" "$tabx $y"
      set pintext [.makesymbol.pinself.right.list get $j]
      lappend pinlabels $pintext
      set tlab [label make pin "$pintext" "$x $y"]
      label $tlab anchor left
      label $tlab anchor middle
      set nlab [element $tlab copy relative "-$halfspace 0"]
      label $nlab type normal
      label $nlab anchor right
      incr y $pinspace
      deselect selected
   }
   set x [expr -$hwidth - $qtrspace]
   set y [expr -($leftpins - 1) * $halfspace]
   for {set j 0} {$j < $leftpins} {incr j} {
      set tabx [expr $x + $qtrspace]
      polygon make 2 "$x $y" "$tabx $y"
      set pintext [.makesymbol.pinself.left.list get $j]
      lappend pinlabels $pintext
      set tlab [label make pin "$pintext" "$x $y"]
      label $tlab anchor right
      label $tlab anchor middle
      set nlab [element $tlab copy relative "$halfspace 0"]
      label $nlab type normal
      label $nlab anchor left
      incr y $pinspace
      deselect selected
   }
   set y [expr -$hheight -$qtrspace]
   set x [expr -($botpins - 1) * $halfspace]
   for {set j 0} {$j < $botpins} {incr j} {
      set taby [expr $y + $qtrspace]
      polygon make 2 "$x $y" "$x $taby"
      set pintext [.makesymbol.pinself.bottom.list get $j]
      lappend pinlabels $pintext
      set tlab [label make pin "$pintext" "$x $y"]
      rotate $tlab 270
      label $tlab anchor right
      label $tlab anchor middle
      set nlab [element $tlab copy relative "0 $halfspace"]
      label $nlab type normal
      label $nlab anchor left
      incr x $pinspace
      deselect selected
   }
   set y [expr $hheight + $qtrspace]
   set x [expr -($toppins - 1) * $halfspace]
   for {set j 0} {$j < $toppins} {incr j} {
      set taby [expr $y - $qtrspace]
      polygon make 2 "$x $y" "$x $taby"
      set pintext [.makesymbol.pinself.top.list get $j]
      lappend pinlabels $pintext
      set tlab [label make pin "$pintext" "$x $y"]
      rotate $tlab 90
      label $tlab anchor right
      label $tlab anchor middle
      set nlab [element $tlab copy relative "0 -$halfspace"]
      label $nlab type normal
      label $nlab anchor left
      incr x $pinspace
      deselect selected
   }

   deselect selected
   set nlab [label make "$symbolname" {0 0}]
   label $nlab anchor middle
   label $nlab anchor center
   element $nlab color set blue

   deselect selected
   parameter make substring index "?"
   parameter make substring class "X"
   if {$schematicname == $symbolname} {
      parameter make substring link "%n"
   } else {
      parameter make substring link "$schematicname"
   }

   set nlab [label make "{Parameter class} {Parameter index}" "0 -$pinspace"]
   label $nlab anchor center
   element $nlab color set blue
   deselect selected

   # Determine if the schematic already has a "subckt" line.  If so,
   # attempt to arrange the pin ordering from it.  If not, create one.
   if {$noschem == 1} {
      set subckttext ""
      set pinlabels $orderedpins
   } else {
      set subckttext [xcircuit::getsubckttext $schematicname]
   }

   if {$subckttext == ""} {
      set subckttext [list {Text "spice1:.subckt %n"}]
      set pstring ""
      foreach j $pinlabels {
	 if {[string length $pstring] > 60} {
            lappend subckttext [subst {Text "$pstring"}]
	    lappend subckttext {Return}
	    set pstring "+"
	 }
	 set pstring [join [list $pstring "$j"]]   ;# preserves whitespace
      }
      lappend subckttext [subst {Text "$pstring"}]

      if {$noschem == 0} {
         schematic goto
         set bbox [join [page bbox all]] 
         set x [expr ([lindex $bbox 2] + [lindex $bbox 0]) / 2]
         set y [expr [lindex $bbox 1] - $pinspace]
         set nlab [label make info "$subckttext" "$x $y"] 
         label $nlab anchor center
         deselect selected
         set y [expr $y - $pinspace]
         set nlab [label make info "spice-1:.ends" "$x $y"] 
         symbol goto
      }

      set itext [list {Text "spice:"} {Parameter class} {Parameter index}]
      set pstring ""
      foreach j $pinlabels {
	 if {[string length $pstring] > 60} {
            lappend itext [subst {Text "$pstring"}]
            lappend itext {Return}
	    set pstring "+"
	 }
	 set pstring [join [list $pstring "%p$j"]]   ;# preserves whitespace
      }
      set pstring [join [list $pstring "%n"]]   ;# preserves whitespace
      lappend itext [subst {Text "$pstring"}]
   } else {
      set itext [list {Text "spice:"} {Parameter class} {Parameter index}]
      set pstring ""
      foreach j [lrange [lindex $subckttext 0] 2 end] {
	 if {[string length $pstring] > 60} {
            lappend itext [subst {Text "$pstring"}]
            lappend itext {Return}
	    set pstring "+"
	 }
	 set pstring [join [list $pstring "%p$j"]]   ;# preserves whitespace
      }
      set pstring [join [list $pstring "%n"]]   ;# preserves whitespace
      lappend itext [subst {Text "$pstring"}]
   }

   set y [expr -$hheight - 3 * $pinspace]

   deselect selected
   set nlab [label make info "$itext" "0 $y"] 
   label $nlab anchor center

   if {$noschem == 1} {
      deselect selected
      if {[string index $filename 0] != "/"} {
	 set filename [join [concat [pwd] $filename] "/"]
      }
      set y [expr -$hheight - 4 * $pinspace]
      set itext [list {Text "spice@1:%F"}]
      lappend itext [subst {Text "$filename"}]
      set nlab [label make info "$itext" "0 $y"]
      label $nlab anchor center
      deselect selected
      pop
   } else {
      library $XCOps(library) compose
      deselect selected
      symbol goto
      zoom view
   }

   config suspend false 	;# unlocked state
}

#---------------------------------------------------------------------------
# Get the list of pins for the object.  If the schematic has a "subckt"
# line, then we use the pin names from it, in order.  If not, then we
# compile a list of all unique pin labels and arrange them in dictionary
# alphabetical order.
#---------------------------------------------------------------------------

proc xcircuit::getpinlist {schematicname} {
   global XCOps

   set subckttext [xcircuit::getsubckttext $schematicname]

   if {$subckttext == {}} {
      set pinlist {}
      deselect selected
      set objlist [object parts]
      
      foreach j $objlist {
         set etype [element $j type]
         if {$etype == "Label"} {
	    set ltype [label $j type]
	    if {$ltype == "local" || $ltype == "global"} {

	       # Avoid netlist-generated pins by rejecting labels that
	       # don't start with a font specifier.

	       set subtype [lindex [lindex [lindex [label $j list] 0] 0] 0]
	       if {$subtype == "Font"} {
	          lappend pinlist [label $j text]
	       }
	    }
	 }
      }
      set pinlist [lsort -unique -dictionary $pinlist]
   } else {
      set pinlist [lrange [lindex $subckttext 0] 2 end]
   }
   deselect selected
   return $pinlist
}

#---------------------------------------------------------------------------
# Figure out which list has the selection
#---------------------------------------------------------------------------

proc xcircuit::getselectedpinwidget {} {
   set w .makesymbol.pinself.left.list
   set result [$w curselection]
   if {$result != {}} {return $w}

   set w .makesymbol.pinself.top.list
   set result [$w curselection]
   if {$result != {}} {return $w}

   set w .makesymbol.pinself.right.list
   set result [$w curselection]
   if {$result != {}} {return $w}

   set w .makesymbol.pinself.bottom.list
   set result [$w curselection]
   if {$result != {}} {return $w}
}

#---------------------------------------------------------------------------
# Remove a pin from the pin list
#---------------------------------------------------------------------------

proc xcircuit::removeselectedpin {} {
   set w [xcircuit::getselectedpinwidget]
   if {$w != {}} {
      set idx [$w curselection]
      $w delete $idx
   }
}

#---------------------------------------------------------------------------
# Move a pin to the left side of the symbol
#---------------------------------------------------------------------------

proc xcircuit::movepinleft {} {
   set w [xcircuit::getselectedpinwidget]
   if {$w != {}} {
      set idx [$w curselection]
      set pinname [$w get $idx]
      $w delete $idx
      .makesymbol.pinself.left.list insert end $pinname
      $w selection set $idx
   }
}

#---------------------------------------------------------------------------
# Move a pin to the top side of the symbol
#---------------------------------------------------------------------------

proc xcircuit::movepintop {} {
   set w [xcircuit::getselectedpinwidget]
   if {$w != {}} {
      set idx [$w curselection]
      set pinname [$w get $idx]
      $w delete $idx
      .makesymbol.pinself.top.list insert end $pinname
      $w selection set $idx

   }
}

#---------------------------------------------------------------------------
# Move a pin to the right side of the symbol
#---------------------------------------------------------------------------

proc xcircuit::movepinright {} {
   set w [xcircuit::getselectedpinwidget]
   if {$w != {}} {
      set idx [$w curselection]
      set pinname [$w get $idx]
      $w delete $idx
      .makesymbol.pinself.right.list insert end $pinname
      $w selection set $idx
   }
}

#---------------------------------------------------------------------------
# Move a pin to the bottom side of the symbol
#---------------------------------------------------------------------------

proc xcircuit::movepinbottom {} {
   set w [xcircuit::getselectedpinwidget]
   if {$w != {}} {
      set idx [$w curselection]
      set pinname [$w get $idx]
      $w delete $idx
      .makesymbol.pinself.bottom.list insert end $pinname
      $w selection set $idx
   }
}

#---------------------------------------------------------------------------
# Create the pin arranger widget and add it to the dialog box.
#---------------------------------------------------------------------------

proc xcircuit::addpinarranger {w {pinlist {}}} {

   frame ${w}.pinself
   frame ${w}.pinself.left
   frame ${w}.pinself.top
   frame ${w}.pinself.right
   frame ${w}.pinself.bottom

   label ${w}.pinself.left.title -text "Left Pins"
   label ${w}.pinself.top.title -text "Top Pins"
   label ${w}.pinself.right.title -text "Right Pins"
   label ${w}.pinself.bottom.title -text "Bottom Pins"

   listbox ${w}.pinself.left.list
   listbox ${w}.pinself.top.list
   listbox ${w}.pinself.right.list
   listbox ${w}.pinself.bottom.list

   pack ${w}.pinself.left.title -side top
   pack ${w}.pinself.left.list -side top -fill y -expand true
   pack ${w}.pinself.top.title -side top
   pack ${w}.pinself.top.list -side top -fill y -expand true
   pack ${w}.pinself.right.title -side top
   pack ${w}.pinself.right.list -side top -fill y -expand true
   pack ${w}.pinself.bottom.title -side top
   pack ${w}.pinself.bottom.list -side top -fill y -expand true

   grid ${w}.pinself.left -row 0 -column 0 -sticky news -padx 1 -pady 1
   grid ${w}.pinself.top -row 0 -column 1 -sticky news -padx 1 -pady 1
   grid ${w}.pinself.right -row 0 -column 2 -sticky news -padx 1 -pady 1
   grid ${w}.pinself.bottom -row 0 -column 3 -sticky news -padx 1 -pady 1

   grid columnconfigure ${w}.pinself 0 -weight 1 -minsize 50
   grid columnconfigure ${w}.pinself 1 -weight 1 -minsize 50
   grid columnconfigure ${w}.pinself 2 -weight 1 -minsize 50
   grid columnconfigure ${w}.pinself 3 -weight 1 -minsize 50

   grid rowconfigure ${w}.pinself 0 -weight 1 -minsize 50

   # Determine if the pinlist is fixed by either being taken from a "subckt"
   # line in a schematic, or being taken from a "subckt" line in a SPICE deck.
   # If so, we pass the ordered list to the symbol construction routine, and
   # we also prevent symbol pins from being deleted.  

   config suspend true	;# suspend graphics and change count

   if {$pinlist == {}} {
      set pinlist [xcircuit::getpinlist [page label]]
      if {[xcircuit::getsubckttext [page label]] != {}} {
         set orderedpins 1
      } else {
         set orderedpins 0
      }
   } else {
      set orderedpins 1
   }

   # Break the pinlist up into 4 parts

   set rightpins [expr [llength $pinlist] / 2]
   set bottompins  [expr [llength $pinlist] - $rightpins]
   set leftpins [expr $rightpins / 2]
   set rightpins [expr $rightpins - $leftpins]
   set toppins [expr $bottompins / 2]
   set bottompins [expr $bottompins - $toppins]
   incr leftpins $rightpins
   incr bottompins $leftpins
   incr toppins $bottompins

   for {set k 0} {$k < $rightpins} {incr k} {
      ${w}.pinself.right.list insert end [lindex $pinlist $k]
   }
   for {} {$k < $leftpins} {incr k} {
      ${w}.pinself.left.list insert end [lindex $pinlist $k]
   }
   for {} {$k < $bottompins} {incr k} {
      ${w}.pinself.bottom.list insert end [lindex $pinlist $k]
   }
   for {} {$k < $toppins} {incr k} {
      ${w}.pinself.top.list insert end [lindex $pinlist $k]
   }

   pack ${w}.pinself -side top -anchor w -padx 20 -pady 5 -fill y -expand true

   catch {
      if {$orderedpins == 0} {
         button ${w}.bbar.remove -text "Remove Pin" -bg beige -command \
	   {xcircuit::removeselectedpin}
      }
      button ${w}.bbar.moveleft -text "Move Left" -bg beige -command \
	{xcircuit::movepinleft}
      button ${w}.bbar.movetop -text "Move Top" -bg beige -command \
	{xcircuit::movepintop}
      button ${w}.bbar.moveright -text "Move Right" -bg beige -command \
	{xcircuit::movepinright}
      button ${w}.bbar.movebottom -text "Move Bottom" -bg beige -command \
	{xcircuit::movepinbottom}
   }
   config suspend false

   if {$orderedpins == 0} {
      pack ${w}.bbar.remove -side left -ipadx 10
   }
   pack ${w}.bbar.moveleft -side left -ipadx 10
   pack ${w}.bbar.movetop -side left -ipadx 10
   pack ${w}.bbar.moveright -side left -ipadx 10
   pack ${w}.bbar.movebottom -side left -ipadx 10
}

#---------------------------------------------------------------------------
# Remove the pin arranger widget from the dialog box.
#---------------------------------------------------------------------------

proc xcircuit::removepinarranger {w} {
   catch {
      pack forget ${w}.pinself
      destroy ${w}.pinself
      pack forget ${w}.bbar.movebottom
      pack forget ${w}.bbar.moveright
      pack forget ${w}.bbar.movetop
      pack forget ${w}.bbar.moveleft
      pack forget ${w}.bbar.remove
   }
} 

#---------------------------------------------------------------------------
# Redefine the procedure for the "Make Matching Symbol" menu button.
#---------------------------------------------------------------------------

proc xcircuit::promptmakesymbol {{name ""}} {
  global XCOps
  .makesymbol.bbar.okay configure -command \
          {if {[string first "Page " [page label]] >= 0} { \
          page label [.makesymbol.textent.txt get]}; \
          xcircuit::makesymbol; \
          wm withdraw .makesymbol}
  .makesymbol.textent.title.field configure -text "Confirm symbol name:"
  .makesymbol.textent.txt delete 0 end
  if {[string length $name] == 0 && [string first "Page " [page label]] < 0} {
      set name [page label]}
  .makesymbol.textent.txt insert 0 $name

  xcircuit::popupdialog .makesymbol
  xcircuit::addtechlist .makesymbol "Technology: "
  xcircuit::addliblist .makesymbol "Place in: "
  xcircuit::addpinarranger .makesymbol
}        

#---------------------------------------------------------------------------
# Routine which parses a spice file for the first .subckt line and
# generates a symbol to match, with a "%F" escape pointing to the
# spice file to include.
#---------------------------------------------------------------------------

proc xcircuit::spice2symbol {filename {subcktname ""}} {
  global XCOps

  set f [open $filename]
  set infolabel ""
  while {[gets $f line] >= 0} {
     set dnline [string tolower $line]
     if {[string first .subckt $dnline] == 0} {
	while {[gets $f nextline] >= 0} {
	   if {[string first + $nextline] != 0} {break}
	   append line [string range $nextline 1 end]
	}
	set infolabel $line
	if {$subcktname == ""} {break}
	if {[string compare $subcktname [lindex $infolabel 1]] == 0} {break}
     }
  }
  close $f

  if {[string length $infolabel] == 0} {return}
  set pinlabels [lrange $infolabel 2 end]

  .makesymbol.bbar.okay configure -command \
          "if {[string first {Page } [page label]] >= 0} { \
          page label [.makesymbol.textent.txt get]}; \
          xcircuit::makesymbol $filename [list $pinlabels]; \
          wm withdraw .makesymbol"
  .makesymbol.textent.title.field configure -text "Confirm symbol name:"
  .makesymbol.textent.txt delete 0 end
  .makesymbol.textent.txt insert 0 [lindex $infolabel 1]
  xcircuit::popupdialog .makesymbol
  xcircuit::addliblist .makesymbol "Place in: "
  xcircuit::addpinarranger .makesymbol $pinlabels
}

#---------------------------------------------------------------------------
# Procedure that creates the dialog to find a spice file to parse and
# calls spice2symbol.
#---------------------------------------------------------------------------

proc xcircuit::promptspicesymbol {} {
  global XCOps
  .filelist.bbar.okay configure -command \
	{xcircuit::spice2symbol [.filelist.textent.txt get] ; \
	wm withdraw .filelist}
  .filelist.listwin.win configure -data "cir"
  .filelist.textent.title.field configure -text "Select spice file to parse:"
  .filelist.textent.txt delete 0 end
  xcircuit::popupfilelist
  xcircuit::removelists .filelist
}

#---------------------------------------------------------------------------
# Add a menu item to invoke promptspicesymbol.
#---------------------------------------------------------------------------

set m .xcircuit.menubar.netlistbutton.netlistmenu
$m insert 7 command -label "SPICE to symbol" -command {xcircuit::promptspicesymbol}
unset m

#---------------------------------------------------------------------------
