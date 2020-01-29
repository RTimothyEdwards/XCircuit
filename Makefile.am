## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign no-dependencies no-dist

AM_CFLAGS = $(cairo_CFLAGS) $(fontconfig_CFLAGS) ${GS_CFLAGS}
LIBS += $(cairo_LIBS) $(fontconfig_LIBS) ${GS_LIBS}

# Additional files to distribute
EXTRA_DIST = COPYRIGHT README README.ISOLatin2 README.notes

SUBDIRS = @SUBDIRS@
VERSION = @VERSION@
REVISION = @REVISION@
FULL_VERSION = @VERSION@.@REVISION@

# Override standard "make" target when compiling under TCL
ALL_TARGET = @ALL_TARGET@
INSTALL_TARGET = @INSTALL_TARGET@

# Menudep program generates menudep.h needed by some of the
# source files for Xcircuit
noinst_PROGRAMS = menudep$(EXEEXT)

# Automake files
ac_aux_dir = dist

# Requirements for Xcircuit
bin_PROGRAMS = xcircuit

# Temporary directory (if not overridden by environment variable TMPDIR)
tmpdir = /tmp

# Directories for app-defaults file and manual page
appdefaultsdir = $(libdir)/$(PACKAGE)-$(VERSION)/app-defaults
appmandir = $(libdir)/$(PACKAGE)-$(VERSION)/man

install_appdefaultsdir = @DIST_DIR@/lib/$(PACKAGE)-$(VERSION)/app-defaults
install_appmandir = @DIST_DIR@/lib/$(PACKAGE)-$(VERSION)/man
install_bindir = @DIST_DIR@/bin

PATHNAMES = -DPROG_VERSION=\"$(VERSION)\" \
	-DPROG_REVISION=\"$(REVISION)\" \
	-DCAD_DIR=\"$(libdir)\" \
	-DTEMP_DIR=\"$(tmpdir)\" \
	-DRESOURCES_DIR=\"$(appdefaultsdir)\" \
	-DSCRIPTS_DIR=\"$(scriptsdir)\" \
	-DBUILTINS_DIR=\"$(librarydir)\" \
	-DBUILTINS_FILE=\"$(BUILTINS_FILE)\" \
	-DUSER_RC_FILE=\"$(USER_RC_FILE)\" \
	-DPROLOGUE_DIR=\"$(librarydir)\" \
	-DPROLOGUE_FILE=\"$(PROLOGUE_FILE)\" \
	-DCYRILLIC_ENC_FILE=\"$(CYRILLIC_ENC_FILE)\" \
	-DISOLATIN2_ENC_FILE=\"$(ISOLATIN2_ENC_FILE)\" \
	-DISOLATIN5_ENC_FILE=\"$(ISOLATIN5_ENC_FILE)\" \
	-DSTARTUP_FILE=\"$(STARTUP_FILE)\" \
	-DLGF_LIB=\"$(LGF_LIB)\" \
	-DASG_SPICE_LIB=\"$(ASG_SPICE_LIB)\"

xcircuit_SOURCES = elements.c events.c filelist.c files.c \
	flate.c fontfile.c formats.c functions.c graphic.c \
	help.c keybindings.c libraries.c menucalls.c \
	netlist.c ngspice.c parameter.c python.c \
	rcfile.c render.c schema.c selection.c svg.c text.c undo.c \
	tclxcircuit.c tkSimple.c xcircuit.c xtfuncs.c xtgui.c \
	cairo.c utf8encodings.c
cairo.$(OBJEXT) : CFLAGS += -pedantic -Wall -Wextra
elements.$(OBJEXT) : CFLAGS += -pedantic -Wall -Wextra
events.$(OBJEXT) : CFLAGS += -pedantic -Wall -Wextra
fontfile.$(OBJEXT) : CFLAGS += -pedantic -Wall -Wextra
text.$(OBJEXT) : CFLAGS += -pedantic -Wall -Wextra
utf8encodings.$(OBJEXT) : CFLAGS += -pedantic -Wall -Wextra

