noinst_LIBRARIES = libcelmath.a
noinst_HEADERS = \
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

INCLUDES = -I..

libcelmath_a_SOURCES = \
	frustum.cpp \
	perlin.cpp

AM_CXXFLAGS = @CXXFLAGS@\
	@CELESTIA_CXXFLAGS@

LIBS = @LIBS@ \
	@CELESTIA_LIBS@

