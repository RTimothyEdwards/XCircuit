# Commands to run in the console prior to launching the
# slave interpreter for XCircuit
#
# Since the TkCon window is not a necessity in XCircuit, we withdraw it
# but set up commands which can be executed from the slave interpreter
# that can be used to reinstate the console when necessary (such as
# when the "%" key macro is typed).
# (suggested by Joel Kuusk)
#
slave alias xcircuit::consoledown wm withdraw .
slave alias xcircuit::consoleup wm deiconify .
slave alias xcircuit::consoleontop raise .
#
# The setup is that tkcon is the master interpreter and the layout window
# is the slave interpreter.  However, the end-user has the impression that
# tkcon is a subsidiary window of the layout.  Thus, closing the console
# window should pop down the console rather than forcing an immediate and
# irrevocable exit from xcircuit.

wm protocol . WM_DELETE_WINDOW {tkcon slave slave xcircuit::lowerconsole}