xcircuit_DEPEND = @SUBLIB@
xcircuit_LDADD = @SUBLIB@ @STDLIBS@

help.c: menudep.h
menucalls.c: menudep.h
rcfile.c: menudep.h
schema.c: menudep.h
xcircuit.c: menudep.h
parameter.c: menudep.h
python.c: menudep.h
tclxcircuit.c: menudep.h
xtgui.c: menudep.h
xtfuncs.c: menudep.h

# Man page
M4_DEFS = @M4_DEFS@ @DEFS@
man_MANS = lib/xcircuit.1

# Main compiler arguments
DEFS = @DEFS@ $(PATHNAMES)
INCLUDES = -IXw ${INC_SPECS}
SHDLIB_EXT = @SHDLIB_EXT@
LDDL_FLAGS = @LDDL_FLAGS@
LDFLAGS = @LDFLAGS@
EXTRA_LIB_SPECS = @EXTRA_LIB_SPECS@
TCL_LIB_DIR = @TCL_LIB_DIR@
WISH_EXE = @WISH_EXE@
INTERPRETER = @INTERPRETER@

# Library directory and files
librarydir = $(libdir)/$(PACKAGE)-$(VERSION)
scriptsdir = $(libdir)/$(PACKAGE)-$(VERSION)
install_librarydir = @DIST_DIR@/lib/$(PACKAGE)-$(VERSION)
install_scriptsdir = @DIST_DIR@/lib/$(PACKAGE)-$(VERSION)
USER_RC_FILE = .xcircuitrc
PROLOGUE_FILE = xcircps2.pro
CYRILLIC_ENC_FILE = cyrillic.enc
ISOLATIN2_ENC_FILE = isolatin2.enc
ISOLATIN5_ENC_FILE = isolatin5.enc
ENCODING_FILES = ${CYRILLIC_ENC_FILE} ${ISOLATIN2_ENC_FILE} ${ISOLATIN5_ENC_FILE}
LGF_LIB = lgf.lps
ASG_SPICE_LIB = asg_spice.lps
SIGNAL_LIB = signal.lps
MUSIC_LIB = musiclib.lps
STARTUP_FILE = @STARTUP_FILE@
INTERP_PATH = @INTERP_PATH@

FONTS_LPS = courier courieriso2 courieriso5 helvetica helveticaiso2 \
	helveticaiso5 myfont symbol times_roman times_romaniso2 \
	times_romaniso5
FONTS_XFE = courier courieriso courieriso2 courieriso5 courier_cyrillic \
	helvetica helveticaiso helveticaiso2 helveticaiso5 helvetica_cyrillic \
	times_roman times_romaniso times_romaniso2 times_romaniso5 \
	times_roman_cyrillic myfont symbol
SCRIPTS_PY = spice gettext pagebbox

WRAPPER_INIT = xcircuit.tcl
WRAPPER_OBJ = xcircuit${SHDLIB_EXT}
WRAPPER_SO = xcircuit.so
WRAPPER_SH = xcircuit.sh
CONSOLE = tkcon.tcl
CONSOLE_SCRIPT = console.tcl

menudep$(EXEEXT): menudep.o
	$(CC) -o $@ menudep.o

menudep.h: menudep$(EXEEXT)
	./menudep$(EXEEXT)

xcwrap.o: xcwrap.c menudep.h
	$(CC) ${CPPFLAGS} ${CFLAGS} ${DEFS} \
		$(PATHNAMES) $(INCLUDES) \
		xcwrap.c -c -o xcwrap.o

lib/$(INTERP_PATH)/$(WRAPPER_OBJ): xcwrap.o ${xcircuit_OBJECTS} ${xcircuit_DEPEND}
	$(RM) lib/$(INTERP_PATH)/$(WRAPPER_OBJ)
	$(SHLIB_LD) ${CFLAGS} -o $@ ${LDDL_FLAGS} xcwrap.o \
		${xcircuit_OBJECTS} ${xcircuit_LDADD} ${SHLIB_LIB_SPECS} \
		${LDFLAGS} ${X_EXTRA_LIBS} ${EXTRA_LIBS} -lc ${LIBS} \
		${LIB_SPECS} ${EXTRA_LIB_SPECS}

