#!/usr/local/bin/wish8.5
#
# Usage:  xc_remote <filename>
#
# This script attempts to communicate with an existing xcircuit
# application.  If none exists, it starts a new xcircuit process.
# If one exists, then it forces xcircuit to create a new window,
# and the file <filename> is loaded into it as a new page.

exec xhost -
foreach host [lrange [split [exec xhost] \n] 1 end] {
   exec xhost -$host
}
set appname "tkcon.tcl #2"
if {[catch {send $appname xcircuit::forkwindow}]} {
   exec xcircuit [lindex $argv 0] &
} else {
   send $appname {config focus [lindex [config windownames] 0]}
   set npages [send $appname page links total]
   incr npages
   send $appname page $npages goto -force
   send $appname page load [lindex $argv 0]
}
exit
