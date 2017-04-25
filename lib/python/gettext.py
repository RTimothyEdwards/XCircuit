# gettext.py
#-----------------------------------------------------------
# Python script which creates a function "gettext(filename)"
# that reads an ASCII file and turns the contents into a
# text label.
# Execute this script using menu option "File/Execute Script",
# if Python has been compiled in.
#
# Also:  defines a function which can be called with a key
# macro (gets the filename via popup prompt) and binds it
# to key "^G" (ctrl-"g"), and defines a button named "Get Text"
# under the "Edit" menu which does the same.
#-----------------------------------------------------------

def newlabel(x, y, l):
   h1=newelement("Label")
   d = {"scale": 1.0, "rotation": 0, "justify": 0, "pin": 0,
	"position": (x, y), "string": l}
   setattr(h1, d)
   return h1

def gettext(f):
   try:
      fi = open(f, 'r')
   except IOError:
      return
   else:
      T = getcursor();
      D = {'Font': 'Times-Roman'}
      S2 = [D]
      S = fi.readlines()
      for X in S:
         Y = X[0:len(X)-1]
	 D = {'Text': Y}
         S2.append(D)
         S2.append('Return')
      h1 = newlabel(T[0], T[1], S2)
      return h1

def prompttext():
   filepopup('Enter filename to import:', 'gettext')

bind('Control_G', 'prompttext')
newbutton('Edit', 'Get Text (^G)', 'prompttext')

#-----------------------------------------------------------
