noinst_LIBRARIES = libceltxf.a
noinst_HEADERS = \
	texturefont.h

dosstuff = celtxf.dsp txf.mak
noinst_DATA = $(dosstuff)

EXTRA_DIST = $(noinst_DATA)

libceltxf_a_SOURCES = \
	texturefont.cpp

AM_CXXFLAGS = @CXXFLAGS@\
	@CELESTIA_CXXFLAGS@

INCLUDES = -I.. -I$(top_srcdir)/thirdparty/glew/include

LIBS = @LIBS@ \
	@CELESTIA_LIBS@

