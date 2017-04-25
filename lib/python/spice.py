# spice.py
#-----------------------------------------------------------------------
# Python script which writes a SPICE-format netlist.
# Replaces the code formerly in "netlist.c" (deprecated).
# Python scripting is now the preferred method for handling
# netlist output formats.
#-----------------------------------------------------------------------

import string

#-----------------------------------------------------------------------
# Parse the output string, substituting for index, ports, parameters,
# and automatic numbering using the '?' parameter.
#-----------------------------------------------------------------------

def parsedev(lstr, portdict, devdict, classidx, paramdict):
   rstr = lstr
   ltext = ''
   classname = rstr[0]		# single-character SPICE element type
   try:
      index = classidx[classname]
   except KeyError:
      index = 1
      classidx[classname] = 1

   if string.find(rstr, '%i'):
      rstr = string.replace(rstr, '%i', str(index))
      classidx[classname] = classidx[classname] + 1

   for pt in portdict.keys():
      ptstr = '%p' + pt
      rstr = string.replace(rstr, ptstr, portdict[pt])
      ptstr = '%p' + '"' + pt + '"'
      rstr = string.replace(rstr, ptstr, portdict[pt])

   for vt in paramdict.keys():
      vtvalue = paramdict[vt]
      if vtvalue == '?':
	 vtvalue = str(index);
	 classidx[classname] = classidx[classname] + 1
     
      vtstr = '%v' + vt
      rstr = string.replace(rstr, vtstr, vtvalue)
      vtstr = '%v' + '"' + vt + '"'
      rstr = string.replace(rstr, vtstr, vtvalue)

   # Simple replacements (%%, %r, %t) (all occurrences)
   #...................................................
   if string.find(rstr, '%%'):
      rstr = string.replace(rstr, '%%', '%')
   if string.find(rstr, '%n'):
      rstr = string.replace(rstr, '%n', devdict['name'])
   if string.find(rstr, '%r'):
      rstr = string.replace(rstr, '%r', '\n')
   if string.find(rstr, '%t'):
      rstr = string.replace(rstr, '%t', '\t')

   return rstr

#-----------------------------------------------------------------------
# Generate a dictionary of port substitutions and create the
# device string for output
#-----------------------------------------------------------------------

def devwrite(calldict, devdict, classidx):
   ltext = ''

   # Resolve network connections to ports and store as a dictionary
   #...............................................................
   portdict = {}
   ports_from = calldict['ports']
   ports_to = devdict['ports']
   for i in range(0,len(ports_from)):
      portdict[textprint(ports_to[i], params)] = textprint(ports_from[i], [])

   # Resolve instanced vs. default parameters and store as a dictionary
   # (only for use with the "%v" statement in info labels)
   #...............................................................
   paramdict = {}
   try:
      def_params = devdict['parameters']
   except KeyError:
      def_params = []

   try:
      params = calldict['parameters']
   except KeyError:
      params = []

   params += def_params[len(params):]

   for i in range(0,len(params)):
      paramdict[textprint(def_params[i], [])] = textprint(params[i], [])

   # For each "spice:" info label, parse the info label
   #...............................................................
   w = devdict['devices']
   for y in w:
      for u in y:
	 lstr = select(textprint(u, params), 'spice')
	 if lstr <> '':
	    ltext += parsedev(lstr, portdict, devdict, classidx, paramdict)
	    ltext += '\n'
   return ltext
	 
#-----------------------------------------------------------------------
# Check if a device is "fundamental";  that is, has a "devices" dictionary
# with at least one entry with "spice:" for SPICE output.
#-----------------------------------------------------------------------

def isfundamental(devdict):
   try:
      w = devdict['devices']
   except KeyError:
      return 0
   else:
      for y in w:
	 for u in y:
	    lstr = select(textprint(u, []), 'spice')
	    if lstr <> '':
	       return 1
   return 0

#-----------------------------------------------------------------------
# Select the device string corresponding to the given prefix.
# Return the body of the string, or an empty string if the prefix
# doesn't match.
#-----------------------------------------------------------------------

def select(sstr, prefix):
   ltext = ''
   if sstr.startswith(prefix):
      ltext += sstr[len(prefix) + 1:]
   return ltext
	 
#-----------------------------------------------------------------------
# Generate an ASCII string from an xcircuit string (list).  Translate
# overlines to "!" and Greek "mu" (either from Symbol font or from the
# ISO-Latin1 encoding of any standard font) to "u".
# String parameter substitution is performed if "params" is a non-empty
# list and the dictionary key "Parameter" is encountered in the string
# list "slist".
#-----------------------------------------------------------------------

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
	   try:
	      plist = params[x[x.keys()[0]]]
	   except IndexError:
	      ltext += ' --unknown parameter %d-- ' % x[x.keys()[0]]
	   else:
	      ltext += textprint(plist, [])
	   continue;
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
	
#-----------------------------------------------------------------------
# Write netlist to the output
#-----------------------------------------------------------------------

def writespice():
   subidx=1		# subcircuit numbering
   devdict={}		# dictionary of low-level devices
   classidx = {}	# indices of SPICE devices (other than subcircuits)
   p=netlist()		# Generate the netlist
   g=p['globals']	# Get the list of global networks
   c=p['circuit']	# Get the list of (sub)circuits
   l=len(c)
   top=c[l-1]		# Top-level circuit
   topname=top['name']	# Name of the top-level circuit
   topname += '.spc'	# Filename to write to
   try:
      outfile=open(topname, 'w')
   except IOError:
      return

   # Print header line
   #...............................................................

   outfile.write('*SPICE circuit "' + topname + '"')
   outfile.write(' from XCircuit v' + str(xc_version))
   outfile.write(' (Python script "spice.py")\n')

   # Print global variables.  This is for hspice and should be
   # commented out for spice3.
   #...............................................................

   for x in g:	# 'globals' is a list of strings
      outfile.write('.GLOBAL ' + textprint(x, []) + '\n')
   outfile.write('\n')

   # Print the (sub)circuits.  Top level circuit does not get a "subckt"
   # entry.
   #...............................................................

   for x in c:	# 'circuits' is a list of dictionaries
      is_device = 0
      try:
	 w = x['ports']		# write circuit name and ports, if any
      except KeyError:
	 is_top = 1
      else:
	 is_top = 0
	 if isfundamental(x) == 0:
            outfile.write('.subckt ' + x['name'])
            for y in w:
	       outfile.write(' ' + textprint(y, []))
            outfile.write('\n')
	 else:
	    is_device = 1
	    devdict[x['name']] = x

      try:
	 v = x['calls']		# calls to subcircuits in xcircuit
      except KeyError:
	 pass
      else:
         for y in v:
	    try:
	       d = devdict[y['name']]
	    except KeyError:		# A SPICE subcircuit instance
	       outfile.write('X' + str(subidx))
	       subidx+=1
	       for z in y['ports']:
	          outfile.write(' ' + textprint(z, []))
	       outfile.write(' ' + y['name'] + '\n') 
	    else:			# A SPICE circuit element
	       nstr = devwrite(y, d, classidx);
	       outfile.write(nstr)

      if is_top:
	 outfile.write('.end\n')
      elif is_device == 0:
	 outfile.write('.ends\n\n')

#-----------------------------------------------------------------------
# Key binding and menu button for the spice netlist output
# bind('Alt_S', 'writespice')
#-----------------------------------------------------------------------

newbutton('Netlist', 'Write Spice', 'writespice')

#-----------------------------------------------------------------------
