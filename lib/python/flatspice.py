# flatspice.py
#-----------------------------------------------------------
# Python script which writes a SPICE-format netlist.
# Replaces the code formerly in "netlist.c" (deprecated).
# Python scripting is now the preferred method for handling
# netlist output formats.
#-----------------------------------------------------------

# Select the device string corresponding to the given prefix.
# Return the body of the string, or an empty string if the prefix doesn't match.

def select(sstr, prefix):
   ltext = ''
   if sstr.startswith(prefix):
      ltext += sstr[len(prefix) + 1:]
   return ltext
	 
# Generate an ASCII string from an xcircuit string (list)

def textprint(slist, params):
   ltext = ''
   is_symbol = 0
   is_iso = 0
   for x in slist:
      try:
	f = x.keys()[0]
      except AttributeError:		# must be a string
	if x == 'Return':
	   ltext += '\n'
	elif x == 'Underline':
	   ltext += '_'
	elif x == 'Overline':
	   ltext += '!'
      else:			# is a dictionary; will have only one key
	if f == 'Font':
	   lfont = x[x.keys()[0]]
	   if lfont.startswith('Symbol'):
	      is_symbol = 1
	   else:
	      is_symbol = 0
	   if lfont.endswith('ISO'):
	      is_iso = 1
	   else:
	      is_iso = 0
	elif f == 'Parameter':
	   ltext += textprint(params[x[x.keys()[0]]], [])
	else:				# text:  SPICE translates "mu" to "u"
	   for y in x[x.keys()[0]]:
	      if is_symbol:
		 if y == 'f':
		    ltext += 'phi'
		 elif y == 'm':
		    ltext += 'u'
		 else:
		    ltext += y
	      else:
		 if ord(y) == 181:
		    ltext += 'u'
		 elif ord(y) > 127:
		    ltext += '/' + str(ord(y))
		 else:
		    ltext += y
   return ltext
	

# Flatten the netlist and write to the output

def recurseflat(outfile, ckt, clist):
   try:
      v = ckt['calls']		# calls to subcircuits
   except KeyError:		# A bottom-level circuit element
      pass
   else:
      for y in v:
	 for z in clist:
	    if z['name'] == y['name']:
	       # copy the object and substitute net names into subcircuit ports
	       lobj = z
	       lobj['ports'] = y['ports']
	       recurseflat(outfile, lobj, clist)
	       break;
   try:
      w = ckt['devices']
   except KeyError:
      pass
   else:
      for y in w:
	 for u in y:
	    lstr = select(textprint(u, []), 'spice')
	    if lstr <> '':
	       outfile.write('device: ' + lstr + '\n')

# Top of the flattened-circuit writing routine

def writespiceflat():
   p=netlist()
   g=p['globals']
   c=p['circuit']
   l=len(c)
   top=c[l-1]
   topname=top['name']
   topname += '.spc'
   try:
      outfile=open(topname, 'w')
   except IOError:
      return

   # print header line

   outfile.write('*SPICE flattened circuit "' + topname + '"')
   outfile.write(' from XCircuit v' + str(xc_version))
   outfile.write(' (Python script "flatspice.py")\n')

   # print global variables

   for x in g:	# 'globals' is a list of strings
      outfile.write('.GLOBAL ' + textprint(x, []) + '\n')
   outfile.write('\n')

   recurseflat(outfile, top, c)
   outfile.write('.end\n')
   outfile.close()

# Key binding and menu button for the spice netlist output
# bind('Alt_F', 'writespiceflat')
newbutton('Netlist', 'Write Flattened Spice', 'writespiceflat')

