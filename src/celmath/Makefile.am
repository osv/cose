noinst_LIBRARIES = libcelmath.a
noinst_HEADERS = \
	geomutil.h \
	perlin.h \
	aabox.h \
	ray.h \
	plane.h \
	intersect.h \
	frustum.h \
	capsule.h \
	quaternion.h \
	vecmath.h \
	distance.h \
	mathlib.h \
	solve.h \
	sphere.h \
	ellipsoid.h 

dosstuff = celmath.dsp math.mak
noinst_DATA = $(dosstuff)

EXTRA_DIST = $(noinst_DATA)

AM_CXXFLAGS = @CXXFLAGS@\
	@CELESTIA_CXXFLAGS@

INCLUDES = -I.. -I$(top_srcdir)/thirdparty/Eigen

libcelmath_a_SOURCES = \
	frustum.cpp \
	perlin.cpp

LIBS = @LIBS@ \
	@CELESTIA_LIBS@