xcircexec$(EXEEXT): xcircexec.o
	$(RM) ./xcircexec$(EXEEXT)
	$(CC) ${CFLAGS} ${CPPFLAGS} ${DEFS} $(PATHNAMES) $(INCLUDES) \
		xcircexec.c -o xcircexec$(EXEEXT) ${LIB_SPECS} \
		${LD_RUN_PATH} ${LDFLAGS} ${LIBS} ${EXTRA_LIB_SPECS}

xcircdnull$(EXEEXT): xcircdnull.o
	$(RM) ./xcircdnull$(EXEEXT)
	$(CC) ${CFLAGS} ${CPPFLAGS} ${DEFS} $(PATHNAMES) $(INCLUDES) \
		xcircdnull.c -o xcircdnull$(EXEEXT) ${LIB_SPECS} \
		${LD_RUN_PATH} ${LDFLAGS} ${LIBS} ${EXTRA_LIB_SPECS}

tcl:
	@echo Making tcl library object
	list='$(SUBDIRS)'; for subdir in $$list; do \
	   test "$$subdir" = . || (echo "Making tcl in $$subdir"; \
	   cd $$subdir && $(MAKE) $(AM_MAKEFLAGS)) \
	done
	$(MAKE) lib/$(INTERP_PATH)/$(WRAPPER_OBJ)
	$(MAKE) lib/$(INTERP_PATH)/$(WRAPPER_SH)
	$(MAKE) lib/$(INTERP_PATH)/$(WRAPPER_INIT)
	$(MAKE) xcircexec$(EXEEXT)
	$(MAKE) xcircdnull$(EXEEXT)

lib/$(INTERP_PATH)/$(WRAPPER_SH): lib/$(INTERP_PATH)/$(WRAPPER_SH).in
	sed -e '/XCLIBDIR/s#XCLIBDIR#$(librarydir)#' \
		-e '/WRAPPER_INIT/s/WRAPPER_INIT/$(WRAPPER_INIT)/' \
		-e '/CONSOLE_SCRIPT/s/CONSOLE_SCRIPT/$(CONSOLE_SCRIPT)/' \
		-e '/CONSOLE/s/CONSOLE/$(CONSOLE)/' \
		-e '/PROG_VERSION/s/PROG_VERSION/$(VERSION)/' \
		-e '/PROG_REVISION/s/PROG_REVISION/$(REVISION)/' \
		-e '/TCLLIBDIR/s#TCLLIBDIR#$(TCL_LIB_DIR)#' \
		-e '/WISH_EXE/s#WISH_EXE#$(WISH_EXE)#' \
		lib/$(INTERP_PATH)/$(WRAPPER_SH).in > $@
	chmod 0755 $@

lib/$(INTERP_PATH)/$(WRAPPER_INIT): lib/$(INTERP_PATH)/$(WRAPPER_INIT).in
	sed -e '/LIBDIR/s#LIBDIR#$(librarydir)#' \
		-e '/SCRIPTSDIR/s#SCRIPTSDIR#$(scriptsdir)#' \
		lib/$(INTERP_PATH)/$(WRAPPER_INIT).in > $@
	chmod 0755 $@

lib/xcircuit.1: lib/xcircuit.1.in
	sed -e '/LIBDIR/s#LIBDIR#$(libdir)#' \
		-e "/^${HAVE_LGF}/s/^${HAVE_LGF} //" \
		-e "/^${INTERPRETER}/s/^${INTERPRETER} //" \
		-e '/^PYTHON /d' \
		-e '/^TCL /d' \
		-e '/^LGF /d' \
		-e '/^NOINTERP /d' \
		lib/xcircuit.1.in > $@

