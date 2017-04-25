## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign no-dependencies

EXTRA_DIST = sub.c

INCLUDES = -I..

SUBDIRS =
SUBLIB =

# Requirements for Xw library
noinst_LIBRARIES = libxcXw.a

libxcXw_a_SOURCES = BBoard.c Button.c Cascade.c Display.c Form.c \
	Manager.c MapEvents.c MenuBtn.c MenuMgr.c MenuPane.c \
	PButton.c PopupMgr.c Primitive.c ResConvert.c SText.c \
	SourceStr.c TextEdit.c Toggle.c Traversal.c WorkSpace.c
