#-----------------------------------------#
# xcircuitrc file for editing sheet music #
# (python version)			  #
# Rename this file to .xcircuitrc	  #
#-----------------------------------------#

# 1) Don't load any of the default libraries.
#    Instead, load only "musiclib".
override("library")
library("/usr/local/lib/xcircuit-3.6/musiclib")

# 2) Load font Times-Roman and set the default font to Times-RomanISO
font("Times-Roman")
set("font", "Times-RomanISO")

# 3) Set various parameters for music editing
set("boxedit", "rhomboid-y")
set("colorscheme", "inverse")
set("xschema", "off")

# 4) Set Backspace = Delete
unbind("BackSpace", "Text Left")
bind("BackSpace", "Text Delete")
