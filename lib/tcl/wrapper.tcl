#-------------------------------------------------------------------------
# Start of xcircuit GUI configuration file.
# This file is sourced by "xcircuit.tcl" (".wishrc")
#-------------------------------------------------------------------------

# This script sets up all the xcircuit windows and callback functions.
# The callback routines are in the shared object file xcwrap.so
#

#-------------------------------
# Main xcircuit drawing window
#-------------------------------

proc xcircuit::new_window { name } {

  global XCIRCUIT_VERSION XCIRCUIT_REVISION XCOps XCWinOps tcl_platform

  toplevel $name
  wm title $name XCircuit
  wm group $name .
  wm protocol $name WM_DELETE_WINDOW \
	"xcircuit::closewindow ${name}.mainframe.mainarea.drawing"

  # All the internal frames

  frame ${name}.menubar
  frame ${name}.infobar
  frame ${name}.mainframe

  grid propagate ${name} false
  grid ${name}.menubar -sticky news -row 0 -column 0
  grid ${name}.mainframe -sticky news -row 1 -columnspan 2
  grid ${name}.infobar -sticky news -row 2 -columnspan 2

  grid rowconfigure ${name} 0 -weight 0
  grid rowconfigure ${name} 1 -weight 1
  grid rowconfigure ${name} 2 -weight 0

  grid columnconfigure ${name} 0 -weight 0
  grid columnconfigure ${name} 1 -weight 1

  frame ${name}.mainframe.mainarea
  frame ${name}.mainframe.toolbar

  pack ${name}.mainframe.toolbar -side right -fill y
  pack ${name}.mainframe.mainarea -expand true -fill both

  set drawing ${name}.mainframe.mainarea.drawing
  simple $drawing -bg white -commandproc "focus $drawing ; set XCOps(focus) $name"

  simple ${name}.mainframe.mainarea.sbleft -width 13
  simple ${name}.mainframe.mainarea.sbbottom -height 13
  simple ${name}.mainframe.mainarea.corner -width 13 -height 13

  # The drawing area and its scrollbars

  grid ${name}.mainframe.mainarea.sbleft -row 0 -column 0 -sticky ns
  grid ${name}.mainframe.mainarea.sbbottom -row 1 -column 1 -sticky ew
  grid $drawing -row 0 -column 1 -sticky news
  grid ${name}.mainframe.mainarea.corner -row 1 -column 0 -sticky news

  grid rowconfigure ${name}.mainframe.mainarea 0 -weight 1
  grid columnconfigure ${name}.mainframe.mainarea 1 -weight 1

  # The top menu and message bar

  menubutton ${name}.menubar.filebutton -text File \
	-menu ${name}.menubar.filebutton.filemenu
  menubutton ${name}.menubar.editbutton -text Edit \
	-menu ${name}.menubar.editbutton.editmenu
  menubutton ${name}.menubar.textbutton -text Text \
	-menu ${name}.menubar.textbutton.textmenu
  menubutton ${name}.menubar.optionsbutton -text Options \
	-menu ${name}.menubar.optionsbutton.optionsmenu
  menubutton ${name}.menubar.windowbutton -text Window \
	-menu ${name}.menubar.windowbutton.windowmenu
  menubutton ${name}.menubar.netlistbutton -text Netlist \
	-menu ${name}.menubar.netlistbutton.netlistmenu 

  grid ${name}.menubar.filebutton ${name}.menubar.editbutton \
	${name}.menubar.textbutton ${name}.menubar.optionsbutton \
	${name}.menubar.windowbutton ${name}.menubar.netlistbutton \
	-ipadx 10 -sticky news

  # The top message bar

  label ${name}.message -text \
	"Welcome to Xcircuit v${XCIRCUIT_VERSION} rev ${XCIRCUIT_REVISION}" \
	-justify left -anchor w

  grid ${name}.message -row 0 -column 1 -sticky news -ipadx 10

  button ${name}.infobar.symb -text "Symbol" -bg gray30 -fg white
  button ${name}.infobar.schem -text "Schematic" -bg red -fg white
  button ${name}.infobar.mode -text "Wire Mode" -bg skyblue2 -fg gray20
  label ${name}.infobar.message1 -text "Editing: Page 1"
  label ${name}.infobar.message2 -text "Grid 1/6 in : Snap 1/12 in" \
	-justify left -anchor w
  pack ${name}.infobar.symb ${name}.infobar.schem ${name}.infobar.message1 \
	${name}.infobar.mode -side left -ipadx 6 -fill y
  pack ${name}.infobar.message2 -ipadx 6 -expand true -fill both

  #-------------------------------------------------
  # Mouse hint window (if mousehints are enabled)
  #-------------------------------------------------

  if {$XCOps(mousehints) == 0} {
     bind ${name}.infobar.mode <Button-1> {xcircuit::enable_mousehints}
  } elseif {$XCOps(mousehints) == 1} {
     xcircuit::mousehint_create ${name}
  }

  #-------------------------------------------------
  # Create the menus, toolbar and associated tools
  #-------------------------------------------------

  xcircuit::makemenus $name
  xcircuit::createtoolbar $name
  xcircuit::arrangetoolbar $name
  xcircuit::allcolorbuttons $name
  xcircuit::allfontbuttons $name

  #-----------------------------------------------------------------
  # Add key and button bindings for XCircuit commands (standard actions)
  # These can be overridden by binding to specific keys and/or buttons.
  #-----------------------------------------------------------------

  bind $drawing <ButtonPress> {standardaction %b down %s}
  bind $drawing <ButtonRelease> {standardaction %b up %s}
  bind $drawing <KeyPress> {standardaction %k down %s}
  bind $drawing <KeyRelease> {standardaction %k up %s}

  # Here are some extra key functions that come with the TCL wrapper

  bind $drawing <Key-Next> {catch {page [expr {[page] + 1}]}}
  bind $drawing <Key-Prior> {catch {page [expr {[page] - 1}]}}
  bind $drawing <Control-Key-p> {xcircuit::prompteditparams}

  xcircuit::keybind <Key-bracketleft> {if {[select get] == {}} \
		{select here; element lower; deselect} else {element lower}; \
		refresh} $drawing
  xcircuit::keybind <Key-bracketright> {if {[select get] == {}} \
		{select here; element raise; deselect} else {element raise}; \
		refresh} $drawing

  # Bind numbers 1-9 and 0 so that they call the Tcl "page" command,
  # and so can take advantage of the tag callback to "pageupdate".

  xcircuit::keybind <Key-1> {page 1} $drawing
  xcircuit::keybind <Key-2> {page 2} $drawing
  xcircuit::keybind <Key-3> {page 3} $drawing
  xcircuit::keybind <Key-4> {page 4} $drawing
  xcircuit::keybind <Key-5> {page 5} $drawing
  xcircuit::keybind <Key-6> {page 6} $drawing
  xcircuit::keybind <Key-7> {page 7} $drawing
  xcircuit::keybind <Key-8> {page 8} $drawing
  xcircuit::keybind <Key-9> {page 9} $drawing
  xcircuit::keybind <Key-0> {page 10} $drawing

  # These are supposed to disable the scroll wheel on the scrollbars. . .

  if {$tcl_platform(platform) == "windows"} {
     bind $name <FocusIn> \
	"catch {config focus ${drawing} ; focus ${drawing}; \
	set XCOps(focus) ${name} ; xcircuit::updatedialog}"

     bind ${name}.mainframe.mainarea.sbleft <MouseWheel> {}
     bind ${name}.mainframe.mainarea.sbbottom <MouseWheel> {}

  } else {
     bind $drawing <Enter> {focus %W}
     bind $name <FocusIn> "catch {config focus $drawing ; \
	set XCOps(focus) ${name} ; xcircuit::updatedialog}"

     bind ${name}.mainframe.mainarea.sbleft <Button-4> {}
     bind ${name}.mainframe.mainarea.sbleft <Button-5> {}
     bind ${name}.mainframe.mainarea.sbbottom <Button-4> {}
     bind ${name}.mainframe.mainarea.sbbottom <Button-5> {}
  }

  # Window-specific variable defaults (variables associated with toggle
  # and radio buttons, etc.).  Note that we really should set these
  # defaults for the first window only, and subsequent windows should
  # inherit values from the first window.

  set XCWinOps(${name},button1) None
  set XCWinOps(${name},colorval) inherit
  set XCWinOps(${name},jhoriz) left
  set XCWinOps(${name},jvert) bottom
  set XCWinOps(${name},justif) left
  set XCWinOps(${name},linestyle) solid
  set XCWinOps(${name},fillamount) 0
  set XCWinOps(${name},opaque) false
  set XCWinOps(${name},polyedittype) manhattan
  set XCWinOps(${name},pathedittype) tangents
  set XCWinOps(${name},showgrid) true
  set XCWinOps(${name},showsnap) true
  set XCWinOps(${name},showaxes) true
  set XCWinOps(${name},showbbox) false
  set XCWinOps(${name},fontfamily) Helvetica
  set XCWinOps(${name},fontstyle) normal
  set XCWinOps(${name},fontencoding) ISOLatin1
  set XCWinOps(${name},fontlining) normal
  set XCWinOps(${name},fontscript) normal
  set XCWinOps(${name},gridstyle) "internal units"
  set XCWinOps(${name},flipinvariant) true
  set XCWinOps(${name},pinvisible) false
  set XCWinOps(${name},netlistable) true
  set XCWinOps(${name},showclipmasks) show
  set XCWinOps(${name},latexmode) false
  set XCWinOps(${name},colorscheme) normal
  set XCWinOps(${name},editinplace) true
  set XCWinOps(${name},pinpositions) invisible
  set XCWinOps(${name},pinattach) false
  set XCWinOps(${name},namespaces) false
  set XCWinOps(${name},centerobject) true
  set XCWinOps(${name},manhattandraw) false
  set XCWinOps(${name},polyclosed) closed
  set XCWinOps(${name},scaleinvariant) invariant
  set XCWinOps(${name},endcaps) round
  set XCWinOps(${name},bboxtype) false
  set XCWinOps(${name},clipmask) false
  set XCWinOps(${name},substringparam) false
  set XCWinOps(${name},numericparam) false
  set XCWinOps(${name},expressparam) false
  set XCWinOps(${name},xposparam) false
  set XCWinOps(${name},yposparam) false
  set XCWinOps(${name},styleparam) false
  set XCWinOps(${name},anchorparam) false
  set XCWinOps(${name},startparam) false
  set XCWinOps(${name},endparam) false
  set XCWinOps(${name},radiusparam) false
  set XCWinOps(${name},minorparam) false
  set XCWinOps(${name},rotationparam) false
  set XCWinOps(${name},scaleparam) false
  set XCWinOps(${name},linewidthparam) false
  set XCWinOps(${name},colorparam) false
  set XCWinOps(${name},sel_lab) true
  set XCWinOps(${name},sel_inst) true
  set XCWinOps(${name},sel_poly) true
  set XCWinOps(${name},sel_arc) true
  set XCWinOps(${name},sel_spline) true
  set XCWinOps(${name},sel_graphic) true
  set XCWinOps(${name},sel_path) true
  set XCWinOps(${name},labeltype) Text
  set XCWinOps(${name},labelimage) img_t
  set XCWinOps(${name},rotateamount) 15

  #-----------------------------------------------------------------
  # The "catch" statement here allows "i" and "I" to have other bindings for
  # normal mode (e.g., "I" for "make info label") when nothing is selected.
  #-----------------------------------------------------------------

  xcircuit::keybind i {if {[catch {xcircuit::autoincr}]} \
	{standardaction %k down %s}} $drawing
  xcircuit::keybind I {if {[catch {xcircuit::autoincr -1}]} \
	{standardaction %k down %s}} $drawing

  #-----------------------------------------------------------------
  # Function bindings for the mouse scroll wheel.
  # Note that Windows uses MouseWheel and direction passed as %D,
  # while Linux uses Button-4 and Button-5.
  #-----------------------------------------------------------------

  if {$tcl_platform(platform) == "windows"} {
     xcircuit::keybind <MouseWheel> {if { %D/120 >= 1} \
	  {pan up 0.1 ; refresh} else {pan down 0.1 ; refresh}} $drawing 
     xcircuit::keybind <Shift-MouseWheel> {if { %D/120 >= 1} \
	  {pan left 0.1 ; refresh} else {pan right 0.1 ; refresh}} $drawing
     xcircuit::keybind <Control-MouseWheel> {if { %D/120 >= 1} \
	  {zoom in ; refresh} else {zoom out ; refresh}} $drawing
  } else {
     xcircuit::keybind <Button-4> { pan up 0.05 ; refresh} $drawing
     xcircuit::keybind <Button-5> { pan down 0.05 ; refresh} $drawing
     xcircuit::keybind <Shift-Button-4> { pan left 0.05 ; refresh} $drawing
     xcircuit::keybind <Shift-Button-5> { pan right 0.05 ; refresh} $drawing
     xcircuit::keybind <Control-Button-4> { zoom in ; refresh} $drawing
     xcircuit::keybind <Control-Button-5> { zoom out ; refresh} $drawing
  }

  #-----------------------------------------------------------------
  # Evaluate registered callback procedures
  #-----------------------------------------------------------------

  catch {eval $XCOps(callback)}
  catch {eval $XCWinOps(${name}, callback)}
}

#----------------------------------------------------------------------
# Delete a window.
#----------------------------------------------------------------------

proc xcircuit::closewindow {name} {
   global XCOps

   set winlist [config windownames]
   if {[llength $winlist] > 1} {
      if {[lsearch $winlist $name] != -1} {
         config delete $name
	 set newwin [lindex [config windownames] 0]
	 destroy [winfo top $name]
	 config focus $newwin
	 set XCOps(focus) [winfo top $newwin]
      }
   } else {
      quit
   }
}

#----------------------------------------------------------------------
# Create a new window, and set it to the same page as the current one.
#----------------------------------------------------------------------

proc xcircuit::forkwindow {} {
   set suffix [llength [config windownames]]
   set newname .xcircuit${suffix}
   xcircuit::new_window $newname
   config init $newname
}

#----------------------------------------------------------------------
# Find the geometry position that centers a window on the cursor
# position
#----------------------------------------------------------------------

proc xcircuit::centerwin {wname} {
   set xmax [expr {[winfo screenwidth $wname] - 10}]
   set ymax [expr {[winfo screenheight $wname] - 35}] ;# allow for titlebar height
   set x [winfo pointerx $wname]
   set y [winfo pointery $wname]
   tkwait visibility $wname
   set w [winfo width $wname]
   set h [winfo height $wname]
   set x [expr $x - $w / 2]
   set y [expr $y - $h / 2]
   if {$x < 10} {set x 10}
   if {$y < 10} {set y 10}
   if {[expr {$x + $w}] > $xmax} {set x [expr {$xmax - $w}]}
   if {[expr {$y + $h}] > $ymax} {set y [expr {$ymax - $h}]}
   if {$x > 0} {set x "+$x"}
   if {$y > 0} {set y "+$y"}
   wm geometry $wname $x$y
}

#----------------------------------------------------------------------
# Message handling.  Assumes that windows are named
#  1) "pagename"	where the page name is printed
#  2) "coordinates"	where the coordinates are printed
#  3) "status"		the general-purpose status and message line.
#----------------------------------------------------------------------

proc xcircuit::print {wname string} {
   global XCOps
   set window $XCOps(focus)

   switch -glob ${wname} {
      page* {
	 ${window}.infobar.message1 configure -text ${string}
      }
      coord* {
	 ${window}.message configure -text ${string}
      }
      mes* -
      stat* {
	 ${window}.infobar.message2 configure -text ${string}
      }
   }
}

proc xcircuit::getinitstate {wname} {
   if [winfo exists $wname] {
      set wstate [wm state $wname]
   } else {
      set wstate "none"
   }
   return $wstate
}

#----------------------------------------------------------------------
# Support procedures for tag callbacks
#----------------------------------------------------------------------

proc xcircuit::popupdialog {{w .dialog}} {
   set wstate [xcircuit::getinitstate ${w}]
   xcircuit::removelists ${w}
   wm deiconify ${w}
   if {"$wstate" != "normal"} {centerwin ${w}}
   raise ${w}
   focus ${w}.textent.txt
}

proc xcircuit::popupfilelist {{w .filelist}} {
   set wstate [xcircuit::getinitstate ${w}]
   xcircuit::removelists ${w}
   wm deiconify ${w}
   centerwin ${w}
   raise ${w}
   focus ${w}.textent.txt
}

#----------------------------------------------------------------------
# A refined "page size" that keeps a lid on the numerical precision
# out of Tcl. . .
#----------------------------------------------------------------------

proc xcircuit::getpagesize {} {
   set slist [xcircuit::page size]
   set coordstyle [lindex $slist 3]
   if {$coordstyle == "cm"} {
      # Round centimeter coords to the nearest 0.1
      set xcm [lindex $slist 0]
      set xcm [expr {round($xcm * 10.0) / 10.0}]
      set slist [lreplace $slist 0 0 $xcm]
      set ycm [lindex $slist 2]
      set ycm [expr {round($ycm * 10.0) / 10.0}]
      set slist [lreplace $slist 2 2 $ycm]
   } elseif {$coordstyle == "in"} {
      # Round inch coords to the nearest 1/8
      set xin [lindex $slist 0]
      set xin [expr {round($xin * 8.0) / 8.0}]
      set slist [lreplace $slist 0 0 $xin]
      set yin [lindex $slist 2]
      set yin [expr {round($yin * 8.0) / 8.0}]
      set slist [lreplace $slist 2 2 $yin]
   }
   return $slist
}

#----------------------------------------------------------------------
# This procedure configures the sheet size according to the page
# dimensions (if they match, within reason)
#----------------------------------------------------------------------

