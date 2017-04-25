# loadlgf.py
#-----------------------------------------------------------
# Python script which creates a function "loadlgf" that
# replaces the code formerly in "formats.c" (deprecated).
# Python scripting is now the preferred method for handling
# alternate file formats.
#-----------------------------------------------------------

def loadlgf(f):
   try:
      fi = open(f, 'r')
   except IOError:
      return
   else:

      # check magic cookie to see if it's a real LGF file

      S = fi.readline()
      if (S <> '-5\n'):
	 return

      S = fi.readline()
      if (S <> 'f s\n'):
	 return

      # Now go load the LGF library (required)
      # '-1' loads at the end of the current library pages

      library('lgf.lps', -1)

      # clear the page

      reset()

      # read in the file

      S = fi.readlines()
      for X in S:
	 if (S[0] == '#'):
	 elif (S[0] == 'n'):
	 elif (S[0] == 's'):
	 elif (S[0] == 'l'):
	 elif (S[0] == 'w'):
	 elif (S[0] == 'p'):
	 elif (S[0] == 'b'):
	 elif (S[0] == 'g'):
	 elif (S[0] == 'h'):
	 elif (S[0] == '.'):
	 else:

      h1 = getpage();
      return h1

def promptlgf():
   filepopup('Enter filename to load:', 'loadlgf')

bind('Control_l', 'promptlgf')
newbutton('Edit', 'Load LGF File (^L)', 'promptlgf')

#-----------------------------------------------------------
