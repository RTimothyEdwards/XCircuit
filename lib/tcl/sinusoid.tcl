proc sinusoid {a b {c 1.0}} {
   set mylist {}
   for {set t 0} {$t < [expr int(50.0 * $c)]} {incr t} {
      set x [expr int($b * $t / 50.0)]
      set y [expr int($a * sin(2 * 3.1415926 * $t / 50.0))]
      lappend mylist [list $x $y]
   }
   set handle [eval polygon make [llength $mylist] $mylist]
   refresh
   return $handle
}
