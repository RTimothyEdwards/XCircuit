# panzoom.tcl --- rebinds keys Z and z for a zoom style in which
#		  the page is centered on the cursor position
#		  (provded by Ed Casas, 9/4/03)

xcircuit::keybind Z { pan here ; zoom in }
xcircuit::keybind z { pan here ; zoom out }
