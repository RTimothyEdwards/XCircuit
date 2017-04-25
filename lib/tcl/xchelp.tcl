#-----------------------------------------------------------------
# Procedures to generate and display a help window in xcircuit
#-----------------------------------------------------------------

proc xcircuit::printhelp {} {
   set csel [.help.listwin.func curselection]
   if {$csel == ""} {return}
   set key [.help.listwin.func get $csel]
   switch -glob $key {
      {Page} {.help.listwin.win configure -text \
	"Switch to the indicated page."}
      {Justify} {.help.listwin.win configure -text \
	"Change justification of the currently selected or\
	edited label."}
      {Text Delete} {.help.listwin.win configure -text \
	"When editing a label, delete one character."}
      {Text Return} {.help.listwin.win configure -text \
	"When editing a label, accept edits and return to normal drawing mode."}
      {Text Left} {.help.listwin.win configure -text \
	"When editing a label, move cursor left one character or procedure."}
      {Text Right} {.help.listwin.win configure -text \
	"When editing a label, move cursor right one character or procedure."}
      {Text Up} {.help.listwin.win configure -text \
	"When editing a multi-line label, move cursor up one line of text."}
      {Text Down} {.help.listwin.win configure -text \
	"When editing a multi-line label, move cursor down one line of text."}
      {Text Split} {.help.listwin.win configure -text \
	"When editing a label, split the label into two separate labels at\
	the cursor position."}
      {Text Home} {.help.listwin.win configure -text \
	"When editing a label, move cursor to the beginning of the label."}
      {Text End} {.help.listwin.win configure -text \
	"When editing a label, move cursor to the end of the label."}
      {Tab Forward} {.help.listwin.win configure -text \
	"When editing a label, move forward to the next defined tab stop."}
      {Tab Backward} {.help.listwin.win configure -text \
	"When editing a label, move backward to the previously defined tab stop."}
      {Tab Stop} {.help.listwin.win configure -text \
	"When editing a label, declare a tab stop at the current horizontal cursor\
	 position.  This may be used to align text in multiple lines to certain\
	 positions."}
      {Superscript} {.help.listwin.win configure -text \
	"When editing a label, make text superscripted (about 2/3 size, and\
	raised about half a line)."}
      {Subscript} {.help.listwin.win configure -text \
	"When editing a label, make text subscripted (about 2/3 size, and\
	lowered about 3/4 of a line)."}
      {Normalscript} {.help.listwin.win configure -text \
	"When editing a label, return from a subscript or superscript alignment\
	to a normal text alignment."}
      {Nextfont} {.help.listwin.win configure -text \
	"When editing a label, change the font at the cursor position to the\
	next font family in the list.  This will cycle through all of the\
	defined fonts if executed repeatedly."}
      {Boldfont} {.help.listwin.win configure -text \
	"When editing a label, change the current font family to a boldface\
	type, if available."}
      {Italicfont} {.help.listwin.win configure -text \
	"When editing a label, change the current font family to an italic\
	or slanted type, if available."}
      {Normalfont} {.help.listwin.win configure -text \
	"When editing a label, change the current font family to a regular\
	type (non-italic, non-bold)."}
      {Underline} {.help.listwin.win configure -text \
	"When editing a label, begin underlining the text."}
      {Overline} {.help.listwin.win configure -text \
	"When editing a label, begin overlining the text."}
      {ISO Encoding} {.help.listwin.win configure -text \
	"When editing a label, change the font encoding from normal to ISO."}
      {Linebreak} {.help.listwin.win configure -text \
	"When editing a label, append a linebreak to the label.  This generates\
	multi-line text."}
      {Halfspace} {.help.listwin.win configure -text \
	"When editing a label, insert a half-space."}
      {Quarterspace} {.help.listwin.win configure -text \
	"When editing a label, insert a quarter-space."}
      {Special} {.help.listwin.win configure -text \
	"When editing a label, insert a special character.  This is a command\
	that brings up the font encoding array, allowing the user to select\
	any character from the current font in the current encoding type."}
      {Parameter} {.help.listwin.win configure -text \
	"When editing a label, insert a parameter.  The parameter must first\
	exist in the current object.  If only one parameter is defined, it\
	will be inserted.  If more than one parameter is defined, a selection\
	box will appear, allowing the user to select which parameter to\
	insert into the text."}
      {Parameterize Point} {.help.listwin.win configure -text \
	"When editing a polygon, parameterize the X and Y position of the\
	currently edited point.  The point may then be at a different\
	position in different instances of the current object."}
      {Break at Point} {.help.listwin.win configure -text \
	"When editing a polygon, break the polygon into two pieces at\
	the currently edited point."}
      {Delete Point} {.help.listwin.win configure -text \
	"When editing a polygon, remove the currently edited point, and\
	move to the next point."}
      {Insert Point} {.help.listwin.win configure -text \
	"When editing a polygon, insert a point at the same position as\
	the currently edited point."}
      {Append Point} {.help.listwin.win configure -text \
	"When editing a polygon, append a point at the same position as\
	the currently edited point, and move the currently edited point\
	to the new point."}
      {Next Point} {.help.listwin.win configure -text \
	"When editing a polygon, move to the next point."}
      {Attach} {.help.listwin.win configure -text \
	"When editing a polygon, select the nearest element that is not\
	the polygon, and force the currently edited point to terminate\
	on that element.  The element to attach to may be a polygon, an\
	arc, a spline, or a path.  The edit point will continue to follow\
	the cursor within the constraint that it must remain connected to\
	the other element."}
      {Virtual Copy} {.help.listwin.win configure -text \
	"Create a virtual library instance of the currently selected\
	object instance.  This will duplicate the existing library entry\
	but will have the scale and rotation of the selected instance,\
	as well as any non-default parameters given to that instance."}
      {Next Library} {.help.listwin.win configure -text \
	"When on a library page, go to the next defined library.  The\
	User Library is always last, and then the function cycles back\
	to the first defined library."}
      {Library Directory} {.help.listwin.win configure -text \
	"Go to the master list of libraries."}
      {Library Copy} {.help.listwin.win configure -text \
	"When on a library page, select the object under the cursor and\
	any selected objects, and return to the originating page in\
	copy mode."}
      {Library Edit} {.help.listwin.win configure -text \
	"When on a library page, edit the name of an object by editing\
	the text label underneath the picture of the object."}
      {Library Delete} {.help.listwin.win configure -text \
	"When on a library page, remove the selected object from the\
	library.  An object can be removed only if there are no instances\
	of that object on any page or in the hierarchy of another object.\
	A removed object is destroyed and cannot be recovered."}
      {Library Duplicate} {.help.listwin.win configure -text \
	"When on a library page, create a duplicate object of the selected\
	object, placing it in the User Library."}
      {Library Hide} {.help.listwin.win configure -text \
	"When on a library page, remove the selected object from view but\
	do not destroy it.  An object can be hidden from view only if\
	it exists in the hierarchy of another object (such as the arrow
	object inside the current source object)."}
      {Library Virtual Copy} {.help.listwin.win configure -text \
	"When on a library page, create a duplicate instance of the\
	selected object.  The duplicate instance may have different\
	(instanced) parameters.  Default parameters are defined by\
	the master object.  The name of the master object is printed\
	in black, while the names of virtual instances are in gray.\
	To define a virtual copy with different scale or rotation,\
	use the (non-Library) Virtual Copy function."}
      {Library Move} {.help.listwin.win configure -text \
	"When on a library page, move the position of an object relative\
	to other objects.  If one object is selected, it will be moved\
	to the position nearest the cursor.  If two objects are selected,\
	their positions will be swapped.  If in the Page Directory, the\
	relative positions of pages may be changed with the same function."}
      {Library Pop} {.help.listwin.win configure -text \
	"When on a library page, return to the originating page."}
      {Page Directory} {.help.listwin.win configure -text \
	"Go to the master list of pages."}
      {Help} {.help.listwin.win configure -text \
	"Display the window of help information and key bindings."}
      {Redraw} {.help.listwin.win configure -text \
	"Redraw everything in the window."}
      {View} {.help.listwin.win configure -text \
	"Scale and position the view of the current page so that elements\
	on the page fill the window to the maximum extent possible (minus\
	small margins on all sides)."}
      {Zoom In} {.help.listwin.win configure -text \
	"Increase the magnification of the view of the current page in\
	the window.  The location of the page at the center point of the\
	window remains at the center."}
      {Zoom Out} {.help.listwin.win configure -text \
	"Decrease the magnification of the view of the current page in\
	the window.  The location of the page at the center point of the\
	window remains at the center."}
      {Pan} {.help.listwin.win configure -text \
	"Change the view of the current page.  There are various modes,\
	including here, center, left, right, up, down, and follow.  Pan\
	center changes the view such that the point nearest\
	the cursor becomes the center point of the window. Pan up, down,\
	right, and left change the view by one-half page in the indicated\
	direction."}
      {Double Snap} {.help.listwin.win configure -text \
	"Increase the spacing between snap points by a factor of two."}
      {Halve Snap} {.help.listwin.win configure -text \
	"Decrease the spacing between snap points by half."}
      {Write} {.help.listwin.win configure -text \
	"Display the Output Properties and File Write window.  If output\
	properties are changed but user does not want to write the file\
	(such as when attaching multiple pages or schematics), use the\
	Cancel button to accept all applied changes and pop down the\
	window without writing."}
      {Rotate} {.help.listwin.win configure -text \
	"Rotate the selected elements, or element nearest the cursor, by\
	the number of degrees in the argument.  A positive number indicates\
	a clockwise rotation, a negative number, a counterclockwise\
	rotation."}
      {Flip X} {.help.listwin.win configure -text \
	"Flip the selected elements, or element nearest the cursor, in the\
	horizontal (X) direction (that is, across the Y axis).  The axis\
	defining the flip is the vertical line passing through the cursor X\
	position."}
      {Flip Y} {.help.listwin.win configure -text \
	"Flip the selected elements, or element nearest the cursor, in the\
	vertical (Y) direction (that is, across the X axis).  The axis\
	defining the flip is the horizontal line passing through the cursor\
	Y position."}
      {Snap} {.help.listwin.win configure -text \
	"Snap the selected elements, or element nearest the cursor, to the\
	nearest point on the snap grid.  Each point of a polygon is snapped.\
	Spline control points and arc centers are snapped.  Label and object\
	instance reference positions are snapped."}
      {Pop} {.help.listwin.win configure -text \
	"Return from editing an object (return from the last push)."}
      {Push} {.help.listwin.win configure -text \
	"Edit the object represented by the selected object instance, or the\
	object instance closest to the cursor.  The current page is pushed\
	onto a stack, and the object becomes the current page."}
      {Delete} {.help.listwin.win configure -text \
	"Delete the selected elements or element closest to the cursor."}
      {Select} {.help.listwin.win configure -text \
	"Select the element or elements near the cursor.  If multiple\
	elements match the selection criteria, an interactive method is\
	initiated in which individual elements may be selected or rejected\
	by pressing the mouse buttons 2 or 3, respectively."}
      {Box} {.help.listwin.win configure -text \
	"Begin drawing a box in the current default style.  One corner of\
	 the box is anchored at the cursor position, and the other corner\
	dragged with the cursor.  The box is completed by pressing buttons\
	1 or 2 or canceled with button 3."}
      {Arc} {.help.listwin.win configure -text \
	"Begin drawing a circle in the current default style.  The center\
	of the circle is anchored at the cursor position, and the radius\
	is defined by dragging the cursor to the desired position.  Button\
	2 completes the arc, and button 3 cancels it.  Button 1 switches\
	from defining the arc radius to (in sequence) defining the arc\
	start and stop angles, and defining the ellipse minor axis."}
      {Text} {.help.listwin.win configure -text \
	"Begin a normal text label.  In text edit mode, all keystrokes are\
	interpreted as input to the label, except for the key that is bound\
	to action Text Return.  Mouse buttons 1 and 2 complete the text\
	label, while button 3 cancels (deletes) the label."}
      {Exchange} {.help.listwin.win configure -text \
	"If two elements are selected, their relative positions (drawing\
	order) are swapped (drawing order determines what objects obscure\
	other objects when overlapping).  If one element is selected, it\
	is brought to the front of the drawing, unless it is already at\
	the front, in which case it is sent to the back."}
      {Copy} {.help.listwin.win configure -text \
	"Make a copy of the selected elements or element closest to the\
	cursor.  Elements are dragged as a group with the cursor.  Mouse\
	button 1 places the copies, creates a new set of copies, and\
	continues the copy operation.  Mouse button 2 places the copies\
	and ends the copy operation.  Mouse button 3 removes the current\
	selection being dragged and completes the copy operation."}
      {Move} {.help.listwin.win configure -text \
	"Move the selected elements or element nearest the cursor.  This\
	function is normally handled by the mouse button press, not the\
	Move binding."}
      {Join} {.help.listwin.win configure -text \
	"Join selected elements into a single element.  If all of the selected\
	elements are polygons, the new element will be a single polygon.  If\
	the selected elements include a mixture of one or more types\
	(polygons, arcs, splines, and paths), the resulting type will be a\
	path."}
      {Unjoin} {.help.listwin.win configure -text \
	"Break the selected path or path nearest the cursor into its\
	constituent elements.  This operation applies only to paths."}
      {Spline} {.help.listwin.win configure -text \
	"Begin a bezier curve (inconsistently and, technically, incorrectly\
	referred to in XCircuit documentation as a spline).  The curve is\
	anchored at the cursor position, and the other endpoint of the\
	curve is dragged with the cursor.  Mouse button 2 completes the\
	curve.  Mouse button 3 cancels (deletes) the curve.  Mouse button\
	1 switches between the two endpoints and two control points."}
      {Edit} {.help.listwin.win configure -text \
	"Edit the selected element or element nearest the cursor.  The\
	edit function applies to polygons, arcs, splines, paths, and\
	labels, but not to object instances (editing the instance implies\
	editing the object itself, which requires the Push function).\
	Editing a label, arc, or spline is the same as creating it.\
	Editing a polygon (including boxes and wires) allows individual\
	points to be selected and moved.  In all cases except labels,\
	mouse button 1 moves to the next point, mouse button 2 completes\
	the edit, and mouse button 3 cancels the last edit maneuver."}
      {Undo} {.help.listwin.win configure -text \
	"Undo the last action.  Note that not all actions are undoable."}
      {Redo} {.help.listwin.win configure -text \
	"Perform again the last action which was undone with the Undo\
	 function."}
      {Select Save} {.help.listwin.win configure -text \
	"Take all of the selected elements and turn them into a new\
	object.  The individual elements will be removed from the page\
	and replaced with an instance of the newly defined object, and\
	the object itself will be placed in the User Library.  The\
	user will be prompted for a name of the new library object."}
      {Unselect} {.help.listwin.win configure -text \
	"Remove the element closest to the cursor from the list of\
	currently selected objects."}
      {Dashed} {.help.listwin.win configure -text \
	"Set the border style of the selected elements or element\
	closest to the cursor to a dashed line."}
      {Dotted} {.help.listwin.win configure -text \
	"Set the border style of the selected elements or element\
	closest to the cursor to a dotted line."}
      {Solid} {.help.listwin.win configure -text \
	"Set the border style of the selected elements or element\
	closest to the cursor to a solid line."}
      {Prompt} {.help.listwin.win configure -text \
	"Generate the TCL command prompt."}
      {Dot} {.help.listwin.win configure -text \
	"Create a solder dot, connecting wires passing through it.  The\
	dot is placed at the current cursor position.  If a library\
	object named dot exists, it is used."}
      {Wire} {.help.listwin.win configure -text \
	"Begin drawing a wire (or unclosed polygon).  The wire is anchored\
	at the cursor position, and the other end is dragged with the\
	cursor.  Mouse button 1 anchors the endpoint and starts a new\
	wire segment.  Mouse button 2 anchors the endpoint and completes\
	the wire.  Mouse button 3 deletes the last anchored point."}
      {Nothing} {.help.listwin.win configure -text \
	"Null function.  Does nothing."}
      {Exit} {.help.listwin.win configure -text \
	"Exit the program immediately without prompting for action on\
	pages which have been changed but not saved."}
      {Start} {.help.listwin.win configure -text \
	"General-purpose start function, normally associated with mouse\
	button 1."}
      {Finish} {.help.listwin.win configure -text \
	"General-purpose completion function, normally associated with mouse\
	button 2."}
      {Cancel} {.help.listwin.win configure -text \
	"General-purpose cancelation function, normally associated with mouse\
	button 3."}
      {Snap To} {.help.listwin.win configure -text \
	"Toggle the state of the snapping function.  If on, points and element\
	positions are always snapped to the snap grid.  If off, points and\
	element position are unconstrained."}
      {Netlist} {.help.listwin.win configure -text \
	"Generate a netlist of the default type."}
      {Swap} {.help.listwin.win configure -text \
	"Switch between editing a schematic and its symbol.  If the schematic\
	covers multiple pages, will switch between the secondary page, primary\
	page, and symbol."}
      {Pin Label} {.help.listwin.win configure -text \
	"Create a local pin, a label that is interpreted as\
	the primary name of the network to which it is attached."}
      {Pin Global} {.help.listwin.win configure -text \
	"Create a global pin, a label that is interpreted as\
	belonging to the same network whereever it occurs on\
	the schematic."}
      {Info Label} {.help.listwin.win configure -text \
	"Create an info label, a label that is not displayed\
	except when editing the object, and which is parsed\
	to extract information on how to output the device in\
	a netlist."}
      {Connectivity} {.help.listwin.win configure -text \
	"Highlight the connectivity of the electrical network\
	 that includes the selected item or that is closest\
	 to the cursor.  Item must be a valid network element."}
      {Sim} {.help.listwin.win configure -text \
	"Generate a .sim (flat) netlist from the current page schematic,\
	especially for use with the IRSIM simulator and the netgen and\
	gemini LVS netlist comparators."}
      {SPICE} {.help.listwin.win configure -text \
	"Generate a .spc (hierarchical) netlist from the current page\
	schematic.  The format is compatible with Berkeley spice3 and\
	tclspice."}
      {PCB} {.help.listwin.win configure -text \
	"Generate a .pcbnet netlist from the current page schematic.\
	This format is compatible with the pcb layout program."}
      {SPICE Flat} {.help.listwin.win configure -text \
	"Generate a .fspc (flattened) SPICE netlist from the current\
	 page schematic."}
      {default} {.help.listwin.win configure -text ""}
   }
}

