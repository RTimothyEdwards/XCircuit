#----------------------------------------
# Library management for XCircuit
#----------------------------------------

set XCOps(module,library) 1

#----------------------------------------

proc xcircuit::raisemanager {} {
   makelibmanager
   set midx [.xcircuit.menubar.filebutton.filemenu index *Manager*]
   .xcircuit.menubar.filebutton.filemenu entryconfigure \
         $midx -label "Hide Library Manager" -command {xcircuit::lowermanager}  
}

#----------------------------------------

proc xcircuit::lowermanager {} {
   destroy .libmgr
   set midx [.xcircuit.menubar.filebutton.filemenu index *Manager*]
   .xcircuit.menubar.filebutton.filemenu entryconfigure \
         $midx -label "Library Manager" -command {xcircuit::raisemanager}  
}

#----------------------------------------

proc xcircuit::promptadddirectory {} {
   .filelist.bbar.okay configure -command \
        {.libmgr.search.list insert 0 [.filelist.textent.txt get];\
         updatelibmanager; wm withdraw .filelist}
   .filelist.listwin.win configure -data "lps"
   .filelist.textent.title.field configure -text "Select directory to add to search:"
   .filelist.textent.txt delete 0 end
   wm deiconify .filelist
   focus .filelist.textent.txt
}

#----------------------------------------

proc xcircuit::promptnewlibrary {} {
  .dialog.bbar.okay configure -command \
        {library make [.dialog.textent.txt get]; updatelibmanager; \
	wm withdraw .dialog}
  .dialog.textent.title.field configure -text "Enter name for new library page:"
  .dialog.textent.txt delete 0 end
  wm deiconify .dialog
  focus .dialog.textent.txt
}

#----------------------------------------

proc xcircuit::prompttechtarget {} {
  .dialog.bbar.okay configure -command \
	{foreach i [.libmgr.object.list curselection] \
	{technology objects [.dialog.textent.txt get] \
	[.libmgr.object.list get $i]}; \
        updatelibmanager; wm withdraw .dialog}
  .dialog.textent.title.field configure -text "Enter name of target technology:"
  .dialog.textent.txt delete 0 end
  wm deiconify .dialog
  focus .dialog.textent.txt
}

#----------------------------------------

proc makelibmanager {} {
   global XCOps
   global XCIRCUIT_LIB_DIR

   toplevel .libmgr

   wm protocol .libmgr WM_DELETE_WINDOW {xcircuit::lowermanager}

   label .libmgr.title1 -text "Source Technology File" -foreground "blue"
   label .libmgr.title2 -text "Target Library Page" -foreground "blue"
   label .libmgr.title3 -text "Search Directories" -foreground "blue"
   frame .libmgr.tblock

   label .libmgr.tblock.title -text "Objects" -foreground "blue"

   menubutton .libmgr.srclib -text "" -menu .libmgr.srclib.menu -relief groove
   menubutton .libmgr.tgtlib -text "" -menu .libmgr.tgtlib.menu -relief groove

   frame .libmgr.object
   frame .libmgr.search

   listbox .libmgr.object.list -relief groove -background white \
	-yscrollcommand ".libmgr.object.scroll set" -width 30 \
	-selectmode extended
   listbox .libmgr.search.list -relief groove -background white \
	-yscrollcommand ".libmgr.object.scroll set" -width 30

   scrollbar .libmgr.object.scroll -orient vertical \
	-command ".libmgr.object.list yview"
   scrollbar .libmgr.search.scroll -orient vertical \
	-command ".libmgr.search.list yview"

   pack .libmgr.object.list	-side left -fill both -expand true
   pack .libmgr.object.scroll   -side right -fill y

   pack .libmgr.search.list	-side left -fill both -expand true
   pack .libmgr.search.scroll   -side right -fill y

   frame .libmgr.buttons

   button .libmgr.buttons.add -text "Add Directory" \
	-command {xcircuit::promptadddirectory}
   button .libmgr.buttons.new -text "Add New Library Page" \
	-command {xcircuit::promptnewlibrary}
   button .libmgr.buttons.done -text "Load Selected" \
	-command {foreach i [.libmgr.object.list curselection] \
	{library $XCOps(tgtlib) import $XCOps(srclib) \
	[.libmgr.object.list get $i]}; updateobjects $XCOps(srclib); \
	refresh}
   button .libmgr.buttons.all -text "Load All" \
	-command {library $XCOps(tgtlib) load $XCOps(srclib); \
	updateobjects $XCOps(srclib); refresh}
   button .libmgr.buttons.move -text "Move Selected" \
	-command {xcircuit::prompttechtarget}

   checkbutton .libmgr.tblock.check -text "Show Loaded" \
	-variable XCOps(showloaded) -onvalue 1 -offvalue 0 \
	-command {updateobjects $XCOps(srclib)}

   set XCOps(showloaded) 0

   pack .libmgr.buttons.add -side left
   pack .libmgr.buttons.new -side left
   pack .libmgr.buttons.done -side left
   pack .libmgr.buttons.all -side left
   # pack .libmgr.buttons.move -side left

   grid .libmgr.title1 -row 0 -column 0 -sticky news
   grid .libmgr.title2 -row 0 -column 1 -sticky news
   grid .libmgr.title3 -row 2 -column 0 -sticky news
   grid .libmgr.tblock -row 2 -column 1 -sticky news

   pack .libmgr.tblock.title  -side left
   pack .libmgr.tblock.check  -side left

   grid .libmgr.srclib -row 1 -column 0 -sticky news -padx 4
   grid .libmgr.tgtlib -row 1 -column 1 -sticky news -padx 4
   grid .libmgr.object -row 3 -column 1 -sticky news
   grid .libmgr.search -row 3 -column 0 -sticky news
   grid .libmgr.buttons -row 4 -column 0 -columnspan 2 -sticky news

   grid columnconfigure .libmgr 0 -weight 1
   grid columnconfigure .libmgr 1 -weight 1
   grid rowconfigure .libmgr 3 -weight 1

   menu .libmgr.srclib.menu -tearoff 0
   menu .libmgr.tgtlib.menu -tearoff 0

   # Initial set of library directories to search
   set searchpath [config search lib]
   if {$searchpath == {}} {
      .libmgr.search.list insert end $XCIRCUIT_LIB_DIR
      .libmgr.search.list insert end "."
   } else {
      set searchlist [string map {: " "} $searchpath]
      foreach i $searchlist {
         .libmgr.search.list insert end $i
      }
   }

   # Do an update
   updatelibmanager
}

