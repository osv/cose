DEFS = -DCONFIG_DATA_DIR='"$(PKGDATADIR)"' -DLOCALEDIR='"$(datadir)/locale"' @DEFS@
AM_CXXFLAGS = @CXXFLAGS@ \
	@CGAME_CXXFLAGS@ \
	@AGAR_CXXFLAGS@ 

INCLUDES = -I.. -I../celestia \
	-I$(top_srcdir)/thirdparty/Eigen \
	-I$(top_srcdir)/thirdparty/glew/include

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
	ui.h \
	videocapture.h \
	geekconsole.h \
	geekbind.h \
	ui_theme.h

openspace_client_LDADD = \
	../celestia/libcelestia.a \
	../celengine/libcelengine.a \
	../celtxf/libceltxf.a \
	../cel3ds/libcel3ds.a \
	../celmath/libcelmath.a \
	../celutil/libcelutil.a \
	$(SPICELIB) \
	$(INTLLIBS) 

openspace_client_SOURCES = \
	agarmain.cpp \
	gamecore.cpp \
	ui_main.cpp \
	ui_context.cpp \
	videocapture.cpp \
	cgame.cpp \
	geekconsole.cpp \
	geekbind.cpp \
	ui_theme.cpp

noinst_DATA = ../../openspace_client
CLEANFILES = ../../openspace_client

../../openspace_client: openspace_client
	(cd ../..; ln -s src/cgame/openspace_client)

endif