install-data-local: lib/xcircuit.1
	@echo "Installing app-defaults file"
	$(mkinstalldirs) $(DESTDIR)$(install_appdefaultsdir)
	$(mkinstalldirs) $(DESTDIR)$(install_appmandir)
	$(INSTALL_DATA) lib/XCircuit.ad $(DESTDIR)$(install_appdefaultsdir)/XCircuit
	$(INSTALL_DATA) lib/xcircuit.1 $(DESTDIR)$(install_appmandir)/xcircuit.1
	@echo "Installing library and netlist files"
	$(mkinstalldirs) $(DESTDIR)$(install_librarydir)
	( cd lib ; for i in $(PROLOGUE_FILE) $(ENCODING_FILES) *.lps *.cir; do \
		$(INSTALL_DATA) $$i $(DESTDIR)$(install_librarydir); \
	done )
	( cd lib/$(INTERP_PATH) ; for i in $(STARTUP_FILE); do \
		$(INSTALL_DATA) $$i $(DESTDIR)$(install_librarydir); \
	done )
	@echo "Installing .lps font files"
	$(mkinstalldirs) $(DESTDIR)$(install_librarydir)/fonts
	for i in $(FONTS_LPS); do \
		$(INSTALL_DATA) lib/fonts/$$i.lps $(DESTDIR)$(install_librarydir)/fonts; \
	done
	@echo "Installing .xfe font files"
	for i in $(FONTS_XFE); do \
		$(INSTALL_DATA) lib/fonts/$$i.xfe $(DESTDIR)$(install_librarydir)/fonts; \
	done
	@echo "Installing scripts (if option enabled)"
	if test "$(PYTHON)" != ""; then \
		$(mkinstalldirs) $(DESTDIR)$(install_scriptsdir)/python; \
		for i in $(SCRIPTS_PY); do \
			$(INSTALL_DATA) lib/python/$$i.py \
				$(DESTDIR)$(install_scriptsdir)/python; \
		done; \
	fi

# Note that MacOS/Fink uses SHDLIB_EXT = .dylib but Tcl expects .so anyway.
# So we make a symbolic link if SHDLIB_EXT != .so

install-tcl: xcircexec$(EXEEXT) xcircdnull$(EXEEXT) lib/$(INTERP_PATH)/$(WRAPPER_OBJ) lib/$(INTERP_PATH)/$(WRAPPER_SH) lib/$(INTERP_PATH)/$(WRAPPER_INIT)
	@echo "Installing standard XCircuit library files"
	$(MAKE) $(AM_MAKEFLAGS) install-data-local
	@echo "Installing Tcl files"
	$(mkinstalldirs) $(DESTDIR)$(install_librarydir)
	$(mkinstalldirs) $(DESTDIR)$(install_scriptsdir)
	( cd lib/$(INTERP_PATH) ; for i in $(WRAPPER_OBJ) *.tcl ; do \
		$(INSTALL_DATA) $$i $(DESTDIR)$(install_scriptsdir); \
		done )
	$(INSTALL_DATA) xcircexec$(EXEEXT) $(DESTDIR)$(install_librarydir)
	$(INSTALL_DATA) xcircdnull$(EXEEXT) $(DESTDIR)$(install_librarydir)
	chmod 0755 $(DESTDIR)$(install_librarydir)/$(CONSOLE)
	chmod 0755 $(DESTDIR)$(install_librarydir)/$(CONSOLE_SCRIPT)
	chmod 0755 $(DESTDIR)$(install_librarydir)/$(WRAPPER_OBJ)
	chmod 0755 $(DESTDIR)$(install_librarydir)/xcircexec$(EXEEXT)
	chmod 0755 $(DESTDIR)$(install_librarydir)/xcircdnull$(EXEEXT)

	if test "${SHDLIB_EXT}" != ".so"; then \
	  ( cd $(DESTDIR)$(install_librarydir); \
	    $(RM) $(WRAPPER_SO) ; \
	    ln -s $(WRAPPER_OBJ) $(WRAPPER_SO) ) ; \
	fi

	@echo "Installing GIF images"
	$(mkinstalldirs) $(DESTDIR)$(install_librarydir)/pixmaps
	( cd lib/pixmaps ; for i in `ls *.gif *.xbm *.ico` ; do \
		$(INSTALL_DATA) $$i $(DESTDIR)$(install_librarydir)/pixmaps; \
	done )
	@echo "Installing shell script as xcircuit executable"
	$(mkinstalldirs) $(DESTDIR)$(install_bindir)
	$(INSTALL_DATA) lib/$(INTERP_PATH)/$(WRAPPER_SH) $(DESTDIR)$(install_bindir)/xcircuit
	chmod 0755 $(DESTDIR)$(install_bindir)/xcircuit

