## boot xcircuit within a starkit (replaces bin/xcircuit)

package provide app-xcircuit 3.6.36

# define the source code location
set XCIRCUIT_SRC_DIR [file join $::starkit::topdir lib app-xcircuit]
set env(XCIRCUIT_SRC_DIR) $XCIRCUIT_SRC_DIR

# define the library location (outside the starkit)
#set loclibdir [file join [file dirname $::starkit::topdir] xclib]
set loclibdir [file join $env(HOME) xclib]
#set XCIRCUIT_LIB_DIR [file join $loclibdir app-xcircuit]
set XCIRCUIT_LIB_DIR $loclibdir
set env(XCIRCUIT_LIB_DIR) $XCIRCUIT_LIB_DIR

# create the library if necessary (usually just the first time)
if {![file exists $loclibdir]} {
  file mkdir $loclibdir
  catch {unset libfiles}
  # directories
  lappend libfiles [file join $XCIRCUIT_SRC_DIR app-defaults]
  lappend libfiles [file join $XCIRCUIT_SRC_DIR fonts]
  lappend libfiles [file join $XCIRCUIT_SRC_DIR pixmaps]
  # individual files
  lappend libfiles [file join $XCIRCUIT_SRC_DIR xcircps2.pro]
  foreach lf [glob -nocomplain -- [file join $XCIRCUIT_SRC_DIR *.lps]] {
    lappend libfiles $lf
  }
  lappend libfiles [file join $XCIRCUIT_SRC_DIR xcstartup.tcl]
  #
  foreach lf $libfiles {
    file copy $lf $loclibdir
  }
  unset libfiles
}

# start xcircuit thru tkcon w/o args
source [file join $XCIRCUIT_SRC_DIR tkcon.tcl]
tkcon main source [file join $XCIRCUIT_SRC_DIR console.tcl]
tkcon slave slave package require Tk
tkcon slave slave source [file join $XCIRCUIT_SRC_DIR xcircuit.tcl]
tkcon slave slave source [file join $XCIRCUIT_SRC_DIR xcstartup.tcl]
