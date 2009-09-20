noinst_LIBRARIES = libcelutil.a
noinst_HEADERS = \
	debug.h \
	basictypes.h \
	bigfix.h \
	bytes.h \
	directory.h \
	memorypool.h \
	watcher.h \
	resmanager.h \
	util.h \
	utf8.h \
	filetype.h \
	color.h \
	timer.h \
	winutil.h \
	formatnum.h \
	reshandle.h 

dosstuff = util.mak celutil.dsp
noinst_DATA = $(dosstuff)

libcelutil_a_SOURCES = \
	bigfix.cpp \
	color.cpp \
	debug.cpp \
	directory.cpp \
	filetype.cpp \
	formatnum.cpp \
	utf8.cpp \
	util.cpp \
	unixdirectory.cpp \
	unixtimer.cpp

WINSOURCES = \
	wintimer.cpp \
	winutil.cpp \
        windirectory.cpp

EXTRA_DIST = $(noinst_DATA)

EXTRA_libcelutil_a_SOURCES= $(WINSOURCES)

AM_CXXFLAGS = @CXXFLAGS@\
	@CELESTIA_CXXFLAGS@

INCLUDES = -I$(top_srcdir)/thirdparty/Eigen

LIBS = @LIBS@ \
	@CELESTIA_LIBS@

