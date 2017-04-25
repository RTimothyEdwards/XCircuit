#---------------------------------------------------------------------------
# files.tcl ---
#
#	xcircuit Tcl script for handling multiple-file writes
#
#	Tim Edwards 12/5/05 for MultiGiG
#---------------------------------------------------------------------------

set XCOps(module,files) 1

#---------------------------------------------------------------------------
# Forced write all
#---------------------------------------------------------------------------

proc xcircuit::forcewriteall {} {
   set pages [page links all]
   for {set i 1} {$i <= $pages} {incr i} {
      page $i changes 1
      set techlist [technology used $i]
      foreach tech $techlist {
	 if {[technology writable $tech]} {technology changed $tech 1}
      }
   }
   destroy .writeall
   xcircuit::promptwriteall
}

#---------------------------------------------------------------------------
# Write all pages.  Determine which pages are enabled/disabled in the
# ".writeall" window, and save each one.
#---------------------------------------------------------------------------

proc xcircuit::writeall {} {
   global XCOps

   if {![catch {raise .writeall}]} {
      set rows [lindex [grid size .writeall.list] 1]
      incr rows -1
      for {set i 1} {$i <= $rows} {incr i} {
	 if {$XCOps(cbox${i}) > 0} {
	    set pagelist [pack slaves .writeall.list.pframe${i}]
	    set firstpage [lindex $pagelist 0]
	    set pageno [${firstpage} cget -text]
	    if {$pageno == "-" || $pageno == "X"} {
	       set tname [.writeall.list.f${i} cget -text]
	       set fname [technology filename $tname]
	       if {$fname == "(no associated file)"} {
		  # Fix me:  this works only on one technology at a time!
		  xcircuit::promptsavetech
	       } else {
	          technology save $tname
	       }
	    } else {
	       page ${pageno} update
	       page ${pageno} save
	    }
	 }
	 unset XCOps(cbox${i})
      }
      destroy .writeall
   }
}

#---------------------------------------------------------------------------
# Prompt for writing all pages
#---------------------------------------------------------------------------

proc xcircuit::promptwriteall {} {
   global XCOps

   # Get list of unique filenames.

   set pages [page links all]
   set filelist {}
   for {set i 1} {$i <= $pages} {incr i} {
      set changes [page $i changes]
      if {$changes > 0} {
	 if {[page $i filename] == {}} {
	    page $i filename [page $i label]
	 }
         lappend filelist [page $i filename]
      }
   }
   set filelist [lsort -unique -dictionary $filelist]

   set techlist {}
   set badtechslist {}
   foreach tech [technology list] {
      if {[technology changed $tech] > 0} {
	 if {[technology writable $tech] > 0} {
	    lappend techlist $tech
	 } else {
	    lappend badtechslist $tech
	 }
      }
   }

   set filestowrite [expr [llength $filelist] + [llength $techlist]]

   # Set up the "writeall" window

   catch {destroy .writeall}
   toplevel .writeall -bg beige
   frame .writeall.tbar -bg beige
   frame .writeall.list -bg beige
   frame .writeall.bbar -bg beige

   pack .writeall.tbar -side top -padx 5 -pady 5 -fill x
   pack .writeall.list -side top -padx 5 -pady 5 -fill x
   pack .writeall.bbar -side top -padx 5 -pady 5 -fill x

   if {$filestowrite == 0} {
      label .writeall.tbar.title -text \
		"(No modified pages or technologies to write)" -bg \
		beige -fg gray40
      pack .writeall.tbar.title -side left
   } else {
      label .writeall.tbar.title -text "Write All Modified Pages and Technologies:" \
		-bg beige -fg blue
      pack .writeall.tbar.title -side left

      label .writeall.list.ftitle -text "Filename" -bg beige -fg brown
      label .writeall.list.ptitle -text "Pages" -bg beige -fg brown
      label .writeall.list.wtitle -text "Write?" -bg beige -fg brown
      grid .writeall.list.ftitle -row 0 -column 0 -sticky news
      grid .writeall.list.ptitle -row 0 -column 1 -sticky news
      grid .writeall.list.wtitle -row 0 -column 2 -sticky news
   }

   set k 0
   foreach j $filelist {
      incr k
      label .writeall.list.f${k} -text "$j" -anchor w -bg beige
      frame .writeall.list.pframe${k} -bg beige
      set XCOps(cbox${k}) 1
      checkbutton .writeall.list.cbox${k} -bg beige -variable XCOps(cbox${k})

      set l 0
      for {set i 1} {$i <= $pages} {incr i} {
	 if {[page $i filename] == $j} {
            set changes [page $i changes]
            if {$changes > 0} {
	       incr l
	       button .writeall.list.pframe${k}.b${l} -text "$i" -bg \
			white -command "page goto $i ; \
			promptsavepage"
	       pack .writeall.list.pframe${k}.b${l} -side left -padx 5
	    }
         }
      }
      grid .writeall.list.f${k} -row $k -column 0 -sticky news
      grid .writeall.list.pframe${k} -row $k -column 1 -sticky news
      grid .writeall.list.cbox${k} -row $k -column 2 -sticky news
   }

   foreach j $techlist {
      incr k
      label .writeall.list.f${k} -text "$j" -anchor w -bg beige
      frame .writeall.list.pframe${k} -bg beige
      set XCOps(cbox${k}) 1
      checkbutton .writeall.list.cbox${k} -bg beige -variable XCOps(cbox${k})

      button .writeall.list.pframe${k}.b1 -text "-" -bg white \
			-command "set XCOps(technology) $j ; \
				  xcircuit::promptsavetech"
      pack .writeall.list.pframe${k}.b1 -side left -padx 5
      grid .writeall.list.f${k} -row $k -column 0 -sticky news
      grid .writeall.list.pframe${k} -row $k -column 1 -sticky news
      grid .writeall.list.cbox${k} -row $k -column 2 -sticky news
   }

   foreach j $badtechslist {
      incr k
      label .writeall.list.f${k} -text "$j" -anchor w -bg beige -fg gray60
      frame .writeall.list.pframe${k} -bg beige
      set XCOps(cbox${k}) 0
      checkbutton .writeall.list.cbox${k} -bg beige -variable XCOps(cbox${k})

      button .writeall.list.pframe${k}.b1 -text "X" -bg white -fg gray60 \
			-command "set XCOps(technology) $j ; \
				  xcircuit::promptsavetech"
      pack .writeall.list.pframe${k}.b1 -side left -padx 5
      grid .writeall.list.f${k} -row $k -column 0 -sticky news
      grid .writeall.list.pframe${k} -row $k -column 1 -sticky news
      grid .writeall.list.cbox${k} -row $k -column 2 -sticky news
   }

   if {$filestowrite > 0} {
      button .writeall.bbar.okay -text Write -bg beige -command \
		{xcircuit::writeall}
      pack .writeall.bbar.okay -side left -ipadx 10 -padx 5
   }

   if {$filestowrite < $pages} {
      button .writeall.bbar.force -text Force -bg beige -command \
	{xcircuit::forcewriteall}
      pack .writeall.bbar.force -side left -ipadx 10 -padx 5
   }

   button .writeall.bbar.cancel -text Cancel -bg beige -command \
	{destroy .writeall}
   pack .writeall.bbar.cancel -side right -ipadx 10 -padx 5
}