#-----------------------------------------------------------------
# Procedure to generate the help window
#-----------------------------------------------------------------

proc xcircuit::makehelpwindow {} {
   toplevel .help -bg beige
   wm withdraw .help

   frame .help.title -bg beige
   frame .help.listwin

   pack .help.title -side top -fill x
   pack .help.listwin -side top -fill both -expand true

   label .help.title.field -text "XCircuit Help" -bg beige
   button .help.title.dbut -text "Dismiss" -bg beige -command {wm withdraw .help}
   pack .help.title.field -side left -padx 10
   pack .help.title.dbut -side right -ipadx 10

   listbox .help.listwin.func -yscrollcommand ".help.listwin.sb set" \
	-setgrid 1 -height 20
   listbox .help.listwin.keys -yscrollcommand ".help.listwin.sb set" \
	-setgrid 1 -height 20
   scrollbar .help.listwin.sb -orient vertical -command \
	[list scrollboth [list .help.listwin.func .help.listwin.keys]]
   message .help.listwin.win -width 200 -justify left -anchor n \
	-relief groove -text "Click on a function for help text"

   grid .help.listwin.func -row 0 -column 0 -sticky news -padx 1 -pady 1
   grid .help.listwin.keys -row 0 -column 1 -sticky news -padx 1 -pady 1
   grid .help.listwin.sb -row 0 -column 2 -sticky ns -padx 1 -pady 1
   grid .help.listwin.win -row 0 -column 3 -sticky news -padx 1 -pady 1

   grid columnconfigure .help.listwin 1 -weight 1 -minsize 100
   grid rowconfigure .help.listwin 0 -weight 1 -minsize 100

   bind .help.listwin.func <ButtonRelease-1> "xcircuit::printhelp"
}

#-----------------------------------------------------------------
# Procedure to update and display the help window
#-----------------------------------------------------------------

proc xcircuit::helpwindow {} {

   # Create the help window if it doesn't exist
   if {[catch {wm state .help}]} {
      xcircuit::makehelpwindow
   }

   .help.listwin.func delete 0 end
   .help.listwin.keys delete 0 end

   set k [lsort -dictionary [xcircuit::bindkey]]

   .help.listwin.func insert end "Function"
   .help.listwin.keys insert end "Keys"
   .help.listwin.func insert end ""
   .help.listwin.keys insert end ""

   foreach i $k {
      set pkeys [xcircuit::bindkey -func $i]
      .help.listwin.func insert end "$i"
      .help.listwin.keys insert end "$pkeys"
   }

   wm deiconify .help
}

#-----------------------------------------------------------------
