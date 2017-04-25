#---------------------------------------------------------------
# Text regexp manipulation and text file import
#
# The core routine text_regexp takes either the selected
# items or all items, searches them for text labels, and
# makes the text replacement according to the regular
# expression "searchstr" and the replacement text "replacestr".
#
#---------------------------------------------------------------

# Declare the package by setting variable XCOps(module,text)
set XCOps(module,text) 1

proc text_regexp {searchstr replacestr {mode all}} {
   switch -glob -- $mode {
      select* {set partlist [select get]; set resel 1}
      default {deselect selected; set partlist [object parts]; set resel 0}
   }
   config suspend true
   undo series start
   foreach j $partlist {
      if {[element $j type] == "Label"} {
	 set labelparts [lindex [label $j list] 0]
	 set newparts {}
         set modified 0
	 foreach k $labelparts {
	    if {[lindex $k 0] == "Text"} {
	       set tstring [lindex $k 1]
	       set newt [regsub [list $searchstr] $tstring $replacestr]
	       set newk [lreplace $k 1 1 $newt]
	       lappend newparts $newk
	       if {"$newt" != "$tstring"} {set modified 1}
	    } else {
	       lappend newparts $k
	    }
	 }
	 if {$modified == 1} {
	    label $j replace $newparts
	 }
      }
   }
   if {$resel == 1} {catch {select $partlist}}
   undo series end
   refresh
   config suspend false
}

#-----------------------------------------------------------------
# Procedures to help with label generation (label autoincrement)
#-----------------------------------------------------------------

# autoincrement the first number found in the text of the indicated
# label(s).  Use argument "amount" to decrement, or increment by 10,
# etc.
#
# example:   xcircuit::textincrement selected

proc xcircuit::textincrement {mode {amount 1} {position first}} {
   switch -glob -- $mode {
      select* {set handle [select get]; set resel 1}
      default {deselect selected; set handle [object parts]; set resel 0}
   }
   config suspend true
   undo series start
   
   foreach h $handle {
      if {[element $h type] == "Label"} {
         set tlist [join [label $h list]]
         set tlen [llength $tlist]
         for {set i 0} {$i < $tlen} {incr i} {
	    set t [lindex $tlist $i]
            set esc [lindex $t 0]
            if {$esc == "Text"} {
	       set ltext [lindex $t 1]
	       set idx 0
	       if {"$position" == "last"} {
	          set result [regexp -indices -all {([+-]?)[0]*[[:digit:]]+} \
				$ltext lmatch bounds]
		  set idx [lindex $bounds 0]
		  if {$result > 0} {
	             regexp -start $idx {([+-]?)([0]*)([[:digit:]]+)} $ltext \
				lmatch pre zer num
	          }
	       } else {
	          set result [regexp {([+-]?)([0]*)([[:digit:]]+)} $ltext \
				lmatch pre zer num]
	       }
		  
	       if {$result > 0 && $num != ""} {
		  set num $pre$num
	          incr num $amount
		  if {$num < 0} {
		     set num [expr abs($num)]
		     set pre "-"
		  }
		  if {$num == 0 && "$pre" == "-"} {set pre ""}
		  if {"$position" == "last"} {
	             regsub -start $idx {[+-]?[0]*[[:digit:]]+} $ltext $pre$zer$num ltext
		  } else {
	             regsub {[+-]?[0]*[[:digit:]]+} $ltext $pre$zer$num ltext
		  }
	          set t [lreplace $t 1 1 $ltext]
	          set tlist [lreplace $tlist $i $i $t]
	          label $h replace $tlist
	          break
	       }
	    }
	 }
      }
   }
   if {$resel == 1} {catch {select $handle}}
   undo series end
   refresh
   config suspend false
}

proc xcircuit::autoincr {{value 1} {position first}} {
   set e [eventmode]
   set nopreselect 0
   if {$e != "text" && $e != "etext" && $e != "epoly"} {
      if {[select] == 0} {
	 set nopreselect 1
	 undo series start
	 select here
      }
      if {[select] > 0} {
         xcircuit::textincrement selected $value $position
	 if {$nopreselect} {
	    deselect
	    undo series end
            refresh
	 }
      } else {
	 if {$nopreselect} {
	    undo series end
	 }
	 error "no selection"
      }
   } else {
      error "no auto-incr in text mode"
   }
}

#-----------------------------------------------------------------
# Create a popup window for text modification
#-----------------------------------------------------------------