clean:
	(cd ./Xw; $(RM) *.o *.a *.bak core)
	(cd ./asg; $(MAKE) clean)
	(cd ./spiceparser; $(MAKE) clean)
	$(RM) lib/$(INTERP_PATH)/$(WRAPPER_OBJ) lib/$(INTERP_PATH)/$(WRAPPER_SH)
	$(RM) lib/$(INTERP_PATH)/$(WRAPPER_INIT)
	$(RM) xcircuit *.o *.bak lib/xcircuit.1 core
	$(RM) xcircexec$(EXEEXT) xcircdnull$(EXEEXT)
	$(RM) menudep$(EXEEXT) menudep.h
	$(RM) config.cache config.log

distclean:
	(cd ./Xw; $(RM) *.o *.a *.bak Makefile core)
	(cd ./asg; $(MAKE) clean ; $(RM) Makefile)
	(cd ./spiceparser; $(MAKE) clean ; $(RM) Makefile)
	$(RM) xcircuit *.o *.bak core
	$(RM) lib/$(INTERP_PATH)/$(WRAPPER_OBJ) lib/$(INTERP_PATH)/$(WRAPPER_SH)
	$(RM) menudep$(EXEEXT) menudep.h lib/xcircuit.1
	$(RM) xcircexec$(EXEEXT) xcircdnull$(EXEEXT)
	$(RM) Makefile config.cache config.log config.status
	$(RM) xcircuit-$(FULL_VERSION) xcircuit.spec xcircuit-$(FULL_VERSION).tgz

dist:
	$(RM) xcircuit.spec xcircuit-$(FULL_VERSION) xcircuit-$(FULL_VERSION).tgz
	sed -e /%VERSION%/s/%VERSION%/$(FULL_VERSION)/ \
	    xcircuit.spec.in > xcircuit.spec
	ln -nsf . xcircuit-$(FULL_VERSION)
	tar zchvf xcircuit-$(FULL_VERSION).tgz --exclude CVS \
	    --exclude xcircuit-$(FULL_VERSION)/xcircuit-$(FULL_VERSION) \
	    --exclude xcircuit-$(FULL_VERSION)/xcircuit-$(FULL_VERSION).tgz \
	    xcircuit-$(FULL_VERSION)

quiteclean:
	(cd ./Xw; $(RM) *.o *.a *.bak Makefile.in Makefile core)
	(cd ./asg; $(MAKE) clean ; $(RM) Makefile.in Makefile)
	(cd ./spiceparser; $(MAKE) clean ; $(RM) Makefile.in Makefile)
	$(RM) xcircuit *.o *.bak core
	$(RM) lib/$(INTERP_PATH)/$(WRAPPER_OBJ) lib/$(INTERP_PATH)/$(WRAPPER_SH)
	$(RM) menudep$(EXEEXT) menudep.h lib/xcircuit.1
	$(RM) xcircexec$(EXEEXT) xcircdnull$(EXEEXT)
	$(RM) install-sh missing mkinstalldirs Makefile.in Makefile
	$(RM) config.* aclocal.m4

$(ALL_TARGET):
	$(MAKE) tcl

$(INSTALL_TARGET):
	$(MAKE) install-tcl

