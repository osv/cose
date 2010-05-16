noinst_LIBRARIES = libcelmodel.a
noinst_HEADERS = material.h \
	mesh.h \
	model.h \
	modelfile.h

INCLUDES = -I.. -I$(top_srcdir)/thirdparty/Eigen

libcelmodel_a_SOURCES = \
	material.cpp \
	mesh.cpp \
	model.cpp \
	modelfile.cpp

AM_CXXFLAGS = @CXXFLAGS@\
	@CELESTIA_CXXFLAGS@