#----------------------------------------

proc updateobjects {techfile} {
   global XCOps

   # Erase the current entries
   .libmgr.object.list delete 0 end
   catch unset XObjs(objects)

   # Find all the objects defined in library "techfile"
   set objlist {}
   set fileId [open $techfile]
   foreach line [split [read $fileId] \n] {
      switch -glob -- $line {
	 "/* \{" {
	    lappend objlist [string range [lindex [split $line] 0] 1 end]
	 }
      }
   }
   close $fileId

   set objlist [lsort -dictionary $objlist]
   foreach objname $objlist {
      if {[catch {object handle $objname}]} {
	 .libmgr.object.list insert end $objname
      } elseif {$XCOps(showloaded) == 1} {
	 .libmgr.object.list insert end $objname
	 .libmgr.object.list itemconfigure end -fg forestgreen
      }
   }
}

#----------------------------------------------
# Filename sorting function---sort by root name
#----------------------------------------------

proc FileCompare {a b} {
   set aname [file rootname [file tail $a]]
   set bname [file rootname [file tail $b]]
   set res [string compare $aname $bname]
   if {$res != 0} {
      return $res
   } else {
      # If the root matches, then order by order of search directory
      set dlist [.libmgr.search.list get 0 end]
      set aidx [lsearch $dlist [file dirname $a]]
      set bidx [lsearch $dlist [file dirname $b]]
      if {$aidx > $bidx} {	
	 return 1
      } elseif {$aidx < $bidx} {
	 return -1
      } else {
	 return 0
      }
   }
}

#----------------------------------------

proc updatelibmanager {} {
   global XCOps

   # Get the list of directories in which to search for libraries
   set dirlist [.libmgr.search.list get 0 end]

   # Erase the current entries
   .libmgr.tgtlib.menu delete 0 end
   .libmgr.srclib.menu delete 0 end

   # Create the list of source libraries
   set srclist {}
   foreach i $dirlist {
      if {[file isdirectory $i]} {       
	 set srclist [concat $srclist [glob -nocomplain ${i}/*.lps]]
      }
   }
   set srclist [lsort -command FileCompare $srclist]

   foreach j $srclist {
      set libname [file rootname [file tail $j]]
      if {[.libmgr.srclib.menu entrycget end -label] != $libname} {
	 .libmgr.srclib.menu add radio -label $libname -variable XCOps(srclib) \
		-value $j -command ".libmgr.srclib configure \
		-text $libname; updateobjects $j"
      }
   }

   # Reset "Source Technology" only if not set before, or if the name doesn't match
   # the menu entry.
   set cres [catch {set entry [.libmgr.srclib.menu index [file rootname \
		[file tail $XCOps(srclib)]]]}]
   if {$cres == 0} {
      if {[.libmgr.srclib cget -text] == [.libmgr.srclib.menu entrycget $entry -label]} {
	 set cres 1
      }
   }
   if {$cres == 1} {
      set XCOps(srclib) [.libmgr.srclib.menu entrycget 0 -value]
      .libmgr.srclib configure -text [.libmgr.srclib.menu entrycget 0 -label]
   }

   # Find the number of technologies in XCircuit
   set numlibs [library "User Library"]

   # Create the list of target libraries
   for {set i 1} {$i <= $numlibs} {incr i} {
      .libmgr.tgtlib.menu add radio -label [library $i] -variable XCOps(tgtlib) \
		-value $i -command ".libmgr.tgtlib configure -text {[library $i]}"
   }

   # Reset "Target Library" only if not set before, or if the name doesn't match
   # the menu entry.
   set cres [catch {set entry [.libmgr.srclib.menu index $XCOps(tgtlib)]}]
   if {$cres == 0} {
      if {[.libmgr.tgtlib cget -text] == [.libmgr.tgtlib.menu entrycget $entry -label]} {
	 set cres 1
      }
   }
   if {$cres == 1} {
      set XCOps(tgtlib) $numlibs
      .libmgr.tgtlib configure -text [library $numlibs]
   }

  # Create the object list for the first source library entry
  updateobjects $XCOps(srclib)
}

#----------------------------------------