proc xcircuit::setsheetsize {} {
   global XCOps

   set slist [xcircuit::getpagesize]
   set coordstyle [lindex $slist 3]
   if {$coordstyle == "cm"} {
      set xcm [lindex $slist 0]
      set ycm [lindex $slist 2]

      if {$xcm == 21.0 && $ycm == 29.7} {
	 set XCOps(sheetsize) a4
	 .output.textent.txtf.sizb configure -text "A4"
      } elseif {$xcm == 29.7 && $ycm == 42.0} {
	 set XCOps(sheetsize) a3
	 .output.textent.txtf.sizb configure -text "A3"
      } elseif {$xcm == 14.8 && $ycm == 18.4} {
	 set XCOps(sheetsize) a5
	 .output.textent.txtf.sizb configure -text "A5"
      } elseif {$xcm == 25.7 && $ycm == 36.4} {
	 set XCOps(sheetsize) b4
	 .output.textent.txtf.sizb configure -text "B4"
      } elseif {$xcm == 18.2 && $ycm == 25.7} {
	 set XCOps(sheetsize) b5
	 .output.textent.txtf.sizb configure -text "B5"
      } else {
	 set XCOps(sheetsize) special
	 .output.textent.txtf.sizb configure -text "Special"
      }
   } elseif {$coordstyle == "in"} {
      set xin [lindex $slist 0]
      set yin [lindex $slist 2]

      if {$xin == 8.5 && $yin == 11.0} {
	 set XCOps(sheetsize) letter
	 .output.textent.txtf.sizb configure -text Letter
      } elseif {$xin == 8.5 && $yin == 14.0} {
	 set XCOps(sheetsize) legal
	 .output.textent.txtf.sizb configure -text Legal
      } elseif {$xin == 5.5 && $yin == 8.5} {
	 set XCOps(sheetsize) statement
	 .output.textent.txtf.sizb configure -text Statement
      } elseif {$xin == 11.0 && $yin == 17.0} {
	 set XCOps(sheetsize) tabloid
	 .output.textent.txtf.sizb configure -text Tabloid
      } elseif {$xin == 17.0 && $yin == 11.0} {
	 set XCOps(sheetsize) ledger
	 .output.textent.txtf.sizb configure -text Ledger
      } elseif {$xin == 8.5 && $yin == 13.0} {
	 set XCOps(sheetsize) folio
	 .output.textent.txtf.sizb configure -text Folio
      } elseif {$xin == 10.0 && $yin == 14.0} {
	 set XCOps(sheetsize) tenfourteen
	 .output.textent.txtf.sizb configure -text 10x14
      } elseif {$xin == 7.5 && $yin == 10.0} {
	 set XCOps(sheetsize) executive
	 .output.textent.txtf.sizb configure -text Executive
      } elseif {$xin == 17.0 && $yin == 22.0} {
	 set XCOps(sheetsize) ansic
	 .output.textent.txtf.sizb configure -text "ANSI C"
      } elseif {$xin == 22.0 && $yin == 34.0} {
	 set XCOps(sheetsize) ansid
	 .output.textent.txtf.sizb configure -text "ANSI D"
      } elseif {$xin == 34.0 && $yin == 44.0} {
	 set XCOps(sheetsize) ansie
	 .output.textent.txtf.sizb configure -text "ANSI E"
      } else {
	 set XCOps(sheetsize) special
	 .output.textent.txtf.sizb configure -text "Special"
      }
   }
}

#----------------------------------------------------------------------
# This procedure configures the output properties window according to
# the page mode (full or encapsulated)
#----------------------------------------------------------------------

proc xcircuit::setpstype {mode} {
   global XCOps
   switch -- $mode {
      {eps} { .output.textent.butp configure -text "Embedded (EPS)"
	 grid remove .output.textent.but7
	 grid remove .output.textent.butf
	 grid remove .output.textent.txtf
      }
      {full} {.output.textent.butp configure -text "Full Page"
	 grid .output.textent.but7 -row 6 -column 3 -pady 5 -ipadx 10
	 grid .output.textent.butf -row 5 -column 2 -padx 10
	 grid .output.textent.txtf -row 6 -column 2 -sticky ew -padx 10
      }
   }
   set XCOps(pstype) $mode
   xcircuit::page encapsulation $mode
}

#----------------------------------------------------------------------

proc xcircuit::dolinks {} {
   global XCOps
   set ilinks [xcircuit::page links independent]
   if {$ilinks > 1} {
      set XCOps(imultiple) 1
   } else {
      set XCOps(imultiple) 0
   }
   if {$ilinks == 1} { set plural ""} else { set plural "s"}
   .output.title.imulti configure -text "$ilinks schematic$plural"

   if {$XCOps(dmultiple) == 1} {
      set dlinks [xcircuit::page links dependent]
   } else {
      set dlinks 0
   }
   if {$dlinks == 1} { set plural ""} else { set plural "s"}
   .output.title.dmulti configure -text "$dlinks subcircuit$plural"
}

#----------------------------------------------------------------------

proc xcircuit::setlinksmenu {} {
   set m .output.textent.butl.linksmenu
   $m delete 0 end
   $m add radio -label "None" -variable XCOps(links) -command \
	{.output.textent.butl configure -text None ; \
	xcircuit::page filename {}}
   if {![catch {set plist [xcircuit::page list]}]} {
      set fnames {}
      foreach p $plist {
         set pfile [xcircuit::page $p filename]
	 if {"$pfile" != ""} {
	    lappend fnames $pfile
	 }
      }
      foreach f [lsort -uniq $fnames] {
         $m add radio -label $f -variable XCOps(links) \
		-command ".output.textent.butl configure -text $f ; \
		xcircuit::page filename $f"
      }
   }
   .output.textent.butl configure -text "(change)"
}

#----------------------------------------------------------------------

proc xcircuit::pageupdate { {subcommand "none"} } {
   global XCOps
   if {[info level] <= 1} {
     switch -- $subcommand {
	save {
	   .output.bbar.okay configure -text "Done"
	   .output.bbar.okay configure -command {wm withdraw .output}
	}
	make {
	   xcircuit::newpagebutton [xcircuit::page label]
	}
	default {
	  .output.title.field configure -text \
		"PostScript output properties (Page [xcircuit::page])"
	  set fname [xcircuit::page filename]
	  .output.textent.but1 configure -text Apply
	  .output.textent.but2 configure -text Apply
	  .output.textent.but3 configure -text Apply
	  .output.textent.but4 configure -text Apply
	  .output.textent.but5 configure -text Apply
	  .output.textent.but7 configure -text Apply
	  .output.textent.txt1 delete 0 end
	  .output.textent.txt1 insert 0 $fname
	  .output.textent.txt2 delete 0 end
	  .output.textent.txt2 insert 0 [xcircuit::page label]
	  .output.textent.txt3 delete 0 end
	  set stext [format "%g" [xcircuit::page scale]]
	  .output.textent.txt3 insert 0 $stext
	  .output.textent.txt4 delete 0 end
	  set wtext [format "%g" [xcircuit::page width]]
	  .output.textent.txt4 insert 0 $wtext
	  .output.textent.txt4 insert end " "
	  .output.textent.txt4 insert end [xcircuit::coordstyle get]
	  .output.textent.txt5 delete 0 end
	  set htext [format "%g" [xcircuit::page height]]
	  .output.textent.txt5 insert 0 $htext
	  .output.textent.txt5 insert end " "
	  .output.textent.txt5 insert end [xcircuit::coordstyle get]
	  .output.textent.txtf.txtp delete 0 end
	  .output.textent.txtf.txtp insert 0 [xcircuit::getpagesize]
	  xcircuit::setpstype [xcircuit::page encapsulation]
	  set XCOps(orient) [xcircuit::page orientation]
	  if {$XCOps(orient) == 0} {
	     .output.textent.buto configure -text Portrait
	  } else {
	     .output.textent.buto configure -text Landscape
	  }
	  xcircuit::dolinks
	  xcircuit::setlinksmenu
	  xcircuit::setsheetsize

	  set XCOps(autofit) [xcircuit::page fit]
	  if {[string match *.* $fname] == 0} {append fname .ps}
	  if {[glob -nocomplain ${fname}] == {}} {
	    .output.bbar.okay configure -text "Write File"
	  } else {
	    .output.bbar.okay configure -text "Overwrite File"
	  }
	  .output.bbar.okay configure -command \
		{.output.textent.but1 invoke; \
		 .output.textent.but2 invoke; \
		 if {$XCOps(autofit)} {xcircuit::page fit true}; \
		 if {$XCOps(dmultiple) == 1} {xcircuit::page save} else { \
		 xcircuit::page saveonly }; wm withdraw .output}
        }
     }
  }
}

#----------------------------------------------------------------------
# Update the GUI based on the schematic class of the current page
# This is called internally from the xcircuit code and the function
# must be defined, even if it is a null proc.
#----------------------------------------------------------------------

proc xcircuit::setsymschem {} {
   global XCOps
   set window $XCOps(focus)

   if {[info level] <= 1} {
      set schemtype [xcircuit::schematic type]
      set symschem [xcircuit::schematic get]
      set m ${window}.menubar.netlistbutton.netlistmenu
      switch -- $schemtype {
         primary -
	 secondary -
	 schematic {
	    ${window}.infobar.schem configure -background red -foreground white
	    if {$symschem == {}} {
		${window}.infobar.symb configure -background gray70 \
			-foreground gray40
		$m entryconfigure 6 -label "Make Matching Symbol" \
			-command {xcircuit::promptmakesymbol [page label]}
		$m entryconfigure 7 -label "Associate With Symbol" \
			-command {xcircuit::symbol associate}
	    } else {
		${window}.infobar.symb configure -background white -foreground black
		$m entryconfigure 6 -label "Go To Symbol" \
			-command {xcircuit::symbol go}
		$m entryconfigure 7 -label "Disassociate Symbol" \
			-command {xcircuit::symbol disassociate}
	    }
	 }
	 symbol -
	 fundamental -
	 trivial {
	    ${window}.infobar.symb configure -foreground white
	    if {$symschem == {}} {
		${window}.infobar.schem configure -background gray70 -foreground \
			gray40
		$m entryconfigure 6 -label "Make Matching Schematic" \
			-command {xcircuit::schematic make}
		$m entryconfigure 7 -label "Associate With Schematic" \
			-command {xcircuit::schematic associate}
	    } else {
		${window}.infobar.schem configure -background white -foreground black
		$m entryconfigure 6 -label "Go To Schematic" \
			-command {xcircuit::schematic go}
		$m entryconfigure 7 -label "Disassociate Schematic" \
			-command {xcircuit::schematic disassociate}
	    }
	 }
      }
      switch -- $schemtype {
	 trivial {
	    ${window}.infobar.symb configure -background red
	 }
	 fundamental {
	    ${window}.infobar.symb configure -background green4 ;# bboxcolor
	 }
	 symbol {
	    ${window}.infobar.symb configure -background blue2 
	 }
      }
   }
}
   
#----------------------------------------------------------------------
# Set the coordinate style to inches from cm and vice versa.
# This routine avoids switching from fractional to decimal inches
# and vice versa if we are already in one of the two inches modes.
#
# with no argument, or argument "get", returns the "short" name
# ("cm" or "in") of the style.
#----------------------------------------------------------------------

proc xcircuit::coordstyle { { mode get } } {
   global XCOps XCWinOps
   set curstyle [xcircuit::config coordstyle]
   switch -- $mode {
      inches {
	 switch -- $curstyle {
	    centimeters {
	       xcircuit::config coordstyle "decimal inches"
	       xcircuit::pageupdate
	    }
	 }
      }
      centimeters -
      cm {
	 switch -- $curstyle {
	    centimeters {
	    }
	    default {
	       xcircuit::config coordstyle "centimeters"
	       xcircuit::pageupdate
	    }
	 }
      }
      get {
	 switch -- $curstyle {
	    centimeters {
	       return "cm"
	    }
	    default {
	       return "in"
	    }
	 }
      }
   }
}

#----------------------------------------------------------------------

proc xcircuit::raiseconsole {} {
   global XCOps
   set window $XCOps(focus)

   xcircuit::consoleup
   xcircuit::consoleontop
   set cidx [${window}.menubar.filebutton.filemenu index *Console]
   ${window}.menubar.filebutton.filemenu entryconfigure \
	 $cidx -label "No Console" -command {xcircuit::lowerconsole}
}

#----------------------------------------------------------------------

proc xcircuit::lowerconsole {} {
   global XCOps
   set window $XCOps(focus)

   xcircuit::consoledown
   set cidx [${window}.menubar.filebutton.filemenu index *Console]
   ${window}.menubar.filebutton.filemenu entryconfigure \
	 $cidx -label "Tcl Console" -command {xcircuit::raiseconsole}
}

#----------------------------------------------------------------------
# Command tags---these let the command-line entry functions update the
# Tk windows, so that the Tk window structure doesn't need to be hard-
# coded into the source.
#----------------------------------------------------------------------

xcircuit::tag page {xcircuit::pageupdate %1 ; xcircuit::updateparams}
xcircuit::tag promptsavepage {xcircuit::pageupdate ;
        set wstate [xcircuit::getinitstate .output] ; wm deiconify .output ;
	if {"$wstate" != "normal"} {xcircuit::centerwin .output} ; raise .output}
xcircuit::tag loadfont {xcircuit::newfontbutton %r}
xcircuit::tag color { if {"%1" == "set"} {
	set XCWinOps($XCOps(focus),colorval) %2; set iname img_co;
	if {"%2" != "inherit"} {append iname l%2} ;
	$XCOps(focus).mainframe.toolbar.bco configure -image $iname} }
xcircuit::tag border {if {%# == 2} {
	switch -- %1 {
	   dashed { set XCWinOps($XCOps(focus),linestyle) dashed}
	   dotted { set XCWinOps($XCOps(focus),linestyle) dotted}
	   unbordered { set XCWinOps($XCOps(focus),linestyle) unbordered}
	   solid { set XCWinOps($XCOps(focus),linestyle) solid}
	   square { set XCWinOps($XCOps(focus),endcaps) square}
	   round { set XCWinOps($XCOps(focus),endcaps) round}
	   closed { set XCWinOps($XCOps(focus),polyclosed) closed}
	   unclosed { set XCWinOps($XCOps(focus),polyclosed) unclosed}
	}} elseif {%# == 3} {
	switch -- %1 {
	   bbox { set XCWinOps($XCOps(focus),bboxtype) %2}
	   clipmask { set XCWinOps($XCOps(focus),clipmask) %2}
	}}}
xcircuit::tag fill { foreach i %N { switch -- "$i" {
	   opaque { set XCWinOps($XCOps(focus),opaque) true }
	   transparent { set XCWinOps($XCOps(focus),opaque) false } 0 - unfilled
		{set XCWinOps($XCOps(focus),fillamount) 0;
	      $XCOps(focus).mainframe.toolbar.bfi configure -image img_fi}
	   solid {set XCWinOps($XCOps(focus),fillamount) 100;
	      $XCOps(focus).mainframe.toolbar.bfi configure -image img_stip100}
	   default {set XCWinOps($XCOps(focus),fillamount) $i;
	      $XCOps(focus).mainframe.toolbar.bfi configure -image img_stip$i} } } }

xcircuit::tag select {if {%N > 1} {xcircuit::updateparams; xcircuit::updatedialog}}
xcircuit::tag unselect {xcircuit::updateparams; xcircuit::updatedialog}
xcircuit::tag schematic {xcircuit::setsymschem}
xcircuit::tag symbol {xcircuit::setsymschem}