proc xcircuit::maketextmod {} {
   toplevel .textmod -bg beige
   wm withdraw .textmod

   frame .textmod.title -bg beige
   label .textmod.title.field -text "Label text modification for:" -bg beige
   menubutton .textmod.title.type -text "selected text" -bg beige \
	-menu .textmod.title.type.seltype
   button .textmod.title.dbut -text "Dismiss" -bg beige -command \
	{wm withdraw .textmod}

   menu .textmod.title.type.seltype -tearoff 0
   .textmod.title.type.seltype add command -label "selected text" -command \
	{.textmod.title.type configure -text "selected text"}
   .textmod.title.type.seltype add command -label "all text in page" -command \
	{.textmod.title.type configure -text "all text in page"}


   pack .textmod.title -side top -fill x
   pack .textmod.title.field -side left -padx 10
   pack .textmod.title.type -side left -padx 10
   pack .textmod.title.dbut -side right -ipadx 10

   labelframe .textmod.replace -text "Search and Replace" -bg beige
   pack .textmod.replace -side top -fill x -expand true -pady 10

   label .textmod.replace.title1 -text "Search for:" -bg beige
   entry .textmod.replace.original -bg white
   label .textmod.replace.title2 -text "Replace with:" -bg beige
   entry .textmod.replace.new -bg white
   button .textmod.replace.apply -text "Apply" -bg beige -command \
	{text_regexp [.textmod.replace.original get] \
	[.textmod.replace.new get] [.textmod.title.type cget -text]}

   pack .textmod.replace.title1 -side left
   pack .textmod.replace.original -side left
   pack .textmod.replace.title2 -side left
   pack .textmod.replace.new -side left
   pack .textmod.replace.apply -side right -ipadx 10 -padx 10

   # Numeric Increment/Decrement

   labelframe .textmod.numeric -text "Embedded Numbers" -bg beige
   pack .textmod.numeric -side top -fill x -expand true

   button .textmod.numeric.incr -text "Increment" -bg beige -command \
        {xcircuit::textincrement [.textmod.title.type cget -text] \
	[.textmod.numeric.amount get] [.textmod.numeric.pos cget -text]}
   button .textmod.numeric.decr -text "Decrement" -bg beige -command \
        {xcircuit::textincrement [.textmod.title.type cget -text] \
	[expr -[.textmod.numeric.amount get]] [.textmod.numeric.pos cget -text]}
   label .textmod.numeric.title1 -text "Amount: " -bg beige
   entry .textmod.numeric.amount -bg white

   menubutton .textmod.numeric.pos -text "first" -bg beige \
	-menu .textmod.numeric.pos.posmenu
   menu .textmod.numeric.pos.posmenu -tearoff 0
   .textmod.numeric.pos.posmenu add command -label "first" -command \
	{.textmod.numeric.pos configure -text "first"}
   .textmod.numeric.pos.posmenu add command -label "last" -command \
	{.textmod.numeric.pos configure -text "last"}

   .textmod.numeric.amount insert 0 "1"

   pack .textmod.numeric.incr -side left -ipadx 10 -padx 10
   pack .textmod.numeric.decr -side left -ipadx 10 -padx 10
   pack .textmod.numeric.title1 -side left
   pack .textmod.numeric.amount -side left
   pack .textmod.numeric.pos -side left
}

proc xcircuit::textmod {} {

   if {[catch {wm state .textmod}]} {
      xcircuit::maketextmod
   }
   set wstate [xcircuit::getinitstate .textmod]

   # setup goes here

   if {"$wstate" != "normal"} {
      wm deiconify .textmod
      xcircuit::centerwin .textmod
   }
   raise .textmod
}

#--------------------------------------------------------------
# This procedure reads an entire file into an xcircuit string.
#--------------------------------------------------------------

proc xcircuit::importtext {filename} {
   set ffile [open $filename]
   set tlist {}
   if {[gets $ffile line] >= 0} {
      set line [string map {\t "        "} $line]
      lappend tlist [list Text $line]
      while {[gets $ffile line] >= 0} {
         set line [string map {\t "        "} $line]
	 lappend tlist Return
         lappend tlist [list Text $line]
      }
   }
   label make normal $tlist "0 0"
   close $ffile
   refresh
}

#-----------------------------------------------------------------

proc xcircuit::promptimporttext {} {
   .filelist.bbar.okay configure -command \
	{xcircuit::importtext [.filelist.textent.txt get]; \
	wm withdraw .filelist}
   .filelist.listwin.win configure -data ""
   .filelist.textent.title.field configure -text "Select text file:"
   .filelist.textent.txt delete 0 end
   xcircuit::popupfilelist
}

#-----------------------------------------------------------------
