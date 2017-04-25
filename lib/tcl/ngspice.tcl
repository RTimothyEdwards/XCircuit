# ngspice.tcl
#
# This script creates the active loop for the background ngspice
# simulator, and creates buttons for various simulator controls.
#
# Labels wishing to access spice variables can use, e.g.:
# spice send "print TIME\[$STEP\]"

proc ngsim {} {
   if {![catch {spice break}]} {
      refresh;
      spice resume
      after 500 ngsim	;# update at 1/2 second intervals
   }
}

proc startspice {} {
   spice start
   spice run
   after 500 ngsim
}

proc stopspice {} {
   after cancel ngsim
   spice break;
   refresh;
}
