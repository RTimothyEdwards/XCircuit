#------------------------------------------------------------------------
# Mouse button hints---written by James Vernon March 2006 
#------------------------------------------------------------------------

#------------------------------------------------------------------------
# Write 3 strings corresponding to the 3 mouse button functions
# Use the "bindkey -compat" to get the current button bindings
#------------------------------------------------------------------------

proc xcircuit::automousehint {window} {
   
   set frame [winfo top $window]
   set btext [bindkey $window -compat Button1]
   ${frame}.infobar.mousehints.left configure -text $btext
   set btext [bindkey $window -compat Button2]
   ${frame}.infobar.mousehints.middle configure -text $btext
   set btext [bindkey $window -compat Button3]
   ${frame}.infobar.mousehints.right configure -text $btext
}

#------------------------------------------------------------------------
# Creates a canvas showing the 3 buttons of a 3 button mouse
# and adds labels that can contain strings telling the user what
# the current function of those buttons is
#------------------------------------------------------------------------

proc xcircuit::mousehint_create {name} {

  frame ${name}.infobar.mousehints -background beige

  label ${name}.infobar.mousehints.title -background beige -foreground brown4 \
	-anchor e -text "Button bindings: "
  button ${name}.infobar.mousehints.left -background beige -relief groove
  button ${name}.infobar.mousehints.middle -background beige -relief groove
  button ${name}.infobar.mousehints.right -background beige -relief groove

  grid ${name}.infobar.mousehints.title -row 0 -column 0 -sticky news
  grid ${name}.infobar.mousehints.left -row 0 -column 1 -sticky news
  grid ${name}.infobar.mousehints.middle -row 0 -column 2 -sticky news
  grid ${name}.infobar.mousehints.right -row 0 -column 3 -sticky news

  grid columnconfigure ${name}.infobar.mousehints 0 -weight 0
  grid columnconfigure ${name}.infobar.mousehints 1 -weight 0
  grid columnconfigure ${name}.infobar.mousehints 2 -weight 0
  grid columnconfigure ${name}.infobar.mousehints 3 -weight 0

  mousehint_bindings ${name} ${name}.infobar.mousehints	

  xcircuit::mousehint_show ${name}
}

#------------------------------------------------------------------------
# Displays the mouse_canvas that shows a picture of the 3
# buttons on a mouse in the top right corner of the top level window
# and moves the menubar.message widget under the menu buttons
#------------------------------------------------------------------------

proc xcircuit::mousehint_show { name } {

  pack forget ${name}.infobar.message2
  pack ${name}.infobar.mousehints -padx 2 -side left -ipadx 6 -fill y
  pack ${name}.infobar.message2 -side left -padx 2 -ipadx 6 -expand true -fill both

  bind ${name}.infobar.mode <ButtonPress-1> "::xcircuit::mousehint_hide ${name}"
}

#------------------------------------------------------------------------
# Hides the mouse_canvas that shows a picture of the 3
# buttons on a mouse and moves the menubar.message label
# back in its place.
#------------------------------------------------------------------------

proc xcircuit::mousehint_hide { name } {
	
  pack forget ${name}.infobar.mousehints
  bind ${name}.infobar.mode <ButtonPress-1> "::xcircuit::mousehint_show ${name}"
}

#------------------------------------------------------------------------
# Highlights buttons on the mouse_canvas when the user
# presses the corresponding mouse buttons
# Seems unnescesary, but without it who would guess that those
# rectangles are supposed to look like mouse buttons?
#------------------------------------------------------------------------

proc xcircuit::mousehint_bindings { name mouse_frame } {

   bind all <Button-1> "${mouse_frame}.left configure -state active ; \
	xcircuit::automousehint ${name}.mainframe.mainarea.drawing"
   bind all <Button-2> "${mouse_frame}.middle configure -state active ; \
	xcircuit::automousehint ${name}.mainframe.mainarea.drawing"
   bind all <Button-3> "${mouse_frame}.right configure -state active ; \
	xcircuit::automousehint ${name}.mainframe.mainarea.drawing"

   bind all <ButtonRelease-1> "${mouse_frame}.left configure -state normal"
   bind all <ButtonRelease-2> "${mouse_frame}.middle configure -state normal"
   bind all <ButtonRelease-3> "${mouse_frame}.right configure -state normal"

   bind all <KeyPress> "xcircuit::automousehint ${name}.mainframe.mainarea.drawing"

   bind ${mouse_frame}.title <ButtonPress-1> "::xcircuit::mousehint_hide ${name}"
}

#------------------------------------------------------------------------
