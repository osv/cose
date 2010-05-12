DEFS = -DCONFIG_DATA_DIR='"$(PKGDATADIR)"' -DLOCALEDIR='"$(datadir)/locale"' @DEFS@

AM_CXXFLAGS = @CXXFLAGS@\
	@CGAME_CXXFLAGS@

INCLUDES = -I.. -I$(top_srcdir)/thirdparty/Eigen -I$(top_srcdir)/thirdparty/glew/include

LIBS = @LIBS@ \
	@CGAME_LIBS@

if ENABLE_CGAME

#bin_PROGRAMS = celestia
noinst_LIBRARIES = libcelestia.a


COMMONSOURCES = \
	configfile.cpp \
	destination.cpp \
	eclipsefinder.cpp\
	favorites.cpp \
	imagecapture.cpp \
	url.cpp \
	celestiacore.cpp 	

if ENABLE_CELX
CELXSOURCES = \
	celx.cpp \
	celx_celestia.cpp \
	celx_frame.cpp \
	celx_gl.cpp \
	celx_object.cpp \
	celx_observer.cpp \
	celx_phase.cpp \
	celx_position.cpp \
	celx_rotation.cpp \
	celx_vector.cpp
        
endif

THEORASOURCES = oggtheoracapture.cpp

# celestia_SOURCES = $(COMMONSOURCES) $(CELXSOURCES) $(GLUTSOURCES) $(THEORASOURCES)

EXTRA_DIST = \
	Celestia.dsp \
	celestia.mak

# celestia_LDADD = \
# 	../celengine/libcelengine.a \
# 	../celtxf/libceltxf.a \
# 	../cel3ds/libcel3ds.a \
# 	../celmath/libcelmath.a \
# 	../celutil/libcelutil.a \
# 	$(SPICELIB) \
# 	$(INTLLIBS)

# libcelestia_a_LIBADD = \
# 	../celengine/libcelengine.a \
# 	../celtxf/libceltxf.a \
# 	../cel3ds/libcel3ds.a \
# 	../celmath/libcelmath.a \
# 	../celutil/libcelutil.a \
# 	$(SPICELIB)

noinst_HEADERS = \
	celx_phase.h \
	celx_frame.h \
	configfile.h \
	url.h \
	celx_gl.h \
	avicapture.h \
	celx.h \
	moviecapture.h \
	scriptmenu.h \
	celx_object.h \
	celx_position.h \
	favorites.h \
	celx_internal.h \
	oggtheoracapture.h \
	celx_vector.h \
	eclipsefinder.h \
	celx_rotation.h \
	celx_observer.h \
	celestiacore.h \
	destination.h \
	imagecapture.h \
	celx_celestia.h 

#noinst_DATA = ../../celestia
#CLEANFILES = ../../celestia

libcelestia_a_SOURCES = $(COMMONSOURCES) $(CELXSOURCES) $(THEORASOURCES)

#../../celestia: celestia
#	(cd ../..; ln -s src/celestia/celestia)
endif
