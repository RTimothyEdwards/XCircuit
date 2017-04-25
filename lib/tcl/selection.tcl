# selection.tcl --- rebinds middle mouse button in text mode
#		  to implement a clipboard paste.

xcircuit::keybind <Button-2> { if {[eventmode] == "text" || \
	[eventmode] == "etext"} {label insert text "[selection get]"} else { \
	standardaction %b down %s}}
