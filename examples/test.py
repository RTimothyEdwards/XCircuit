#-----------------------------------------------------------
# Test of the python interpreter and use of animation in
# xcircuit. Execute this script using menu option
# "File/Execute Script", if Python has been compiled in.
#-----------------------------------------------------------

from math import pi,sin,cos

def move(h1, x, y):
   d = {"position": (x, y)}
   setattr(h1, d) 

def newarc(x, y, r):
   h1=newelement("Arc")
   d = {"radius": r, "minor axis": r, "position": (x, y)}
   setattr(h1, d)
   return h1

x = y = 0
x2 = y2 = 0
bigrx = 400
bigry = 200
nsteps = 200
step = 2 * pi / (nsteps - 1) 

set("grid","off")
set("axis","off")
set("snap","off")

h1 = newarc(x, y, 100)
h2 = newarc(x2, y2, 85)

pause(0.5)
for i in range(0,nsteps):
   x2 = x
   y2 = y
   x = int(round(bigrx * sin(i * step)))
   y = int(round(bigry * cos(i * step)))
   move(h1, x, y)
   move(h2, x2, y2)
#  pause(0.01)
   refresh();

set("grid","on")
set("axis","on")
set("snap","on")

#-----------------------------------------------------------