xcircuit::tag parameter { if {"%1" == "make"} {set cond true} else {set cond false} 
    switch %1 {
	set -
	forget -
	delete {xcircuit::updateparams}
	make -
	replace {switch -- "%2" {
		"x position" {set XCWinOps($XCOps(focus),xposparam) $cond}
		"y position" {set XCWinOps($XCOps(focus),yposparam) $cond}
		style {set XCWinOps($XCOps(focus),styleparam) $cond}
		"start angle" {set XCWinOps($XCOps(focus),startparam) $cond}
		"end angle" {set XCWinOps($XCOps(focus),endparam) $cond}
		anchoring {set XCWinOps($XCOps(focus),anchorparam) $cond}
		radius {set XCWinOps($XCOps(focus),radiusparam) $cond}
		"minor axis" {set XCWinOps($XCOps(focus),minorparam) $cond}
		rotation {set XCWinOps($XCOps(focus),rotationparam) $cond}
		scale {set XCWinOps($XCOps(focus),scaleparam) $cond}
		linewidth {set XCWinOps($XCOps(focus),linewidthparam) $cond}
		color {set XCWinOps($XCOps(focus),colorparam) $cond}
		default {xcircuit::updateparams}
	}}
	default {if {%# == 4} {xcircuit::updateparams}}
    }}

xcircuit::tag config {if {%# == 3} {
	switch -- %1 {
	   colorscheme {set XCWinOps($XCOps(focus),colorscheme) [config colorscheme];
		refresh}
	   bbox {set XCWinOps($XCOps(focus),showbbox) [config bbox]}
	   editinplace {set XCWinOps($XCOps(focus),editinplace) [config editinplace]}
	   pinpositions {set XCWinOps($XCOps(focus),pinpositions) [config pinpositions]}
	   pinattach {set XCWinOps($XCOps(focus),pinattach) [config pinattach]}
	   technologies {set XCWinOps($XCOps(focus),namespaces) [config technologies]}
	   hold {set XCOps(hold) [config hold]}
	   grid {catch {set XCWinOps($XCOps(focus),showgrid) [config grid]}}
	   snap {catch {set XCWinOps($XCOps(focus),showsnap) [config snap]}}
	   axes {set XCWinOps($XCOps(focus),showaxes) [config axes]}
	   centering {set XCWinOps($XCOps(focus),centerobject) [config centering]}
	   manhattan {set XCWinOps($XCOps(focus),manhattandraw) [config manhattan]}
	   coordstyle {set XCWinOps($XCOps(focus),gridstyle) [config coordstyle]}
	   boxedit {set XCWinOps($XCOps(focus),polyedittype) [config boxedit]}
	   pathedit {set XCWinOps($XCOps(focus),pathedittype) [config pathedit]}
	   technologies {set XCWinOps($XCOps(focus),showtech) [config technologies]}
	}} elseif {(%# == 4) && ("%1" == "filter")} {
	   set XCWinOps($XCOps(focus),sel_%2) [config filter %2]
	}}

xcircuit::tag label {if {%# == 3} {
	switch -- %1 {
	   encoding {
	      set XCWinOps($XCOps(focus),fontencoding) %2
	      xcircuit::newencodingbutton %2
	   }
	   family {if {"%2" != "-all"} {set XCWinOps($XCOps(focus),fontfamily) %2}}
	   style {set XCWinOps($XCOps(focus),fontstyle) %2}
	   anchor {
	      switch -- %2 {
		 top -
		 bottom -
		 middle {set XCWinOps($XCOps(focus),jvert) %2}
		 default {set XCWinOps($XCOps(focus),jhoriz) %2}
	      }
	   }
	   justify {set XCWinOps($XCOps(focus),justif) %2}
	   flipinvariant {set XCWinOps($XCOps(focus),flipinvariant) %2}
	   visible {set XCWinOps($XCOps(focus),pinvisible) %2}
	   latex {set XCWinOps($XCOps(focus),latexmode) %2}
        }} elseif {(%# == 4) && ("%1" == "anchor")} {
	   switch -- %2 {
	      top -
	      bottom -
	      middle {set XCWinOps($XCOps(focus),jvert) %2 ;
	      	      set XCWinOps($XCOps(focus),jhoriz) %3}
	      default {set XCWinOps($XCOps(focus),jhoriz) %2 ;
		      set XCWinOps($XCOps(focus),jvert) %3}
	   }
	}}

#------------------------------
# Create the file-list window
#------------------------------

# First, set the variables associated with toggle and radio buttons
set XCOps(filter) 1

toplevel .filelist -bg beige
wm title .filelist "File List Window"
wm group .filelist .
wm protocol .filelist WM_DELETE_WINDOW {wm withdraw .filelist}
wm withdraw .filelist

frame .filelist.listwin
frame .filelist.textent -bg beige
frame .filelist.bbar	-bg beige

pack .filelist.listwin -side top -padx 20 -pady 7 -expand true -fill both
pack .filelist.textent -side top -padx 20 -pady 7 -fill x
pack .filelist.bbar -side bottom -padx 20 -pady 7 -fill x

simple .filelist.listwin.win -bg white
simple .filelist.listwin.sb -width 13 -bg beige

grid .filelist.listwin.win -row 0 -column 0 -sticky news -padx 1 -pady 1
grid .filelist.listwin.sb -row 0 -column 1 -sticky ns -padx 1 -pady 1

grid columnconfigure .filelist.listwin 0 -weight 1 -minsize 100
grid rowconfigure .filelist.listwin 0 -weight 1 -minsize 100

frame .filelist.textent.title -bg beige
pack .filelist.textent.title -side top -fill x

label .filelist.textent.title.field -text "Select file to load:" -bg beige
label .filelist.textent.title.chklabel -text "Filter" -bg beige
checkbutton .filelist.textent.title.filter -bg beige -variable XCOps(filter) \
   -command {event generate .filelist.listwin.win <ButtonPress> -button 3 ; \
	event generate .filelist.listwin.win <ButtonRelease> -button 3}

entry .filelist.textent.txt -bg white -relief sunken -width 50

pack .filelist.textent.title.filter -side right
pack .filelist.textent.title.chklabel -side right
pack .filelist.textent.title.field -side left
pack .filelist.textent.txt -side bottom -fill x -expand true

button .filelist.bbar.okay -text Okay -bg beige
button .filelist.bbar.cancel -text Cancel -bg beige -command {wm withdraw .filelist}

pack .filelist.bbar.okay -side left -ipadx 10
pack .filelist.bbar.cancel -side right -ipadx 10

# Allow <return> to update or accept entry
bind .filelist.textent.txt <Return> \
	{event generate .filelist.listwin.win <ButtonPress> -button 2 ; \
	event generate .filelist.listwin.win <ButtonRelease> -button 2}

#--------------------------------------
# Create the output generating window
#--------------------------------------

# First, set the variables associated with toggle and radio buttons
set XCOps(autofit) 0
set XCOps(imultiple) 0
set XCOps(dmultiple) 0	;# don't save subcircuits with different filenames
if {[catch {set XCOps(technology)}]} {set XCOps(technology) "(user)"}
if {[catch {set XCOps(library)}]} {set XCOps(library) "User Library"}

toplevel .output -bg beige
wm title .output "PostScript Output Properties"
wm group .output .
wm protocol .output WM_DELETE_WINDOW {wm withdraw .output}
wm withdraw .output

frame .output.title -bg beige
frame .output.textent -bg beige
frame .output.bbar -bg beige

pack .output.title -side top -padx 20 -pady 7 -fill x
pack .output.textent -side top -padx 20 -pady 7 -fill x
pack .output.bbar -side bottom -padx 20 -pady 7 -fill x

label .output.title.field -text "PostScript output properties (Page 1):" -bg tan
checkbutton .output.title.imulti -text "1 schematic" -bg beige \
	-variable XCOps(imultiple) \
	-command {xcircuit::dolinks ; \
	if {$XCOps(imultiple) == 1} {.output.textent.txt1 \
	delete 0 end; .output.textent.but1 configure -text Apply; xcircuit::page \
	filename {}; focus .output.textent.txt1 ; xcircuit::dolinks }}
checkbutton .output.title.dmulti -text "0 subcircuits" -bg beige \
	-variable XCOps(dmultiple) \
	-command {xcircuit::dolinks ; .output.textent.but1 configure -text Apply; \
	if {$XCOps(dmultiple) == 1} {xcircuit::page filename {}; \
	.output.textent.txt1 delete 0 end; focus .output.textent.txt1 }; \
	xcircuit::dolinks }

pack .output.title.dmulti -side right -padx 5
pack .output.title.imulti -side right -padx 5

pack .output.title.field -side left

label .output.textent.lab1 -text "Filename:" -bg beige
label .output.textent.lab2 -text "Page label:" -bg beige
label .output.textent.lab3 -text "Scale:" -bg beige
label .output.textent.lab4 -text "Width:" -bg beige
label .output.textent.lab5 -text "Height:" -bg beige
label .output.textent.lab6 -text "Orientation:" -bg beige
label .output.textent.lab7 -text "Mode:" -bg beige
label .output.textent.lab8 -text "Link to:" -bg beige

entry .output.textent.txt1 -bg white -relief sunken -width 20
entry .output.textent.txt2 -bg white -relief sunken -width 20
entry .output.textent.txt3 -bg white -relief sunken -width 20
entry .output.textent.txt4 -bg white -relief sunken -width 20
entry .output.textent.txt5 -bg white -relief sunken -width 20

menubutton .output.textent.buto -text Portrait -bg beige \
	-menu .output.textent.buto.orientmenu
menubutton .output.textent.butp -text "Embedded (EPS)" -bg beige \
	-menu .output.textent.butp.psmenu
menubutton .output.textent.butl -text "(change)" -bg beige \
	-menu .output.textent.butl.linksmenu

checkbutton .output.textent.butf -text "Auto-fit" -bg beige \
	-variable XCOps(autofit) -onvalue true -offvalue false \
	-command {xcircuit::page fit $XCOps(autofit)}
frame .output.textent.txtf -bg beige
menubutton .output.textent.txtf.sizb -text "Sizes" -bg beige \
	-menu .output.textent.txtf.sizb.sizemenu
entry .output.textent.txtf.txtp -bg white -relief sunken -width 14

pack .output.textent.txtf.txtp -side left -fill y
pack .output.textent.txtf.sizb -side left

button .output.textent.but1 -text Apply -bg beige \
	-command {xcircuit::page filename [.output.textent.txt1 get]
	if {[llength [xcircuit::page label]] > 1} {
	   xcircuit::page label [file root [.output.textent.txt1 get]]};\
	.output.textent.but1 configure -text Okay}
button .output.textent.but2 -text Apply -bg beige \
	-command {xcircuit::page label [.output.textent.txt2 get];\
	.output.textent.but2 configure -text Okay}
button .output.textent.but3 -text Apply -bg beige \
	-command {xcircuit::page scale [.output.textent.txt3 get];\
	.output.textent.but3 configure -text Okay}
button .output.textent.but4 -text Apply -bg beige \
	-command {xcircuit::page width [.output.textent.txt4 get];\
	.output.textent.but4 configure -text Okay}
button .output.textent.but5 -text Apply -bg beige \
	-command {xcircuit::page height [.output.textent.txt5 get];\
	.output.textent.but5 configure -text Okay}
button .output.textent.but7 -text Apply -bg beige \
	-command {xcircuit::page size [.output.textent.txtf.txtp get];\
	.output.textent.but7 configure -text Okay}

bind .output.textent.txt1 <Return> {.output.textent.but1 invoke}
bind .output.textent.txt2 <Return> {.output.textent.but2 invoke}
bind .output.textent.txt3 <Return> {.output.textent.but3 invoke}
bind .output.textent.txt4 <Return> {.output.textent.but4 invoke}
bind .output.textent.txt5 <Return> {.output.textent.but5 invoke}

grid .output.textent.lab1 -row 0 -column 0 -sticky w
grid .output.textent.lab2 -row 1 -column 0 -sticky w
grid .output.textent.lab3 -row 2 -column 0 -sticky w
grid .output.textent.lab4 -row 3 -column 0 -sticky w
grid .output.textent.lab5 -row 4 -column 0 -sticky w
grid .output.textent.lab6 -row 5 -column 0 -sticky w
grid .output.textent.lab7 -row 6 -column 0 -sticky w
grid .output.textent.lab8 -row 7 -column 0 -sticky w

grid .output.textent.txt1 -row 0 -column 1 -columnspan 2 -sticky ew -padx 10
grid .output.textent.txt2 -row 1 -column 1 -columnspan 2 -sticky ew -padx 10
grid .output.textent.txt3 -row 2 -column 1 -columnspan 2 -sticky ew -padx 10
grid .output.textent.txt4 -row 3 -column 1 -columnspan 2 -sticky ew -padx 10
grid .output.textent.txt5 -row 4 -column 1 -columnspan 2 -sticky ew -padx 10
grid .output.textent.buto -row 5 -column 1 -sticky w -padx 10
grid .output.textent.butp -row 6 -column 1 -sticky w -padx 10
grid .output.textent.butl -row 7 -column 1 -sticky w -padx 10

grid .output.textent.but1 -row 0 -column 3 -pady 5 -ipadx 10
grid .output.textent.but2 -row 1 -column 3 -pady 5 -ipadx 10
grid .output.textent.but3 -row 2 -column 3 -pady 5 -ipadx 10
grid .output.textent.but4 -row 3 -column 3 -pady 5 -ipadx 10
grid .output.textent.but5 -row 4 -column 3 -pady 5 -ipadx 10

grid columnconfigure .output.textent 2 -weight 1

button .output.bbar.okay -text Okay -bg beige -command {xcircuit::page save; \
  wm withdraw .output}
button .output.bbar.cancel -text Cancel -bg beige -command {wm withdraw .output}

# Setup simple choice menus for page type and orientation
# First, set the variables associated with the radio buttons. . . 
set XCOps(orient) 0
set XCOps(pstype) eps

set m [menu .output.textent.buto.orientmenu -tearoff 0]
$m add radio -label "Portrait" -variable XCOps(orient) -value 0 -command \
	{.output.textent.buto configure -text Portrait ; \
	xcircuit::page orientation 0}
$m add radio -label "Landscape" -variable XCOps(orient) -value 90 -command \
	{.output.textent.buto configure -text Landscape ; \
	xcircuit::page orientation 90}

set m [menu .output.textent.butp.psmenu -tearoff 0]
$m add radio -label "Embedded (EPS)" -variable XCOps(pstype) -value eps -command \
	{xcircuit::setpstype eps}
$m add radio -label "Full Page" -variable XCOps(pstype) -value full -command \
	{xcircuit::setpstype full}

menu .output.textent.butl.linksmenu -tearoff 0
xcircuit::setlinksmenu

pack .output.bbar.okay -side left -ipadx 10
pack .output.bbar.cancel -side right -ipadx 10

set m [menu .output.textent.txtf.sizb.sizemenu -tearoff 0]
$m add radio -label "Letter (ANSI A)" -variable XCOps(sheetsize) \
	-value letter -command \
	{ xcircuit::coordstyle inches; xcircuit::page size "8.5 x 11.0 in"}
$m add radio -label "Legal" -variable XCOps(sheetsize) -value legal -command \
	{ xcircuit::coordstyle inches; xcircuit::page size "8.5 x 14.0 in"}
$m add radio -label "Statement" -variable XCOps(sheetsize) -value statement \
	-command \
	{ xcircuit::coordstyle inches; xcircuit::page size "5.5 x 8.5 in"}
$m add radio -label "Tabloid (ANSI B)" -variable XCOps(sheetsize) \
	 -value tabloid -command \
	{ xcircuit::coordstyle inches; xcircuit::page size "11.0 x 17.0 in"}
$m add radio -label "Ledger" -variable XCOps(sheetsize) -value ledger -command \
	{ xcircuit::coordstyle inches; xcircuit::page size "17.0 x 11.0 in"}
$m add radio -label "Folio" -variable XCOps(sheetsize) -value folio -command \
	{ xcircuit::coordstyle inches; xcircuit::page size "8.5 x 13.0 in"}
$m add radio -label "Quarto" -variable XCOps(sheetsize) -value quarto -command \
	{ xcircuit::coordstyle inches; xcircuit::page size "8.472 x 10.833 in"}
$m add radio -label "10x14" -variable XCOps(sheetsize) -value tenfourteen -command \
	{ xcircuit::coordstyle inches; xcircuit::page size "10.0 x 14.0 in"}
$m add radio -label "Executive" -variable XCOps(sheetsize) -value executive -command \
	{ xcircuit::coordstyle inches; xcircuit::page size "7.5 x 10.0 in"}
$m add radio -label "ANSI C" -variable XCOps(sheetsize) -value ansic -command \
	{ xcircuit::coordstyle inches; xcircuit::page size "17.0 x 22.0 in"}
$m add radio -label "ANSI D" -variable XCOps(sheetsize) -value ansid -command \
	{ xcircuit::coordstyle inches; xcircuit::page size "22.0 x 34.0 in"}
$m add radio -label "ANSI E" -variable XCOps(sheetsize) -value ansie -command \
	{ xcircuit::coordstyle inches; xcircuit::page size "34.0 x 44.0 in"}
$m add radio -label "A3" -variable XCOps(sheetsize) -value a3 -command \
	{ xcircuit::coordstyle centimeters; xcircuit::page size "29.7 x 42.0 cm"}
$m add radio -label "A4" -variable XCOps(sheetsize) -value a4 -command \
	{ xcircuit::coordstyle centimeters; xcircuit::page size "21.0 x 29.7 cm"}
$m add radio -label "A5" -variable XCOps(sheetsize) -value a5 -command \
	{ xcircuit::coordstyle centimeters; xcircuit::page size "14.82 x 18.43 cm"}
$m add radio -label "B4" -variable XCOps(sheetsize) -value b4 -command \
	{ xcircuit::coordstyle centimeters; xcircuit::page size "25.7 x 36.4 cm"}
$m add radio -label "B5" -variable XCOps(sheetsize) -value b5 -command \
	{ xcircuit::coordstyle centimeters; xcircuit::page size "18.2 x 25.7 cm"}
$m add radio -label "Special" -variable XCOps(sheetsize) -value special}

#-----------------------------------------------------------------
# Clear the selection listbox.  Create it if it does not exist.
#-----------------------------------------------------------------

proc xcircuit::make_parameter_listbox {} {
   if {[catch {wm state .parameter}]} {
      toplevel .parameter -bg beige
      wm group .parameter .
      wm withdraw .parameter

      label .parameter.title -text "Parameters" -bg beige
      label .parameter.keytitle -text "Key" -bg beige
      label .parameter.valtitle -text "Value" -bg beige

      listbox .parameter.keylist -bg white
      listbox .parameter.vallist -bg white
      listbox .parameter.parvals -bg white

      # Code to get the listboxes to scroll in synchrony
      bind .parameter.keylist <Button-4> {xcircuit::paramscroll -1}
      bind .parameter.keylist <Button-5> {xcircuit::paramscroll 1}
      bind .parameter.vallist <Button-4> {xcircuit::paramscroll -1}
      bind .parameter.vallist <Button-5> {xcircuit::paramscroll 1}
      bind .parameter.parvals <Button-4> {xcircuit::paramscroll -1}
      bind .parameter.parvals <Button-5> {xcircuit::paramscroll 1}
      # Also bind to the mouse wheel (Windows-specific, generally)
      bind .parameter.keylist <MouseWheel> {xcircuit::paramscroll %D}
      bind .parameter.vallist <MouseWheel> {xcircuit::paramscroll %D}
      bind .parameter.parvals <MouseWheel> {xcircuit::paramscroll %D}

      button .parameter.dismiss -text "Dismiss" -bg beige \
		-command {wm withdraw .parameter}

      menubutton .parameter.delete -text "Delete..." -bg beige \
		-menu .parameter.delete.deleteparam
      menu .parameter.delete.deleteparam -tearoff 0

      menubutton .parameter.create -text "New..." -bg beige \
		-menu .parameter.create.newparam
      menu .parameter.create.newparam -tearoff 0
      .parameter.create.newparam add command -label "Substring" -command \
		"xcircuit::promptmakeparam substring"
      .parameter.create.newparam add command -label "Numeric" -command \
		"xcircuit::promptmakeparam numeric"
      .parameter.create.newparam add command -label "Expression" -command \
		"xcircuit::promptmakeparam expression"

      labelframe .parameter.valedit -text "Edit value" -bg beige
      entry .parameter.valedit.entry -textvariable new_paramval -bg white
      button .parameter.valedit.apply -text "Apply" -bg beige

      pack .parameter.valedit.entry -side left -fill x -expand true -padx 2
      pack .parameter.valedit.apply -side top

      grid .parameter.title -row 0 -column 0 -columnspan 2 -sticky news
      grid .parameter.keytitle -row 1 -column 0 -sticky news
      grid .parameter.keylist -row 2 -column 0 -sticky news
      grid .parameter.valtitle -row 1 -column 1 -sticky news
      grid .parameter.vallist -row 2 -column 1 -sticky news
      grid .parameter.parvals -row 2 -column 1 -sticky news

      grid .parameter.valedit -row 3 -column 0 -columnspan 2 -padx 2 \
		-pady 2 -sticky ew

      grid .parameter.create -row 4 -column 1 -sticky ns
      grid .parameter.delete -row 4 -column 0 -sticky ns
      grid .parameter.dismiss -row 5 -column 0 -columnspan 2 -sticky ns

      grid rowconfigure .parameter 2 -weight 1
      grid columnconfigure .parameter 0 -weight 1
      grid columnconfigure .parameter 1 -weight 2

      raise .parameter.vallist

      bind .parameter <Escape> {wm withdraw .parameter}
      bind .parameter.valedit.entry <Return> {.parameter.valedit.apply invoke}
   }
}

#-----------------------------------------------------------------
# Scroll all listboxes in the .parameter window at the same
# time, in reponse to any one of them receiving a scroll event.
#-----------------------------------------------------------------

proc xcircuit::paramscroll {value} {
   global tcl_platform
   set idx [.parameter.keylist nearest 0]

   if {$tcl_platform(platform) == "windows"} {
      set idx [expr {$idx + $value / 120}]
   } else {
      set idx [expr {$idx + $value}]
   }

   .parameter.keylist yview $idx
   .parameter.vallist yview $idx
   .parameter.parvals yview $idx

   # Important!  This prohibits the default binding actions.
   return -code break
}

# Update the dialog box, if it has been left visible
# (Corrected 2/4/12:  Don't delete contents except in these specific cases!)

proc xcircuit::updatedialog {{w dialog}} {
   global XCOps
   if {[xcircuit::getinitstate .${w}] == "normal"} {
      switch -- $XCOps(${w}) {
 	 linewidth {
	    set btext [format "%g" [lindex [xcircuit::border get] 0]]
            .${w}.textent.txt delete 0 end
	    .${w}.textent.txt insert 0 $btext
	 }
	 textscale {
	    set cscale [xcircuit::label scale]
            .${w}.textent.txt delete 0 end
	    .${w}.textent.txt insert 0 $cscale
	 }
	 elementscale {
            set selects [xcircuit::select]
	    if {$selects > 0} {
	       set cscale [xcircuit::element scale]
               .${w}.textent.txt delete 0 end
	       .${w}.textent.txt insert 0 $cscale
	    }
	 }
      }
   }
}

proc xcircuit::makedialogline {dframe textline {w dialog}} {
   if {[catch {frame .${w}.${dframe} -bg beige}]} {
      .${w}.${dframe}.title.field configure -text ${textline}
   } else {
      pack .${w}.${dframe} -side top -padx 20 -pady 7 -fill x

      frame .${w}.${dframe}.title -bg beige
      entry .${w}.${dframe}.txt -bg white -relief sunken -width 50

      pack .${w}.${dframe}.title -side top -fill x
      pack .${w}.${dframe}.txt -side bottom -fill x -expand true

      label .${w}.${dframe}.title.field -text ${textline} -bg beige
      pack .${w}.${dframe}.title.field -side left
   }
}

proc xcircuit::removedialogline {dframe {w dialog}} {
   global XCOps
   pack forget .${w}.${dframe}
   destroy .${w}.${dframe}
   set XCOps(${w}) 0
}

#--------------------------------------------
# Create a simple popup prompt window
# With "Apply", "Okay", and "Cancel" buttons
#--------------------------------------------

proc make_simple_dialog {name} {
    set window .${name}
    toplevel ${window} -bg beige
    wm title ${window} "Dialog Box"
    wm group ${window} .
    wm protocol ${window} WM_DELETE_WINDOW [subst {wm withdraw ${window}}]
    wm withdraw ${window}
    set XCOps(${name}) 0

    xcircuit::makedialogline textent "Select file to load:" ${name}

    frame ${window}.bbar	-bg beige
    pack ${window}.bbar -side bottom -padx 20 -pady 7 -fill x

    button ${window}.bbar.okay -text Okay -bg beige \
	-command [subst {${window}.bbar.apply invoke ;\
	wm withdraw ${window}}]
    button ${window}.bbar.apply -text Apply -bg beige
    button ${window}.bbar.cancel -text Cancel -bg beige -command \
		[subst {wm withdraw ${window}}]

    bind ${window}.textent.txt <Return> [subst {${window}.bbar.apply invoke}]

    pack ${window}.bbar.okay -side left -ipadx 10
    pack ${window}.bbar.apply -side left -ipadx 10
    pack ${window}.bbar.cancel -side right -ipadx 10
}

#--------------------------------------
# Create a query prompt window with
# "Okay" and "Cancel" buttons, and a
# "Select:" title message
#--------------------------------------

proc make_query_dialog {name} {
    set window .${name}
    toplevel ${window} -bg beige
    wm title ${window} "Query Dialog Box"
    wm group ${window} .
    wm protocol ${window} WM_DELETE_WINDOW [subst {wm withdraw ${window}}]
    wm withdraw ${window}

    frame ${window}.title -bg beige
    frame ${window}.bbar	-bg beige

    pack ${window}.title -side top -padx 20 -pady 7 -fill x
    pack ${window}.bbar -side bottom -padx 20 -pady 7 -fill x

    label ${window}.title.field -text "Select:" -bg beige
    pack ${window}.title.field -side left

    button ${window}.bbar.okay -text Okay -bg beige
    button ${window}.bbar.cancel -text Cancel -bg beige -command \
	[subst {wm withdraw ${window}}]

    pack ${window}.bbar.okay -side left -ipadx 10
    pack ${window}.bbar.cancel -side right -ipadx 10
}

make_query_dialog query
make_simple_dialog dialog
make_simple_dialog savetech
make_simple_dialog makesymbol

#--------------------------------------------------------
# Generate all of the menu cascades
# Most commands reference XCircuit internal routines
#--------------------------------------------------------

# Supporting procedures

proc xcircuit::printstring {stringlist} {
   set p ""
   foreach i $stringlist {
      switch -- [lindex $i 0] {
	 Text {append p [lindex $i 1]}
	 Half -
	 Quarter {append p " "} 
      }
   }
   return $p
}

proc xcircuit::printanchor {anchor} {
   switch [expr {$anchor & 3}] {
      0 {set p "left"}
      1 {set p "center"}
      3 {set p "right"}
   }
   switch [expr {$anchor & 12}] {
      0 {append p " bottom"}
      4 {append p " middle"}
      12 {append p " top"}
   }
   return $p
}

proc xcircuit::labelmakeparam {} {
   global XCOps
   if {[xcircuit::select] > 0} {	;# this should be true. . .
      set XCOps(dialog) paramname
      xcircuit::removedialogline textent2 dialog	;# default is the selected text
      .dialog.bbar.apply configure -command \
	 [subst {xcircuit::parameter make substring \[.dialog.textent.txt get\];\
	 xcircuit::updateparams substring}]
      .dialog.textent.title.field configure -text "Parameter name:"
      .dialog.textent.txt delete 0 end
      xcircuit::popupdialog
   }
}

proc xcircuit::promptmakeparam {{mode substring}} {
   global XCOps

   set XCOps(dialog) paramdefault
   if {$mode == "label"} {set mode substring}
   xcircuit::makedialogline textent2 "Default value:" dialog
   .dialog.bbar.apply configure -command \
	 [subst {xcircuit::parameter make $mode \
	 \[.dialog.textent.txt get\] \[.dialog.textent2.txt get\] -forward; \
	 xcircuit::removedialogline textent2 dialog; \
	 xcircuit::updateparams $mode}]
   .dialog.textent.title.field configure -text \
		"Parameter name:"
   .dialog.textent.txt delete 0 end
   xcircuit::popupdialog
}

#----------------------------------------------------------------------
# This procedure generates a new index number for list selection
# inside a parameter, using "regsub" and "subst" to replace the
# old index with the new one.  This procedure depends on the
# existance of the listbox widget ".paramlist.plist".
#----------------------------------------------------------------------

proc xcircuit::renewparam {key y args} {
   set newidx [.parameter.parvals nearest $y]
   set current [join [xcircuit::parameter get $key ${args} -verbatim]]
   regsub {(.*lindex +{.*} +)([0-9]+)(.*)} $current {\1$newidx\3} tmpkey
   set newkey [subst -nocommands -nobackslashes "$tmpkey"]
   xcircuit::parameter set $key $newkey $args
}

#----------------------------------------------------------------------
# Prompt for a new value of a parameter.  Do some sophisticated checking
# for parameters that declare a list of possible options, and handle
# that situation separately.
#----------------------------------------------------------------------

proc xcircuit::changeparamvalue {key current args} {

  .parameter.valedit.entry delete 0 end
  if {[xcircuit::parameter type $key -forward] == "expression"} {

     # Use regexp processing to find if there is some part of the expression
     # that chooses a single fixed value from a list.  If so, generate a
     # listbox to present the choices in the list.

     set loccurnt [join [xcircuit::parameter get $key ${args} -verbatim]]
     if {[regexp {.*lindex +{(.*)} +[0-9]+.*} $loccurnt qall sellist] > 0} {
        .parameter.parvals delete 0 end
        raise .parameter.parvals

	foreach item $sellist {
	   .parameter.parvals insert end $item
	}

	#Abort the parameter value selection
	bind .parameter.parvals <ButtonRelease-3> {raise .parameter.vallist}

	bind .parameter.parvals <ButtonRelease-1> [subst {xcircuit::renewparam \
	  $key %y $args ;\
	  .parameter.keylist configure -state normal ;\
	  xcircuit::updateparams ;\
	  raise .parameter.vallist}]

	.parameter.valedit.entry delete 0 end
	.parameter.valedit.entry insert 0 $loccurnt

     } else {
	# If the parameter is an expression but not a choice-list type, then
	# we had better print the verbatim entry, or people will just get
	# confused when the value becomes "invalid result:..."
        .parameter.valedit.entry insert 0 $loccurnt
     }
  } else {
     .parameter.valedit.entry insert 0 $current
  }
  focus .parameter.valedit.entry
  .parameter.valedit.apply configure \
	-command [subst {xcircuit::parameter set \
	$key \[.parameter.valedit.entry get\] \
	${args}; xcircuit::updateparams}]
}

#----------------------------------------------------------------------

proc xcircuit::updateparams { {mode {substring numeric expression}} } {

   if {[catch {wm state .parameter}]} {return}

   # Avoid infinite recursion if a parameter invokes updateparams
   # (e.g., any "page" command in a parameter will do this!)

   if {$mode != "force"} {
      if {[info level] > 1} {return}
   } else {
      set mode {substring numeric expression}
   }

   while {[.parameter.keylist size] > 0} {.parameter.keylist delete 0}
   while {[.parameter.vallist size] > 0} {.parameter.vallist delete 0}

   .parameter.delete.deleteparam delete 0 last

   if {$mode == "none"} {
      set dlist [xcircuit::parameter get -forward]
   } else {
      set dlist {}
      foreach i $mode {
         set dlist [concat $dlist [parameter get $i -forward]]
      }
   }

   # Ensure that the parvals list is not present
   if {[select] == 0} { lower .parameter.parvals }

   # All selections will be lost, so make sure that the value field is clear
   .parameter.valedit.entry delete 0 end
   .parameter.valedit.apply configure -command {}

   bind .parameter.vallist <ButtonRelease-1> {
		set kidx [.parameter.keylist nearest %y]; \
		xcircuit::changeparamvalue \
		[.parameter.keylist get $kidx] [.parameter.vallist get $kidx] \
		-forward}

   ;#The insertion of parameters should only be applicable in "text" mode
   bind .parameter.keylist <ButtonRelease-1> {
		set kidx [.parameter.keylist nearest %y]; \
		if {[string last "text" [xcircuit::eventmode]] >= 0} {
		   label insert parameter [.parameter.keylist get $kidx]
		}
	    }

   if {[catch {set oname [xcircuit::object name]}]} {
      .parameter.title configure -text "Parameters"
   } else {
      .parameter.title configure -text "Parameters of $oname"
   }
   foreach i $dlist {
      set p_name [lindex $i 0]
      set p_val [lindex $i 1]
      .parameter.delete.deleteparam add command -label $p_name -command \
	 "xcircuit::parameter delete $p_name -forward"
      .parameter.keylist insert end $p_name
      switch -- [xcircuit::parameter type $p_name -forward] {
	 "substring" {
	    .parameter.vallist insert end [xcircuit::printstring $p_val]
	 }
	 "anchoring" {
	    .parameter.vallist insert end [xcircuit::printanchor $p_val]
	 }
	 default {
	    .parameter.vallist insert end $p_val
         }
      }
   }
}

#----------------------------------------------------------------------

proc xcircuit::prompteditparams {} {
   set wstate [xcircuit::getinitstate .parameter]
   xcircuit::make_parameter_listbox
   xcircuit::updateparams force
   if {"$wstate" != "normal"} {
      wm deiconify .parameter
      xcircuit::centerwin .parameter
   }
   raise .parameter
}

#----------------------------------------------------------------------

proc xcircuit::promptmakesymbol {{name ""}} {
  global XCOps
  
  set XCOps(dialog) makeobject
  .makesymbol.bbar.apply configure -command \
	  {if {[string first "Page " [page label]] >= 0} { \
	  page label [.makesymbol.textent.txt get]}; \
	  xcircuit::symbol make [.makesymbol.textent.txt get] $XCOps(library)}
  xcircuit::removedialogline textent2 makesymbol
  .makesymbol.textent.title.field configure -text "Name for new object:"
  .makesymbol.textent.txt delete 0 end
  .makesymbol.textent.txt insert 0 $name
  xcircuit::popupdialog .makesymbol
  xcircuit::addliblist .makesymbol Place in: "
}

#----------------------------------------------------------------------

proc xcircuit::prompttargettech {{name ""}} {
  global XCOps
  
  set XCOps(dialog) targettech
  .savetech.bbar.apply configure -command { \
	  set selects [xcircuit::select]; \
	  if {$selects > 0} { \
	    if {[catch {set techname [.savetech.textent2.txt get]}]} {\
		set techname $XCOps(technology)}; \
	    technology objects $techname [.savetech.textent.txt get]}\
	  } 
  xcircuit::removedialogline textent2 savetech
  .savetech.textent.title.field configure -text "Objects to move:"
  .savetech.textent.txt delete 0 end
  .savetech.textent.txt insert 0 $name
  xcircuit::popupdialog
  xcircuit::addtechlist .savetech "Target technology: "

  # Add an additional selection to the tech menu for adding a new
  # technology namespace.  This is relevant only to "prompttargettech".

  .savetech.techself.techselect.menu add \
	command -label "Add New Tech" -command \
	"xcircuit::makedialogline textent2 {New tech name:}" savetech
}

#----------------------------------------------------------------------

proc xcircuit::promptelementsize {} {
   global XCOps
   if {![catch {set cscale [xcircuit::element scale]}]} {
      set XCOps(dialog) elementscale
      .dialog.bbar.apply configure -command \
	  {xcircuit::element scale [.dialog.textent.txt get]}
      .dialog.textent.title.field configure -text "Element scale:"
      .dialog.textent.txt delete 0 end
      .dialog.textent.txt insert 0 $cscale
      xcircuit::popupdialog
   }
}

#----------------------------------------------------------------------

proc xcircuit::promptborderwidth {} {
   global XCOps
   .dialog.textent.txt delete 0 end
   set XCOps(dialog) linewidth
   set elist [xcircuit::select get]
   if {[llength $elist] == 0} {
      .dialog.bbar.apply configure -command \
	   [subst {config focus [config focus] ;\
	    xcircuit::border set \[.dialog.textent.txt get\]}]
      .dialog.textent.title.field configure -text "Default linewidth scale:"
      set btext [format "%g" [xcircuit::border get]]
      .dialog.textent.txt insert 0 $btext
   } else {
      .dialog.bbar.apply configure -command \
	   [subst {config focus [config focus] ;\
	   xcircuit::border set \[.dialog.textent.txt get \]}]
      .dialog.textent.title.field configure -text "Element linewidth:"
      set btext [format "%g" [lindex [xcircuit::border get] 0]]
      .dialog.textent.txt insert 0 $btext
   }
   xcircuit::popupdialog
}

#----------------------------------------------------------------------

proc xcircuit::promptlinewidth {} {
   global XCOps
   set XCOps(dialog) linescale
   .dialog.bbar.apply configure -command \
	[subst {config focus [config focus] ;\
	xcircuit::config linewidth \[.dialog.textent.txt get \]}]
   .dialog.textent.title.field configure -text "Page linewidth scaling:"
   .dialog.textent.txt delete 0 end
   set ltext [format "%g" [xcircuit::config linewidth]]
   .dialog.textent.txt insert 0 $ltext
   xcircuit::popupdialog
}

#----------------------------------------------------------------------

proc xcircuit::promptdrawingscale {} {
   global XCOps
   set XCOps(dialog) drawingscale
   .dialog.bbar.apply configure -command \
	{xcircuit::config drawingscale [.dialog.textent.txt get]}
   .dialog.textent.title.field configure -text "Drawing scale:"
   .dialog.textent.txt delete 0 end
   .dialog.textent.txt insert 0 [xcircuit::config drawingscale]
   xcircuit::popupdialog
}

#----------------------------------------------------------------------

proc xcircuit::promptgridspace {} {
   .dialog.bbar.apply configure -command \
	{xcircuit::config grid spacing [.dialog.textent.txt get]}
   .dialog.textent.title.field configure -text "Grid spacing:"
   .dialog.textent.txt delete 0 end
   .dialog.textent.txt insert 0 [xcircuit::config grid space]
   xcircuit::popupdialog
}

#----------------------------------------------------------------------

proc xcircuit::promptsnapspace {} {
   global XCOps
   set XCOps(dialog) snapspace
   .dialog.bbar.apply configure -command \
	{xcircuit::config snap spacing [.dialog.textent.txt get]}
   .dialog.textent.title.field configure -text "Snap spacing:"
   .dialog.textent.txt delete 0 end
   .dialog.textent.txt insert 0 [xcircuit::config snap space]
   xcircuit::popupdialog
}

#----------------------------------------------------------------------

proc xcircuit::promptmakeobject {} {
   global XCOps
   if {[xcircuit::select] > 0} {
      set XCOps(dialog) makeobject
      .dialog.bbar.apply configure -command \
	   {if {[select get] != {}} {\
	   if {[.dialog.textent.txt get] == ""} {\
	   .dialog.textent.title.field configure -text "Please enter a name" }\
	   elseif {[catch {xcircuit::object handle [.dialog.textent.txt get]}]} {\
	   xcircuit::object make [.dialog.textent.txt get] $XCOps(library)}\
	   else {.dialog.textent.title.field configure -text \
	   "Name already used.  Choose another name" ; .dialog.textent.txt \
	   delete 0 end}}}
      .dialog.textent.title.field configure -text "Name for new object"
      .dialog.textent.txt delete 0 end
      if {$XCOps(technology) != "(user)"} {
         .dialog.textent.txt insert 0 "${XCOps(technology)}::"
      }
      xcircuit::popupdialog
      xcircuit::addtechlist .dialog "Technology: " {(user)} prefix
      xcircuit::addliblist .dialog "Place in: "

   }
}

#----------------------------------------------------------------------

proc xcircuit::promptreplaceobject {} {
   global XCOps
   if {[xcircuit::select] > 0} {
      set XCOps(dialog) replaceobject
      .dialog.bbar.apply configure -command \
	  {xcircuit::element selected object [.dialog.textent.txt get]}
      .dialog.textent.title.field configure -text "Name of replacement object"
      .dialog.textent.txt delete 0 end
      xcircuit::popupdialog
   }
}

#----------------------------------------------------------------------

proc xcircuit::promptloadlibrary {} {
   global XCOps

   .filelist.bbar.okay configure -command \
	{xcircuit::library [.filelist.libself.libselect cget -text] load \
	[.filelist.textent.txt get]; wm withdraw .filelist}
   .filelist.listwin.win configure -data "lps"
   .filelist.textent.title.field configure -text "Select technology file to load:"
   .filelist.textent.txt delete 0 end
   xcircuit::popupfilelist
   xcircuit::addliblist .filelist "Load to which library page: "
}

#----------------------------------------------------------------------

proc xcircuit::promptsavetech {} {
   global XCOps

   set XCOps(dialog) techname
   .savetech.bbar.apply configure -command \
	{xcircuit::technology save [.savetech.techself.techselect cget -text] \
	 [.savetech.textent.txt get]}
   .savetech.textent.title.field configure -text "Filename to save technology as:"
   .savetech.textent.txt delete 0 end
   xcircuit::popupdialog .savetech
   xcircuit::addtechlist .savetech "Save which technology: " {(user)} true
   set fname ""
   catch {set fname [technology filename $XCOps(technology)]}
   if {$fname == "(no associated file)"} {
      set fname $XCOps(technology).lps
   }
   .savetech.textent.txt insert 0 $fname
}

#----------------------------------------------------------------------

proc xcircuit::promptaddlibrary {} {
   global XCOps
   set XCOps(dialog) libname
   .dialog.bbar.apply configure -command \
	{xcircuit::library make [.dialog.textent.txt get]}
   .dialog.textent.title.field configure -text "Name of new library page:"
   .dialog.textent.txt delete 0 end
   xcircuit::popupdialog
}

#----------------------------------------------------------------------

proc xcircuit::promptloadfile {} {
   .filelist.bbar.okay configure -command \
	{xcircuit::page load [.filelist.textent.txt get] -replace \
	[.filelist.techself.techselect cget -text] \
	-target $XCOps(library); wm withdraw .filelist}
   .filelist.listwin.win configure -data "ps eps"
   .filelist.textent.title.field configure -text "Select file to load:"
   .filelist.textent.txt delete 0 end
   xcircuit::popupfilelist
   xcircuit::addtechlist .filelist "Replace from: " {(user) all none}
   xcircuit::addliblist .filelist "Target library: "
}

#----------------------------------------------------------------------

proc xcircuit::promptimportspice {} {
   .filelist.bbar.okay configure -command \
	{xcircuit::page import spice \
	[.filelist.textent.txt get]; wm withdraw .filelist}
   .filelist.listwin.win configure -data "spice spc spi ckt sp cir"
   .filelist.textent.title.field configure -text "Select SPICE file to import:"
   .filelist.textent.txt delete 0 end
   xcircuit::popupfilelist
}

#----------------------------------------------------------------------

proc xcircuit::promptimportfile {} {
   .filelist.bbar.okay configure -command \
	{xcircuit::page import xcircuit \
	[.filelist.textent.txt get]; wm withdraw .filelist}
   .filelist.listwin.win configure -data "ps eps"
   .filelist.textent.title.field configure -text "Select file to import:"
   xcircuit::popupfilelist
}

#----------------------------------------------------------------------

proc xcircuit::promptimportbackground {} {
   .filelist.bbar.okay configure -command \
	{xcircuit::page import background \
	[.filelist.textent.txt get]; wm withdraw .filelist}
   .filelist.listwin.win configure -data "ps eps"
   .filelist.textent.title.field configure -text "Select file to use as background:"
   .filelist.textent.txt delete 0 end
   xcircuit::popupfilelist
}

#----------------------------------------------------------------------
# Convert a graphic image using ImageMagick "convert" (if available)
#----------------------------------------------------------------------

proc xcircuit::convertgraphic {filename} {
   set fileext [file extension $filename]
   set fileroot [file rootname $filename]
   set temp true
   switch -- $fileext {
      .gif -
      .jpg -
      .png -
      .pnm {
	 exec convert $filename ${fileroot}.ppm
      }
      .ppm {
	 set temp false
      }
      .ps -
      .pdf {
	 exec convert -density 300x300 $filename ${fileroot}.ppm
      }
   }
   xcircuit::graphic make ${fileroot}.ppm {0 0} 1;
   if {$temp == true} {
      file delete ${fileroot}.ppm
   }
}

#----------------------------------------------------------------------

proc xcircuit::promptimportgraphic {} {
   if {![catch {exec convert -version}]} {
      .filelist.bbar.okay configure -command \
		{xcircuit::convertgraphic [.filelist.textent.txt get];
	 	refresh; wm withdraw .filelist}
      .filelist.listwin.win configure -data "pnm ppm gif jpg png"
   } else {
      .filelist.bbar.okay configure -command \
		{xcircuit::graphic make [.filelist.textent.txt get] {0 0} 1;
	 	refresh; wm withdraw .filelist}
      .filelist.listwin.win configure -data "pnm ppm"
   }
   .filelist.textent.title.field configure -text "Select graphic image file:"
   .filelist.textent.txt delete 0 end
   xcircuit::popupfilelist
}

#----------------------------------------------------------------------

proc xcircuit::promptexecscript {} {
   .filelist.bbar.okay configure -command \
	{source [.filelist.textent.txt get]; wm withdraw .filelist}
   .filelist.listwin.win configure -data "tcl xcircuitrc"
   .filelist.textent.title.field configure -text "Select script to execute:"
   xcircuit::popupfilelist
}

#----------------------------------------------------------------------

proc xcircuit::prompttextsize {} {
   global XCOps
   set XCOps(dialog) textscale
   .dialog.bbar.apply configure -command \
	{xcircuit::label scale [.dialog.textent.txt get]}
   .dialog.textent.title.field configure -text "Text scale:"
   .dialog.textent.txt delete 0 end
   set stext [format "%g" [xcircuit::label scale]]
   .dialog.textent.txt insert 0 $stext
   xcircuit::popupdialog
}

#----------------------------------------------------------------------
# add a library list widget to a dialog box
#----------------------------------------------------------------------

proc xcircuit::addliblist {w {prompt "Target: "}} {
   global XCOps

   frame ${w}.libself
   label ${w}.libself.title -text $prompt -bg beige

   set liblist [library directory list]

   menubutton ${w}.libself.libselect -menu ${w}.libself.libselect.menu -relief groove
   menu ${w}.libself.libselect.menu -tearoff 0
   foreach j $liblist {
      ${w}.libself.libselect.menu add \
	radio -label "$j" -variable XCOps(library) -value \
	"$j" -command "${w}.libself.libselect configure -text {$j}"
   }
   ${w}.libself.libselect configure -text $XCOps(library)

   pack ${w}.libself.title -side left
   pack ${w}.libself.libselect -side left
   pack ${w}.libself -side top -anchor w -padx 20
}

#----------------------------------------------------------------------
# Add a technology list widget to a dialog box
#
# If "update" is "true", then a selection of a technology in the list
# will update the text entry window contents with the corresponding
# filename.  If "update" is "prefix", then a selection of a technology
# in the list will update the text entry window contents with the
# technology namespace prefix corresponding to the selected technology.
# If "update" is "false", then selecting the technology will set the
# global variable XCOps(technology) but will not alter the window
# contents.
#----------------------------------------------------------------------

proc xcircuit::addtechlist {w {prompt "Technology: "} {endlist {}} {update {false}}} {
   global XCOps

   frame ${w}.techself
   label ${w}.techself.title -text $prompt -bg beige

   set techlist [technology list]
   foreach j $endlist { lappend techlist $j }

   menubutton ${w}.techself.techselect -menu ${w}.techself.techselect.menu -relief groove
   menu ${w}.techself.techselect.menu -tearoff 0
   foreach j $techlist {
      if {$update == true} {
	 set fname ""
	 catch {set fname [technology filename "$j"]}
	 
         ${w}.techself.techselect.menu add \
	    radio -label "$j" -variable XCOps(technology) -value \
	    "$j" -command "${w}.techself.techselect configure -text {$j} ; \
		${w}.textent.txt delete 0 end ; ${w}.textent.txt insert 0 {$fname}"
      } elseif {$update == "prefix"} {
	 if {$j != {} && $j != "(user)"} {
	    set tpfix "${j}::"
	 } else {
	    set tpfix {}
	 }
         ${w}.techself.techselect.menu add \
	    radio -label "$j" -variable XCOps(technology) -value \
	    "$j" -command [subst {${w}.techself.techselect configure -text {$j} ; \
		set pfixend \[string last :: \[${w}.textent.txt get\]\] ; \
		if {\$pfixend < 0} {set pfixend 0} else {incr pfixend 2} ; \
		${w}.textent.txt delete 0 \$pfixend ; \
		${w}.textent.txt insert 0 {$tpfix}}]
      } else {
         ${w}.techself.techselect.menu add \
	    radio -label "$j" -variable XCOps(technology) -value \
	    "$j" -command "${w}.techself.techselect configure -text {$j}"
      }
   }
   ${w}.techself.techselect configure -text $XCOps(technology)

   pack ${w}.techself.title -side left
   pack ${w}.techself.techselect -side left
   pack ${w}.techself -side right -anchor w -padx 20
}

#----------------------------------------------------------------------

proc xcircuit::removelists {w} {
   global XCOps

   catch {
      pack forget ${w}.libself
      destroy ${w}.libself
   }
   catch {
      pack forget ${w}.techself
      destroy ${w}.techself
   }
}

#----------------------------------------------------------------------
# newcolorbutton is called internally to xcircuit---don't mess with it!
#----------------------------------------------------------------------

proc xcircuit::newcolorbutton {r g b idx} {
   global XCWinOps XCIRCUIT_LIB_DIR

   set colorrgb [format "#%04X%04X%04X" $r $g $b]
   image create bitmap img_col$idx -foreground $colorrgb -file \
		${XCIRCUIT_LIB_DIR}/pixmaps/solid.xbm

   foreach window [config windownames] {
      set frame [winfo top $window]
      ${frame}.menubar.optionsbutton.optionsmenu.elementsmenu.colormenu \
		add radio -image img_col$idx -activebackground $colorrgb \
		-variable XCWinOps(${frame},colorval) -value $idx -command \
		"xcircuit::color set $idx"
   }
}

#----------------------------------------------------------------------
# Regenerate the list of color buttons for a new window
#----------------------------------------------------------------------

proc xcircuit::allcolorbuttons {window} {
   global XCWinOps

   set colorlist [color get -all]
   set frame [winfo top $window]
   set idx 18		;# NUMBER_OF_COLORS in colordefs.h
   foreach colorrgb $colorlist {
      ${frame}.menubar.optionsbutton.optionsmenu.elementsmenu.colormenu \
		add radio -image img_col$idx -activebackground $colorrgb \
		-variable XCWinOps(${frame},colorval) -value $idx -command \
		"xcircuit::color set $idx"
      incr idx
   }
}

#----------------------------------------------------------------------

proc xcircuit::picknewcolor {} {
   if {[catch {set colorrgb [tk_chooseColor]}]} {
      set colorrgb [tkColorDialog]
   }
   xcircuit::color add $colorrgb
}

#----------------------------------------------------------------------

proc xcircuit::newencodingbutton {encodingname} {
   global XCWinOps

   foreach window [config windownames] {
      set frame [winfo top $window]
      if {[catch {${frame}.menubar.textbutton.textmenu.encodingmenu \
		index $encodingname} result]} {
         ${frame}.menubar.textbutton.textmenu.encodingmenu add radio -label \
	  	$encodingname -command "xcircuit::label encoding $encodingname" \
		-variable XCWinOps(${frame},fontencoding) -value $encodingname
      }
   }
}

#----------------------------------------------------------------------

proc xcircuit::newfontbutton {familyname} {
   global XCWinOps

   foreach window [config windownames] {
      set frame [winfo top $window]
      if {[catch {${frame}.menubar.textbutton.textmenu.fontmenu \
		index $familyname} result]} {
         ${frame}.menubar.textbutton.textmenu.fontmenu add radio -label \
	  	$familyname -command "xcircuit::label family $familyname" \
		-variable XCWinOps(${frame},fontfamily) -value $familyname
      }
   }
}

#----------------------------------------------------------------------
# Regenerate the list of known font families for a new window
#----------------------------------------------------------------------

proc xcircuit::allfontbuttons {window} {
   global XCWinOps

   set familylist [label family -all]
   set frame [winfo top $window]
   while {$familylist != {}} {
      set familyname [lindex $familylist 0]
      ${frame}.menubar.textbutton.textmenu.fontmenu add radio -label \
	  	$familyname -command "xcircuit::label family $familyname" \
		-variable XCWinOps(${frame},fontfamily) -value $familyname
      # Remove all such entries (this works like "lsort -unique" but doesn't
      # scramble the list entries).
      set familylist [lsearch -all -inline -not $familylist $familyname]
   }
}

#----------------------------------------------------------------------

proc xcircuit::newlibrarybutton {libname} {
   if {[catch {.librarymenu index $libname} result]} {
      set libidx [.librarymenu index end]
      if {$libidx <= 1} {set libidx [expr $libidx + 1]}
      .librarymenu insert $libidx command -label $libname -command \
		"xcircuit::library \"$libname\" goto"
   }
}

#----------------------------------------------------------------------

proc xcircuit::newpagebutton {pagename {pageno 0}} {
   if {[catch {.pagemenu index $pagename} result]} {
      set target $pagename
      if {$pageno > 0} { set target $pageno }
      .pagemenu add command -label $pagename -command \
		"xcircuit::page \"$target\" goto"
   }
}

#----------------------------------------------------------------------

proc xcircuit::renamelib {libno libname} {
   set target [expr $libno + 1]
   .librarymenu entryconfigure $target -label $libname -command \
		"xcircuit::library \"$libname\" goto"
}

#----------------------------------------------------------------------

proc xcircuit::renamepage {pageno pagename} {
   set target [expr $pageno + 1]
   .pagemenu entryconfigure $target -label $pagename -command \
		"xcircuit::page \"$pagename\" goto"
}

#----------------------------------------------------------------------

proc xcircuit::promptnewfont {} {
   global XCOps
   set XCOps(dialog) fontname
   .dialog.bbar.apply configure -command \
	{xcircuit::loadfont [.dialog.textent.txt get]}
   .dialog.textent.title.field configure -text "Font name:"
   .dialog.textent.txt delete 0 end
   xcircuit::popupdialog
}

#----------------------------------------------------------------------

proc xcircuit::promptkern {} {
   global XCOps
   set XCOps(dialog) kernamount
   .dialog.bbar.apply configure -command \
	{xcircuit::label insert kern [.dialog.textent.txt get]}
   .dialog.textent.title.field configure -text "Kern amount:"
   .dialog.textent.txt delete 0 end
   xcircuit::popupdialog
}

#----------------------------------------------------------------------

proc xcircuit::promptmargin {} {
   global XCOps
   set XCOps(dialog) marginamount
   .dialog.bbar.apply configure -command \
	{xcircuit::label insert margin [.dialog.textent.txt get]}
   .dialog.textent.title.field configure -text "Margin amount:"
   .dialog.textent.txt delete 0 end
   xcircuit::popupdialog
}

#----------------------------------------------------------------------

proc xcircuit::maketoolimages {} {
    global XCOps XCIRCUIT_LIB_DIR
    set XCOps(tools) [list pn w b a s t mv cp e d2 cw ccw fx fy r pu2 po2 mk pz \
		uj co bd fi pm pa li yp pl z4 z5 i]

    if [catch {set XCOps(scale)}] {
    	set XCOps(scale) [expr {int([font measure TkDefaultFont M] / 10)}]
    }
    puts stdout "XCOps(scale) set to $XCOps(scale)"
    set gsize [expr {int($XCOps(scale) * 20)}]
    set gscale [expr {int($XCOps(scale))}]

    for {set i 0} {$i < [llength $XCOps(tools)]} {incr i 1} {
	set bname [lindex $XCOps(tools) $i]
	image create photo stdimage -file ${XCIRCUIT_LIB_DIR}/pixmaps/${bname}.gif
	image create photo img_${bname} -width $gsize -height $gsize
	img_${bname} copy stdimage -zoom $gscale
    }
}

#----------------------------------------------------------------------

proc xcircuit::modebutton {widget} {
   global XCOps XCWinOps
   set window $XCOps(focus)

   for {set i 0} {$i < [llength $XCOps(tools)]} {incr i 1} {
      set bname [lindex $XCOps(tools) $i]
      ${window}.mainframe.toolbar.b${bname} configure -relief raised \
		-highlightbackground gray90 -background gray90
   }
   $widget configure -relief solid -highlightbackground green3 -background green3
   if {$XCWinOps(${window},button1) == "Rotate"} {
      ${window}.infobar.mode configure -text "$XCWinOps(${window},button1) \
		$XCWinOps(${window},rotateamount) Mode"
   } else {
      ${window}.infobar.mode configure -text "$XCWinOps(${window},button1) Mode"
   }
}

#----------------------------------------------------------------------

proc xcircuit::button1action {button cursor action {value {}}} {
   global XCOps XCWinOps

   set window $XCOps(focus).mainframe.mainarea.drawing
   catch {bindkey $window Button1 $XCWinOps($XCOps(focus),button1) forget}

   if {$value == {}} {
      bindkey $window Button1 $action
   } else {
      bindkey $window Button1 $action $value
   }
   set XCWinOps($XCOps(focus),button1) $action
   xcircuit::modebutton $button
   catch {cursor $cursor}
   catch {xcircuit::automousehint $window}
}

#----------------------------------------------------------------------

proc xcircuit::createtoolbar {window} {
   global XCOps XCWinOps XCIRCUIT_LIB_DIR

   set tooltips [list "pan window" "draw wire" "draw box" \
	"draw arc" "draw spline" "enter text" \
	"move element" "copy element" "edit element" "delete element" \
	"rotate 15 degrees clockwise" "rotate 15 degrees counterclockwise" \
	"flip horizontal" "flip vertical" "rescale" "push (edit object)" \
	"pop (return from object edit)" "make an object from selection" \
	"join elements into polygon or path" "separate path into elements" \
	"set color" "set border and line properties" "set fill properties" \
	"parameterize properties" "parameter selection" \
	"go to next library" "go to library directory" \
	"go to page directory" "zoom in" "zoom out" "pop up help window"]	
   set toolactions [list \
	{xcircuit::button1action %W hand Pan 6} \
	{xcircuit::button1action %W cross Wire} \
	{xcircuit::button1action %W cross Box} \
	{xcircuit::button1action %W circle Arc} \
	{if {[select]} {spline make} else { \
		xcircuit::button1action %W arrow Spline}} \
	{xcircuit::button1action %W text $XCWinOps($XCOps(focus),labeltype) ; \
		$XCOps(focus).mainframe.toolbar.bt configure -image \
		$XCWinOps($XCOps(focus),labelimage)} \
	{if {[select]} {move selected} else { \
		xcircuit::button1action %W arrow Move}} \
	{if {[select]} {copy selected} else { \
		xcircuit::button1action %W copy Copy}} \
	{if {[select] == 1} {edit selected} else { \
		xcircuit::button1action %W edit Edit}} \
	{if {[select]} {delete selected} else { \
		xcircuit::button1action %W scissors Delete}} \
	{if {[select]} {rotate $XCWinOps($XCOps(focus),rotateamount)} else {\
		xcircuit::button1action %W rotate Rotate \
		$XCWinOps($XCOps(focus),rotateamount)}} \
	{if {[select]} {rotate -$XCWinOps($XCOps(focus),rotateamount)} else {\
		xcircuit::button1action %W rotate Rotate \
		-$XCWinOps($XCOps(focus),rotateamount)}} \
	{if {[select]} {flip horizontal} else { \
		xcircuit::button1action %W rotate "Flip X"}} \
	{if {[select]} {flip vertical} else { \
		xcircuit::button1action %W rotate "Flip Y"}} \
	{if {[select]} {xcircuit::promptelementsize} else { \
		xcircuit::button1action %W arrow Rescale}} \
	{if {[select] == 1} {push selected} else { \
		xcircuit::button1action %W question Push}} \
	{pop} \
	{if {[select]} {xcircuit::promptmakeobject} else { \
		xcircuit::button1action %W question "Select Save"}} \
	{if {[select]} {path join selected} else { \
		xcircuit::button1action %W arrow Join}} \
	{if {[select] == 1} {path unjoin selected} else { \
		xcircuit::button1action %W arrow Unjoin}} \
	{tk_popup $XCOps(focus).colormenu [expr {[winfo rootx \
		$XCOps(focus).mainframe.toolbar.bco] \
		- [winfo width $XCOps(focus).colormenu]}] \
		[expr {[winfo rooty $XCOps(focus).mainframe.toolbar.bco] \
		- [winfo height $XCOps(focus).colormenu] / 2}] } \
	{tk_popup $XCOps(focus).bordermenu [expr {[winfo rootx \
		$XCOps(focus).mainframe.toolbar.bbd] \
		- [winfo width $XCOps(focus).bordermenu]}] \
		[expr {[winfo rooty $XCOps(focus).mainframe.toolbar.bbd] \
		- [winfo height $XCOps(focus).bordermenu] / 2}] } \
	{tk_popup $XCOps(focus).fillmenu [expr {[winfo rootx \
		$XCOps(focus).mainframe.toolbar.bfi] \
		- [winfo width $XCOps(focus).fillmenu]}] \
		[expr {[winfo rooty $XCOps(focus).mainframe.toolbar.bfi] \
		- [winfo height $XCOps(focus).fillmenu] / 2}] } \
	{tk_popup $XCOps(focus).parammenu [expr {[winfo rootx \
		$XCOps(focus).mainframe.toolbar.bpm] \
		- [winfo width $XCOps(focus).parammenu]}] \
		[expr {[winfo rooty $XCOps(focus).mainframe.toolbar.bpm] \
		- [winfo height $XCOps(focus).parammenu] / 2}] } \
	{xcircuit::prompteditparams} \
	{library next} \
	{library directory} {page directory} \
	{zoom 1.5; refresh} {zoom [expr {1 / 1.5}]; refresh} \
	{xcircuit::helpwindow} ]

   # Make the tool images if they have not yet been created.
   if [catch {set XCOps(tools)}] {xcircuit::maketoolimages}

   for {set i 0} {$i < [llength $XCOps(tools)]} {incr i 1} {
      set bname [lindex $XCOps(tools) $i]
      set btip [lindex $tooltips $i]
      regsub -all -- %W [lindex $toolactions $i] \
		${window}.mainframe.toolbar.b${bname} bcmd
      button ${window}.mainframe.toolbar.b${bname} -image img_${bname} -command \
		"$bcmd"
      bind ${window}.mainframe.toolbar.b${bname} <Enter> \
		[subst {${window}.infobar.message2 configure -text "$btip"}]
      bind ${window}.mainframe.toolbar.b${bname} <Leave> \
		[subst {${window}.infobar.message2 configure -text ""}]
   }

   # pack the first button so we can query its height for arrangement.
   # this assumes that the height of each button is the same!
   set bname [lindex $XCOps(tools) 0]
   place ${window}.mainframe.toolbar.b${bname} -x 0 -y 0
   update idletasks
}

#----------------------------------------------------------------------

proc xcircuit::arrangetoolbar {window} {
   global XCOps

   set numtools [llength $XCOps(tools)]
   for {set i 0} {$i < $numtools} {incr i 1} {
      set bname [lindex $XCOps(tools) $i]
      place forget ${window}.mainframe.toolbar.b${bname}
   }
   set bname [lindex $XCOps(tools) 0]
   set bheight [winfo height ${window}.mainframe.toolbar.b${bname}]
   set bwidth [winfo width ${window}.mainframe.toolbar.b${bname}]
   set wheight [winfo height ${window}.mainframe]
   set nrows [expr {$wheight / $bheight}]
   ${window}.mainframe.toolbar configure -width [expr {$bwidth}]
   set j 0
   set k 0
   for {set i 0} {$i < [llength $XCOps(tools)]} {incr i; incr j} {
      if {$j == $nrows} {
	 set j 0
	 incr k
	 ${window}.mainframe.toolbar configure -width [expr {($k + 1) * $bwidth}]
      }
      set bname [lindex $XCOps(tools) $i]
      place ${window}.mainframe.toolbar.b${bname} \
		-x [expr {$k * $bwidth}] \
		-y [expr {$j * $bheight}]
   }
}

#----------------------------------------------------------------------

proc xcircuit::toolbar {value} {
   global XCOps
   set window $XCOps(focus)

   switch -- $value {
      true -
      enable {
	 pack forget ${window}.mainframe.mainarea
	 pack ${window}.mainframe.toolbar -side right -fill y -padx 2
	 pack ${window}.mainframe.mainarea -expand true -fill both
	 set midx [${window}.menubar.optionsbutton.optionsmenu \
		index "Enable Toolbar"]
	 ${window}.menubar.optionsbutton.optionsmenu entryconfigure $midx \
		-command {xcircuit::toolbar disable} -label \
		"Disable Toolbar"
      }
      false -
      disable {
	 pack forget ${window}.mainframe.toolbar
	 set midx [${window}.menubar.optionsbutton.optionsmenu \
		index "Disable Toolbar"]
	 ${window}.menubar.optionsbutton.optionsmenu entryconfigure $midx \
		-command {xcircuit::toolbar enable} -label \
		"Enable Toolbar"
      }
   }
}

#----------------------------------------------------------------------
# These variables are associated with toggle and radio buttons
# but must be the same for all windows.
#----------------------------------------------------------------------

set XCOps(sheetsize) letter
set XCOps(spiceend)  true
set XCOps(forcenets) true
set XCOps(hold)      true

set XCOps(focus)     .xcircuit

#----------------------------------------------------------------------
# Create stipple images
#----------------------------------------------------------------------

image create bitmap img_stip0 -foreground white -background black -file \
	${XCIRCUIT_LIB_DIR}/pixmaps/solid.xbm
image create bitmap img_stip12 -foreground black -background white -file \
	${XCIRCUIT_LIB_DIR}/pixmaps/stip12.xbm
image create bitmap img_stip25 -foreground black -background white -file \
	${XCIRCUIT_LIB_DIR}/pixmaps/stip25.xbm
image create bitmap img_stip38 -foreground black -background white -file \
	${XCIRCUIT_LIB_DIR}/pixmaps/stip38.xbm
image create bitmap img_stip50 -foreground black -background white -file \
	${XCIRCUIT_LIB_DIR}/pixmaps/stip50.xbm
image create bitmap img_stip62 -foreground black -background white -file \
	${XCIRCUIT_LIB_DIR}/pixmaps/stip62.xbm
image create bitmap img_stip75 -foreground black -background white -file \
	${XCIRCUIT_LIB_DIR}/pixmaps/stip75.xbm
image create bitmap img_stip88 -foreground black -background white -file \
	${XCIRCUIT_LIB_DIR}/pixmaps/stip88.xbm
image create bitmap img_stip100 -foreground black -background white -file \
	${XCIRCUIT_LIB_DIR}/pixmaps/solid.xbm

#----------------------------------------------------------------------
# alternate label type images (info, global, and pin labels)
#----------------------------------------------------------------------

image create photo img_ti -file ${XCIRCUIT_LIB_DIR}/pixmaps/ti.gif
image create photo img_tg -file ${XCIRCUIT_LIB_DIR}/pixmaps/tg.gif
image create photo img_tp -file ${XCIRCUIT_LIB_DIR}/pixmaps/tp.gif
 
#----------------------------------------------------------------------

proc xcircuit::makemenus {window} {
   global XCOps

   set m [menu ${window}.menubar.filebutton.filemenu -tearoff 0]
   $m add command -label "New Window" -command {xcircuit::forkwindow}
   $m add command -label "Close Window" -command \
	"xcircuit::closewindow ${window}.mainframe.mainarea.drawing"
   $m add separator
   $m add command -label "Read XCircuit File" -command {xcircuit::promptloadfile}
   if {![catch {set XCOps(module,files)}]} {
	$m add command -label "Write All..." -command {xcircuit::promptwriteall}
   }
   $m add command -label "Format Page Output" -command {xcircuit::promptsavepage}
   $m add separator
   $m add command -label "Load Dependencies" -command \
	{while {[page links load -replace $XCOps(technology) \
	-target $XCOps(library)]} {}}
   $m add cascade -label "Import" -menu $m.importmenu
   $m add cascade -label "Export" -menu $m.exportmenu

   $m add command -label "Execute Script" -command {xcircuit::promptexecscript}
   if {[file tail [info nameofexecutable]] != "xcircexec"} {
      $m add command -label "Tcl Console" -command {xcircuit::raiseconsole}
   }
   $m add separator
   if {![catch {set XCOps(module,library)}]} {
      $m add command -label "Library Manager" -command {xcircuit::raisemanager}
   }
   $m add command -label "New Library Page" -command {xcircuit::promptaddlibrary}
   $m add command -label "Load Technology (.lps)" -command {xcircuit::promptloadlibrary}
   $m add command -label "Save Technology (.lps)" -command {xcircuit::promptsavetech}

   $m add separator
   $m add command -label "Clear Page" -command {xcircuit::page reset}
   $m add separator
   $m add command -label "Quit" -command {quit}

   # Sub-menu for Import functions

   set m2 [menu $m.importmenu -tearoff 0]
   $m2 add command -label "Import XCircuit File" -command {xcircuit::promptimportfile}
   $m2 add command -label "Import background PS" -command \
	{xcircuit::promptimportbackground}
   $m2 add command -label "Import graphic image" -command \
	{xcircuit::promptimportgraphic}
   if {![catch {set XCOps(module,edif)}]} {
      $m2 add command -label "Read EDIF file" -command {xcircuit::promptreadedif}
   }
   if {![catch {set XCOps(module,synopsys)}]} {
      $m2 add command -label "Read Synopsys library" -command {xcircuit::promptreadsynopsys}
   }
   if {![catch {set XCOps(module,matgen)}]} {
      $m2 add command -label "Import Matlab PS" -command {xcircuit::promptimportmatlab}
   }
   if {![catch {set XCIRCUIT_ASG}]} {
      $m2 add command -label "Import SPICE Deck" -command \
	{xcircuit::promptimportspice}
   }
   if {![catch {set XCOps(module,text)}]} {
      $m2 add command -label "Import text" -command {xcircuit::promptimporttext}
   }

   # Sub-menu for Export functions

   set m2 [menu $m.exportmenu -tearoff 0]
   $m2 add command -label "Export SVG" -command {svg -fullscale}

   set m [menu ${window}.menubar.editbutton.editmenu -tearoff 0]
   $m add command -label "Undo" -command {undo}
   $m add command -label "Redo" -command {redo}
   $m add separator
   $m add command -label "Delete" -command \
	"${window}.mainframe.toolbar.bd2 invoke"
   $m add command -label "Copy" -command \
	"${window}.mainframe.toolbar.bcp invoke"
   $m add command -label "Move" -command \
	"${window}.mainframe.toolbar.bmv invoke"
   $m add command -label "Edit" -command \
	"${window}.mainframe.toolbar.be invoke"
   $m add cascade -label "Rotate/Flip" -menu $m.rotmenu
   $m add command -label "Deselect" -command {deselect selected}
   $m add cascade -label "Select Filter" -menu $m.selmenu
   $m add command -label "Push Selected" -command \
	"${window}.mainframe.toolbar.pu2 invoke"
   $m add command -label "Pop Hierarchy" -command {pop}
   $m add separator
   $m add command -label "Change Technology" -command \
	{xcircuit::prompttargettech [element selected object]}
   $m add separator
   $m add command -label "Make User Object" -command \
	"${window}.mainframe.toolbar.bmk invoke"
   $m add command -label "Make Arc" -command \
	"${window}.mainframe.toolbar.ba invoke"
   $m add command -label "Make Box" -command \
	"${window}.mainframe.toolbar.bb invoke"
   $m add command -label "Make Spline" -command \
	"${window}.mainframe.toolbar.bs invoke"
   $m add command -label "Make Wire" -command \
	"${window}.mainframe.toolbar.bw invoke"
   $m add command -label "Replace" -command {xcircuit::promptreplaceobject}
   $m add command -label "Join" -command \
	"${window}.mainframe.toolbar.bpz invoke"
   $m add command -label "Unjoin" -command \
	"${window}.mainframe.toolbar.buj invoke"
   $m add command -label "Raise/Lower" -command {element exchange}

   set m2 [menu $m.rotmenu -tearoff 0]
   $m2 add command -label "Flip Horizontal" -command \
	"${window}.mainframe.toolbar.bfx invoke"
   $m2 add command -label "Flip Vertical" -command \
	"${window}.mainframe.toolbar.bfy invoke"
   $m2 add command -label "Rescale" -command \
	"${window}.mainframe.toolbar.r invoke"
   $m2 add separator
   $m2 add command -label "Rotate CW 90" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 90; \
	$XCOps(focus).mainframe.toolbar.bcw invoke}
   $m2 add command -label "Rotate CW 45" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 45; \
	$XCOps(focus).mainframe.toolbar.bcw invoke}
   $m2 add command -label "Rotate CW 30" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 30; \
	$XCOps(focus).mainframe.toolbar.bcw invoke}
   $m2 add command -label "Rotate CW 15" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 15; \
	$XCOps(focus).mainframe.toolbar.bcw invoke}
   $m2 add command -label "Rotate CW 5" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 5; \
	$XCOps(focus).mainframe.toolbar.bcw invoke}
   $m2 add command -label "Rotate CW 1" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 1; \
	$XCOps(focus).mainframe.toolbar.bcw invoke}
   $m2 add separator
   $m2 add command -label "Rotate CCW 90" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 90; \
	$XCOps(focus).mainframe.toolbar.bccw invoke}
   $m2 add command -label "Rotate CCW 45" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 45; \
	$XCOps(focus).mainframe.toolbar.bccw invoke}
   $m2 add command -label "Rotate CCW 30" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 30; \
	$XCOps(focus).mainframe.toolbar.bccw invoke}
   $m2 add command -label "Rotate CCW 15" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 15; \
	$XCOps(focus).mainframe.toolbar.bccw invoke}
   $m2 add command -label "Rotate CCW 5" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 5; \
	$XCOps(focus).mainframe.toolbar.bccw invoke}
   $m2 add command -label "Rotate CCW 1" -command \
	{set XCWinOps($XCOps(focus),rotateamount) 1; \
	$XCOps(focus).mainframe.toolbar.bccw invoke}

   set m2 [menu $m.selmenu -tearoff 0]
   $m2 add command -label "Disable selection" -command {element select hide}
   $m2 add command -label "Remove all disabled" -command {element select allow}
   $m2 add separator
   $m2 add check -label "Labels" -variable XCWinOps(${window},sel_lab) \
	-onvalue true -offvalue false -command \
	{xcircuit::config filter label $XCWinOps($XCOps(focus),sel_lab)}
   $m2 add check -label "Objects" -variable XCWinOps(${window},sel_inst) \
	-onvalue true -offvalue false -command \
	{xcircuit::config filter instance $XCWinOps($XCOps(focus),sel_inst)}
   $m2 add check -label "Polygons" -variable XCWinOps(${window},sel_poly) \
	-onvalue true -offvalue false -command \
	{xcircuit::config filter polygon $XCWinOps($XCOps(focus),sel_poly)}
   $m2 add check -label "Arcs" -variable XCWinOps(${window},sel_arc) \
	-onvalue true -offvalue false -command \
	{xcircuit::config filter arc $XCWinOps($XCOps(focus),sel_arc)}
   $m2 add check -label "Splines" -variable XCWinOps(${window},sel_spline) \
	-onvalue true -offvalue false -command \
	{xcircuit::config filter spline $XCWinOps($XCOps(focus),sel_spline)}
   $m2 add check -label "Paths" -variable XCWinOps(${window},sel_path) \
	-onvalue true -offvalue false -command \
	{xcircuit::config filter path $XCWinOps($XCOps(focus),sel_path)}
   $m2 add check -label "Graphic Images" -variable XCWinOps(${window},sel_graphic) \
	-onvalue true -offvalue false -command \
	{xcircuit::config filter graphic $XCWinOps($XCOps(focus),sel_graphic)}

   set m [menu ${window}.menubar.textbutton.textmenu -tearoff 0]
   $m add command -label "Text Size" -command {xcircuit::prompttextsize}
   $m add cascade -label "Font" -menu $m.fontmenu
   $m add cascade -label "Style" -menu $m.stylemenu
   $m add cascade -label "Encoding" -menu $m.encodingmenu
   $m add cascade -label "Insert" -menu $m.insertmenu
   $m add cascade -label "Anchoring" -menu $m.anchormenu
   $m add command -label "Parameterize" \
	-command {xcircuit::labelmakeparam}
   $m add command -label "Unparameterize" \
	-command {xcircuit::parameter replace substring}
   $m add separator
   $m add check -label "LaTeX mode" -variable XCWinOps(${window},latexmode) \
	-onvalue true -offvalue false -command {xcircuit::label latex \
	$XCWinOps($XCOps(focus),latexmode)}
   if {![catch {set XCOps(module,text)}]} {
      $m add separator
      $m add command -label "Modify Text..." -command {xcircuit::textmod}
   }
   $m add separator
   $m add command -label "Make Text" -command \
	{set XCWinOps($XCOps(focus),labeltype) "Text"; set \
	XCWinOps($XCOps(focus),labelimage) img_t; \
	$XCOps(focus).mainframe.toolbar.bt invoke}

   set m2 [menu $m.fontmenu -tearoff 0]
   $m2 add command -label "Add New Font" -command {xcircuit::promptnewfont}
   $m2 add separator

   set m2 [menu $m.stylemenu -tearoff 0]
   $m2 add radio -label "Normal" -variable XCWinOps(${window},fontstyle) \
	-value normal -command "xcircuit::label style normal"
   $m2 add radio -label "Bold" -variable XCWinOps(${window},fontstyle) \
	-value bold -command "xcircuit::label style bold"
   $m2 add radio -label "Italic" -variable XCWinOps(${window},fontstyle) \
	-value italic -command "xcircuit::label style italic"
   $m2 add radio -label "BoldItalic" -variable XCWinOps(${window},fontstyle) \
	-value bolditalic -command "xcircuit::label style bolditalic"
   $m2 add separator
   $m2 add radio -label "Subscript" -variable XCWinOps(${window},fontscript) \
	-value subscript -command "xcircuit::label insert subscript"
   $m2 add radio -label "Superscript" -variable XCWinOps(${window},fontscript) \
	-value superscript -command "xcircuit::label insert superscript"
   $m2 add radio -label "Normalscript" -variable XCWinOps(${window},fontscript) \
	-value normal -command "xcircuit::label insert normalscript"
   $m2 add separator
   $m2 add radio -label "Underline" -variable XCWinOps(${window},fontlining) \
	-value underline -command "xcircuit::label insert underline"
   $m2 add radio -label "Overline" -variable XCWinOps(${window},fontlining) \
	-value overline -command "xcircuit::label insert overline"
   $m2 add radio -label "No Line" -variable XCWinOps(${window},fontlining) \
	-value normal -command "xcircuit::label insert noline"

   set m2 [menu $m.encodingmenu -tearoff 0]
   $m2 add radio -label "Standard" -variable XCWinOps(${window},fontencoding) \
	-value Standard -command "xcircuit::label encoding Standard"
   $m2 add radio -label "ISOLatin1" -variable XCWinOps(${window},fontencoding) \
	-value ISOLatin1 -command "xcircuit::label encoding ISOLatin1"

   set m2 [menu $m.insertmenu -tearoff 0]
   $m2 add command -label "Tab stop" -command "xcircuit::label insert stop"
   $m2 add command -label "Tab forward" -command "xcircuit::label insert forward"
   $m2 add command -label "Tab backward" -command "xcircuit::label insert backward"
   $m2 add command -label "Margin stop" -command {if {[lindex [xcircuit::label \
		substring] 1] <= 1} {xcircuit::promptmargin} else \
		{xcircuit::label insert margin}}
   $m2 add command -label "Carriage Return" -command "xcircuit::label insert return"
   $m2 add command -label "1/2 space" -command "xcircuit::label insert halfspace"
   $m2 add command -label "1/4 space" -command "xcircuit::label insert quarterspace"
   $m2 add command -label "Kern" -command "xcircuit::promptkern"
   $m2 add command -label "Character" -command "xcircuit::label insert special"
   $m2 add command -label "Parameter" -command "xcircuit::prompteditparams"

   set m2 [menu $m.anchormenu -tearoff 0]
   $m2 add radio -label "Left Anchored" -variable XCWinOps(${window},jhoriz) \
	-value left -command "xcircuit::label anchor left"
   $m2 add radio -label "Center Anchored" -variable XCWinOps(${window},jhoriz) \
	-value center -command "xcircuit::label anchor center"
   $m2 add radio -label "Right Anchored" -variable XCWinOps(${window},jhoriz) \
	-value right -command "xcircuit::label anchor right"
   $m2 add separator
   $m2 add radio -label "Top Anchored" -variable XCWinOps(${window},jvert) \
	-value top -command "xcircuit::label anchor top"
   $m2 add radio -label "Middle Anchored" -variable XCWinOps(${window},jvert) \
	-value middle -command "xcircuit::label anchor middle"
   $m2 add radio -label "Bottom Anchored" -variable XCWinOps(${window},jvert) \
	-value bottom -command "xcircuit::label anchor bottom"
   $m2 add separator
   $m2 add radio -label "Left Justified" -variable XCWinOps(${window},justif) \
	-value left -command "xcircuit::label justify left"
   $m2 add radio -label "Center Justified" -variable XCWinOps(${window},justif) \
	-value center -command "xcircuit::label justify center"
   $m2 add radio -label "Right Justified" -variable XCWinOps(${window},justif) \
	-value right -command "xcircuit::label justify right"
   $m2 add separator
   $m2 add check -label "Flip Invariant" \
	-variable XCWinOps(${window},flipinvariant) \
	-onvalue true -offvalue false -command {xcircuit::label flipinvariant \
	$XCWinOps($XCOps(focus),flipinvariant)}

   set m [menu ${window}.menubar.optionsbutton.optionsmenu -tearoff 0]
   $m add check -label "Alt Colors" -variable XCWinOps(${window},colorscheme) \
	-onvalue inverse -offvalue normal -command {xcircuit::config \
	colorscheme $XCWinOps($XCOps(focus),colorscheme)}
   $m add check -label "Show Bounding Box" -variable XCWinOps(${window},showbbox) \
	-onvalue visible -offvalue invisible -command \
	{xcircuit::config bbox $XCWinOps($XCOps(focus),showbbox)}
   $m add check -label "Edit In Place" -variable XCWinOps(${window},editinplace) \
	-onvalue true -offvalue false -command {xcircuit::config editinplace \
	$XCWinOps($XCOps(focus),editinplace)}
   $m add check -label "Show Pin Positions" \
	-variable XCWinOps(${window},pinpositions) \
	-onvalue visible -offvalue invisible -command \
	{xcircuit::config pinpositions $XCWinOps($XCOps(focus),pinpositions)}
   $m add check -label "Wires Stay Attached to Pins" \
	-variable XCWinOps(${window},pinattach) \
	-onvalue true -offvalue false -command \
	{xcircuit::config pinattach $XCWinOps($XCOps(focus),pinattach)}
   $m add check -label "Show Clipmask Outlines" \
	-variable XCWinOps(${window},showclipmasks) \
	-onvalue show -offvalue hide -command \
	{xcircuit::config clipmasks $XCWinOps($XCOps(focus),showclipmasks)}
   $m add check -label "Show Technology Namespaces" \
	-variable XCWinOps(${window},namespaces) \
	-onvalue true -offvalue false -command \
	{xcircuit::config technologies $XCWinOps($XCOps(focus),namespaces)}

   $m add command -label "Disable Toolbar" -command {xcircuit::toolbar disable}
   $m add check -label "Allow HOLD Mode" -variable XCOps(hold) -onvalue true \
	-offvalue false -command {xcircuit::config hold $XCOps(hold)}
   $m add cascade -label "Grid" -menu $m.gridmenu
   $m add cascade -label "Snap-to" -menu $m.snapmenu
   $m add cascade -label "Linewidth" -menu $m.linemenu
   $m add cascade -label "Elements" -menu $m.elementsmenu
   $m add separator
   $m add command -label "Help!" -command {xcircuit::helpwindow}

   set m2 [menu $m.gridmenu -tearoff 0]
   $m2 add check -label "Grid" -variable XCWinOps(${window},showgrid) \
	-onvalue true -offvalue false \
	-command {xcircuit::config grid $XCWinOps($XCOps(focus),showgrid); refresh}
   $m2 add check -label "Axes" -variable XCWinOps(${window},showaxes) \
	-onvalue true -offvalue false \
	-command {xcircuit::config axes $XCWinOps($XCOps(focus),showaxes); refresh}
   $m2 add command -label "Grid Spacing" -command {xcircuit::promptgridspace}
   $m2 add cascade -label "Grid type/display" -menu $m2.gridsubmenu

   set m3 [menu $m2.gridsubmenu -tearoff 0]
   $m3 add radio -label "Decimal Inches" -variable XCWinOps(${window},gridstyle) \
	-value "decimal inches" \
	-command {xcircuit::config coordstyle "decimal inches"}
   $m3 add radio -label "Fractional Inches" -variable XCWinOps(${window},gridstyle) \
	-value "fractional inches" \
	-command {xcircuit::config coordstyle "fractional inches"}
   $m3 add radio -label "Centimeters" -variable XCWinOps(${window},gridstyle) \
	-value "centimeters" -command {xcircuit::config coordstyle "centimeters"}
   $m3 add radio -label "Internal Units" -variable XCWinOps(${window},gridstyle) \
	-value "internal units" -command \
	{xcircuit::config coordstyle "internal units"}
   $m3 add separator
   $m3 add command -label "Drawing Scale" -command {xcircuit::promptdrawingscale}

   set m2 [menu $m.snapmenu -tearoff 0]
   $m2 add check -label "Snap-to" -variable XCWinOps(${window},showsnap) \
	-onvalue true \
	-offvalue false -command {xcircuit::config snap \
	$XCWinOps($XCOps(focus),showsnap); refresh}
   $m2 add command -label "Snap Spacing" -command {xcircuit::promptsnapspace}

   set m2 [menu $m.linemenu -tearoff 0]
   $m2 add command -label "Wire Linewidth" -command {xcircuit::promptborderwidth}
   $m2 add command -label "Global Linewidth" -command {xcircuit::promptlinewidth}

   set m2 [menu $m.elementsmenu -tearoff 0]
   $m2 add cascade -label "Border" -menu $m2.bordermenu
   $m2 add cascade -label "Fill" -menu $m2.fillmenu
   $m2 add cascade -label "Color" -menu $m2.colormenu
   $m2 add separator
   $m2 add cascade -label "Parameters" -menu $m2.parammenu
   $m2 add command -label "Scale" -command {xcircuit::promptelementsize}
   $m2 add check -label "Center Object" -variable XCWinOps(${window},centerobject) \
	-onvalue true -offvalue false -command {xcircuit::config centering \
	$XCWinOps($XCOps(focus),centerobject)}
   $m2 add check -label "Instance Scale-invariant Linewidth" \
	-variable XCWinOps(${window},scaleinvariant) \
	-onvalue invariant -offvalue variant -command {xcircuit::instance linewidth \
	$XCWinOps($XCOps(focus),scaleinvariant)}
   $m2 add check -label "Manhattan Draw" \
	-variable XCWinOps(${window},manhattandraw) \
	-onvalue true -offvalue false -command {xcircuit::config manhattan \
	$XCWinOps($XCOps(focus),manhattandraw)}
   $m2 add check -label "Link Curve Tangents" \
	-variable XCWinOps(${window},pathedittype) \
	-onvalue tangents -offvalue normal -command {xcircuit::config pathedit \
	$XCWinOps($XCOps(focus),pathedittype)}
   $m2 add cascade -label "Polygon Edit" -menu $m2.polyeditmenu

   set m3 [menu $m2.bordermenu -tearoff 0]
   $m3 add command -label "Linewidth" -command {xcircuit::promptborderwidth}
   $m3 add separator
   $m3 add radio -label "Solid" -variable XCWinOps(${window},linestyle) \
	-value solid -command {xcircuit::border solid}
   $m3 add radio -label "Dashed" -variable XCWinOps(${window},linestyle) \
	-value dashed -command {xcircuit::border dashed}
   $m3 add radio -label "Dotted" -variable XCWinOps(${window},linestyle) \
	-value dotted -command {xcircuit::border dotted}
   $m3 add radio -label "Unbordered" -variable XCWinOps(${window},linestyle) \
	-value unbordered -command {xcircuit::border unbordered}
   $m3 add separator
   $m3 add check -label "Closed" -variable XCWinOps(${window},polyclosed) \
	-onvalue closed -offvalue unclosed -command \
	{xcircuit::border $XCWinOps($XCOps(focus),polyclosed)}
   $m3 add check -label "Square Endcaps" -variable XCWinOps(${window},endcaps) \
	-onvalue square -offvalue round -command {xcircuit::border \
	$XCWinOps($XCOps(focus),endcaps)}
   $m3 add check -label "Bounding Box" -variable XCWinOps(${window},bboxtype) \
	-onvalue true -offvalue false -command {xcircuit::border bbox \
	$XCWinOps($XCOps(focus),bboxtype)}
   $m3 add check -label "Clipmask" -variable XCWinOps(${window},clipmask) \
	-onvalue true -offvalue false -command {xcircuit::border clipmask \
	$XCWinOps($XCOps(focus),clipmask)}
   $m3 add check -label "Manhattan Draw" -variable XCWinOps(${window},manhattandraw) \
	-onvalue true -offvalue false -command {xcircuit::config manhattan \
	$XCWinOps($XCOps(focus),manhattandraw)}
   $m3 add check -label "Manhattan Edit" \
	-variable XCWinOps(${window},polyedittype) \
	-onvalue manhattan -offvalue normal \
	-command {xcircuit::config boxedit \
	$XCWinOps($XCOps(focus),polyedittype)}

   set m3 [menu $m2.fillmenu -tearoff 0]
   $m3 add radio -image img_stip100 -variable XCWinOps(${window},fillamount) \
	-value 100 -command {xcircuit::fill 100 opaque}
   $m3 add radio -image img_stip88 -variable XCWinOps(${window},fillamount) \
	-value 88 -command {xcircuit::fill 88 opaque}
   $m3 add radio -image img_stip75 -variable XCWinOps(${window},fillamount) \
	-value 75 -command {xcircuit::fill 75 opaque}
   $m3 add radio -image img_stip62 -variable XCWinOps(${window},fillamount) \
	-value 62 -command {xcircuit::fill 62 opaque}
   $m3 add radio -image img_stip50 -variable XCWinOps(${window},fillamount) \
	-value 50 -command {xcircuit::fill 50 opaque}
   $m3 add radio -image img_stip38 -variable XCWinOps(${window},fillamount) \
	-value 38 -command {xcircuit::fill 38 opaque}
   $m3 add radio -image img_stip25 -variable XCWinOps(${window},fillamount) \
	-value 25 -command {xcircuit::fill 25 opaque}
   $m3 add radio -image img_stip12 -variable XCWinOps(${window},fillamount) \
	-value 12 -command {xcircuit::fill 12 opaque}
   $m3 add radio -image img_stip0 -variable XCWinOps(${window},fillamount) \
	-value 0 -command {xcircuit::fill 0 transparent}
   $m3 add separator
   $m3 add radio -label "Opaque" -variable XCWinOps(${window},opaque) \
	-value true -command {xcircuit::fill opaque}
   $m3 add radio -label "Transparent" -variable XCWinOps(${window},opaque) \
	-value false -command {xcircuit::fill transparent}

   set m3 [menu $m2.colormenu -tearoff 0]
   $m3 add command -label "Add New Color" -command {xcircuit::picknewcolor}
   $m3 add separator
   $m3 add radio -label "Inherit Color" -variable XCWinOps(${window},colorval) \
	-value inherit -command {color set inherit}

   set m3 [menu $m2.parammenu -tearoff 0]
   $m3 add command -label "Manage Parameters" -command {xcircuit::prompteditparams}
   $m3 add separator
   $m3 add check -label "X Position" -variable XCWinOps(${window},xposparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),xposparam)} \
		{xcircuit::parameter make "x position"} \
		{xcircuit::parameter replace "x position"}}
   $m3 add check -label "Y Position" -variable XCWinOps(${window},yposparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),yposparam)} \
		{xcircuit::parameter make "y position"} \
		{xcircuit::parameter replace "y position"}}
   $m3 add check -label "Anchoring" -variable XCWinOps(${window},anchorparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),anchorparam)} \
		{xcircuit::parameter make anchoring} \
		{xcircuit::parameter replace anchoring}}
   $m3 add check -label "Rotation" -variable XCWinOps(${window},rotationparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),rotationparam)} \
		{xcircuit::parameter make rotation} \
		{xcircuit::parameter replace rotation}}
   $m3 add check -label "Style" -variable XCWinOps(${window},styleparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),styleparam)} \
		{xcircuit::parameter make style} \
		{xcircuit::parameter replace style}}
   $m3 add check -label "Scale" -variable XCWinOps(${window},scaleparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),scaleparam)} \
		{xcircuit::parameter make scale} \
		{xcircuit::parameter replace scale}}
   $m3 add check -label "Linewidth" -variable XCWinOps(${window},linewidthparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),linewidthparam)} \
		{xcircuit::parameter make linewidth} \
		{xcircuit::parameter replace linewidth}}
   $m3 add check -label "Color" -variable XCWinOps(${window},colorparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),colorparam)} \
		{xcircuit::parameter make color} \
		{xcircuit::parameter replace color}}
   $m3 add check -label "Start Angle" -variable XCWinOps(${window},startparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),startparam)} \
		{xcircuit::parameter make "start angle"} \
		{xcircuit::parameter replace "start angle"}}
   $m3 add check -label "End Angle" -variable XCWinOps(${window},endparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),endparam)} \
		{xcircuit::parameter make "end angle"} \
		{xcircuit::parameter replace "end angle"}}
   $m3 add check -label "Radius" -variable XCWinOps(${window},radiusparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),radiusparam)} \
		{xcircuit::parameter make radius} \
		{xcircuit::parameter replace radius}}
   $m3 add check -label "Minor Axis" -variable XCWinOps(${window},minorparam) \
	-onvalue true -offvalue false -command \
	{if {$XCWinOps($XCOps(focus),minorparam)} \
		{xcircuit::parameter make "minor axis"} \
		{xcircuit::parameter replace "minor axis"}}

   set m3 [menu $m2.polyeditmenu -tearoff 0]
   $m3 add radio -label "Manhattan Box Edit" \
	-variable XCWinOps(${window},polyedittype) \
	-value manhattan -command {xcircuit::config boxedit manhattan}
   $m3 add radio -label "Rhomboid X" -variable XCWinOps(${window},polyedittype) \
	-value rhomboidx -command {xcircuit::config boxedit rhomboidx}
   $m3 add radio -label "Rhomboid Y" -variable XCWinOps(${window},polyedittype) \
	-value rhomboidy -command {xcircuit::config boxedit rhomboidy}
   $m3 add radio -label "Rhomboid A" -variable XCWinOps(${window},polyedittype) \
	-value rhomboida -command {xcircuit::config boxedit rhomboida}
   $m3 add radio -label "Normal" -variable XCWinOps(${window},polyedittype) \
	-value normal -command {xcircuit::config boxedit normal}

   set m [menu ${window}.menubar.windowbutton.windowmenu -tearoff 0]
   $m add command -label "Zoom In" -command {zoom 1.5; refresh}
   $m add command -label "Zoom Out" -command {zoom [expr {1 / 1.5}]; refresh}
   $m add command -label "Pan" -command {$XCOps(focus).mainframe.toolbar.bp invoke}
   $m add command -label "Full View" -command {zoom view; refresh}
   $m add command -label "Refresh" -command {refresh}
   $m add separator
   $m add command -label "Library Directory" -command {xcircuit::library directory}
   $m add cascade -label "Goto Library" -menu $m.librarymenu
   $m add separator
   $m add command -label "Page Directory" -command {xcircuit::page directory}
   $m add cascade -label "Goto Page" -menu $m.pagemenu

   set m [menu ${window}.menubar.netlistbutton.netlistmenu -tearoff 0]
   $m add command -label "Make Pin" -command \
	{set XCWinOps($XCOps(focus),labeltype) "Pin Label"; set \
	XCWinOps($XCOps(focus),labelimage) img_tp; \
	$XCOps(focus).mainframe.toolbar.bt invoke}
   $m add command -label "Make Info Pin" -command \
	{set XCWinOps($XCOps(focus),labeltype) "Info Label"; set \
	XCWinOps($XCOps(focus),labelimage) img_ti; \
	$XCOps(focus).mainframe.toolbar.bt invoke}
   $m add command -label "Make Global Pin" -command \
	{set XCWinOps($XCOps(focus),labeltype) "Pin Global"; set \
	XCWinOps($XCOps(focus),labelimage) img_tg; \
	$XCOps(focus).mainframe.toolbar.bt invoke}
   $m add cascade -label "Convert Label To..." -menu $m.pinmenu
   $m add check -label "Pin Visibility" -variable XCWinOps(${window},pinvisible) \
	-onvalue true -offvalue false -command {xcircuit::label visible \
	$XCWinOps($XCOps(focus),pinvisible)}
   $m add check -label "Netlistable Instance" \
	-variable XCWinOps(${window},netlistable) \
	-onvalue true -offvalue false -command {xcircuit::instance netlist \
	$XCWinOps($XCOps(focus),netlistable)}
   $m add command -label "Make Matching Symbol" -command \
	{xcircuit::promptmakesymbol [page label]}
   $m add command -label "Associate With Symbol" -command \
	{xcircuit::symbol associate}
   $m add command -label "Highlight Connectivity" -command \
	{xcircuit::netlist highlight}
   $m add command -label "Auto-number Components" -command \
	{xcircuit::netlist autonumber}
   $m add command -label "Un-number Components" -command \
	{xcircuit::netlist autonumber -forget}
   $m add separator
   $m add check -label "SPICE .end statement" -variable XCOps(spiceend) \
	-onvalue true -offvalue false
   $m add check -label "Always regenerate netlists" -variable XCOps(forcenets) \
	-onvalue true -offvalue false
   $m add separator
   $m add command -label "Write SPICE netlist" -command \
	{if {$XCOps(forcenets)} {xcircuit::netlist update}; \
	 xcircuit::netlist write spice spc $XCOps(spiceend)}
   $m add command -label "Write flattened SPICE" -command \
	{if {$XCOps(forcenets)} {xcircuit::netlist update}; \
	xcircuit::netlist write flatspice fspc}
   $m add command -label "Write sim" -command \
	{if {$XCOps(forcenets)} {xcircuit::netlist update}; \
	xcircuit::netlist write flatsim sim}
   $m add command -label "Write pcb" -command \
	{if {$XCOps(forcenets)} {xcircuit::netlist update}; \
	xcircuit::netlist write pcb pcbnet}

   set m2 [menu $m.pinmenu -tearoff 0]
   $m2 add command -label "Normal label" -command {xcircuit::label type normal}
   $m2 add command -label "Local Pin" -command {xcircuit::label type pin}
   $m2 add command -label "Global Pin" -command {xcircuit::label type global}
   $m2 add command -label "Info label" -command {xcircuit::label type info}

   #---------------------------------------------------------------------------
   # Create the cloned menu links used by the toolbar
   #---------------------------------------------------------------------------

   ${window}.menubar.optionsbutton.optionsmenu.elementsmenu.parammenu \
		clone ${window}.parammenu
   ${window}.menubar.optionsbutton.optionsmenu.elementsmenu.colormenu \
		clone ${window}.colormenu
   ${window}.menubar.optionsbutton.optionsmenu.elementsmenu.bordermenu \
		clone ${window}.bordermenu
   ${window}.menubar.optionsbutton.optionsmenu.elementsmenu.fillmenu \
		clone ${window}.fillmenu

   .librarymenu clone ${window}.menubar.windowbutton.windowmenu.librarymenu
   .pagemenu clone ${window}.menubar.windowbutton.windowmenu.pagemenu
}

