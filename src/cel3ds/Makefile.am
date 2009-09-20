noinst_LIBRARIES = libcel3ds.a
noinst_HEADERS = \
	3dsread.h \
	3dsmodel.h \
	3dschunk.h 

AM_CXXFLAGS = @CXXFLAGS@\
	@CELESTIA_CXXFLAGS@

LIBS = @LIBS@ \
	@CELESTIA_LIBS@

dosstuff = 3ds.mak cel3ds.dsp
noinst_DATA = $(dosstuff)

EXTRA_DIST = $(noinst_DATA)

INCLUDES = -I.. -I$(top_srcdir)/thirdparty/Eigen

libcel3ds_a_SOURCES = \
	3dsmodel.cpp \
	3dsread.cpp

