DEFS = -DCONFIG_DATA_DIR='"$(PKGDATADIR)"' -DLOCALEDIR='"$(datadir)/locale"' @DEFS@
INCLUDES = -I.. -I../celestia

AM_CXXFLAGS = @CXXFLAGS@ \
	@CGAME_CXXFLAGS@ \
	@AGAR_CXXFLAGS@ 

AM_CFLAGS = @CFLAGS@ \
	@AGAR_CXXFLAGS@ 

LIBS = @LIBS@ \
	@CGAME_LIBS@ \
	@AGAR_LIBS@


if ENABLE_CGAME
bin_PROGRAMS = openspace_client

noinst_HEADERS = \
	cgame.h\
	gamecore.h\
	gkey.h \
	ui.h \
	videocapture.h \
	geekconsole.h

openspace_client_LDADD = \
	../celestia/libcelestia.a \
	../celengine/libcelengine.a \
	../celtxf/libceltxf.a \
	../cel3ds/libcel3ds.a \
	../celmath/libcelmath.a \
	../celutil/libcelutil.a \
	$(SPICELIB) \
	$(INTLLIBS) 

openspace_client_SOURCES = gkey.c \
	agarmain.cpp \
	gamecore.cpp \
	ui_main.cpp \
	ui_context.cpp \
	videocapture.cpp \
	cgame.cpp \
	geekconsole.cpp

noinst_DATA = ../../openspace_client
CLEANFILES = ../../openspace_client

../../openspace_client: openspace_client
	(cd ../..; ln -s src/cgame/openspace_client)

endif

