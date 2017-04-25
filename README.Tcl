Tcl command summary (ASCII text version)
-------------------------------------------------
This list can be found in HTML format at URL:
    http://opencircuitdesign.com/xcircuit/tcl.html
-------------------------------------------------

I. Built-in commands (C source)
--------------------------------
A. Elements:  Main command to create and manipulate objects.  "element" is
	partly a superset of the individual elements, when the command is
	given a <handle> value and the element type is unknown.  A <handle>
	is an integer pointer to the element's location in memory.  The
	notation <handle...> represents a list of handles to elements.
	<handle> can also be the keyword "selected", in which case it
	implies all selected elements.  The <handle> is represented as a
	new Tcl type called "handle", and has an "H" followed by an 8-digit
	hexidecimal number.  This distinguishes it from integers and
	allows the command line to be parsed correctly.  It also discourages
	the practice of manipulating handles, as arithmetic cannot be
	performed directly on handle types.
	
	One option, "type", does not apply to individual elements.
	When no <handle> is supplied, the option operates on all currently
	selected elements in the drawing.  If no <handle> is supplied and
	no elements are currently selected, then the program goes into an
	interactive mode, prompting for an element to select and apply the
	option to.  Option "deselect" behaves differently in that 

   1. element [<handle...>] type
	Returns the type of the element, which may be one of "label",
	"polygon", "instance", "spline", "path", or "arc".

   2. <element> [<handle...>] <option>
	Where <element> may be any of "element", "label", "polygon",
	"instance", "spline", "path", or "arc".
	Options are:

	<element> [<handle...>] deselect
	   deselect the indicated element(s).
	<element> [<handle...>] parameter <option...>
	   Option may be one of the following:
	   a) allowed
		List the parameter types which are allowed for the
		indicated element type.
	   b) make <type> [<key>]
		Generate a parameter of the indicated type for
		the indicated element.  <type> may be one of:
		"position", "substring", "x position", "y position",
		"style", "justification", "start angle", "end angle",
		"radius", "minor axis", "rotation", "scale", "linewidth",
		or "color".  Items with more than one word must be
		quoted.  The initial value given to the parameter is
		the default.  <key> is a unique name given to the
		parameter.  It is necessary for substring parameters but
		may be omitted for other parameter types.
	   c) set <key> <value> [-forward]
		Change the value of the parameter with key <key>.  This
		assumes that the parameter exists;  if not, option "make"
		must be used.
	   c) get [<type|key>] [-forward]
		List the parameters of the indicated element(s).  If
		<type> is given, get the instance value of the parameter(s)
		of type <type>.   If <key> is given, then get the instance
		value of the parameter with key <key>.  If the parameter
		takes the default value, a null list is returned.  If the
		parameter type does not exist for the element, an error is
		generated.
	   e) default <type|key> [-forward]
		Get the default value of the indicated parameter, if <key>
		specified, or all parameters of the indicated type, if
		<type> is specified.
	   f) forget <type|key> [-forward]
		Delete the indicated parameter, if <key> is specified, or
		delete all parameters of the indicated type, if <type> is
		specified.  The parameter is removed entirely from the
		object.

	   Note that several commands take the optional argument "-forward".
	   The "-forward" argument applied only to selected object
	   instances, and indicates a forward-referenced parameter, that is,
	   a parameter belonging to the object of the selected instance,
	   and not the top-level object.
	<element> [<handle...>] delete
	   Deletes the indicated element(s).
	<element> [<handle...>] copy [relative] <position>
	   Makes a copy of the indicated element(s).  <position> is a list of
	   2 elements representing absolute X and Y positions, unless the
	   keyword "relative" is present, in which case they represent positions
	   relative to the current element position.  If more than one element
	   is specified, the position *must* be indicated as relative.
	<element> [<handle...>] move [relative] <position>
	   Moves the indicated element(s).  <position> is a list of
	   2 elements representing absolute X and Y positions, unless the
	   keyword "relative" is present, in which case they represent positions
	   relative to the current element position.  If more than one element
	   is specified, the position *must* be indicated as relative.
	<element> [<handle...>] flip horizontal|vertical [<position>]
	   Flips the indicated element(s) around the horizontal or vertical axis.
	   If <position> is specified, then element or element group will be
	   flipped around the indicated point.  <position> may be a single
	   number, representing an X value for horizontal flips and a Y value
	   for vertical flips.
	<element> [<handle...>] rotate <angle> [<position>]
	   Rotates the indicated element(s) by the specified angle (in degrees).
	   Positive angles are clockwise, negative angles are counterclockwise.
	   If <position> is specified, then element or element group will be
	   rotated around the indicated point.
	<element> [<handle...>] edit
	   Puts the specified element(s) into interactive edit mode.
	<element> [<handle...>] select
	   Selects the specified element(s).
	<element> [<handle...>] snap [<direction>]
	   Snaps the indicated elements onto the snap grid.  If <direction> is
	   specified (n, s, e, w, ne, nw, se, or sw), elements will be snapped
	   in that direction.  Otherwise, elements will be snapped to the
	   closest point.
	<element> [<handle...>] raise [<number>]
	   Raise the position of the indicated element(s) toward the drawing
	   front (end of list; last to be drawn).  If <number> is specified,
	   it is raised in front of the next <number> elements in the list.
	   If no <number> is specified, it is raised to the front of the drawing.
	<element> [<handle...>] lower [<number>]
	   Lower the position of the indicated element(s) toward the drawing
	   back (beginning of list; first to be drawn).  If <number> is specified,
	   it is lowered behind the next <number> elements in the list.  If no
	   <number> is specified, it is lowered to the back of the drawing.
	<element> [<handle1> [<handle2>]] exchange
	   Exchanges the positions of the two elements in the list.  If no
	   handles are specified, then exactly one or two objects must be
	   previously selected.  If only one handle is specified or only
	   one object previously selected, the behavior is to raise it to the
	   front of the drawing, unless it is already at the front of the
	   drawing, in which case it is moved to the back.
	<element> [<handle...>] color [<idx>]
	   Sets the color of the specified element(s) to value <idx>, an index
	   into xcircuit's color table.  With no arguments, returns the color
	   of the indicated element.

   3. <path_element> [<handle...>] <option>
	Where <path_element> may be any of  "polygon", "spline", "arc", or "path".
	Options are:
	<path_element> [<handle...>] border [<value>]
	    Set the border linewidth scaling to floating-point value <value>,
	    or return the current scale if <value> is not given.
	<path_element> [<handle...>] border [<type>]
	    Set the border style of the indicated element(s) to <type>, or return
	    the type (or list of types) if <type> is not given. <type> may be one
	    of "solid", "dashed", "dotted", or "closed"
	<path_element> [<handle...>] fill [<type>]
	    Set the fill style of the indicated element(s) to <type>, or return
	    the type (or list of types) if <type> is not given.  <type> may be
	    a fill percentage (representing a stipple pattern) or the keyword
	    "opaque".

   4. <segmented_element> [<handle...>] point <option>
	Where <segmented_element> may be any of "polygon", "spline", or "path".
	Options are:
	<segmented_element> [<handle...>] point [<number>] insert [after|before]
		[relative] [<position>]
	   Insert a new point before or after point <number> or the current
	   point if editing interactively.  If non-interactive, the point must
	   be given a position as a list of size 2, which is either an absolute
	   position, or a relative position if the keyword "relative" is given.
	<segmented_element> [<handle...>] point [<number>] delete [<number>]
	   Delete point <number>, or the current point if editing interactively.
	<segmented_element> [<handle...>] point [<number>] parameter
	   Parameterize the position of point <number>, or the current point
	   if editing interactively.
	<segmented_element> [<handle...>] point [<number>] break
	   Break the element at point <number>, or at the current point if
	   editing interactively.  Returns a handle to the new element
	   generated by the break.
	<segmented_element> [<handle...>] point [<number>] next
	   Return the point following point <number>, if specified, or move
	   to the next point if editing interactively.
	<segmented_element> [<handle...>] point [<number>] snap [<direction>]
	   Snap the indicated point <number> to the snap-to grid.  Snapping
	   is in the indicated direction, if supplied, or the closest point,
	   if not.  <direction> may be one of "n", "s", "e", "w", "ne", "nw",
	   "se", or "sw".

   4. instance [<handle...>] <option>
	instance make <object_name> [<position>]
	instance [<handle>] push
	   Edit the indicated instance by "pushing" down in the hierarchy.
	   Exactly one handle must be specified or one object instance
	   selected prior to executing the command.  This command operates
	   like "object push" except that if any values in the object are
	   parameterized, the instance values will be changed, not the default
	   values.
	instance [<handle...>] scale [<value>]
	   Change the scale of the indicated instance to <value>.  If no value
	   is specified, return the current scale of the instance.
   5. label [<handle...>] <option>
	label make
	   Interactively create a new label element.
	label make [pin|global|info] <string_list> <position>
	   Create a new label element with the text specified by <string_list>
	   and origin at <position>.  Optional keywords "pin", "global", or
	   "info" make the label a schematic pin type.  <string_list> is a
	   list of string parts or a single string.  String parts 
	label [<handle...>] append <string_list>
	   Add <string_list> to the end of the indicated label.
	label [<handle...>] insert <position> <string_list>
	   Insert <string_list> into the indicated label at the indicated position.
	label [<handle...>] delete <start> <end>
	   Delete a substring of the indicated label beginning at position <start>
	   and ending before position <end>.
	label [<handle...>] get <start> <end>
	   Return the substring (list) between beginning at position <start> and
	   ending before position <end>
	label [<handle...>] scale [<value>]
	   Change the scale of the indicated label.  Return the scale if <value>
	   is not specified.
	label [<handle...>] justify [<hjust>] [<vjust>]
	   Change the justification of the indicated label, where <hjust> may be
	   one of "left", "center", "right", and <vjust> may be one of "top",
	   "middle", or "bottom".  If neither <hjust> nor <vjust> is specified,
	   command returns a list of size 2 containing the horizontal and
	   vertical justifications.
	label [<handle...>] flipinvariant [true|false]
	   Set the flip-invariance of the indicated label.  If no value is
	   supplied, return the state of the flip-invariance on the label.
	label [<handle...>] style [<font_style>]
	   Set the label style to <font_style>, which may be one of "normal",
	   "bold", "italic", or "bolditalic".  If no style is specified,
	   return the current font style.
	label [<handle...>] family [<font_family>]
	   Set the label font family to <font_family>.  If no font family
	   is specified, return the current font family.
	label [<handle...>] encoding [<font_encoding>]
	   Set the label encoding to <font_encoding>, which may be one of
	   "standard", "special", or "ISO-Latin1" through "ISO-Latin6".
	   If no font encoding is specified, return the current font encoding.
   6. polygon [<handle...>] <option>
	polygon make [box]
	   Interactively create a new polygon element.  A rectangle is
	   created if "box" is specified.  Otherwise, the polygon is
	   generated point by point, in wire-drawing mode.
	polygon make <N> <position1> <position2> ... <positionN>
	   Create a new polygon element with <N> points.  Each <position>
	   is a list of size 2 with X, Y coordinates.
	polygon make box <position1> <position2> <position3> <position4>
	   Create a new polygon element with 4 points and with border style
	   "closed".  Each <position> is a list of size 2 with X, Y
	   coordinates.
   7. spline [<handle...>] <option>
	spline make
	   Interactively create a new spline element.
	spline make [<position1> <position2> <position3> <position4>]
	   Create a new spline object with endpoints <position1> and
	   <position4> and control points <position2> and <position3>.
   8. arc [<handle...>] <option>
	arc make
	   Interactively create a new arc element.
	arc make [<position> <radius> [<minor>] [<angle1> <angle2>]]
	   Create a new arc element with the indicated values.
	   By default, <minor> is set to the value of <radius>, <angle1>
	   is zero, and <angle2> is 360.  Angle values are in degrees.
	arc [<handle...>] radius|minor [<value>]
	   Specify the major or minor axis radius for the indicated arc.
	arc [<handle...>] angle start|end [<value>]
	   Specify the start and end angles for the indicated arc.
   9. path [<handle...>] <option>
	path make [<handles...>]
	   Generate a path element from the indicated components.
  10. object [<handle...>|<name...>] <option>
	object make [<handle...>] [<library>] [<name>]
	   Creates a new object out of the elements specified by <handle...>,
	   or from the selected elements if <handle...> is not specified.
	   The new object is placed into library <library>, or the User
	   Library, if not specified.  The object is given the name <name>,
	   if specified; otherwise the user is prompted for a name
	object [<handle|name>] push
	   Edit the indicated object.  This is like an instance edit except
	   that it is the library object itself, with default values for
	   parameters, that is edited.  If the object takes no parameters,
	   then there is no difference between editing an object and editing
	   any of its instances.
	object [<handle|name>] center [<position>]
	   Set the object's origin to <position> (a list of X, Y values).
	   If <position> is not specified, then return the coordinates of
	   the center of the object's bounding box.
	object [<handle|name>] copy [<library>] [<name>]
	   A copy of the object is made and placed in the named <library>, or
	   in the User Library if not specified.  The new copy is given the
	   name <name>, or the original name prepended with an underscore if
	   the name is not specified.  If the originating and destination
	   libraries are the same, the copy will be a "virtual" copy.
	object [<handle>] move [<library>]
	   Move the object to the indicated library, or the User Library if
	   <library> is not specified.
	object [<handle>] hide
	   Hide the object from view in its library, unless hiding the object
	   would render the object unaccessible.

