noinst_LIBRARIES = libcelephem.a

noinst_HEADERS = customorbit.h \
	customrotation.h \
	jpleph.cpp \
	jpleph.h \
	nutation.h \
	orbit.h \
	precession.h \
	rotation.h \
	samporbit.h \
	samporient.h \
	scriptobject.h \
	scriptorbit.h \
	scriptrotation.h \
	spiceinterface.h \
	spiceorbit.h \
	spicerotation.h \
	vsop87.h

if ENABLE_SPICE
SPICESOURCES = spiceinterface.cpp spiceorbit.cpp spicerotation.cpp
endif

SCRIPT_OBJ_SOURCES = scriptobject.cpp scriptorbit.cpp scriptrotation.cpp

libcelephem_a_SOURCES = \
	customorbit.cpp \
	customrotation.cpp \
	jpleph.cpp \
	nutation.cpp \
	orbit.cpp \
	precession.cpp \
	rotation.cpp \
	samporbit.cpp \
	samporient.cpp \
	vsop87.cpp \
	$(SPICESOURCES) \
	$(SCRIPT_OBJ_SOURCES)

AM_CXXFLAGS = @CELESTIA_CXXFLAGS@ \
	@CXXFLAGS@

INCLUDES = -I.. -I$(top_srcdir)/thirdparty/Eigen

LIBS = @LIBS@ \
	@CELESTIA_LIBS@