#-----------------------------------------------------------------
# Wrapper procedure to (re)bind a key to a Tcl procedure (Ed Casas 9/4/03)
# With no arguments, prints a list of bindings to stdout.  Key
# bindings should use "keyaction" to check for text mode, so that
# rebinding of keys does not prevent text entry.  Button bindings
# do not need this restriction.
#-----------------------------------------------------------------

proc xcircuit::keybind { {key {}} {proc {}} {window {}} } {
   global XCOps

   if { $window == {} } { set window $XCOps(focus).mainframe.mainarea.drawing }

   switch -glob -- $key {
      {} {
	puts stdout "XCircuit standard key bindings:"
	puts stdout "Key		Binding"
	puts stdout "-------------------------------------"
	set kpairs [xcircuit::bindkey]
	foreach i $kpairs {
	   set pkey [lindex $i 0]
	   set pval [lindex $i 1]
	puts stdout "$pkey		$pval"
	}
	puts stdout ""
      }
      <[Bb]utton-?> {
	bind ${window} $key $proc
      }
      default {
	bind ${window} $key "if \{!\[xcircuit::keyaction %k %s\]\} \{ $proc \}"
      }
   }
}

#-----------------------------------------------------------------
# Enable mouse hints in the window
#-----------------------------------------------------------------