B. Pages
   1. page
	Returns the current page
   2. page directory
	Go to the page directory listing (interactive command)
   3. page [<number>|<name>]
	Same as " page [<number>|<name>] goto" (see below)
   4. page make [<name>]
	Make a new page, giving it the optional name <name> if supplied,
	or "Page X" where X is the page number.  Generate a new menu button
	entry for the indicated page.  Go to the indicated page.
   4. page [<number>|<name>] <option>
	Where the page may be specfied either by page number or by page
	name (page label).  If neither number or name is supplied, then
	the current page is assumed.  Options are:

      load <filename...>
	Load the xcircuit file named <filename> into the indicated page.
      import <filename...>
	Import the xcircuit file named <filename> into the indicated page.
      background <filename>
	Read the PostScript file named <filename> into the indicated page
	as a background image.
      save [<filename>]
	Save the indicated page as <filename>.  Normally, <filename> is
	not specified and the filename given to the page by the "filename"
	command is used.
      goto
	Go to the page directory listing (interactive command)
      reset
	Resets (clears) the indicated page, or the current page if no
	arguements are given.
      links
	Returns the page labels of all pages which have the same filename
	as the current or indicated page.
      fit [true|false]
	If "true" or "false" is given, sets or clears the auto-fit
	function for the current page.  If no value is given, then
	it rescales the drawing to fit the output page.  Only valid
	in "full page" mode.
      filename [<name>]
	Sets the filename of the current page to <name>.  With no argument,
	returns the filename of the current page.
      label [<name>]
	Sets the name (page label) of the current page to <name>.  With no
	argument, returns the page label of the current page.
      scale [<value>]
	Sets the scale of the current page to <value>.  With no argument,
	returns the scale of the current page.
      width [<value>]
	Sets the scale of the current page such that the width is <value>.
	With no argument, returns the width of the object in the current
	page.
      height [<value>]
	Sets the scale of the current page such that the height is <value>
	With no argument, returns the height of the object in the current
	page.
      size [<dimension>]
	Sets the size of the output page for full-page mode to the given
	dimension, which can be a list of size 2 containing the page
	width and height, or a string in the format "width x height".
	With no argument, returns the size of the current page as a
	string in the format "width x height".

