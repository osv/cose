DEFS = -DCONFIG_DATA_DIR='"$(PKGDATADIR)"' -DLOCALEDIR='"$(datadir)/locale"' @DEFS@
AM_CXXFLAGS = @CXXFLAGS@ \
	@CGAME_CXXFLAGS@ \
	@AGAR_CXXFLAGS@ 

INCLUDES = -I.. -I../celestia \
	-I$(top_srcdir)/thirdparty/Eigen \
	-I$(top_srcdir)/thirdparty/geekconsole \
	-I$(top_srcdir)/thirdparty/geekconsole/gcinfo \
	-I$(top_srcdir)/thirdparty/glew/include

AM_CFLAGS = @CFLAGS@ \
	@AGAR_CXXFLAGS@ 

LIBS = @LIBS@ \
	@CGAME_LIBS@ \
	@AGAR_LIBS@

GEEKCONSOLESRC = \
	$(top_srcdir)/thirdparty/geekconsole/geekconsole.cpp \
	$(top_srcdir)/thirdparty/geekconsole/geekbind.cpp \
	$(top_srcdir)/thirdparty/geekconsole/geeklua.cpp \
	$(top_srcdir)/thirdparty/geekconsole/gcinfo/search.c \
	$(top_srcdir)/thirdparty/geekconsole/gcinfo/nodes.c \
	$(top_srcdir)/thirdparty/geekconsole/gcinfo/info-utils.c \
	$(top_srcdir)/thirdparty/geekconsole/gcinfo/filesys.c \
	$(top_srcdir)/thirdparty/geekconsole/gcinfo/dir.c \
	$(top_srcdir)/thirdparty/geekconsole/gcinfo/tilde.c \
	$(top_srcdir)/thirdparty/geekconsole/infointer.cpp \
	$(top_srcdir)/thirdparty/geekconsole/m-x.cpp \
	$(top_srcdir)/thirdparty/geekconsole/m-x_cel.cpp \
	$(top_srcdir)/thirdparty/geekconsole/gvar.cpp

if ENABLE_CGAME
bin_PROGRAMS = openspace_client

noinst_HEADERS = \
	cgame.h\
	gamecore.h\
	ui.h \
	videocapture.h \
	configfile.h \
	ui_theme.h

openspace_client_LDADD = \
	../celestia/libcelestia.a \
	../celengine/libcelengine.a \
	../celephem/libcelephem.a \
	../celmodel/libcelmodel.a \
	../celtxf/libceltxf.a \
	../cel3ds/libcel3ds.a \
	../celmath/libcelmath.a \
	../celutil/libcelutil.a \
	../sound/libsound.a \
	$(SPICELIB) \
	$(INTLLIBS) 

# ../celephem/libcelephem.a 
# ../celmodel/libcelmodel.a \
# ../celestia/libcelestia.a \
# ../celengine/libcelengine.a \
# ../celtxf/libceltxf.a \
# ../cel3ds/libcel3ds.a \
# ../celmath/libcelmath.a \
# ../celutil/libcelutil.a 

openspace_client_SOURCES = \
	agarmain.cpp \
	gamecore.cpp \
	ui_main.cpp \
	ui_context.cpp \
	videocapture.cpp \
	cgame.cpp \
	configfile.cpp \
	$(GEEKCONSOLESRC) \
	ui_theme.cpp

noinst_DATA = ../../openspace_client
CLEANFILES = ../../openspace_client

../../openspace_client: openspace_client
	(cd ../..; ln -s src/cgame/openspace_client)

endif

