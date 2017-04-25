# XCircuit startup script for Tcl/Tk version
# Does the work previously handled by "builtins.lps".  Commands
# "loadlibrary" and "loadfontencoding" should no longer be used.
# Update, version 3.3.34:  Commands "library override" and
# "font override" should not be used; set XCOps(fontoverride)
# and XCOps(liboverride) instead.
#
#  Written by Tim Edwards 12/19/00, 6/24/02 (tim@bach.ece.jhu.edu)
#  The Johns Hopkins University

global XCOps

#
# Look for a file "site.tcl" in the scripts directory.  If it's there,
# execute it instead of this file.  "site.tcl" must NEVER be overwritten
# by any installation procedure.
#

if {![catch {source ${XCIRCUIT_SRC_DIR}/site.tcl}]} {return}

if {[catch {set XCOps(fontoverride)}]} {

  loadfont times_roman.xfe
  loadfont times_romaniso.xfe
  loadfont helvetica.xfe
  loadfont helveticaiso.xfe
  loadfont courier.xfe
  loadfont courieriso.xfe
  loadfont symbol.xfe

  # Alternate font encodings known to xcircuit

  loadfont times_romaniso2.xfe
  loadfont courieriso2.xfe
  loadfont helveticaiso2.xfe
  loadfont times_romaniso5.xfe
  loadfont courieriso5.xfe
  loadfont helveticaiso5.xfe
  loadfont times_roman_cyrillic.xfe
  loadfont courier_cyrillic.xfe
  loadfont helvetica_cyrillic.xfe
}

if {[catch {set XCOps(liboverride)}]} {

# Create library pages
  library make Generic

# First library page
  library 1 load generic.lps
  library 1 load analog.lps
  library 1 load avlsi.lps
  library 1 load digital.lps
  library 1 load digitaltcl.lps

# Second library page
  library make AnalogLib
  library 2 load analoglib3.lps

# Third library page
# library make LeadFrame
# library 3 load ic_templates.lps

# Fourth library page
# library make 74LSXX
# library 4 load quadparts.lps
}
