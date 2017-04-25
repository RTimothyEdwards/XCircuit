#-----------------------------------------------------------------------
# Set of Tcl procedures to convert an xcircuit schematic to an initial
# (unrouted) PCB layout.
#
# Written by Tim Edwards for MultiGiG Inc., February 27, 2004.
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Procedure to convert xcircuit strings to plain ASCII text.
#-----------------------------------------------------------------------

proc string_to_text {xcstr} {
   set rst ""
   foreach sst $xcstr {
      switch -glob $sst {
	 Text* {
	    set atxt [lindex $sst 1]
	    append rst $atxt
	 }
      }
   }
  return $rst
}

#-----------------------------------------------------------------------
# Procedure to map XCircuit names to PBC names.  This is just a hack.
# Ideally, we would want an XCircuit library whose names map directly
# into the PCB library, so no translation is needed.  There would be
# parameters for the description line, package line, and name line to
# be passed to the program.
#-----------------------------------------------------------------------
# Currently, understands 7400 series devices from the "diplib" and
# "newdiplib" libraries. ("DIP7400" or "dil_7400" maps to "7400_dil",
# and so forth).
#-----------------------------------------------------------------------
# Understanding of axial and SMD resistors and capacitors added
# 1/4/05.  This is somewhat hacked-up.  PCB has it backwards: The
# package type should be, e.g., "SMD_SIMPLE_603", not "SMD_SIMPLE",
# while the name should be "smd_resistor", not "smd_resistor_603".
#-----------------------------------------------------------------------

proc xcirc_to_pcbname {xcname {pkgparam ""}} {
   switch -glob $xcname {
      dil_* {
	 set lstyp [string range $xcname 4 end]
	 set rst "${lstyp}_dil"
      }
      DIP* {
	 set lstyp [string range $xcname 3 end]
	 set rst "${lstyp}_dil"
      }
      res* -
      Res* {
	 switch -glob $pkgparam {
	    SMD* {
	       set lpos [string last "_" $pkgparam]
	       set pkgsize [string range $pkgparam $lpos end]
	       set rst "smd_resistor$pkgsize"
	    }
	    AXIAL* {
	       set lpos [string last "_" $pkgparam]
	       set pkgsize [string range $pkgparam $lpos end]
	       set rst "generic_resistor_axial$pkgsize"
	    }
	    default {
	       set rst "generic_resistor_axial_400"
	    }
	 }
      }
      cap* -
      Cap* {
	 switch -glob $pkgparam {
	    SMD* {
	       set lpos [string last "_" $pkgparam]
	       set pkgsize [string range $pkgparam $lpos end]
	       set rst "smd_capacitor$pkgsize"
	    }
	    AXIAL* {
	       set lpos [string last "_" $pkgparam]
	       set pkgsize [string range $pkgparam $lpos end]
	       set rst "generic_capacitor_axial$pkgsize"
	    }
	    default {
	       set rst "generic_capacitor_axial_400"
	    }
	 }
      }
      default {set rst $xcname}
   }
   return $rst
}

#-----------------------------------------------------------------------
# Procedure to map XCircuit names to PCB package names.
#-----------------------------------------------------------------------
# Currently, understands DIP packages (7400 series)
#-----------------------------------------------------------------------

proc xcirc_to_pkgname {pkgparam numpins} {
   switch -glob $pkgparam {
      DIP {set rst "DIP$numpins"}
      SMD* {set rst SMD_SIMPLE}
      AXIAL* {set rst AXIAL_LAY}
      default {set rst $pkgparam}
   }
   return $rst
}

#-----------------------------------------------------------------------
# Add an element to the (open) pcb file
#-----------------------------------------------------------------------

proc add_pcb_element {fileId element numpins devname pkgparam} {
   global PCBLIBDIR SUBDIR GEN_ELEM_SCRIPT
   set pcbelem [xcirc_to_pcbname $element $pkgparam]
   set pkgname [xcirc_to_pkgname $pkgparam $numpins]
   set elist [exec ${PCBLIBDIR}/${GEN_ELEM_SCRIPT} ${PCBLIBDIR} ${SUBDIR} \
	$pcbelem $element $pkgname]
   eval [subst {regsub {""} \$elist {"$devname"} efinal}]
   puts stdout "IN: QueryLibrary.sh pcblib $pcbelem $element $pkgname"
   puts stdout "OUT: $efinal"
   puts $fileId $efinal
}

#-----------------------------------------------------------------------
# Generate elements from the xcircuit schematic
#
# This assumes a pcb-like schematic, with a flat schematic.  Needs to be
# expanded to handle hierarchical schematics.  This will work with
# multipage schematics.
#
# For now, this only supports 7400 series devices, so we can assume that
# the string is "U".  However, different packages of the devices can be
# specified, and any device name can be processed as long as it has a
# valid package type (DIL, SO, US, etc.).
#
# Extended 1/4/05 to handle resistors and capacitors.
#-----------------------------------------------------------------------

