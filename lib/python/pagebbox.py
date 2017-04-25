# pagebbox.py
#-----------------------------------------------------------
# Python script which creates a function "pagebbox(width, height)"
# that generates a bounding box of the specified width and height.
#
# Execute this script using menu option "File/Execute Script",
# if Python has been compiled in.
#
# Add key 'B' for popuppromt asking for page size.  Add button to
# "Edit" menu for same.  Add Ctrl-B and Ctrl-Shift-B for standard
# 8 1/2 x 11 and 11 x 17 page bounding boxes.
#-----------------------------------------------------------

def newbox(lx, ly, ux, uy):
   h1=newelement("Polygon")
   plist = [(lx, ly), (ux, ly), (ux, uy), (lx, uy)]
   d = {"points": plist}
   setattr(h1, d)
   return h1

def pagebbox(w, h):
   h1 = newbox(0, 0, w, h)
   d = {"style": 512, "color": (255, 165, 0)}
   setattr(h1, d)
   return h1

def intbbox(s):
   slist = s.split()
   slen = len(slist)
   if slen > 1:
      w = eval(slist[0])
   else:
      w = 8.5

   if slen == 3:
      h = eval(slist[2])
   elif slen == 2:
      h = eval(slist[1])
   else:
      h = 11

   # note to self:  should look at coord style, set default for A4 size
   # if style is CM, not INCHES.

   p1 = getpage()
   oscale = p1['output scale']

   inchsc = 192 
   h1 = pagebbox(w * oscale * inchsc, h * oscale * inchsc)
   return h1

def promptbbox():
   popupprompt('Enter width x height:', 'intbbox')

def stdbbox():
   intbbox("8.5 x 11")

def bigbbox():
   intbbox("11 x 17")

bind('B', 'promptbbox')
bind('Control_b', 'stdbbox')
bind('Control_Shift_B', 'bigbbox')

newbutton('Edit', 'New BBox (B)', 'promptbbox')
# newtool('BBox', 'promptbbox', '', 'bounding box');

#-----------------------------------------------------------