C. Libraries
   1. library
	Returns the current library, or "none" if none is being viewed.
   2. library directory
	Go to the library directory (interactive command).
   3. library make [<name>]
	Make a new library, giving it the optional name <name> if supplied,
	or "Library: X" where X is the library number.  Generate a new
	menu button entry for the indicated library.
   4. library [<number>|<name>] <option>
	Where the library is specified by number or by name.  If no
	library is specified, then the current library is assumed if
	a library page is currently in the xcircuit drawing window,
	or the User Library is assumed if not.  Options are:
      load <filename>
	Load the library from file <filename> into the indicated library.
      save <filename>
	Save the indicated library to the file <filename>.
      goto
	Go to the indicated library (interactive command).
   5. library <filename> [<number>]
	Backward compatibility;  same as "library [<number>] load <filename>"

D. Actions:  Actions are described under the "element" command.  These
   commands are exactly like the "element" subcommands except that the
   arguments are rearranged:  "element [<handle...>] <command> <options...>"
   becomes "<command> [<handle...>] <options...>".  In addition, these
   commands can take a position list <position> (list of size 2 containing X
   and Y values) in place of the handle, in which case the command attempts
   to select an element at the indicated position and apply the command to
   that element.

   1. delete [<handle...>|here] <options>
   2. undelete [<handle...>|here] <options>
   3. select [<handle...>|here|get] <options>
	The additional subcommand "get" returns a handle or list of handles
	of all currently selected elements.
   4. deselect [<handle...>|here] <options>
   5. copy [<handle...>|here] <options>
   6. edit [<handle...>|here] <options>
   7. parameter [<handle...>|here] <options>
   8. push [<handle...>|here] <options>
   9. pop [<handle...>|here] <options>
  10. rotate [<handle...>|here] <options>
  11. flip [<handle...>|here] <options>