# James Vernon's mouse button hints

if {[catch {source $XCIRCUIT_SRC_DIR/mousehint.tcl}]} {
   set XCOps(mousehints) -1
} else {
   set XCOps(mousehints) 0
}

proc xcircuit::enable_mousehints {} {
   global XCOps

   if {$XCOps(mousehints) == 0} {
      set XCOps(mousehints) 1
      foreach window [config windownames] {
         set frame [winfo top $window]
         xcircuit::mousehint_create $frame
      }
   }
}

#-----------------------------------------------------------------
# Final setup stuff before exiting back to interpreter
#-----------------------------------------------------------------

# This gets rid of the original "wish", in favor of our own window

if {[string range [wm title .] 0 3] == "wish"} {
   wm withdraw .
}

#----------------------------------------------------------------------
# Library and Page menus (common to all windows)
#----------------------------------------------------------------------

menu .librarymenu -tearoff 0
.librarymenu add command -label "New Library Page" -command \
	{xcircuit::promptaddlibrary}
.librarymenu add separator

menu .pagemenu -tearoff 0
.pagemenu add command -label "Add New Page" -command {xcircuit::page make}
.pagemenu add separator

#----------------------------------------------------------------------
# Source other Tcl scripts, if they exist in the $XCIRCUIT_SRC_DIR path
# and add the capabilities to the GUI.
#----------------------------------------------------------------------