proc gen_pcb_elements {fileId} {
   set numpins 0
   set pkgname "unknown"

   set nl [netlist make]
   set ckt [lindex $nl 3]

   foreach subckt $ckt {
      set ename "default"
      # MUST parse in order: name, device, (everything else)
      foreach {key value} $subckt {
	 switch -- $key {
	    name {
	       set ename $value
	       break;
	    }
	 }
      }
      foreach {key value} $subckt {
	 switch -- $key {
	    devices {
	       foreach dev $value {
		  foreach tpart $dev {
		     set ascl [string_to_text $tpart]
		     set cpos [string first "pcb:" $ascl]
		     if {$cpos >= 0} {
		        set defpfix($ename) [string range $ascl 4 end]
			break
		     }
		  }
	       }
	    }
	 }
      }
      foreach {key value} $subckt {
	 switch -- $key {
	    name {set ename $value}
	    ports {set defpins($ename) [expr {[llength $value] / 2}]}
	    parameters {
	       foreach {pkey pval} $value {
		  switch -- $pkey {
		     idx -
		     v1 {
			set devnum [string_to_text $pval]
			set defnum($ename) "${devnum}"
		     }
		     pkg {
			set defpkg($ename) [string_to_text $pval]
		     }
		  }
	       }
	    }
	 }
      }
   }

   set top [lindex $ckt [expr {[llength $ckt] - 1}]]
   foreach {key calllist} $top {
      if {$key == "calls"} {break}
   }
   set devnum 0 
   foreach call $calllist {
      set numpins 0
      incr devnum
      set devpfix "U"
      set devname "U?"
      set pkgname "unknown"
      set ename "default"
      foreach {key value} $call {
	 switch -- $key {
	    name {
	       set ename $value
	       catch {set numpins $defpins($ename)}
	       catch {set devpfix $defpfix($ename)}
	       catch {set pkgname $defpkg($ename)}
	       catch {set locdevnum $defnum($ename); \
		      set devname ${devpfix}${locdevnum}}
	    }
	    ports {set numpins [expr {[llength $value] / 2}]}
	    parameters {
	       foreach {pkey pval} $value {
		  switch -- $pkey {
		     idx -
		     v1 {
			set locdevnum [string_to_text $pval]
			set devname "${devpfix}${locdevnum}"
		     }
		     pkg {
			set pkgname [string_to_text $pval]
		     }
		  }
	       }
	    }
	 }
      }
      if {[string range $devname end end] == "?"} {set devname "${devpfix}${devnum}"}
      add_pcb_element $fileId $ename $numpins $devname $pkgname
   }
}

#-----------------------------------------------------------------------
# Tcl procedure to write an xcircuit layout to an initial PCB layout.
#-----------------------------------------------------------------------

proc xcirc_to_pcb {filename} {
   global PCBLIBDIR SUBDIR GEN_ELEM_SCRIPT

   # Open the pcb file and generate a valid header
   set fileId [open $filename w 0600]
   puts $fileId "PCB(\"\" 6000 5000)"
   puts $fileId ""
   puts $fileId "Grid(10 0 0 0)"
   puts $fileId "Cursor(160 690 3)"
   puts $fileId "Flags(0x00000000000006d0)"
   puts $fileId "Groups(\"1,s:2,c:3:4:5:6:7:8\")"
   puts $fileId "Styles(\"Signal,10,55,28,10:Power,25,60,35,10:Fat,40,60,35,10:Skinny,8,36,20,7\")"
   puts $fileId ""

   # Generate elements
   gen_pcb_elements $fileId

   # Generate a valid trailer and close the pcb file

   puts $fileId ""
   puts $fileId "Layer(1 \"solder\")"
   puts $fileId "("
   puts $fileId ")"
   puts $fileId "Layer(2 \"component\")"
   puts $fileId "("
   puts $fileId ")"
   puts $fileId "Layer(3 \"GND\")"
   puts $fileId "("
   puts $fileId ")"
   puts $fileId "Layer(4 \"power\")"
   puts $fileId "("
   puts $fileId ")"
   puts $fileId "Layer(5 \"signal1\")"
   puts $fileId "("
   puts $fileId ")"
   puts $fileId "Layer(6 \"signal2\")"
   puts $fileId "("
   puts $fileId ")"
   puts $fileId "Layer(7 \"unused\")"
   puts $fileId "("
   puts $fileId ")"
   puts $fileId "Layer(8 \"unused\")"
   puts $fileId "("
   puts $fileId ")"
   puts $fileId "Layer(9 \"silk\")"
   puts $fileId "("
   puts $fileId ")"
   puts $fileId "Layer(10 \"silk\")"
   puts $fileId "("
   puts $fileId ")"
   close $fileId
}

#-----------------------------------------------------------------------
# Create a dialog for querying the name of the output layout file
#-----------------------------------------------------------------------

proc xcircuit::promptpcblayout {} {
   .dialog.bbar.okay configure -command \
	{xcirc_to_pcb [.dialog.textent.txt get]; wm withdraw .dialog}
   .dialog.textent.title.field configure -text "Enter name for PCB layout:"
   .dialog.textent.txt delete 0 end
   set lname [xcircuit::page label]
   append lname ".pcb"
   .dialog.textent.txt insert 0 $lname
   wm deiconify .dialog
   focus .dialog.textent.txt
}     

# These may be reset at any time. . .

set PCBLIBDIR "/usr/local/share/pcb/"
set SUBDIR "pcblib"
set GEN_ELEM_SCRIPT "QueryLibrary.sh"

#-----------------------------------------------------------------------
# Add the PCB layout command to the XCircuit "Netlist" menu.
#-----------------------------------------------------------------------

set m .xcircuit.menubar.netlistbutton.netlistmenu
$m add command -label "Create PCB Layout" -command {xcircuit::promptpcblayout}

#-----------------------------------------------------------------------