E. Options
   1. config <option> [value...]
	Main option setting.  Options are:
	config axis|axes [on|off]
		Turn axis drawing on or off.  If no argument supplied,
		toggle the state of axis drawing.
	config grid [on|off]
		Turn grid drawing on or off.  If no argument supplied,
		toggle the state of grid drawing.
	config grid spacing <value>
		Set the grid spacing to <value>.  <value> is a number
		representing distance in the current coordinate system.
		Currently, coordinate system specifiers like "in" and "cm"
		may be included but are ignored.
	config snap [on|off]
		Turn snap point drawing on or off.  If no argument supplied,
		toggle the state of snap point drawing.
	config snap spacing <value>
		Set the snap spacing to <value>.  <value> is a number
		representing distance in the current coordinate system.
		Currently, coordinate system specifiers like "in" and "cm"
		may be included but are ignored.
	config bbox [on|off]
		Turn bounding box drawing on or off.  If no argument supplied,
		toggle the state of bounding box drawing.
	config editinplace [on|off]
		Turn edit-in-place on or off.  If no argument supplied,
		toggle the state of edit-in-place.  If "on", when the
		drawing hierarchy is descended, the entire drawing is drawn
		from the top level, but everything above the current edit
		level is drawn in gray.
	config pinpositions [on|off]
		Turn pin position drawing on or off.  If no argument supplied,
		toggle the state of pin position drawing.  If "on", pin
		positions inside an object instance appear in levels of the
		drawing hierarchy outside of the object.
	config linewidth <value>
		Set global line scaling.  All linewidths in the drawing are
		determined relative to this overall scaling value (default
		1.0).
	config colorscheme normal|inverse
		Set the overall colorscheme.  "normal" is black-on-white.
		"inverse" is white-on-dark gray.
	config drawingscale <scale>
		Set the drawing scale.  <scale> is represented as
		<divisor>:<multiplier>.  Positions reported in the message
		bars are scaled by <multiplier>/<divisor> with respect to
		the scale of the actual output.
	config manhattan [on|off]
		Sets the style of polygon drawing.  When "on", lines can
		only be drawn vertical and horizontal.  If no argument is
		supplied, the current polygon drawing mode is toggled.
	config boxedit	manhattan|rhomboidx|rhomboidy|rhomboida|normal
		Sets the style of polygon editing.  "manhattan" forces
		lines to remain vertical or horizontal, but does not
		affect lines which are already diagonal.  "normal"
		places no restrictions on line position.  The "rhomboid"
		styles place manhattan restrictions on horizontal or vertical
		lines, but not both, and are of limited practicality.
	config coordstyle "decimal inches"|"fractional inches"|centimeters
		Sets the coordinate measurement system to metric or standard.
		"fractional inches" reports values in whole number fractions
		when possible.
   2. color [<option>]
	Where option is one of:
	a) set inherit|<idx>
		Sets color to the indicated color index (from xcircuit's
		color table).  "inherit" is the same as <idx>=-1, and
		represents a color which is inherited from the parent
		element in the drawing hierarchy.
	b) get
		Returns the color of the currently selected element,
		or the default color if nothing is selected.
	c) add <name>
		Adds a new color to the color index table (including
		adding an entry to the GUI color selection menu).
		Color may be specified by name (a la rgb.txt) or by
		"#RRGGBB" notation.
	
   3. fill [<option>|<fillfactor>]
	Given an integer between 0 and 100 inclusive, sets the fill style
	to the given fillfactor.  Values are rounded to the nearest known
	fillfactor value.  Know values are 0, 12, 25, 37, 50, 62, 75, 87,
	and 100.  Other options include "solid" (equivalent to "100"),
	"opaque", and "transparent".  With no arguments, returns a list
	of all the fill styles of the currently selected element, or the
	default fill style if nothing is selected.

   4. border [<option>]
	Sets the border style to the given option, which is one of "solid",
	"dashed", "dotted", "unbordered", "bounding box", "closed", and
	"unclosed".  The two-word "bounding box" must be quoted or in
	braces ({}).  "bounding box" takes an additional argument, "true"
	or "false".  With no option, returns a list of all border styles
	of the currently selected element, or the default border style
	if nothing is selected.