# "Write All" feature

catch {source $XCIRCUIT_SRC_DIR/files.tcl}

# Library manager widget

catch {source $XCIRCUIT_SRC_DIR/library.tcl}

# Fancy "Make Matching Symbol" feature

catch {source $XCIRCUIT_SRC_DIR/symbol.tcl}

# Help window

catch {source $XCIRCUIT_SRC_DIR/xchelp.tcl}

# EDIF file parser

catch {source $XCIRCUIT_SRC_DIR/edif.tcl}

# Synopsys symbol library file parser

catch {source $XCIRCUIT_SRC_DIR/synopsys.tcl}

# System Clipboard paste into labels

catch {source $XCIRCUIT_SRC_DIR/selection.tcl}

# Regexp label substitutions and auto-increment feature

catch {source $XCIRCUIT_SRC_DIR/text.tcl}

# Wim Vereecken's Matlab PostScript import function

catch {source $XCIRCUIT_SRC_DIR/matgen.tcl}

#----------------------------------------------------------------------
# Create the initial window.
#----------------------------------------------------------------------

xcircuit::new_window $XCOps(focus)

#----------------------------------------------------------------------
# Add buttons for the pre-allocated pages
#----------------------------------------------------------------------

xcircuit::newpagebutton "Page 1" 1
xcircuit::newpagebutton "Page 2" 2
xcircuit::newpagebutton "Page 3" 3
xcircuit::newpagebutton "Page 4" 4
xcircuit::newpagebutton "Page 5" 5
xcircuit::newpagebutton "Page 6" 6
xcircuit::newpagebutton "Page 7" 7
xcircuit::newpagebutton "Page 8" 8
xcircuit::newpagebutton "Page 9" 9
xcircuit::newpagebutton "Page 10" 10

