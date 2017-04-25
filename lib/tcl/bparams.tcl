#--------------------------------------------------------------------------
# bparams.tcl
#
# This Tcl script re-binds Button-3 to bring up the parameter edit
# popup window when the right mouse button is clicked over a (non-selected)
# object instance.  Otherwise, Button-3 behaves as usual (e.g., deselects
# selected objects).
#--------------------------------------------------------------------------

bind .xcircuit.mainframe.mainarea.drawing <ButtonPress-3> { \
   if {[eventmode] == "normal" && [select get] == {} \
                && [select here] != {}} {
      xcircuit::prompteditparams
   } else {
      standardaction %b down %s
   }
}