F. Netlists
   1. netlist <option>
	Perform various netlist functions.  Options are:
	a) write <format>
		Generate a netlist output file in one of the following
		formats:  "spice", "spiceflat", "sim", or "pcb".
	b) highlight
		Highlight connectivity of any selected network element
		(wire or pin), or start an interactive method for
		selecting a network to highlight.
	c) make
		Generates and returns a Tcl list element representing
		the netlist for the current circuit schematic.  The
		list is heavily nested.  The outermost list contains
		four elements:  The string "globals", a list of
		global networks, the string "circuit", and a list of
		circuit netlists.  These are further subdivided into
		hierarchical lists, a description of which is not
		contained here.
	d) autonumber
		Automatically substitues indices for unnumbered
		circuit components.
   2. schematic <option>
	Perform various netlist functions.  Options are:
	a) associate [<name>]
		Associate the schematic with the symbol named <name>.
		If <name> is not specified, xcircuit starts an interactive
		method for selecting a symbol for association.
	b) disassociate
		Disassociate any existing symbol from the schematic.
	c) make [<name>]
		Generate a new symbol associated with the current
		schematic.  The new symbol will be named after the
		page.  If the current page has not been named, then 
		option <name> must be provided.
	d) goto
		Change the current page to the associated symbol, if
		it exists
	e) get
		Return the name of the associated symbol, or {}
		(empty list) if no symbol is associated.
	f) type [<value>]
		Return the type of the current page, which may be one
		of "schematic", "symbol", "trivial", or "fundamental".
		If "value" is specified, change the current type to
		"value".  It is only possible to change types between
		"symbol", "trivial", and "fundamental", which are all
		symbol classes.  It is not possible to change symbols
		to schematics and vice versa.
   3. symbol
	"symbol" is simply an alias for "schematic".  Whether the command
	operates on a symbol or a schematic is determined purely from
	context.
	