#----------------------------------------------------------------------
# Add buttons for the pre-allocated libraries
#----------------------------------------------------------------------

xcircuit::newlibrarybutton "User Library"

#-----------------------------------------------------------------
# New key bindings should pass through this function so that key
# strokes are captured correctly for label editing.
#----------------------------------------------------------------------

proc xcircuit::keyaction {keycode {keystate 0}} {
   switch -- [eventmode] {
      text -
      etext -
      cattext {
	 standardaction $keycode down $keystate
	 return true
      }
   }
   return false
}

#-----------------------------------------------------------------

proc scrollboth { lists args } {
   foreach l $lists {
      eval {$l yview} $args
   }
}

#-----------------------------------------------------------------
# Procedure to generate the help window
#-----------------------------------------------------------------

proc xcircuit::makehelpwindow {} {
   toplevel .help -bg beige
   wm group .help .
   wm withdraw .help

   frame .help.title -bg beige
   frame .help.listwin

   pack .help.title -side top -fill x
   pack .help.listwin -side top -fill both -expand true

   label .help.title.field -text "XCircuit Help" -bg beige
   button .help.title.dbut -text "Dismiss" -bg beige -command {wm withdraw .help}
   pack .help.title.field -side left -padx 10
   pack .help.title.dbut -side right -ipadx 10

   listbox .help.listwin.func -yscrollcommand ".help.listwin.sb set" \
	-setgrid 1 -height 20
   listbox .help.listwin.keys -yscrollcommand ".help.listwin.sb set" \
	-setgrid 1 -height 20
   scrollbar .help.listwin.sb -orient vertical -command \
	[list scrollboth [list .help.listwin.func .help.listwin.keys]]
   message .help.listwin.win -width 200 -justify left -anchor n \
	-relief groove -text "Click on a function for help text"

   # Keep boxes aligned!
   bind .help.listwin.keys <Button-4> {xcircuit::helpscroll -1}
   bind .help.listwin.keys <Button-5> {xcircuit::helpscroll 1}
   bind .help.listwin.func <Button-4> {xcircuit::helpscroll -1}
   bind .help.listwin.func <Button-5> {xcircuit::helpscroll 1}
   # Also bind to the mouse wheel (Windows-specific, generally)
   bind .help.listwin.keys <MouseWheel> {xcircuit::helpscroll %D}
   bind .help.listwin.func <MouseWheel> {xcircuit::helpscroll %D}

   grid .help.listwin.func -row 0 -column 0 -sticky news -padx 1 -pady 1
   grid .help.listwin.keys -row 0 -column 1 -sticky news -padx 1 -pady 1
   grid .help.listwin.sb -row 0 -column 2 -sticky ns -padx 1 -pady 1
   grid .help.listwin.win -row 0 -column 3 -sticky news -padx 1 -pady 1

   grid columnconfigure .help.listwin 1 -weight 1 -minsize 100
   grid rowconfigure .help.listwin 0 -weight 1 -minsize 100

   bind .help.listwin.func <ButtonRelease-1> "xcircuit::printhelp"
}

