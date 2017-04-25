## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign no-dependencies

EXTRA_DIST =

INCLUDES = -I.. ${INC_SPECS}

SUBDIRS =
SUBLIB =

# Requirements for spiceparser library
noinst_LIBRARIES = libspiceparser.a

libspiceparser_a_SOURCES = \
  bitlist.c eqn.c eqnlist.c equations.c hash.c list.c list_search.c \
  memory.c mergedup.c names.c netlist.c netlist_dev.c netlist_extract.c \
  netlist_funcs.c netlist_lib.c netlist_spice.c netlist_template.c \
  scanner.c sort.c xcircspice.c