G. GUI
   1. refresh
	Redraws the current window.
		Automatically substitues indices for unnumbered
		circuit components.
   2. zoom [in [<amount>]|out [<amount>]|box|view|factor [<amount>]]
	With no option, same as "zoom view".
	Options:
	   in [<amount>]:  Zoom in by current zoom factor or by [<amount>].
	   out [<amount>]:  Zoom out by current zoom factor or by [<amount>].
	   box: prompt for zoom box.  Zoom occurs as soon as box is drawn and
		the mouse button is released.
	   view:  Fits the drawing to the xcircuit window.
	   factor:  Returns the current zoom factor.
	   factor <amount>:  Sets the current zoom factor to <amount>.
	<amount> is a floating-point number representing a scale multiplier.
	Values greater than 1.0 imply a zoom "in", while values less than one
	imply a zoom "out".  Zero or negative values not allowed.
   3. pan [here|<position>]
	Center the drawing window on the indicated position.  If "here",
	position is centered on the cursor.  Otherwise, <position> is a
	list of size 2 with X and Y values.
   4. quit
	Quit xcircuit, with a prompt if any files with outstanding changes
	have not been saved.
   5. promptsavepage [<page_number>]
	Start the page output dialog.  Currently, <page_number>, if present,
	must be the current page number.
   6. here
	Returns a list size two with the X and Y position of the cursor
	relative to the XCircuit coordinate system.

H. Files
   1. loadfont <fontname>
	Loads an xcircuit font.  Expects to see file <fontname>.xfe (Xcircuit
	Font Encoding) in the default system library path.
	
   2. filerecover
	Recovers files from a crash or Ctrl-C exit that are left in the /tmp
	directory.