#-----------------------------------------------------------------
# Scroll all listboxes in the .help.listwin window at the same
# time, in reponse to any one of them receiving a scroll event.
#-----------------------------------------------------------------

proc xcircuit::helpscroll {value} {
   global tcl_platform
   set idx [.help.listwin.func nearest 0]

   if {$tcl_platform(platform) == "windows"} {
      set idx [expr {$idx + $value / 120}]
   } else {
      set idx [expr {$idx + $value}]
   }

   .help.listwin.func yview $idx
   .help.listwin.keys yview $idx

   # Important!  This prohibits the default binding actions.
   return -code break
}

#-----------------------------------------------------------------
# Procedure to update and display the help window
#-----------------------------------------------------------------

proc xcircuit::helpwindow {} {

   # Create the help window if it doesn't exist
   if {[catch {wm state .help}]} {
      xcircuit::makehelpwindow
   }
   set wstate [xcircuit::getinitstate .help]

   .help.listwin.func delete 0 end
   .help.listwin.keys delete 0 end

   set k [lsort -dictionary [xcircuit::bindkey]]

   .help.listwin.func insert end "Function"
   .help.listwin.keys insert end "Keys"
   .help.listwin.func insert end ""
   .help.listwin.keys insert end ""

   foreach i $k {
      set pkeys [xcircuit::bindkey -func $i]
      .help.listwin.func insert end "$i"
      .help.listwin.keys insert end "$pkeys"
   }

   if {"$wstate" != "normal"} { 
      wm deiconify .help
      xcircuit::centerwin .help
   }
   raise .help
}

#-----------------------------------------------------------------
# Prevent "Tab" from removing focus from the window during text edits,
# but allow it to take its normal meaning at other times.
#-----------------------------------------------------------------

bind all <Tab> { 
   switch -- [eventmode] {                        
      text -
      etext -
      cattext {} 
      default {tk::TabToWindow [tk_focusNext %W]}
   }  
}  

bind all <<PrevWindow>> {
   switch -- [eventmode] {
      text -
      etext -
      cattext {}
      default {tk::TabToWindow [tk_focusPrev %W]}
   }
}

#-----------------------------------------------------------------
# Wait for the drawing area to become visible, and set the focus on it.
# Invoke the "wire" button so we have a default button1 action.
#-----------------------------------------------------------------

tkwait visibility $XCOps(focus).mainframe.mainarea.drawing
focus -force $XCOps(focus).mainframe.mainarea.drawing

#-----------------------------------------------------------------
# This pops down the interpreter window, if the "console.tcl" script was run.
#-----------------------------------------------------------------

catch xcircuit::consoledown

#-----------------------------------------------------------------
# End of GUI configuration file.  Xcircuit continues to load the xcircuit
# startup configuration files.
#-----------------------------------------------------------------