I. General
   1. quitnocheck
	Immediate exit from xcircuit
   2. start <args>
	XCircuit startup.  Usually called internally from the wrapper
	script.  However, for purposes of debugging problems, it can
	be called manually from the Tcl command after loading the
	shared object file "xcircuit.so".
   3. simple <pathname>
	This is a Tk extension which generates a "simple" window, a blank
	area into which a C-source application is supposed to draw.
   4. image create xpm <name> -file <filename.xpm>
	This is an extension to the Tk "image" command which allows images
	to be generated from "xpm"-format files.  Since xpm files contain
	all color and size information, the options are fixed (although it
	might be nice to add things like color substitutions).
   5. tag <name> [<procedure>]
	Attach a procedure to a command, such that the procedure is
	executed after every call to the command "name".  The procedure
	may name a Tcl procedure or it may be entirely inline.
	"procedure" may make use of several escape sequence substitutions,
	as follows:
		%0 through %5:  Substitute the argument passed to <name>.
		%r:  Substitute the Tcl result from <name>.  The result
		     is transparent; any result produced by the tag
		     procedure will be ignored and the calling function
		     will get back the original result (%r).
		%R:  Substitute the Tcl result from <name>.  The result
		     is absorbed, and the tag function generates a new 
		     result which is passed back to the calling function.
		%%:  Substitute a single percent character.
	All other uses of the percent character within the tag procedure
	will be evaluated as-is.
	With only one argument, <name>, the "tag" command will return
	the procedure currently attached to that command.
	Tagging a procedure to a command which is already tagged will
	cause the new tag procedure to overwrite the old one.
	Any tag procedure which calls the function to which it is tagged
	should wrap the entire procedure inside
	   if {[info level] <= 1} {...}
	to prevent infinite recursion.

	
II. Scripted commands (Tcl source)
----------------------------------

A. Namespace (Toolscript)
   1. pushnamespace <namespace>
	Makes all commands in <namespace> available in the current namespace.
	Conflicting commands names are not pushed.
   2. popnamespace <namespace>
	Revert command names back to original set.

B. Dialogs 
   1. promptobjectsize
	Dialog to ask for object size
   2. promptborderwidth
	Dialog to ask for element border width
   3. promptlinewidth
	Dialog to ask for overall linewidth scaling
   4. promptgridspace
	Dialog to ask for grid spacing
   5. promptdrawingscale
	Dialog to ask for drawing scale
   6. promptsnapspace
	Dialog to ask for snap spacing
   7. promptmakeobject
	Dialog to ask for object name before making new library entry
   8. promptsavelib <library_number>|<library_name>
	Dialog to ask for filename of library to save.
   9. promptloadlibrary
	Dialog to ask for filename of library to load
  10. promptaddlibrary
	Dialog to ask for filename of library to add to current library page
  11. promptloadfile
	Dialog to ask for filename of xcircuit file to load
  12. promptimportfile
	Dialog to ask for filename of xcircuit file to add to current page
  13. promptimportbackground
	Dialog to ask for filename of PostScript file to use as background
  14. promptexecscript
	Dialog to ask for filename of Tcl file to execute
  15. prompttextsize
	Dialog to ask for label scaling
  16. promptnewfont
	Dialog to ask for name of font to load
  17. promptkern
	Dialog to ask for X Y kerning values inside a label
  18. promptmakesymbol
	Dialog to ask for a name for a page, if the page has not yet been
	named, before generating a matching symbol of the same name.

C. Menu manipulation:  These functions should only be called from inside
	xcircuit, as they change only the menu appearance and callbacks
	and not xcircuit's internal data structures.

   1. newcolorbutton <r> <g> <b> <idx>
	Adds a new button to the color menu
	<r> <g> and <b> are color components, and <idx> is the position in
	xcircuit's color table.
   2. newencodingbutton <encodingname>
	Adds a new button to the font encoding menu.  Valid encoding names
	are "special", "standard", and "ISO-Latin1" through "ISO-Latin6".
   3. newfontbutton <familyname>
	Adds a new button to the font family menu.
   4. newlibrarybutton <libraryname>
	Adds a new button to the library menu with a callback to switch to
	the indicated library.
   5. newpagebutton <pagename>
	Adds a new button to the page menu with a callback to switch to the
	indicated page.
   6. renamelib <number> <libraryname>
	Changes the name and callback function of the indicated library menu button.
	<number> is the index position of the button in the library menu.
   7. renamepage <number> <pagename>
	Changes the name and callback function of the indicated page menu button
	<number> is the index position of the button in the page menu.
   8. xschema true|false
	Turns the netlisting functions on or off.
   9. toolbar true|false
	Turns the toolbar functions on or off.

D. Command tag callbacks:  These functions are attached to specific xcircuit
	commands using the "tag" command, and execute after the tagged
	command has executed.  This keeps the GUI synchronized with commands
	called from the command-line.

   1. pageupdate [subcommand]
	Updates the file output window to match the current page status.
   2. setsymschem
	Updates the schematic and symbol buttons, and the netlist menu
	options, to match the current page.
	
